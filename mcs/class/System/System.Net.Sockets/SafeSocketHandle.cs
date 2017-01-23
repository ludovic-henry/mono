//
// System.Net.Sockets.SafeSocketHandle
//
// Authors:
//	Marcos Henrich  <marcos.henrich@xamarin.com>
//

using System;
using System.IO;
using System.Text;
using System.Threading;
using System.Diagnostics;
using System.Collections.Generic;
using Microsoft.Win32.SafeHandles;

namespace System.Net.Sockets {

	sealed class SafeSocketHandle : SafeHandleZeroOrMinusOneIsInvalid {

		const int RefCount_Mask = 0x7ffffffe;
		const int RefCount_One = 0x2;

		const int State_Closed = 0x1;

		int state;
		List<Thread> blocking_threads;

		const int ABORT_RETRIES = 10;
		static bool THROW_ON_ABORT_RETRIES = Environment.GetEnvironmentVariable("MONO_TESTS_IN_PROGRESS") == "yes";
		Dictionary<Thread, StackTrace> threads_stacktraces;

		public SafeSocketHandle (IntPtr preexistingHandle, bool ownsHandle) : base (ownsHandle)
		{
			SetHandle (preexistingHandle);

			if (THROW_ON_ABORT_RETRIES)
				threads_stacktraces = new Dictionary<Thread, StackTrace> ();
		}

		// This is just for marshalling
		internal SafeSocketHandle () : base (true)
		{
		}

		protected override bool ReleaseHandle ()
		{
			int error;
			Socket.Close_internal (handle, out error);
			return error == 0;
		}

		public void RegisterForBlockingSyscall (ref bool release, ref bool unregister)
		{
			if (blocking_threads == null)
				Interlocked.CompareExchange (ref blocking_threads, new List<Thread> (), null);

			base.DangerousAddRef (ref release);

			try {} finally {
				int old_state, new_state;

				do {
					old_state = state;

					if ((old_state & State_Closed) != 0)
						throw new SocketException (SocketError.Interrupted);

					new_state = old_state + RefCount_One;
				} while (Interlocked.CompareExchange (ref state, new_state, old_state) != old_state);

				unregister = true;

				lock (blocking_threads) {
					blocking_threads.Add (Thread.CurrentThread);
					if (THROW_ON_ABORT_RETRIES)
						threads_stacktraces.Add (Thread.CurrentThread, new StackTrace (true));
				}
			}
		}

		/* This must be called from a finally block! */
		public void UnregisterForBlockingSyscall (bool unregister)
		{
			if (unregister) {
				try {} finally {
					int old_state, new_state;

					do {
						old_state = state;

						if ((old_state & RefCount_Mask) == 0) {
							/* Calls to RegisterForBlockingSyscall and UnregisterForBlockingSyscall are unbalanced */
							throw new InvalidOperationException ();
						}

						new_state = old_state - RefCount_One;
					} while (Interlocked.CompareExchange (ref state, new_state, old_state) != old_state);

					lock (blocking_threads) {
						blocking_threads.Remove (Thread.CurrentThread);
						if (THROW_ON_ABORT_RETRIES) {
							if (blocking_threads.IndexOf (Thread.CurrentThread) == -1)
								threads_stacktraces.Remove (Thread.CurrentThread);
						}

						if ((new_state & State_Closed) != 0 && (new_state & RefCount_Mask) == 0)
							Monitor.Pulse (blocking_threads);
					}
				}
			}

			base.DangerousRelease ();
		}

		public void InterruptBlockingSyscall ()
		{
			try {} finally {
				int old_state, new_state;

				do {
					old_state = state;

					new_state = old_state | State_Closed;
				} while (Interlocked.CompareExchange (ref state, new_state, old_state) != old_state);

				if ((new_state & RefCount_Mask) != 0) {
					int error;

					try {
						Mono.PAL.Sockets.SetBlocking (this, false);
					} catch (SocketException e) {
					}

#if FULL_AOT_DESKTOP
					/* It's only for platforms that do not have working syscall abort mechanism, like WatchOS and TvOS */
					Socket.Shutdown_internal (handle, SocketShutdown.Both, out error);
#endif

					if (blocking_threads != null) {
						lock (blocking_threads) {
							int abort_attempts = 0;
							while (blocking_threads.Count > 0) {
								if (abort_attempts++ >= ABORT_RETRIES) {
									if (THROW_ON_ABORT_RETRIES) {
										StringBuilder sb = new StringBuilder ();
										sb.AppendLine ("Could not abort registered blocking threads before closing socket.");
										foreach (var thread in blocking_threads) {
											sb.AppendLine ("Thread StackTrace:");
											sb.AppendLine (threads_stacktraces[thread].ToString ());
										}
										sb.AppendLine ();

										throw new Exception (sb.ToString ());
									}

									// Attempts to close the socket safely failed.
									// We give up, and close the socket with pending blocking system calls.
									// This should not occur, nonetheless if it does this avoids an endless loop.
									break;
								}

								/*
								* This method can be called by the DangerousRelease inside RegisterForBlockingSyscall
								* When this happens blocking_threads contains the current thread.
								* We can safely close the socket and throw SocketException in RegisterForBlockingSyscall
								* before the blocking system call.
								*/
								if (blocking_threads.Count == 1 && blocking_threads[0] == Thread.CurrentThread)
									break;

								// abort registered threads
								foreach (var t in blocking_threads)
									Socket.cancel_blocking_socket_operation (t);

								// Sleep so other threads can resume
								Monitor.Wait (blocking_threads, 100);
							}
						}
					}
				}
			}
		}
	}
}

