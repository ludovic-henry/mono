
namespace System.Net
{
	internal partial class SocketProtocolSupportPal
	{
		public static bool OSSupportsIPv6
		{
			get
			{
				if (Environment.IsRunningOnWindows)
					return Windows.OSSupportsIPv6;
				else
					return Unix.OSSupportsIPv6;
			}
		}

		public static bool OSSupportsIPv4
		{
			get
			{
				if (Environment.IsRunningOnWindows)
					return Windows.OSSupportsIPv4;
				else
					return Unix.OSSupportsIPv4;
			}
		}
	}
}
