
#include <mono/utils/mono-poll.h>

#define POLL_NEVENTS 1024

typedef struct {
	mono_pollfd *poll_fds;
	guint poll_fds_capacity;
	guint poll_fds_size;
} ThreadPoolIOBackendPoll;

static inline void
POLL_INIT_FD (mono_pollfd *poll_fd, gint fd, gint events)
{
	poll_fd->fd = fd;
	poll_fd->events = events;
	poll_fd->revents = 0;
}

static gint
poll_init (gpointer *backend_data, gint wakeup_pipe_fd, gchar *error, gint error_size)
{
	ThreadPoolIOBackendPoll *poll_backend_data;
	guint i;

	g_assert (backend_data);

	*backend_data = poll_backend_data = g_new0 (ThreadPoolIOBackendPoll, 1);
	if (poll_backend_data == NULL) {
		g_snprintf (error, error_size, "poll_init: g_new0 (ThreadPoolIOBackendPoll, 1) failed, error (%d) %s", errno, g_strerror (errno));
		return -1;
	}

	poll_backend_data->poll_fds_size = 1;
	poll_backend_data->poll_fds_capacity = POLL_NEVENTS;
	poll_backend_data->poll_fds = g_new0 (mono_pollfd, poll_backend_data->poll_fds_capacity);
	if (poll_backend_data->poll_fds == NULL) {
		g_snprintf (error, error_size, "poll_init: g_new0 (mono_pollfd, POLL_NEVENTS) failed, error (%d) %s", errno, g_strerror (errno));
		return -1;
	}

	POLL_INIT_FD (&poll_backend_data->poll_fds [0], wakeup_pipe_fd, MONO_POLLIN);
	for (i = 1; i < poll_backend_data->poll_fds_capacity; ++i)
		POLL_INIT_FD (&poll_backend_data->poll_fds [i], -1, 0);

	return 0;
}

static void
poll_cleanup (gpointer backend_data)
{
	ThreadPoolIOBackendPoll *poll_backend_data;

	g_assert (backend_data);
	poll_backend_data = backend_data;

	g_free (poll_backend_data->poll_fds);
	g_free (poll_backend_data);
}

static inline gint
poll_mark_bad_fds (mono_pollfd *poll_fds, gint poll_fds_size)
{
	gint i;
	gint ret;
	gint ready = 0;
	mono_pollfd *poll_fd;

	for (i = 0; i < poll_fds_size; i++) {
		poll_fd = &poll_fds [i];
		if (poll_fd->fd == -1)
			continue;

		ret = mono_poll (poll_fd, 1, 0);
		if (ret == 1)
			ready++;
		if (ret == -1) {
#if !defined(HOST_WIN32)
			if (errno == EBADF)
#else
			if (WSAGetLastError () == WSAEBADF)
#endif
			{
				poll_fd->revents |= MONO_POLLNVAL;
				ready++;
			}
		}
	}

	return ready;
}

static gint
poll_update_add (gpointer backend_data, ThreadPoolIOUpdate *update, gchar *error, gint error_size)
{
	ThreadPoolIOBackendPoll *poll_backend_data;
	gboolean found = FALSE;
	gint operations;
	gint j, k;

	g_assert (backend_data);
	poll_backend_data = backend_data;

	for (j = 1; j < poll_backend_data->poll_fds_size; ++j) {
		mono_pollfd *poll_fd = &poll_backend_data->poll_fds [j];
		if (poll_fd->fd == GPOINTER_TO_INT (update->handle)) {
			found = TRUE;
			break;
		}
	}

	if (!found) {
		for (j = 1; j < poll_backend_data->poll_fds_capacity; ++j) {
			mono_pollfd *poll_fd = &poll_backend_data->poll_fds [j];
			if (poll_fd->fd == -1)
				break;
		}
	}

	if (j == poll_backend_data->poll_fds_capacity) {
		poll_backend_data->poll_fds_capacity += POLL_NEVENTS;
		poll_backend_data->poll_fds = g_renew (mono_pollfd, poll_backend_data->poll_fds, poll_backend_data->poll_fds_capacity);
		for (k = j; k < poll_backend_data->poll_fds_capacity; ++k)
			POLL_INIT_FD (&poll_backend_data->poll_fds [k], -1, 0);
	}

	operations = ((update->operations & IO_OP_OUT) ? MONO_POLLOUT : 0)
	               | ((update->operations & IO_OP_IN) ? MONO_POLLIN : 0);

	POLL_INIT_FD (&poll_backend_data->poll_fds [j], GPOINTER_TO_INT (update->handle), operations);

	if (j >= poll_backend_data->poll_fds_size)
		poll_backend_data->poll_fds_size = j + 1;

	return 0;
}

