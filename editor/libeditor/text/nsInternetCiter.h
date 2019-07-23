




































#ifndef nsInternetCiter_h__
#define nsInternetCiter_h__

#include "nsICiter.h"

#include "nsString.h"



class nsInternetCiter  : public nsICiter
{
public:
  nsInternetCiter();
  virtual ~nsInternetCiter();


  NS_DECL_ISUPPORTS

  NS_IMETHOD GetCiteString(const nsAString & aInString, nsAString & aOutString);

  NS_IMETHOD StripCites(const nsAString & aInString, nsAString & aOutString);

  NS_IMETHOD Rewrap(const nsAString & aInString,
                    PRUint32 aWrapCol, PRUint32 aFirstLineOffset,
                    PRBool aRespectNewlines,
                    nsAString & aOutString);

protected:
  nsresult StripCitesAndLinebreaks(const nsAString& aInString, nsAString& aOutString,
                                   PRBool aLinebreaksToo, PRInt32* aCiteLevel);
};

#endif 

