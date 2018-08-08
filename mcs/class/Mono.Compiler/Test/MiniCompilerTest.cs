using System;
using System.Runtime.InteropServices;
using NUnit.Framework;
using Mono.Compiler;

namespace MonoTests.Mono.Compiler
{
	[TestFixture]
	public class MiniCompilerTests
	{
		IRuntimeInformation runtimeInfo = null;
		ICompiler compiler = null;

		[TestFixtureSetUp]
		public void Init () {
			runtimeInfo = null;
			compiler = new MiniCompiler ();
		}

		delegate void EmptyDelegate ();
		public static void EmptyMethod () {
		}

		delegate int AddDelegate (int a, int b);
		public static int AddMethod (int a, int b) {
			return a + b;
		}

		delegate int AddDelegate3 (int a, int b, int c);
		public static int AddMethod3 (int a, int b, int c) {
			return a + b + c;
		}

		[Test]
		public unsafe void TestEmptyMethod ()
		{
			MethodInfo methodInfo = new MethodInfo(this.GetType().GetMethod("EmptyMethod").MethodHandle);

			CompilationResult result = compiler.CompileMethod (runtimeInfo, methodInfo, CompilationFlags.None, out NativeCodeHandle nativeCode);
			Assert.AreEqual(result, CompilationResult.Ok);
			Assert.AreNotEqual((IntPtr)nativeCode.Blob, IntPtr.Zero);
			// AssertHelper.Greater(nativeCode.Length, 0);

			Marshal.GetDelegateForFunctionPointer<EmptyDelegate> ((IntPtr) nativeCode.Blob) ();
		}

		[Test]
		public unsafe void TestAddMethod ()
		{
			MethodInfo methodInfo = new MethodInfo(this.GetType().GetMethod("AddMethod").MethodHandle);

			CompilationResult result = compiler.CompileMethod (runtimeInfo, methodInfo, CompilationFlags.None, out NativeCodeHandle nativeCode);
			Assert.AreEqual(result, CompilationResult.Ok);
			Assert.AreNotEqual((IntPtr)nativeCode.Blob, IntPtr.Zero);
			// AssertHelper.Greater(nativeCode.Length, 0);

			Assert.AreEqual (3, Marshal.GetDelegateForFunctionPointer<AddDelegate> ((IntPtr) nativeCode.Blob) (1, 2));
		}

		[Test]
		public unsafe void TestAddMethod3 ()
		{
			MethodInfo methodInfo = new MethodInfo(this.GetType().GetMethod("AddMethod3").MethodHandle);

			CompilationResult result = compiler.CompileMethod (runtimeInfo, methodInfo, CompilationFlags.None, out NativeCodeHandle nativeCode);
			Assert.AreEqual(result, CompilationResult.Ok);
			Assert.AreNotEqual((IntPtr)nativeCode.Blob, IntPtr.Zero);
			// AssertHelper.Greater(nativeCode.Length, 0);

			Assert.AreEqual (6, Marshal.GetDelegateForFunctionPointer<AddDelegate3> ((IntPtr) nativeCode.Blob) (1, 2, 3));
		}
	}
}
