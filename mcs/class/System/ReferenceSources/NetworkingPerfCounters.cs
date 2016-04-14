
using System.Diagnostics;

namespace System.Net
{
	internal enum NetworkingPerfCounterName
	{
		SocketConnectionsEstablished = 0, // these enum values are used as index
		SocketBytesReceived,
		SocketBytesSent,
		SocketDatagramsReceived,
		SocketDatagramsSent,
		HttpWebRequestCreated,
		HttpWebRequestAvgLifeTime,
		HttpWebRequestAvgLifeTimeBase,
		HttpWebRequestQueued,
		HttpWebRequestAvgQueueTime,
		HttpWebRequestAvgQueueTimeBase,
		HttpWebRequestAborted,
		HttpWebRequestFailed
	}

	internal sealed class NetworkingPerfCounters
	{
		public static NetworkingPerfCounters Instance { get; } = new NetworkingPerfCounters ();

		public bool Enabled
		{
			get { return false; }
		}

		public void Increment (NetworkingPerfCounterName perfCounter, long amount = 1)
		{
		}

		public void Decrement(NetworkingPerfCounterName perfCounter, long amount = 1)
		{
		}

		public void IncrementAverage(NetworkingPerfCounterName perfCounter, long startTimestamp)
		{
		}

		public static long GetTimestamp()
		{
			return Stopwatch.GetTimestamp();
		}
	}
}

