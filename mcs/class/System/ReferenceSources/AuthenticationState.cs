
namespace System.Net
{
	internal class AuthenticationState
	{
		internal Authorization Authorization;

		internal IAuthenticationModule Module;

		internal string UniqueGroupId;

		internal Uri ChallengedUri;

		internal TransportContext TransportContext
		{
			get { throw new NotSupportedException (); }
			set { throw new NotSupportedException (); }
		}

		internal HttpResponseHeader AuthenticateHeader
		{
			get { throw new NotSupportedException (); }
		}

		internal string AuthorizationHeader
		{
			get { throw new NotSupportedException (); }
		}

		internal HttpStatusCode StatusCodeMatch
		{
			get { throw new NotSupportedException (); }
		}

		internal AuthenticationState(bool isProxyAuth)
		{
			throw new NotSupportedException ();
		}

		internal SpnToken GetComputeSpn(HttpWebRequest httpWebRequest)
		{
			throw new NotSupportedException ();
		}

		internal void PreAuthIfNeeded(HttpWebRequest httpWebRequest, ICredentials authInfo)
		{
			throw new NotSupportedException ();
		}

		internal bool AttemptAuthenticate(HttpWebRequest httpWebRequest, ICredentials authInfo)
		{
			throw new NotSupportedException ();
		}

		internal void ClearAuthReq(HttpWebRequest httpWebRequest)
		{
			throw new NotSupportedException ();
		}

		internal void Update(HttpWebRequest httpWebRequest)
		{
			throw new NotSupportedException ();
		}

		internal void ClearSession()
		{
			throw new NotSupportedException ();
		}

		internal void ClearSession(HttpWebRequest httpWebRequest)
		{
			throw new NotSupportedException ();
		}
	}
}
