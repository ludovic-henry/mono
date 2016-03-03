//
// System.Net.HttpWebRequest
//
// Authors:
// 	Lawrence Pit (loz@cable.a2000.nl)
// 	Gonzalo Paniagua Javier (gonzalo@ximian.com)
//
// (c) 2002 Lawrence Pit
// (c) 2003 Ximian, Inc. (http://www.ximian.com)
// (c) 2004 Novell, Inc. (http://www.novell.com)
//

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

#if SECURITY_DEP
#if MONO_SECURITY_ALIAS
extern alias MonoSecurity;
using MonoSecurity::Mono.Security.Interface;
#else
using Mono.Security.Interface;
#endif
#endif

using System;
using System.Collections;
using System.Configuration;
using System.Globalization;
using System.IO;
using System.Net;
using System.Net.Cache;
using System.Net.Sockets;
using System.Net.Security;
using System.Runtime.Remoting.Messaging;
using System.Runtime.Serialization;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Threading;
using Mono.Net.Security;

namespace System.Net 
{
	public partial class HttpWebRequest : WebRequest, ISerializable {
		bool hostChanged;
		bool haveContentLength;
		bool haveResponse; // -> HaveResponse
		bool haveRequest;
		bool requestSent;
		string method = "GET"; // -> _OriginVerb.Name
		string initialMethod = "GET";
		bool usedPreAuth;
		Version version = HttpVersion.Version11; // -> IsVersionHttp10 ? HttpVersion.Version10 : HttpVersion.Version11
		bool force_version;
		Version actualVersion;
		bool sendChunked; // -> (_Booleans&Booleans.SendChunked) != 0
		ServicePoint servicePoint;
		
		WebConnectionStream writeStream;
		HttpWebResponse webResponse;
		WebAsyncResult asyncWrite;
		WebAsyncResult asyncRead;
		EventHandler abortHandler;
		bool gotRequestStream;
		int redirects;
		bool expectContinue;
		byte[] bodyBuffer;
		int bodyBufferLength;
		bool getResponseCalled;
		Exception saved_exc;
		object locker = new object ();
		bool finished_reading;
		internal WebConnection WebConnection;
		DecompressionMethods auto_decomp; // -> m_AutomaticDecompression
		IMonoTlsProvider tlsProvider;
#if SECURITY_DEP
		MonoTlsSettings tlsSettings;
#endif

		enum NtlmAuthState {
			None,
			Challenge,
			Response
		}
		AuthorizationState auth_state, proxy_auth_state;

		[NonSerialized]
		internal Action<Stream> ResendContentFactory;

		// Constructors
		static HttpWebRequest ()
		{
#if !NET_2_1
			NetConfig config = ConfigurationSettings.GetConfig ("system.net/settings") as NetConfig;
			if (config != null) {
				int x = config.MaxResponseHeadersLength;
				if (x != -1)
					x *= 64;
			}
#endif
		}

#if NET_2_1
		public
#else
		internal
#endif
		HttpWebRequest (Uri uri) 
		{
			this._OriginUri = uri;
			this._Uri = uri;
			this._Proxy = GlobalProxySelection.Select;
			this._HttpRequestHeaders = new WebHeaderCollection (WebHeaderCollectionType.HttpWebRequest);
			ThrowOnError = true;
			ResetAuthorization ();
		}

#if SECURITY_DEP
		internal HttpWebRequest (Uri uri, IMonoTlsProvider tlsProvider, MonoTlsSettings settings = null)
			: this (uri)
		{
			this.tlsProvider = tlsProvider;
			this.tlsSettings = settings;
		}
#endif

		void ResetAuthorization ()
		{
			auth_state = new AuthorizationState (this, false);
			proxy_auth_state = new AuthorizationState (this, true);
		}
		
		// Properties

		public Uri Address {
			get { return _Uri; }
			internal set { _Uri = value; } // Used by Ftp+proxy
		}

