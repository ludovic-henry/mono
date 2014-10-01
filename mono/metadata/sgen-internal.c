/*
 * sgen-internal.c: Internal lock-free memory allocator.
 *
 * Copyright (C) 2012 Xamarin Inc
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License 2.0 as published by the Free Software Foundation;
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License 2.0 along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * This is a simplified version of the lock-free allocator described in
 *
 * Scalable Lock-Free Dynamic Memory Allocation
 * Maged M. Michael, PLDI 2004
 *
 * I could not get Michael's allocator working bug free under heavy
 * stress tests.  The paper doesn't provide correctness proof and after
 * failing to formalize the ownership of descriptors I devised this
 * simpler allocator.
 *
 * Allocation within superblocks proceeds exactly like in Michael's
 * allocator.  The simplification is that a thread has to "acquire" a
 * descriptor before it can allocate from its superblock.  While it owns
 * the descriptor no other thread can acquire and hence allocate from
 * it.  A consequence of this is that the ABA problem cannot occur, so
 * we don't need the tag field and don't have to use 64 bit CAS.
 *
 * Descriptors are stored in two locations: The partial queue and the
 * active field.  They can only be in at most one of those at one time.
 * If a thread wants to allocate, it needs to get a descriptor.  It
 * tries the active descriptor first, CASing it to NULL.  If that
 * doesn't work, it gets a descriptor out of the partial queue.  Once it
 * has the descriptor it owns it because it is not referenced anymore.
 * It allocates a slot and then gives the descriptor back (unless it is
 * FULL).
 *
 * Note that it is still possible that a slot is freed while an
 * allocation is in progress from the same superblock.  Ownership in
 * this case is not complicated, though.  If the block was FULL and the
 * free set it to PARTIAL, the free now owns the block (because FULL
 * blocks are not referenced from partial and active) and has to give it
 * back.  If the block was PARTIAL then the free doesn't own the block
 * (because it's either still referenced, or an alloc owns it).  A
 * special case of this is that it has changed from PARTIAL to EMPTY and
 * now needs to be retired.  Technically, the free wouldn't have to do
 * anything in this case because the first thing an alloc does when it
 * gets ownership of a descriptor is to check whether it is EMPTY and
 * retire it if that is the case.  As an optimization, our free does try
 * to acquire the descriptor (by CASing the active field, which, if it
 * is lucky, points to that descriptor) and if it can do so, retire it.
 * If it can't, it tries to retire other descriptors from the partial
 * queue, so that we can be sure that even if no more allocations
 * happen, descriptors are still retired.  This is analogous to what
 * Michael's allocator does.
 *
 * Another difference to Michael's allocator is not related to
 * concurrency, however: We don't point from slots to descriptors.
 * Instead we allocate superblocks aligned and point from the start of
 * the superblock to the descriptor, so we only need one word of
 * metadata per superblock.
 *
 * FIXME: Having more than one allocator per size class is probably
 * buggy because it was never tested.
 */

#include "config.h"

#ifdef HAVE_SGEN_GC

#include "utils/atomic.h"
#include "utils/lock-free-queue.h"
#include "utils/hazard-pointer.h"
#include "utils/mono-counters.h"
#include "utils/mono-mmap.h"
#include "utils/mono-membar.h"
#include "metadata/sgen-gc.h"
#include "metadata/sgen-memory-governor.h"

/* keep each size a multiple of ALLOC_ALIGN */
#if SIZEOF_VOID_P == 4
static const int allocator_sizes [] = {
	   8,   16,   24,   32,   40,   48,   64,   80,
	  96,  128,  160,  192,  224,  248,  296,  320,
	 384,  448,  504,  528,  584,  680,  816, 1088,
	1360, 2040, 2336, 2728, 3272, 4088, 5456, 8184 };
#else
static const int allocator_sizes [] = {
	   8,   16,   24,   32,   40,   48,   64,   80,
	  96,  128,  160,  192,  224,  248,  320,  328,
	 384,  448,  528,  584,  680,  816, 1016, 1088,
	1360, 2040, 2336, 2728, 3272, 4088, 5456, 8184 };
#endif

#define NUM_ALLOCATORS	(sizeof (allocator_sizes) / sizeof (int))

#define SB_MAX_SIZE					16384
#define SB_HEADER_SIZE				(sizeof (MonoLockFreeAllocator))
#define SB_USABLE_SIZE(block_size)	((block_size) - SB_HEADER_SIZE)

