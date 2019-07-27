









#ifndef WEBRTC_BASE_SSLADAPTER_H_
#define WEBRTC_BASE_SSLADAPTER_H_

#include "webrtc/base/asyncsocket.h"

namespace rtc {



class SSLAdapter : public AsyncSocketAdapter {
 public:
  explicit SSLAdapter(AsyncSocket* socket)
    : AsyncSocketAdapter(socket), ignore_bad_cert_(false) { }

  bool ignore_bad_cert() const { return ignore_bad_cert_; }
  void set_ignore_bad_cert(bool ignore) { ignore_bad_cert_ = ignore; }

  
  
  
  virtual int StartSSL(const char* hostname, bool restartable) = 0;

  
  
  
  static SSLAdapter* Create(AsyncSocket* socket);

 private:
  
  bool ignore_bad_cert_;
};



typedef bool (*VerificationCallback)(void* cert);



bool InitializeSSL(VerificationCallback callback = NULL);


bool InitializeSSLThread();


bool CleanupSSL();



}  

#endif  
