using System;
using System.Threading;

class Driver
{
    const int RUN = 1 * 1000 * 1000;

    static void Main ()
    {
        // ManualResetEvent mre = new ManualResetEvent(false);
        // int completed = 0;

        for (int i = 1; i <= RUN; ++i)
        {
            int local_i = i;

            ThreadPool.QueueUserWorkItem (
                _ => {
                    int sum = 0;
                    for (int j = 1; j <= 10 * 1000; ++j)
                    {
                        sum += j;

                        if (j % 1000 == 0)
                            Thread.Sleep (0);
                    }

                    if (local_i % (RUN / 100) == 0)
                        Console.Write (".");

                    // if (Interlocked.Increment (ref completed) == RUN)
                    //     mre.Set ();
                },
                null
            );

        }

        // mre.WaitOne ();
    }
}