enum {
	STATE_FULL,
	STATE_PARTIAL,
	STATE_EMPTY
};

typedef union {
	gint32 value;
	struct {
		guint32 avail : 15;
		guint32 count : 15;
		guint32 state : 2;
	} data;
} Anchor;

typedef struct {
	MonoLockFreeQueue partial;
	unsigned int slot_size;
	unsigned int block_size;
} MonoLockFreeAllocSizeClass;

struct _MonoLockFreeAllocDescriptor;

typedef struct {
	struct _MonoLockFreeAllocDescriptor *active;
	MonoLockFreeAllocSizeClass *sc;
} MonoLockFreeAllocator;

typedef struct _MonoLockFreeAllocDescriptor Descriptor;
struct _MonoLockFreeAllocDescriptor {
	MonoLockFreeQueueNode node;
	MonoLockFreeAllocator *heap;
	volatile Anchor anchor;
	unsigned int slot_size;
	unsigned int block_size;
	unsigned int max_count;
	gpointer sb;
#ifndef DESC_AVAIL_DUMMY
	Descriptor * volatile next;
#endif
	gboolean in_use;	/* used for debugging only */
};

static int allocator_block_sizes [NUM_ALLOCATORS];

static MonoLockFreeAllocSizeClass size_classes [NUM_ALLOCATORS];
static MonoLockFreeAllocator allocators [NUM_ALLOCATORS];

#ifdef HEAVY_STATISTICS
static int allocator_sizes_stats [NUM_ALLOCATORS];
#endif

/*
 * Allocator indexes for the fixed INTERNAL_MEM_XXX types.  -1 if that
 * type is dynamic.
 */
static int fixed_type_allocator_indexes [INTERNAL_MEM_MAX];

static size_t
block_size (size_t slot_size)
{
	static int pagesize = -1;
	if (pagesize == -1)
		pagesize = mono_pagesize ();

	int size;
	for (size = pagesize; size < SB_MAX_SIZE; size <<= 1) {
		if (slot_size * 2 <= SB_USABLE_SIZE (size))
			return size;
	}
	return SB_MAX_SIZE;
}

/*
 * Find the allocator index for memory chunks that can contain @size
 * objects.
 */
static int
index_for_size (size_t size)
{
	int slot;
	/* do a binary search or lookup table later. */
	for (slot = 0; slot < NUM_ALLOCATORS; ++slot) {
		if (allocator_sizes [slot] >= size)
			return slot;
	}
	g_assert_not_reached ();
	return -1;
}

