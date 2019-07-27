









#ifndef WEBRTC_BASE_ASYNCPACKETSOCKET_H_
#define WEBRTC_BASE_ASYNCPACKETSOCKET_H_

#include "webrtc/base/dscp.h"
#include "webrtc/base/sigslot.h"
#include "webrtc/base/socket.h"
#include "webrtc/base/timeutils.h"

namespace rtc {




struct PacketTimeUpdateParams {
  PacketTimeUpdateParams()
      : rtp_sendtime_extension_id(-1), srtp_auth_tag_len(-1),
        srtp_packet_index(-1) {
  }

  int rtp_sendtime_extension_id;    
  std::vector<char> srtp_auth_key;  
  int srtp_auth_tag_len;            
  int64 srtp_packet_index;          
};



struct PacketOptions {
  PacketOptions() : dscp(DSCP_NO_CHANGE) {}
  explicit PacketOptions(DiffServCodePoint dscp) : dscp(dscp) {}

  DiffServCodePoint dscp;
  PacketTimeUpdateParams packet_time_params;
};



struct PacketTime {
  PacketTime() : timestamp(-1), not_before(-1) {}
  PacketTime(int64 timestamp, int64 not_before)
      : timestamp(timestamp), not_before(not_before) {
  }

  int64 timestamp;  
  int64 not_before; 
                    
                    
                    
                    
};

inline PacketTime CreatePacketTime(int64 not_before) {
  return PacketTime(TimeMicros(), not_before);
}



class AsyncPacketSocket : public sigslot::has_slots<> {
 public:
  enum State {
    STATE_CLOSED,
    STATE_BINDING,
    STATE_BOUND,
    STATE_CONNECTING,
    STATE_CONNECTED
  };

  AsyncPacketSocket() { }
  virtual ~AsyncPacketSocket() { }

  
  
  virtual SocketAddress GetLocalAddress() const = 0;

  
  virtual SocketAddress GetRemoteAddress() const = 0;

  
  virtual int Send(const void *pv, size_t cb, const PacketOptions& options) = 0;
  virtual int SendTo(const void *pv, size_t cb, const SocketAddress& addr,
                     const PacketOptions& options) = 0;

  
  virtual int Close() = 0;

  
  virtual State GetState() const = 0;

  
  virtual int GetOption(Socket::Option opt, int* value) = 0;
  virtual int SetOption(Socket::Option opt, int value) = 0;

  
  
  virtual int GetError() const = 0;
  virtual void SetError(int error) = 0;

  
  
  sigslot::signal5<AsyncPacketSocket*, const char*, size_t,
                   const SocketAddress&,
                   const PacketTime&> SignalReadPacket;

  
  sigslot::signal1<AsyncPacketSocket*> SignalReadyToSend;

  
  
  
  
  sigslot::signal2<AsyncPacketSocket*, const SocketAddress&> SignalAddressReady;

  
  
  sigslot::signal1<AsyncPacketSocket*> SignalConnect;

  
  
  sigslot::signal2<AsyncPacketSocket*, int> SignalClose;

  
  sigslot::signal2<AsyncPacketSocket*, AsyncPacketSocket*> SignalNewConnection;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(AsyncPacketSocket);
};

}  

#endif
