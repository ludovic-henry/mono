
using System.Net.Sockets;

namespace System.Net.Internals
{
	internal static partial class SocketExceptionFactory
	{
		public static SocketException CreateSocketException(SocketError errorCode, int platformError)
		{
			if (Environment.IsRunningOnWindows)
				return Windows_CreateSocketException(errorCode, platformError);
			else
				return Unix_CreateSocketException(errorCode, platformError);
		}
	}
}