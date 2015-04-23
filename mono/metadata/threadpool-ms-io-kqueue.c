
#if defined(HAVE_KQUEUE)

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

#if defined(HOST_WIN32)
/* We assume that kqueue is not available on windows */
#error
#endif

#define KQUEUE_NEVENTS 128

typedef struct {
	gint kqueue_fd;
	struct kevent *kqueue_events;
} IOSelectorBackendKqueue;

static gint
kqueue_init (gpointer *backend_data, gint wakeup_fd, gchar *error, gint error_size)
{
	IOSelectorBackendKqueue *kqueue_backend_data;
	struct kevent event;

	g_assert (backend_data);

	*backend_data = kqueue_backend_data = g_new0 (IOSelectorBackendKqueue, 1);

	kqueue_backend_data->kqueue_fd = kqueue ();
	if (kqueue_backend_data->kqueue_fd == -1) {
		g_snprintf (error, error_size, "kqueue_init: kqueue () failed, error (%d) %s", errno, g_strerror (errno));
		return -1;
	}

	EV_SET (&event, wakeup_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, 0);
	if (kevent (kqueue_backend_data->kqueue_fd, &event, 1, NULL, 0, NULL) == -1) {
		g_snprintf (error, error_size, "kqueue_init: kevent () failed, error (%d) %s", errno, g_strerror (errno));
		close (kqueue_backend_data->kqueue_fd);
		return -1;
	}

	kqueue_backend_data->kqueue_events = g_new0 (struct kevent, KQUEUE_NEVENTS);

	return 0;
}

static void
kqueue_cleanup (gpointer backend_data)
{
	IOSelectorBackendKqueue *kqueue_backend_data;

	g_assert (backend_data);

	kqueue_backend_data = backend_data;

	close (kqueue_backend_data->kqueue_fd);
	g_free (kqueue_backend_data->kqueue_events);
	g_free (kqueue_backend_data);
}

static gint
kqueue_update_add (gpointer backend_data, MonoIOSelectorUpdate *update, gchar *error, gint error_size)
{
	IOSelectorBackendKqueue *kqueue_backend_data;
	struct kevent event;

	g_assert (backend_data);

	kqueue_backend_data = backend_data;

	if ((update->operations & IO_OP_IN) != 0)
		EV_SET (&event, GPOINTER_TO_INT (update->fd), EVFILT_READ, EV_ADD | EV_ENABLE | EV_ONESHOT, 0, 0, 0);
	if ((update->operations & IO_OP_OUT) != 0)
		EV_SET (&event, GPOINTER_TO_INT (update->fd), EVFILT_WRITE, EV_ADD | EV_ENABLE | EV_ONESHOT, 0, 0, 0);

	if (kevent (kqueue_backend_data->kqueue_fd, &event, 1, NULL, 0, NULL) == -1) {
		switch (errno) {
		case EBADF:
			g_snprintf (error, error_size, "kqueue_update_add: kevent(update) failed, error (%d) %s, fd = %d", errno, g_strerror (errno), GPOINTER_TO_INT (update->fd));
			break;
		default:
			g_snprintf (error, error_size, "kqueue_update_add: kevent(update) failed, error (%d) %s", errno, g_strerror (errno));
			break;
		}
		return -1;
	}

	return 0;
}

static gint
kqueue_event_wait (gpointer backend_data, gchar *error, gint error_size)
{
	IOSelectorBackendKqueue *kqueue_backend_data;
	gint ready;

	g_assert (backend_data);

	kqueue_backend_data = backend_data;

	ready = kevent (kqueue_backend_data->kqueue_fd, NULL, 0, kqueue_backend_data->kqueue_events, KQUEUE_NEVENTS, NULL);
	if (ready == -1) {
		switch (errno) {
		case EINTR:
			mono_thread_internal_check_for_interruption_critical (mono_thread_internal_current ());
			ready = 0;
			break;
		default:
			g_snprintf (error, error_size, "kqueue_event_wait: kevent () failed, error (%d) %s", errno, g_strerror (errno));
			break;
		}
	}

	return ready;
}

static gint
kqueue_event_get_fd_max (gpointer backend_data)
{
	return KQUEUE_NEVENTS;
}

static gint
kqueue_event_get_fd_at (gpointer backend_data, gint i, gint *operations)
{
	IOSelectorBackendKqueue *kqueue_backend_data;
	struct kevent *kqueue_event;

	g_assert (backend_data);
	g_assert (operations);

	kqueue_backend_data = backend_data;
	kqueue_event = &kqueue_backend_data->kqueue_events [i];

	*operations = ((kqueue_event->filter == EVFILT_READ || (kqueue_event->flags & EV_ERROR) != 0) ? IO_OP_IN : 0)
	                | ((kqueue_event->filter == EVFILT_WRITE || (kqueue_event->flags & EV_ERROR) != 0) ? IO_OP_OUT : 0);

	return kqueue_event->ident;
}

static gint
kqueue_event_reset_fd_at (gpointer backend_data, gint i, gint operations, gchar *error, gint error_size)
{
	IOSelectorBackendKqueue *kqueue_backend_data;
	struct kevent *kqueue_event;

	g_assert (backend_data);

	kqueue_backend_data = backend_data;
	kqueue_event = &kqueue_backend_data->kqueue_events [i];

	if (kqueue_event->filter == EVFILT_READ && (operations & IO_OP_IN) != 0) {
		EV_SET (kqueue_event, kqueue_event->ident, EVFILT_READ, EV_ADD | EV_ENABLE | EV_ONESHOT, 0, 0, 0);
		if (kevent (kqueue_backend_data->kqueue_fd, kqueue_event, 1, NULL, 0, NULL) == -1) {
			g_snprintf (error, error_size, "kqueue_event_reset_fd_at: kevent (read) failed, error (%d) %s", errno, g_strerror (errno));
			return -1;
		}
	}
	if (kqueue_event->filter == EVFILT_WRITE && (operations & IO_OP_OUT) != 0) {
		EV_SET (kqueue_event, kqueue_event->ident, EVFILT_WRITE, EV_ADD | EV_ENABLE | EV_ONESHOT, 0, 0, 0);
		if (kevent (kqueue_backend_data->kqueue_fd, kqueue_event, 1, NULL, 0, NULL) == -1) {
			g_snprintf (error, error_size, "kqueue_event_reset_fd_at: kevent (write) failed, error (%d) %s", errno, g_strerror (errno));
			return -1;
		}
	}

	return 0;
}

static IOSelectorBackend backend_kqueue = {
	.init = kqueue_init,
	.cleanup = kqueue_cleanup,
	.update_add = kqueue_update_add,
	.event_wait = kqueue_event_wait,
	.event_get_fd_max = kqueue_event_get_fd_max,
	.event_get_fd_at = kqueue_event_get_fd_at,
	.event_reset_fd_at = kqueue_event_reset_fd_at,
};

#endif
