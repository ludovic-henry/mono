
using System.Security.Principal;

namespace System.Net
{
    partial class ContextAwareResult
    {
        internal WindowsIdentity Identity
        {
            get
            {
                if (Environment.IsRunningOnWindows)
                    return Windows_Identity;
                else
                    throw new PlatformNotSupportedException();
            }
        }

        private void SafeCaptureIdentity()
        {
            if (Environment.IsRunningOnWindows)
                Windows_SafeCaptureIdentity();
            else
                Unix_SafeCaptureIdentity();
        }

        private void CleanupInternal()
        {
            if (Environment.IsRunningOnWindows)
                Windows_CleanupInternal();
            else
                Unix_CleanupInternal();
        }
    }
}
