




































#ifndef nsPangoLineBreaker_h__
#define nsPangoLineBreaker_h__

#include "nsILineBreaker.h"

class nsPangoLineBreaker : public nsILineBreaker
{
  NS_DECL_ISUPPORTS

public:
  PRBool BreakInBetween(const PRUnichar* aText1 , PRUint32 aTextLen1,
                        const PRUnichar* aText2 , PRUint32 aTextLen2);

  PRInt32 Next(const PRUnichar* aText, PRUint32 aLen, PRUint32 aPos);

  PRInt32 Prev(const PRUnichar* aText, PRUint32 aLen, PRUint32 aPos);

  void GetJISx4051Breaks(const PRUnichar* aText, PRUint32 aLen,
                         PRPackedBool* aBreakBefore);
};

#endif  
