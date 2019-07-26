





#include "CertVerifier.h"
#include "nsNSSCertificate.h"
#include "nsNSSComponent.h"
#include "mozilla/RefPtr.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsStreamUtils.h"
#include "nsNetUtil.h"
#include "nsILineInputStream.h"
#include "nsPromiseFlatString.h"
#include "nsTArray.h"

#include "cert.h"
#include "base64.h"
#include "nsSSLStatus.h"
#include "ScopedNSSTypes.h"

using namespace mozilla;

#ifdef DEBUG
#ifndef PSM_ENABLE_TEST_EV_ROOTS
#define PSM_ENABLE_TEST_EV_ROOTS
#endif
#endif

#ifdef PR_LOGGING
extern PRLogModuleInfo* gPIPNSSLog;
#endif

#define CONST_OID static const unsigned char
#define OI(x) { siDEROID, (unsigned char *)x, sizeof x }

struct nsMyTrustedEVInfo
{
  const char *dotted_oid;
  const char *oid_name; 
                  
  SECOidTag oid_tag;
  const char *ev_root_sha1_fingerprint;
  const char *issuer_base64;
  const char *serial_base64;
  CERTCertificate *cert;
};

























































static struct nsMyTrustedEVInfo myTrustedEVInfos[] = {
  





#ifdef DEBUG
  






