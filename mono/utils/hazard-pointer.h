/*
 * hazard-pointer.h: Hazard pointer related code.
 *
 * (C) Copyright 2011 Novell, Inc
 */
#ifndef __MONO_HAZARD_POINTER_H__
#define __MONO_HAZARD_POINTER_H__

#include <glib.h>
#include <mono/utils/mono-compiler.h>
#include <mono/utils/mono-membar.h>

#define HAZARD_POINTER_COUNT 3

typedef struct {
	gpointer hazard_pointers [HAZARD_POINTER_COUNT];
} MonoThreadHazardPointers;

typedef void (*MonoHazardousFreeFunc) (gpointer p);

void mono_thread_hazardous_free_or_queue (gpointer p, MonoHazardousFreeFunc free_func,
		gboolean free_func_might_lock, gboolean lock_free_context);
void mono_thread_hazardous_try_free_all (void);
void mono_thread_hazardous_try_free_some (void);
MonoThreadHazardPointers* mono_hazard_pointer_get (void);
gpointer get_hazardous_pointer (gpointer volatile *pp, MonoThreadHazardPointers *hp, int hazard_index);

static inline void
mono_hazard_pointer_set (MonoThreadHazardPointers* hp, int index, gpointer value)
{
	g_assert (index >= 0);
	g_assert (index < HAZARD_POINTER_COUNT);

	hp->hazard_pointers [index] = value;
	mono_memory_write_barrier ();
}

static inline gpointer
mono_hazard_pointer_get_val (MonoThreadHazardPointers* hp, int index)
{
	return hp->hazard_pointers [index];
}

static inline void
mono_hazard_pointer_clear (MonoThreadHazardPointers* hp, int index)
{
	g_assert (index >= 0);
	g_assert (index < HAZARD_POINTER_COUNT);

	hp->hazard_pointers [index] = NULL;
	mono_memory_write_barrier ();
}

void mono_thread_small_id_free (int id);
int mono_thread_small_id_alloc (void);

int mono_hazard_pointer_save_for_signal_handler (void);
void mono_hazard_pointer_restore_for_signal_handler (int small_id);

void mono_thread_smr_init (void);
void mono_thread_smr_cleanup (void);
#endif /*__MONO_HAZARD_POINTER_H__*/
