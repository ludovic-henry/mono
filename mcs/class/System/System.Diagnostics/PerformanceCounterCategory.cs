//
// System.Diagnostics.PerformanceCounterCategory.cs
//
// Authors:
//   Jonathan Pryor (jonpryor@vt.edu)
//
// (C) 2002
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

using System.Security.Permissions;
using System.Runtime.CompilerServices;
using System.Text;

namespace System.Diagnostics 
{
	[PermissionSet (SecurityAction.LinkDemand, Unrestricted = true)]
	public sealed class PerformanceCounterCategory 
	{
		private string categoryName;
		private string machineName;
		private PerformanceCounterCategoryType type = PerformanceCounterCategoryType.Unknown;

		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		static unsafe extern bool CategoryDelete (byte* name);

		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		static unsafe extern string CategoryHelpInternal (byte* category, byte* machine);

		/* this icall allows a null counter and it will just search for the category */
		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		static unsafe extern bool CounterCategoryExists (byte* counter, byte* category, byte* machine);

		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		static unsafe extern bool Create (byte* categoryName, byte* categoryHelp,
			PerformanceCounterCategoryType categoryType, CounterCreationData[] items);

		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		static unsafe extern bool InstanceExistsInternal (byte* instance, byte* category, byte* machine);

		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		static unsafe extern string[] GetCategoryNames (byte* machine);

		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		static unsafe extern string[] GetCounterNames (byte* category, byte* machine);

		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		static unsafe extern string[] GetInstanceNames (byte* category, byte* machine);

		static void CheckCategory (string categoryName) {
			if (categoryName == null)
				throw new ArgumentNullException ("categoryName");
			if (categoryName == "")
				throw new ArgumentException ("categoryName");
		}

		public PerformanceCounterCategory ()
			: this ("", ".")
		{
		}

		// may throw ArgumentException (""), ArgumentNullException
		public PerformanceCounterCategory (string categoryName)
			: this (categoryName, ".")
		{
		}

		// may throw ArgumentException (""), ArgumentNullException
		public PerformanceCounterCategory (string categoryName, string machineName)
		{
			CheckCategory (categoryName);
			if (machineName == null)
				throw new ArgumentNullException ("machineName");
			// TODO checks and whatever else is needed
			this.categoryName = categoryName;
			this.machineName = machineName;
		}

		// may throw InvalidOperationException, Win32Exception
		public string CategoryHelp {
			get {
				unsafe {
					fixed (byte* categoryName_b = Encoding.UTF8.GetBytes (categoryName + '\0'))
					fixed (byte* machineName_b = Encoding.UTF8.GetBytes (machineName + '\0')) {
						string res = CategoryHelpInternal (categoryName_b, machineName_b);
						if (res == null)
							throw new InvalidOperationException ();
						return res;
					}
				}
			}
		}

		// may throw ArgumentException (""), ArgumentNullException
		public string CategoryName {
			get {return categoryName;}
			set {
				if (value == null)
					throw new ArgumentNullException ("value");
				if (value == "")
					throw new ArgumentException ("value");
				categoryName = value;
			}
		}

		// may throw ArgumentException
		public string MachineName {
			get {return machineName;}
			set {
				if (value == null)
					throw new ArgumentNullException ("value");
				if (value == "")
					throw new ArgumentException ("value");
				machineName = value;
			}
		}

		public PerformanceCounterCategoryType CategoryType {
			get {
				return type;
			}
		}

		public bool CounterExists (string counterName)
		{
			return CounterExists (counterName, categoryName, machineName);
		}

		public static bool CounterExists (string counterName, string categoryName)
		{
			return CounterExists (counterName, categoryName, ".");
		}

		// may throw ArgumentNullException, InvalidOperationException
		// (categoryName is "", machine name is bad), Win32Exception
		public static bool CounterExists (string counterName, string categoryName, string machineName)
		{
			if (counterName == null)
				throw new ArgumentNullException ("counterName");
			CheckCategory (categoryName);
			if (machineName == null)
				throw new ArgumentNullException ("machineName");
			unsafe {
				fixed (byte* counterName_b = Encoding.UTF8.GetBytes (counterName + '\0'))
				fixed (byte* categoryName_b = Encoding.UTF8.GetBytes (categoryName + '\0'))
				fixed (byte* machineName_b = Encoding.UTF8.GetBytes (machineName + '\0')) {
					return CounterCategoryExists (counterName_b, categoryName_b, machineName_b);
				}
			}
		}

		[Obsolete ("Use another overload that uses PerformanceCounterCategoryType instead")]
		public static PerformanceCounterCategory Create (
			string categoryName,
			string categoryHelp,
			CounterCreationDataCollection counterData)
		{
			return Create (categoryName, categoryHelp,
				PerformanceCounterCategoryType.Unknown, counterData);
		}

		[Obsolete ("Use another overload that uses PerformanceCounterCategoryType instead")]
		public static PerformanceCounterCategory Create (
			string categoryName,
			string categoryHelp,
			string counterName,
			string counterHelp)
		{
			return Create (categoryName, categoryHelp,
				PerformanceCounterCategoryType.Unknown, counterName, counterHelp);
		}

