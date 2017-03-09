// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System;
using System.Diagnostics;
using System.Net;
using System.Runtime.InteropServices;
using System.Threading;
using Microsoft.Win32;

namespace System.Net.Sockets
{
	internal partial class BaseOverlappedAsyncResult : ContextAwareResult
	{
		// Unix

		protected void CompletionCallback(int numBytes, SocketError errorCode)
		{
			if (Environment.IsRunningOnWindows)
				throw new PlatformNotSupportedException();
			else
				Unix_CompletionCallback(numBytes, errorCode);
		}

		// Windows

		internal void SetUnmanagedStructures(object objectsToPin)
		{
			if (Environment.IsRunningOnWindows)
				Windows_SetUnmanagedStructures(objectsToPin);
			else
				throw new PlatformNotSupportedException();
		}

		internal SafeHandle OverlappedHandle
		{
			get
			{
				if (Environment.IsRunningOnWindows)
					return Windows_OverlappedHandle;
				else
					throw new PlatformNotSupportedException();
			}
		}

		internal SocketError ProcessOverlappedResult(bool success, int bytesTransferred)
		{
			if (Environment.IsRunningOnWindows)
				return Windows_ProcessOverlappedResult(success, bytesTransferred);
			else
				throw new PlatformNotSupportedException();
		}

		internal void ReleaseUnmanagedStructures()
		{
			if (Environment.IsRunningOnWindows)
				Windows_ReleaseUnmanagedStructures();
			else
				throw new PlatformNotSupportedException();
		}

		protected override void Cleanup()
		{
			if (Environment.IsRunningOnWindows)
				Windows_Cleanup();
			else
				base.Cleanup();
		}

		protected virtual void ForceReleaseUnmanagedStructures()
		{
			if (Environment.IsRunningOnWindows)
				Windows_ForceReleaseUnmanagedStructures();
			else
				throw new PlatformNotSupportedException();
		}

		// Unix + Windows

		public BaseOverlappedAsyncResult(Socket socket, Object asyncState, AsyncCallback asyncCallback)
			: base(socket, asyncState, asyncCallback)
		{
			if (Environment.IsRunningOnWindows)
				Windows_BaseOverlappedAsyncResult(socket, asyncState, asyncCallback);
			else
				Unix_BaseOverlappedAsyncResult(socket, asyncState, asyncCallback);
		}
	}
}
