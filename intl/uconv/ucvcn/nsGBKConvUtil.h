



































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
  PRBool UnicodeToGBKChar(PRUnichar aChar, PRBool aToGL, 
                           char* aOutByte1, char* aOutByte2);
  void FillInfo(PRUint32 *aInfo, PRUint8 aStart1, PRUint8 aEnd1,
                                 PRUint8 aStart2, PRUint8 aEnd2);
  void FillGB2312Info(PRUint32 *aInfo);
};
#endif 