static const char*
description_for_type (int type)
{
	switch (type) {
	case INTERNAL_MEM_PIN_QUEUE: return "pin-queue";
	case INTERNAL_MEM_FRAGMENT: return "fragment";
	case INTERNAL_MEM_SECTION: return "section";
	case INTERNAL_MEM_SCAN_STARTS: return "scan-starts";
	case INTERNAL_MEM_FIN_TABLE: return "fin-table";
	case INTERNAL_MEM_FINALIZE_ENTRY: return "finalize-entry";
	case INTERNAL_MEM_FINALIZE_READY_ENTRY: return "finalize-ready-entry";
	case INTERNAL_MEM_DISLINK_TABLE: return "dislink-table";
	case INTERNAL_MEM_DISLINK: return "dislink";
	case INTERNAL_MEM_ROOTS_TABLE: return "roots-table";
	case INTERNAL_MEM_ROOT_RECORD: return "root-record";
	case INTERNAL_MEM_STATISTICS: return "statistics";
	case INTERNAL_MEM_STAT_PINNED_CLASS: return "pinned-class";
	case INTERNAL_MEM_STAT_REMSET_CLASS: return "remset-class";
	case INTERNAL_MEM_GRAY_QUEUE: return "gray-queue";
	case INTERNAL_MEM_MS_TABLES: return "marksweep-tables";
	case INTERNAL_MEM_MS_BLOCK_INFO: return "marksweep-block-info";
	case INTERNAL_MEM_MS_BLOCK_INFO_SORT: return "marksweep-block-info-sort";
	case INTERNAL_MEM_EPHEMERON_LINK: return "ephemeron-link";
	case INTERNAL_MEM_WORKER_DATA: return "worker-data";
	case INTERNAL_MEM_WORKER_JOB_DATA: return "worker-job-data";
	case INTERNAL_MEM_BRIDGE_DATA: return "bridge-data";
	case INTERNAL_MEM_OLD_BRIDGE_HASH_TABLE: return "old-bridge-hash-table";
	case INTERNAL_MEM_OLD_BRIDGE_HASH_TABLE_ENTRY: return "old-bridge-hash-table-entry";
	case INTERNAL_MEM_BRIDGE_HASH_TABLE: return "bridge-hash-table";
	case INTERNAL_MEM_BRIDGE_HASH_TABLE_ENTRY: return "bridge-hash-table-entry";
	case INTERNAL_MEM_TARJAN_BRIDGE_HASH_TABLE: return "tarjan-bridge-hash-table";
	case INTERNAL_MEM_TARJAN_BRIDGE_HASH_TABLE_ENTRY: return "tarjan-bridge-hash-table-entry";
	case INTERNAL_MEM_TARJAN_OBJ_BUCKET: return "tarjan-bridge-object-buckets";
	case INTERNAL_MEM_BRIDGE_ALIVE_HASH_TABLE: return "bridge-alive-hash-table";
	case INTERNAL_MEM_BRIDGE_ALIVE_HASH_TABLE_ENTRY: return "bridge-alive-hash-table-entry";
	case INTERNAL_MEM_BRIDGE_DEBUG: return "bridge-debug";
	case INTERNAL_MEM_JOB_QUEUE_ENTRY: return "job-queue-entry";
	case INTERNAL_MEM_TOGGLEREF_DATA: return "toggleref-data";
	case INTERNAL_MEM_CARDTABLE_MOD_UNION: return "cardtable-mod-union";
	case INTERNAL_MEM_BINARY_PROTOCOL: return "binary-protocol";
	default:
		g_assert_not_reached ();
	}
}

//#define DESC_AVAIL_DUMMY

#define NUM_DESC_BATCH	64

static MONO_ALWAYS_INLINE gpointer
sb_header_for_addr (gpointer addr, size_t block_size)
{
	return (gpointer)(((size_t)addr) & (~(block_size - 1)));
}

/* Taken from SGen */

static unsigned long
prot_flags_for_activate (int activate)
{
	unsigned long prot_flags = activate? MONO_MMAP_READ|MONO_MMAP_WRITE: MONO_MMAP_NONE;
	return prot_flags | MONO_MMAP_PRIVATE | MONO_MMAP_ANON;
}

static gpointer
alloc_sb (Descriptor *desc)
{
	static int pagesize = -1;
	if (pagesize == -1)
		pagesize = mono_pagesize ();

	gpointer sb_header = desc->block_size == pagesize ?
		mono_valloc (0, desc->block_size, prot_flags_for_activate (TRUE)) :
		mono_valloc_aligned (desc->block_size, desc->block_size, prot_flags_for_activate (TRUE));

	g_assert (sb_header == sb_header_for_addr (sb_header, desc->block_size));

	*(Descriptor**)sb_header = desc;
	//g_print ("sb %p for %p\n", sb_header, desc);

	return (char*)sb_header + SB_HEADER_SIZE;
}

static void
free_sb (gpointer sb, size_t block_size)
{
	gpointer sb_header = sb_header_for_addr (sb, block_size);
	g_assert ((char*)sb_header + SB_HEADER_SIZE == sb);
	mono_vfree (sb_header, block_size);
	//g_print ("free sb %p\n", sb_header);
}

#ifndef DESC_AVAIL_DUMMY
static Descriptor * volatile desc_avail;

