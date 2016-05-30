
abstract class IOSelectorBackend : IDisposable
{
	public SafeHandle WakeupPipe;

	public IOSelectorBackend (SafeHandle wakeup_pipe)
	{
		WakeupPipe = wakeup_pipe;
	}

	public abstract void Add (SafeHandle handle, IOOperation operations, bool is_new);

	public abstract void Remove (SafeHandle handle);

	public abstract int Poll (ref BackendEvent[] events);

	void IDisposable.Dispose ()
	{
		Dispose (true);
		GC.SuppressFinalize (this);
	}

	void ~IOSelectorBackend ()
	{
		Dispose (false);
	}

	protected abstract void Dispose (bool disposing);
}