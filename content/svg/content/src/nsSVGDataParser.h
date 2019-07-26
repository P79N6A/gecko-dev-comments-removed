




#ifndef __NS_SVGDATAPARSER_H__
#define __NS_SVGDATAPARSER_H__

#include "mozilla/RangedPtr.h"
#include "nsString.h"





class nsSVGDataParser
{
public:
  nsSVGDataParser(const nsAString& aValue);

protected:
  static bool IsAlpha(PRUnichar aCh) {
    
    return (aCh & 0x7f) == aCh && isalpha(aCh);
  }

  
  bool SkipCommaWsp();

  
  bool SkipWsp();

  mozilla::RangedPtr<const PRUnichar> mIter;
  const mozilla::RangedPtr<const PRUnichar> mEnd;
};


#endif 
