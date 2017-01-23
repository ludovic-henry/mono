
using System;
using System.Net;
using System.Net.Sockets;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Mono.PAL
{
	internal static partial class Sockets
	{
		static class Unix
		{
			[MethodImplAttribute(MethodImplOptions.InternalCall)]
			static extern IntPtr Socket (int family, int type, int protocol, out int error);

			internal static SafeSocketHandle Socket (AddressFamily family, SocketType type, ProtocolType protocol)
			{
				int error;
				SafeSocketHandle ret;
				try {} finally {
					/* Ensure we do not leak the fd in case of ThreadAbortException */
					ret = new SafeSocketHandle (Socket ((int) family, (int) type, (int) protocol, out error), true);
				}

				if (error != 0)
					throw new SocketException (error);

				return ret;
			}

			[MethodImplAttribute(MethodImplOptions.InternalCall)]
			static extern IntPtr Accept (IntPtr socket, out int error);

			internal static SafeSocketHandle Accept (SafeSocketHandle socket, bool blocking)
			{
				int error;
				SafeSocketHandle ret;
				try {} finally {
					/* Ensure we do not leak the fd in case of ThreadAbortException */
					ret = new SafeSocketHandle (Accept (socket.DangerousGetHandle (), out error), true);
				}

				if (error != 0)
					throw new SocketException (error);

				return ret;
			}

			[MethodImplAttribute(MethodImplOptions.InternalCall)]
			static extern SocketAddress GetLocalEndPoint (IntPtr socket, int family, out int error);

			internal static SocketAddress GetLocalEndPoint (SafeSocketHandle socket, AddressFamily family)
			{
				bool release = false;
				try {
					socket.DangerousAddRef (ref release);

					int error;
					SocketAddress ret = GetLocalEndPoint (socket.DangerousGetHandle (), (int) family, out error);
					if (error != 0)
						throw new SocketException (error);

					return ret;
				} finally {
					if (release)
						socket.DangerousRelease ();
				}
			}

			[MethodImplAttribute(MethodImplOptions.InternalCall)]
			static extern SocketAddress GetRemoteEndPoint (IntPtr socket, int family, out int error);

			internal static SocketAddress GetRemoteEndPoint (SafeSocketHandle socket, AddressFamily family)
			{
				bool release = false;
				try {
					socket.DangerousAddRef (ref release);

					int error;
					SocketAddress ret = GetRemoteEndPoint (socket.DangerousGetHandle (), (int) family, out error);
					if (error != 0)
						throw new SocketException (error);

					return ret;
				} finally {
					if (release)
						socket.DangerousRelease ();
				}
			}

			[MethodImplAttribute(MethodImplOptions.InternalCall)]
			static extern void SetBlocking (IntPtr socket, bool blocking, out int error);

			internal static void SetBlocking (SafeSocketHandle socket, bool blocking)
			{
				bool release = false;
				try {
					socket.DangerousAddRef (ref release);

					int error;
					SetBlocking (socket.DangerousGetHandle (), blocking, out error);
					if (error != 0)
						throw new SocketException (error);
				} finally {
					if (release)
						socket.DangerousRelease ();
				}
			}

			[MethodImplAttribute(MethodImplOptions.InternalCall)]
			static extern int GetAvailable (IntPtr socket, out int error);

			internal static int GetAvailable (SafeSocketHandle socket)
			{
				bool release = false;
				try {
					socket.DangerousAddRef (ref release);

					int error;
					int ret = GetAvailable (socket.DangerousGetHandle (), out error);
					if (error != 0)
						throw new SocketException (error);

					return ret;
				} finally {
					if (release)
						socket.DangerousRelease ();
				}
			}

			[MethodImplAttribute(MethodImplOptions.InternalCall)]
			static extern void Bind (IntPtr socket, SocketAddress addr, out int error);

			internal static void Bind (SafeSocketHandle socket, SocketAddress addr)
			{
				bool release = false;
				try {
					socket.DangerousAddRef (ref release);

					int error;
					Bind (socket.DangerousGetHandle (), addr, out error);
					if (error != 0)
						throw new SocketException (error);
				} finally {
					if (release)
						socket.DangerousRelease ();
				}
			}

			[MethodImplAttribute(MethodImplOptions.InternalCall)]
			static extern void Connect (IntPtr socket, SocketAddress addr, out int error);

			internal static void Connect (SafeSocketHandle socket, SocketAddress addr, bool blocking)
			{
				int error;
				Connect (socket.DangerousGetHandle (), addr, out error);
				if (error != 0)
					throw new SocketException (error);
			}

			[MethodImplAttribute(MethodImplOptions.InternalCall)]
			static extern int Receive (IntPtr socket, IntPtr buffer, int count, int flags, out int error);

			internal static int Receive (SafeSocketHandle socket, IntPtr buffer, int count, SocketFlags flags, bool blocking)
			{
				int error;
				int ret = Receive (socket.DangerousGetHandle (), buffer, count, (int) flags, out error);
				if (error != 0)
					throw new SocketException (error);

				return ret;
			}

			[MethodImplAttribute(MethodImplOptions.InternalCall)]
			static extern int ReceiveBuffers (IntPtr socket, IntPtr buffers, int count, int flags, out int error);

			internal static int ReceiveBuffers (SafeSocketHandle socket, IntPtr buffers, int count, SocketFlags flags, bool blocking)
			{
				int error;
				int ret = ReceiveBuffers (socket.DangerousGetHandle (), buffers, count, (int) flags, out error);
				if (error != 0)
					throw new SocketException (error);

				return ret;
			}

			[MethodImplAttribute(MethodImplOptions.InternalCall)]
			static extern int ReceiveFrom (IntPtr socket, IntPtr buffer, int count, int flags, ref SocketAddress addr, out int error);

			internal static int ReceiveFrom (SafeSocketHandle socket, IntPtr buffer, int count, SocketFlags flags, ref SocketAddress addr, bool blocking)
			{
				int error;
				int ret = ReceiveFrom (socket.DangerousGetHandle (), buffer, count, (int) flags, ref addr, out error);
				if (error != 0)
					throw new SocketException (error);

				return ret;
			}

			[MethodImplAttribute(MethodImplOptions.InternalCall)]
			static extern int Send (IntPtr socket, IntPtr buffer, int count, int flags, out int error);

			internal static int Send (SafeSocketHandle socket, IntPtr buffer, int count, SocketFlags flags, bool blocking)
			{
				int error;
				int ret = Send (socket.DangerousGetHandle (), buffer, count, (int) flags, out error);
				if (error != 0)
					throw new SocketException (error);

				return ret;
			}

			[MethodImplAttribute(MethodImplOptions.InternalCall)]
			static extern int SendBuffers (IntPtr socket, IntPtr buffers, int count, int flags, out int error);

			internal static int SendBuffers (SafeSocketHandle socket, IntPtr buffers, int count, SocketFlags flags, bool blocking)
			{
				int error;
				int ret = SendBuffers (socket.DangerousGetHandle (), buffers, count, (int) flags, out error);
				if (error != 0)
					throw new SocketException (error);

				return ret;
			}

			[MethodImplAttribute(MethodImplOptions.InternalCall)]
			static extern int SendTo (IntPtr socket, IntPtr buffer, int count, int flags, SocketAddress addr, out int error);

			internal static int SendTo (SafeSocketHandle socket, IntPtr buffer, int count, SocketFlags flags, SocketAddress addr, bool blocking)
			{
				int error;
				int ret = SendTo (socket.DangerousGetHandle (), buffer, count, (int) flags, addr, out error);
				if (error != 0)
					throw new SocketException (error);

				return ret;
			}

			[MethodImplAttribute(MethodImplOptions.InternalCall)]
			static extern void SendFile (IntPtr socket, IntPtr file, IntPtr buffers, int flags, out int error);

			internal static void SendFile (SafeSocketHandle socket, IntPtr file, IntPtr buffers, TransmitFileOptions flags)
			{
				int error;
				SendFile (socket.DangerousGetHandle (), file, buffers, (int) flags, out error);
				if (error != 0)
					throw new SocketException (error);
			}
		}
	}
}