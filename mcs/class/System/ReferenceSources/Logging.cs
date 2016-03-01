#if !MONO_FEATURE_NEW_TLS
using System.Diagnostics;

namespace System.Net {
	class Logging {
		internal static bool On {
			get {
				return false;
			}
		}

		internal static TraceSource Web {
			get {
				return null;
			}
		}

		internal static TraceSource HttpListener {
			get {
				return null;
			}
		}

		internal static TraceSource RequestCache {
			get {
				return null;
			}
		}

		internal static bool IsVerbose(TraceSource traceSource) { return false; }

		internal static void Associate(TraceSource traceSource, object objA, object objB) {}

		internal static void Enter(TraceSource source, string method) {}
		[Conditional ("TRACE")]
		internal static void Enter(TraceSource traceSource, object obj, string method, object paramObject) {}

		internal static void Exit(TraceSource traceSource, string method) {}
		[Conditional ("TRACE")]
		internal static void Exit(TraceSource traceSource, object obj, string method, object retObject) {}

		internal static void Exception(TraceSource traceSource, object obj, string method, Exception e) {}

		internal static void Dump(TraceSource traceSource, object obj, string method, IntPtr bufferPtr, int length) {}
		internal static void Dump(TraceSource traceSource, object obj, string method, byte[] buffer, int offset, int length) {}

		internal static void PrintInfo(TraceSource traceSource, string msg) {}
		internal static void PrintInfo(TraceSource traceSource, object obj, string msg) {}
		internal static void PrintInfo(TraceSource traceSource, object obj, string method, string msg) {}

		internal static void PrintWarning(TraceSource traceSource, string msg) {}
		internal static void PrintWarning(TraceSource traceSource, object obj, string method, string msg) {}

		internal static void PrintError(TraceSource traceSource, string msg) {}
		internal static void PrintError(TraceSource traceSource, object obj, string method, string msg) {}
	}

#if MOBILE

	class TraceSource
	{
		
	}

#endif
}
#endif