static Descriptor*
desc_alloc (void)
{
	MonoThreadHazardPointers *hp = mono_hazard_pointer_get ();
	Descriptor *desc;

	for (;;) {
		gboolean success;

		desc = get_hazardous_pointer ((gpointer * volatile)&desc_avail, hp, 1);
		if (desc) {
			Descriptor *next = desc->next;
			success = (InterlockedCompareExchangePointer ((gpointer * volatile)&desc_avail, next, desc) == desc);
		} else {
			size_t desc_size = sizeof (Descriptor);
			Descriptor *d;
			int i;

			desc = mono_valloc (0, desc_size * NUM_DESC_BATCH, prot_flags_for_activate (TRUE));

			/* Organize into linked list. */
			d = desc;
			for (i = 0; i < NUM_DESC_BATCH; ++i) {
				Descriptor *next = (i == (NUM_DESC_BATCH - 1)) ? NULL : (Descriptor*)((char*)desc + ((i + 1) * desc_size));
				d->next = next;
				mono_lock_free_queue_node_init (&d->node, TRUE);
				d = next;
			}

			mono_memory_write_barrier ();

			success = (InterlockedCompareExchangePointer ((gpointer * volatile)&desc_avail, desc->next, NULL) == NULL);

			if (!success)
				mono_vfree (desc, desc_size * NUM_DESC_BATCH);
		}

		mono_hazard_pointer_clear (hp, 1);

		if (success)
			break;
	}

	g_assert (!desc->in_use);
	desc->in_use = TRUE;

	return desc;
}

static void
desc_enqueue_avail (gpointer _desc)
{
	Descriptor *desc = _desc;
	Descriptor *old_head;

	g_assert (desc->anchor.data.state == STATE_EMPTY);
	g_assert (!desc->in_use);

	do {
		old_head = desc_avail;
		desc->next = old_head;
		mono_memory_write_barrier ();
	} while (InterlockedCompareExchangePointer ((gpointer * volatile)&desc_avail, desc, old_head) != old_head);
}

static void
desc_retire (Descriptor *desc)
{
	g_assert (desc->anchor.data.state == STATE_EMPTY);
	g_assert (desc->in_use);
	desc->in_use = FALSE;
	free_sb (desc->sb, desc->block_size);
	mono_thread_hazardous_free_or_queue (desc, desc_enqueue_avail, FALSE, TRUE);
}
#else
MonoLockFreeQueue available_descs;

static Descriptor*
desc_alloc (void)
{
	Descriptor *desc = (Descriptor*)mono_lock_free_queue_dequeue (&available_descs);

	if (desc)
		return desc;

	return calloc (1, sizeof (Descriptor));
}

static void
desc_retire (Descriptor *desc)
{
	free_sb (desc->sb, desc->block_size);
	mono_lock_free_queue_enqueue (&available_descs, &desc->node);
}
#endif

static Descriptor*
list_get_partial (MonoLockFreeAllocSizeClass *sc)
{
	for (;;) {
		Descriptor *desc = (Descriptor*) mono_lock_free_queue_dequeue (&sc->partial);
		if (!desc)
			return NULL;
		if (desc->anchor.data.state != STATE_EMPTY)
			return desc;
		desc_retire (desc);
	}
}

static void
desc_put_partial (gpointer _desc)
{
	Descriptor *desc = _desc;

	g_assert (desc->anchor.data.state != STATE_FULL);

	mono_lock_free_queue_node_free (&desc->node);
	mono_lock_free_queue_enqueue (&desc->heap->sc->partial, &desc->node);
}

static void
list_put_partial (Descriptor *desc)
{
	g_assert (desc->anchor.data.state != STATE_FULL);
	mono_thread_hazardous_free_or_queue (desc, desc_put_partial, FALSE, TRUE);
}

static void
list_remove_empty_desc (MonoLockFreeAllocSizeClass *sc)
{
	int num_non_empty = 0;
	for (;;) {
		Descriptor *desc = (Descriptor*) mono_lock_free_queue_dequeue (&sc->partial);
		if (!desc)
			return;
		/*
		 * We don't need to read atomically because we're the
		 * only thread that references this descriptor.
		 */
		if (desc->anchor.data.state == STATE_EMPTY) {
			desc_retire (desc);
		} else {
			g_assert (desc->heap->sc == sc);
			mono_thread_hazardous_free_or_queue (desc, desc_put_partial, FALSE, TRUE);
			if (++num_non_empty >= 2)
				return;
		}
	}
}

static Descriptor*
heap_get_partial (MonoLockFreeAllocator *heap)
{
	return list_get_partial (heap->sc);
}

static void
heap_put_partial (Descriptor *desc)
{
	list_put_partial (desc);
}

static gboolean
set_anchor (Descriptor *desc, Anchor old_anchor, Anchor new_anchor)
{
	if (old_anchor.data.state == STATE_EMPTY)
		g_assert (new_anchor.data.state == STATE_EMPTY);

	return InterlockedCompareExchange (&desc->anchor.value, new_anchor.value, old_anchor.value) == old_anchor.value;
}

