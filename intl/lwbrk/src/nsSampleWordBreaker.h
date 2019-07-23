



































#ifndef nsSampleWordBreaker_h__
#define nsSampleWordBreaker_h__


#include "nsIWordBreaker.h"

typedef enum {
  kWbClassSpace = 0,
  kWbClassAlphaLetter,
  kWbClassPunct,
  kWbClassHanLetter,
  kWbClassKatakanaLetter,
  kWbClassHiraganaLetter,
  kWbClassHWKatakanaLetter,
  kWbClassThaiLetter
} wb_class;

class nsSampleWordBreaker : public nsIWordBreaker
{
  NS_DECL_ISUPPORTS
public:

  nsSampleWordBreaker() ;
  virtual ~nsSampleWordBreaker() ;

  PRBool BreakInBetween(const PRUnichar* aText1 , PRUint32 aTextLen1,
                        const PRUnichar* aText2 , PRUint32 aTextLen2);
  nsWordRange FindWord(const PRUnichar* aText1 , PRUint32 aTextLen1,
                       PRUint32 aOffset);

  PRInt32 NextWord(const PRUnichar* aText, PRUint32 aLen, PRUint32 aPos);

  PRInt32 PrevWord(const PRUnichar* aText, PRUint32 aLen, PRUint32 aPos);

protected:
  PRUint8  GetClass(PRUnichar aChar);
};

#endif  
