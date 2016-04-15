
using System.Runtime.InteropServices;

namespace System.Net.WebSockets
{
	internal static class WebSocketProtocolComponent
	{
		internal static class Errors
		{
			internal const int E_INVALID_OPERATION = unchecked((int)0x80000050);
			internal const int E_INVALID_PROTOCOL_OPERATION = unchecked((int)0x80000051);
			internal const int E_INVALID_PROTOCOL_FORMAT = unchecked((int)0x80000052);
			internal const int E_NUMERIC_OVERFLOW = unchecked((int)0x80000053);
			internal const int E_FAIL = unchecked((int)0x80004005);
		}

		internal enum Action
		{
			NoAction = 0,
			SendToNetwork = 1,
			IndicateSendComplete = 2,
			ReceiveFromNetwork = 3,
			IndicateReceiveComplete = 4,
		}

		internal enum BufferType : uint
		{
			None = 0x00000000,
			UTF8Message = 0x80000000,
			UTF8Fragment = 0x80000001,
			BinaryMessage = 0x80000002,
			BinaryFragment = 0x80000003,
			Close = 0x80000004,
			PingPong = 0x80000005,
			UnsolicitedPong = 0x80000006
		}

		internal enum PropertyType
		{
			ReceiveBufferSize = 0,
			SendBufferSize = 1,
			DisableMasking = 2,
			AllocatedBuffer = 3,
			DisableUtf8Verification = 4,
			KeepAliveInterval = 5,
		}

		internal enum ActionQueue
		{
			Send = 1,
			Receive = 2,
		}

		[StructLayout(LayoutKind.Sequential)]
		internal struct Property
		{
			internal PropertyType Type;
			internal IntPtr PropertyData;
			internal uint PropertySize;
		}

		[StructLayout(LayoutKind.Explicit)]
		internal struct Buffer
		{
			[FieldOffset(0)]
			internal DataBuffer Data;
			[FieldOffset(0)]
			internal CloseBuffer CloseStatus;
		}

		[StructLayout(LayoutKind.Sequential)]
		internal struct DataBuffer
		{
			internal IntPtr BufferData;
			internal uint BufferLength;
		}

		[StructLayout(LayoutKind.Sequential)]
		internal struct CloseBuffer
		{
			internal IntPtr ReasonData;
			internal uint ReasonLength;
			internal ushort CloseStatus;
		}

		[StructLayout(LayoutKind.Sequential)]
		internal struct HttpHeader
		{
			[MarshalAs(UnmanagedType.LPStr)]
			internal string Name;
			internal uint NameLength;
			[MarshalAs(UnmanagedType.LPStr)]
			internal string Value;
			internal uint ValueLength;
		}

		static WebSocketProtocolComponent()
		{
			throw new NotSupportedException ();
		}

		internal static string SupportedVersion
		{
			get { throw new NotSupportedException (); }
		}

		internal static bool IsSupported
		{
			get { throw new NotSupportedException (); }
		}

		internal static string GetSupportedVersion()
		{
			throw new NotSupportedException ();
		}

		internal static void WebSocketCreateClientHandle(Property[] properties, out SafeWebSocketHandle webSocketHandle)
		{
			throw new NotSupportedException ();
		}

		internal static void WebSocketCreateServerHandle(Property[] properties, int propertyCount, out SafeWebSocketHandle webSocketHandle)
		{
			throw new NotSupportedException ();
		}

		internal static void WebSocketAbortHandle(SafeHandle webSocketHandle)
		{
			throw new NotSupportedException ();
		}

		internal static void WebSocketDeleteHandle(IntPtr webSocketPtr)
		{
			throw new NotSupportedException ();
		}

		internal static void WebSocketSend(WebSocketBase webSocket, BufferType bufferType, Buffer buffer)
		{
			throw new NotSupportedException ();
		}

		internal static void WebSocketSendWithoutBody(WebSocketBase webSocket, BufferType bufferType)
		{
			throw new NotSupportedException ();
		}

		internal static void WebSocketReceive(WebSocketBase webSocket)
		{
			throw new NotSupportedException ();
		}

		internal static void WebSocketGetAction(WebSocketBase webSocket, ActionQueue actionQueue, Buffer[] dataBuffers, ref uint dataBufferCount, out Action action, out BufferType bufferType, out IntPtr actionContext)
		{
			throw new NotSupportedException ();
		}

		internal static void WebSocketCompleteAction(WebSocketBase webSocket, IntPtr actionContext, int bytesTransferred)
		{
			throw new NotSupportedException ();
		}

		internal static TimeSpan WebSocketGetDefaultKeepAliveInterval()
		{
			throw new NotSupportedException ();
		}

		public static bool Succeeded(int hr)
		{
			throw new NotSupportedException ();
		}
	}
}