static gpointer
alloc_from_active_or_partial (MonoLockFreeAllocator *heap)
{
	Descriptor *desc;
	Anchor old_anchor, new_anchor;
	gpointer addr;

 retry:
	desc = heap->active;
	if (desc) {
		if (InterlockedCompareExchangePointer ((gpointer * volatile)&heap->active, NULL, desc) != desc)
			goto retry;
	} else {
		desc = heap_get_partial (heap);
		if (!desc)
			return NULL;
	}

	/* Now we own the desc. */

	do {
		unsigned int next;

		new_anchor = old_anchor = *(volatile Anchor*)&desc->anchor.value;
		if (old_anchor.data.state == STATE_EMPTY) {
			/* We must free it because we own it. */
			desc_retire (desc);
			goto retry;
		}
		g_assert (old_anchor.data.state == STATE_PARTIAL);
		g_assert (old_anchor.data.count > 0);

		addr = (char*)desc->sb + old_anchor.data.avail * desc->slot_size;

		mono_memory_read_barrier ();

		next = *(unsigned int*)addr;
		g_assert (next < SB_USABLE_SIZE (desc->block_size) / desc->slot_size);

		new_anchor.data.avail = next;
		--new_anchor.data.count;

		if (new_anchor.data.count == 0)
			new_anchor.data.state = STATE_FULL;
	} while (!set_anchor (desc, old_anchor, new_anchor));

	/* If the desc is partial we have to give it back. */
	if (new_anchor.data.state == STATE_PARTIAL) {
		if (InterlockedCompareExchangePointer ((gpointer * volatile)&heap->active, desc, NULL) != NULL)
			heap_put_partial (desc);
	}

	return addr;
}

static gpointer
alloc_from_new_sb (MonoLockFreeAllocator *heap)
{
	unsigned int slot_size, block_size, count, i;
	Descriptor *desc = desc_alloc ();

	slot_size = desc->slot_size = heap->sc->slot_size;
	block_size = desc->block_size = heap->sc->block_size;
	count = SB_USABLE_SIZE (block_size) / slot_size;

	desc->heap = heap;
	/*
	 * Setting avail to 1 because 0 is the block we're allocating
	 * right away.
	 */
	desc->anchor.data.avail = 1;
	desc->slot_size = heap->sc->slot_size;
	desc->max_count = count;

	desc->anchor.data.count = desc->max_count - 1;
	desc->anchor.data.state = STATE_PARTIAL;

	desc->sb = alloc_sb (desc);

	/* Organize blocks into linked list. */
	for (i = 1; i < count - 1; ++i)
		*(unsigned int*)((char*)desc->sb + i * slot_size) = i + 1;

	mono_memory_write_barrier ();

	/* Make it active or free it again. */
	if (InterlockedCompareExchangePointer ((gpointer * volatile)&heap->active, desc, NULL) == NULL) {
		return desc->sb;
	} else {
		desc->anchor.data.state = STATE_EMPTY;
		desc_retire (desc);
		return NULL;
	}
}

static gpointer
allocator_alloc (MonoLockFreeAllocator *heap)
{
	gpointer addr;

	for (;;) {

		addr = alloc_from_active_or_partial (heap);
		if (addr)
			break;

		addr = alloc_from_new_sb (heap);
		if (addr)
			break;
	}

	return addr;
}

static void
allocator_free (gpointer ptr, size_t block_size)
{
	Anchor old_anchor, new_anchor;
	Descriptor *desc;
	gpointer sb;
	MonoLockFreeAllocator *heap = NULL;

	desc = *(Descriptor**) sb_header_for_addr (ptr, block_size);
	g_assert (block_size == desc->block_size);

	sb = desc->sb;

	do {
		new_anchor = old_anchor = *(volatile Anchor*)&desc->anchor.value;
		*(unsigned int*)ptr = old_anchor.data.avail;
		new_anchor.data.avail = ((char*)ptr - (char*)sb) / desc->slot_size;
		g_assert (new_anchor.data.avail < SB_USABLE_SIZE (block_size) / desc->slot_size);

		if (old_anchor.data.state == STATE_FULL)
			new_anchor.data.state = STATE_PARTIAL;

		if (++new_anchor.data.count == desc->max_count) {
			heap = desc->heap;
			new_anchor.data.state = STATE_EMPTY;
		}
	} while (!set_anchor (desc, old_anchor, new_anchor));

	if (new_anchor.data.state == STATE_EMPTY) {
		g_assert (old_anchor.data.state != STATE_EMPTY);

		if (InterlockedCompareExchangePointer ((gpointer * volatile)&heap->active, NULL, desc) == desc) {
			/* We own it, so we free it. */
			desc_retire (desc);
		} else {
			/*
			 * Somebody else must free it, so we do some
			 * freeing for others.
			 */
			list_remove_empty_desc (heap->sc);
		}
	} else if (old_anchor.data.state == STATE_FULL) {
		/*
		 * Nobody owned it, now we do, so we need to give it
		 * back.
		 */

		g_assert (new_anchor.data.state == STATE_PARTIAL);

		if (InterlockedCompareExchangePointer ((gpointer * volatile)&desc->heap->active, desc, NULL) != NULL)
			heap_put_partial (desc);
	}
}