		public static PerformanceCounterCategory Create (
			string categoryName,
			string categoryHelp,
			PerformanceCounterCategoryType categoryType,
			CounterCreationDataCollection counterData)
		{
			CheckCategory (categoryName);
			if (counterData == null)
				throw new ArgumentNullException ("counterData");
			if (counterData.Count == 0)
				throw new ArgumentException ("counterData");
			CounterCreationData[] items = new CounterCreationData [counterData.Count];
			counterData.CopyTo (items, 0);
			unsafe {
				fixed (byte* categoryName_b = Encoding.UTF8.GetBytes (categoryName + '\0'))
				fixed (byte* categoryHelp_b = Encoding.UTF8.GetBytes (categoryHelp + '\0')) {
					if (!Create (categoryName_b, categoryHelp_b, categoryType, items))
						throw new InvalidOperationException ();
				}
			}
			return new PerformanceCounterCategory (categoryName, categoryHelp);
		}

		public static PerformanceCounterCategory Create (
			string categoryName,
			string categoryHelp,
			PerformanceCounterCategoryType categoryType,
			string counterName,
			string counterHelp)
		{
			CheckCategory (categoryName);
			CounterCreationData[] items = new CounterCreationData [1];
			// we use PerformanceCounterType.NumberOfItems32 as the default type
			items [0] = new CounterCreationData (counterName, counterHelp, PerformanceCounterType.NumberOfItems32);
			unsafe {
				fixed (byte* categoryName_b = Encoding.UTF8.GetBytes (categoryName + '\0'))
				fixed (byte* categoryHelp_b = Encoding.UTF8.GetBytes (categoryHelp + '\0')) {
					if (!Create (categoryName_b, categoryHelp_b, categoryType, items))
						throw new InvalidOperationException ();
				}
			}
			return new PerformanceCounterCategory (categoryName, categoryHelp);
		}

		public static void Delete (string categoryName)
		{
			CheckCategory (categoryName);
			unsafe {
				fixed (byte* categoryName_b = Encoding.UTF8.GetBytes (categoryName + '\0')) {
					if (!CategoryDelete (categoryName_b))
						throw new InvalidOperationException ();
				}
			}
		}

		public static bool Exists (string categoryName)
		{
			return Exists (categoryName, ".");
		}

		public static bool Exists (string categoryName, string machineName)
		{
			CheckCategory (categoryName);
			unsafe {
				fixed (byte* categoryName_b = Encoding.UTF8.GetBytes (categoryName + '\0'))
				fixed (byte* machineName_b = Encoding.UTF8.GetBytes (machineName + '\0')) {
					return CounterCategoryExists ((byte*) IntPtr.Zero, categoryName_b, machineName_b);
				}
			}
		}

		public static PerformanceCounterCategory[] GetCategories ()
		{
			return GetCategories (".");
		}

		public static PerformanceCounterCategory[] GetCategories (string machineName)
		{
			if (machineName == null)
				throw new ArgumentNullException ("machineName");
			string[] catnames;
			unsafe {
				fixed (byte* machineName_b = Encoding.UTF8.GetBytes (machineName + '\0')) {
					catnames = GetCategoryNames (machineName_b);
				}
			}
			PerformanceCounterCategory[] cats = new PerformanceCounterCategory [catnames.Length];
			for (int i = 0; i < catnames.Length; ++i)
				cats [i] = new PerformanceCounterCategory (catnames [i], machineName);
			return cats;
		}

		public PerformanceCounter[] GetCounters ()
		{
			return GetCounters ("");
		}

		public PerformanceCounter[] GetCounters (string instanceName)
		{
			string[] countnames;
			unsafe {
				fixed (byte* categoryName_b = Encoding.UTF8.GetBytes (categoryName + '\0'))
				fixed (byte* machineName_b = Encoding.UTF8.GetBytes (machineName + '\0')) {
					countnames = GetCounterNames (categoryName_b, machineName_b);
				}
			}
			PerformanceCounter[] counters = new PerformanceCounter [countnames.Length];
			for (int i = 0; i < countnames.Length; ++i) {
				counters [i] = new PerformanceCounter (categoryName, countnames [i], instanceName, machineName);
			}
			return counters;
		}

		public string[] GetInstanceNames ()
		{
			unsafe {
				fixed (byte* categoryName_b = Encoding.UTF8.GetBytes (categoryName + '\0'))
				fixed (byte* machineName_b = Encoding.UTF8.GetBytes (machineName + '\0')) {
					return GetInstanceNames (categoryName_b, machineName_b);
				}
			}
		}

		public bool InstanceExists (string instanceName)
		{
			return InstanceExists (instanceName, categoryName, machineName);
		}

		public static bool InstanceExists (string instanceName, string categoryName)
		{
			return InstanceExists (instanceName, categoryName, ".");
		}

		public static bool InstanceExists (string instanceName, string categoryName, string machineName)
		{
			if (instanceName == null)
				throw new ArgumentNullException ("instanceName");
			CheckCategory (categoryName);
			if (machineName == null)
				throw new ArgumentNullException ("machineName");
			unsafe {
				fixed (byte* instanceName_b = Encoding.UTF8.GetBytes (instanceName + '\0'))
				fixed (byte* categoryName_b = Encoding.UTF8.GetBytes (categoryName + '\0'))
				fixed (byte* machineName_b = Encoding.UTF8.GetBytes (machineName + '\0')) {
					return InstanceExistsInternal (instanceName_b, categoryName_b, machineName_b);
				}
			}
		}

		[MonoTODO]
		public InstanceDataCollectionCollection ReadCategory ()
		{
			throw new NotImplementedException ();
		}
	}
}

