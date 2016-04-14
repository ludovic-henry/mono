
using System.Threading;
using System.Net.Sockets;
using System.Security.Cryptography.X509Certificates;
using System.Security.Authentication.ExtendedProtection;

namespace System.Net
{
	internal class TlsStream : NetworkStream, IDisposable
	{
		public TlsStream(string destinationHost, NetworkStream networkStream, X509CertificateCollection clientCertificates, ServicePoint servicePoint, object initiatingRequest, ExecutionContext executionContext)
			: base(networkStream, true)
		{
			throw new NotSupportedException ();
		}

		internal WebExceptionStatus ExceptionStatus
		{
			get { throw new NotSupportedException (); }
		}

		protected override void Dispose(bool disposing)
		{
			throw new NotSupportedException ();
		}

		public override bool DataAvailable
		{
			get { throw new NotSupportedException (); }
		}

		public override int Read(byte[] buffer, int offset, int size)
		{
			throw new NotSupportedException ();
		}

		public override IAsyncResult BeginRead(byte[] buffer, int offset, int size, AsyncCallback asyncCallback, object asyncState)
		{
			throw new NotSupportedException ();
		}

		internal override IAsyncResult UnsafeBeginRead(byte[] buffer, int offset, int size, AsyncCallback asyncCallback, object asyncState)
		{
			throw new NotSupportedException ();
		}

		public override int EndRead(IAsyncResult asyncResult)
		{
			throw new NotSupportedException ();
		}

		public override void Write(byte[] buffer, int offset, int size)
		{
			throw new NotSupportedException ();
		}

		public override IAsyncResult BeginWrite( byte[] buffer, int offset, int size, AsyncCallback asyncCallback, object asyncState)
		{
			throw new NotSupportedException ();
		}

		internal override IAsyncResult UnsafeBeginWrite( byte[] buffer, int offset, int size, AsyncCallback asyncCallback, object asyncState)
		{
			throw new NotSupportedException ();
		}

		public override void EndWrite(IAsyncResult asyncResult)
		{
			throw new NotSupportedException ();
		}

		internal override void MultipleWrite(BufferOffsetSize[] buffers)
		{
			throw new NotSupportedException ();
		}

		internal override IAsyncResult BeginMultipleWrite(BufferOffsetSize[] buffers, AsyncCallback callback, object state)
		{
			throw new NotSupportedException ();
		}

		internal override IAsyncResult UnsafeBeginMultipleWrite(BufferOffsetSize[] buffers, AsyncCallback callback, object state)
		{
			throw new NotSupportedException ();
		}

		internal override void EndMultipleWrite(IAsyncResult asyncResult)
		{
			throw new NotSupportedException ();
		}

		public X509Certificate ClientCertificate
		{
			get { throw new NotSupportedException (); }
		}

		internal ChannelBinding GetChannelBinding(ChannelBindingKind kind)
		{
			throw new NotSupportedException ();
		}
	}
}