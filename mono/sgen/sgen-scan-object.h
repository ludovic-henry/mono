/*
 * sgen-scan-object.h: Generic object scan.
 *
 * Copyright 2001-2003 Ximian, Inc
 * Copyright 2003-2010 Novell, Inc.
 * Copyright (C) 2013 Xamarin Inc
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

#ifndef __MONO_SGEN_SCAN_OBJECT_H__
#define __MONO_SGEN_SCAN_OBJECT_H__

#include "sgen-descriptor.h"

typedef void (*SgenHandlePtr) (GCObject **ptr, GCObject *obj, gpointer data);

enum SgenScanObjectFlags
{
	SCAN_OBJECT_NOSCAN   = 1 << 0,
	SCAN_OBJECT_NOVTABLE = 1 << 1,
	SCAN_OBJECT_PROTOCOL = 1 << 2,
};

static inline MONO_ALWAYS_INLINE void
OBJ_RUN_LEN_FOREACH_PTR (SgenDescriptor desc, GCObject *obj, SgenHandlePtr handle_ptr, gpointer data)
{
	if (desc & 0xffff0000) {
		/* there are pointers */
		void **objptr = (void**) obj + ((desc >> 16) & 0xff);
		void **objptr_end = objptr + ((desc >> 24) & 0xff);
		while (objptr < objptr_end) {
			handle_ptr ((GCObject**)objptr, obj, data);
			objptr++;
		}
	}
}

/* a bitmap desc means that there are pointer references or we'd have
 * choosen run-length, instead: add an assert to check.
 */
static inline MONO_ALWAYS_INLINE void
OBJ_BITMAP_FOREACH_PTR (SgenDescriptor desc, GCObject *obj, SgenHandlePtr handle_ptr, gpointer data)
{
	/* there are pointers */
	void **objptr = (void**) obj + OBJECT_HEADER_WORDS;
	gsize bmap = desc >> LOW_TYPE_BITS;
	do {
#ifdef __GNUC__
		int index = GNUC_BUILTIN_CTZ (bmap);
		bmap >>= index + 1;
		objptr += index;
		handle_ptr ((GCObject**) objptr, obj, data);
#else
		if (bmap & 1)
			handle_ptr ((GCObject**) objptr, obj, data);
		bmap >>= 1;
#endif
		++objptr;
	} while (bmap);
}

static inline MONO_ALWAYS_INLINE void
OBJ_COMPLEX_FOREACH_PTR (SgenDescriptor desc, GCObject *obj, SgenHandlePtr handle_ptr, gpointer data)
{
	/* there are pointers */
	void **objptr = (void**) obj;
	gsize *bitmap_data = sgen_get_complex_descriptor (desc);
	gsize bwords = *bitmap_data - 1;
	void **start_run = objptr;
	bitmap_data++;
	while (bwords-- > 0) {
		gsize bmap = *bitmap_data++;
		objptr = start_run;
		/*g_print ("bitmap: 0x%x/%d at %p\n", bmap, bwords, objptr);*/
		while (bmap) {
			if (bmap & 1)
				handle_ptr ((GCObject**) objptr, obj, data);
			bmap >>= 1;
			++objptr;
		}
		start_run += GC_BITS_PER_WORD;
	}
}

/* this one is untested */
static inline MONO_ALWAYS_INLINE void
OBJ_COMPLEX_ARR_FOREACH_PTR (SgenDescriptor desc, GCObject *obj, SgenHandlePtr handle_ptr, gpointer data)
{
	/* there are pointers */
	GCVTable vt = SGEN_LOAD_VTABLE (obj);
	gsize *mbitmap_data = sgen_get_complex_descriptor (desc);
	gsize mbwords = (*mbitmap_data++) - 1;
	gsize el_size = sgen_client_array_element_size (vt);
	char *e_start = sgen_client_array_data_start ((GCObject*) obj);
	char *e_end = e_start + el_size * sgen_client_array_length ((GCObject*) obj);
	while (e_start < e_end) {
		void **objptr = (void**)e_start;
		gsize *bitmap_data = mbitmap_data;
		gsize bwords = mbwords;
		while (bwords-- > 0) {
			gsize bmap = *bitmap_data++;
			void **start_run = objptr;
			/*g_print ("bitmap: 0x%x\n", bmap);*/
			while (bmap) {
				if (bmap & 1)
					handle_ptr ((GCObject**) objptr, obj, data);
				bmap >>= 1;
				++objptr;
			}
			objptr = start_run + GC_BITS_PER_WORD;
		}
		e_start += el_size;
	}
}

