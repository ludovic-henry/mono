
using System.IO;
using System.Runtime.CompilerServices;
using System.Threading.Tasks;
using System.Threading;

namespace System.Net.WebSockets
{

#if !SECURITY_DEP
	public class HttpListenerWebSocketContext
	{
	}

	public sealed class HttpListenerContext
	{
	}
#endif

	internal static class WebSocketHelpers
	{
		internal const string SecWebSocketKeyGuid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
		internal const string WebSocketUpgradeToken = "websocket";
		internal const int DefaultReceiveBufferSize = 16 * 1024;
		internal const int DefaultClientSendBufferSize = 16 * 1024;
		internal const int MaxControlFramePayloadLength = 123;

		internal const int ClientTcpCloseTimeout = 1000;

		internal static ArraySegment<byte> EmptyPayload
		{
			get { throw new NotSupportedException (); }
		}

		internal static Task<HttpListenerWebSocketContext> AcceptWebSocketAsync(HttpListenerContext context, string subProtocol, int receiveBufferSize, TimeSpan keepAliveInterval, ArraySegment<byte> internalBuffer)
		{
			throw new NotSupportedException ();
		}

		internal static string GetSecWebSocketAcceptString(string secWebSocketKey)
		{
			throw new NotSupportedException ();
		}

		internal static string GetTraceMsgForParameters(int offset, int count, CancellationToken cancellationToken)
		{
			throw new NotSupportedException ();
		}

		internal static bool ProcessWebSocketProtocolHeader(string clientSecWebSocketProtocol, string subProtocol, out string acceptProtocol)
		{
			throw new NotSupportedException ();
		}

		internal static ConfiguredTaskAwaitable SuppressContextFlow(this Task task)
		{
			throw new NotSupportedException ();
		}

		internal static ConfiguredTaskAwaitable<T> SuppressContextFlow<T>(this Task<T> task)
		{
			throw new NotSupportedException ();
		}
		
		internal static void ValidateBuffer(byte[] buffer, int offset, int count)
		{
			throw new NotSupportedException ();
		}

		internal static void PrepareWebRequest(ref HttpWebRequest request)
		{
			throw new NotSupportedException ();
		}

		internal static void ValidateSubprotocol(string subProtocol)
		{
			throw new NotSupportedException ();
		}

		internal static void ValidateCloseStatus(WebSocketCloseStatus closeStatus, string statusDescription)
		{
			throw new NotSupportedException ();
		}

		internal static void ValidateOptions(string subProtocol, int receiveBufferSize, int sendBufferSize, TimeSpan keepAliveInterval)
		{
			throw new NotSupportedException ();
		}

		internal static void ValidateBufferSizes(int receiveBufferSize, int sendBufferSize)
		{
			throw new NotSupportedException ();
		}

		internal static void ValidateInnerStream(Stream innerStream)
		{
			throw new NotSupportedException ();
		}

		internal static void ThrowIfConnectionAborted(Stream connection, bool read)
		{
			throw new NotSupportedException ();
		}

		internal static void ThrowPlatformNotSupportedException_WSPC()
		{
			throw new PlatformNotSupportedException(SR.GetString(SR.net_WebSockets_UnsupportedPlatform));
		}

		internal static void ValidateArraySegment<T>(ArraySegment<T> arraySegment, string parameterName)
		{
			throw new NotSupportedException ();
		}

		internal static class MethodNames
		{
			internal const string AcceptWebSocketAsync = "AcceptWebSocketAsync";
			internal const string ValidateWebSocketHeaders = "ValidateWebSocketHeaders";
		}
	}
}
