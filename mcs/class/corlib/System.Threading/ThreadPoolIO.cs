
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Remoting.Messaging;

namespace System.Threading
{
	internal delegate void IOAsyncCallback (IOAsyncResult ioares);

	internal interface IThreadPoolIOBackend
	{
		void Add (IntPtr handle, IOAsyncCallback iocallback, IOAsyncResult ioares);

		void Remove (IntPtr handle);
	}

	/* It is split in 2 different classes, so if we want to add new ways to
	 * select (like IOCP on windows) we just have to create a new class (with
	 * its own runtime support) implementing IThreadPoolIOBackend. */
	internal static class ThreadPoolIO
	{
		public static readonly IThreadPoolIOBackend Default = new NativeThreadPoolIOBackend ();
	}

	/* This implementation is based on epoll/kqueue/poll (depending on the
	 * platform), and the runtime support is implemented in threadpool-ms-io.c
	 *
	 * Whenever there is an IO event, the associated IOAsyncResult will be enqueued
	 * on the threadpool. Maybe we should have our own threads, to respect entirely
	 * microsoft implementation. */
	[StructLayout (LayoutKind.Sequential)]
	internal sealed class NativeThreadPoolIOBackend : IThreadPoolIOBackend
	{
		static readonly IntPtr invalid_handle = new IntPtr (-1);

		/* Keep in sync with MonoThreadPoolIO in metadata/threadpool-ms-io.c */
		IntPtr backend;
		IntPtr backend_data;

		/* Following fields are not in native */

		Dictionary<IntPtr, List<IOAsyncResult>> states = new Dictionary<IntPtr, List<IOAsyncResult>> ();
		List<ThreadPoolIOUpdate> updates = new List<ThreadPoolIOUpdate> ();

		IntPtr[] wakeup_handles = new IntPtr [2];

		Thread selector_thread;

		public NativeThreadPoolIOBackend ()
		{
			SelectorThreadCreateWakeupPipes_internal (out wakeup_handles [0], out wakeup_handles [1]);
			Init_internal (wakeup_handles [0]);

			selector_thread = new Thread (SelectorThread);
			selector_thread.Start ();
		}

		~NativeThreadPoolIOBackend ()
		{
			foreach (var items in states.Values) {
				for (int i = 0; i < items.Count; ++i)
					items [i].Cancel ();
			}

			SelectorThreadWakeup_internal (wakeup_handles [1]);
			while (selector_thread.IsAlive)
				Thread.Yield ();
		}

		public void Add (IntPtr handle, IOAsyncCallback iocallback, IOAsyncResult ioares)
		{
			if (Environment.HasShutdownStarted) {
				// FIXME : throw exception ?
				return;
			}

			ioares.async_result = new AsyncResult (s => iocallback ((IOAsyncResult) s), ioares, false);

			lock (states) {
				bool is_new = false;

				if (!states.ContainsKey (handle)) {
					is_new = true;
					states.Add (handle, new List<IOAsyncResult> ());
				}

				states [handle].Add (ioares);

				lock (updates) {
					updates.Add (new ThreadPoolIOUpdate {
						handle = handle,
						operation = ioares.operation,
						is_new = is_new,
					});
				}
			}

			SelectorThreadWakeup_internal (wakeup_handles [1]);
		}

		public void Remove (IntPtr handle)
		{
			List<IOAsyncResult> items = null;

			lock (states) {
				if (states.ContainsKey (handle)) {
					items = states [handle];
					states.Remove (handle);
				}
			}

			if (items != null) {
				for (int i = 0; i < items.Count; ++i)
					items [i].Cancel ();
			}
		}

		IOOperation GetOperations (List<IOAsyncResult> items)
		{
			IOOperation operations = 0;
			for (int i = 0; i < items.Count; ++i)
				operations |= items [i].operation;

			return operations;
		}

		IOAsyncResult GetIOAsyncResultForOperation (List<IOAsyncResult> items, IOOperation operation)
		{
			for (int i = 0; i < items.Count; ++i) {
				IOAsyncResult ioares = items [i];
				if ((ioares.operation & operation) != 0) {
					items.RemoveAt (i);
					return ioares;
				}
			}

			return null;
		}

		void SelectorThread ()
		{
			do {
				try {}
				finally  {
					try {
						lock (updates) {
							if (updates.Count > 0) {
								int i = 0;
								try {
									for (; i < updates.Count; ++i) {
										UpdateAdd_internal (updates [i]);
									}
								} finally {
									updates.RemoveRange (0, Math.Min (i + 1, updates.Count));
								}
							}
						}

						int ready = EventWait_internal ();

						lock (states) {
							for (int i = 0, max = EventGetHandleMax_internal (); ready > 0 && i < max; ++i) {
								IOOperation operations;
								IntPtr handle = EventGetHandleAt_internal (i, out operations);

								if (handle == invalid_handle)
									continue;

								if (handle == wakeup_handles [0]) {
									SelectorThreadDrainWakeupPipes_internal (wakeup_handles [0]);
								} else {
									if (states.ContainsKey (handle)) {
										List<IOAsyncResult> items = states [handle];

										if (items.Count != 0 && (operations & IOOperation.In) != 0) {
											IOAsyncResult ioares = GetIOAsyncResultForOperation (items, IOOperation.In);
											if (ioares != null)
												ThreadPool.UnsafeQueueCustomWorkItem (ioares, false);
										}
										if (items.Count != 0 && (operations & IOOperation.Out) != 0) {
											IOAsyncResult ioares = GetIOAsyncResultForOperation (items, IOOperation.Out);
											if (ioares != null)
												ThreadPool.UnsafeQueueCustomWorkItem (ioares, false);
										}

										if (items.Count == 0)
											states.Remove (handle);
									}

									EventResetHandleAt_internal (i, states.ContainsKey (handle) ? GetOperations (states [handle]) : 0);
								}

								ready -= 1;
							}
						}
					} catch (Exception e) {
						/* FIXME: what should we do :
						 *  - nothing : we are going to try it again
						 *  - limit the number of attemps : fail if we try more than N times in a row without success
						 *  - fail
						 */
						 Console.WriteLine (e);
					}
				}
			} while (!Environment.HasShutdownStarted);
		}

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		static extern void SelectorThreadCreateWakeupPipes_internal (out IntPtr rdhandle, out IntPtr wrhandle);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		static extern void SelectorThreadWakeup_internal (IntPtr wrhandle);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		static extern void SelectorThreadDrainWakeupPipes_internal (IntPtr rdhandle);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		extern void Init_internal (IntPtr wakeup_pipe_handle);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		extern void Cleanup_internal ();

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		extern void UpdateAdd_internal (ThreadPoolIOUpdate update);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		extern int EventWait_internal ();

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		extern int EventGetHandleMax_internal ();

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		extern IntPtr EventGetHandleAt_internal (int i, out IOOperation operations);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		extern void EventResetHandleAt_internal (int i, IOOperation operations);
	}

	[StructLayout (LayoutKind.Sequential)]
	struct ThreadPoolIOUpdate
	{
		internal IntPtr handle;
		internal IOOperation operation;
		internal bool is_new;
	}
}