static gint
poll_event_wait (gpointer backend_data, gchar *error, gint error_size)
{
	ThreadPoolIOBackendPoll *poll_backend_data;
	gint ready;

	g_assert (backend_data);
	poll_backend_data = backend_data;

	ready = mono_poll (poll_backend_data->poll_fds, poll_backend_data->poll_fds_size, -1);
	if (ready == -1) {
		/*
		 * Apart from EINTR, we only check EBADF, for the rest:
		 *  EINVAL: mono_poll() 'protects' us from descriptor
		 *      numbers above the limit if using select() by marking
		 *      then as MONO_POLLERR.  If a system poll() is being
		 *      used, the number of descriptor we're passing will not
		 *      be over sysconf(_SC_OPEN_MAX), as the error would have
		 *      happened when opening.
		 *
		 *  EFAULT: we own the memory pointed by pfds.
		 *  ENOMEM: we're doomed anyway
		 *
		 */
#if !defined(HOST_WIN32)
		switch (errno)
#else
		switch (WSAGetLastError ())
#endif
		{
#if !defined(HOST_WIN32)
		case EINTR:
#else
		case WSAEINTR:
#endif
			mono_thread_internal_check_for_interruption_critical (mono_thread_internal_current ());
			ready = 0;
			break;
#if !defined(HOST_WIN32)
		case EBADF:
#else
		case WSAEBADF:
#endif
			ready = poll_mark_bad_fds (poll_backend_data->poll_fds, poll_backend_data->poll_fds_size);
			break;
		default:
#if !defined(HOST_WIN32)
			g_snprintf (error, error_size, "poll_event_wait: mono_poll () failed, error (%d) %s", errno, g_strerror (errno));
#else
			g_snprintf (error, error_size, "poll_event_wait: mono_poll () failed, error (%d)\n", WSAGetLastError ());
#endif
			break;
		}
	}

	return ready;
}

static gint
poll_event_get_fd_max (gpointer backend_data)
{
	ThreadPoolIOBackendPoll *poll_backend_data;

	g_assert (backend_data);
	poll_backend_data = backend_data;

	return poll_backend_data->poll_fds_size;
}

static gint
poll_event_get_fd_at (gpointer backend_data, gint i, gint32 *operations)
{
	ThreadPoolIOBackendPoll *poll_backend_data;
	mono_pollfd *poll_fd;

	g_assert (backend_data);
	poll_backend_data = backend_data;

	poll_fd = &poll_backend_data->poll_fds [i];

	g_assert (operations);
	*operations = ((poll_fd->revents & (MONO_POLLIN | MONO_POLLERR | MONO_POLLHUP | MONO_POLLNVAL)) ? IO_OP_IN : 0)
	                | ((poll_fd->revents & (MONO_POLLOUT | MONO_POLLERR | MONO_POLLHUP | MONO_POLLNVAL)) ? IO_OP_OUT : 0);

	/* if nothing happened on the fd, then just return
	 * an invalid fd number so managed code discard it */
	return poll_fd->revents == 0 ? -1 : poll_fd->fd;
}

static gint
poll_event_reset_fd_at (gpointer backend_data, gint i, gint32 operations, gchar *error, gint error_size)
{
	ThreadPoolIOBackendPoll *poll_backend_data;
	mono_pollfd *poll_fd;

	g_assert (backend_data);
	poll_backend_data = backend_data;

	poll_fd = &poll_backend_data->poll_fds [i];
	g_assert (poll_fd->fd != -1);
	g_assert (poll_fd->revents != 0);

	POLL_INIT_FD (poll_fd, operations == 0 ? -1 : poll_fd->fd, operations);

	return 0;
}

static ThreadPoolIOBackend backend_poll = {
	.init = poll_init,
	.cleanup = poll_cleanup,
	.update_add = poll_update_add,
	.event_wait = poll_event_wait,
	.event_get_fd_max = poll_event_get_fd_max,
	.event_get_fd_at = poll_event_get_fd_at,
	.event_reset_fd_at = poll_event_reset_fd_at,
};
