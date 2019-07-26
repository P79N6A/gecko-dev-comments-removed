



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

  bool BreakInBetween(const PRUnichar* aText1 , uint32_t aTextLen1,
                        const PRUnichar* aText2 , uint32_t aTextLen2);
  nsWordRange FindWord(const PRUnichar* aText1 , uint32_t aTextLen1,
                       uint32_t aOffset);

  int32_t NextWord(const PRUnichar* aText, uint32_t aLen, uint32_t aPos);

protected:
  uint8_t  GetClass(PRUnichar aChar);
};

#endif  
