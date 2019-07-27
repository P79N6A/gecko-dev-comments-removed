




#ifndef nsInternetCiter_h__
#define nsInternetCiter_h__

#include "nscore.h"



class nsInternetCiter
{
public:
  static nsresult GetCiteString(const nsAString & aInString, nsAString & aOutString);

  static nsresult StripCites(const nsAString & aInString, nsAString & aOutString);

  static nsresult Rewrap(const nsAString & aInString,
                         uint32_t aWrapCol, uint32_t aFirstLineOffset,
                         bool aRespectNewlines,
                         nsAString & aOutString);

protected:
  static nsresult StripCitesAndLinebreaks(const nsAString& aInString, nsAString& aOutString,
                                          bool aLinebreaksToo, int32_t* aCiteLevel);
};

#endif 

