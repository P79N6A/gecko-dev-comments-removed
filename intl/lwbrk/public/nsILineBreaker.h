



































#ifndef nsILineBreaker_h__
#define nsILineBreaker_h__

#include "nsISupports.h"

#include "nscore.h"

#define NS_LINEBREAKER_NEED_MORE_TEXT -1


#define NS_ILINEBREAKER_IID \
{ 0xc9c5938e, 0x70ef, 0x4db2, \
    { 0xad, 0xee, 0xe7, 0xb2, 0xcc, 0xfb, 0xbe, 0xe6 } }

class nsILineBreaker : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ILINEBREAKER_IID)
  virtual PRBool BreakInBetween( const PRUnichar* aText1 , PRUint32 aTextLen1,
                                 const PRUnichar* aText2 , 
                                 PRUint32 aTextLen2) = 0;

  virtual PRInt32 Next( const PRUnichar* aText, PRUint32 aLen, 
                        PRUint32 aPos) = 0;

  virtual PRInt32 Prev( const PRUnichar* aText, PRUint32 aLen, 
                        PRUint32 aPos) = 0;

  
  
  
  
  
  
  virtual void GetJISx4051Breaks(const PRUnichar* aText, PRUint32 aLength,
                                 PRPackedBool* aBreakBefore) = 0;
  virtual void GetJISx4051Breaks(const PRUint8* aText, PRUint32 aLength,
                                 PRPackedBool* aBreakBefore) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsILineBreaker, NS_ILINEBREAKER_IID)

#endif  