  {
    
    
    "1.3.6.1.4.1.13769.666.666.666.1.500.9.1",
    "DEBUGtesting EV OID",
    SEC_OID_UNKNOWN,
    "AD:FE:0E:44:16:45:B0:17:46:8B:76:01:74:B7:FF:64:5A:EC:35:91",
    "MIHhMQswCQYDVQQGEwJVUzELMAkGA1UECBMCQ0ExFjAUBgNVBAcTDU1vdW50YWlu"
    "IFZpZXcxIzAhBgNVBAoTGk1vemlsbGEgLSBFViBkZWJ1ZyB0ZXN0IENBMR0wGwYD"
    "VQQLExRTZWN1cml0eSBFbmdpbmVlcmluZzEmMCQGA1UEAxMdRVYgVGVzdGluZyAo"
    "dW50cnVzdHdvcnRoeSkgQ0ExEzARBgNVBCkTCmV2LXRlc3QtY2ExLDAqBgkqhkiG"
    "9w0BCQEWHWNoYXJsYXRhbkB0ZXN0aW5nLmV4YW1wbGUuY29t",
    "AK/FPSJmJkky",
    nullptr
  },
  {
    
    
    "1.3.6.1.4.1.13769.666.666.666.1.500.9.1",
    "DEBUGtesting EV OID",
    SEC_OID_UNKNOWN,
    "9C:62:EF:DB:AE:F9:EB:36:58:FB:3B:D3:47:64:93:9D:86:29:6A:E0",
    "MIGnMQswCQYDVQQGEwJVUzELMAkGA1UECAwCQ0ExFjAUBgNVBAcMDU1vdW50YWlu"
    "IFZpZXcxIzAhBgNVBAoMGk1vemlsbGEgLSBFViBkZWJ1ZyB0ZXN0IENBMR0wGwYD"
    "VQQLDBRTZWN1cml0eSBFbmdpbmVlcmluZzEvMC0GA1UEAwwmWFBDU2hlbGwgRVYg"
    "VGVzdGluZyAodW50cnVzdHdvcnRoeSkgQ0E=",
    "At+3zdo=",
    nullptr
  },
#endif
  {
    
    "1.2.392.200091.100.721.1",
    "SECOM EV OID",
    SEC_OID_UNKNOWN,
    "FE:B8:C4:32:DC:F9:76:9A:CE:AE:3D:D8:90:8F:FD:28:86:65:64:7D",
    "MGAxCzAJBgNVBAYTAkpQMSUwIwYDVQQKExxTRUNPTSBUcnVzdCBTeXN0ZW1zIENP"
    "LixMVEQuMSowKAYDVQQLEyFTZWN1cml0eSBDb21tdW5pY2F0aW9uIEVWIFJvb3RD"
    "QTE=",
    "AA==",
    nullptr
  },
  {
    
    "1.3.6.1.4.1.6334.1.100.1",
    "Cybertrust EV OID",
    SEC_OID_UNKNOWN,
    "5F:43:E5:B1:BF:F8:78:8C:AC:1C:C7:CA:4A:9A:C6:22:2B:CC:34:C6",
    "MDsxGDAWBgNVBAoTD0N5YmVydHJ1c3QsIEluYzEfMB0GA1UEAxMWQ3liZXJ0cnVz"
    "dCBHbG9iYWwgUm9vdA==",
    "BAAAAAABD4WqLUg=",
    nullptr
  },
  {
    
    "2.16.756.1.89.1.2.1.1",
    "SwissSign EV OID",
    SEC_OID_UNKNOWN,
    "D8:C5:38:8A:B7:30:1B:1B:6E:D4:7A:E6:45:25:3A:6F:9F:1A:27:61",
    "MEUxCzAJBgNVBAYTAkNIMRUwEwYDVQQKEwxTd2lzc1NpZ24gQUcxHzAdBgNVBAMT"
    "FlN3aXNzU2lnbiBHb2xkIENBIC0gRzI=",
    "ALtAHEP1Xk+w",
    nullptr
  },
  {
    
    "1.3.6.1.4.1.23223.1.1.1",
    "StartCom EV OID",
    SEC_OID_UNKNOWN,
    "3E:2B:F7:F2:03:1B:96:F3:8C:E6:C4:D8:A8:5D:3E:2D:58:47:6A:0F",
    "MH0xCzAJBgNVBAYTAklMMRYwFAYDVQQKEw1TdGFydENvbSBMdGQuMSswKQYDVQQL"
    "EyJTZWN1cmUgRGlnaXRhbCBDZXJ0aWZpY2F0ZSBTaWduaW5nMSkwJwYDVQQDEyBT"
    "dGFydENvbSBDZXJ0aWZpY2F0aW9uIEF1dGhvcml0eQ==",
    "AQ==",
    nullptr
  },
  {
    
    "1.3.6.1.4.1.23223.1.1.1",
    "StartCom EV OID",
    SEC_OID_UNKNOWN,
    "A3:F1:33:3F:E2:42:BF:CF:C5:D1:4E:8F:39:42:98:40:68:10:D1:A0",
    "MH0xCzAJBgNVBAYTAklMMRYwFAYDVQQKEw1TdGFydENvbSBMdGQuMSswKQYDVQQL"
    "EyJTZWN1cmUgRGlnaXRhbCBDZXJ0aWZpY2F0ZSBTaWduaW5nMSkwJwYDVQQDEyBT"
    "dGFydENvbSBDZXJ0aWZpY2F0aW9uIEF1dGhvcml0eQ==",
    "LQ==",
    nullptr
  },
  {
    
    "1.3.6.1.4.1.23223.1.1.1",
    "StartCom EV OID",
    SEC_OID_UNKNOWN,
    "31:F1:FD:68:22:63:20:EE:C6:3B:3F:9D:EA:4A:3E:53:7C:7C:39:17",
    "MFMxCzAJBgNVBAYTAklMMRYwFAYDVQQKEw1TdGFydENvbSBMdGQuMSwwKgYDVQQD"
    "EyNTdGFydENvbSBDZXJ0aWZpY2F0aW9uIEF1dGhvcml0eSBHMg==",
    "Ow==",
    nullptr
  },
  {
    
    "2.16.840.1.113733.1.7.23.6",
    "VeriSign EV OID",
    SEC_OID_UNKNOWN,
    "4E:B6:D5:78:49:9B:1C:CF:5F:58:1E:AD:56:BE:3D:9B:67:44:A5:E5",
    "MIHKMQswCQYDVQQGEwJVUzEXMBUGA1UEChMOVmVyaVNpZ24sIEluYy4xHzAdBgNV"
    "BAsTFlZlcmlTaWduIFRydXN0IE5ldHdvcmsxOjA4BgNVBAsTMShjKSAyMDA2IFZl"
    "cmlTaWduLCBJbmMuIC0gRm9yIGF1dGhvcml6ZWQgdXNlIG9ubHkxRTBDBgNVBAMT"
    "PFZlcmlTaWduIENsYXNzIDMgUHVibGljIFByaW1hcnkgQ2VydGlmaWNhdGlvbiBB"
    "dXRob3JpdHkgLSBHNQ==",
    "GNrRniZ96LtKIVjNzGs7Sg==",
    nullptr
  },
  {
    
    "1.3.6.1.4.1.14370.1.6",
    "GeoTrust EV OID",
    SEC_OID_UNKNOWN,
    "32:3C:11:8E:1B:F7:B8:B6:52:54:E2:E2:10:0D:D6:02:90:37:F0:96",
    "MFgxCzAJBgNVBAYTAlVTMRYwFAYDVQQKEw1HZW9UcnVzdCBJbmMuMTEwLwYDVQQD"
    "EyhHZW9UcnVzdCBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0aG9yaXR5",
    "GKy1av1pthU6Y2yv2vrEoQ==",
    nullptr
  },
  {
    
    "2.16.840.1.113733.1.7.48.1",
    "Thawte EV OID",
    SEC_OID_UNKNOWN,
    "91:C6:D6:EE:3E:8A:C8:63:84:E5:48:C2:99:29:5C:75:6C:81:7B:81",
    "MIGpMQswCQYDVQQGEwJVUzEVMBMGA1UEChMMdGhhd3RlLCBJbmMuMSgwJgYDVQQL"
    "Ex9DZXJ0aWZpY2F0aW9uIFNlcnZpY2VzIERpdmlzaW9uMTgwNgYDVQQLEy8oYykg"
    "MjAwNiB0aGF3dGUsIEluYy4gLSBGb3IgYXV0aG9yaXplZCB1c2Ugb25seTEfMB0G"
    "A1UEAxMWdGhhd3RlIFByaW1hcnkgUm9vdCBDQQ==",
    "NE7VVyDV7exJ9C/ON9srbQ==",
    nullptr
  },
  {
    
    "2.16.840.1.114404.1.1.2.4.1",
    "Trustwave EV OID",
    SEC_OID_UNKNOWN,
    "B8:01:86:D1:EB:9C:86:A5:41:04:CF:30:54:F3:4C:52:B7:E5:58:C6",
    "MIGCMQswCQYDVQQGEwJVUzEeMBwGA1UECxMVd3d3LnhyYW1wc2VjdXJpdHkuY29t"
    "MSQwIgYDVQQKExtYUmFtcCBTZWN1cml0eSBTZXJ2aWNlcyBJbmMxLTArBgNVBAMT"
    "JFhSYW1wIEdsb2JhbCBDZXJ0aWZpY2F0aW9uIEF1dGhvcml0eQ==",
    "UJRs7Bjq1ZxN1ZfvdY+grQ==",
    nullptr
  },
  {
    
    "2.16.840.1.114404.1.1.2.4.1",
    "Trustwave EV OID",
    SEC_OID_UNKNOWN,
    "87:82:C6:C3:04:35:3B:CF:D2:96:92:D2:59:3E:7D:44:D9:34:FF:11",
    "MEgxCzAJBgNVBAYTAlVTMSAwHgYDVQQKExdTZWN1cmVUcnVzdCBDb3Jwb3JhdGlv"
    "bjEXMBUGA1UEAxMOU2VjdXJlVHJ1c3QgQ0E=",
    "DPCOXAgWpa1Cf/DrJxhZ0A==",
    nullptr
  },
  {
    
    "2.16.840.1.114404.1.1.2.4.1",
    "Trustwave EV OID",
    SEC_OID_UNKNOWN,
    "3A:44:73:5A:E5:81:90:1F:24:86:61:46:1E:3B:9C:C4:5F:F5:3A:1B",
    "MEoxCzAJBgNVBAYTAlVTMSAwHgYDVQQKExdTZWN1cmVUcnVzdCBDb3Jwb3JhdGlv"
    "bjEZMBcGA1UEAxMQU2VjdXJlIEdsb2JhbCBDQQ==",
    "B1YipOjUiolN9BPI8PjqpQ==",
    nullptr
  },
  {
    
    "1.3.6.1.4.1.6449.1.2.1.5.1",
    "Comodo EV OID",
    SEC_OID_UNKNOWN,
    "9F:74:4E:9F:2B:4D:BA:EC:0F:31:2C:50:B6:56:3B:8E:2D:93:C3:11",
    "MIGFMQswCQYDVQQGEwJHQjEbMBkGA1UECBMSR3JlYXRlciBNYW5jaGVzdGVyMRAw"
    "DgYDVQQHEwdTYWxmb3JkMRowGAYDVQQKExFDT01PRE8gQ0EgTGltaXRlZDErMCkG"
    "A1UEAxMiQ09NT0RPIEVDQyBDZXJ0aWZpY2F0aW9uIEF1dGhvcml0eQ==",
    "H0evqmIAcFBUTAGem2OZKg==",
    nullptr
  },
  {
    
    "1.3.6.1.4.1.6449.1.2.1.5.1",
    "Comodo EV OID",
    SEC_OID_UNKNOWN,
    "66:31:BF:9E:F7:4F:9E:B6:C9:D5:A6:0C:BA:6A:BE:D1:F7:BD:EF:7B",
    "MIGBMQswCQYDVQQGEwJHQjEbMBkGA1UECBMSR3JlYXRlciBNYW5jaGVzdGVyMRAw"
    "DgYDVQQHEwdTYWxmb3JkMRowGAYDVQQKExFDT01PRE8gQ0EgTGltaXRlZDEnMCUG"
    "A1UEAxMeQ09NT0RPIENlcnRpZmljYXRpb24gQXV0aG9yaXR5",
    "ToEtioJl4AsC7j41AkblPQ==",
    nullptr
  },
  {
    
    "1.3.6.1.4.1.6449.1.2.1.5.1",
    "Comodo EV OID",
    SEC_OID_UNKNOWN,
    "02:FA:F3:E2:91:43:54:68:60:78:57:69:4D:F5:E4:5B:68:85:18:68",
    "MG8xCzAJBgNVBAYTAlNFMRQwEgYDVQQKEwtBZGRUcnVzdCBBQjEmMCQGA1UECxMd"
    "QWRkVHJ1c3QgRXh0ZXJuYWwgVFRQIE5ldHdvcmsxIjAgBgNVBAMTGUFkZFRydXN0"
    "IEV4dGVybmFsIENBIFJvb3Q=",
    "AQ==",
    nullptr
  },
  {
    
    "1.3.6.1.4.1.6449.1.2.1.5.1",
    "Comodo EV OID",
    SEC_OID_UNKNOWN,
    "58:11:9F:0E:12:82:87:EA:50:FD:D9:87:45:6F:4F:78:DC:FA:D6:D4",
    "MIGTMQswCQYDVQQGEwJVUzELMAkGA1UECBMCVVQxFzAVBgNVBAcTDlNhbHQgTGFr"
    "ZSBDaXR5MR4wHAYDVQQKExVUaGUgVVNFUlRSVVNUIE5ldHdvcmsxITAfBgNVBAsT"
    "GGh0dHA6Ly93d3cudXNlcnRydXN0LmNvbTEbMBkGA1UEAxMSVVROIC0gREFUQUNv"
    "cnAgU0dD",
    "RL4Mi1AAIbQR0ypoBqmtaQ==",
    nullptr
  },
  {
    
    "1.3.6.1.4.1.6449.1.2.1.5.1",
    "Comodo EV OID",
    SEC_OID_UNKNOWN,
    "04:83:ED:33:99:AC:36:08:05:87:22:ED:BC:5E:46:00:E3:BE:F9:D7",
    "MIGXMQswCQYDVQQGEwJVUzELMAkGA1UECBMCVVQxFzAVBgNVBAcTDlNhbHQgTGFr"
    "ZSBDaXR5MR4wHAYDVQQKExVUaGUgVVNFUlRSVVNUIE5ldHdvcmsxITAfBgNVBAsT"
    "GGh0dHA6Ly93d3cudXNlcnRydXN0LmNvbTEfMB0GA1UEAxMWVVROLVVTRVJGaXJz"
    "dC1IYXJkd2FyZQ==",
    "RL4Mi1AAJLQR0zYq/mUK/Q==",
    nullptr
  },
  {
    
    "2.16.840.1.114413.1.7.23.3",
    "Go Daddy EV OID a",
    SEC_OID_UNKNOWN,
    "27:96:BA:E6:3F:18:01:E2:77:26:1B:A0:D7:77:70:02:8F:20:EE:E4",
    "MGMxCzAJBgNVBAYTAlVTMSEwHwYDVQQKExhUaGUgR28gRGFkZHkgR3JvdXAsIElu"
    "Yy4xMTAvBgNVBAsTKEdvIERhZGR5IENsYXNzIDIgQ2VydGlmaWNhdGlvbiBBdXRo"
    "b3JpdHk=",
    "AA==",
    nullptr
  },
  {
    
    "2.16.840.1.114413.1.7.23.3",
    "Go Daddy EV OID a",
    SEC_OID_UNKNOWN,
    "47:BE:AB:C9:22:EA:E8:0E:78:78:34:62:A7:9F:45:C2:54:FD:E6:8B",
    "MIGDMQswCQYDVQQGEwJVUzEQMA4GA1UECBMHQXJpem9uYTETMBEGA1UEBxMKU2Nv"
    "dHRzZGFsZTEaMBgGA1UEChMRR29EYWRkeS5jb20sIEluYy4xMTAvBgNVBAMTKEdv"
    "IERhZGR5IFJvb3QgQ2VydGlmaWNhdGUgQXV0aG9yaXR5IC0gRzI=",
    "AA==",
    nullptr
  },
  {
    
    "2.16.840.1.114413.1.7.23.3",
    "Go Daddy EV OID a",
    SEC_OID_UNKNOWN,
    "31:7A:2A:D0:7F:2B:33:5E:F5:A1:C3:4E:4B:57:E8:B7:D8:F1:FC:A6",
    "MIG7MSQwIgYDVQQHExtWYWxpQ2VydCBWYWxpZGF0aW9uIE5ldHdvcmsxFzAVBgNV"
    "BAoTDlZhbGlDZXJ0LCBJbmMuMTUwMwYDVQQLEyxWYWxpQ2VydCBDbGFzcyAyIFBv"
    "bGljeSBWYWxpZGF0aW9uIEF1dGhvcml0eTEhMB8GA1UEAxMYaHR0cDovL3d3dy52"
    "YWxpY2VydC5jb20vMSAwHgYJKoZIhvcNAQkBFhFpbmZvQHZhbGljZXJ0LmNvbQ==",
    "AQ==",
    nullptr
  },
  {
    
    "2.16.840.1.114414.1.7.23.3",
    "Go Daddy EV OID b",
    SEC_OID_UNKNOWN,
    "31:7A:2A:D0:7F:2B:33:5E:F5:A1:C3:4E:4B:57:E8:B7:D8:F1:FC:A6",
    "MIG7MSQwIgYDVQQHExtWYWxpQ2VydCBWYWxpZGF0aW9uIE5ldHdvcmsxFzAVBgNV"
    "BAoTDlZhbGlDZXJ0LCBJbmMuMTUwMwYDVQQLEyxWYWxpQ2VydCBDbGFzcyAyIFBv"
    "bGljeSBWYWxpZGF0aW9uIEF1dGhvcml0eTEhMB8GA1UEAxMYaHR0cDovL3d3dy52"
    "YWxpY2VydC5jb20vMSAwHgYJKoZIhvcNAQkBFhFpbmZvQHZhbGljZXJ0LmNvbQ==",
    "AQ==",
    nullptr
  },
  {
    
    "2.16.840.1.114414.1.7.23.3",
    "Go Daddy EV OID b",
    SEC_OID_UNKNOWN,
    "AD:7E:1C:28:B0:64:EF:8F:60:03:40:20:14:C3:D0:E3:37:0E:B5:8A",
    "MGgxCzAJBgNVBAYTAlVTMSUwIwYDVQQKExxTdGFyZmllbGQgVGVjaG5vbG9naWVz"
    "LCBJbmMuMTIwMAYDVQQLEylTdGFyZmllbGQgQ2xhc3MgMiBDZXJ0aWZpY2F0aW9u"
    "IEF1dGhvcml0eQ==",
    "AA==",
    nullptr
  },
  {
    
    "2.16.840.1.114414.1.7.23.3",
    "Go Daddy EV OID b",
    SEC_OID_UNKNOWN,
    "B5:1C:06:7C:EE:2B:0C:3D:F8:55:AB:2D:92:F4:FE:39:D4:E7:0F:0E",
    "MIGPMQswCQYDVQQGEwJVUzEQMA4GA1UECBMHQXJpem9uYTETMBEGA1UEBxMKU2Nv"
    "dHRzZGFsZTElMCMGA1UEChMcU3RhcmZpZWxkIFRlY2hub2xvZ2llcywgSW5jLjEy"
    "MDAGA1UEAxMpU3RhcmZpZWxkIFJvb3QgQ2VydGlmaWNhdGUgQXV0aG9yaXR5IC0g"
    "RzI=",
    "AA==",
    nullptr
  },
  {
    
    "2.16.840.1.114412.2.1",
    "DigiCert EV OID",
    SEC_OID_UNKNOWN,
    "5F:B7:EE:06:33:E2:59:DB:AD:0C:4C:9A:E6:D3:8F:1A:61:C7:DC:25",
    "MGwxCzAJBgNVBAYTAlVTMRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsT"
    "EHd3dy5kaWdpY2VydC5jb20xKzApBgNVBAMTIkRpZ2lDZXJ0IEhpZ2ggQXNzdXJh"
    "bmNlIEVWIFJvb3QgQ0E=",
    "AqxcJmoLQJuPC3nyrkYldw==",
    nullptr
  },
  {
    
    "1.3.6.1.4.1.8024.0.2.100.1.2",
    "Quo Vadis EV OID",
    SEC_OID_UNKNOWN,
    "CA:3A:FB:CF:12:40:36:4B:44:B2:16:20:88:80:48:39:19:93:7C:F7",
    "MEUxCzAJBgNVBAYTAkJNMRkwFwYDVQQKExBRdW9WYWRpcyBMaW1pdGVkMRswGQYD"
    "VQQDExJRdW9WYWRpcyBSb290IENBIDI=",
    "BQk=",
    nullptr
  },
  {
    
    "1.3.6.1.4.1.782.1.2.1.8.1",
    "Network Solutions EV OID",
    SEC_OID_UNKNOWN,
    "74:F8:A3:C3:EF:E7:B3:90:06:4B:83:90:3C:21:64:60:20:E5:DF:CE",
    "MGIxCzAJBgNVBAYTAlVTMSEwHwYDVQQKExhOZXR3b3JrIFNvbHV0aW9ucyBMLkwu"
    "Qy4xMDAuBgNVBAMTJ05ldHdvcmsgU29sdXRpb25zIENlcnRpZmljYXRlIEF1dGhv"
    "cml0eQ==",
    "V8szb8JcFuZHFhfjkDFo4A==",
    nullptr
  },
  {
    
    "2.16.840.1.114028.10.1.2",
    "Entrust EV OID",
    SEC_OID_UNKNOWN,
    "B3:1E:B1:B7:40:E3:6C:84:02:DA:DC:37:D4:4D:F5:D4:67:49:52:F9",
    "MIGwMQswCQYDVQQGEwJVUzEWMBQGA1UEChMNRW50cnVzdCwgSW5jLjE5MDcGA1UE"
    "CxMwd3d3LmVudHJ1c3QubmV0L0NQUyBpcyBpbmNvcnBvcmF0ZWQgYnkgcmVmZXJl"
    "bmNlMR8wHQYDVQQLExYoYykgMjAwNiBFbnRydXN0LCBJbmMuMS0wKwYDVQQDEyRF"
    "bnRydXN0IFJvb3QgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHk=",
    "RWtQVA==",
    nullptr
  },
  {
    
    "1.3.6.1.4.1.4146.1.1",
    "GlobalSign EV OID",
    SEC_OID_UNKNOWN,
    "B1:BC:96:8B:D4:F4:9D:62:2A:A8:9A:81:F2:15:01:52:A4:1D:82:9C",
    "MFcxCzAJBgNVBAYTAkJFMRkwFwYDVQQKExBHbG9iYWxTaWduIG52LXNhMRAwDgYD"
    "VQQLEwdSb290IENBMRswGQYDVQQDExJHbG9iYWxTaWduIFJvb3QgQ0E=",
    "BAAAAAABFUtaw5Q=",
    nullptr
  },
  {
    
    "1.3.6.1.4.1.4146.1.1",
    "GlobalSign EV OID",
    SEC_OID_UNKNOWN,
    "75:E0:AB:B6:13:85:12:27:1C:04:F8:5F:DD:DE:38:E4:B7:24:2E:FE",
    "MEwxIDAeBgNVBAsTF0dsb2JhbFNpZ24gUm9vdCBDQSAtIFIyMRMwEQYDVQQKEwpH"
    "bG9iYWxTaWduMRMwEQYDVQQDEwpHbG9iYWxTaWdu",
    "BAAAAAABD4Ym5g0=",
    nullptr
  },
  {
    
    "1.3.6.1.4.1.4146.1.1",
    "GlobalSign EV OID",
    SEC_OID_UNKNOWN,
    "D6:9B:56:11:48:F0:1C:77:C5:45:78:C1:09:26:DF:5B:85:69:76:AD",
    "MEwxIDAeBgNVBAsTF0dsb2JhbFNpZ24gUm9vdCBDQSAtIFIzMRMwEQYDVQQKEwpH"
    "bG9iYWxTaWduMRMwEQYDVQQDEwpHbG9iYWxTaWdu",
    "BAAAAAABIVhTCKI=",
    nullptr
  },
  {
    
    "2.16.578.1.26.1.3.3",
    "Buypass EV OID",
    SEC_OID_UNKNOWN,
    "61:57:3A:11:DF:0E:D8:7E:D5:92:65:22:EA:D0:56:D7:44:B3:23:71",
    "MEsxCzAJBgNVBAYTAk5PMR0wGwYDVQQKDBRCdXlwYXNzIEFTLTk4MzE2MzMyNzEd"
    "MBsGA1UEAwwUQnV5cGFzcyBDbGFzcyAzIENBIDE=",
    "Ag==",
    nullptr
  },
  {
    
    "2.16.578.1.26.1.3.3",
    "Buypass EV OID",
    SEC_OID_UNKNOWN,
    "DA:FA:F7:FA:66:84:EC:06:8F:14:50:BD:C7:C2:81:A5:BC:A9:64:57",
    "ME4xCzAJBgNVBAYTAk5PMR0wGwYDVQQKDBRCdXlwYXNzIEFTLTk4MzE2MzMyNzEg"
    "MB4GA1UEAwwXQnV5cGFzcyBDbGFzcyAzIFJvb3QgQ0E=",
    "Ag==",
    nullptr
  },
  {
    
    "1.3.6.1.4.1.22234.2.5.2.3.1",
    "Certplus EV OID",
    SEC_OID_UNKNOWN,
    "74:20:74:41:72:9C:DD:92:EC:79:31:D8:23:10:8D:C2:81:92:E2:BB",
    "MD0xCzAJBgNVBAYTAkZSMREwDwYDVQQKEwhDZXJ0cGx1czEbMBkGA1UEAxMSQ2xh"
    "c3MgMiBQcmltYXJ5IENB",
    "AIW9S/PY2uNp9pTXX8OlRCM=",
    nullptr
  },
  {
    
    "1.3.6.1.4.1.17326.10.14.2.1.2",
    "Camerfirma EV OID a",
    SEC_OID_UNKNOWN,
    "78:6A:74:AC:76:AB:14:7F:9C:6A:30:50:BA:9E:A8:7E:FE:9A:CE:3C",
    "MIGuMQswCQYDVQQGEwJFVTFDMEEGA1UEBxM6TWFkcmlkIChzZWUgY3VycmVudCBh"
    "ZGRyZXNzIGF0IHd3dy5jYW1lcmZpcm1hLmNvbS9hZGRyZXNzKTESMBAGA1UEBRMJ"
    "QTgyNzQzMjg3MRswGQYDVQQKExJBQyBDYW1lcmZpcm1hIFMuQS4xKTAnBgNVBAMT"
    "IENoYW1iZXJzIG9mIENvbW1lcmNlIFJvb3QgLSAyMDA4",
    "AKPaQn6ksa7a",
    nullptr
  },
  {
    
    "1.3.6.1.4.1.17326.10.8.12.1.2",
    "Camerfirma EV OID b",
    SEC_OID_UNKNOWN,
    "4A:BD:EE:EC:95:0D:35:9C:89:AE:C7:52:A1:2C:5B:29:F6:D6:AA:0C",
    "MIGsMQswCQYDVQQGEwJFVTFDMEEGA1UEBxM6TWFkcmlkIChzZWUgY3VycmVudCBh"
    "ZGRyZXNzIGF0IHd3dy5jYW1lcmZpcm1hLmNvbS9hZGRyZXNzKTESMBAGA1UEBRMJ"
    "QTgyNzQzMjg3MRswGQYDVQQKExJBQyBDYW1lcmZpcm1hIFMuQS4xJzAlBgNVBAMT"
    "Hkdsb2JhbCBDaGFtYmVyc2lnbiBSb290IC0gMjAwOA==",
    "AMnN0+nVfSPO",
    nullptr
  },
  {
    
    "1.2.276.0.44.1.1.1.4",
    "TC TrustCenter EV OID",
    SEC_OID_UNKNOWN,
    "96:56:CD:7B:57:96:98:95:D0:E1:41:46:68:06:FB:B8:C6:11:06:87",
    "MHsxCzAJBgNVBAYTAkRFMRwwGgYDVQQKExNUQyBUcnVzdENlbnRlciBHbWJIMSQw"
    "IgYDVQQLExtUQyBUcnVzdENlbnRlciBVbml2ZXJzYWwgQ0ExKDAmBgNVBAMTH1RD"
    "IFRydXN0Q2VudGVyIFVuaXZlcnNhbCBDQSBJSUk=",
    "YyUAAQACFI0zFQLkbPQ=",
    nullptr
  },
  {
    
    "1.3.6.1.4.1.34697.2.1",
    "AffirmTrust EV OID a",
    SEC_OID_UNKNOWN,
    "F9:B5:B6:32:45:5F:9C:BE:EC:57:5F:80:DC:E9:6E:2C:C7:B2:78:B7",
    "MEQxCzAJBgNVBAYTAlVTMRQwEgYDVQQKDAtBZmZpcm1UcnVzdDEfMB0GA1UEAwwW"
    "QWZmaXJtVHJ1c3QgQ29tbWVyY2lhbA==",
    "d3cGJyapsXw=",
    nullptr
  },
  {
    
    "1.3.6.1.4.1.34697.2.2",
    "AffirmTrust EV OID b",
    SEC_OID_UNKNOWN,
    "29:36:21:02:8B:20:ED:02:F5:66:C5:32:D1:D6:ED:90:9F:45:00:2F",
    "MEQxCzAJBgNVBAYTAlVTMRQwEgYDVQQKDAtBZmZpcm1UcnVzdDEfMB0GA1UEAwwW"
    "QWZmaXJtVHJ1c3QgTmV0d29ya2luZw==",
    "fE8EORzUmS0=",
    nullptr
  },
  {
    
    "1.3.6.1.4.1.34697.2.3",
    "AffirmTrust EV OID c",
    SEC_OID_UNKNOWN,
    "D8:A6:33:2C:E0:03:6F:B1:85:F6:63:4F:7D:6A:06:65:26:32:28:27",
    "MEExCzAJBgNVBAYTAlVTMRQwEgYDVQQKDAtBZmZpcm1UcnVzdDEcMBoGA1UEAwwT"
    "QWZmaXJtVHJ1c3QgUHJlbWl1bQ==",
    "bYwURrGmCu4=",
    nullptr
  },
  {
    
    "1.3.6.1.4.1.34697.2.4",
    "AffirmTrust EV OID d",
    SEC_OID_UNKNOWN,
    "B8:23:6B:00:2F:1D:16:86:53:01:55:6C:11:A4:37:CA:EB:FF:C3:BB",
    "MEUxCzAJBgNVBAYTAlVTMRQwEgYDVQQKDAtBZmZpcm1UcnVzdDEgMB4GA1UEAwwX"
    "QWZmaXJtVHJ1c3QgUHJlbWl1bSBFQ0M=",
    "dJclisc/elQ=",
    nullptr
  },
  {
    
    "1.2.616.1.113527.2.5.1.1",
    "Certum EV OID",
    SEC_OID_UNKNOWN,
    "07:E0:32:E0:20:B7:2C:3F:19:2F:06:28:A2:59:3A:19:A7:0F:06:9E",
    "MH4xCzAJBgNVBAYTAlBMMSIwIAYDVQQKExlVbml6ZXRvIFRlY2hub2xvZ2llcyBT"
    "LkEuMScwJQYDVQQLEx5DZXJ0dW0gQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkxIjAg"
    "BgNVBAMTGUNlcnR1bSBUcnVzdGVkIE5ldHdvcmsgQ0E=",
    "BETA",
    nullptr
  },
  {
    
    "1.3.6.1.4.1.14777.6.1.1",
    "Izenpe EV OID 1",
    SEC_OID_UNKNOWN,
    "2F:78:3D:25:52:18:A7:4A:65:39:71:B5:2C:A2:9C:45:15:6F:E9:19",
    "MDgxCzAJBgNVBAYTAkVTMRQwEgYDVQQKDAtJWkVOUEUgUy5BLjETMBEGA1UEAwwK"
    "SXplbnBlLmNvbQ==",
    "ALC3WhZIX7/hy/WL1xnmfQ==",
    nullptr
  },
  {
    
    "1.3.6.1.4.1.14777.6.1.2",
    "Izenpe EV OID 2",
    SEC_OID_UNKNOWN,
    "2F:78:3D:25:52:18:A7:4A:65:39:71:B5:2C:A2:9C:45:15:6F:E9:19",
    "MDgxCzAJBgNVBAYTAkVTMRQwEgYDVQQKDAtJWkVOUEUgUy5BLjETMBEGA1UEAwwK"
    "SXplbnBlLmNvbQ==",
    "ALC3WhZIX7/hy/WL1xnmfQ==",
    nullptr
  },
  {
    
    "1.2.40.0.17.1.22",
    "A-Trust EV OID",
    SEC_OID_UNKNOWN,
    "D3:C0:63:F2:19:ED:07:3E:34:AD:5D:75:0B:32:76:29:FF:D5:9A:F2",
    "MIGNMQswCQYDVQQGEwJBVDFIMEYGA1UECgw/QS1UcnVzdCBHZXMuIGYuIFNpY2hl"
    "cmhlaXRzc3lzdGVtZSBpbSBlbGVrdHIuIERhdGVudmVya2VociBHbWJIMRkwFwYD"
    "VQQLDBBBLVRydXN0LW5RdWFsLTAzMRkwFwYDVQQDDBBBLVRydXN0LW5RdWFsLTAz",
    "AWwe",
    nullptr
  },
  {
    
    "1.3.6.1.4.1.7879.13.24.1",
    "T-Systems EV OID",
    SEC_OID_UNKNOWN,
    "55:A6:72:3E:CB:F2:EC:CD:C3:23:74:70:19:9D:2A:BE:11:E3:81:D1",
    "MIGCMQswCQYDVQQGEwJERTErMCkGA1UECgwiVC1TeXN0ZW1zIEVudGVycHJpc2Ug"
    "U2VydmljZXMgR21iSDEfMB0GA1UECwwWVC1TeXN0ZW1zIFRydXN0IENlbnRlcjEl"
    "MCMGA1UEAwwcVC1UZWxlU2VjIEdsb2JhbFJvb3QgQ2xhc3MgMw==",
    "AQ==",
    nullptr
  },
  {
    
    "2.16.792.3.0.3.1.1.5",
    "TurkTrust EV OID",
    SEC_OID_UNKNOWN,
    "F1:7F:6F:B6:31:DC:99:E3:A3:C8:7F:FE:1C:F1:81:10:88:D9:60:33",
    "MIG/MT8wPQYDVQQDDDZUw5xSS1RSVVNUIEVsZWt0cm9uaWsgU2VydGlmaWthIEhp"
    "em1ldCBTYcSfbGF5xLFjxLFzxLExCzAJBgNVBAYTAlRSMQ8wDQYDVQQHDAZBbmth"
    "cmExXjBcBgNVBAoMVVTDnFJLVFJVU1QgQmlsZ2kgxLBsZXRpxZ9pbSB2ZSBCaWxp"
    "xZ9pbSBHw7x2ZW5sacSfaSBIaXptZXRsZXJpIEEuxZ4uIChjKSBBcmFsxLFrIDIw"
    "MDc=",
    "AQ==",
    nullptr
  },
  {
    
    "1.3.6.1.4.1.29836.1.10",
    "CNNIC EV OID",
    SEC_OID_UNKNOWN,
    "4F:99:AA:93:FB:2B:D1:37:26:A1:99:4A:CE:7F:F0:05:F2:93:5D:1E",
    "MIGKMQswCQYDVQQGEwJDTjEyMDAGA1UECgwpQ2hpbmEgSW50ZXJuZXQgTmV0d29y"
    "ayBJbmZvcm1hdGlvbiBDZW50ZXIxRzBFBgNVBAMMPkNoaW5hIEludGVybmV0IE5l"
    "dHdvcmsgSW5mb3JtYXRpb24gQ2VudGVyIEVWIENlcnRpZmljYXRlcyBSb290",
    "SJ8AAQ==",
    nullptr
  },
  {
    
    "1.3.6.1.4.1.40869.1.1.22.3",
    "TWCA EV OID",
    SEC_OID_UNKNOWN,
    "CF:9E:87:6D:D3:EB:FC:42:26:97:A3:B5:A3:7A:A0:76:A9:06:23:48",
    "MF8xCzAJBgNVBAYTAlRXMRIwEAYDVQQKDAlUQUlXQU4tQ0ExEDAOBgNVBAsMB1Jv"
    "b3QgQ0ExKjAoBgNVBAMMIVRXQ0EgUm9vdCBDZXJ0aWZpY2F0aW9uIEF1dGhvcml0"
    "eQ==",
    "AQ==",
    nullptr
  },
  {
    
    "1.3.6.1.4.1.4788.2.202.1",
    "D-TRUST EV OID",
    SEC_OID_UNKNOWN,
    "96:C9:1B:0B:95:B4:10:98:42:FA:D0:D8:22:79:FE:60:FA:B9:16:83",
    "MFAxCzAJBgNVBAYTAkRFMRUwEwYDVQQKDAxELVRydXN0IEdtYkgxKjAoBgNVBAMM"
    "IUQtVFJVU1QgUm9vdCBDbGFzcyAzIENBIDIgRVYgMjAwOQ==",
    "CYP0",
    nullptr
  },
  {
    
    "2.16.756.1.83.21.0",
    "Swisscom  EV OID",
    SEC_OID_UNKNOWN,
    "E7:A1:90:29:D3:D5:52:DC:0D:0F:C6:92:D3:EA:88:0D:15:2E:1A:6B",
    "MGcxCzAJBgNVBAYTAmNoMREwDwYDVQQKEwhTd2lzc2NvbTElMCMGA1UECxMcRGln"
    "aXRhbCBDZXJ0aWZpY2F0ZSBTZXJ2aWNlczEeMBwGA1UEAxMVU3dpc3Njb20gUm9v"
    "dCBFViBDQSAy",
    "APL6ZOJ0Y9ON/RAdBB92ylg=",
    nullptr
  },
  {
    
    "2.16.840.1.113733.1.7.23.6",
    "VeriSign EV OID",
    SEC_OID_UNKNOWN,
    "36:79:CA:35:66:87:72:30:4D:30:A5:FB:87:3B:0F:A7:7B:B7:0D:54",
    "MIG9MQswCQYDVQQGEwJVUzEXMBUGA1UEChMOVmVyaVNpZ24sIEluYy4xHzAdBgNV"
    "BAsTFlZlcmlTaWduIFRydXN0IE5ldHdvcmsxOjA4BgNVBAsTMShjKSAyMDA4IFZl"
    "cmlTaWduLCBJbmMuIC0gRm9yIGF1dGhvcml6ZWQgdXNlIG9ubHkxODA2BgNVBAMT"
    "L1ZlcmlTaWduIFVuaXZlcnNhbCBSb290IENlcnRpZmljYXRpb24gQXV0aG9yaXR5",
    "QBrEZCGzEyEDDrvkEhrFHQ==",
    nullptr
  },
  {
    
    "1.3.6.1.4.1.14370.1.6",
    "GeoTrust EV OID",
    SEC_OID_UNKNOWN,
    "03:9E:ED:B8:0B:E7:A0:3C:69:53:89:3B:20:D2:D9:32:3A:4C:2A:FD",
    "MIGYMQswCQYDVQQGEwJVUzEWMBQGA1UEChMNR2VvVHJ1c3QgSW5jLjE5MDcGA1UE"
    "CxMwKGMpIDIwMDggR2VvVHJ1c3QgSW5jLiAtIEZvciBhdXRob3JpemVkIHVzZSBv"
    "bmx5MTYwNAYDVQQDEy1HZW9UcnVzdCBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0"
    "aG9yaXR5IC0gRzM=",
    "FaxulBmyeUtB9iepwxgPHw==",
    nullptr
  },
  {
    
    "2.16.840.1.113733.1.7.48.1",
    "Thawte EV OID",
    SEC_OID_UNKNOWN,
    "F1:8B:53:8D:1B:E9:03:B6:A6:F0:56:43:5B:17:15:89:CA:F3:6B:F2",
    "MIGuMQswCQYDVQQGEwJVUzEVMBMGA1UEChMMdGhhd3RlLCBJbmMuMSgwJgYDVQQL"
    "Ex9DZXJ0aWZpY2F0aW9uIFNlcnZpY2VzIERpdmlzaW9uMTgwNgYDVQQLEy8oYykg"
    "MjAwOCB0aGF3dGUsIEluYy4gLSBGb3IgYXV0aG9yaXplZCB1c2Ugb25seTEkMCIG"
    "A1UEAxMbdGhhd3RlIFByaW1hcnkgUm9vdCBDQSAtIEcz",
    "YAGXt0an6rS0mtZLL/eQ+w==",
    nullptr
  },
  {
    
    "0.0.0.0",
    0, 
    SEC_OID_UNKNOWN,
    "00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD:EE:FF:00:11:22:33", 
    "Cg==",
    "Cg==",
    nullptr
  }
};

