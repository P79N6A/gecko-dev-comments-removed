



#ifndef nsGBKConvUtil_h_
#define nsGBKConvUtil_h_
#include "nscore.h"
class nsGBKConvUtil {
public:
  nsGBKConvUtil() {  }
  ~nsGBKConvUtil() { }
  char16_t GBKCharToUnicode(char aByte1, char aByte2);
  bool UnicodeToGBKChar(char16_t aChar, bool aToGL, 
                           char* aOutByte1, char* aOutByte2);
};
#endif 
