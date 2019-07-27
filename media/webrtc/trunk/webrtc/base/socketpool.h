









#ifndef WEBRTC_BASE_SOCKETPOOL_H_
#define WEBRTC_BASE_SOCKETPOOL_H_

#include <deque>
#include <list>
#include "webrtc/base/logging.h"
#include "webrtc/base/sigslot.h"
#include "webrtc/base/socketaddress.h"

namespace rtc {

class AsyncSocket;
class LoggingAdapter;
class SocketFactory;
class SocketStream;
class StreamInterface;





class StreamPool {
public:
  virtual ~StreamPool() { }

  virtual StreamInterface* RequestConnectedStream(const SocketAddress& remote,
                                                  int* err) = 0;
  virtual void ReturnConnectedStream(StreamInterface* stream) = 0;
};






class StreamCache : public StreamPool, public sigslot::has_slots<> {
public:
  StreamCache(StreamPool* pool);
  virtual ~StreamCache();

  
  virtual StreamInterface* RequestConnectedStream(const SocketAddress& remote,
                                                  int* err);
  virtual void ReturnConnectedStream(StreamInterface* stream);

private:
  typedef std::pair<SocketAddress, StreamInterface*> ConnectedStream;
  typedef std::list<ConnectedStream> ConnectedList;

  void OnStreamEvent(StreamInterface* stream, int events, int err);

  
  StreamPool* pool_;
  
  ConnectedList active_;
  
  ConnectedList cached_;
};






class NewSocketPool : public StreamPool {
public:
  NewSocketPool(SocketFactory* factory);
  virtual ~NewSocketPool();
  
  
  virtual StreamInterface* RequestConnectedStream(const SocketAddress& remote,
                                                  int* err);
  virtual void ReturnConnectedStream(StreamInterface* stream);
  
private:
  SocketFactory* factory_;
};







class ReuseSocketPool : public StreamPool, public sigslot::has_slots<> {
public:
  ReuseSocketPool(SocketFactory* factory);
  virtual ~ReuseSocketPool();

  
  virtual StreamInterface* RequestConnectedStream(const SocketAddress& remote,
                                                  int* err);
  virtual void ReturnConnectedStream(StreamInterface* stream);
  
private:
  void OnStreamEvent(StreamInterface* stream, int events, int err);

  SocketFactory* factory_;
  SocketStream* stream_;
  SocketAddress remote_;
  bool checked_out_;  
};






class LoggingPoolAdapter : public StreamPool {
public:
  LoggingPoolAdapter(StreamPool* pool, LoggingSeverity level,
                     const std::string& label, bool binary_mode);
  virtual ~LoggingPoolAdapter();

  
  virtual StreamInterface* RequestConnectedStream(const SocketAddress& remote,
                                                  int* err);
  virtual void ReturnConnectedStream(StreamInterface* stream);

private:
  StreamPool* pool_;
  LoggingSeverity level_;
  std::string label_;
  bool binary_mode_;
  typedef std::deque<LoggingAdapter*> StreamList;
  StreamList recycle_bin_;
};



}  

#endif  
