





#ifndef _SDPENUM_H_
#define _SDPENUM_H_

#include <ostream>

#include "mozilla/Assertions.h"

namespace mozilla
{
namespace sdp
{

enum NetType { kNetTypeNone, kInternet };

inline std::ostream& operator<<(std::ostream& os, sdp::NetType t)
{
  switch (t) {
    case sdp::kNetTypeNone:
      MOZ_ASSERT(false);
      return os << "NONE";
    case sdp::kInternet:
      return os << "IN";
  }
  MOZ_CRASH("Unknown NetType");
}

enum AddrType { kAddrTypeNone, kIPv4, kIPv6 };

inline std::ostream& operator<<(std::ostream& os, sdp::AddrType t)
{
  switch (t) {
    case sdp::kAddrTypeNone:
      MOZ_ASSERT(false);
      return os << "NONE";
    case sdp::kIPv4:
      return os << "IP4";
    case sdp::kIPv6:
      return os << "IP6";
  }
  MOZ_CRASH("Unknown AddrType");
}

} 

} 

#endif
