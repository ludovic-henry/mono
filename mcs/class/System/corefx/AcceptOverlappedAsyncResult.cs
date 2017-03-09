
namespace System.Net.Sockets
{
	internal sealed partial class AcceptOverlappedAsyncResult : BaseOverlappedAsyncResult
	{
		// Unix

		public void CompletionCallback(IntPtr acceptedFileDescriptor, byte[] socketAddress, int socketAddressLen, SocketError errorCode)
		{
			if (Environment.IsRunningOnWindows)
				throw new PlatformNotSupportedException();
			else
				Unix_CompletionCallback(acceptedFileDescriptor, socketAddress, socketAddressLen, errorCode);
		}

		// Windows

		internal void SetUnmanagedStructures(byte[] buffer, int addressBufferLength)
		{
			if (Environment.IsRunningOnWindows)
				Windows_SetUnmanagedStructures(buffer, addressBufferLength);
			else
				throw new PlatformNotSupportedException();
		}

		// Unix + Windows

		internal Socket AcceptSocket
		{
			set
			{
				if (Environment.IsRunningOnWindows)
					Windows_AcceptSocket = value;
				else
					Unix_AcceptSocket = value;
			}
		}

		internal override object PostCompletion(int numBytes)
		{
			if (Environment.IsRunningOnWindows)
				return Windows_PostCompletion(numBytes);
			else
				return Unix_PostCompletion(numBytes);
		}
	}
}