static SECOidTag
register_oid(const SECItem *oid_item, const char *oid_name)
{
  if (!oid_item)
    return SEC_OID_UNKNOWN;

  SECOidData od;
  od.oid.len = oid_item->len;
  od.oid.data = oid_item->data;
  od.offset = SEC_OID_UNKNOWN;
  od.desc = oid_name;
  od.mechanism = CKM_INVALID_MECHANISM;
  od.supportedExtension = INVALID_CERT_EXTENSION;
  return SECOID_AddEntry(&od);
}

#ifdef PSM_ENABLE_TEST_EV_ROOTS
class nsMyTrustedEVInfoClass : public nsMyTrustedEVInfo
{
public:
  nsMyTrustedEVInfoClass();
  ~nsMyTrustedEVInfoClass();
};

nsMyTrustedEVInfoClass::nsMyTrustedEVInfoClass()
{
  dotted_oid = nullptr;
  oid_name = nullptr;
  oid_tag = SEC_OID_UNKNOWN;
  ev_root_sha1_fingerprint = nullptr;
  issuer_base64 = nullptr;
  serial_base64 = nullptr;
  cert = nullptr;
}

nsMyTrustedEVInfoClass::~nsMyTrustedEVInfoClass()
{
  
  free(const_cast<char*>(dotted_oid));
  free(const_cast<char*>(oid_name));
  free(const_cast<char*>(ev_root_sha1_fingerprint));
  free(const_cast<char*>(issuer_base64));
  free(const_cast<char*>(serial_base64));
  if (cert)
    CERT_DestroyCertificate(cert);
}

