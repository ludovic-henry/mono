
#if defined(HAVE_EPOLL)

#include <sys/epoll.h>

#if defined(HOST_WIN32)
/* We assume that epoll is not available on windows */
#error
#endif

#define EPOLL_NEVENTS 128

typedef struct {
	gint epoll_fd;
	struct epoll_event *epoll_events;
} ThreadPoolIOBackendEpoll;

static gint
epoll_init (gpointer *backend_data, gint wakeup_pipe_fd, gchar *error, gint error_size)
{
	ThreadPoolIOBackendEpoll *epoll_backend_data;
	struct epoll_event event;

	g_assert (backend_data);

	*backend_data = epoll_backend_data = g_new0 (ThreadPoolIOBackendEpoll, 1);
	if (epoll_backend_data == NULL) {
		g_snprintf (error, error_size, "epoll_init: g_new0 (ThreadPoolIOBackendEpoll, 1) failed, error (%d) %s", errno, g_strerror (errno));
		return -1;
	}

#ifdef EPOOL_CLOEXEC
	epoll_backend_data->epoll_fd = epoll_create1 (EPOLL_CLOEXEC);
#else
	epoll_backend_data->epoll_fd = epoll_create (256);
	fcntl (epoll_backend_data->epoll_fd, F_SETFD, FD_CLOEXEC);
#endif

	if (epoll_backend_data->epoll_fd == -1) {
#ifdef EPOOL_CLOEXEC
		g_snprintf (error, error_size, "epoll_init: epoll (EPOLL_CLOEXEC) failed, error (%d) %s\n", errno, g_strerror (errno));
#else
		g_snprintf (error, error_size, "epoll_init: epoll (256) failed, error (%d) %s\n", errno, g_strerror (errno));
#endif
		return -1;
	}

	event.events = EPOLLIN;
	event.data.fd = wakeup_pipe_fd;
	if (epoll_ctl (epoll_backend_data->epoll_fd, EPOLL_CTL_ADD, event.data.fd, &event) == -1) {
		g_snprintf (error, error_size, "epoll_init: epoll_ctl () failed, error (%d) %s", errno, g_strerror (errno));
		close (epoll_backend_data->epoll_fd);
		return -1;
	}

	epoll_backend_data->epoll_events = g_new0 (struct epoll_event, EPOLL_NEVENTS);
	if (epoll_backend_data->epoll_events == NULL) {
		g_snprintf (error, error_size, "epoll_init: g_new0 (struct epoll_event, EPOLL_NEVENTS) failed, error (%d) %s", errno, g_strerror (errno));
		return -1;
	}

	return 0;
}

static void
epoll_cleanup (gpointer backend_data)
{
	ThreadPoolIOBackendEpoll *epoll_backend_data;

	g_assert (backend_data);
	epoll_backend_data = backend_data;

	close (epoll_backend_data->epoll_fd);
	g_free (epoll_backend_data->epoll_events);
	g_free (epoll_backend_data);
}

static gint
epoll_update_add (gpointer backend_data, ThreadPoolIOUpdate *update, gchar *error, gint error_size)
{
	ThreadPoolIOBackendEpoll *epoll_backend_data;
	struct epoll_event event;

	g_assert (backend_data);
	epoll_backend_data = backend_data;

	event.data.fd = GPOINTER_TO_INT (update->handle);
	if ((update->operations & IO_OP_IN) != 0)
		event.events |= EPOLLIN;
	if ((update->operations & IO_OP_OUT) != 0)
		event.events |= EPOLLOUT;

	if (epoll_ctl (epoll_backend_data->epoll_fd, update->is_new ? EPOLL_CTL_ADD : EPOLL_CTL_MOD, event.data.fd, &event) == -1) {
		g_snprintf (error, error_size, "epoll_update_add: epoll_ctl(%s) failed, error (%d) %s", update->is_new ? "EPOLL_CTL_ADD" : "EPOLL_CTL_MOD", errno, g_strerror (errno));
		return -1;
	}

	return 0;
}

static gint
epoll_event_wait (gpointer backend_data, gchar *error, gint error_size)
{
	ThreadPoolIOBackendEpoll *epoll_backend_data;
	gint ready;

	g_assert (backend_data);
	epoll_backend_data = backend_data;

	ready = epoll_wait (epoll_backend_data->epoll_fd, epoll_backend_data->epoll_events, EPOLL_NEVENTS, -1);
	if (ready == -1) {
		switch (errno) {
		case EINTR:
			mono_thread_internal_check_for_interruption_critical (mono_thread_internal_current ());
			ready = 0;
			break;
		default:
			g_snprintf (error, error_size, "epoll_event_wait: epoll_wait () failed, error (%d) %s", errno, g_strerror (errno));
			break;
		}
	}

	return ready;
}

static gint
epoll_event_get_fd_max (gpointer backend_data)
{
	return EPOLL_NEVENTS;
}

static gint
epoll_event_get_fd_at (gpointer backend_data, gint i, gint32 *operations)
{
	ThreadPoolIOBackendEpoll *epoll_backend_data;
	struct epoll_event *epoll_event;

	g_assert (backend_data);
	epoll_backend_data = backend_data;

	epoll_event = &epoll_backend_data->epoll_events [i];

	g_assert (operations);
	*operations = ((epoll_event->events & (EPOLLIN | EPOLLERR | EPOLLHUP)) ? IO_OP_IN : 0)
	                | ((epoll_event->events & (EPOLLOUT | EPOLLERR | EPOLLHUP)) ? IO_OP_OUT : 0);

	return epoll_event->data.fd;
}

static gint
epoll_event_reset_fd_at (gpointer backend_data, gint i, gint32 operations, gchar *error, gint error_size)
{
	ThreadPoolIOBackendEpoll *epoll_backend_data;
	struct epoll_event *epoll_event;

	g_assert (backend_data);
	epoll_backend_data = backend_data;

	epoll_event = &epoll_backend_data->epoll_events [i];

	if (operations == 0) {
		if (epoll_ctl (epoll_backend_data->epoll_fd, EPOLL_CTL_DEL, epoll_event->data.fd, epoll_event) == -1) {
			g_snprintf (error, error_size, "epoll_event_reset_fd_at: epoll_ctl (EPOLL_CTL_DEL) failed, error (%d) %s", errno, g_strerror (errno));
			return -1;
		}
	} else {
		epoll_event->events = ((operations & IO_OP_OUT) ? EPOLLOUT : 0) | ((operations & IO_OP_IN) ? EPOLLIN : 0);

		if (epoll_ctl (epoll_backend_data->epoll_fd, EPOLL_CTL_MOD, epoll_event->data.fd, epoll_event) == -1) {
			g_snprintf (error, error_size, "epoll_event_reset_fd_at: epoll_ctl (EPOLL_CTL_MOD) failed, error (%d) %s", errno, g_strerror (errno));
			return -1;
		}
	}

	return 0;
}

static ThreadPoolIOBackend backend_epoll = {
	.init = epoll_init,
	.cleanup = epoll_cleanup,
	.update_add = epoll_update_add,
	.event_wait = epoll_event_wait,
	.event_get_fd_max = epoll_event_get_fd_max,
	.event_get_fd_at = epoll_event_get_fd_at,
	.event_reset_fd_at = epoll_event_reset_fd_at,
};

#endif
