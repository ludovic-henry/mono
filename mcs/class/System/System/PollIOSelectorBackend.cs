
using System;

class PollIOSelectorBackend : IOSelectorBackend
{
	public PollIOSelectorBackend (SafeHandle wakeup_pipe)
		: base (wakeup_pipe)
	{
	}

	public override void Add (SafeHandle handle, IOOperation operations, bool is_new)
	{
		throw new NotImplementedException ();
	}

	public override void Remove (SafeHandle handle)
	{
		throw new NotImplementedException ();
	}

	public override int Poll (ref BackendEvent[] events)
	{
		throw new NotImplementedException ();
	}

	protected override void Dispose (bool disposing)
	{
	}
}