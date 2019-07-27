



#ifndef BASE_SAFE_CONVERSIONS_IMPL_H_
#define BASE_SAFE_CONVERSIONS_IMPL_H_

#include <limits>

#include "base/template_util.h"

namespace base {
namespace internal {




template <typename NumericType>
struct MaxExponent {
  static const int value = std::numeric_limits<NumericType>::is_iec559
                               ? std::numeric_limits<NumericType>::max_exponent
                               : (sizeof(NumericType) * 8 + 1 -
                                  std::numeric_limits<NumericType>::is_signed);
};

enum IntegerRepresentation {
  INTEGER_REPRESENTATION_UNSIGNED,
  INTEGER_REPRESENTATION_SIGNED
};






enum NumericRangeRepresentation {
  NUMERIC_RANGE_NOT_CONTAINED,
  NUMERIC_RANGE_CONTAINED
};




template <
    typename Dst,
    typename Src,
    IntegerRepresentation DstSign = std::numeric_limits<Dst>::is_signed
                                            ? INTEGER_REPRESENTATION_SIGNED
                                            : INTEGER_REPRESENTATION_UNSIGNED,
    IntegerRepresentation SrcSign =
        std::numeric_limits<Src>::is_signed
            ? INTEGER_REPRESENTATION_SIGNED
            : INTEGER_REPRESENTATION_UNSIGNED >
struct StaticDstRangeRelationToSrcRange;



template <typename Dst, typename Src, IntegerRepresentation Sign>
struct StaticDstRangeRelationToSrcRange<Dst, Src, Sign, Sign> {
  static const NumericRangeRepresentation value =
      MaxExponent<Dst>::value >= MaxExponent<Src>::value
          ? NUMERIC_RANGE_CONTAINED
          : NUMERIC_RANGE_NOT_CONTAINED;
};



template <typename Dst, typename Src>
struct StaticDstRangeRelationToSrcRange<Dst,
                                        Src,
                                        INTEGER_REPRESENTATION_SIGNED,
                                        INTEGER_REPRESENTATION_UNSIGNED> {
  static const NumericRangeRepresentation value =
      MaxExponent<Dst>::value > MaxExponent<Src>::value
          ? NUMERIC_RANGE_CONTAINED
          : NUMERIC_RANGE_NOT_CONTAINED;
};


template <typename Dst, typename Src>
struct StaticDstRangeRelationToSrcRange<Dst,
                                        Src,
                                        INTEGER_REPRESENTATION_UNSIGNED,
                                        INTEGER_REPRESENTATION_SIGNED> {
  static const NumericRangeRepresentation value = NUMERIC_RANGE_NOT_CONTAINED;
};

enum RangeConstraint {
  RANGE_VALID = 0x0,  
  RANGE_UNDERFLOW = 0x1,  
  RANGE_OVERFLOW = 0x2,  
  RANGE_INVALID = RANGE_UNDERFLOW | RANGE_OVERFLOW  
};


inline RangeConstraint GetRangeConstraint(int integer_range_constraint) {
  DCHECK(integer_range_constraint >= RANGE_VALID &&
         integer_range_constraint <= RANGE_INVALID);
  return static_cast<RangeConstraint>(integer_range_constraint);
}




inline RangeConstraint GetRangeConstraint(bool is_in_upper_bound,
                                   bool is_in_lower_bound) {
  return GetRangeConstraint((is_in_upper_bound ? 0 : RANGE_OVERFLOW) |
                            (is_in_lower_bound ? 0 : RANGE_UNDERFLOW));
}

template <
    typename Dst,
    typename Src,
    IntegerRepresentation DstSign = std::numeric_limits<Dst>::is_signed
                                            ? INTEGER_REPRESENTATION_SIGNED
                                            : INTEGER_REPRESENTATION_UNSIGNED,
    IntegerRepresentation SrcSign = std::numeric_limits<Src>::is_signed
                                            ? INTEGER_REPRESENTATION_SIGNED
                                            : INTEGER_REPRESENTATION_UNSIGNED,
    NumericRangeRepresentation DstRange =
        StaticDstRangeRelationToSrcRange<Dst, Src>::value >
struct DstRangeRelationToSrcRangeImpl;






template <typename Dst,
          typename Src,
          IntegerRepresentation DstSign,
          IntegerRepresentation SrcSign>
struct DstRangeRelationToSrcRangeImpl<Dst,
                                      Src,
                                      DstSign,
                                      SrcSign,
                                      NUMERIC_RANGE_CONTAINED> {
  static RangeConstraint Check(Src value) { return RANGE_VALID; }
};



template <typename Dst, typename Src>
struct DstRangeRelationToSrcRangeImpl<Dst,
                                      Src,
                                      INTEGER_REPRESENTATION_SIGNED,
                                      INTEGER_REPRESENTATION_SIGNED,
                                      NUMERIC_RANGE_NOT_CONTAINED> {
  static RangeConstraint Check(Src value) {
    return std::numeric_limits<Dst>::is_iec559
               ? GetRangeConstraint(value <= std::numeric_limits<Dst>::max(),
                                    value >= -std::numeric_limits<Dst>::max())
               : GetRangeConstraint(value <= std::numeric_limits<Dst>::max(),
                                    value >= std::numeric_limits<Dst>::min());
  }
};


template <typename Dst, typename Src>
struct DstRangeRelationToSrcRangeImpl<Dst,
                                      Src,
                                      INTEGER_REPRESENTATION_UNSIGNED,
                                      INTEGER_REPRESENTATION_UNSIGNED,
                                      NUMERIC_RANGE_NOT_CONTAINED> {
  static RangeConstraint Check(Src value) {
    return GetRangeConstraint(value <= std::numeric_limits<Dst>::max(), true);
  }
};


template <typename Dst, typename Src>
struct DstRangeRelationToSrcRangeImpl<Dst,
                                      Src,
                                      INTEGER_REPRESENTATION_SIGNED,
                                      INTEGER_REPRESENTATION_UNSIGNED,
                                      NUMERIC_RANGE_NOT_CONTAINED> {
  static RangeConstraint Check(Src value) {
    return sizeof(Dst) > sizeof(Src)
               ? RANGE_VALID
               : GetRangeConstraint(
                     value <= static_cast<Src>(std::numeric_limits<Dst>::max()),
                     true);
  }
};



template <typename Dst, typename Src>
struct DstRangeRelationToSrcRangeImpl<Dst,
                                      Src,
                                      INTEGER_REPRESENTATION_UNSIGNED,
                                      INTEGER_REPRESENTATION_SIGNED,
                                      NUMERIC_RANGE_NOT_CONTAINED> {
  static RangeConstraint Check(Src value) {
    return (MaxExponent<Dst>::value >= MaxExponent<Src>::value)
               ? GetRangeConstraint(true, value >= static_cast<Src>(0))
               : GetRangeConstraint(
                     value <= static_cast<Src>(std::numeric_limits<Dst>::max()),
                     value >= static_cast<Src>(0));
  }
};

template <typename Dst, typename Src>
inline RangeConstraint DstRangeRelationToSrcRange(Src value) {
  static_assert(std::numeric_limits<Src>::is_specialized,
                "Argument must be numeric.");
  static_assert(std::numeric_limits<Dst>::is_specialized,
                "Result must be numeric.");
  return DstRangeRelationToSrcRangeImpl<Dst, Src>::Check(value);
}

}  
}  

#endif  

