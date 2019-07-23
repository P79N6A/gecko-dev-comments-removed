



































#ifndef nsILineBreaker_h__
#define nsILineBreaker_h__

#include "nsISupports.h"

#include "nscore.h"

#define NS_LINEBREAKER_NEED_MORE_TEXT -1


#define NS_ILINEBREAKER_IID \
{ 0xc3d9f25f, 0x7cea, 0x4a76, \
    { 0xa0, 0x8f, 0x05, 0xc4, 0x31, 0x35, 0x34, 0x48 } }

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
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsILineBreaker, NS_ILINEBREAKER_IID)

#endif  
