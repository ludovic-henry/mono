
using System;
using System.Net.Sockets;
namespace System.Net
{
    internal static partial class SocketAddressPal
    {
        // Unix

        public static unsafe void SetIPv4Address(byte[] buffer, byte* address)
        {
            if (Environment.IsRunningOnWindows)
                throw new PlatformNotSupportedException();
            else
                Unix.SetIPv4Address(buffer, address);
        }

        public static unsafe void SetIPv6Address(byte[] buffer, byte* address, int addressLength, uint scope)
        {
            if (Environment.IsRunningOnWindows)
                throw new PlatformNotSupportedException();
            else
                Unix.SetIPv6Address(buffer, address, addressLength, scope);
        }

        // Unix + Windows

        public static int DataOffset
        {
            get
            {
                if (Environment.IsRunningOnWindows)
                    return Windows.DataOffset;
                else
                    return Unix.DataOffset;
            }
        }

        public static int IPv6AddressSize
        {
            get
            {
                if (Environment.IsRunningOnWindows)
                    return Windows.IPv6AddressSize;
                else
                    return Unix.IPv6AddressSize;
            }
        }

        public static int IPv4AddressSize
        {
            get
            {
                if (Environment.IsRunningOnWindows)
                    return Windows.IPv4AddressSize;
                else
                    return Unix.IPv4AddressSize;
            }
        }

        public static AddressFamily GetAddressFamily(byte[] buffer)
        {
            if (Environment.IsRunningOnWindows)
                return Windows.GetAddressFamily(buffer);
            else
                return Unix.GetAddressFamily(buffer);
        }

        public static void SetAddressFamily(byte[] buffer, AddressFamily family)
        {
            if (Environment.IsRunningOnWindows)
                Windows.SetAddressFamily(buffer, family);
            else
                Unix.SetAddressFamily(buffer, family);
        }

        public static ushort GetPort(byte[] buffer)
        {
            if (Environment.IsRunningOnWindows)
                return Windows.GetPort(buffer);
            else
                return Unix.GetPort(buffer);
        }

        public static void SetPort(byte[] buffer, ushort port)
        {
            if (Environment.IsRunningOnWindows)
                Windows.SetPort(buffer, port);
            else
                Unix.SetPort(buffer, port);
        }

        public static uint GetIPv4Address(byte[] buffer)
        {
            if (Environment.IsRunningOnWindows)
                return Windows.GetIPv4Address(buffer);
            else
                return Unix.GetIPv4Address(buffer);
        }

        public static void SetIPv4Address(byte[] buffer, uint address)
        {
            if (Environment.IsRunningOnWindows)
                Windows.SetIPv4Address(buffer, address);
            else
                Unix.SetIPv4Address(buffer, address);
        }

        public static void GetIPv6Address(byte[] buffer, byte[] address, out uint scope)
        {
            if (Environment.IsRunningOnWindows)
                Windows.GetIPv6Address(buffer, address, out scope);
            else
                Unix.GetIPv6Address(buffer, address, out scope);
        }

        public static void SetIPv6Address(byte[] buffer, byte[] address, uint scope)
        {
            if (Environment.IsRunningOnWindows)
                Windows.SetIPv6Address(buffer, address, scope);
            else
                Unix.SetIPv6Address(buffer, address, scope);
        }
    }
}
