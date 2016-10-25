
using System;
using System.Threading;

class Driver
{
	public static void Main ()
	{
		bool shutdown = false;

		Thread gc = new Thread (() => {
			while (!shutdown) {
				GC.Collect ();
				Thread.Yield ();
			}
		});

		gc.Start ();

		for (int i = 0; i < 5000; ++i)
			Thread.Sleep (1);

		shutdown = true;
		gc.Join ();
	}
}