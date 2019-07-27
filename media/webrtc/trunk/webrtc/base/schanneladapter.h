









#ifndef WEBRTC_BASE_SCHANNELADAPTER_H__
#define WEBRTC_BASE_SCHANNELADAPTER_H__

#include <string>
#include "webrtc/base/ssladapter.h"
#include "webrtc/base/messagequeue.h"
struct _SecBufferDesc;

namespace rtc {



class SChannelAdapter : public SSLAdapter, public MessageHandler {
public:
  SChannelAdapter(AsyncSocket* socket);
  virtual ~SChannelAdapter();

  virtual int StartSSL(const char* hostname, bool restartable);
  virtual int Send(const void* pv, size_t cb);
  virtual int Recv(void* pv, size_t cb);
  virtual int Close();

  
  virtual ConnState GetState() const;

protected:
  enum SSLState {
    SSL_NONE, SSL_WAIT, SSL_CONNECTING, SSL_CONNECTED, SSL_ERROR
  };
  struct SSLImpl;

  virtual void OnConnectEvent(AsyncSocket* socket);
  virtual void OnReadEvent(AsyncSocket* socket);
  virtual void OnWriteEvent(AsyncSocket* socket);
  virtual void OnCloseEvent(AsyncSocket* socket, int err);
  virtual void OnMessage(Message* pmsg);

  int BeginSSL();
  int ContinueSSL();
  int ProcessContext(long int status, _SecBufferDesc* sbd_in,
                     _SecBufferDesc* sbd_out);
  int DecryptData();

  int Read();
  int Flush();
  void Error(const char* context, int err, bool signal = true);
  void Cleanup();

  void PostEvent();

private:
  SSLState state_;
  std::string ssl_host_name_;
  
  bool restartable_; 
  
  bool signal_close_;
  
  bool message_pending_;
  SSLImpl* impl_;
};



} 

#endif
