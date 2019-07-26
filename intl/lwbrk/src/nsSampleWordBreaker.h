



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

  bool BreakInBetween(const char16_t* aText1 , uint32_t aTextLen1,
                        const char16_t* aText2 , uint32_t aTextLen2);
  nsWordRange FindWord(const char16_t* aText1 , uint32_t aTextLen1,
                       uint32_t aOffset);

  int32_t NextWord(const char16_t* aText, uint32_t aLen, uint32_t aPos);

protected:
  uint8_t  GetClass(char16_t aChar);

  virtual ~nsSampleWordBreaker();
};

#endif  
