




#include "nsSVGDataParser.h"
#include "SVGContentUtils.h"

nsSVGDataParser::nsSVGDataParser(const nsAString& aValue)
  : mIter(SVGContentUtils::GetStartRangedPtr(aValue)),
    mEnd(SVGContentUtils::GetEndRangedPtr(aValue))
{
}

bool
nsSVGDataParser::SkipCommaWsp()
{
  if (!SkipWsp()) {
    
    return false;
  }
  if (*mIter != ',') {
    return true;
  }
  ++mIter;
  return SkipWsp();
}

bool
nsSVGDataParser::SkipWsp()
{
  while (mIter != mEnd) {
    if (!IsSVGWhitespace(*mIter)) {
      return true;
    }
    ++mIter;
  }
  return false;
}