typedef nsTArray< nsMyTrustedEVInfoClass* > testEVArray; 
static testEVArray *testEVInfos;
static bool testEVInfosLoaded = false;
#endif

#ifdef PSM_ENABLE_TEST_EV_ROOTS
static const char kTestEVRootsFileName[] = "test_ev_roots.txt";

static void
loadTestEVInfos()
{
  if (!testEVInfos)
    return;

  testEVInfos->Clear();

  char *env_val = getenv("ENABLE_TEST_EV_ROOTS_FILE");
  if (!env_val)
    return;
    
  int enabled_val = atoi(env_val);
  if (!enabled_val)
    return;

  nsCOMPtr<nsIFile> aFile;
  NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(aFile));
  if (!aFile)
    return;

  aFile->AppendNative(NS_LITERAL_CSTRING(kTestEVRootsFileName));

  nsresult rv;
  nsCOMPtr<nsIInputStream> fileInputStream;
  rv = NS_NewLocalFileInputStream(getter_AddRefs(fileInputStream), aFile);
  if (NS_FAILED(rv))
    return;

  nsCOMPtr<nsILineInputStream> lineInputStream = do_QueryInterface(fileInputStream, &rv);
  if (NS_FAILED(rv))
    return;

  nsAutoCString buffer;
  bool isMore = true;

  














  int line_counter = 0;
  bool found_error = false;

  enum { 
    pos_fingerprint, pos_readable_oid, pos_issuer, pos_serial
  } reader_position = pos_fingerprint;

  nsCString fingerprint, readable_oid, issuer, serial;

  while (isMore && NS_SUCCEEDED(lineInputStream->ReadLine(buffer, &isMore))) {
    ++line_counter;
    if (buffer.IsEmpty() || buffer.First() == '#') {
      continue;
    }

    int32_t seperatorIndex = buffer.FindChar(' ', 0);
    if (seperatorIndex == 0) {
      found_error = true;
      break;
    }

    const nsASingleFragmentCString &descriptor = Substring(buffer, 0, seperatorIndex);
    const nsASingleFragmentCString &data = 
            Substring(buffer, seperatorIndex + 1, 
                      buffer.Length() - seperatorIndex + 1);

    if (reader_position == pos_fingerprint &&
        descriptor.EqualsLiteral(("1_fingerprint"))) {
      fingerprint = data;
      reader_position = pos_readable_oid;
      continue;
    }
    else if (reader_position == pos_readable_oid &&
        descriptor.EqualsLiteral(("2_readable_oid"))) {
      readable_oid = data;
      reader_position = pos_issuer;
      continue;
    }
    else if (reader_position == pos_issuer &&
        descriptor.EqualsLiteral(("3_issuer"))) {
      issuer = data;
      reader_position = pos_serial;
      continue;
    }
    else if (reader_position == pos_serial &&
        descriptor.EqualsLiteral(("4_serial"))) {
      serial = data;
      reader_position = pos_fingerprint;
    }
    else {
      found_error = true;
      break;
    }

    nsMyTrustedEVInfoClass *temp_ev = new nsMyTrustedEVInfoClass;
    if (!temp_ev)
      return;

    temp_ev->ev_root_sha1_fingerprint = strdup(fingerprint.get());
    temp_ev->oid_name = strdup(readable_oid.get());
    temp_ev->dotted_oid = strdup(readable_oid.get());
    temp_ev->issuer_base64 = strdup(issuer.get());
    temp_ev->serial_base64 = strdup(serial.get());

    SECStatus rv;
    CERTIssuerAndSN ias;

    rv = ATOB_ConvertAsciiToItem(&ias.derIssuer, const_cast<char*>(temp_ev->issuer_base64));
    NS_ASSERTION(rv==SECSuccess, "error converting ascii to binary.");
    rv = ATOB_ConvertAsciiToItem(&ias.serialNumber, const_cast<char*>(temp_ev->serial_base64));
    NS_ASSERTION(rv==SECSuccess, "error converting ascii to binary.");

    temp_ev->cert = CERT_FindCertByIssuerAndSN(nullptr, &ias);
    NS_ASSERTION(temp_ev->cert, "Could not find EV root in NSS storage");

    SECITEM_FreeItem(&ias.derIssuer, false);
    SECITEM_FreeItem(&ias.serialNumber, false);

    if (!temp_ev->cert)
      return;

    nsNSSCertificate c(temp_ev->cert);
    nsAutoString fingerprint;
    c.GetSha1Fingerprint(fingerprint);

    NS_ConvertASCIItoUTF16 sha1(temp_ev->ev_root_sha1_fingerprint);

    if (sha1 != fingerprint) {
      NS_ASSERTION(sha1 == fingerprint, "found EV root with unexpected SHA1 mismatch");
      CERT_DestroyCertificate(temp_ev->cert);
      temp_ev->cert = nullptr;
      return;
    }

    SECItem ev_oid_item;
    ev_oid_item.data = nullptr;
    ev_oid_item.len = 0;
    SECStatus srv = SEC_StringToOID(nullptr, &ev_oid_item,
                                    readable_oid.get(), readable_oid.Length());
    if (srv != SECSuccess) {
      delete temp_ev;
      found_error = true;
      break;
    }

    temp_ev->oid_tag = register_oid(&ev_oid_item, temp_ev->oid_name);
    SECITEM_FreeItem(&ev_oid_item, false);

    testEVInfos->AppendElement(temp_ev);
  }

  if (found_error) {
    fprintf(stderr, "invalid line %d in test_ev_roots file\n", line_counter);
  }
}

