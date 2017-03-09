// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Threading;
using Microsoft.Win32.SafeHandles;
using System.Diagnostics;

namespace System.Net.Sockets
{
	internal partial class SafeCloseSocket :
#if DEBUG
		DebugSafeHandleMinusOneIsInvalid
#else
		SafeHandleMinusOneIsInvalid
#endif
	{
		// Unix

		public SocketAsyncContext AsyncContext
		{
			get
			{
				if (Environment.IsRunningOnWindows)
					throw new PlatformNotSupportedException();
				else
					return Unix_AsyncContext;
			}
		}


		public bool IsNonBlocking
		{
			get
			{
				if (Environment.IsRunningOnWindows)
					throw new PlatformNotSupportedException();
				else
					return Unix_IsNonBlocking;
			}
			set
			{
				if (Environment.IsRunningOnWindows)
					throw new PlatformNotSupportedException();
				else
					Unix_IsNonBlocking = value;
			}
		}

		public int ReceiveTimeout
		{
			get
			{
				if (Environment.IsRunningOnWindows)
					throw new PlatformNotSupportedException();
				else
					return Unix_ReceiveTimeout;
			}
			set
			{
				if (Environment.IsRunningOnWindows)
					throw new PlatformNotSupportedException();
				else
					Unix_ReceiveTimeout = value;
			}
		}

		public int SendTimeout
		{
			get
			{
				if (Environment.IsRunningOnWindows)
					throw new PlatformNotSupportedException();
				else
					return Unix_SendTimeout;
			}
			set
			{
				if (Environment.IsRunningOnWindows)
					throw new PlatformNotSupportedException();
				else
					Unix_SendTimeout = value;
			}
		}

		public static unsafe SafeCloseSocket CreateSocket(IntPtr fileDescriptor)
		{
			if (Environment.IsRunningOnWindows)
				throw new PlatformNotSupportedException();
			else
				return Unix_CreateSocket(fileDescriptor);
		}

		public static unsafe SocketError CreateSocket(AddressFamily addressFamily, SocketType socketType, ProtocolType protocolType, out SafeCloseSocket socket)
		{
			if (Environment.IsRunningOnWindows)
				throw new PlatformNotSupportedException();
			else
				return Unix_CreateSocket(addressFamily, socketType, protocolType, out socket);
		}

		public static unsafe SocketError Accept(SafeCloseSocket socketHandle, byte[] socketAddress, ref int socketAddressSize, out SafeCloseSocket socket)
		{
			if (Environment.IsRunningOnWindows)
				throw new PlatformNotSupportedException();
			else
				return Unix_Accept(socketHandle, socketAddress, ref socketAddressSize, out socket);
		}

		// Windows

		public ThreadPoolBoundHandle IOCPBoundHandle
		{
			get
			{
				if (Environment.IsRunningOnWindows)
					return Windows_IOCPBoundHandle;
				else
					throw new PlatformNotSupportedException();
			}
		}

		public bool SkipCompletionPortOnSuccess
		{
			get
			{
				if (Environment.IsRunningOnWindows)
					return Windows_SkipCompletionPortOnSuccess;
				else
					throw new PlatformNotSupportedException();
			}
		}

		public ThreadPoolBoundHandle GetOrAllocateThreadPoolBoundHandle(bool trySkipCompletionPortOnSuccess)
		{
			if (Environment.IsRunningOnWindows)
				return Windows_GetOrAllocateThreadPoolBoundHandle(trySkipCompletionPortOnSuccess);
			else
				throw new PlatformNotSupportedException();
		}

		internal static unsafe SafeCloseSocket CreateWSASocket(byte* pinnedBuffer)
		{
			if (Environment.IsRunningOnWindows)
				return Windows_CreateWSASocket(pinnedBuffer);
			else
				throw new PlatformNotSupportedException();
		}

		internal static SafeCloseSocket CreateWSASocket(AddressFamily addressFamily, SocketType socketType, ProtocolType protocolType)
		{
			if (Environment.IsRunningOnWindows)
				return Windows_CreateWSASocket(addressFamily, socketType, protocolType);
			else
				throw new PlatformNotSupportedException();
		}

		internal static SafeCloseSocket Accept(SafeCloseSocket socketHandle, byte[] socketAddress, ref int socketAddressSize)
		{
			if (Environment.IsRunningOnWindows)
				return Windows_Accept(socketHandle, socketAddress, ref socketAddressSize);
			else
				throw new PlatformNotSupportedException();
		}

		// Unix + Windows

		private void InnerReleaseHandle()
		{
			if (Environment.IsRunningOnWindows)
				Windows_InnerReleaseHandle();
			else
				Unix_InnerReleaseHandle();
		}

		internal sealed partial class InnerSafeCloseSocket : SafeHandleMinusOneIsInvalid
		{
			// Unix

			public static InnerSafeCloseSocket CreateSocket(IntPtr fileDescriptor)
			{
				if (Environment.IsRunningOnWindows)
					throw new PlatformNotSupportedException();
				else
					return Unix_CreateSocket(fileDescriptor);
			}

			public static unsafe InnerSafeCloseSocket CreateSocket(AddressFamily addressFamily, SocketType socketType, ProtocolType protocolType, out SocketError errorCode)
			{
				if (Environment.IsRunningOnWindows)
					throw new PlatformNotSupportedException();
				else
					return Unix_CreateSocket(addressFamily, socketType, protocolType, out errorCode);
			}

			public static unsafe InnerSafeCloseSocket Accept(SafeCloseSocket socketHandle, byte[] socketAddress, ref int socketAddressLen, out SocketError errorCode)
			{
				if (Environment.IsRunningOnWindows)
					throw new PlatformNotSupportedException();
				else
					return Unix_Accept(socketHandle, socketAddress, ref socketAddressLen, out errorCode);
			}

			// Windows

			internal static unsafe InnerSafeCloseSocket CreateWSASocket(byte* pinnedBuffer)
			{
				if (Environment.IsRunningOnWindows)
					return Windows_CreateWSASocket(pinnedBuffer);
				else
					throw new PlatformNotSupportedException();
			}

			internal static InnerSafeCloseSocket CreateWSASocket(AddressFamily addressFamily, SocketType socketType, ProtocolType protocolType)
			{
				if (Environment.IsRunningOnWindows)
					return Windows_CreateWSASocket(addressFamily, socketType, protocolType);
				else
					throw new PlatformNotSupportedException();
			}

			internal static InnerSafeCloseSocket Accept(SafeCloseSocket socketHandle, byte[] socketAddress, ref int socketAddressSize)
			{
				if (Environment.IsRunningOnWindows)
					return Windows_Accept(socketHandle, socketAddress, ref socketAddressSize);
				else
					throw new PlatformNotSupportedException();
			}

			// Unix + Windows

			private unsafe SocketError InnerReleaseHandle()
			{
				if (Environment.IsRunningOnWindows)
					return Windows_InnerReleaseHandle();
				else
					return Unix_InnerReleaseHandle();
			}
		}
	}
}