static inline MONO_ALWAYS_INLINE void
OBJ_VECTOR_FOREACH_PTR (SgenDescriptor desc, GCObject *obj, SgenHandlePtr handle_ptr, gpointer data)
{
	/* note: 0xffffc000 excludes DESC_TYPE_V_PTRFREE */
	if ((desc) & 0xffffc000) {
		int el_size = ((desc) >> 3) & MAX_ELEMENT_SIZE;
		/* there are pointers */
		int etype = (desc) & 0xc000;
		if (etype == (DESC_TYPE_V_REFS << 14)) {
			void **p = (void**)sgen_client_array_data_start ((GCObject*)(obj));
			void **end_refs = (void**)((char*)p + el_size * sgen_client_array_length ((GCObject*)(obj)));
			/* Note: this code can handle also arrays of struct with only references in them */
			while (p < end_refs) {
				handle_ptr ((GCObject**)p, obj, data);
				++p;
			}
		} else if (etype == DESC_TYPE_V_RUN_LEN << 14) {
			int offset = ((desc) >> 16) & 0xff;
			int num_refs = ((desc) >> 24) & 0xff;
			char *e_start = sgen_client_array_data_start ((GCObject*)(obj));
			char *e_end = e_start + el_size * sgen_client_array_length ((GCObject*)(obj));
			while (e_start < e_end) {
				void **p = (void**)e_start;
				int i;
				p += offset;
				for (i = 0; i < num_refs; ++i)
					handle_ptr ((GCObject**)p + i, obj, data);
				e_start += el_size;
			}
		} else if (etype == DESC_TYPE_V_BITMAP << 14) {
			char *e_start = sgen_client_array_data_start ((GCObject*)(obj));
			char *e_end = e_start + el_size * sgen_client_array_length ((GCObject*)(obj));
			while (e_start < e_end) {
				void **p = (void**)e_start;
				gsize bmap = (desc) >> 16;
				/* Note: there is no object header here to skip */
				while (bmap) {
					if ((bmap & 1))
						handle_ptr ((GCObject**)p, obj, data);
					bmap >>= 1;
					++p;
				}
				e_start += el_size;
			}
		}
	}
}

/*
 * Scan one object, using the OBJ_XXX macros. The start of the object must be given in the parameter obj. The object's
 * GC descriptor must be in the parameter desc.
 *
 * The function `handle_ptr` will be invoked for every reference encountered while scanning the object. It is called
 * with three parameters: The pointer to the reference (not the reference itself!), the pointer to the scanned object
 * and the data pointer passed to sgen_scan_object.
 *
 * Flags:
 *
 * SCAN_OBJECT_NOSCAN - if defined, don't actually scan the object, i.e. don't invoke the OBJ_XXX macros.
 *
 * SCAN_OBJECT_NOVTABLE - desc is provided by the includer, instead of vt. Complex arrays cannot not be scanned.
 *
 * SCAN_OBJECT_PROTOCOL - if defined, binary protocol the scan. Should only be used for scanning that's done for the
 *   actual collection, not for debugging scans.
 */
static inline MONO_ALWAYS_INLINE void
sgen_scan_object (SgenDescriptor desc, GCObject *obj, SgenHandlePtr handle_ptr, gpointer data, enum SgenScanObjectFlags flags)
{
#if defined(SGEN_HEAVY_BINARY_PROTOCOL)
	if (flags & SCAN_OBJECT_PROTOCOL) {
		if (flags & SCAN_OBJECT_NOVTABLE)
			binary_protocol_scan_begin (obj, SGEN_LOAD_VTABLE (obj), sgen_safe_object_get_size (obj));
		else
			binary_protocol_scan_vtype_begin ((char*) obj + sizeof (GCObject), size);
	}
#endif

	switch (desc & DESC_TYPE_MASK) {
	case DESC_TYPE_RUN_LENGTH:
		if (!(flags & SCAN_OBJECT_NOSCAN))
			OBJ_RUN_LEN_FOREACH_PTR (desc, obj, handle_ptr, data);
		break;
	case DESC_TYPE_VECTOR:
		if (!(flags & SCAN_OBJECT_NOSCAN))
			OBJ_VECTOR_FOREACH_PTR (desc, obj, handle_ptr, data);
		break;
	case DESC_TYPE_BITMAP:
		if (!(flags & SCAN_OBJECT_NOSCAN))
			OBJ_BITMAP_FOREACH_PTR (desc, obj, handle_ptr, data);
		break;
	case DESC_TYPE_COMPLEX:
		/* this is a complex object */
		if (!(flags & SCAN_OBJECT_NOSCAN))
			OBJ_COMPLEX_FOREACH_PTR (desc, obj, handle_ptr, data);
		break;
	case DESC_TYPE_COMPLEX_ARR:
		g_assert (!(flags & SCAN_OBJECT_NOVTABLE));

		/* this is an array of complex structs */
		if (!(flags & SCAN_OBJECT_NOSCAN))
			OBJ_COMPLEX_ARR_FOREACH_PTR (desc, obj, handle_ptr, data);
		break;
	case DESC_TYPE_SMALL_PTRFREE:
	case DESC_TYPE_COMPLEX_PTRFREE:
		/*Nothing to do*/
		break;
	default:
		g_assert_not_reached ();
	}
}

#endif /* __MONO_SGEN_SCAN_OBJECT_H__ */