static bool 
isEVPolicyInExternalDebugRootsFile(SECOidTag policyOIDTag)
{
  if (!testEVInfos)
    return false;

  char *env_val = getenv("ENABLE_TEST_EV_ROOTS_FILE");
  if (!env_val)
    return false;
    
  int enabled_val = atoi(env_val);
  if (!enabled_val)
    return false;

  for (size_t i=0; i<testEVInfos->Length(); ++i) {
    nsMyTrustedEVInfoClass *ev = testEVInfos->ElementAt(i);
    if (!ev)
      continue;
    if (policyOIDTag == ev->oid_tag)
      return true;
  }

  return false;
}

static bool 
getRootsForOidFromExternalRootsFile(CERTCertList* certList, 
                                    SECOidTag policyOIDTag)
{
  if (!testEVInfos)
    return false;

  char *env_val = getenv("ENABLE_TEST_EV_ROOTS_FILE");
  if (!env_val)
    return false;
    
  int enabled_val = atoi(env_val);
  if (!enabled_val)
    return false;

  for (size_t i=0; i<testEVInfos->Length(); ++i) {
    nsMyTrustedEVInfoClass *ev = testEVInfos->ElementAt(i);
    if (!ev)
      continue;
    if (policyOIDTag == ev->oid_tag)
      CERT_AddCertToListTail(certList, CERT_DupCertificate(ev->cert));
  }

  return false;
}
#endif

