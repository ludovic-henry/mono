
#include "fdhandle.h"

#include "utils/mono-lazy-init.h"

static MonoRefHandleTable *fdhandles_table;
static MonoFDHandleCallback fds_callback[MONO_FDTYPE_COUNT];
static mono_lazy_init_t fds_init = MONO_LAZY_INIT_STATUS_NOT_INITIALIZED;

static void
fdhandle_close (MonoRefHandle *refhandle)
{
	MonoFDHandle* fdhandle;

	fdhandle = (MonoFDHandle*) refhandle;
	g_assert (fdhandle);

	g_assert (fds_callback [fdhandle->type].close);
	fds_callback [fdhandle->type].close (fdhandle);
}

static void
fdhandle_destroy (MonoRefHandle *refhandle)
{
	MonoFDHandle* fdhandle;

	fdhandle = (MonoFDHandle*) refhandle;
	g_assert (fdhandle);

	g_assert (fds_callback [fdhandle->type].destroy);
	fds_callback [fdhandle->type].destroy (fdhandle);
}

static void
initialize (void)
{
	fdhandles_table = mono_reftable_new (fdhandle_close);
}

void
mono_fdhandle_register (MonoFDType type, MonoFDHandleCallback *callback)
{
	mono_lazy_initialize (&fds_init, initialize);
	memcpy (&fds_callback [type], callback, sizeof (MonoFDHandleCallback));
}

void
mono_fdhandle_init (MonoFDHandle *fdhandle, MonoFDType type, gint fd)
{
	mono_refhandle_init ((MonoRefHandle*) fdhandle, fdhandle_destroy);
	fdhandle->type = type;
	fdhandle->fd = fd;
}

void
mono_fdhandle_insert (MonoFDHandle *fdhandle)
{
	mono_reftable_insert (fdhandles_table, GINT_TO_POINTER(fdhandle->fd), (MonoRefHandle*) fdhandle);
}

gboolean
mono_fdhandle_try_insert (MonoFDHandle *fdhandle)
{
	return mono_reftable_try_insert (fdhandles_table, GINT_TO_POINTER(fdhandle->fd), (MonoRefHandle*) fdhandle);
}

gboolean
mono_fdhandle_lookup_and_ref (gint fd, MonoFDHandle **fdhandle)
{
	return mono_reftable_lookup_and_ref (fdhandles_table, GINT_TO_POINTER(fd), (MonoRefHandle**) fdhandle);
}

void
mono_fdhandle_unref (MonoFDHandle *fdhandle)
{
	mono_refhandle_unref ((MonoRefHandle*) fdhandle);
}

gboolean
mono_fdhandle_close (gint fd)
{
	return mono_reftable_remove (fdhandles_table, GINT_TO_POINTER (fd));
}
