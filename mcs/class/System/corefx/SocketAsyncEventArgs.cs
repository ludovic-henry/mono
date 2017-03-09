
namespace System.Net.Sockets
{
    public partial class SocketAsyncEventArgs : EventArgs, IDisposable
    {
        // Windows

        internal static int s_controlDataSize
        {
            get
            {
                if (Environment.IsRunningOnWindows)
                    return Windows_s_controlDataSize;
                else
                    throw new PlatformNotSupportedException();
            }
        }

        internal static int s_controlDataIPv6Size
        {
            get
            {
                if (Environment.IsRunningOnWindows)
                    return Windows_s_controlDataIPv6Size;
                else
                    throw new PlatformNotSupportedException();
            }
        }

        internal static int s_wsaMsgSize
        {
            get
            {
                if (Environment.IsRunningOnWindows)
                    return Windows_s_wsaMsgSize;
                else
                    throw new PlatformNotSupportedException();
            }
        }

        // Unix + Windows

        internal int? SendPacketsDescriptorCount
        {
            get
            {
                if (Environment.IsRunningOnWindows)
                    return Windows_SendPacketsDescriptorCount;
                else
                    return Unix_SendPacketsDescriptorCount;
            }
        }

        private void InitializeInternals()
        {
            if (Environment.IsRunningOnWindows)
                Windows_InitializeInternals();
            else
                Unix_InitializeInternals();
        }

        private void FreeInternals(bool calledFromFinalizer)
        {
            if (Environment.IsRunningOnWindows)
                Windows_FreeInternals(calledFromFinalizer);
            else
                Unix_FreeInternals(calledFromFinalizer);
        }

        private void SetupSingleBuffer()
        {
            if (Environment.IsRunningOnWindows)
                Windows_SetupSingleBuffer();
            else
                Unix_SetupSingleBuffer();
        }

        private void SetupMultipleBuffers()
        {
            if (Environment.IsRunningOnWindows)
                Windows_SetupMultipleBuffers();
            else
                Unix_SetupMultipleBuffers();
        }

        private void SetupSendPacketsElements()
        {
            if (Environment.IsRunningOnWindows)
                Windows_SetupSendPacketsElements();
            else
                Unix_SetupSendPacketsElements();
        }

        private void InnerComplete()
        {
            if (Environment.IsRunningOnWindows)
                Windows_InnerComplete();
            else
                Unix_InnerComplete();
        }

        private void InnerStartOperationAccept(bool userSuppliedBuffer)
        {
            if (Environment.IsRunningOnWindows)
                Windows_InnerStartOperationAccept(userSuppliedBuffer);
            else
                Unix_InnerStartOperationAccept(userSuppliedBuffer);
        }

        internal SocketError DoOperationAccept(Socket socket, SafeCloseSocket handle, SafeCloseSocket acceptHandle)
        {
            if (Environment.IsRunningOnWindows)
                return Windows_DoOperationAccept(socket, handle, acceptHandle);
            else
                return Unix_DoOperationAccept(socket, handle, acceptHandle);
        }

        private void InnerStartOperationConnect()
        {
            if (Environment.IsRunningOnWindows)
                Windows_InnerStartOperationConnect();
            else
                Unix_InnerStartOperationConnect();
        }

        internal SocketError DoOperationConnect(Socket socket, SafeCloseSocket handle)
        {
            if (Environment.IsRunningOnWindows)
                return Windows_DoOperationConnect(socket, handle);
            else
                return Unix_DoOperationConnect(socket, handle);
        }

        private void InnerStartOperationDisconnect()
        {
            if (Environment.IsRunningOnWindows)
                Windows_InnerStartOperationDisconnect();
            else
                Unix_InnerStartOperationDisconnect();
        }

        internal SocketError DoOperationDisconnect(Socket socket, SafeCloseSocket handle)
        {
            if (Environment.IsRunningOnWindows)
                return Windows_DoOperationDisconnect(socket, handle);
            else
                return Unix_DoOperationDisconnect(socket, handle);
        }

        private void InnerStartOperationReceive()
        {
            if (Environment.IsRunningOnWindows)
                Windows_InnerStartOperationReceive();
            else
                Unix_InnerStartOperationReceive();
        }

