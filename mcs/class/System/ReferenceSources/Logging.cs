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

		internal static TraceSource WebSockets
		{
			get
			{
				return null;
			}
		}

		internal static void Associate(TraceSource traceSource, object objA, object objB) {}

		internal static void Dump(TraceSource traceSource, object obj, string method, IntPtr bufferPtr, int length) {}
		internal static void Dump(TraceSource traceSource, object obj, string method, byte[] buffer, int offset, int length) {}

		internal static void Enter(TraceSource traceSource, string msg) {}
		[Conditional ("TRACE")]
 		internal static void Enter(TraceSource traceSource, object obj, string method, object paramObject) {}

 		internal static void Exit(TraceSource traceSource, string msg) {}
		[Conditional ("TRACE")]
		internal static void Exit(TraceSource traceSource, object obj, string method, object retObject) {}

		internal static void Exception(TraceSource traceSource, object obj, string method, Exception e) {}

		internal static void PrintInfo(TraceSource traceSource, string msg) {}
		internal static void PrintInfo(TraceSource traceSource, object obj, string msg) {}
		internal static void PrintInfo(TraceSource traceSource, object obj, string method, string msg) {}

		internal static void PrintWarning(TraceSource traceSource, string msg) {}
		internal static void PrintWarning(TraceSource traceSource, object obj, string method, string msg) {}

		internal static void PrintError(TraceSource traceSource, string msg) {}
		internal static void PrintError(TraceSource traceSource, object obj, string method, string msg) {}

		internal static string GetObjectLogHash(object obj)
		{
			return GetObjectName(obj) + "#" + ValidationHelper.HashString(obj);
		}

		private static string GetObjectName(object obj) {
			if (obj is Uri || obj is System.Net.IPAddress || obj is System.Net.IPEndPoint) {
				return obj.ToString();
			} else {
				return obj.GetType().Name;
			}
		}
	}

#if MOBILE

	class TraceSource
	{
		
	}

#endif
}
#endif