		internal bool InternalAllowBuffering {
			get {
				return AllowWriteStreamBuffering && MethodWithBuffer;
			}
		}

		bool MethodWithBuffer {
			get {
				return method != "HEAD" && method != "GET" &&
				method != "MKCOL" && method != "CONNECT" &&
				method != "TRACE";
			}
		}

		internal IMonoTlsProvider TlsProvider {
			get { return tlsProvider; }
		}

#if SECURITY_DEP
		internal MonoTlsSettings TlsSettings {
			get { return tlsSettings; }
		}
#endif

		internal long InternalContentLength {
			set { _ContentLength = value; }
		}

		internal bool ThrowOnError { get; set; }

		internal ServicePoint ServicePointNoLock {
			get { return servicePoint; }
		}

		internal bool GotRequestStream {
			get { return gotRequestStream; }
		}

		internal bool ExpectContinue {
			get { return expectContinue; }
			set { expectContinue = value; }
		}
		
		internal Uri AuthUri {
			get { return _Uri; }
		}
		
		internal bool ProxyQuery {
			get { return servicePoint.UsesProxy && !servicePoint.UseConnect; }
		}

		// Methods
		
		internal ServicePoint GetServicePoint ()
		{
			lock (locker) {
				if (hostChanged || servicePoint == null) {
					servicePoint = ServicePointManager.FindServicePoint (_Uri, _Proxy);
					hostChanged = false;
				}
			}

			return servicePoint;
		}

		bool CheckIfForceWrite (SimpleAsyncResult result)
		{
			if (writeStream == null || writeStream.RequestWritten || !InternalAllowBuffering)
				return false;
			if (_ContentLength < 0 && writeStream.CanWrite == true && writeStream.WriteBufferLength < 0)
				return false;

			if (_ContentLength < 0 && writeStream.WriteBufferLength >= 0)
				InternalContentLength = writeStream.WriteBufferLength;

			// This will write the POST/PUT if the write stream already has the expected
			// amount of bytes in it (ContentLength) (bug #77753) or if the write stream
			// contains data and it has been closed already (xamarin bug #1512).

			if (writeStream.WriteBufferLength == _ContentLength || (_ContentLength == -1 && writeStream.CanWrite == false))
				return writeStream.WriteRequestAsync (result);

			return false;
		}

		public override IAsyncResult BeginGetResponse (AsyncCallback callback, object state)
		{
			if (Aborted)
				throw new WebException ("The request was canceled.", WebExceptionStatus.RequestCanceled);

			if (method == null)
				throw new ProtocolViolationException ("Method is null.");

			string transferEncoding = TransferEncoding;
			if (!sendChunked && transferEncoding != null && transferEncoding.Trim () != "")
				throw new ProtocolViolationException ("SendChunked should be true.");

			Monitor.Enter (locker);
			getResponseCalled = true;
			if (asyncRead != null && !haveResponse) {
				Monitor.Exit (locker);
				throw new InvalidOperationException ("Cannot re-call start of asynchronous " +
							"method while a previous call is still in progress.");
			}

			asyncRead = new WebAsyncResult (this, callback, state);
			WebAsyncResult aread = asyncRead;
			initialMethod = method;

			SimpleAsyncResult.RunWithLock (locker, CheckIfForceWrite, inner => {
				var synch = inner.CompletedSynchronously;

				if (inner.GotException) {
					aread.SetCompleted (synch, inner.Exception);
					aread.DoCallback ();
					return;
				}

				if (haveResponse) {
					Exception saved = saved_exc;
					if (webResponse != null) {
						if (saved == null) {
							aread.SetCompleted (synch, webResponse);
						} else {
							aread.SetCompleted (synch, saved);
						}
						aread.DoCallback ();
						return;
					} else if (saved != null) {
						aread.SetCompleted (synch, saved);
						aread.DoCallback ();
						return;
					}
				}

				if (requestSent)
					return;

				try {
					requestSent = true;
					redirects = 0;
					servicePoint = GetServicePoint ();
					abortHandler = servicePoint.SendRequest (this, _ConnectionGroupName);
				} catch (Exception ex) {
					aread.SetCompleted (synch, ex);
					aread.DoCallback ();
				}
			});

			return aread;
		}

