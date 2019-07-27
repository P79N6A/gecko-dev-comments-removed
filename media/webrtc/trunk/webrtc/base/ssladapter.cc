









#if HAVE_CONFIG_H
#include "config.h"
#endif  

#include "webrtc/base/ssladapter.h"

#include "webrtc/base/sslconfig.h"

#if SSL_USE_SCHANNEL

#include "schanneladapter.h"

#elif SSL_USE_OPENSSL  

#include "openssladapter.h"

#elif SSL_USE_NSS     

#include "nssstreamadapter.h"

#endif  



namespace rtc {

SSLAdapter*
SSLAdapter::Create(AsyncSocket* socket) {
#if SSL_USE_SCHANNEL
  return new SChannelAdapter(socket);
#elif SSL_USE_OPENSSL  
  return new OpenSSLAdapter(socket);
#else  
  delete socket;
  return NULL;
#endif  
}



#if SSL_USE_OPENSSL

bool InitializeSSL(VerificationCallback callback) {
  return OpenSSLAdapter::InitializeSSL(callback);
}

bool InitializeSSLThread() {
  return OpenSSLAdapter::InitializeSSLThread();
}

bool CleanupSSL() {
  return OpenSSLAdapter::CleanupSSL();
}

#elif SSL_USE_NSS  

bool InitializeSSL(VerificationCallback callback) {
  return NSSContext::InitializeSSL(callback);
}

bool InitializeSSLThread() {
  return NSSContext::InitializeSSLThread();
}

bool CleanupSSL() {
  return NSSContext::CleanupSSL();
}

#else  

bool InitializeSSL(VerificationCallback callback) {
  return true;
}

bool InitializeSSLThread() {
  return true;
}

bool CleanupSSL() {
  return true;
}

#endif  



}  
