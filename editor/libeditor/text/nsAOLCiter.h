




































#ifndef nsAOLCiter_h__
#define nsAOLCiter_h__

#include "nsString.h"
#include "nsICiter.h"




class nsAOLCiter  : public nsICiter
{
public:
  nsAOLCiter();
  virtual ~nsAOLCiter();


  NS_DECL_ISUPPORTS

  NS_IMETHOD GetCiteString(const nsAString & aInString, nsAString & aOutString);

  NS_IMETHOD StripCites(const nsAString & aInString, nsAString & aOutString);

  NS_IMETHOD Rewrap(const nsAString & aInString,
                    PRUint32 aWrapCol, PRUint32 aFirstLineOffset,
                    PRBool aRespectNewlines,
                    nsAString & aOutString);
};

#endif 

