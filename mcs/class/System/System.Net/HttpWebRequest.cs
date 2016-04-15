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
	public partial class HttpWebRequest : WebRequest, ISerializable
	{
		internal HttpWebRequest (Uri uri) 
		{
			throw new NotSupportedException ();
		}

#if SECURITY_DEP
		internal HttpWebRequest (Uri uri, IMonoTlsProvider tlsProvider, MonoTlsSettings settings = null)
			: this (uri)
		{
			throw new NotSupportedException ();
		}
#endif

		internal bool InternalAllowBuffering
		{
			get { throw new NotSupportedException (); }
		}

		internal IMonoTlsProvider TlsProvider
		{
			get { throw new NotSupportedException (); }
		}

#if SECURITY_DEP
		internal MonoTlsSettings TlsSettings
		{
			get { throw new NotSupportedException (); }
		}
#endif

		internal long InternalContentLength
		{
			set { throw new NotSupportedException (); }
		}

		internal bool ThrowOnError
		{
			get { throw new NotSupportedException (); }
			set { throw new NotSupportedException (); }
		}

		internal ServicePoint ServicePointNoLock
		{
			get { throw new NotSupportedException (); }
		}

		internal bool GotRequestStream
		{
			get { throw new NotSupportedException (); }
		}

		internal Uri AuthUri
		{
			get { throw new NotSupportedException (); }
		}
		
		internal bool ProxyQuery
		{
			get { throw new NotSupportedException (); }
		}

		internal ServicePoint GetServicePoint ()
		{
			throw new NotSupportedException ();
		}
		
		internal bool FinishedReading
		{
			get { throw new NotSupportedException (); }
			set { throw new NotSupportedException (); }
		}

		internal void DoContinueDelegate (int statusCode, WebHeaderCollection headers)
		{
			throw new NotSupportedException ();
		}

		internal void SetWriteStreamError (WebExceptionStatus status, Exception exc)
		{
			throw new NotSupportedException ();
		}

		internal byte[] GetRequestHeaders ()
		{
			throw new NotSupportedException ();
		}

		internal void SetWriteStream (WebConnectionStream stream)
		{
			throw new NotSupportedException ();
		}

		internal void SetResponseError (WebExceptionStatus status, Exception e, string where)
		{
			throw new NotSupportedException ();
		}

		internal void SetResponseData (WebHeaderCollection headers, Version version, int statusCode, string statusDescription, Stream stream, HttpWebRequest request)
		{
			throw new NotSupportedException ();
		}

		internal bool ReuseConnection {
			get { throw new NotSupportedException (); }
			set { throw new NotSupportedException (); }
		}

		internal WebConnection StoredConnection
		{
			get { throw new NotSupportedException (); }
			set { throw new NotSupportedException (); }
		}

		public Uri Address {
			get { return _Uri; }
			internal set { _Uri = value; }
		}
	}
}

