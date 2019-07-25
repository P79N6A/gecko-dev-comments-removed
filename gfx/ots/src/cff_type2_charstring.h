



#ifndef OTS_CFF_TYPE2_CHARSTRING_H_
#define OTS_CFF_TYPE2_CHARSTRING_H_

#include "cff.h"
#include "ots.h"

#include <map>
#include <vector>

namespace ots {






















bool ValidateType2CharStringIndex(
    const CFFIndex &char_strings_index,
    const CFFIndex &global_subrs_index,
    const std::map<uint16_t, uint8_t> &fd_select,
    const std::vector<CFFIndex *> &local_subrs_per_font,
    const CFFIndex *local_subrs,
    Buffer *cff_table);


enum Type2CharStringOperator {
  kHStem = 1,
  kVStem = 3,
  kVMoveTo = 4,
  kRLineTo = 5,
  kHLineTo = 6,
  kVLineTo = 7,
  kRRCurveTo = 8,
  kCallSubr = 10,
  kReturn = 11,
  kEndChar = 14,
  kHStemHm = 18,
  kHintMask = 19,
  kCntrMask = 20,
  kRMoveTo = 21,
  kHMoveTo = 22,
  kVStemHm = 23,
  kRCurveLine = 24,
  kRLineCurve = 25,
  kVVCurveTo = 26,
  kHHCurveTo = 27,
  kCallGSubr = 29,
  kVHCurveTo = 30,
  kHVCurveTo = 31,
  kAnd = (12 << 8) + 3,
  kOr = (12 << 8) + 4,
  kNot = (12 << 8) + 5,
  kAbs = (12 << 8) + 9,
  kAdd = (12 << 8) + 10,
  kSub = (12 << 8) + 11,
  kDiv = (12 << 8) + 12,
  kNeg = (12 << 8) + 14,
  kEq = (12 << 8) + 15,
  kDrop = (12 << 8) + 18,
  kPut = (12 << 8) + 20,
  kGet = (12 << 8) + 21,
  kIfElse = (12 << 8) + 22,
  kRandom = (12 << 8) + 23,
  kMul = (12 << 8) + 24,
  kSqrt = (12 << 8) + 26,
  kDup = (12 << 8) + 27,
  kExch = (12 << 8) + 28,
  kIndex = (12 << 8) + 29,
  kRoll = (12 << 8) + 30,
  kHFlex = (12 << 8) + 34,
  kFlex = (12 << 8) + 35,
  kHFlex1 = (12 << 8) + 36,
  kFlex1 = (12 << 8) + 37,
  
  
};

}  

#endif  
