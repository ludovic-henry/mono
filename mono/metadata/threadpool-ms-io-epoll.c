
#if defined(HAVE_EPOLL)

#include <sys/epoll.h>

#if defined(HOST_WIN32)
/* We assume that epoll is not available on windows */
#error
#endif

#define EPOLL_NEVENTS 128

static gint epoll_fd;
static struct epoll_event *epoll_events;

static gboolean
epoll_init (gint wakeup_pipe_fd)
{
	struct epoll_event event;

#ifdef EPOOL_CLOEXEC
	epoll_fd = epoll_create1 (EPOLL_CLOEXEC);
#else
	epoll_fd = epoll_create (256);
	fcntl (epoll_fd, F_SETFD, FD_CLOEXEC);
#endif

	if (epoll_fd == -1) {
#ifdef EPOOL_CLOEXEC
		g_warning ("epoll_init: epoll (EPOLL_CLOEXEC) failed, error (%d) %s\n", errno, g_strerror (errno));
#else
		g_warning ("epoll_init: epoll (256) failed, error (%d) %s\n", errno, g_strerror (errno));
#endif
		return FALSE;
	}

	event.events = EPOLLIN;
	event.data.fd = wakeup_pipe_fd;
	if (epoll_ctl (epoll_fd, EPOLL_CTL_ADD, event.data.fd, &event) == -1) {
		g_warning ("epoll_init: epoll_ctl () failed, error (%d) %s", errno, g_strerror (errno));
		close (epoll_fd);
		return FALSE;
	}

	epoll_events = g_new0 (struct epoll_event, EPOLL_NEVENTS);

	return TRUE;
}

static void
epoll_cleanup (void)
{
	g_free (epoll_events);
	close (epoll_fd);
}

static void
epoll_update_add (ThreadPoolIOUpdate *update)
{
	struct epoll_event event;

	event.data.fd = update->fd;
	if ((update->operations & IO_OP_IN) != 0)
		event.events |= EPOLLIN;
	if ((update->operations & IO_OP_OUT) != 0)
		event.events |= EPOLLOUT;

	if (epoll_ctl (epoll_fd, update->is_new ? EPOLL_CTL_ADD : EPOLL_CTL_MOD, event.data.fd, &event) == -1)
		g_warning ("epoll_update_add: epoll_ctl(%s) failed, error (%d) %s", update->is_new ? "EPOLL_CTL_ADD" : "EPOLL_CTL_MOD", errno, g_strerror (errno));
}

static gint
epoll_event_wait (void)
{
	gint ready;

	ready = epoll_wait (epoll_fd, epoll_events, EPOLL_NEVENTS, -1);
	if (ready == -1) {
		switch (errno) {
		case EINTR:
			mono_thread_internal_check_for_interruption_critical (mono_thread_internal_current ());
			ready = 0;
			break;
		default:
			g_warning ("epoll_event_wait: epoll_wait () failed, error (%d) %s", errno, g_strerror (errno));
			break;
		}
	}

	return ready;
}

static gint
epoll_event_get_fd_max (void)
{
	return EPOLL_NEVENTS;
}

static gint
epoll_event_get_fd_at (gint i, gint *operations)
{
	struct epoll_event *epoll_event;

	g_assert (operations);

	epoll_event = &epoll_events [i];

	*operations = ((epoll_event->events & (EPOLLIN | EPOLLERR | EPOLLHUP)) ? IO_OP_IN : 0)
	                | ((epoll_event->events & (EPOLLOUT | EPOLLERR | EPOLLHUP)) ? IO_OP_OUT : 0);

	return epoll_event->data.fd;
}

static void
epoll_event_reset_fd_at (gint i, gint operations)
{
	struct epoll_event *epoll_event;

	epoll_event = &epoll_events [i];

	if (operations == 0) {
		if (epoll_ctl (epoll_fd, EPOLL_CTL_DEL, epoll_event->data.fd, epoll_event) == -1) {
			g_warning ("epoll_event_reset_fd_at: epoll_ctl (EPOLL_CTL_DEL) failed, error (%d) %s", errno, g_strerror (errno));
		}
	} else {
		epoll_event->events = ((operations & IO_OP_OUT) ? EPOLLOUT : 0) | ((operations & IO_OP_IN) ? EPOLLIN : 0);

		if (epoll_ctl (epoll_fd, EPOLL_CTL_MOD, epoll_event->data.fd, epoll_event) == -1) {
			g_warning ("epoll_event_get_ioares_at: epoll_ctl (EPOLL_CTL_MOD) failed, error (%d) %s", errno, g_strerror (errno));
		}
	}
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