		public override WebResponse EndGetResponse (IAsyncResult asyncResult)
		{
			if (asyncResult == null)
				throw new ArgumentNullException ("asyncResult");

			WebAsyncResult result = asyncResult as WebAsyncResult;
			if (result == null)
				throw new ArgumentException ("Invalid IAsyncResult", "asyncResult");

			if (!result.WaitUntilComplete (_Timeout, false)) {
				Abort ();
				throw new WebException("The request timed out", WebExceptionStatus.Timeout);
			}

			if (result.GotException)
				throw result.Exception;

			return result.Response;
		}

		public override WebResponse GetResponse()
		{
			WebAsyncResult result = (WebAsyncResult) BeginGetResponse (null, null);
			return EndGetResponse (result);
		}
		
		internal bool FinishedReading {
			get { return finished_reading; }
			set { finished_reading = value; }
		}

		void CheckRequestStarted () 
		{
			if (requestSent)
				throw new InvalidOperationException ("request started");
		}

		internal void DoContinueDelegate (int statusCode, WebHeaderCollection headers)
		{
			if (_ContinueDelegate != null)
				_ContinueDelegate (statusCode, headers);
		}

		void RewriteRedirectToGet ()
		{
			method = "GET";
			_HttpRequestHeaders.RemoveInternal ("Transfer-Encoding");
			sendChunked = false;
		}
		
		bool Redirect (WebAsyncResult result, HttpStatusCode code, WebResponse response)
		{
			redirects++;
			Exception e = null;
			string uriString = null;
			switch (code) {
			case HttpStatusCode.Ambiguous: // 300
				e = new WebException ("Ambiguous redirect.");
				break;
			case HttpStatusCode.MovedPermanently: // 301
			case HttpStatusCode.Redirect: // 302
				if (method == "POST")
					RewriteRedirectToGet ();
				break;
			case HttpStatusCode.TemporaryRedirect: // 307
				break;
			case HttpStatusCode.SeeOther: //303
				RewriteRedirectToGet ();
				break;
			case HttpStatusCode.NotModified: // 304
				return false;
			case HttpStatusCode.UseProxy: // 305
				e = new NotImplementedException ("Proxy support not available.");
				break;
			case HttpStatusCode.Unused: // 306
			default:
				e = new ProtocolViolationException ("Invalid status code: " + (int) code);
				break;
			}

			if (method != "GET" && !InternalAllowBuffering && (writeStream.WriteBufferLength > 0 || _ContentLength > 0))
				e = new WebException ("The request requires buffering data to succeed.", null, WebExceptionStatus.ProtocolError, webResponse);

			if (e != null)
				throw e;

			if (AllowWriteStreamBuffering || method == "GET")
				_ContentLength = -1;

			uriString = webResponse.Headers ["Location"];

			if (uriString == null)
				throw new WebException ("No Location header found for " + (int) code,
							WebExceptionStatus.ProtocolError);

			Uri prev = _Uri;
			try {
				_Uri = new Uri (_Uri, uriString);
			} catch (Exception) {
				throw new WebException (String.Format ("Invalid URL ({0}) for {1}",
									uriString, (int) code),
									WebExceptionStatus.ProtocolError);
			}

			hostChanged = (_Uri.Scheme != prev.Scheme || Host != prev.Authority);
			return true;
		}

