using System;
using System.Net;
using System.Net.Sockets;
using System.Threading;

class Driver
{
	public static void Main ()
	{
		Socket listener;

		listener = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
		listener.Bind (new IPEndPoint(IPAddress.Parse ("127.0.0.1"), 13578));
		listener.Listen (1);

		Thread thread = new Thread (() => {
			try {
				Socket s = listener.Accept ();
				Console.WriteLine ("After Accept, s = {0}", s);
			} catch (SocketException e) {
				Console.WriteLine ("Accept : SocketException: {0}", e.Message);
			}
		});

		thread.Start ();

		/* wait for accept to kick off */
		Thread.Sleep (100);

		/* this should cancel the syscall of Accept */
		Console.WriteLine ("Before Close");
		listener.Close ();
		Console.WriteLine ("After Close");

		Thread.Sleep (100);
	}
}