
using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Remoting.Messaging;

namespace System.Threading
{
	internal interface IIOSelector
	{
		void Add (IntPtr handle, AsyncResult ares, IOAsyncResult ioares);

		void Remove (IntPtr handle);
	}

	/* It is split in 2 different classes, so if we want to add new ways to
	 * select (like IOCP on windows) we just have to create a new class (with
	 * its own runtime support) implementing IIOSelector. */
	internal static class IOSelector
	{
		public readonly static IIOSelector Default = new NativePollingIOSelector ();
	}

	/* This implementation is based on epoll/kqueue/poll (depending on the
	 * platform), and the runtime support is implemented in io-selector.c
	 *
	 * Whenever there is an IO event, the associated IOAsyncResult will be enqueued
	 * on the threadpool. Maybe we should have our own threads, to respect entirely
	 * microsoft implementation. */
	[StructLayout (LayoutKind.Sequential)]
	internal sealed class NativePollingIOSelector : IIOSelector
	{
		/* Keep in sync with MonoIOSelector in metadata/io-selector.c */
		IntPtr backend;
		IntPtr backend_data;

		/* Following fields are not in native */

		Dictionary<IntPtr, List<IOAsyncResult>> states = new Dictionary<IntPtr, List<IOAsyncResult>> ();
		List<Update> updates = new List<Update> ();

		IntPtr[] wakeup_handles = new IntPtr [2];

		Thread selector_thread;
		int selector_thread_status = SelectorThreadStatus.NotRunning;

		public NativePollingIOSelector ()
		{
			SelectorThreadCreateWakeupPipes_internal (out wakeup_handles [0], out wakeup_handles [1]);
			Init_internal (wakeup_handles [0]);
		}

		~NativePollingIOSelector ()
		{
			while (selector_thread != null && selector_thread.IsAlive) {
				SelectorThreadWakeup_internal (wakeup_handles [1]);
				Thread.Yield ();
			}
		}

		public void Add (IntPtr handle, AsyncResult ares, IOAsyncResult ioares)
		{
			ioares.async_result = ares;

			lock (states) {
				bool is_new = false;

				if (!states.ContainsKey (handle)) {
					is_new = true;
					states.Add (handle, new List<IOAsyncResult> ());
				}

				states [handle].Add (ioares);

				lock (updates) {
					updates.Add (new Update {
						handle = handle,
						operation = ioares.operation,
						is_new = is_new,
					});
				}
			}

			SelectorThreadEnsureRunning ();
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
					ThreadPool.UnsafeQueueCustomWorkItem (items [i], false);
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

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		static extern void SelectorThreadCreateWakeupPipes_internal (out IntPtr rdhandle, out IntPtr wrhandle);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		static extern void SelectorThreadWakeup_internal (IntPtr wrhandle);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		static extern void SelectorThreadDrainWakeupPipes_internal (IntPtr rdhandle);

		void SelectorThreadEnsureRunning ()
		{
			for (;;) {
				switch (selector_thread_status) {
				case SelectorThreadStatus.Requested:
					return;
				case SelectorThreadStatus.WaitingForRequest:
					Interlocked.CompareExchange (ref selector_thread_status, SelectorThreadStatus.Requested, SelectorThreadStatus.WaitingForRequest);
					break;
				case SelectorThreadStatus.NotRunning:
					if (Interlocked.CompareExchange (ref selector_thread_status, SelectorThreadStatus.Requested, SelectorThreadStatus.NotRunning) == SelectorThreadStatus.NotRunning) {
						try {
							selector_thread = new Thread (SelectorThread);
							selector_thread.Start ();
						} catch (ThreadStateException) {
							selector_thread_status = SelectorThreadStatus.NotRunning;
						} catch (OutOfMemoryException) {
							selector_thread_status = SelectorThreadStatus.NotRunning;
						}
						return;
					}
					break;
				default:
					throw new InvalidOperationException ();
				}
			}
		}

		bool SelectorThreadShouldKeepRunning ()
		{
			if (selector_thread_status != SelectorThreadStatus.WaitingForRequest && selector_thread_status != SelectorThreadStatus.Requested)
				throw new InvalidOperationException ();

			if (Interlocked.Exchange (ref selector_thread_status, SelectorThreadStatus.WaitingForRequest) == SelectorThreadStatus.WaitingForRequest) {
				if (Environment.HasShutdownStarted) {
					if (Interlocked.CompareExchange (ref selector_thread_status, SelectorThreadStatus.NotRunning, SelectorThreadStatus.WaitingForRequest) == SelectorThreadStatus.WaitingForRequest)
							return false;
				}
			}

			if (selector_thread_status != SelectorThreadStatus.WaitingForRequest && selector_thread_status != SelectorThreadStatus.Requested)
				throw new InvalidOperationException ();

			return true;
		}

		void SelectorThread ()
		{
			do {
				try {
					lock (updates) {
						for (int i = 0; i < updates.Count; ++i) {
							UpdateAdd_internal (updates [i]);
						}
						updates.Clear ();
					}

					int ready = EventWait_internal ();

					lock (states) {
						for (int i = 0, max = EventGetHandleMax_internal (); ready > 0 && i < max; ++i) {
							IOOperation operations;
							IntPtr handle = EventGetHandleAt_internal (i, out operations);

							if (handle == new IntPtr (-1))
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
				} catch {
					/* FIXME: what should we do :
					 *  - nothing : we are going to try it again
					 *  - limit the number of attemps : fail if we try more than N times in a row without success
					 *  - fail
					 */
				}
			} while (SelectorThreadShouldKeepRunning ());
		}

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		extern void Init_internal (IntPtr wakeup_pipe_handle);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		extern void Cleanup_internal ();

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		extern void UpdateAdd_internal (Update update);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		extern int EventWait_internal ();

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		extern int EventGetHandleMax_internal ();

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		extern IntPtr EventGetHandleAt_internal (int i, out IOOperation operations);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		extern void EventResetHandleAt_internal (int i, IOOperation operations);

		[StructLayout (LayoutKind.Sequential)]
		struct Update
		{
			internal IntPtr handle;
			internal IOOperation operation;
			internal bool is_new;
		}

		static class SelectorThreadStatus
		{
			public const int NotRunning = 0;
			public const int Requested = 1;
			public const int WaitingForRequest = 2;
		}
	}
}