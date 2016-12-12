//
// System.Threading.Thread.cs
//
// Author:
//   Dick Porter (dick@ximian.com)
//
// (C) Ximian, Inc.  http://www.ximian.com
// Copyright (C) 2004-2006 Novell, Inc (http://www.novell.com)
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

using System.Runtime.Remoting.Contexts;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Formatters.Binary;
using System.Security.Permissions;
using System.Security.Principal;
using System.Globalization;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.IO;
using System.Collections.Generic;
using System.Reflection;
using System.Security;
using System.Diagnostics;
using System.Runtime.ConstrainedExecution;

namespace System.Threading {
	[StructLayout (LayoutKind.Sequential)]
	struct InternalThread
	{
		IntPtr data;

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static void Construct (Thread thread, out InternalThreadContainer container);

		internal static bool IsNull (InternalThread thread)
		{
			return thread.data == IntPtr.Zero;
		}

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern bool GetIsThreadPoolThread ();
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern void SetIsThreadPoolThread (bool value);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern int GetStackSize ();
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern void SetStackSize (int value);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern byte GetApartmentState ();
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern void SetApartmentState (byte value);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern void IncCriticalRegionLevel ();
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern void DecCriticalRegionLevel ();

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern int GetManagedID ();
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern void SetManagedID (int value);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern byte[] GetSerializedPrincipal ();
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern void SetSerializedPrincipal (byte[] value);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern int GetSerializedPrincipalVersion ();
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern void SetSerializedPrincipalVersion (int value);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern long GetThreadID ();
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern void SetThreadID (long value);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern string GetName ();
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern void SetName (string value);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern int GetState ();
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern void SetState (int value);
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern void ClrState (int value);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern void Abort (object stateInfo);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern void Unref ();
	}

	/*
	 * This class is only used to keep a reference to a InternalThread.
	 *
	 * We use it so we can enjoy the GC's memory management to ensure we unref the
	 * InternalThread struct only when it's not accessible from anywhere anymore.
	 *
	 * It is constructed in the runtime in threads.c.
	 */
	[StructLayout (LayoutKind.Sequential)]
	sealed class InternalThreadContainer : CriticalFinalizerObject
	{
		InternalThread Thread;

		public InternalThreadContainer (InternalThread thread)
		{
			Thread = thread;
		}

		~InternalThreadContainer()
		{
			Thread.Unref ();
		}
	}

	[StructLayout (LayoutKind.Sequential)]
	public sealed partial class Thread {
#pragma warning disable 414		
		#region Sync with metadata/object-internals.h
		InternalThread internal_thread;
		InternalThreadContainer internal_thread_container;
		object m_ThreadStartArg;
		object pending_exception;
		#endregion
#pragma warning restore 414

		IPrincipal principal;
		int principal_version;

		// the name of current_thread is
		// important because they are used by the runtime.

		[ThreadStatic]
		static Thread current_thread;

		// can be both a ThreadStart and a ParameterizedThreadStart
		private MulticastDelegate m_Delegate;

		private ExecutionContext m_ExecutionContext;    // this call context follows the logical thread

		private bool m_ExecutionContextBelongsToOuterScope;

		InternalThread Internal {
			get {
				if (InternalThread.IsNull (internal_thread))
					InternalThread.New (this, out internal_thread_container);
				return internal_thread;
			}
		}

		public static Context CurrentContext {
			[SecurityPermission (SecurityAction.LinkDemand, Infrastructure=true)]
			get {
				return(AppDomain.InternalGetContext ());
			}
		}

		/*
		 * These two methods return an array in the target
		 * domain with the same content as the argument.  If
		 * the argument is already in the target domain, then
		 * the argument is returned, otherwise a copy.
		 */
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		private extern static byte[] ByteArrayToRootDomain (byte[] arr);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		private extern static byte[] ByteArrayToCurrentDomain (byte[] arr);

		static void DeserializePrincipal (Thread th)
		{
			MemoryStream ms = new MemoryStream (ByteArrayToCurrentDomain (th.Internal.GetSerializedPrincipal ()));
			int type = ms.ReadByte ();
			if (type == 0) {
				BinaryFormatter bf = new BinaryFormatter ();
				th.principal = (IPrincipal) bf.Deserialize (ms);
				th.principal_version = th.Internal.GetSerializedPrincipalVersion ();
			} else if (type == 1) {
				BinaryReader reader = new BinaryReader (ms);
				string name = reader.ReadString ();
				string auth_type = reader.ReadString ();
				int n_roles = reader.ReadInt32 ();
				string [] roles = null;
				if (n_roles >= 0) {
					roles = new string [n_roles];
					for (int i = 0; i < n_roles; i++)
						roles [i] = reader.ReadString ();
				}
				th.principal = new GenericPrincipal (new GenericIdentity (name, auth_type), roles);
			} else if (type == 2 || type == 3) {
				string [] roles = type == 2 ? null : new string [0];
				th.principal = new GenericPrincipal (new GenericIdentity ("", ""), roles);
			}
		}

