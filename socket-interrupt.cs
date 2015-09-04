using System;
using System.Net;
using System.Net.Sockets;
using System.Threading;

class Driver
{
	public static void Main (string[] args)
	{
		ManualResetEvent mre = new ManualResetEvent (false);
		Thread thread;
		Socket sock;
		Socket sock_server, sock_wr;

		switch (Int32.Parse (args [0])) {
		case 1: {
			IPEndPoint end_point = new IPEndPoint(IPAddress.Parse ("127.0.0.1"), 13578);

			sock_server = new Socket (AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
			sock_server.Bind (end_point);
			sock_server.Listen (1);

			sock_wr = new Socket (AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
			sock_wr.Connect (end_point);

			sock = sock_server.Accept ();

			thread = new Thread (() => {
				Console.WriteLine ("Before Receive");
				try {
					byte[] buffer = new byte [128];
					sock.Receive (buffer);

					Console.WriteLine ("[error] After Receive");
				} catch (SocketException e) {
					Console.WriteLine ("Accept : SocketException: {0}", e.Message);
				}
				Console.WriteLine ("After  Receive");

				mre.Set ();
			});

			break;
		}
		case 2: {
			sock = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
			sock.Bind (new IPEndPoint(IPAddress.Parse ("127.0.0.1"), 13578));
			sock.Listen (1);

			thread = new Thread (() => {
				Console.WriteLine ("Before Accept");
				try {
						sock.Accept ();

						Console.WriteLine ("[error] After Accept");
				} catch (SocketException e) {
						Console.WriteLine ("Accept : SocketException: {0}", e.Message);
				}
				Console.WriteLine ("After  Accept");

				mre.Set ();
			});

			break;
		}
		default:
			throw new InvalidOperationException ();
		}

		thread.Start ();

		/* wait for accept to kick off */
		Thread.Sleep (1000);

		switch (Int32.Parse (args [1])) {
		case 1: {
			Console.WriteLine ("Before Blocking");
			sock.Blocking = false;
			Console.WriteLine ("After  Blocking");

			Thread.Sleep (1000);

			Console.WriteLine ("Before Shutdown");
			sock.Shutdown (SocketShutdown.Receive);
			Console.WriteLine ("After  Shutdown");

			break;
		}
		case 2: {
			Console.WriteLine ("Before Close");
			sock.Close ();
			Console.WriteLine ("After  Close");

			break;
		}
		default:
			throw new InvalidOperationException ();
		}

		thread.Join ();
	}
}
