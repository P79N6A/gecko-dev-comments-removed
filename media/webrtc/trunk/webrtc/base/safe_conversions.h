











#ifndef WEBRTC_BASE_SAFE_CONVERSIONS_H_
#define WEBRTC_BASE_SAFE_CONVERSIONS_H_

#include <limits>

#include "webrtc/base/common.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/safe_conversions_impl.h"

namespace rtc {

inline void Check(bool condition) {
  if (!condition) {
    LOG(LS_ERROR) << "CHECK failed.";
    Break();
    
  }
}



template <typename Dst, typename Src>
inline bool IsValueInRangeForNumericType(Src value) {
  return internal::RangeCheck<Dst>(value) == internal::TYPE_VALID;
}




template <typename Dst, typename Src>
inline Dst checked_cast(Src value) {
  Check(IsValueInRangeForNumericType<Dst>(value));
  return static_cast<Dst>(value);
}




template <typename Dst, typename Src>
inline Dst saturated_cast(Src value) {
  
  if (std::numeric_limits<Dst>::is_iec559)
    return static_cast<Dst>(value);

  switch (internal::RangeCheck<Dst>(value)) {
    case internal::TYPE_VALID:
      return static_cast<Dst>(value);

    case internal::TYPE_UNDERFLOW:
      return std::numeric_limits<Dst>::min();

    case internal::TYPE_OVERFLOW:
      return std::numeric_limits<Dst>::max();

    
    case internal::TYPE_INVALID:
      Check(false);
      return std::numeric_limits<Dst>::max();
  }

  Check(false); 
  return static_cast<Dst>(value);
}

}  

#endif  
