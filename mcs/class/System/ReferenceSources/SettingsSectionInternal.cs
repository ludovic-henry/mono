
using System.Net.Security;

namespace System.Net.Configuration
{
	sealed class SettingsSectionInternal
	{
		static readonly SettingsSectionInternal instance = new SettingsSectionInternal ();

		internal static SettingsSectionInternal Section {
			get {
				return instance;
			}
		}

#if !MOBILE
		internal UnicodeEncodingConformance WebUtilityUnicodeEncodingConformance = UnicodeEncodingConformance.Auto;
		internal UnicodeDecodingConformance WebUtilityUnicodeDecodingConformance = UnicodeDecodingConformance.Auto;
#endif

		internal bool HttpListenerUnescapeRequestUrl = true;

		public int MaximumErrorResponseLength { get; set; } = 64;
		public int MaximumResponseHeadersLength { get; set; } = 64;
		public bool UseUnsafeHeaderParsing { get; set; } = false;

		internal bool CheckCertificateName { get; } = true;
		internal bool CheckCertificateRevocationList { get; set; } = false;
		internal int DnsRefreshTimeout { get; set; } = 120000;
		internal bool EnableDnsRoundRobin { get; set; } = false;
		internal bool Expect100Continue { get; set; } = true;
		internal bool UseNagleAlgorithm { get; set; } = true;

		internal EncryptionPolicy EncryptionPolicy { get; } = EncryptionPolicy.RequireEncryption;
		internal int MaximumUnauthorizedUploadLength { get; } = -1;
	}
}
