using System;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Threading;
using System.Threading.Tasks;
using System.Timers;

public class Test
{
    static int totalRequests = 0;

    public static void Main ()
    {
        ThreadPool.SetMinThreads (1, 1);
        ThreadPool.SetMaxThreads (10, 10);

        var timer = new System.Timers.Timer(500);
        timer.Elapsed += MakeWebRequest;
        timer.Start();

        UseUpThreadPool();
        Console.ReadLine();
    }

    private static void MakeWebRequest(object source, ElapsedEventArgs e)
    {
        var requestId = totalRequests++;

        try
        {
            var request = WebRequest.Create("http://download.mono-project.com/archive");
            request.Timeout = 500;
            using(var response = request.GetResponse())
            using(var reader = new StreamReader(response.GetResponseStream()))
            {
                reader.ReadToEnd();
                Log(" Request #{0} succeeded.", requestId);
            }
        }
        catch(Exception ex)
        {
            int availableThreads, availableIoThreads;
            ThreadPool.GetAvailableThreads(out availableThreads, out availableIoThreads);
            Log("Request #{0} Available Threads: {1} IO threads: {2}, ex = " + ex.ToString(), requestId, availableThreads, availableIoThreads);
        }
    }

    private static void UseUpThreadPool()
    {
        Log(" Starting threads.");
        var N = 100;

        List<Task> tasks = new List<Task>(N);
        for (int i = 0; i < N; i++)
        {
            tasks.Add(Task.Factory.StartNew(() => Thread.Sleep(2000)));
        }

        Task.WaitAll(tasks.ToArray());
        int availableThreads, availableIoThreads;
        ThreadPool.GetAvailableThreads(out availableThreads, out availableIoThreads);
        Log(" Finished running tasks. Number of available threads: {0}", availableThreads);
    }

    private static void Log(string message, params object[] args)
    {
        message = DateTime.Now.ToString() + " " + message;
        Console.WriteLine(message, args);
    }
}