static bool 
isEVPolicy(SECOidTag policyOIDTag)
{
  for (size_t iEV=0; iEV < (sizeof(myTrustedEVInfos)/sizeof(nsMyTrustedEVInfo)); ++iEV) {
    nsMyTrustedEVInfo &entry = myTrustedEVInfos[iEV];
    if (!entry.oid_name) 
      continue;
    if (policyOIDTag == entry.oid_tag) {
      return true;
    }
  }

#ifdef PSM_ENABLE_TEST_EV_ROOTS
  if (isEVPolicyInExternalDebugRootsFile(policyOIDTag)) {
    return true;
  }
#endif

  return false;
}

namespace mozilla { namespace psm {

CERTCertList*
getRootsForOid(SECOidTag oid_tag)
{
  CERTCertList *certList = CERT_NewCertList();
  if (!certList)
    return nullptr;

  for (size_t iEV=0; iEV < (sizeof(myTrustedEVInfos)/sizeof(nsMyTrustedEVInfo)); ++iEV) {
    nsMyTrustedEVInfo &entry = myTrustedEVInfos[iEV];
    if (!entry.oid_name) 
      continue;
    if (entry.oid_tag == oid_tag)
      CERT_AddCertToListTail(certList, CERT_DupCertificate(entry.cert));
  }

#ifdef PSM_ENABLE_TEST_EV_ROOTS
  getRootsForOidFromExternalRootsFile(certList, oid_tag);
#endif
  return certList;
}

} } 

