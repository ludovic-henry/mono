
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
		static NetworkingPerfCounters instance = new NetworkingPerfCounters ();

		private NetworkingPerfCounters() {}

		public bool Enabled { get { return false; } }

		public void Increment(NetworkingPerfCounterName perfCounter) {}
		public void Increment(NetworkingPerfCounterName perfCounter, long amount) {}
		public void Decrement(NetworkingPerfCounterName perfCounter) {}
		public void Decrement(NetworkingPerfCounterName perfCounter, long amount) {}
		public void IncrementAverage(NetworkingPerfCounterName perfCounter, long startTimestamp) {}

		public static NetworkingPerfCounters Instance
		{
			get
			{
				return instance;
			}
		}

		public static long GetTimestamp()
		{
			return Stopwatch.GetTimestamp();
		}
	}
}