
using System;
using System.Globalization;
using System.Runtime.CompilerServices;
using System.Runtime.ConstrainedExecution;
using System.Runtime.Versioning;
using System.Runtime.Serialization;
using System.Diagnostics.Contracts;

namespace System
{
	public partial struct Decimal : IFormattable, IComparable, IConvertible, IDeserializationCallback, IComparable<Decimal>, IEquatable<Decimal>
	{
		public static long ToOACurrency (decimal value)
		{
			return (long) (value * 10000);
		}

		public static decimal FromOACurrency (long cy)
		{
			return (decimal)cy / (decimal)10000;
		}

		[OnSerializing]
		void OnSerializing(StreamingContext ctx) {
			// OnSerializing is called before serialization of an object
			try {
				SetBits( GetBits(this) );
			} catch (ArgumentException e) {
				throw new SerializationException(Environment.GetResourceString("Overflow_Decimal"), e); 
			} 
		}

		void IDeserializationCallback.OnDeserialization(Object sender) {
			// OnDeserialization is called after each instance of this class is deserialized.
			// This callback method performs decimal validation after being deserialized.
			try {
				SetBits( GetBits(this) );
			} catch (ArgumentException e) {
				throw new SerializationException(Environment.GetResourceString("Overflow_Decimal"), e); 
			} 
		}

		internal static void GetBytes(Decimal d, byte [] buffer) {
			Contract.Requires((buffer != null && buffer.Length >= 16), "[GetBytes]buffer != null && buffer.Length >= 16");
			buffer[0] = (byte) d._lo;
			buffer[1] = (byte) (d._lo >> 8);
			buffer[2] = (byte) (d._lo >> 16);
			buffer[3] = (byte) (d._lo >> 24);
			
			buffer[4] = (byte) d._mid;
			buffer[5] = (byte) (d._mid >> 8);
			buffer[6] = (byte) (d._mid >> 16);
			buffer[7] = (byte) (d._mid >> 24);

			buffer[8] = (byte) d._hi;
			buffer[9] = (byte) (d._hi >> 8);
			buffer[10] = (byte) (d._hi >> 16);
			buffer[11] = (byte) (d._hi >> 24);
			
			buffer[12] = (byte) d._flags;
			buffer[13] = (byte) (d._flags >> 8);
			buffer[14] = (byte) (d._flags >> 16);
			buffer[15] = (byte) (d._flags >> 24);
		}

		internal static decimal ToDecimal(byte [] buffer) {
			Contract.Requires((buffer != null && buffer.Length >= 16), "[ToDecimal]buffer != null && buffer.Length >= 16");
			int lo = ((int)buffer[0]) | ((int)buffer[1] << 8) | ((int)buffer[2] << 16) | ((int)buffer[3] << 24);
			int mid = ((int)buffer[4]) | ((int)buffer[5] << 8) | ((int)buffer[6] << 16) | ((int)buffer[7] << 24);
			int hi = ((int)buffer[8]) | ((int)buffer[9] << 8) | ((int)buffer[10] << 16) | ((int)buffer[11] << 24);
			int flags = ((int)buffer[12]) | ((int)buffer[13] << 8) | ((int)buffer[14] << 16) | ((int)buffer[15] << 24);
			return new Decimal(lo,mid,hi,flags);
		}

		public static Decimal Round(Decimal d) {
			return Round(d, 0);
		}

		public static Decimal Round(Decimal d, MidpointRounding mode) {
			return Round(d, 0, mode);
		}
	}
}
