



































#ifndef nsGBKConvUtil_h__
#define nsGBKConvUtil_h__
#include "prtypes.h"
#include "nscore.h"
class nsGBKConvUtil {
public:
  nsGBKConvUtil() {  }
  ~nsGBKConvUtil() { }
  void InitToGBKTable();
  PRUnichar GBKCharToUnicode(char aByte1, char aByte2);
  bool UnicodeToGBKChar(PRUnichar aChar, bool aToGL, 
                           char* aOutByte1, char* aOutByte2);
};
#endif 
