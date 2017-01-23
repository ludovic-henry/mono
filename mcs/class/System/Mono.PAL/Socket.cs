
using System;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Runtime.InteropServices;

namespace Mono.PAL
{
	internal static partial class Sockets
	{
		internal static SafeSocketHandle Socket (AddressFamily family, SocketType type, ProtocolType protocol)
		{
			if (Environment.IsRunningOnWindows)
				return Win32.Socket (family, type, protocol);
			else
				return Unix.Socket (family, type, protocol);
		}

		internal static SafeSocketHandle Accept (SafeSocketHandle socket, bool blocking)
		{
			bool release = false, unregister = false;
			try {
				socket.RegisterForBlockingSyscall (ref release, ref unregister);

				if (Environment.IsRunningOnWindows)
					return Win32.Accept (socket, blocking);
				else
					return Unix.Accept (socket, blocking);
			} finally {
				if (release)
					socket.UnregisterForBlockingSyscall (unregister);
			}
		}

		internal static SocketAddress GetLocalEndPoint (SafeSocketHandle socket, AddressFamily family)
		{
			if (Environment.IsRunningOnWindows)
				return Win32.GetLocalEndPoint (socket, family);
			else
				return Unix.GetLocalEndPoint (socket, family);
		}

		internal static SocketAddress GetRemoteEndPoint (SafeSocketHandle socket, AddressFamily family)
		{
			if (Environment.IsRunningOnWindows)
				return Win32.GetRemoteEndPoint (socket, family);
			else
				return Unix.GetRemoteEndPoint (socket, family);
		}

		internal static void SetBlocking (SafeSocketHandle socket, bool blocking)
		{
			if (Environment.IsRunningOnWindows)
				Win32.SetBlocking (socket, blocking);
			else
				Unix.SetBlocking (socket, blocking);
		}

		internal static int GetAvailable (SafeSocketHandle socket)
		{
			if (Environment.IsRunningOnWindows)
				return Win32.GetAvailable (socket);
			else
				return Unix.GetAvailable (socket);
		}

		internal static void Bind (SafeSocketHandle socket, SocketAddress addr)
		{
			if (Environment.IsRunningOnWindows)
				Win32.Bind (socket, addr);
			else
				Unix.Bind (socket, addr);
		}

		internal static void Connect (SafeSocketHandle socket, SocketAddress addr, bool blocking)
		{
			bool release = false, unregister = false;
			try {
				socket.RegisterForBlockingSyscall (ref release, ref unregister);

				if (Environment.IsRunningOnWindows)
					Win32.Connect (socket, addr, blocking);
				else
					Unix.Connect (socket, addr, blocking);
			} finally {
				if (release)
					socket.UnregisterForBlockingSyscall (unregister);
			}
		}

		internal static int Receive (SafeSocketHandle socket, byte[] buffer, int offset, int count, SocketFlags flags, bool blocking)
		{
			if (buffer.Length < offset + count)
				return 0;

			bool release = false, unregister = false;
			try {
				socket.RegisterForBlockingSyscall (ref release, ref unregister);

				unsafe {
					fixed (byte *buf = &buffer [offset]) {
						if (Environment.IsRunningOnWindows)
							return Win32.Receive (socket, (IntPtr) buf, count, flags, blocking);
						else
							return Unix.Receive (socket, (IntPtr) buf, count, flags, blocking);
					}
				}
			} finally {
				if (release)
					socket.UnregisterForBlockingSyscall (unregister);
			}
		}

