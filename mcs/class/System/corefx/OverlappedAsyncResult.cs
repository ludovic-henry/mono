
using System;
using System.Collections.Generic;

namespace System.Net.Sockets
{
    internal partial class OverlappedAsyncResult : BaseOverlappedAsyncResult
    {
        // Unix

        public void CompletionCallback(int numBytes, byte[] socketAddress, int socketAddressSize, SocketFlags receivedFlags, SocketError errorCode)
        {
            if (Environment.IsRunningOnWindows)
                throw new PlatformNotSupportedException();
            else
                Unix_CompletionCallback(numBytes, socketAddress, socketAddressSize, receivedFlags, errorCode);
        }

        // Windows

        internal IntPtr GetSocketAddressPtr()
        {
            if (Environment.IsRunningOnWindows)
                return Windows_GetSocketAddressPtr();
            else
                throw new PlatformNotSupportedException();
        }

        internal IntPtr GetSocketAddressSizePtr()
        {
            if (Environment.IsRunningOnWindows)
                return Windows_GetSocketAddressSizePtr();
            else
                throw new PlatformNotSupportedException();
        }

        internal void SetUnmanagedStructures(byte[] buffer, int offset, int size, Internals.SocketAddress socketAddress, bool pinSocketAddress)
        {
            if (Environment.IsRunningOnWindows)
                Windows_SetUnmanagedStructures(buffer, offset, size, socketAddress, pinSocketAddress);
            else
                throw new PlatformNotSupportedException();
        }

        internal void SetUnmanagedStructures(IList<ArraySegment<byte>> buffers)
        {
            if (Environment.IsRunningOnWindows)
                Windows_SetUnmanagedStructures(buffers);
            else
                throw new PlatformNotSupportedException();
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
