



#ifndef BASE_SAFE_CONVERSIONS_H_
#define BASE_SAFE_CONVERSIONS_H_

#include <limits>

#include "base/logging.h"
#include "base/numerics/safe_conversions_impl.h"

namespace base {



template <typename Dst, typename Src>
inline bool IsValueInRangeForNumericType(Src value) {
  return internal::DstRangeRelationToSrcRange<Dst>(value) ==
         internal::RANGE_VALID;
}




template <typename Dst, typename Src>
inline Dst checked_cast(Src value) {
  CHECK(IsValueInRangeForNumericType<Dst>(value));
  return static_cast<Dst>(value);
}




template <typename Dst, typename Src>
inline Dst saturated_cast(Src value) {
  
  if (std::numeric_limits<Dst>::is_iec559)
    return static_cast<Dst>(value);

  switch (internal::DstRangeRelationToSrcRange<Dst>(value)) {
    case internal::RANGE_VALID:
      return static_cast<Dst>(value);

    case internal::RANGE_UNDERFLOW:
      return std::numeric_limits<Dst>::min();

    case internal::RANGE_OVERFLOW:
      return std::numeric_limits<Dst>::max();

    
    case internal::RANGE_INVALID:
      CHECK(false);
      return std::numeric_limits<Dst>::max();
  }

  NOTREACHED();
  return static_cast<Dst>(value);
}

}  

#endif  