PRStatus
nsNSSComponent::IdentityInfoInit()
{
  for (size_t iEV=0; iEV < (sizeof(myTrustedEVInfos)/sizeof(nsMyTrustedEVInfo)); ++iEV) {
    nsMyTrustedEVInfo &entry = myTrustedEVInfos[iEV];
    if (!entry.oid_name) 
      continue;

    SECStatus rv;
    CERTIssuerAndSN ias;

    rv = ATOB_ConvertAsciiToItem(&ias.derIssuer, const_cast<char*>(entry.issuer_base64));
    NS_ASSERTION(rv==SECSuccess, "error converting ascii to binary.");
    rv = ATOB_ConvertAsciiToItem(&ias.serialNumber, const_cast<char*>(entry.serial_base64));
    NS_ASSERTION(rv==SECSuccess, "error converting ascii to binary.");
    ias.serialNumber.type = siUnsignedInteger;

    entry.cert = CERT_FindCertByIssuerAndSN(nullptr, &ias);

#ifdef DEBUG
    
    if (iEV > 1) {
       NS_ASSERTION(entry.cert, "Could not find EV root in NSS storage");
    }
#endif

    SECITEM_FreeItem(&ias.derIssuer, false);
    SECITEM_FreeItem(&ias.serialNumber, false);

    if (!entry.cert)
      continue;

    nsNSSCertificate c(entry.cert);
    nsAutoString fingerprint;
    c.GetSha1Fingerprint(fingerprint);

    NS_ConvertASCIItoUTF16 sha1(entry.ev_root_sha1_fingerprint);

    if (sha1 != fingerprint) {
      NS_ASSERTION(sha1 == fingerprint, "found EV root with unexpected SHA1 mismatch");
      CERT_DestroyCertificate(entry.cert);
      entry.cert = nullptr;
      continue;
    }

    SECItem ev_oid_item;
    ev_oid_item.data = nullptr;
    ev_oid_item.len = 0;
    SECStatus srv = SEC_StringToOID(nullptr, &ev_oid_item, 
                                    entry.dotted_oid, 0);
    if (srv != SECSuccess)
      continue;

    entry.oid_tag = register_oid(&ev_oid_item, entry.oid_name);

    SECITEM_FreeItem(&ev_oid_item, false);
  }

#ifdef PSM_ENABLE_TEST_EV_ROOTS
  if (!testEVInfosLoaded) {
    testEVInfosLoaded = true;
    testEVInfos = new testEVArray;
    if (testEVInfos) {
      loadTestEVInfos();
    }
  }
#endif

  return PR_SUCCESS;
}