		string GetHeaders ()
		{
			bool continue100 = false;
			if (sendChunked) {
				continue100 = true;
				_HttpRequestHeaders.RemoveAndAdd ("Transfer-Encoding", "chunked");
				_HttpRequestHeaders.RemoveInternal ("Content-Length");
			} else if (_ContentLength != -1) {
				if (auth_state.NtlmAuthState == NtlmAuthState.Challenge || proxy_auth_state.NtlmAuthState == NtlmAuthState.Challenge) {
					// We don't send any body with the NTLM Challenge request.
					if (haveContentLength || gotRequestStream || _ContentLength > 0)
						_HttpRequestHeaders.SetInternal ("Content-Length", "0");
					else
						_HttpRequestHeaders.RemoveInternal ("Content-Length");
				} else {
					if (_ContentLength > 0)
						continue100 = true;

					if (haveContentLength || gotRequestStream || _ContentLength > 0)
						_HttpRequestHeaders.SetInternal ("Content-Length", _ContentLength.ToString ());
				}
				_HttpRequestHeaders.RemoveInternal ("Transfer-Encoding");
			} else {
				_HttpRequestHeaders.RemoveInternal ("Content-Length");
			}

			if (actualVersion == HttpVersion.Version11 && continue100 &&
			    servicePoint.SendContinue) { // RFC2616 8.2.3
				_HttpRequestHeaders.RemoveAndAdd ("Expect" , "100-continue");
				expectContinue = true;
			} else {
				_HttpRequestHeaders.RemoveInternal ("Expect");
				expectContinue = false;
			}

			bool proxy_query = ProxyQuery;
			string connectionHeader = (proxy_query) ? "Proxy-Connection" : "Connection";
			_HttpRequestHeaders.RemoveInternal ((!proxy_query) ? "Proxy-Connection" : "Connection");
			Version proto_version = servicePoint.ProtocolVersion;
			bool spoint10 = (proto_version == null || proto_version == HttpVersion.Version10);

			if (m_KeepAlive && (version == HttpVersion.Version10 || spoint10)) {
				if (_HttpRequestHeaders[connectionHeader] == null
				    || _HttpRequestHeaders[connectionHeader].IndexOf ("keep-alive", StringComparison.OrdinalIgnoreCase) == -1)
					_HttpRequestHeaders.RemoveAndAdd (connectionHeader, "keep-alive");
			} else if (!m_KeepAlive && version == HttpVersion.Version11) {
				_HttpRequestHeaders.RemoveAndAdd (connectionHeader, "close");
			}

			_HttpRequestHeaders.SetInternal ("Host", Host);
			if (_CookieContainer != null) {
				string cookieHeader = _CookieContainer.GetCookieHeader (_Uri);
				if (cookieHeader != "")
					_HttpRequestHeaders.RemoveAndAdd ("Cookie", cookieHeader);
				else
					_HttpRequestHeaders.RemoveInternal ("Cookie");
			}

			string accept_encoding = null;
			if ((auto_decomp & DecompressionMethods.GZip) != 0)
				accept_encoding = "gzip";
			if ((auto_decomp & DecompressionMethods.Deflate) != 0)
				accept_encoding = accept_encoding != null ? "gzip, deflate" : "deflate";
			if (accept_encoding != null)
				_HttpRequestHeaders.RemoveAndAdd ("Accept-Encoding", accept_encoding);

			if (!usedPreAuth && m_PreAuthenticate)
				DoPreAuthenticate ();

			return _HttpRequestHeaders.ToString ();
		}

		void DoPreAuthenticate ()
		{
			bool isProxy = (_Proxy != null && !_Proxy.IsBypassed (_Uri));
			ICredentials creds = (!isProxy || _AuthInfo != null) ? _AuthInfo : _Proxy.Credentials;
			Authorization auth = AuthenticationManager.PreAuthenticate (this, creds);
			if (auth == null)
				return;

			_HttpRequestHeaders.RemoveInternal ("Proxy-Authorization");
			_HttpRequestHeaders.RemoveInternal ("Authorization");
			string authHeader = (isProxy && _AuthInfo == null) ? "Proxy-Authorization" : "Authorization";
			_HttpRequestHeaders [authHeader] = auth.Message;
			usedPreAuth = true;
		}
		
