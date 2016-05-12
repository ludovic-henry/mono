using System;
using System.Reflection;
using System.Text;

class GenJitRegressionTests {

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
		var outputFile = args[0];
		var opts = args[1] == "0" ? "None" : ToCamelCase(args[1]);
        using (System.IO.StreamWriter writer = new System.IO.StreamWriter(outputFile))
        {
            writer.WriteLine("#include \"jit-regression-tests.h\"");
            writer.WriteLine();

            string[] assemblyNames = new string[args.Length - 2];
            Array.Copy(args, 2, assemblyNames, 0, assemblyNames.Length);
            foreach (var assemblyName in assemblyNames)
	        {
	            var assembly = Assembly.LoadFrom(AppDomain.CurrentDomain.BaseDirectory + assemblyName);
	            var type = assembly.GetType("Tests");
	            var methods = type.GetMethods();
	            var testClassName = ToCamelCase(assembly.GetName().Name) + "ExeWithOpts" + opts;
	            writer.WriteLine("DEFINE_TEST_CLASS(" + testClassName + ", \"" + assemblyName + "\", \"" + type + "\")");
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