		static void SerializePrincipal (Thread th, IPrincipal value)
		{
			MemoryStream ms = new MemoryStream ();
			bool done = false;
			if (value.GetType () == typeof (GenericPrincipal)) {
				GenericPrincipal gp = (GenericPrincipal) value;
				if (gp.Identity != null && gp.Identity.GetType () == typeof (GenericIdentity)) {
					GenericIdentity id = (GenericIdentity) gp.Identity;
					if (id.Name == "" && id.AuthenticationType == "") {
						if (gp.Roles == null) {
							ms.WriteByte (2);
							done = true;
						} else if (gp.Roles.Length == 0) {
							ms.WriteByte (3);
							done = true;
						}
					} else {
						ms.WriteByte (1);
						BinaryWriter br = new BinaryWriter (ms);
						br.Write (gp.Identity.Name);
						br.Write (gp.Identity.AuthenticationType);
						string [] roles = gp.Roles;
						if  (roles == null) {
							br.Write ((int) (-1));
						} else {
							br.Write (roles.Length);
							foreach (string s in roles) {
								br.Write (s);
							}
						}
						br.Flush ();
						done = true;
					}
				}
			}
			if (!done) {
				ms.WriteByte (0);
				BinaryFormatter bf = new BinaryFormatter ();
				try {
					bf.Serialize (ms, value);
				} catch {}
			}
			th.Internal.SetSerializedPrincipal (ByteArrayToRootDomain (ms.ToArray ()));
		}

		public static IPrincipal CurrentPrincipal {
			get {
				Thread th = CurrentThread;

				if (th.principal_version != th.Internal.GetSerializedPrincipalVersion ())
					th.principal = null;

				if (th.principal != null)
					return th.principal;

				if (th.Internal.GetSerializedPrincipal () != null) {
					try {
						DeserializePrincipal (th);
						return th.principal;
					} catch {}
				}

				th.principal = GetDomain ().DefaultPrincipal;
				th.principal_version = th.Internal.GetSerializedPrincipalVersion ();
				return th.principal;
			}
			[SecurityPermission (SecurityAction.Demand, ControlPrincipal = true)]
			set {
				Thread th = CurrentThread;

				if (value != GetDomain ().DefaultPrincipal) {
					th.Internal.SetSerializedPrincipalVersion (th.Internal.GetSerializedPrincipalVersion () + 1);
					try {
						SerializePrincipal (th, value);
					} catch (Exception) {
						th.Internal.SetSerializedPrincipal (null);
					}
					th.principal_version = th.Internal.GetSerializedPrincipalVersion ();
				} else {
					th.Internal.SetSerializedPrincipal (null);
				}

				th.principal = value;
			}
		}

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		private extern static Thread GetCurrentThread ();

		public static Thread CurrentThread {
			[ReliabilityContract (Consistency.WillNotCorruptState, Cer.MayFail)]
			get {
				Thread current = current_thread;
				if (current != null)
					return current;
				// This will set the current_thread tls variable
				return GetCurrentThread ();
			}
		}

		internal static int CurrentThreadId {
			get {
				return (int)(CurrentThread.Internal.GetThreadID ());
			}
		}

		public static AppDomain GetDomain() {
			return AppDomain.CurrentDomain;
		}

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		public extern static int GetDomainID();

		// Returns the system thread handle
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		private extern IntPtr Thread_internal (MulticastDelegate start);

		// part of ".NETPortable,Version=v4.0,Profile=Profile3" i.e. FX4 and SL4
		[ReliabilityContract (Consistency.WillNotCorruptState, Cer.Success)]
		~Thread ()
		{
		}

		[Obsolete ("Deprecated in favor of GetApartmentState, SetApartmentState and TrySetApartmentState.")]
		public ApartmentState ApartmentState {
			get {
				if ((ThreadState & ThreadState.Stopped) != 0)
					throw new ThreadStateException ("Thread is dead; state can not be accessed.");

				return (ApartmentState)Internal.GetApartmentState ();
			}

			set {
				TrySetApartmentState (value);
			}
		}

		public bool IsThreadPoolThread {
			get {
				return IsThreadPoolThreadInternal;
			}
		}

		internal bool IsThreadPoolThreadInternal {
			get {
				return Internal.GetIsThreadPoolThread ();
			}
			set {
				Internal.SetIsThreadPoolThread (value);
			}
		}

