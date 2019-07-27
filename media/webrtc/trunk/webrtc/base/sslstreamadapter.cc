









#if HAVE_CONFIG_H
#include "config.h"
#endif  

#include "webrtc/base/sslstreamadapter.h"
#include "webrtc/base/sslconfig.h"

#if SSL_USE_SCHANNEL



#elif SSL_USE_OPENSSL  

#include "webrtc/base/opensslstreamadapter.h"

#elif SSL_USE_NSS      

#include "webrtc/base/nssstreamadapter.h"

#endif  



namespace rtc {

SSLStreamAdapter* SSLStreamAdapter::Create(StreamInterface* stream) {
#if SSL_USE_SCHANNEL
  return NULL;
#elif SSL_USE_OPENSSL  
  return new OpenSSLStreamAdapter(stream);
#elif SSL_USE_NSS     
  return new NSSStreamAdapter(stream);
#else  
  return NULL;
#endif
}


#if SSL_USE_SCHANNEL
bool SSLStreamAdapter::HaveDtls() { return false; }
bool SSLStreamAdapter::HaveDtlsSrtp() { return false; }
bool SSLStreamAdapter::HaveExporter() { return false; }
#elif SSL_USE_OPENSSL
bool SSLStreamAdapter::HaveDtls() {
  return OpenSSLStreamAdapter::HaveDtls();
}
bool SSLStreamAdapter::HaveDtlsSrtp() {
  return OpenSSLStreamAdapter::HaveDtlsSrtp();
}
bool SSLStreamAdapter::HaveExporter() {
  return OpenSSLStreamAdapter::HaveExporter();
}
#elif SSL_USE_NSS
bool SSLStreamAdapter::HaveDtls() {
  return NSSStreamAdapter::HaveDtls();
}
bool SSLStreamAdapter::HaveDtlsSrtp() {
  return NSSStreamAdapter::HaveDtlsSrtp();
}
bool SSLStreamAdapter::HaveExporter() {
  return NSSStreamAdapter::HaveExporter();
}
#endif  



}  
