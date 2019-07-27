









#ifndef WEBRTC_BASE_SOCKETADDRESSPAIR_H__
#define WEBRTC_BASE_SOCKETADDRESSPAIR_H__

#include "webrtc/base/socketaddress.h"

namespace rtc {




class SocketAddressPair {
public:
  SocketAddressPair() {}
  SocketAddressPair(const SocketAddress& srs, const SocketAddress& dest);

  const SocketAddress& source() const { return src_; }
  const SocketAddress& destination() const { return dest_; }

  bool operator ==(const SocketAddressPair& r) const;
  bool operator <(const SocketAddressPair& r) const;

  size_t Hash() const;

private:
  SocketAddress src_;
  SocketAddress dest_;
};

} 

#endif 
