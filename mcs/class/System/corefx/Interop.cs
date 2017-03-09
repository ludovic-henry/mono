
using System;

internal static partial class Interop
{
	// Unix

	internal static partial class Sys
	{
		static Sys()
		{
			if (!Environment.IsRunningOnUnix)
				throw new PlatformNotSupportedException();
		}
	}

	// Windows

	internal static partial class Kernel32
	{
		static Kernel32()
		{
			if (!Environment.IsRunningOnWindows)
				throw new PlatformNotSupportedException();
		}
	}

	internal static partial class Winsock
	{
		static Winsock()
		{
			if (!Environment.IsRunningOnWindows)
				throw new PlatformNotSupportedException();
		}
	}

	internal static partial class Mswsock
	{
		static Mswsock()
		{
			if (!Environment.IsRunningOnWindows)
				throw new PlatformNotSupportedException();
		}
	}
}