		internal void SetWriteStreamError (WebExceptionStatus status, Exception exc)
		{
			if (Aborted)
				return;

			WebAsyncResult r = asyncWrite;
			if (r == null)
				r = asyncRead;

			if (r != null) {
				string msg;
				WebException wex;
				if (exc == null) {
					msg = "Error: " + status;
					wex = new WebException (msg, status);
				} else {
					msg = String.Format ("Error: {0} ({1})", status, exc.Message);
					wex = new WebException (msg, exc, status, null);
				}
				r.SetCompleted (false, wex);
				r.DoCallback ();
			}
		}

		internal byte[] GetRequestHeaders ()
		{
			StringBuilder req = new StringBuilder ();
			string query;
			if (!ProxyQuery) {
				query = _Uri.PathAndQuery;
			} else {
				query = String.Format ("{0}://{1}{2}",  _Uri.Scheme,
									Host,
									_Uri.PathAndQuery);
			}
			
			if (!force_version && servicePoint.ProtocolVersion != null && servicePoint.ProtocolVersion < version) {
				actualVersion = servicePoint.ProtocolVersion;
			} else {
				actualVersion = version;
			}

			req.AppendFormat ("{0} {1} HTTP/{2}.{3}\r\n", method, query,
								actualVersion.Major, actualVersion.Minor);
			req.Append (GetHeaders ());
			string reqstr = req.ToString ();
			return Encoding.UTF8.GetBytes (reqstr);
		}

		internal void SetWriteStream (WebConnectionStream stream)
		{
			if (Aborted)
				return;
			
			writeStream = stream;
			if (bodyBuffer != null) {
				_HttpRequestHeaders.RemoveInternal ("Transfer-Encoding");
				_ContentLength = bodyBufferLength;
				writeStream.SendChunked = false;
			}

			writeStream.SetHeadersAsync (false, result => {
				if (result.GotException) {
					SetWriteStreamError (result.Exception);
					return;
				}

				haveRequest = true;

				SetWriteStreamInner (inner => {
					if (inner.GotException) {
						SetWriteStreamError (inner.Exception);
						return;
					}

					if (asyncWrite != null) {
						asyncWrite.SetCompleted (inner.CompletedSynchronously, writeStream);
						asyncWrite.DoCallback ();
						asyncWrite = null;
					}
				});
			});
		}

		void SetWriteStreamInner (SimpleAsyncCallback callback)
		{
			SimpleAsyncResult.Run (result => {
				if (bodyBuffer != null) {
					// The body has been written and buffered. The request "user"
					// won't write it again, so we must do it.
					if (auth_state.NtlmAuthState != NtlmAuthState.Challenge && proxy_auth_state.NtlmAuthState != NtlmAuthState.Challenge) {
						// FIXME: this is a blocking call on the thread pool that could lead to thread pool exhaustion
						writeStream.Write (bodyBuffer, 0, bodyBufferLength);
						bodyBuffer = null;
						writeStream.Close ();
					}
				} else if (MethodWithBuffer) {
					if (getResponseCalled && !writeStream.RequestWritten)
						return writeStream.WriteRequestAsync (result);
				}

				return false;
			}, callback);
		}

		void SetWriteStreamError (Exception exc)
		{
			WebException wexc = exc as WebException;
			if (wexc != null)
				SetWriteStreamError (wexc.Status, wexc);
			else
				SetWriteStreamError (WebExceptionStatus.SendFailure, exc);
		}

		internal void SetResponseError (WebExceptionStatus status, Exception e, string where)
		{
			if (Aborted)
				return;
			lock (locker) {
			string msg = String.Format ("Error getting response stream ({0}): {1}", where, status);
			WebAsyncResult r = asyncRead;
			if (r == null)
				r = asyncWrite;

			WebException wexc;
			if (e is WebException) {
				wexc = (WebException) e;
			} else {
				wexc = new WebException (msg, e, status, null); 
			}
			if (r != null) {
				if (!r.IsCompleted) {
					r.SetCompleted (false, wexc);
					r.DoCallback ();
				} else if (r == asyncWrite) {
					saved_exc = wexc;
				}
				haveResponse = true;
				asyncRead = null;
				asyncWrite = null;
			} else {
				haveResponse = true;
				saved_exc = wexc;
			}
			}
		}

