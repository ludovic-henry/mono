
using System.Runtime.InteropServices;

namespace System
{
	[StructLayout (LayoutKind.Sequential)]
	class MonoManagedObject
	{
		object obj;

		GCHandle handle_this;
		GCHandle handle_obj;

		MonoManagedObject (object obj)
		{
			this.obj = obj;
			this.handle_this = GCHandle.Alloc (this, GCHandleType.Pinned);
			this.handle_obj = GCHandle.Alloc (obj, GCHandleType.Pinned);
		}

		public static MonoManagedObject Create (object obj)
		{
			return new MonoManagedObject (obj);
		}

		public static MonoManagedObject Duplicate (MonoManagedObject mobj)
		{
			return new MonoManagedObject (mobj.obj);
		}

		public static void Destroy (MonoManagedObject mobj)
		{
			mobj.handle_this.Free ();
		}

		~MonoManagedObject ()
		{
			this.handle_obj.Free ();
		}
	}
}
