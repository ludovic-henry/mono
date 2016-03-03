
using System;
using System.Collections;

namespace System.Net.Configuration
{
	internal sealed class ConnectionManagementSectionInternal
	{
		internal Hashtable ConnectionManagement { get; private set; }

		internal static object ClassSyncObject { get; } = new object ();

		static internal ConnectionManagementSectionInternal GetSection ()
		{
			lock (ClassSyncObject)
			{
				return new ConnectionManagementSectionInternal () {
					ConnectionManagement = null,
				};
			}
		}
	}
}
