









#ifndef WEBRTC_BASE_SOCKETADAPTERS_H_
#define WEBRTC_BASE_SOCKETADAPTERS_H_

#include <map>
#include <string>

#include "webrtc/base/asyncsocket.h"
#include "webrtc/base/cryptstring.h"
#include "webrtc/base/logging.h"

namespace rtc {

struct HttpAuthContext;
class ByteBuffer;






class BufferedReadAdapter : public AsyncSocketAdapter {
 public:
  BufferedReadAdapter(AsyncSocket* socket, size_t buffer_size);
  virtual ~BufferedReadAdapter();

  virtual int Send(const void* pv, size_t cb);
  virtual int Recv(void* pv, size_t cb);

 protected:
  int DirectSend(const void* pv, size_t cb) {
    return AsyncSocketAdapter::Send(pv, cb);
  }

  void BufferInput(bool on = true);
  virtual void ProcessInput(char* data, size_t* len) = 0;

  virtual void OnReadEvent(AsyncSocket * socket);

 private:
  char * buffer_;
  size_t buffer_size_, data_len_;
  bool buffering_;
  DISALLOW_EVIL_CONSTRUCTORS(BufferedReadAdapter);
};




class AsyncProxyServerSocket : public BufferedReadAdapter {
 public:
  AsyncProxyServerSocket(AsyncSocket* socket, size_t buffer_size)
      : BufferedReadAdapter(socket, buffer_size) {}
  sigslot::signal2<AsyncProxyServerSocket*,
                   const SocketAddress&>  SignalConnectRequest;
  virtual void SendConnectResult(int err, const SocketAddress& addr) = 0;
};





class AsyncSSLSocket : public BufferedReadAdapter {
 public:
  explicit AsyncSSLSocket(AsyncSocket* socket);

  virtual int Connect(const SocketAddress& addr);

 protected:
  virtual void OnConnectEvent(AsyncSocket* socket);
  virtual void ProcessInput(char* data, size_t* len);
  DISALLOW_EVIL_CONSTRUCTORS(AsyncSSLSocket);
};



class AsyncSSLServerSocket : public BufferedReadAdapter {
 public:
  explicit AsyncSSLServerSocket(AsyncSocket* socket);

 protected:
  virtual void ProcessInput(char* data, size_t* len);
  DISALLOW_EVIL_CONSTRUCTORS(AsyncSSLServerSocket);
};




class AsyncHttpsProxySocket : public BufferedReadAdapter {
 public:
  AsyncHttpsProxySocket(AsyncSocket* socket, const std::string& user_agent,
    const SocketAddress& proxy,
    const std::string& username, const CryptString& password);
  virtual ~AsyncHttpsProxySocket();

  
  
  
  void SetForceConnect(bool force) { force_connect_ = force; }

  virtual int Connect(const SocketAddress& addr);
  virtual SocketAddress GetRemoteAddress() const;
  virtual int Close();
  virtual ConnState GetState() const;

 protected:
  virtual void OnConnectEvent(AsyncSocket* socket);
  virtual void OnCloseEvent(AsyncSocket* socket, int err);
  virtual void ProcessInput(char* data, size_t* len);

  bool ShouldIssueConnect() const;
  void SendRequest();
  void ProcessLine(char* data, size_t len);
  void EndResponse();
  void Error(int error);

 private:
  SocketAddress proxy_, dest_;
  std::string agent_, user_, headers_;
  CryptString pass_;
  bool force_connect_;
  size_t content_length_;
  int defer_error_;
  bool expect_close_;
  enum ProxyState {
    PS_INIT, PS_LEADER, PS_AUTHENTICATE, PS_SKIP_HEADERS, PS_ERROR_HEADERS,
    PS_TUNNEL_HEADERS, PS_SKIP_BODY, PS_TUNNEL, PS_WAIT_CLOSE, PS_ERROR
  } state_;
  HttpAuthContext * context_;
  std::string unknown_mechanisms_;
  DISALLOW_EVIL_CONSTRUCTORS(AsyncHttpsProxySocket);
};
















class AsyncSocksProxySocket : public BufferedReadAdapter {
 public:
  AsyncSocksProxySocket(AsyncSocket* socket, const SocketAddress& proxy,
    const std::string& username, const CryptString& password);

  virtual int Connect(const SocketAddress& addr);
  virtual SocketAddress GetRemoteAddress() const;
  virtual int Close();
  virtual ConnState GetState() const;

 protected:
  virtual void OnConnectEvent(AsyncSocket* socket);
  virtual void ProcessInput(char* data, size_t* len);

  void SendHello();
  void SendConnect();
  void SendAuth();
  void Error(int error);

 private:
  enum State {
    SS_INIT, SS_HELLO, SS_AUTH, SS_CONNECT, SS_TUNNEL, SS_ERROR
  };
  State state_;
  SocketAddress proxy_, dest_;
  std::string user_;
  CryptString pass_;
  DISALLOW_EVIL_CONSTRUCTORS(AsyncSocksProxySocket);
};


class AsyncSocksProxyServerSocket : public AsyncProxyServerSocket {
 public:
  explicit AsyncSocksProxyServerSocket(AsyncSocket* socket);

 private:
  virtual void ProcessInput(char* data, size_t* len);
  void DirectSend(const ByteBuffer& buf);

  void HandleHello(ByteBuffer* request);
  void SendHelloReply(uint8 method);
  void HandleAuth(ByteBuffer* request);
  void SendAuthReply(uint8 result);
  void HandleConnect(ByteBuffer* request);
  virtual void SendConnectResult(int result, const SocketAddress& addr);

  void Error(int error);

  static const int kBufferSize = 1024;
  enum State {
    SS_HELLO, SS_AUTH, SS_CONNECT, SS_CONNECT_PENDING, SS_TUNNEL, SS_ERROR
  };
  State state_;
  DISALLOW_EVIL_CONSTRUCTORS(AsyncSocksProxyServerSocket);
};




class LoggingSocketAdapter : public AsyncSocketAdapter {
 public:
  LoggingSocketAdapter(AsyncSocket* socket, LoggingSeverity level,
                 const char * label, bool hex_mode = false);

  virtual int Send(const void *pv, size_t cb);
  virtual int SendTo(const void *pv, size_t cb, const SocketAddress& addr);
  virtual int Recv(void *pv, size_t cb);
  virtual int RecvFrom(void *pv, size_t cb, SocketAddress *paddr);
  virtual int Close();

 protected:
  virtual void OnConnectEvent(AsyncSocket * socket);
  virtual void OnCloseEvent(AsyncSocket * socket, int err);

 private:
  LoggingSeverity level_;
  std::string label_;
  bool hex_mode_;
  LogMultilineState lms_;
  DISALLOW_EVIL_CONSTRUCTORS(LoggingSocketAdapter);
};



}  

#endif  
