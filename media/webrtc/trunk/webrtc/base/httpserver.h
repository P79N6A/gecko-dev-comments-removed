









#ifndef WEBRTC_BASE_HTTPSERVER_H__
#define WEBRTC_BASE_HTTPSERVER_H__

#include <map>
#include "webrtc/base/httpbase.h"

namespace rtc {

class AsyncSocket;
class HttpServer;
class SocketAddress;





const int HTTP_INVALID_CONNECTION_ID = 0;

struct HttpServerTransaction : public HttpTransaction {
public:
  HttpServerTransaction(int id) : connection_id_(id) { }
  int connection_id() const { return connection_id_; }

private:
  int connection_id_;
};

class HttpServer {
public:
  HttpServer();
  virtual ~HttpServer();

  int HandleConnection(StreamInterface* stream);
  
  sigslot::signal3<HttpServer*, int, StreamInterface*> SignalConnectionClosed;

  
  
  
  
  
  
  
  sigslot::signal3<HttpServer*, HttpServerTransaction*, bool*>
    SignalHttpRequestHeader;

  
  
  
  
  
  sigslot::signal2<HttpServer*, HttpServerTransaction*> SignalHttpRequest;
  void Respond(HttpServerTransaction* transaction);

  
  sigslot::signal3<HttpServer*, HttpServerTransaction*, int>
    SignalHttpRequestComplete;

  
  
  
  void Close(int connection_id, bool force);
  void CloseAll(bool force);

  
  
  sigslot::signal1<HttpServer*> SignalCloseAllComplete;

private:
  class Connection : private IHttpNotify {
  public:
    Connection(int connection_id, HttpServer* server);
    virtual ~Connection();

    void BeginProcess(StreamInterface* stream);
    StreamInterface* EndProcess();
    
    void Respond(HttpServerTransaction* transaction);
    void InitiateClose(bool force);

    
    virtual HttpError onHttpHeaderComplete(bool chunked, size_t& data_size);
    virtual void onHttpComplete(HttpMode mode, HttpError err);
    virtual void onHttpClosed(HttpError err);
  
    int connection_id_;
    HttpServer* server_;
    HttpBase base_;
    HttpServerTransaction* current_;
    bool signalling_, close_;
  };

  Connection* Find(int connection_id);
  void Remove(int connection_id);

  friend class Connection;
  typedef std::map<int,Connection*> ConnectionMap;

  ConnectionMap connections_;
  int next_connection_id_;
  bool closing_;
};



class HttpListenServer : public HttpServer, public sigslot::has_slots<> {
public:
  HttpListenServer();
  virtual ~HttpListenServer();

  int Listen(const SocketAddress& address);
  bool GetAddress(SocketAddress* address) const;
  void StopListening();

private:
  void OnReadEvent(AsyncSocket* socket);
  void OnConnectionClosed(HttpServer* server, int connection_id,
                          StreamInterface* stream);

  scoped_ptr<AsyncSocket> listener_;
};



}  

#endif 
