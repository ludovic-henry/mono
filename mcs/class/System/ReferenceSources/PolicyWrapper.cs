
using System.Net.Security;
using System.Security.Cryptography.X509Certificates;
using Microsoft.Win32.SafeHandles;

namespace System.Net
{
	internal class PolicyWrapper
	{
		internal PolicyWrapper (ICertificatePolicy policy, ServicePoint sp, WebRequest wr)
		{
		}

		public bool Accept (X509Certificate Certificate, int CertificateProblem)
		{
			throw new NotImplementedException ();
		}

		internal static uint VerifyChainPolicy (SafeFreeCertChain chainContext, ref ChainPolicyParameter cpp)
		{
			throw new NotImplementedException ();
		}

		internal bool CheckErrors (string hostName, X509Certificate certificate, X509Chain chain, SslPolicyErrors sslPolicyErrors)
		{
			throw new NotImplementedException ();
		}
	}

	internal sealed class SafeFreeCertChain : SafeHandleZeroOrMinusOneIsInvalid
	{
		internal SafeFreeCertChain (IntPtr handle) : base (true)
		{
			SetHandle (handle);
		}

		protected override bool ReleaseHandle ()
		{
			return true;
		}
	}

	internal unsafe struct ChainPolicyParameter
	{
	}
}
