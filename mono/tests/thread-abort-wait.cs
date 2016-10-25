
using System;
using System.Threading;

class Driver
{
	public static int Main ()
	{
		ManualResetEvent mre = new ManualResetEvent (false);

		Thread t = new Thread (() => {
			mre.WaitOne (5000);
		});

		t.Start ();

		Thread.Sleep (100);

		t.Abort ();
		if (!t.Join (500)) {
			mre.Set ();
			return 1;
		}

		return 0;
	}
}
