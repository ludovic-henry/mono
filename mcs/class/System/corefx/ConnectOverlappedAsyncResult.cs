
using System;
using System.Net;
using System.Runtime.InteropServices;
using System.Threading;
using Microsoft.Win32;

namespace System.Net.Sockets
{
	internal sealed partial class ConnectOverlappedAsyncResult : BaseOverlappedAsyncResult
	{
		// Unix

		public void CompletionCallback(SocketError errorCode)
		{
			if (Environment.IsRunningOnWindows)
				throw new PlatformNotSupportedException();
			else
				Unix_CompletionCallback(errorCode);
		}

		// Unix + Windows

		internal override object PostCompletion(int numBytes)
		{
			if (Environment.IsRunningOnWindows)
				return Windows_PostCompletion(numBytes);
			else
				return Unix_PostCompletion(numBytes);
		}
	}
}