namespace mozilla { namespace psm {

SECStatus getFirstEVPolicy(CERTCertificate *cert, SECOidTag &outOidTag)
{
  if (!cert)
    return SECFailure;

  if (cert->extensions) {
    for (int i=0; cert->extensions[i]; i++) {
      const SECItem *oid = &cert->extensions[i]->id;

      SECOidTag oidTag = SECOID_FindOIDTag(oid);
      if (oidTag != SEC_OID_X509_CERTIFICATE_POLICIES)
        continue;

      SECItem *value = &cert->extensions[i]->value;

      CERTCertificatePolicies *policies;
      CERTPolicyInfo **policyInfos, *policyInfo;
    
      policies = CERT_DecodeCertificatePoliciesExtension(value);
      if (!policies)
        continue;
    
      policyInfos = policies->policyInfos;

      bool found = false;
      while (*policyInfos) {
        policyInfo = *policyInfos++;

        SECOidTag oid_tag = policyInfo->oid;
        if (oid_tag != SEC_OID_UNKNOWN && isEVPolicy(oid_tag)) {
          
          outOidTag = oid_tag;
          found = true;
          break;
        }
      }
      CERT_DestroyCertificatePoliciesExtension(policies);
      if (found)
        return SECSuccess;
    }
  }

  return SECFailure;
}

} } 

NS_IMETHODIMP
nsSSLStatus::GetIsExtendedValidation(bool* aIsEV)
{
  NS_ENSURE_ARG_POINTER(aIsEV);
  *aIsEV = false;

#ifdef NSS_NO_LIBPKIX
  return NS_OK;
#else
  nsCOMPtr<nsIX509Cert> cert = mServerCert;
  nsresult rv;
  nsCOMPtr<nsIIdentityInfo> idinfo = do_QueryInterface(cert, &rv);

  
  
  
  
  
  if (!idinfo) {
    NS_ERROR("nsSSLStatus has null mServerCert or was called in the content "
             "process");
    return NS_ERROR_UNEXPECTED;
  }

  
  if (mHaveCertErrorBits)
    return NS_OK;

  return idinfo->GetIsExtendedValidation(aIsEV);
#endif
}

#ifndef NSS_NO_LIBPKIX

nsresult
nsNSSCertificate::hasValidEVOidTag(SECOidTag &resultOidTag, bool &validEV)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return NS_ERROR_NOT_AVAILABLE;

  nsresult nrv;
  nsCOMPtr<nsINSSComponent> nssComponent = 
    do_GetService(PSM_COMPONENT_CONTRACTID, &nrv);
  if (NS_FAILED(nrv))
    return nrv;
  nssComponent->EnsureIdentityInfoLoaded();

  RefPtr<mozilla::psm::CertVerifier> certVerifier(mozilla::psm::GetDefaultCertVerifier());
  NS_ENSURE_TRUE(certVerifier, NS_ERROR_UNEXPECTED);

  validEV = false;
  resultOidTag = SEC_OID_UNKNOWN;

  SECStatus rv = certVerifier->VerifyCert(mCert,
                                          certificateUsageSSLServer, PR_Now(),
                                          nullptr ,
                                          0, nullptr, &resultOidTag);

  if (rv != SECSuccess) {
    resultOidTag = SEC_OID_UNKNOWN;
  }
  if (resultOidTag != SEC_OID_UNKNOWN) {
    validEV = true;
  }
  return NS_OK;
}

nsresult
nsNSSCertificate::getValidEVOidTag(SECOidTag &resultOidTag, bool &validEV)
{
  if (mCachedEVStatus != ev_status_unknown) {
    validEV = (mCachedEVStatus == ev_status_valid);
    if (validEV)
      resultOidTag = mCachedEVOidTag;
    return NS_OK;
  }

  nsresult rv = hasValidEVOidTag(resultOidTag, validEV);
  if (NS_SUCCEEDED(rv)) {
    if (validEV) {
      mCachedEVOidTag = resultOidTag;
    }
    mCachedEVStatus = validEV ? ev_status_valid : ev_status_invalid;
  }
  return rv;
}

#endif 

NS_IMETHODIMP
nsNSSCertificate::GetIsExtendedValidation(bool* aIsEV)
{
#ifdef NSS_NO_LIBPKIX
  *aIsEV = false;
  return NS_OK;
#else
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return NS_ERROR_NOT_AVAILABLE;

  NS_ENSURE_ARG(aIsEV);
  *aIsEV = false;

  if (mCachedEVStatus != ev_status_unknown) {
    *aIsEV = (mCachedEVStatus == ev_status_valid);
    return NS_OK;
  }

  SECOidTag oid_tag;
  return getValidEVOidTag(oid_tag, *aIsEV);
#endif
}

NS_IMETHODIMP
nsNSSCertificate::GetValidEVPolicyOid(nsACString &outDottedOid)
{
  outDottedOid.Truncate();

#ifndef NSS_NO_LIBPKIX
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return NS_ERROR_NOT_AVAILABLE;

  SECOidTag oid_tag;
  bool valid;
  nsresult rv = getValidEVOidTag(oid_tag, valid);
  if (NS_FAILED(rv))
    return rv;

  if (valid) {
    SECOidData *oid_data = SECOID_FindOIDByTag(oid_tag);
    if (!oid_data)
      return NS_ERROR_FAILURE;

    char *oid_str = CERT_GetOidString(&oid_data->oid);
    if (!oid_str)
      return NS_ERROR_FAILURE;

    outDottedOid = oid_str;
    PR_smprintf_free(oid_str);
  }
#endif

  return NS_OK;
}

#ifndef NSS_NO_LIBPKIX

NS_IMETHODIMP
nsNSSComponent::EnsureIdentityInfoLoaded()
{
  PRStatus rv = PR_CallOnce(&mIdentityInfoCallOnce, IdentityInfoInit);
  return (rv == PR_SUCCESS) ? NS_OK : NS_ERROR_FAILURE; 
}


void
nsNSSComponent::CleanupIdentityInfo()
{
  nsNSSShutDownPreventionLock locker;
  for (size_t iEV=0; iEV < (sizeof(myTrustedEVInfos)/sizeof(nsMyTrustedEVInfo)); ++iEV) {
    nsMyTrustedEVInfo &entry = myTrustedEVInfos[iEV];
    if (entry.cert) {
      CERT_DestroyCertificate(entry.cert);
      entry.cert = nullptr;
    }
  }

#ifdef PSM_ENABLE_TEST_EV_ROOTS
  if (testEVInfosLoaded) {
    testEVInfosLoaded = false;
    if (testEVInfos) {
      for (size_t i = 0; i<testEVInfos->Length(); ++i) {
        delete testEVInfos->ElementAt(i);
      }
      testEVInfos->Clear();
      delete testEVInfos;
      testEVInfos = nullptr;
    }
  }
#endif
  memset(&mIdentityInfoCallOnce, 0, sizeof(PRCallOnceType));
}

#endif
