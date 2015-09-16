
using System.Runtime.CompilerServices;
using System.Runtime.Versioning;

namespace System
{
	public abstract partial class Delegate
	{
		[System.Security.SecurityCritical]  // auto-generated
		[ResourceExposure(ResourceScope.None)]
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		private extern bool BindToMethodName(Object target, RuntimeType methodType, String method, DelegateBindingFlags flags);

		[System.Security.SecurityCritical]  // auto-generated
		[ResourceExposure(ResourceScope.None)]
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		private extern bool BindToMethodInfo(Object target, IRuntimeMethodInfo method, RuntimeType methodType, DelegateBindingFlags flags);

		[System.Security.SecurityCritical]  // auto-generated
		[ResourceExposure(ResourceScope.None)]
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		private extern static MulticastDelegate InternalAlloc(RuntimeType type);

		[System.Security.SecurityCritical]  // auto-generated
		[ResourceExposure(ResourceScope.None)]
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static MulticastDelegate InternalAllocLike(Delegate d);

		[System.Security.SecurityCritical]  // auto-generated
		[ResourceExposure(ResourceScope.None)]
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static bool InternalEqualTypes(object a, object b);

		// Used by the ctor. Do not call directly.
		// The name of this function will appear in managed stacktraces as delegate constructor.
		[System.Security.SecurityCritical]  // auto-generated
		[ResourceExposure(ResourceScope.None)]
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		private extern void DelegateConstruct(Object target, IntPtr slot);

		[System.Security.SecurityCritical]  // auto-generated
		[ResourceExposure(ResourceScope.None)]
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern IntPtr GetMulticastInvoke();

		#if !FEATURE_CORECLR
		[System.Runtime.ForceTokenStabilization] // called from IL stubs
		#endif //!FEATURE_CORECLR
		[System.Security.SecurityCritical]  // auto-generated
		[ResourceExposure(ResourceScope.None)]
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern IntPtr GetInvokeMethod();

		[ResourceExposure(ResourceScope.None)]
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern IRuntimeMethodInfo FindMethodHandle();

		[System.Security.SecurityCritical]  // auto-generated
		[ResourceExposure(ResourceScope.None)]
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static bool InternalEqualMethodHandles(Delegate left, Delegate right);

		[System.Security.SecurityCritical]  // auto-generated
		[ResourceExposure(ResourceScope.None)]
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern IntPtr AdjustTarget(Object target, IntPtr methodPtr);

		[System.Security.SecurityCritical]  // auto-generated
		[ResourceExposure(ResourceScope.None)]
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern IntPtr GetCallStub(IntPtr methodPtr);

		[System.Security.SecurityCritical]  // auto-generated
		[ResourceExposure(ResourceScope.None)]
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static bool CompareUnmanagedFunctionPtrs (Delegate d1, Delegate d2);
		}
}