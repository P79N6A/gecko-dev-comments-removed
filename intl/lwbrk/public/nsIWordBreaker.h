



































#ifndef nsIWordBreaker_h__
#define nsIWordBreaker_h__

#include "nsISupports.h"

#include "nscore.h"

#define NS_WORDBREAKER_NEED_MORE_TEXT -1


#define NS_IWORDBREAKER_IID \
{ 0xe86b3379, 0xbf89, 0x11d2, \
   { 0xb3, 0xaf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }

typedef struct {
  PRUint32 mBegin;
  PRUint32 mEnd;
} nsWordRange;

class nsIWordBreaker : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IWORDBREAKER_IID)

  virtual PRBool BreakInBetween(const PRUnichar* aText1 , PRUint32 aTextLen1,
                                const PRUnichar* aText2 ,
                                PRUint32 aTextLen2) = 0;
  virtual nsWordRange FindWord(const PRUnichar* aText1 , PRUint32 aTextLen1,
                               PRUint32 aOffset) = 0;
  virtual PRInt32 NextWord(const PRUnichar* aText, PRUint32 aLen, 
                           PRUint32 aPos) = 0;
                           
  virtual PRInt32 PrevWord(const PRUnichar* aText, PRUint32 aLen, 
                           PRUint32 aPos) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIWordBreaker, NS_IWORDBREAKER_IID)

#endif  
