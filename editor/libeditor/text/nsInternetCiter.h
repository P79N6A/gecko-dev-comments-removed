




































#ifndef nsInternetCiter_h__
#define nsInternetCiter_h__

#include "nsString.h"



class nsInternetCiter
{
public:
  static nsresult GetCiteString(const nsAString & aInString, nsAString & aOutString);

  static nsresult StripCites(const nsAString & aInString, nsAString & aOutString);

  static nsresult Rewrap(const nsAString & aInString,
                         PRUint32 aWrapCol, PRUint32 aFirstLineOffset,
                         bool aRespectNewlines,
                         nsAString & aOutString);

protected:
  static nsresult StripCitesAndLinebreaks(const nsAString& aInString, nsAString& aOutString,
                                          bool aLinebreaksToo, PRInt32* aCiteLevel);
};

#endif 

