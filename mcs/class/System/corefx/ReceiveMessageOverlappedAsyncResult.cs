
using System.Diagnostics;

namespace System.Net.Sockets
{
	internal unsafe sealed partial class ReceiveMessageOverlappedAsyncResult : BaseOverlappedAsyncResult
	{
		// Unix

		public void CompletionCallback(int numBytes, byte[] socketAddress, int socketAddressSize, SocketFlags receivedFlags, IPPacketInformation ipPacketInformation, SocketError errorCode)
		{
			if (Environment.IsRunningOnWindows)
				throw new PlatformNotSupportedException();
			else
				Unix_CompletionCallback(numBytes, socketAddress, socketAddressSize, receivedFlags, ipPacketInformation, errorCode);
		}

		// Windows

		internal void SetUnmanagedStructures(byte[] buffer, int offset, int size, Internals.SocketAddress socketAddress, SocketFlags socketFlags)
		{
			if (Environment.IsRunningOnWindows)
				Windows_SetUnmanagedStructures(buffer, offset, size, socketAddress, socketFlags);
			else
				throw new PlatformNotSupportedException();
		}

		protected override void ForceReleaseUnmanagedStructures()
		{
			if (Environment.IsRunningOnWindows)
				Windows_ForceReleaseUnmanagedStructures();
			else
				base.ForceReleaseUnmanagedStructures();
		}

		internal override object PostCompletion(int numBytes)
		{
			if (Environment.IsRunningOnWindows)
				return Windows_PostCompletion(numBytes);
			else
				return base.PostCompletion(numBytes);
		}

		// Unix + Windows

		internal int GetSocketAddressSize()
		{
			if (Environment.IsRunningOnWindows)
				return Windows_GetSocketAddressSize();
			else
				return Unix_GetSocketAddressSize();
		}
	}
}
