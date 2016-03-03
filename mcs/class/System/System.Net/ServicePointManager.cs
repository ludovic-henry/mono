//
// System.Net.ServicePointManager
//
// Authors:
//   Lawrence Pit (loz@cable.a2000.nl)
//   Gonzalo Paniagua Javier (gonzalo@novell.com)
//
// Copyright (c) 2003-2010 Novell, Inc (http://www.novell.com)
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

using System;
using System.Threading;
using System.Collections;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Configuration;
using System.Net.Configuration;
using System.Security.Cryptography.X509Certificates;

using System.Globalization;
using System.Net.Security;
using System.Diagnostics;

//
// notes:
// A service point manager manages service points (duh!).
// A service point maintains a list of connections (per scheme + authority).
// According to HttpWebRequest.ConnectionGroupName each connection group
// creates additional connections. therefor, a service point has a hashtable
// of connection groups where each value is a list of connections.
// 
// when we need to make an HttpWebRequest, we need to do the following:
// 1. find service point, given Uri and Proxy 
// 2. find connection group, given service point and group name
// 3. find free connection in connection group, or create one (if ok due to limits)
// 4. lease connection
// 5. execute request
// 6. when finished, return connection
//


namespace System.Net 
{
	public partial class ServicePointManager
	{
#if MONOTOUCH
		public const int DefaultPersistentConnectionLimit = 10;
#else
		public const int DefaultPersistentConnectionLimit = 2;
#endif

		private static void EnsureReusePortSettingsInitialized ()
		{
			if (reusePortSettingsInitialized) {
				return;
			}

			lock (reusePortLock) {
				if (reusePortSettingsInitialized) {
					return;
				}

				reusePort = true;
				reusePortSettingsInitialized = true;
			}
		}

		private static void EnsureStrongCryptoSettingsInitialized ()
		{
			if (disableStrongCryptoInitialized) {
				return;
			}

			lock (disableStrongCryptoLock) {
				if (disableStrongCryptoInitialized) {
					return;
				}

				// FIXME: switch per platform? use a configuration variable?
				bool disableStrongCryptoInternal = false;

				if (disableStrongCryptoInternal) {
					// Revert the SecurityProtocol selection to the legacy combination.
					s_SecurityProtocolType = SecurityProtocolType.Tls | SecurityProtocolType.Ssl3;
				} else {
					s_SecurityProtocolType = SecurityProtocolType.Tls12 | SecurityProtocolType.Tls11 | SecurityProtocolType.Tls;
				}

				disableStrongCrypto = disableStrongCryptoInternal;
				disableStrongCryptoInitialized = true;
			}
		}
	}
}

