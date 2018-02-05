/**
 * \file
 * native threadpool worker
 *
 * Author:
 *	Ludovic Henry (ludovic.henry@xamarin.com)
 *
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

#include <stdlib.h>

#include <dispatch/dispatch.h>

#include <config.h>
#include <glib.h>

#include <mono/metadata/threadpool.h>
#include <mono/metadata/threadpool-worker.h>
#include <mono/utils/mono-logger-internals.h>
#include <mono/utils/refcount.h>

struct {
	MonoRefCount ref;

	MonoThreadPoolWorkerCallback callback;

	gint32 work_items_count;

	dispatch_queue_t queue;
} worker;

static void
worker_destroy (gpointer unused)
{
}

void
mono_threadpool_worker_init (MonoThreadPoolWorkerCallback callback)
{
	mono_refcount_init (&worker, worker_destroy);

	worker.callback = callback;

	worker.queue = dispatch_get_global_queue (DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
	g_assert (worker.queue);
}

void
mono_threadpool_worker_cleanup (void)
{
	mono_refcount_dec (&worker);
}

gint32
mono_threadpool_worker_get_min (void)
{
	return 0;
}

gboolean
mono_threadpool_worker_set_min (gint32 value)
{
	return FALSE;
}

gint32
mono_threadpool_worker_get_max (void)
{
	return -1;
}

gboolean
mono_threadpool_worker_set_max (gint32 value)
{
	return FALSE;
}

static void
work_item_push (void)
{
	gint32 old, new;

	do {
		old = mono_atomic_load_i32 (&worker.work_items_count);
		g_assert (old >= 0);

		new = old + 1;
	} while (mono_atomic_cas_i32 (&worker.work_items_count, new, old) != old);
}

static gboolean
work_item_try_pop (void)
{
	gint32 old, new;

	do {
		old = mono_atomic_load_i32 (&worker.work_items_count);
		g_assert (old >= 0);

		if (old == 0)
			return FALSE;

		new = old - 1;
	} while (mono_atomic_cas_i32 (&worker.work_items_count, new, old) != old);

	return TRUE;
}

static void
worker_function (gpointer unused)
{
	static gint32 started = 0;
	gint32 local_started = mono_atomic_inc_i32 (&started);
	if ((local_started % 100) == 0) {
		mono_trace(G_LOG_LEVEL_INFO, MONO_TRACE_THREADPOOL, "threadpool: started = %d", local_started);
	}

	mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_THREADPOOL, "[%p] worker starting",
		GUINT_TO_POINTER (MONO_NATIVE_THREAD_ID_TO_UINT (mono_native_thread_id_get ())));

	if (!mono_refcount_tryinc (&worker))
		return;

	MonoThread *thread = mono_thread_attach (mono_get_root_domain ());
	g_assert (thread);

	thread->internal_thread->threadpool_thread = TRUE;
	mono_thread_set_state (thread->internal_thread, ThreadState_Background);

	while (!mono_runtime_is_shutting_down ()) {
		if (mono_thread_interruption_checkpoint ())
			continue;

		if (!work_item_try_pop ())
			break;

		mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_THREADPOOL, "[%p] worker executing",
			GUINT_TO_POINTER (MONO_NATIVE_THREAD_ID_TO_UINT (mono_native_thread_id_get ())));

		worker.callback ();
	}

	mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_THREADPOOL, "[%p] worker finishing",
		GUINT_TO_POINTER (MONO_NATIVE_THREAD_ID_TO_UINT (mono_native_thread_id_get ())));

	mono_thread_detach (thread);

	mono_refcount_dec (&worker);
}

void
mono_threadpool_worker_request (void)
{
	if (!mono_refcount_tryinc (&worker))
		return;

	work_item_push ();

	mono_trace (G_LOG_LEVEL_DEBUG, MONO_TRACE_THREADPOOL, "[%p] request worker",
		GUINT_TO_POINTER (MONO_NATIVE_THREAD_ID_TO_UINT (mono_native_thread_id_get ())));

	dispatch_async_f (worker.queue, NULL, worker_function);

	mono_refcount_dec (&worker);
}

gboolean
mono_threadpool_worker_notify_completed (void)
{
	return FALSE;
}

void
mono_threadpool_worker_set_suspended (gboolean suspended)
{
}


