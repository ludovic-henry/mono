// System.Net.Sockets.Socket.cs
//
// Authors:
//	Phillip Pearson (pp@myelin.co.nz)
//	Dick Porter <dick@ximian.com>
//	Gonzalo Paniagua Javier (gonzalo@ximian.com)
//	Sridhar Kulkarni (sridharkulkarni@gmail.com)
//	Brian Nickel (brian.nickel@gmail.com)
//	Ludovic Henry (ludovic@xamarin.com)
//
// Copyright (C) 2001, 2002 Phillip Pearson and Ximian, Inc.
//    http://www.myelin.co.nz
// (c) 2004-2011 Novell, Inc. (http://www.novell.com)
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

using System;
using System.Net;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Threading;
using System.Reflection;
using System.IO;
using System.Net.Configuration;
using System.Text;
using System.Timers;
using System.Net.NetworkInformation;

namespace System.Net.Sockets 
{
	public partial class Socket : IDisposable
	{
		/* the field "m_Handle" is looked up by name by the runtime */
		internal SafeSocketHandle m_Handle;

		/*
		 * This EndPoint is used when creating new endpoints. Because
		 * there are many types of EndPoints possible,
		 * seed_endpoint.Create(addr) is used for creating new ones.
		 * As such, this value is set on Bind, SentTo, ReceiveFrom,
		 * Connect, etc.
		 */
		internal EndPoint seed_endpoint = null;

		internal SemaphoreSlim ReadSem = new SemaphoreSlim (1, 1);
		internal SemaphoreSlim WriteSem = new SemaphoreSlim (1, 1);

		internal bool is_blocking = true;
		internal bool is_bound;

		/* When true, the socket was connected at the time of the last IO operation */
		internal bool is_connected;

		internal bool connect_in_progress;

		/* private constructor used by Accept, which already has a socket handle to use */
		internal Socket(AddressFamily family, SocketType type, ProtocolType proto, SafeSocketHandle safe_handle)
		{
			throw new NotSupportedException ();
		}

		internal static void Blocking_internal(IntPtr socket, bool block, out int error)
		{
			throw new NotSupportedException ();
		}

		internal void Accept (Socket acceptSocket)
		{
			throw new NotSupportedException ();
		}

		internal IAsyncResult BeginMConnect (SocketAsyncResult sockares)
		{
			throw new NotSupportedException ();
		}

		internal int ReceiveFrom (byte [] buffer, int offset, int size, SocketFlags socketFlags, ref EndPoint remoteEP, out SocketError errorCode)
		{
			throw new NotSupportedException ();
		}

		internal static void Close_internal (IntPtr socket, out int error)
		{
			throw new NotSupportedException ();
		}

		internal static void Shutdown_internal (IntPtr socket, SocketShutdown how, out int error)
		{
			throw new NotSupportedException ();
		}

		internal static void cancel_blocking_socket_operation (Thread thread)
		{
			throw new NotSupportedException ();
		}

		internal static bool SupportsPortReuse (ProtocolType proto)
		{
			throw new NotSupportedException ();
		}

		internal static int FamilyHint {
			get {
				throw new NotSupportedException ();
			}
		}
	}
}

