









#include "webrtc/base/socketaddresspair.h"

namespace rtc {

SocketAddressPair::SocketAddressPair(
    const SocketAddress& src, const SocketAddress& dest)
    : src_(src), dest_(dest) {
}


bool SocketAddressPair::operator ==(const SocketAddressPair& p) const {
  return (src_ == p.src_) && (dest_ == p.dest_);
}

bool SocketAddressPair::operator <(const SocketAddressPair& p) const {
  if (src_ < p.src_)
    return true;
  if (p.src_ < src_)
    return false;
  if (dest_ < p.dest_)
    return true;
  if (p.dest_ < dest_)
    return false;
  return false;
}

size_t SocketAddressPair::Hash() const {
  return src_.Hash() ^ dest_.Hash();
}

} 
