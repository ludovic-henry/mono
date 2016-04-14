
using System.Threading;

namespace System.Net 
{  
	internal sealed class Semaphore : WaitHandle
	{
		internal Semaphore (int initialCount, int maxCount)
			: base()
		{
			throw new NotSupportedException ();
		}

		internal bool ReleaseSemaphore()
		{
			throw new NotSupportedException ();
		}
	}
}