		void CheckSendError (WebConnectionData data)
		{
			// Got here, but no one called GetResponse
			int status = data.StatusCode;
			if (status < 400 || status == 401 || status == 407)
				return;

			if (writeStream != null && asyncRead == null && !writeStream.CompleteRequestWritten) {
				// The request has not been completely sent and we got here!
				// We should probably just close and cause an error in any case,
				saved_exc = new WebException (data.StatusDescription, null, WebExceptionStatus.ProtocolError, webResponse); 
				if (AllowWriteStreamBuffering || sendChunked || writeStream.totalWritten >= _ContentLength) {
					webResponse.ReadAll ();
				} else {
					writeStream.IgnoreIOErrors = true;
				}
			}
		}

		bool HandleNtlmAuth (WebAsyncResult r)
		{
			bool isProxy = webResponse.StatusCode == HttpStatusCode.ProxyAuthenticationRequired;
			if ((isProxy ? proxy_auth_state.NtlmAuthState : auth_state.NtlmAuthState) == NtlmAuthState.None)
				return false;

			WebConnectionStream wce = webResponse.GetResponseStream () as WebConnectionStream;
			if (wce != null) {
				WebConnection cnc = wce.Connection;
				cnc.PriorityRequest = this;
				ICredentials creds = !isProxy ? _AuthInfo : _Proxy.Credentials;
				if (creds != null) {
					cnc.NtlmCredential = creds.GetCredential (_OriginUri, "NTLM");
					cnc.UnsafeAuthenticatedConnectionSharing = UnsafeAuthenticatedConnectionSharing;
				}
			}
			r.Reset ();
			finished_reading = false;
			haveResponse = false;
			webResponse.ReadAll ();
			webResponse = null;
			return true;
		}

		internal void SetResponseData (WebConnectionData data)
		{
			lock (locker) {
			if (Aborted) {
				if (data.stream != null)
					data.stream.Close ();
				return;
			}

			WebException wexc = null;
			try {
				webResponse = new HttpWebResponse (_Uri, method, data, _CookieContainer);
			} catch (Exception e) {
				wexc = new WebException (e.Message, e, WebExceptionStatus.ProtocolError, null); 
				if (data.stream != null)
					data.stream.Close ();
			}

			if (wexc == null && (method == "POST" || method == "PUT")) {
				CheckSendError (data);
				if (saved_exc != null)
					wexc = (WebException) saved_exc;
			}

			WebAsyncResult r = asyncRead;

			bool forced = false;
			if (r == null && webResponse != null) {
				// This is a forced completion (302, 204)...
				forced = true;
				r = new WebAsyncResult (null, null);
				r.SetCompleted (false, webResponse);
			}

			if (r != null) {
				if (wexc != null) {
					haveResponse = true;
					if (!r.IsCompleted)
						r.SetCompleted (false, wexc);
					r.DoCallback ();
					return;
				}

				bool isProxy = ProxyQuery && !_Proxy.IsBypassed (_Uri);

				bool redirected;
				try {
					redirected = CheckFinalStatus (r);
					if (!redirected) {
						if ((isProxy ? proxy_auth_state.IsNtlmAuthenticated : auth_state.IsNtlmAuthenticated) &&
								webResponse != null && (int)webResponse.StatusCode < 400) {
							WebConnectionStream wce = webResponse.GetResponseStream () as WebConnectionStream;
							if (wce != null) {
								WebConnection cnc = wce.Connection;
								cnc.NtlmAuthenticated = true;
							}
						}

						// clear internal buffer so that it does not
						// hold possible big buffer (bug #397627)
						if (writeStream != null)
							writeStream.KillBuffer ();

						haveResponse = true;
						r.SetCompleted (false, webResponse);
						r.DoCallback ();
					} else {
						if (sendChunked) {
							sendChunked = false;
							_HttpRequestHeaders.RemoveInternal ("Transfer-Encoding");
						}

						if (webResponse != null) {
							if (HandleNtlmAuth (r))
								return;
							webResponse.Close ();
						}
						finished_reading = false;
						haveResponse = false;
						webResponse = null;
						r.Reset ();
						servicePoint = GetServicePoint ();
						abortHandler = servicePoint.SendRequest (this, _ConnectionGroupName);
					}
				} catch (WebException wexc2) {
					if (forced) {
						saved_exc = wexc2;
						haveResponse = true;
					}
					r.SetCompleted (false, wexc2);
					r.DoCallback ();
					return;
				} catch (Exception ex) {
					wexc = new WebException (ex.Message, ex, WebExceptionStatus.ProtocolError, null); 
					if (forced) {
						saved_exc = wexc;
						haveResponse = true;
					}
					r.SetCompleted (false, wexc);
					r.DoCallback ();
					return;
				}
			}
			}
		}

