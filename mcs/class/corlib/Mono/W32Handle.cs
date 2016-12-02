
using System.Runtime.CompilerServices;

namespace Mono
{
	struct W32Handle
	{
		IntPtr handle;

		[DllImport ("__Internal", EntryPoint = "mono_w32handle_new")]
		public extern W32Handle (Type type, IntPtr data);

		public Type Type
		{
			[DllImport ("__Internal", EntryPoint = "mono_w32handle_get_type")]
			extern get;
		}

		[DllImport ("__Internal", EntryPoint = "mono_w32handle_trylock_handle")]
		public extern void TryLock ();

		[DllImport ("__Internal", EntryPoint = "mono_w32handle_lock_handle")]
		public extern void Lock ();

		[DllImport ("__Internal", EntryPoint = "mono_w32handle_unlock_handle")]
		public extern void Unlock ();

		[DllImport ("__Internal", EntryPoint = "mono_w32handle_ref")]
		public extern void Ref ();

		[DllImport ("__Internal", EntryPoint = "mono_w32handle_unref")]
		public extern void Unref ();

		[DllImport ("__Internal", EntryPoint = "mono_w32handle_set_signal_state")]
		public extern void SetSignalled ([MarshalAs (UnmanagedType.I4)] bool value, [MarshalAs (UnmanagedType.I4)] bool broadcast);

		[DllImport ("__Internal", EntryPoint = "mono_w32handle_lookup")]
		public extern bool Lookup ([MarshalAs (UnmanagedType.I4)] Type type, out IntPtr data);

		[DllImport ("__Internal", EntryPoint = "mono_w32handle_register_ops")]
		public extern static void RegisterOperations ([MarshalAs (UnmanagedType.I4)] Type type, ref Operations operations);

		[DllImport ("__Internal", EntryPoint = "mono_w32handle_register_capabilities")]
		public extern static void RegisterCapabilities ([MarshalAs (UnmanagedType.I4)] Type type, Capabilities Capabilities);

		public static W32Handle FromIntPtr (IntPtr value)
		{
			return (W32Handle) value;
		}

		public enum Type
		{
			Unused = 0,
			File,
			Console,
			Thread,
			Semaphore,
			Mutex,
			Event,
			Socket,
			Find,
			Process,
			Pipe,
			NamedMutex,
			NamedSemaphore,
			NamedEvent
		}

		public struct Operations
		{
			public delegate void Close (W32Handle handle, IntPtr data);

			public delegate void Signal (W32Handle handle);

			public delegate bool Own (W32Handle handle, out [MarshalAs (UnmanagedType.I4)] bool abandoned);

			public delegate bool IsOwned (W32Handle handle);

			public delegate WaitResult SpecialWait (W32Handle handle, uint timeout, out [MarshalAs (UnmanagedType.I4)] bool abandoned);

			public delegate void PreWait (W32Handle handle);

			public delegate void Details (IntPtr data);

			public delegate IntPtr TypeName ();

			public delegate IntPtr TypeSize ();
		}

		public enum Capabilities
		{
			Wait        = 1 << 0,
			Signal      = 1 << 1,
			Own         = 1 << 2,
			SpecialWait = 1 << 3,
		}

		public const int WaitMax = 64;

		public enum WaitResult
		{
			Success0   = 0,
			Abandoned0 = Success0 + WaitMax,
			Alerted    = -1,
			Timeout    = -2,
			Failed     = -3,
		}
	}

	public static class W32HandleNamespace
	{
		[DllImport ("__Internal", EntryPoint = "mono_w32handle_namespace_lock")]
		public static extern void Lock ();

		[DllImport ("__Internal", EntryPoint = "mono_w32handle_namespace_unlock")]
		public static extern void Unlock ();

		[DllImport ("__Internal", EntryPoint = "mono_w32handle_namespace_search_handle", CharSet = CharSet.Ansi)]
		public static extern W32Handle Search ([MarshalAs (UnmanagedType.I4)] Type type, string name);
	}
}