#define g_assert_OR_PRINT(c, format, ...)	do {				\
		if (!(c)) {						\
			if (print)					\
				g_print ((format), ## __VA_ARGS__);	\
			else						\
				g_assert (FALSE);				\
		}							\
	} while (0)

static void
descriptor_check_consistency (Descriptor *desc, gboolean print)
{
	int count = desc->anchor.data.count;
	int max_count = SB_USABLE_SIZE (desc->block_size) / desc->slot_size;
#if _MSC_VER
	gboolean* linked = alloca(max_count*sizeof(gboolean));
#else
	gboolean linked [max_count];
#endif
	int i, last;
	unsigned int index;

#ifndef DESC_AVAIL_DUMMY
	Descriptor *avail;

	for (avail = desc_avail; avail; avail = avail->next)
		g_assert_OR_PRINT (desc != avail, "descriptor is in the available list\n");
#endif

	g_assert_OR_PRINT (desc->slot_size == desc->heap->sc->slot_size, "slot size doesn't match size class\n");

	if (print)
		g_print ("descriptor %p is ", desc);

	switch (desc->anchor.data.state) {
	case STATE_FULL:
		if (print)
			g_print ("full\n");
		g_assert_OR_PRINT (count == 0, "count is not zero: %d\n", count);
		break;
	case STATE_PARTIAL:
		if (print)
			g_print ("partial\n");
		g_assert_OR_PRINT (count < max_count, "count too high: is %d but must be below %d\n", count, max_count);
		break;
	case STATE_EMPTY:
		if (print)
			g_print ("empty\n");
		g_assert_OR_PRINT (count == max_count, "count is wrong: is %d but should be %d\n", count, max_count);
		break;
	default:
		g_assert_OR_PRINT (FALSE, "invalid state\n");
	}

	for (i = 0; i < max_count; ++i)
		linked [i] = FALSE;

	index = desc->anchor.data.avail;
	last = -1;
	for (i = 0; i < count; ++i) {
		gpointer addr = (char*)desc->sb + index * desc->slot_size;
		g_assert_OR_PRINT (index >= 0 && index < max_count,
				"index %d for %dth available slot, linked from %d, not in range [0 .. %d)\n",
				index, i, last, max_count);
		g_assert_OR_PRINT (!linked [index], "%dth available slot %d linked twice\n", i, index);
		if (linked [index])
			break;
		linked [index] = TRUE;
		last = index;
		index = *(unsigned int*)addr;
	}
}

static gboolean
allocator_check_consistency (MonoLockFreeAllocator *heap)
{
	Descriptor *active = heap->active;
	Descriptor *desc;
	if (active) {
		g_assert (active->anchor.data.state == STATE_PARTIAL);
		descriptor_check_consistency (active, FALSE);
	}
	while ((desc = (Descriptor*)mono_lock_free_queue_dequeue (&heap->sc->partial))) {
		g_assert (desc->anchor.data.state == STATE_PARTIAL || desc->anchor.data.state == STATE_EMPTY);
		descriptor_check_consistency (desc, FALSE);
	}
	return TRUE;
}

void
sgen_register_fixed_internal_mem_type (int type, size_t size)
{
	int slot;

	g_assert (type >= 0 && type < INTERNAL_MEM_MAX);
	g_assert (size <= allocator_sizes [NUM_ALLOCATORS - 1]);

	slot = index_for_size (size);
	g_assert (slot >= 0);

	if (fixed_type_allocator_indexes [type] == -1)
		fixed_type_allocator_indexes [type] = slot;
	else
		g_assert (fixed_type_allocator_indexes [type] == slot);
}