		public bool IsAlive {
			get {
				ThreadState curstate = (ThreadState) Internal.GetState ();
				
				if((curstate & ThreadState.Aborted) != 0 ||
				   (curstate & ThreadState.Stopped) != 0 ||
				   (curstate & ThreadState.Unstarted) != 0) {
					return(false);
				} else {
					return(true);
				}
			}
		}

		public bool IsBackground {
			get {
				ThreadState thread_state = (ThreadState) Internal.GetState ();
				if ((thread_state & ThreadState.Stopped) != 0)
					throw new ThreadStateException ("Thread is dead; state can not be accessed.");

				return (thread_state & ThreadState.Background) != 0;
			}
			
			set {
				if (value) {
					Internal.SetState ((int) ThreadState.Background);
				} else {
					Internal.ClrState ((int) ThreadState.Background);
				}
			}
		}

		/* 
		 * The thread name must be shared by appdomains, so it is stored in
		 * unmanaged code.
		 */

		public string Name {
			get { return Internal.GetName (); }
			set { Internal.SetName (value); }
		}

		public ThreadState ThreadState {
			get {
				return (ThreadState) Internal.GetState ();
			}
		}

#if MONO_FEATURE_THREAD_ABORT
		[SecurityPermission (SecurityAction.Demand, ControlThread=true)]
		public void Abort () 
		{
			Internal.Abort (null);
		}

		[SecurityPermission (SecurityAction.Demand, ControlThread=true)]
		public void Abort (object stateInfo) 
		{
			Internal.Abort (stateInfo);
		}

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		extern object GetAbortExceptionState ();

		internal object AbortReason {
			get {
				return GetAbortExceptionState ();
			}
		}

		void ClearAbortReason ()
		{
		}
#else
		[Obsolete ("Thread.Abort is not supported on the current platform.", true)]
		public void Abort ()
		{
			throw new PlatformNotSupportedException ("Thread.Abort is not supported on the current platform.");
		}

		[Obsolete ("Thread.Abort is not supported on the current platform.", true)]
		public void Abort (object stateInfo)
		{
			throw new PlatformNotSupportedException ("Thread.Abort is not supported on the current platform.");
		}

		[Obsolete ("Thread.ResetAbort is not supported on the current platform.", true)]
		public static void ResetAbort ()
		{
			throw new PlatformNotSupportedException ("Thread.ResetAbort is not supported on the current platform.");
		}

		internal object AbortReason {
			get {
				throw new PlatformNotSupportedException ("Thread.ResetAbort is not supported on the current platform.");
			}
		}
#endif // MONO_FEATURE_THREAD_ABORT

		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		private extern static void SpinWait_nop ();


		[ReliabilityContractAttribute (Consistency.WillNotCorruptState, Cer.Success)]
		public static void SpinWait (int iterations) 
		{
			if (iterations < 0)
				return;
			while (iterations-- > 0)
			{
				SpinWait_nop ();
			}
		}

		void StartInternal (IPrincipal principal, ref StackCrawlMark stackMark)
		{
#if FEATURE_ROLE_BASED_SECURITY
			Internal.SetSerializedPrincipal (CurrentThread.Internal.GetSerializedPrincipal ());
#endif

			// Thread_internal creates and starts the new thread, 
			if (Thread_internal(m_Delegate) == IntPtr.Zero)
				throw new SystemException ("Thread creation failed.");

			m_ThreadStartArg = null;
		}

		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		extern public static byte VolatileRead (ref byte address);
		
		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		extern public static double VolatileRead (ref double address);
		
		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		extern public static short VolatileRead (ref short address);
		
		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		extern public static int VolatileRead (ref int address);
		
		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		extern public static long VolatileRead (ref long address);
		
		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		extern public static IntPtr VolatileRead (ref IntPtr address);
		
		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		extern public static object VolatileRead (ref object address);

		[CLSCompliant(false)]
		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		extern public static sbyte VolatileRead (ref sbyte address);
		
		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		extern public static float VolatileRead (ref float address);

		[CLSCompliant (false)]
		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		extern public static ushort VolatileRead (ref ushort address);

		[CLSCompliant (false)]
		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		extern public static uint VolatileRead (ref uint address);

		[CLSCompliant (false)]
		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		extern public static ulong VolatileRead (ref ulong address);

		[CLSCompliant (false)]
		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		extern public static UIntPtr VolatileRead (ref UIntPtr address);

		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		extern public static void VolatileWrite (ref byte address, byte value);
		
		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		extern public static void VolatileWrite (ref double address, double value);
		
		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		extern public static void VolatileWrite (ref short address, short value);
		
		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		extern public static void VolatileWrite (ref int address, int value);
		
		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		extern public static void VolatileWrite (ref long address, long value);
		
		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		extern public static void VolatileWrite (ref IntPtr address, IntPtr value);
		
		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		extern public static void VolatileWrite (ref object address, object value);

