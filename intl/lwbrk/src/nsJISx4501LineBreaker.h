



































#ifndef nsJISx4501LineBreaker_h__
#define nsJISx4501LineBreaker_h__


#include "nsILineBreaker.h"

class nsJISx4051LineBreaker : public nsILineBreaker
{
  NS_DECL_ISUPPORTS

public:
  nsJISx4051LineBreaker();
  virtual ~nsJISx4051LineBreaker();

  PRBool BreakInBetween( const PRUnichar* aText1 , PRUint32 aTextLen1,
                         const PRUnichar* aText2 , PRUint32 aTextLen2);

  PRInt32 Next( const PRUnichar* aText, PRUint32 aLen, PRUint32 aPos);

  PRInt32 Prev( const PRUnichar* aText, PRUint32 aLen, PRUint32 aPos);

  virtual void GetJISx4051Breaks(const PRUnichar* aText, PRUint32 aLength,
                                 PRPackedBool* aBreakBefore);
};

#endif  
