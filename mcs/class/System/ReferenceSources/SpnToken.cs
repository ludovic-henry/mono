
namespace System.Net
{
	internal class SpnToken
	{
		internal bool IsTrusted
		{
			get { throw new NotSupportedException (); }
			set { throw new NotSupportedException (); }
		}

		internal string Spn
		{
			get { throw new NotSupportedException (); }
		}

		internal SpnToken (string spn)
			: this(spn, true)
		{
			throw new NotSupportedException ();
		}

		internal SpnToken (string spn, bool trusted)
		{
			throw new NotSupportedException ();
		}
	}
}