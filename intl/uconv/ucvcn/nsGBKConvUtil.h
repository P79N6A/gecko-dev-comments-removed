



#ifndef nsGBKConvUtil_h_
#define nsGBKConvUtil_h_
#include "nscore.h"
class nsGBKConvUtil {
public:
  nsGBKConvUtil() {  }
  ~nsGBKConvUtil() { }
  PRUnichar GBKCharToUnicode(char aByte1, char aByte2);
  bool UnicodeToGBKChar(PRUnichar aChar, bool aToGL, 
                           char* aOutByte1, char* aOutByte2);
};
#endif 