		internal static int ReceiveBuffers (SafeSocketHandle socket, IList<ArraySegment<byte>> buffers, SocketFlags flags, bool blocking)
		{
			GCHandle[] handles = new GCHandle [buffers.Count];

			try {
				WSABUF[] wsaBuffers = new WSABUF [buffers.Count];

				for (int i = 0; i < buffers.Count; i++) {
					ArraySegment<byte> segment = buffers [i];

					if (segment.Offset < 0 || segment.Count < 0 || segment.Array.Length < segment.Count + segment.Offset)
						throw new ArgumentOutOfRangeException ("segment");

					try {} finally {
						/* Ensure we do not leak the GCHandle in case of ThreadAbortException */
						handles [i] = GCHandle.Alloc (segment.Array, GCHandleType.Pinned);
					}
					wsaBuffers [i].len = segment.Count;
					wsaBuffers [i].buf = Marshal.UnsafeAddrOfPinnedArrayElement (segment.Array, segment.Offset);
				}

				bool release = false, unregister = false;
				try {
					socket.RegisterForBlockingSyscall (ref release, ref unregister);

					unsafe {
						fixed (WSABUF* wsaBuf = wsaBuffers) {
							if (Environment.IsRunningOnWindows)
								return Win32.ReceiveBuffers (socket, (IntPtr) wsaBuf, wsaBuffers.Length, flags, blocking);
							else
								return Unix.ReceiveBuffers (socket, (IntPtr) wsaBuf, wsaBuffers.Length, flags, blocking);
						}
					}
				} finally {
					if (release)
						socket.UnregisterForBlockingSyscall (unregister);
				}
			} finally {
				for (int i = 0; i < buffers.Count; i++) {
					if (handles [i].IsAllocated)
						handles [i].Free ();
				}
			}
		}

		internal static int ReceiveFrom (SafeSocketHandle socket, byte[] buffer, int offset, int count, SocketFlags flags, ref SocketAddress addr, bool blocking)
		{
			if (buffer.Length < offset + count)
				return 0;

			bool release = false, unregister = false;
			try {
				socket.RegisterForBlockingSyscall (ref release, ref unregister);

				unsafe {
					fixed (byte *buf = &buffer [offset]) {
						if (Environment.IsRunningOnWindows)
							return Win32.ReceiveFrom (socket, (IntPtr) buf, count, flags, ref addr, blocking);
						else
							return Unix.ReceiveFrom (socket, (IntPtr) buf, count, flags, ref addr, blocking);
					}
				}
			} finally {
				if (release)
					socket.UnregisterForBlockingSyscall (unregister);
			}
		}

		internal static int Send (SafeSocketHandle socket, byte[] buffer, int offset, int count, SocketFlags flags, bool blocking)
		{
			if (buffer.Length < offset + count)
				return 0;

			bool release = false, unregister = false;
			try {
				socket.RegisterForBlockingSyscall (ref release, ref unregister);

				unsafe {
					fixed (byte *buf = &buffer [offset]) {
						if (Environment.IsRunningOnWindows)
							return Win32.Send (socket, (IntPtr) buf, count, flags, blocking);
						else
							return Unix.Send (socket, (IntPtr) buf, count, flags, blocking);
					}
				}
			} finally {
				if (release)
					socket.UnregisterForBlockingSyscall (unregister);
			}
		}

		internal static int SendBuffers (SafeSocketHandle socket, IList<ArraySegment<byte>> buffers, SocketFlags flags, bool blocking)
		{
			GCHandle[] handles = new GCHandle [buffers.Count];

			try {
				WSABUF[] wsaBuffers = new WSABUF [buffers.Count];

				for (int i = 0; i < buffers.Count; i++) {
					ArraySegment<byte> segment = buffers [i];

					if (segment.Offset < 0 || segment.Count < 0 || segment.Array.Length < segment.Count + segment.Offset)
						throw new ArgumentOutOfRangeException ("segment");

					try {} finally {
						/* Ensure we do not leak a GCHandle due to a ThreadAbortException */
						handles [i] = GCHandle.Alloc (segment.Array, GCHandleType.Pinned);
					}
					wsaBuffers [i].len = segment.Count;
					wsaBuffers [i].buf = Marshal.UnsafeAddrOfPinnedArrayElement (segment.Array, segment.Offset);
				}

				bool release = false, unregister = false;
				try {
					socket.RegisterForBlockingSyscall (ref release, ref unregister);

					unsafe {
						fixed (WSABUF* wsaBuf = wsaBuffers) {
							if (Environment.IsRunningOnWindows)
								return Win32.SendBuffers (socket, (IntPtr) wsaBuf, wsaBuffers.Length, flags, blocking);
							else
								return Unix.SendBuffers (socket, (IntPtr) wsaBuf, wsaBuffers.Length, flags, blocking);
						}
					}
				} finally {
					if (release)
						socket.UnregisterForBlockingSyscall (unregister);
				}
			} finally {
				for (int i = 0; i < buffers.Count; i++) {
					if (handles [i].IsAllocated)
						handles [i].Free ();
				}
			}
		}