		[CLSCompliant(false)]
		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		extern public static void VolatileWrite (ref sbyte address, sbyte value);
		
		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		extern public static void VolatileWrite (ref float address, float value);

		[CLSCompliant (false)]
		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		extern public static void VolatileWrite (ref ushort address, ushort value);

		[CLSCompliant (false)]
		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		extern public static void VolatileWrite (ref uint address, uint value);

		[CLSCompliant (false)]
		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		extern public static void VolatileWrite (ref ulong address, ulong value);

		[CLSCompliant (false)]
		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		extern public static void VolatileWrite (ref UIntPtr address, UIntPtr value);
		
		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		extern static int SystemMaxStackStize ();

		static int GetProcessDefaultStackSize (int maxStackSize)
		{
			if (maxStackSize == 0)
				return 0;

			if (maxStackSize < 131072) // make sure stack is at least 128k big
				return 131072;

			int page_size = Environment.GetPageSize ();

			if ((maxStackSize % page_size) != 0) // round up to a divisible of page size
				maxStackSize = (maxStackSize / (page_size - 1)) * page_size;

			/* Respect the max stack size imposed by the system*/
			return Math.Min (maxStackSize, SystemMaxStackStize ());
		}

		void SetStart (MulticastDelegate start, int maxStackSize)
		{
			m_Delegate = start;
			Internal.SetStackSize (maxStackSize);
		}

		public int ManagedThreadId {
			[ReliabilityContractAttribute (Consistency.WillNotCorruptState, Cer.Success)]
			get {
				return Internal.GetManagedID ();
			}
		}

		[ReliabilityContract (Consistency.WillNotCorruptState, Cer.MayFail)]
		public static void BeginCriticalRegion ()
		{
			CurrentThread.Internal.IncCriticalRegionLevel ();
		}

		[ReliabilityContract (Consistency.WillNotCorruptState, Cer.Success)]
		public static void EndCriticalRegion ()
		{
			CurrentThread.Internal.DecCriticalRegionLevel ();
		}

		[ReliabilityContractAttribute (Consistency.WillNotCorruptState, Cer.MayFail)]
		public static void BeginThreadAffinity ()
		{
			// Managed and native threads are currently bound together.
		}

		[ReliabilityContractAttribute (Consistency.WillNotCorruptState, Cer.MayFail)]
		public static void EndThreadAffinity ()
		{
			// Managed and native threads are currently bound together.
		}

		public ApartmentState GetApartmentState ()
		{
			return (ApartmentState)Internal.GetApartmentState ();
		}

		public void SetApartmentState (ApartmentState state)
		{
			if (!TrySetApartmentState (state))
				throw new InvalidOperationException ("Failed to set the specified COM apartment state.");
		}

		public bool TrySetApartmentState (ApartmentState state) 
		{
			if ((ThreadState & ThreadState.Unstarted) == 0)
				throw new ThreadStateException ("Thread was in an invalid state for the operation being executed.");

			if ((ApartmentState)Internal.GetApartmentState () != ApartmentState.Unknown && 
			    (ApartmentState)Internal.GetApartmentState () != state)
				return false;

			Internal.SetApartmentState ((byte)state);

			return true;
		}
		
		[ComVisible (false)]
		public override int GetHashCode ()
		{
			return ManagedThreadId;
		}

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal static extern void GetStackTraces (out Thread[] threads, out object[] stack_frames);

		// This is a mono extension to gather the stack traces for all running threads
		internal static Dictionary<Thread, StackTrace> Mono_GetStackTraces () {
			Thread[] threads;
			object[] stack_frames;

			GetStackTraces (out threads, out stack_frames);

			var res = new Dictionary<Thread, StackTrace> ();
			for (int i = 0; i < threads.Length; ++i)
				res [threads [i]] = new StackTrace ((StackFrame[])stack_frames [i]);
			return res;
		}

#if !MONO_FEATURE_THREAD_SUSPEND_RESUME
		[Obsolete ("Thread.Suspend is not supported on the current platform.", true)]
		public void Suspend ()
		{
			throw new PlatformNotSupportedException ("Thread.Suspend is not supported on the current platform.");
		}

		[Obsolete ("Thread.Resume is not supported on the current platform.", true)]
		public void Resume ()
		{
			throw new PlatformNotSupportedException ("Thread.Resume is not supported on the current platform.");
		}
#endif

		public void DisableComObjectEagerCleanup ()
		{
			throw new PlatformNotSupportedException ();
		}
	}
}