void*
sgen_alloc_internal_dynamic (size_t size, int type, gboolean assert_on_failure)
{
	int index;
	void *p;

	if (size > allocator_sizes [NUM_ALLOCATORS - 1]) {
		p = sgen_alloc_os_memory (size, SGEN_ALLOC_INTERNAL | SGEN_ALLOC_ACTIVATE, NULL);
		if (!p)
			sgen_assert_memory_alloc (NULL, size, description_for_type (type));
	} else {
		index = index_for_size (size);

#ifdef HEAVY_STATISTICS
		++ allocator_sizes_stats [index];
#endif

		p = allocator_alloc (&allocators [index]);
		if (!p)
			sgen_assert_memory_alloc (NULL, size, description_for_type (type));
		memset (p, 0, size);
	}

	MONO_GC_INTERNAL_ALLOC ((mword)p, size, type);
	return p;
}

void
sgen_free_internal_dynamic (void *addr, size_t size, int type)
{
	if (!addr)
		return;

	if (size > allocator_sizes [NUM_ALLOCATORS - 1])
		sgen_free_os_memory (addr, size, SGEN_ALLOC_INTERNAL);
	else
		allocator_free (addr, block_size (size));

	MONO_GC_INTERNAL_DEALLOC ((mword)addr, size, type);
}

void*
sgen_alloc_internal (int type)
{
	int index, size;
	void *p;

	index = fixed_type_allocator_indexes [type];
	g_assert (index >= 0 && index < NUM_ALLOCATORS);

#ifdef HEAVY_STATISTICS
	++ allocator_sizes_stats [index];
#endif

	size = allocator_sizes [index];

	p = allocator_alloc (&allocators [index]);
	memset (p, 0, size);

	MONO_GC_INTERNAL_ALLOC ((mword)p, size, type);

	return p;
}

void
sgen_free_internal (void *addr, int type)
{
	int index;

	if (!addr)
		return;

	index = fixed_type_allocator_indexes [type];
	g_assert (index >= 0 && index < NUM_ALLOCATORS);

	allocator_free (addr, allocator_block_sizes [index]);

	if (MONO_GC_INTERNAL_DEALLOC_ENABLED ()) {
		int size G_GNUC_UNUSED = allocator_sizes [index];
		MONO_GC_INTERNAL_DEALLOC ((mword)addr, size, type);
	}
}

void
sgen_dump_internal_mem_usage (FILE *heap_dump_file)
{
	/*
	int i;

	fprintf (heap_dump_file, "<other-mem-usage type=\"large-internal\" size=\"%lld\"/>\n", large_internal_bytes_alloced);
	fprintf (heap_dump_file, "<other-mem-usage type=\"pinned-chunks\" size=\"%lld\"/>\n", pinned_chunk_bytes_alloced);
	for (i = 0; i < INTERNAL_MEM_MAX; ++i) {
		fprintf (heap_dump_file, "<other-mem-usage type=\"%s\" size=\"%ld\"/>\n",
				description_for_type (i), unmanaged_allocator.small_internal_mem_bytes [i]);
	}
	*/
}

void
sgen_report_internal_mem_usage (void)
{
	int i G_GNUC_UNUSED;
#ifdef HEAVY_STATISTICS
	printf ("size -> # allocations\n");
	for (i = 0; i < NUM_ALLOCATORS; ++i)
		printf ("%d -> %d\n", allocator_sizes [i], allocator_sizes_stats [i]);
#endif
}

void
sgen_init_internal_allocator (void)
{
	int i;

	for (i = 0; i < INTERNAL_MEM_MAX; ++i)
		fixed_type_allocator_indexes [i] = -1;

	for (i = 0; i < NUM_ALLOCATORS; ++i) {
		allocator_block_sizes [i] = block_size (allocator_sizes [i]);

		mono_lock_free_queue_init (&size_classes [i].partial);
		size_classes [i].slot_size = allocator_sizes [i];
		size_classes [i].block_size = allocator_block_sizes [i];

		allocators [i].sc = &size_classes [i];
		allocators [i].active = NULL;
	}
}

#endif
