using System;
using System.Reflection;
using System.Text;

class GenJitRegressionTests {

	private static string[] OPTIMIZATIONS = {
		"none",
		"peephole",
		"branch",
		"cfold",
		"fcmov",
		"alias_analysis",
#if !DISABLE_SIMD
		"simd",
		"sse2",
		"simd+sse2",
#endif
		"branch+peephole+intrins",
		"branch+peephole+intrins+alias_analysis",
		"branch+peephole+linears",
		"branch+peephole+linears+copyprop",
		"branch+peephole+linears+cfold",
		"branch+peephole+linears+copyprop+consprop+deadce",
		"branch+peephole+linears+copyprop+consprop+deadce+alias_analysis",
		"branch+peephole+linears+copyprop+consprop+deadce+loop+inline+intrins",
		"branch+peephole+linears+copyprop+consprop+deadce+loop+inline+intrins+tailc",
		"branch+peephole+linears+copyprop+consprop+deadce+loop+inline+intrins+ssa",
		"branch+peephole+linears+copyprop+consprop+deadce+loop+inline+intrins+exception",
		"branch+peephole+linears+copyprop+consprop+deadce+loop+inline+intrins+exception+cmov",
		"branch+peephole+linears+copyprop+consprop+deadce+loop+inline+intrins+exception+abcrem",
		"branch+peephole+linears+copyprop+consprop+deadce+loop+inline+intrins+abcrem",
		"branch+peephole+linears+copyprop+consprop+deadce+loop+inline+intrins+abcrem+shared",
		"branch+peephole+linears+consprop+copyprop+deadce+loop+inline+intrins+exception+gshared+cmov+simd+alias_analysis+aot+cfold"
	};

	private static string ToCamelCase(string s)
	{
		var parts = s.Split(new Char[] { '-', '_', '+' });
		StringBuilder sb = new StringBuilder();
		foreach (var part in parts)
		{
			sb.Append(part[0].ToString().ToUpper());
			sb.Append(part.Substring(1));
		}
		return sb.ToString();
	}

	static int Main(string[] args)
	{
		var assemblyName = args[0];
		var outputFile = args[1];
		using (System.IO.StreamWriter writer = new System.IO.StreamWriter(outputFile))
		{
			writer.WriteLine("#include \"jit-regression-tests.h\"");
			writer.WriteLine();

			var assembly = Assembly.LoadFrom(AppDomain.CurrentDomain.BaseDirectory + assemblyName);
			var type = assembly.GetType("Tests");
			var methods = type.GetMethods();
			foreach (var opts in OPTIMIZATIONS)
			{
				var optsDefines = opts == "none" ? "0" : "MONO_OPT_" + opts.Replace("+", "|MONO_OPT_").ToUpper();
				var testClassName = ToCamelCase(assembly.GetName().Name) + "ExeWithOpts" + ToCamelCase(opts);
				writer.WriteLine("DEFINE_TEST_CLASS(" + testClassName + ", \"" + assemblyName + "\", \"" + type + "\", " + optsDefines + ")");
				foreach (var method in methods)
				{
					if (method.IsStatic && method.Name.StartsWith("test_") && method.ReturnType == typeof(Int32) && method.GetParameters().Length == 0)
					{
						writer.WriteLine("DEFINE_TEST(" + testClassName + ", " + ToCamelCase(method.Name) + ", \"" + method.Name + "\")");
					}
				}
				writer.WriteLine();
			}
		}
		return 0;
	}
}
