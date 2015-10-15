// System/System.IOSelector.cs
//
// Authors:
//	Ludovic Henry <ludovic@xamarin.com>
//
// Copyright (C) 2015 Xamarin, Inc. (https://www.xamarin.com)
//
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

using Microsoft.Win32.SafeHandles;
using System.Collections.Generic;
using System.IO;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Threading;

namespace System
{
	/* Keep in sync with ThreadPoolIOOperation in mono/metadata/threadpool-ms-io.c */
	internal enum IOOperation
	{
		Read  = 1 << 0,
		Write = 1 << 1,
	}

	internal delegate void IOAsyncCallback (IOAsyncResult ioares);

	internal abstract class IOAsyncResult : IAsyncResult
	{
		AsyncCallback async_callback;
		object async_state;

		ManualResetEvent wait_handle;
		bool completed_synchronously;
		bool completed;

		protected IOAsyncResult ()
		{
		}

		protected void Init (AsyncCallback async_callback, object async_state)
		{
			this.async_callback = async_callback;
			this.async_state = async_state;

			completed = false;
			completed_synchronously = false;

			if (wait_handle != null)
				wait_handle.Reset ();
		}

		protected IOAsyncResult (AsyncCallback async_callback, object async_state)
		{
			this.async_callback = async_callback;
			this.async_state = async_state;
		}

		public AsyncCallback AsyncCallback
		{
			get { return async_callback; }
		}

		public object AsyncState
		{
			get { return async_state; }
		}

		public WaitHandle AsyncWaitHandle
		{
			get {
				lock (this) {
					if (wait_handle == null)
						wait_handle = new ManualResetEvent (completed);
					return wait_handle;
				}
			}
		}

		public bool CompletedSynchronously
		{
			get {
				return completed_synchronously;
			}
			protected set {
				completed_synchronously = value;
			}
		}

		public bool IsCompleted
		{
			get {
				return completed;
			}
			protected set {
				completed = value;
				lock (this) {
					if (value && wait_handle != null)
						wait_handle.Set ();
				}
			}
		}

		internal abstract void CompleteDisposed();
	}

	internal struct IOSelectorJob : IThreadPoolWorkItem
	{
		IOOperation operation;
		IOAsyncCallback callback;
		IOAsyncResult state;

		public IOOperation Operation {
			get { return operation; }
		}

		public IOSelectorJob (IOOperation operation, IOAsyncCallback callback, IOAsyncResult state)
		{
			this.operation = operation;
			this.callback = callback;
			this.state = state;
		}

		void IThreadPoolWorkItem.ExecuteWorkItem ()
		{
			this.callback (this.state);
		}

		void IThreadPoolWorkItem.MarkAborted (ThreadAbortException tae)
		{
		}

		public void MarkDisposed ()
		{
			state.CompleteDisposed ();
		}
	}

	internal static class IOSelector
	{
		static FileStream wakeup_pipes_rd;
		static FileStream wakeup_pipes_wr;

		static Thread polling_thread;

		static readonly Queue<Update> updates = new Queue<Update> ();

		static readonly byte[] send_buffer = new byte [1];
		static readonly byte[] recv_buffer = new byte [128];

		static readonly IntPtr invalid_handle = new IntPtr (-1);

		static IOSelector ()
		{
			try {
			} finally {
				MonoIOError error;
				IntPtr wakeup_pipes_rd_handle;
				IntPtr wakeup_pipes_wr_handle;
				if (!MonoIO.CreatePipe (out wakeup_pipes_rd_handle, out wakeup_pipes_wr_handle, out error))
					throw MonoIO.GetException (error);

				wakeup_pipes_rd = new FileStream (new SafeFileHandle (wakeup_pipes_rd_handle, true), FileAccess.Read);
				wakeup_pipes_wr = new FileStream (new SafeFileHandle (wakeup_pipes_wr_handle, true), FileAccess.Write);
			}

			polling_thread = new Thread (PollingThread, Marshal.SizeOf (typeof (IntPtr)) * 32 * 1024);
			polling_thread.IsBackground = true;
			polling_thread.Start ();
		}

		public static void Add (IntPtr handle, IOSelectorJob job)
		{
			if (Environment.HasShutdownStarted)
				return;

			using (Update update = Update.Borrow ()) {
				update.type = UpdateType.Add;
				update.handle = handle;
				update.job = job;

				lock (updates)
					updates.Enqueue (update);

				wakeup_pipes_wr.Write (send_buffer, 0, send_buffer.Length);

				while (!update.applied)
					Monitor.Wait (update);
			}
		}

		public static void Remove (IntPtr handle)
		{
			if (Environment.HasShutdownStarted)
				return;

			using (Update update = Update.Borrow ()) {
				update.type = UpdateType.Remove;
				update.handle = handle;

				lock (updates)
					updates.Enqueue (update);

				wakeup_pipes_wr.Write (send_buffer, 0, send_buffer.Length);

				while (!update.applied)
					Monitor.Wait (update);
			}
		}

		static bool FirstJobForOperation (List<IOSelectorJob> jobs, IOOperation operation, out IOSelectorJob job)
		{
			job = default (IOSelectorJob);

			for (int i = 0; i < jobs.Count; ++i) {
				job = jobs [i];
				if ((job.Operation & operation) != 0) {
					jobs.RemoveAt (i);
					return true;
				}
			}

			return false;
		}

		static int GetOperationsForJobs (List<IOSelectorJob> jobs)
		{
			int operations = 0;
			for (int i = 0; i < jobs.Count; ++i)
				operations |= (int) jobs [i].Operation;

			return operations;
		}