        internal SocketError DoOperationReceive(SafeCloseSocket handle, out SocketFlags flags)
        {
            if (Environment.IsRunningOnWindows)
                return Windows_DoOperationReceive(handle, out flags);
            else
                return Unix_DoOperationReceive(handle, out flags);
        }

        private void InnerStartOperationReceiveFrom()
        {
            if (Environment.IsRunningOnWindows)
                Windows_InnerStartOperationReceiveFrom();
            else
                Unix_InnerStartOperationReceiveFrom();
        }

        internal SocketError DoOperationReceiveFrom(SafeCloseSocket handle, out SocketFlags flags)
        {
            if (Environment.IsRunningOnWindows)
                return Windows_DoOperationReceiveFrom(handle, out flags);
            else
                return Unix_DoOperationReceiveFrom(handle, out flags);
        }

        private void InnerStartOperationReceiveMessageFrom()
        {
            if (Environment.IsRunningOnWindows)
                Windows_InnerStartOperationReceiveMessageFrom();
            else
                Unix_InnerStartOperationReceiveMessageFrom();
        }

        internal SocketError DoOperationReceiveMessageFrom(Socket socket, SafeCloseSocket handle)
        {
            if (Environment.IsRunningOnWindows)
                return Windows_DoOperationReceiveMessageFrom(socket, handle);
            else
                return Unix_DoOperationReceiveMessageFrom(socket, handle);
        }

        private void InnerStartOperationSend()
        {
            if (Environment.IsRunningOnWindows)
                Windows_InnerStartOperationSend();
            else
                Unix_InnerStartOperationSend();
        }

        internal SocketError DoOperationSend(SafeCloseSocket handle)
        {
            if (Environment.IsRunningOnWindows)
                return Windows_DoOperationSend(handle);
            else
                return Unix_DoOperationSend(handle);
        }

        private void InnerStartOperationSendPackets()
        {
            if (Environment.IsRunningOnWindows)
                Windows_InnerStartOperationSendPackets();
            else
                Unix_InnerStartOperationSendPackets();
        }

        internal SocketError DoOperationSendPackets(Socket socket, SafeCloseSocket handle)
        {
            if (Environment.IsRunningOnWindows)
                return Windows_DoOperationSendPackets(socket, handle);
            else
                return Unix_DoOperationSendPackets(socket, handle);
        }

        private void InnerStartOperationSendTo()
        {
            if (Environment.IsRunningOnWindows)
                Windows_InnerStartOperationSendTo();
            else
                Unix_InnerStartOperationSendTo();
        }

        internal SocketError DoOperationSendTo(SafeCloseSocket handle)
        {
            if (Environment.IsRunningOnWindows)
                return Windows_DoOperationSendTo(handle);
            else
                return Unix_DoOperationSendTo(handle);
        }

        internal void LogBuffer(int size)
        {
            if (Environment.IsRunningOnWindows)
                Windows_LogBuffer(size);
            else
                Unix_LogBuffer(size);
        }

        internal void LogSendPacketsBuffers(int size)
        {
            if (Environment.IsRunningOnWindows)
                Windows_LogSendPacketsBuffers(size);
            else
                Unix_LogSendPacketsBuffers(size);
        }

        private SocketError FinishOperationAccept(Internals.SocketAddress remoteSocketAddress)
        {
            if (Environment.IsRunningOnWindows)
                return Windows_FinishOperationAccept(remoteSocketAddress);
            else
                return Unix_FinishOperationAccept(remoteSocketAddress);
        }

        private SocketError FinishOperationConnect()
        {
            if (Environment.IsRunningOnWindows)
                return Windows_FinishOperationConnect();
            else
                return Unix_FinishOperationConnect();
        }

        private int GetSocketAddressSize()
        {
            if (Environment.IsRunningOnWindows)
                return Windows_GetSocketAddressSize();
            else
                return Unix_GetSocketAddressSize();
        }

        private void FinishOperationReceiveMessageFrom()
        {
            if (Environment.IsRunningOnWindows)
                Windows_FinishOperationReceiveMessageFrom();
            else
                Unix_FinishOperationReceiveMessageFrom();
        }

        private void FinishOperationSendPackets()
        {
            if (Environment.IsRunningOnWindows)
                Windows_FinishOperationSendPackets();
            else
                Unix_FinishOperationSendPackets();
        }
    }
}
