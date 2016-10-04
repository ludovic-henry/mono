// System.Net.Sockets.SocketAsyncResult.cs
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

using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Runtime.Remoting.Messaging;
using System.Threading;
using System.Threading.Tasks;

namespace System.Net.Sockets
{
	[StructLayout (LayoutKind.Sequential)]
	internal sealed class SocketAsyncResult: IOAsyncResult
	{
		public Socket socket;
		public SocketOperation operation;

		Exception DelayedException;

		public EndPoint EndPoint;                 // Connect,ReceiveFrom,SendTo
		public byte [] Buffer;                    // Receive,ReceiveFrom,Send,SendTo
		public int Offset;                        // Receive,ReceiveFrom,Send,SendTo
		public int Size;                          // Receive,ReceiveFrom,Send,SendTo
		public SocketFlags SockFlags;             // Receive,ReceiveFrom,Send,SendTo
		public Socket AcceptSocket;               // AcceptReceive
		public IPAddress[] Addresses;             // Connect
		public int Port;                          // Connect
		public IList<ArraySegment<byte>> Buffers; // Receive, Send
		public bool ReuseSocket;                  // Disconnect
		public int CurrentAddress;                // Connect

		public Socket AcceptedSocket;
		public int Total;

		internal int error;

		public int EndCalled;

		public IntPtr Handle {
			get { return socket != null ? socket.Handle : IntPtr.Zero; }
		}

		/* Used by SocketAsyncEventArgs */
		public SocketAsyncResult ()
			: base ()
		{
		}

		public void Init (Socket socket, AsyncCallback callback, object state, SocketOperation operation)
		{
			base.Init (callback, state);

			this.socket = socket;
			this.operation = operation;

			DelayedException = null;

			EndPoint = null;
			Buffer = null;
			Offset = 0;
			Size = 0;
			SockFlags = SocketFlags.None;
			AcceptSocket = null;
			Addresses = null;
			Port = 0;
			Buffers = null;
			ReuseSocket = false;
			CurrentAddress = 0;

			AcceptedSocket = null;
			Total = 0;

			error = 0;

			EndCalled = 0;
		}

		public SocketAsyncResult (Socket socket, AsyncCallback callback, object state, SocketOperation operation)
			: base (callback, state)
		{
			this.socket = socket;
			this.operation = operation;
		}

		public SocketError ErrorCode {
			get {
				SocketException ex = DelayedException as SocketException;
				if (ex != null)
					return ex.SocketErrorCode;

				if (error != 0)
					return (SocketError) error;

				return SocketError.Success;
			}
		}

		public void CheckIfThrowDelayedException ()
		{
			if (DelayedException != null) {
				socket.is_connected = false;
				throw DelayedException;
			}

			if (error != 0) {
				socket.is_connected = false;
				throw new SocketException (error);
			}
		}

		internal override void SetCanceled ()
		{
			Complete ();
		}

		public void Complete ()
		{
			if (operation != SocketOperation.Receive && socket.CleanedUp)
				DelayedException = new ObjectDisposedException (socket.GetType ().ToString ());

			IsCompleted = true;

			AsyncCallback callback = AsyncCallback;
			if (callback != null) {
				ThreadPool.UnsafeQueueUserWorkItem (_ => callback (this), null);
			}

			switch (operation) {
			case SocketOperation.Receive:
			case SocketOperation.ReceiveFrom:
			case SocketOperation.ReceiveGeneric:
			case SocketOperation.Accept:
				socket.ReadSem.Release ();
				break;
			case SocketOperation.Send:
			case SocketOperation.SendTo:
			case SocketOperation.SendGeneric:
				socket.WriteSem.Release ();
				break;
			}

			// IMPORTANT: 'callback', if any is scheduled from unmanaged code
		}

		public void Complete (bool synch)
		{
			CompletedSynchronously = synch;
			Complete ();
		}

		public void Complete (int total)
		{
			Total = total;
			Complete ();
		}

		public void Complete (Exception e, bool synch)
		{
			DelayedException = e;
			CompletedSynchronously = synch;
			Complete ();
		}

		public void Complete (Exception e)
		{
			DelayedException = e;
			Complete ();
		}

		public void Complete (Socket s)
		{
			AcceptedSocket = s;
			Complete ();
		}

		public void Complete (Socket s, int total)
		{
			AcceptedSocket = s;
			Total = total;
			Complete ();
		}
	}

	abstract class BaseSocketTaskAsyncResult : IAsyncResult
	{
		public AsyncCallback AsyncCallback { get; }

		public object AsyncState { get; }

		public int EndCalled = 0;

		protected abstract IAsyncResult TaskAsIAsyncResult { get; }

		public WaitHandle AsyncWaitHandle
		{
			get { return TaskAsIAsyncResult.AsyncWaitHandle; }
		}

		public bool IsCompleted
		{
			get { return TaskAsIAsyncResult.IsCompleted; }
		}

		public bool CompletedSynchronously
		{
			get { return TaskAsIAsyncResult.CompletedSynchronously; }
		}

		protected BaseSocketTaskAsyncResult (AsyncCallback callback, object state)
		{
			AsyncCallback = callback;
			AsyncState = state;
		}
	}

	abstract class SocketTaskAsyncResult : BaseSocketTaskAsyncResult
	{
		public Task Task { get; protected set; }

		protected override IAsyncResult TaskAsIAsyncResult
		{
			get { return (IAsyncResult) Task; }
		}

		public SocketTaskAsyncResult (AsyncCallback callback, object state)
			: base (callback, state)
		{
		}

		protected async Task CreateTask<T> (Func<T, Task> func) where T : SocketTaskAsyncResult
		{
			try {
				await func ((T) this);
			} finally {
				if (AsyncCallback != null)
					ThreadPool.UnsafeQueueUserWorkItem (o => ((SocketTaskAsyncResult) o).AsyncCallback ((IAsyncResult) o), this);
			}
		}
	}

	abstract class SocketTaskAsyncResult<TResult> : BaseSocketTaskAsyncResult
	{
		public Task<TResult> Task { get; protected set; }

		protected override IAsyncResult TaskAsIAsyncResult
		{
			get { return (IAsyncResult) Task; }
		}

		public TResult Result
		{
			get { return Task.Result; }
		}

		public SocketTaskAsyncResult (AsyncCallback callback, object state)
			: base (callback, state)
		{
		}

		protected async Task<TResult> CreateTask<T> (Func<T, Task<TResult>> func) where T : SocketTaskAsyncResult<TResult>
		{
			try {
				return await func ((T) this);
			} finally {
				if (AsyncCallback != null)
					ThreadPool.UnsafeQueueUserWorkItem (o => ((SocketTaskAsyncResult <TResult>) o).AsyncCallback ((IAsyncResult) o), this);
			}
		}
	}
}