		static void PollingThread ()
		{
			IntPtr backend = IntPtr.Zero;

			try {
				BackendInitialize (wakeup_pipes_rd.SafeFileHandle, out backend);

				Dictionary<IntPtr, List<IOSelectorJob>> states = new Dictionary<IntPtr, List<IOSelectorJob>> ();
				BackendEvent[] backend_events = new BackendEvent [8];

				for (;;) {
					int ready, remaining;

					lock (updates) {
						while (updates.Count > 0) {
							Update u = updates.Dequeue ();

							switch (u.type) {
							case UpdateType.Empty: {
								break;
							}
							case UpdateType.Add: {
								bool exists = states.ContainsKey (u.handle);
								if (!exists)
									states.Add (u.handle, new List<IOSelectorJob> ());

								states [u.handle].Add (u.job);

								BackendAddHandle (backend, u.handle, GetOperationsForJobs (states [u.handle]), !exists);

								break;
							}
							case UpdateType.Remove: {
								if (states.ContainsKey (u.handle)) {
									List<IOSelectorJob> jobs = states [u.handle];
									for (int j = 0; j < jobs.Count; ++j)
										jobs [j].MarkDisposed ();

									states.Remove (u.handle);

									BackendRemoveHandle (backend, u.handle);
								}

								for (int i = 0; i < updates.Count; ++i) {
									Update u1 = updates.GetElement (i);
									if (u1.type == UpdateType.Add && u1.handle == u.handle)
										u1.type = UpdateType.Empty;
								}

								break;
							}
							default:
								throw new InvalidOperationException ();
							}

							u.applied = true;
							Monitor.Pulse (u);
						}
					}

					for (int i = 0; i < backend_events.Length; ++i)
						backend_events [i].handle = invalid_handle;

					ready = remaining = BackendPoll (backend, backend_events);

					if (ready == 0)
						continue;

					for (int i = 0; i < backend_events.Length; ++i) {
						BackendEvent e = backend_events [i];

						if (e.handle == invalid_handle)
							continue;

						if (e.handle == wakeup_pipes_rd.Handle) {
							wakeup_pipes_rd.Read (recv_buffer, 0, recv_buffer.Length);
						} else {
							if (!states.ContainsKey (e.handle))
								throw new InvalidOperationException (String.Format ("handle {0} not found in states dictionary", (int) e.handle));

							IOSelectorJob job;
							List<IOSelectorJob> jobs = states [e.handle];

							if (jobs.Count > 0 && (e.events & (int) BackendEventType.In) != 0) {
								if (FirstJobForOperation (jobs, IOOperation.Read, out job))
									ThreadPool.UnsafeQueueCustomWorkItem (job, false);
							}
							if (jobs.Count > 0 && (e.events & (int) BackendEventType.Out) != 0) {
								if (FirstJobForOperation (jobs, IOOperation.Write, out job))
									ThreadPool.UnsafeQueueCustomWorkItem (job, false);
							}

							if ((e.events & (int) BackendEventType.Error) == 0) {
								BackendAddHandle (backend, e.handle, GetOperationsForJobs (jobs), false);
							} else {
								states.Remove (e.handle);

								BackendRemoveHandle (backend, e.handle);
							}
						}

						if (--remaining == 0)
							break;
					}

					if (ready < backend_events.Length / 3)
						Array.Resize (ref backend_events, backend_events.Length / 2);
					else if (ready > backend_events / 2)
						Array.Resize (ref backend_events, backend_events.Length * 1.5);
				}
			} finally {
				if (backend != IntPtr.Zero)
					BackendCleanup (backend);
			}
		}

		static void BackendInitialize (SafeHandle wakeup_pipe, out IntPtr backend)
		{
			bool release = false;
			try {
				wakeup_pipe.DangerousAddRef (ref release);
				try {
				} finally {
					/* It shouldn't be interrupted by a ThreadAbortException, as that could leak the `backend` resource */
					backend = BackendInitialize (wakeup_pipe.DangerousGetHandle ());
				}
			} finally {
				if (release)
					wakeup_pipe.DangerousRelease ();
			}
		}

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		static extern IntPtr BackendInitialize (IntPtr wakeup_pipes_handle);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		static extern void BackendCleanup (IntPtr backend);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		static extern void BackendAddHandle (IntPtr backend, IntPtr handle, int operations, bool is_new);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		static extern void BackendRemoveHandle (IntPtr backend, IntPtr handle);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		static extern int BackendPoll (IntPtr backend, BackendEvent[] events);

		class Update : IDisposable {
			public UpdateType type;
			public IntPtr handle;
			public IOSelectorJob job;
			public bool applied;

			static ConcurrentStack<Update> update_free_list = new ConcurrentStack<Update> ();

			public static Update Borrow ()
			{
				Update update;
				if (update_free_list.TryPop (out update))
					return update;

				return new Update ();
			}

			public void Reset ()
			{
				type = UpdateType.Empty;
				handle = IntPtr.Zero;
				job = default (IOSelectorJob);
				applied = false;
			}

			void IDisposable.Dispose ()
			{
				if (update_free_list.Count > 32)
					return;

				Reset ();
				update_free_list.Push (this);
			}
		}

		enum UpdateType {
			Empty,
			Add,
			Remove,
		}

		/* Keep in sync with ThreadPoolIOBackendEvent in mono/metadata/threadpool-ms-io.c */
		[StructLayout (LayoutKind.Sequential)]
		struct BackendEvent {
			public IntPtr handle;
			public short events;
		}

		/* Keep in sync with ThreadPoolIOBackendEventType in mono/metadata/threadpool-ms-io.c */
		enum BackendEventType {
			In    = 1 << 0,
			Out   = 1 << 1,
			Error = 1 << 2,
		}
	}
}