		struct AuthorizationState
		{
			readonly HttpWebRequest request;
			readonly bool isProxy;
			bool isCompleted;
			NtlmAuthState ntlm_auth_state;

			public bool IsCompleted {
				get { return isCompleted; }
			}

			public NtlmAuthState NtlmAuthState {
				get { return ntlm_auth_state; }
			}

			public bool IsNtlmAuthenticated {
				get { return isCompleted && ntlm_auth_state != NtlmAuthState.None; }
			}

			public AuthorizationState (HttpWebRequest request, bool isProxy)
			{
				this.request = request;
				this.isProxy = isProxy;
				isCompleted = false;
				ntlm_auth_state = NtlmAuthState.None;
			}

			public bool CheckAuthorization (WebResponse response, HttpStatusCode code)
			{
				isCompleted = false;
				if (code == HttpStatusCode.Unauthorized && request._AuthInfo == null)
					return false;

				// FIXME: This should never happen!
				if (isProxy != (code == HttpStatusCode.ProxyAuthenticationRequired))
					return false;

				if (isProxy && (request._Proxy == null || request._Proxy.Credentials == null))
					return false;

				string [] authHeaders = response.Headers.GetValues_internal (isProxy ? "Proxy-Authenticate" : "WWW-Authenticate");
				if (authHeaders == null || authHeaders.Length == 0)
					return false;

				ICredentials creds = (!isProxy) ? request._AuthInfo : request._Proxy.Credentials;
				Authorization auth = null;
				foreach (string authHeader in authHeaders) {
					auth = AuthenticationManager.Authenticate (authHeader, request, creds);
					if (auth != null)
						break;
				}
				if (auth == null)
					return false;
				request._HttpRequestHeaders [isProxy ? "Proxy-Authorization" : "Authorization"] = auth.Message;
				isCompleted = auth.Complete;
				bool is_ntlm = (auth.Module.AuthenticationType == "NTLM");
				if (is_ntlm)
					ntlm_auth_state = (NtlmAuthState)((int) ntlm_auth_state + 1);
				return true;
			}

			public void Reset ()
			{
				isCompleted = false;
				ntlm_auth_state = NtlmAuthState.None;
				request._HttpRequestHeaders.RemoveInternal (isProxy ? "Proxy-Authorization" : "Authorization");
			}

			public override string ToString ()
			{
				return string.Format ("{0}AuthState [{1}:{2}]", isProxy ? "Proxy" : "", isCompleted, ntlm_auth_state);
			}
		}

		bool CheckAuthorization (WebResponse response, HttpStatusCode code)
		{
			bool isProxy = code == HttpStatusCode.ProxyAuthenticationRequired;
			return isProxy ? proxy_auth_state.CheckAuthorization (response, code) : auth_state.CheckAuthorization (response, code);
		}

