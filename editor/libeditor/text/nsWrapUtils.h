




































#ifndef nsWrapUtils_h__
#define nsWrapUtils_h__

#include "nsString.h"



class nsWrapUtils
{
public:
  static nsresult Rewrap(const nsAString& aInString,
                         PRUint32 aWrapCol, PRUint32 aFirstLineOffset,
                         PRBool aRespectNewlines,
                         const nsAString &aLineStartStr,
                         nsAString& aOutString);
};

#endif 

