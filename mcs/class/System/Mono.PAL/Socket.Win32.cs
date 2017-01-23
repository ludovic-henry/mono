
using System;
using System.Net;
using System.Net.Sockets;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Mono.PAL
{
	internal static partial class Sockets
	{
		static class Win32
		{
			[DllImport ("Kernel32", CharSet = CharSet.Auto, SetLastError = true)]
			static extern SafeSocketHandle WSASocket (AddressFamily family, SocketType type, ProtocolType protocol, IntPtr info, uint group, uint flags);

			internal static SafeSocketHandle Socket (AddressFamily family, SocketType type, ProtocolType protocol)
			{
				SafeSocketHandle socket = WSASocket (family, type, protocol, IntPtr.Zero, 0, 0x01 /* WSA_FLAG_OVERLAPPED */);
				if (socket.IsInvalid)
					throw new SocketException (Marshal.GetLastWin32Error ());

				return socket;
			}

			[MethodImplAttribute(MethodImplOptions.InternalCall)]
			static extern IntPtr Accept (IntPtr socket, bool blocking, out int error);

			internal static SafeSocketHandle Accept (SafeSocketHandle socket, bool blocking)
			{
				int error;
				SafeSocketHandle ret;
				try {} finally {
					/* Ensure we do not leak the fd in case of ThreadAbortException */
					ret = new SafeSocketHandle (Accept (socket.DangerousGetHandle (), blocking, out error), true);
				}

				if (error != 0)
					throw new SocketException (error);

				return ret;
			}

			[DllImport ("Kernel32", CharSet = CharSet.Auto, SetLastError = true)]
			static extern int getsockname (SafeSocketHandle socket, IntPtr sa, ref int sa_size);

			internal static SocketAddress GetLocalEndPoint (SafeSocketHandle socket, AddressFamily family)
			{
				unsafe {
					IntPtr sa = IntPtr.Zero;

					int sa_size;
					switch (family) {
					case AddressFamily.InterNetwork:
						sa_size = sizeof (sockaddr_in);
						break;
					case AddressFamily.InterNetworkV6:
						sa_size = sizeof (sockaddr_in6);
						break;
					case AddressFamily.Unix:
						sa_size = sizeof (sockaddr_un);
						break;
					default:
						throw new SocketException (SocketError.AddressFamilyNotSupported);
					}

					try {
						AllocSockaddr (ref sa, sa_size);

						int res = getsockname (socket, sa, ref sa_size);
						if (res == -1)
							throw new SocketException (Marshal.GetLastWin32Error ());

						return CreateSocketAddressFromSockaddr (sa);
					} finally {
						if (sa != IntPtr.Zero)
							Marshal.FreeHGlobal (sa);
					}
				}
			}

			[DllImport ("Kernel32", CharSet = CharSet.Auto, SetLastError = true)]
			static extern int getpeername (SafeSocketHandle socket, IntPtr sa, ref int sa_size);

			internal static SocketAddress GetRemoteEndPoint (SafeSocketHandle socket, AddressFamily family)
			{
				unsafe {
					IntPtr sa = IntPtr.Zero;

					int sa_size;
					switch (family) {
					case AddressFamily.InterNetwork:
						sa_size = sizeof (sockaddr_in);
						break;
					case AddressFamily.InterNetworkV6:
						sa_size = sizeof (sockaddr_in6);
						break;
					case AddressFamily.Unix:
						sa_size = sizeof (sockaddr_un);
						break;
					default:
						throw new SocketException (SocketError.AddressFamilyNotSupported);
					}

					try {
						AllocSockaddr (ref sa, sa_size);

						int res = getpeername (socket, sa, ref sa_size);
						if (res == -1)
							throw new SocketException (Marshal.GetLastWin32Error ());

						return CreateSocketAddressFromSockaddr (sa);
					} finally {
						if (sa != IntPtr.Zero)
							Marshal.FreeHGlobal (sa);
					}
				}
			}

			[DllImport ("Kernel32", CharSet = CharSet.Auto, SetLastError = true)]
			static extern int ioctlsocket (SafeSocketHandle socket, int command, ref uint value);

			internal static void SetBlocking (SafeSocketHandle socket, bool blocking)
			{
				uint nonBlocking = blocking ? 0u : 1u;
				int res = ioctlsocket (socket, FIONBIO, ref nonBlocking);
				if (res == -1)
					throw new SocketException (Marshal.GetLastWin32Error ());
			}

			internal static int GetAvailable (SafeSocketHandle socket)
			{
				uint amount = 0;
				int res = ioctlsocket (socket, FIONREAD, ref amount);
				if (res == -1)
					throw new SocketException (Marshal.GetLastWin32Error ());

				return (int) amount;
			}

			[DllImport ("Kernel32", CharSet = CharSet.Auto, SetLastError = true)]
			static extern int bind (SafeSocketHandle socket, IntPtr sa, int sa_size);

			internal static void Bind (SafeSocketHandle socket, SocketAddress addr)
			{
				IntPtr sa = IntPtr.Zero;
				int sa_size;

				try {
					CreateSockaddrFromSocketAddress (addr, ref sa, out sa_size);

					int res = bind (socket, sa, sa_size);
					if (res == -1)
						throw new SocketException (Marshal.GetLastWin32Error ());
				} finally {
					if (sa != IntPtr.Zero)
						Marshal.FreeHGlobal (sa);
				}
			}

			[MethodImplAttribute(MethodImplOptions.InternalCall)]
			static extern int Connect (IntPtr socket, IntPtr sa, int sa_size, bool blocking, out int error);

			internal static void Connect (SafeSocketHandle socket, SocketAddress addr, bool blocking)
			{
				IntPtr sa = IntPtr.Zero;
				int sa_size;

				try {
					CreateSockaddrFromSocketAddress (addr, ref sa, out sa_size);

					int error;
					Connect (socket.DangerousGetHandle (), sa, sa_size, blocking, out error);
					if (error != 0)
						throw new SocketException (error);
				} finally {
					if (sa != IntPtr.Zero)
						Marshal.FreeHGlobal (sa);
				}
			}

			[MethodImplAttribute(MethodImplOptions.InternalCall)]
			static extern int Receive (IntPtr socket, IntPtr buffer, int len, int flags, bool blocking, out int error);

			internal static int Receive (SafeSocketHandle socket, IntPtr buffer, int count, SocketFlags flags, bool blocking)
			{
				int error;
				int ret = Receive (socket.DangerousGetHandle (), buffer, count, (int) flags, blocking, out error);
				if (error != 0)
					throw new SocketException (error);

				return ret;
			}

			[MethodImplAttribute(MethodImplOptions.InternalCall)]
			static extern int ReceiveBuffers(IntPtr socket, IntPtr buffers, int count, int flags, bool blocking, out int error);

			internal static int ReceiveBuffers (SafeSocketHandle socket, IntPtr buffers, int count, SocketFlags flags, bool blocking)
			{
				int error;
				int ret = ReceiveBuffers (socket.DangerousGetHandle (), buffers, count, (int) flags, blocking, out error);
				if (error != 0)
					throw new SocketException (error);

				return ret;
			}

			[MethodImplAttribute(MethodImplOptions.InternalCall)]
			static extern int ReceiveFrom (IntPtr socket, IntPtr buffer, int len, int flags, IntPtr sa, int sa_size, bool blocking, out int error);

			internal static int ReceiveFrom (SafeSocketHandle socket, IntPtr buffer, int count, SocketFlags flags, ref SocketAddress addr, bool blocking)
			{
				IntPtr sa = IntPtr.Zero;
				int sa_size;

				try {
					CreateSockaddrFromSocketAddress (addr, ref sa, out sa_size);

					int error;
					int ret = ReceiveFrom (socket.DangerousGetHandle (), buffer, count, (int) flags, sa, sa_size, blocking, out error);
					if (error != 0)
						throw new SocketException (error);

					addr = sa_size != 0 ? CreateSocketAddressFromSockaddr (sa) : null;
					return ret;
				} finally {
					if (sa != IntPtr.Zero)
						Marshal.FreeHGlobal (sa);
				}
			}

			[MethodImplAttribute(MethodImplOptions.InternalCall)]
			static extern int Send (IntPtr socket, IntPtr buffer, int len, int flags, bool blocking, out int error);

			internal static int Send (SafeSocketHandle socket, IntPtr buffer, int count, SocketFlags flags, bool blocking)
			{
				int error;
				int ret = Send (socket.DangerousGetHandle (), buffer, count, (int) flags, blocking, out error);
				if (error != 0)
					throw new SocketException (error);

				return ret;
			}

			[MethodImplAttribute(MethodImplOptions.InternalCall)]
			static extern int SendBuffers(IntPtr socket, IntPtr buffers, int count, int flags, bool blocking, out int error);

			internal static int SendBuffers (SafeSocketHandle socket, IntPtr buffers, int count, SocketFlags flags, bool blocking)
			{
				int error;
				int ret = SendBuffers (socket.DangerousGetHandle (), buffers, count, (int) flags, blocking, out error);
				if (error != 0)
					throw new SocketException (error);

				return ret;
			}

			[MethodImplAttribute(MethodImplOptions.InternalCall)]
			static extern int SendTo (IntPtr socket, IntPtr buffer, int len, int flags, IntPtr sa, int sa_size, bool blocking, out int error);

			internal static int SendTo (SafeSocketHandle socket, IntPtr buffer, int count, SocketFlags flags, SocketAddress addr, bool blocking)
			{
				IntPtr sa = IntPtr.Zero;
				int sa_size;

				try {
					CreateSockaddrFromSocketAddress (addr, ref sa, out sa_size);

					int error;
					int ret = SendTo (socket.DangerousGetHandle (), buffer, count, (int) flags, sa, sa_size, blocking, out error);
					if (error != 0)
						throw new SocketException (error);

					return ret;
				} finally {
					if (sa != IntPtr.Zero)
						Marshal.FreeHGlobal (sa);
				}
			}

			[MethodImplAttribute(MethodImplOptions.InternalCall)]
			static extern void SendFile (IntPtr socket, IntPtr file, IntPtr buffers, int flags, out int error);

			internal static void SendFile (IntPtr socket, IntPtr file, IntPtr buffers, TransmitFileOptions flags)
			{
				int error;
				SendFile (socket, file, buffers, (int) flags, out error);
				if (error != 0)
					throw new SocketException (error);
			}

			[DllImport ("Kernel32", CharSet = CharSet.Auto, SetLastError = true)]
			static extern int select (int __ignored, IntPtr[] readfds, IntPtr[] writefds, IntPtr[] exceptfds, ref timeval timeout);

			internal static bool Poll (IntPtr socket, int ustimeout, SelectMode mode)
			{
				/* fd_set is as follow:
				 *  - IntPtr fd_count
				 *  - IntPtr[] fd_array */
				IntPtr[] fd_set = new IntPtr[2] { (IntPtr) 1, socket };

				timeval timeout = new timeval {
					tv_sec  = ustimeout / 1000 / 1000,
					tv_usec = ustimeout % (1000 * 1000),
				};

				unsafe {
					fixed (IntPtr* fds = fd_set) {
						int ret = select (0, mode == SelectMode.SelectRead ? fds : null, mode == SelectMode.SelectWrite ? fds : null, mode == SelectMode.SelectError ? fds : null, ref timeout);
						if (ret == (int) SocketError.SocketError)
							throw new SocketException (Marshal.GetLastWin32Error ());
					}
				}

				return fd_set [0] != IntPtr.Zero && fd_set [1] == socket;
			}

			const int FIONREAD = unchecked ((int) 0x4004667F);
			const int FIONBIO  = unchecked ((int) 0x8004667E);

			static void AllocSockaddr (ref IntPtr sa, int sa_size)
			{
				try {} finally {
					/* Ensure we do not leak memory in case of ThreadAbortException */
					sa = Marshal.AllocHGlobal (sa_size);
				}
			}

			static void CreateSockaddrFromSocketAddress (SocketAddress addr, ref IntPtr sa, out int sa_size)
			{
				unsafe {
					IPEndPoint ep = addr.GetIPEndPoint ();

					switch (addr.Family) {
					case AddressFamily.InterNetwork: {
						byte[] address = ep.Address.GetAddressBytes ();
						int port = ep.Port;

						AllocSockaddr (ref sa, sa_size = sizeof (sockaddr_in));

						sockaddr_in *sa_in = (sockaddr_in*) sa;
						sa_in->sin_family = (short) AddressFamily.InterNetwork;
						Marshal.Copy (address, 0, (IntPtr) sa_in->sin_addr, address.Length);
						sa_in->sin_port = (ushort) port;

						break;
					}
					case AddressFamily.InterNetworkV6: {
						byte[] address = ep.Address.GetAddressBytes ();
						int port = ep.Port;
						long scopeId = ep.Address.ScopeId;

						AllocSockaddr (ref sa, sa_size = sizeof (sockaddr_in6));

						sockaddr_in6 *sa_in6 = (sockaddr_in6*) sa;
						sa_in6->sin6_family = (short) AddressFamily.InterNetworkV6;
						sa_in6->sin6_port = (ushort) port;
						Marshal.Copy (address, 0, (IntPtr) sa_in6->sin6_addr, address.Length);
						sa_in6->sin6_scope_id = (uint) scopeId;

						break;
					}
					case AddressFamily.Unix: {
						AllocSockaddr (ref sa, sa_size = sizeof (sockaddr_un));

						sockaddr_un *sa_un = (sockaddr_un*) sa;
						sa_un->sun_family = (short) AddressFamily.Unix;
						for (int i = 0; i < 108; ++i) {
							sa_un->sun_path [i] = addr [i + 2];
							if (addr [i + 2] == (byte) '\0')
								break;
						}

						break;
					}
					default:
						throw new SocketException (SocketError.AddressFamilyNotSupported);
					}
				}
			}

			static SocketAddress CreateSocketAddressFromSockaddr (IntPtr sa)
			{
				unsafe {
					IPAddress address;
					int port;

					switch ((AddressFamily) ((sockaddr*) sa)->sa_family) {
					case AddressFamily.InterNetwork: {
						sockaddr_in *sa_in = (sockaddr_in*) sa;

						byte[] bytes = new byte [4];
						Marshal.Copy ((IntPtr) sa_in->sin_addr, bytes, 0, bytes.Length);

						address = new IPAddress (bytes);
						port = sa_in->sin_port;
						break;
					}
					case AddressFamily.InterNetworkV6: {
						sockaddr_in6 *sa_in6 = (sockaddr_in6*) sa;

						byte[] bytes = new byte [16];
						Marshal.Copy ((IntPtr) sa_in6->sin6_addr, bytes, 0, bytes.Length);

						address = new IPAddress (bytes);
						port = sa_in6->sin6_port;
						break;
					}
					case AddressFamily.Unix: {
						sockaddr_un *sa_un = (sockaddr_un*) sa;

						SocketAddress addr = new SocketAddress (AddressFamily.Unix, sizeof (sockaddr_un));
						for (int i = 0; i < 108; ++i) {
							addr [i + 2] = sa_un->sun_path [i];
							if (sa_un->sun_path [i] == (byte) '\0')
								break;
						}

						return addr;
					}
					default:
						throw new SocketException (SocketError.AddressFamilyNotSupported);
					}

					return new SocketAddress (address, port);
				}
			}

			[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
			unsafe struct sockaddr
			{
				public ushort sa_family;
				public fixed byte sa_data[14];
			}

			[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
			unsafe struct sockaddr_in
			{
				public short sin_family;
				public ushort sin_port;
				public fixed byte sin_addr[4];
				public fixed byte sin_zero[8];
			}

			[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
			unsafe struct sockaddr_in6
			{
				public short sin6_family;
				public ushort sin6_port;
				public uint sin6_flowinfo;
				public fixed byte sin6_addr[16];
				public uint sin6_scope_id;
			}

			[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
			unsafe struct sockaddr_un
			{
				public short sun_family;
				public fixed byte sun_path[108];
			}

			[StructLayout (LayoutKind.Sequential)]
			struct timeval {
				public int tv_sec;
				public int tv_usec;
			}
		}
	}
}