		// Returns true if redirected
		bool CheckFinalStatus (WebAsyncResult result)
		{
			if (result.GotException) {
				bodyBuffer = null;
				throw result.Exception;
			}

			Exception throwMe = result.Exception;

			HttpWebResponse resp = result.Response;
			WebExceptionStatus protoError = WebExceptionStatus.ProtocolError;
			HttpStatusCode code = 0;
			if (throwMe == null && webResponse != null) {
				code = webResponse.StatusCode;
				if ((!auth_state.IsCompleted && code == HttpStatusCode.Unauthorized && _AuthInfo != null) ||
					(ProxyQuery && !proxy_auth_state.IsCompleted && code == HttpStatusCode.ProxyAuthenticationRequired)) {
					if (!usedPreAuth && CheckAuthorization (webResponse, code)) {
						// Keep the written body, so it can be rewritten in the retry
						if (MethodWithBuffer) {
							if (AllowWriteStreamBuffering) {
								if (writeStream.WriteBufferLength > 0) {
									bodyBuffer = writeStream.WriteBuffer;
									bodyBufferLength = writeStream.WriteBufferLength;
								}

								return true;
							}

							//
							// Buffering is not allowed but we have alternative way to get same content (we
							// need to resent it due to NTLM Authentication).
					 		//
							if (ResendContentFactory != null) {
								using (var ms = new MemoryStream ()) {
									ResendContentFactory (ms);
									bodyBuffer = ms.ToArray ();
									bodyBufferLength = bodyBuffer.Length;
								}
								return true;
							}
						} else if (method != "PUT" && method != "POST") {
							bodyBuffer = null;
							return true;
						}

						if (!ThrowOnError)
							return false;
							
						writeStream.InternalClose ();
						writeStream = null;
						webResponse.Close ();
						webResponse = null;
						bodyBuffer = null;
							
						throw new WebException ("This request requires buffering " +
									"of data for authentication or " +
									"redirection to be sucessful.");
					}
				}

				bodyBuffer = null;
				if ((int) code >= 400) {
					string err = String.Format ("The remote server returned an error: ({0}) {1}.",
								    (int) code, webResponse.StatusDescription);
					throwMe = new WebException (err, null, protoError, webResponse);
					webResponse.ReadAll ();
				} else if ((int) code == 304 && AllowAutoRedirect) {
					string err = String.Format ("The remote server returned an error: ({0}) {1}.",
								    (int) code, webResponse.StatusDescription);
					throwMe = new WebException (err, null, protoError, webResponse);
				} else if ((int) code >= 300 && AllowAutoRedirect && redirects >= _MaximumAllowedRedirections) {
					throwMe = new WebException ("Max. redirections exceeded.", null,
								    protoError, webResponse);
					webResponse.ReadAll ();
				}
			}

			bodyBuffer = null;
			if (throwMe == null) {
				bool b = false;
				int c = (int) code;
				if (AllowAutoRedirect && c >= 300) {
					b = Redirect (result, code, webResponse);
					if (InternalAllowBuffering && writeStream.WriteBufferLength > 0) {
						bodyBuffer = writeStream.WriteBuffer;
						bodyBufferLength = writeStream.WriteBufferLength;
					}
					if (b && !UnsafeAuthenticatedConnectionSharing) {
						auth_state.Reset ();
						proxy_auth_state.Reset ();
					}
				}

				if (resp != null && c >= 300 && c != 304)
					resp.ReadAll ();

				return b;
			}
				
			if (!ThrowOnError)
				return false;

			if (writeStream != null) {
				writeStream.InternalClose ();
				writeStream = null;
			}

			webResponse = null;

			throw throwMe;
		}

		internal bool ReuseConnection {
			get;
			set;
		}

		internal WebConnection StoredConnection;
	}
}

