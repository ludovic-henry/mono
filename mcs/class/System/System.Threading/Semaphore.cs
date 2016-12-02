
using Mono;
using System;
using System.Security.AccessControl;

namespace System.Threading
{
	public sealed partial class Semaphore : WaitHandle
	{
		const int ERROR_INVALID_HANDLE = 6;

		internal static extern IntPtr CreateSemaphore_internal (int initialCount, int maximumCount, string name, out int errorCode)
		{
		}

		[DllImport("Kernel32", SetLastError=true)]
		internal static extern bool ReleaseSemaphore (IntPtr handle, int releaseCount, out int previousCount);

		internal static extern bool ReleaseSemaphore_internal (IntPtr handleIntPtr, int releaseCount, out int previousCount)
		{
			if (Environment.IsRunningOnWindows)
				return ReleaseSemaphore (handleIntPtr, releaseCount, out previousCount);

			if (handleIntPtr == IntPtr.Zero) {
				Marshal.SetLastWin32Error (ERROR_INVALID_HANDLE);
				return false;
			}

			W32Handle handle = W32Handle.FromIntPtr (handleIntPtr);

			if (handle.Type != W32Handle.Type.Semaphore && handle.Type != W32Handle.Type.NamedSemaphore) {
				Marshal.SetLastWin32Error (ERROR_INVALID_HANDLE);
				return false;
			}


			if (!handle.Lookup (handle.Type, out ))

		}

		private static extern IntPtr OpenSemaphore_internal (string name, SemaphoreRights rights, out int errorCode)
		{

		}

		struct SemaphoreData
		{
			uint val;
			int max;
		}
	}
}