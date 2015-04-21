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

namespace System.Net.Sockets
{
	[StructLayout (LayoutKind.Sequential)]
	internal sealed class SocketAsyncResult : IOAsyncResult
	{
		Exception delayed_exception;

		bool is_blocking;

		Socket accept_socket;
		int total;

		public Socket socket;

		public SocketOperation socket_operation;

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

		public int error;
		public int EndCalled;

		public SocketAsyncWorker Worker;

		public SocketAsyncResult ()
		{
		}

		public SocketAsyncResult (Socket socket, object state, AsyncCallback callback, SocketOperation operation)
		{
			Init (socket, state, callback, operation, new SocketAsyncWorker (this));
		}

		public Socket Socket {
			get {
				return accept_socket;
			}
		}

		public int Total {
			get { return total; }
			set { total = value; }
		}

		public SocketError ErrorCode {
			get {
				SocketException ex = delayed_exception as SocketException;
				if (ex != null)
					return ex.SocketErrorCode;

				if (error != 0)
					return (SocketError) error;

				return SocketError.Success;
			}
		}

		public void Init (Socket socket, object state, AsyncCallback callback, SocketOperation operation, SocketAsyncWorker worker)
		{
			this.socket = socket;
			this.is_blocking = socket != null ? socket.is_blocking : true;
			this.handle = socket != null ? socket.Handle : IntPtr.Zero;
			this.state = state;
			this.async_callback = callback;
			this.socket_operation = operation;

			switch (this.socket_operation) {
			case SocketOperation.Accept:
			case SocketOperation.AcceptReceive:
			case SocketOperation.Receive:
			case SocketOperation.ReceiveFrom:
			case SocketOperation.ReceiveGeneric:
			case SocketOperation.RecvJustCallback:
				this.operation = IOOperation.In;
				break;
			case SocketOperation.Connect:
			case SocketOperation.Disconnect:
			case SocketOperation.Send:
			case SocketOperation.SendGeneric:
			case SocketOperation.SendJustCallback:
			case SocketOperation.SendTo:
				this.operation = IOOperation.Out;
				break;
			default:
				throw new NotImplementedException ();
			}

			if (wait_handle != null)
				wait_handle.Reset ();

			delayed_exception = null;

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
			accept_socket = null;
			total = 0;

			completed_synchronously = false;
			completed = false;
			is_blocking = false;
			error = 0;
			async_result = null;
			EndCalled = 0;
			Worker = worker;
		}

		public void DoMConnectCallback ()
		{
			if (async_callback == null)
				return;
			ThreadPool.UnsafeQueueUserWorkItem (_ => async_callback (this), null);
		}

		public void Dispose ()
		{
			Init (null, null, null, 0, Worker);
			if (wait_handle != null) {
				wait_handle.Close ();
				wait_handle = null;
			}
		}

		public void CheckIfThrowDelayedException ()
		{
			if (delayed_exception != null) {
				socket.is_connected = false;
				throw delayed_exception;
			}

			if (error != 0) {
				socket.is_connected = false;
				throw new SocketException (error);
			}
		}

		void CompleteDisposed (object unused)
		{
			Complete ();
		}

		public void Complete ()
		{
			if (socket_operation != SocketOperation.Receive && socket.is_disposed)
				delayed_exception = new ObjectDisposedException (socket.GetType ().ToString ());

			completed = true;
			lock (this) {
				if (wait_handle != null)
					wait_handle.Set ();
			}

			Queue<SocketAsyncWorker> queue = null;
			switch (socket_operation) {
			case SocketOperation.Receive:
			case SocketOperation.ReceiveFrom:
			case SocketOperation.ReceiveGeneric:
			case SocketOperation.Accept:
				queue = socket.readQ;
				break;
			case SocketOperation.Send:
			case SocketOperation.SendTo:
			case SocketOperation.SendGeneric:
				queue = socket.writeQ;
				break;
			}

			if (queue != null) {
				lock (queue) {
					/* queue.Count will only be 0 if the socket is closed while receive/send/accept
					 * operation(s) are pending and at least one call to this method is waiting
					 * on the lock while another one calls CompleteAllOnDispose() */
					if (queue.Count > 0)
						queue.Dequeue (); /* remove ourselves */
					if (queue.Count > 0) {
						if (!socket.is_disposed) {
							Socket.socket_pool_queue (SocketAsyncWorker.Dispatcher, (queue.Peek ()).result);
						} else {
							/* CompleteAllOnDispose */
							SocketAsyncWorker [] workers = queue.ToArray ();
							for (int i = 0; i < workers.Length; i++)
								ThreadPool.UnsafeQueueUserWorkItem (workers [i].result.CompleteDisposed, null);
							queue.Clear ();
						}
					}
				}
			}

			// IMPORTANT: 'async_callback', if any is scheduled from unmanaged code
		}

		public void Complete (bool synch)
		{
			this.completed_synchronously = synch;
			Complete ();
		}

		public void Complete (int total)
		{
			this.total = total;
			Complete ();
		}

		public void Complete (Exception e, bool synch)
		{
			this.completed_synchronously = synch;
			this.delayed_exception = e;
			Complete ();
		}

		public void Complete (Exception e)
		{
			this.delayed_exception = e;
			Complete ();
		}

		public void Complete (Socket s)
		{
			this.accept_socket = s;
			Complete ();
		}

		public void Complete (Socket s, int total)
		{
			this.accept_socket = s;
			this.total = total;
			Complete ();
		}

		protected override void Invoke()
		{
			async_result.Invoke ();

			if (completed && async_callback != null) {
				ThreadPool.UnsafeQueueCustomWorkItem (new AsyncResult (state => async_callback ((IAsyncResult) state), this, false), false);
			}
		}
	}
}
