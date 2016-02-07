
using System.Reflection;
using System.Runtime.Remoting.Metadata;

namespace System.Runtime.Remoting
{
	partial class RemotingServices
	{
		// This is used by the cached type data to figure out which type name
		//   to use (this should never be publicly exposed; GetDefaultQualifiedTypeName should,
		//   if anything at all)
		[System.Security.SecurityCritical]  // auto-generated
		internal static String DetermineDefaultQualifiedTypeName(Type type)
		{
			if (type == null)
				throw new ArgumentNullException("type");

			// see if there is an xml type name
			String xmlTypeName = null;
			String xmlTypeNamespace = null;
			if (SoapServices.GetXmlTypeForInteropType(type, out xmlTypeName, out xmlTypeNamespace))
			{
				return "soap:" + xmlTypeName + ", " + xmlTypeNamespace;
			}

			// there are no special mappings, so use the fully qualified CLR type name
			// <
			return type.AssemblyQualifiedName;
		}

		[System.Security.SecurityCritical]  // auto-generated
		internal static String GetDefaultQualifiedTypeName(RuntimeType type)
		{
			RemotingTypeCachedData cache = (RemotingTypeCachedData) InternalRemotingServices.GetReflectionCachedData(type);

			return cache.QualifiedTypeName;
		}
	}

	[System.Security.SecurityCritical]  // auto-generated_required
	[System.Runtime.InteropServices.ComVisible(true)]
	partial class InternalRemotingServices
	{
		// access attribute cached on the reflection object
		internal static RemotingMethodCachedData GetReflectionCachedData(MethodBase mi)
		{
			RuntimeMethodInfo rmi = null;
			RuntimeConstructorInfo rci = null;

			if ((rmi = mi as RuntimeMethodInfo) != null)
				return rmi.RemotingCache;
			else if ((rci = mi as RuntimeConstructorInfo) != null)
				return rci.RemotingCache;
			else
				throw new ArgumentException(Environment.GetResourceString("Argument_MustBeRuntimeReflectionObject"));
		}
	}
}