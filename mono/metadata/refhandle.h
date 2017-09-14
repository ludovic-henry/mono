
#ifndef _MONO_METADATA_REFHANDLE_H_
#define _MONO_METADATA_REFHANDLE_H_

#include <config.h>
#include <glib.h>

#include "mono/utils/mono-coop-mutex.h"
#include "mono/utils/refcount.h"

typedef struct {
	MonoRefCount ref;
} MonoRefHandle;

typedef struct {
	GHashTable *table;
	MonoCoopMutex mutex;
	void (*close) (MonoRefHandle *refhandle);
} MonoRefHandleTable;

static inline MonoRefHandleTable*
mono_reftable_new (void (*close) (MonoRefHandle *refhandle))
{
	MonoRefHandleTable *reftable;

	g_assert (close);

	reftable = g_new0 (MonoRefHandleTable, 1);
	reftable->table = g_hash_table_new (NULL, NULL);
	mono_coop_mutex_init (&reftable->mutex);
	reftable->close = close;

	return reftable;
}

static inline void
mono_reftable_destroy (MonoRefHandleTable *reftable)
{
	g_hash_table_destroy (reftable->table);

	mono_coop_mutex_destroy (&reftable->mutex);

	g_free (reftable);
}

static inline void
mono_reftable_insert (MonoRefHandleTable *reftable, gpointer key, MonoRefHandle *refhandle)
{
	mono_coop_mutex_lock (&reftable->mutex);

	if (g_hash_table_lookup_extended (reftable->table, key, NULL, NULL))
		g_error("%s: duplicate handle %p", __func__, key);

	g_hash_table_insert (reftable->table, key, refhandle);

	mono_coop_mutex_unlock (&reftable->mutex);
}

static inline gboolean
mono_reftable_try_insert (MonoRefHandleTable *reftable, gpointer key, MonoRefHandle *refhandle)
{
	mono_coop_mutex_lock (&reftable->mutex);

	if (g_hash_table_lookup_extended (reftable->table, key, NULL, NULL)) {
		mono_coop_mutex_unlock (&reftable->mutex);
		return FALSE;
	}

	g_hash_table_insert (reftable->table, key, refhandle);

	mono_coop_mutex_unlock (&reftable->mutex);

	return TRUE;
}

static inline gboolean
mono_reftable_remove (MonoRefHandleTable *reftable, gpointer key)
{
	MonoRefHandle *refhandle;
	gboolean removed;

	mono_coop_mutex_lock (&reftable->mutex);

	if (!g_hash_table_lookup_extended (reftable->table, key, NULL, (gpointer*) &refhandle)) {
		mono_coop_mutex_unlock (&reftable->mutex);
		return FALSE;
	}

	removed = g_hash_table_remove (reftable->table, key);
	g_assert (removed);

	if (reftable->close)
		reftable->close (refhandle);

	mono_refcount_dec (refhandle);

	mono_coop_mutex_unlock (&reftable->mutex);

	return TRUE;
}

static inline void
mono_reftable_foreach (MonoRefHandleTable *reftable, gboolean (*on_each) (MonoRefHandle *refhandle, gpointer user_data), gpointer user_data)
{
	MonoRefHandle *refhandle;
	GHashTableIter iter;

	mono_coop_mutex_lock (&reftable->mutex);

	g_hash_table_iter_init (&iter, reftable->table);

	while (g_hash_table_iter_next (&iter, NULL, (gpointer*) &refhandle)) {
		if (on_each (refhandle, user_data))
			break;
	}

	mono_coop_mutex_unlock (&reftable->mutex);
}

static inline gboolean
mono_reftable_lookup_and_ref (MonoRefHandleTable *reftable, gpointer key, MonoRefHandle **refhandle)
{
	mono_coop_mutex_lock (&reftable->mutex);

	if (!g_hash_table_lookup_extended (reftable->table, key, NULL, (gpointer*) refhandle)) {
		mono_coop_mutex_unlock (&reftable->mutex);
		return FALSE;
	}

	mono_refcount_inc (*refhandle);

	mono_coop_mutex_unlock (&reftable->mutex);

	return TRUE;
}

static inline void
mono_refhandle_init (MonoRefHandle *refhandle, void (*destroy) (MonoRefHandle *refhandle))
{
	g_assert (destroy);
	mono_refcount_init (refhandle, (void (*) (gpointer)) destroy);
}

static inline void
mono_refhandle_unref (MonoRefHandle *refhandle)
{
	mono_refcount_dec (refhandle);
}

#endif /* _MONO_METADATA_REFHANDLE_H_ */
