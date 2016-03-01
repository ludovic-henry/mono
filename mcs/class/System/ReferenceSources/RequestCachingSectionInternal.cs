
using System.Collections.Specialized;
using System.IO;
using System.Net.Cache;

namespace System.Net.Configuration
{
	internal sealed class RequestCachingSectionInternal
	{
		private RequestCachingSectionInternal () {}

		internal bool DisableAllCaching { get; private set; }
		internal bool IsPrivateCache { get; private set; }
		internal RequestCache DefaultCache { get; private set; }
		internal RequestCachePolicy DefaultCachePolicy { get; private set; }
		internal TimeSpan UnspecifiedMaximumAge { get; private set; }
		internal HttpRequestCachePolicy DefaultHttpCachePolicy { get; private set; }
		internal RequestCachePolicy DefaultFtpCachePolicy { get; private set; }
		internal HttpRequestCacheValidator DefaultHttpValidator { get; private set; }
		internal FtpRequestCacheValidator DefaultFtpValidator { get; private set; }

		internal static object ClassSyncObject { get; } = new object ();

		static internal RequestCachingSectionInternal GetSection()
		{
			lock (ClassSyncObject)
			{
				return new RequestCachingSectionInternal () {
					DisableAllCaching = false,
					IsPrivateCache = true,
					DefaultCache = new System.Net.Cache.VoidRequestCache (true, true),
					DefaultCachePolicy = new RequestCachePolicy (RequestCacheLevel.BypassCache),
					UnspecifiedMaximumAge = TimeSpan.FromDays (1),
					DefaultHttpCachePolicy = null,
					DefaultFtpCachePolicy = null,
					DefaultHttpValidator = new HttpRequestCacheValidator (false, TimeSpan.FromDays (1)),
					DefaultFtpValidator = new FtpRequestCacheValidator(false, TimeSpan.FromDays (1)),
				};
			}
		}

	}
}

namespace System.Net.Cache
{
	sealed class VoidRequestCache : RequestCache
	{
		internal VoidRequestCache (bool isPrivateCache, bool canWrite)
			: base (isPrivateCache, canWrite)
		{
		}

		internal override Stream Retrieve (string key, out RequestCacheEntry cacheEntry)
		{
			throw new IOException ();
		}

		internal override Stream Store (string key, long contentLength, DateTime expiresUtc, DateTime lastModifiedUtc, TimeSpan maxStale, StringCollection entryMetadata, StringCollection systemMetadata)
		{
			throw new IOException ();
		}

		internal override void Remove (string key)
		{
			throw new IOException ();
		}

		internal override void Update (string key, DateTime expiresUtc, DateTime lastModifiedUtc, DateTime lastSynchronizedUtc, TimeSpan maxStale, StringCollection entryMetadata, StringCollection systemMetadata)
		{
			throw new IOException ();
		}

		internal override bool TryRetrieve (string key, out RequestCacheEntry cacheEntry, out Stream readStream)
		{
			cacheEntry = null;
			readStream = null;
			return false;
		}

		internal override bool TryStore (string key, long contentLength, DateTime expiresUtc, DateTime lastModifiedUtc, TimeSpan maxStale, StringCollection entryMetadata, StringCollection systemMetadata, out Stream writeStream)
		{
			writeStream = null;
			return false;
		}


		internal override bool TryRemove (string key)
		{
			return false;
		}

		internal override bool TryUpdate (string key, DateTime expiresUtc, DateTime lastModifiedUtc, DateTime lastSynchronizedUtc, TimeSpan maxStale, StringCollection entryMetadata, StringCollection systemMetadata)
		{
			return false;
		}

		internal override void UnlockEntry (Stream retrieveStream)
		{
		}
	}
}