		internal static int SendTo (SafeSocketHandle socket, byte[] buffer, int offset, int count, SocketFlags flags, SocketAddress addr, bool blocking)
		{
			if (buffer.Length < offset + count)
				return 0;

			bool release = false, unregister = false;
			try {
				socket.RegisterForBlockingSyscall (ref release, ref unregister);

				unsafe {
					fixed (byte *buf = &buffer [offset]) {
						if (Environment.IsRunningOnWindows)
							return Win32.SendTo (socket, (IntPtr) buf, count, flags, addr, blocking);
						else
							return Unix.SendTo (socket, (IntPtr) buf, count, flags, addr, blocking);
					}
				}
			} finally {
				if (release)
					socket.UnregisterForBlockingSyscall (unregister);
			}
		}

		internal static void SendFile (SafeSocketHandle socket, string filename, byte[] preBuffer, byte[] postBuffer, TransmitFileOptions flags)
		{
			using (FileStream filestream = new FileStream (filename, FileMode.Open, FileAccess.Read, FileShare.Read)) {
				SafeHandle file = filestream.SafeFileHandle;

				unsafe {
					fixed (byte* head = preBuffer)
					fixed (byte* tail = postBuffer) {
						TRANSMIT_FILE_BUFFERS buffers = new TRANSMIT_FILE_BUFFERS {
							Head = (IntPtr) head,
							HeadLength = (uint) (preBuffer != null ? preBuffer.Length : 0),
							Tail = (IntPtr) tail,
							TailLength = (uint) (postBuffer != null ? postBuffer.Length : 0),
						};

						bool releaseFile = false;
						try {
							file.DangerousAddRef (ref releaseFile);

							bool release = false, unregister = false;
							try {
								socket.RegisterForBlockingSyscall (ref release, ref unregister);

								if (Environment.IsRunningOnWindows)
									Win32.SendFile (socket.DangerousGetHandle (), file.DangerousGetHandle (), (IntPtr) (&buffers), flags);
								else
									Unix.SendFile (socket.DangerousGetHandle (), file.DangerousGetHandle (), (IntPtr) (&buffers), flags);
							} finally {
								if (release)
									socket.UnregisterForBlockingSyscall (unregister);
							}
						} finally {
							if (releaseFile)
								file.DangerousRelease ();
						}
					}
				}
			}
		}

		internal static bool Poll (SafeSocketHandle socket, int ustimeout, SelectMode mode)
		{
			bool release = false;
			try {
				socket.DangerousAddRef (ref release);

				if (Environment.IsRunningOnWindows)
					return Win32.Poll (socket.DangerousGetHandle (), ustimeout, mode);
				else
					return Unix.Poll (socket.DangerousGetHandle (), ustimeout, mode);
			} finally {
				if (release)
					socket.DangerousRelease ();
			}
		}

		[StructLayout (LayoutKind.Sequential)]
		struct WSABUF {
			public int len;
			public IntPtr buf;
		}

		[StructLayout (LayoutKind.Sequential)]
		struct TRANSMIT_FILE_BUFFERS {
			public IntPtr Head;
			public uint HeadLength;
			public IntPtr Tail;
			public uint TailLength;
		}
	}
}