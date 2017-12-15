using System;
using System.Threading;

class Driver
{
	public static void Main ()
	{
		Thread t1 = new Thread(() => {
			for (int i = 0; i < 10; ++i) {
				Thread t2 = new Thread(() => {
					while (true) {
						Thread t3 = new Thread(() => {});
						t3.Start();
						try {
							while (true) {
								t3.Suspend ();
								t3.Resume ();
								Thread.Yield ();
							}
						} catch {
						}
					}
				});

				t2.Start ();
			}
		});

		t1.Start ();

		Thread.Sleep (10000);

		Environment.Exit (0);
	}
}
