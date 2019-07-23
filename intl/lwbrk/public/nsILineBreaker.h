



































#ifndef nsILineBreaker_h__
#define nsILineBreaker_h__

#include "nsISupports.h"

#include "nscore.h"

#define NS_LINEBREAKER_NEED_MORE_TEXT -1


#define NS_ILINEBREAKER_IID \
{ 0x5ae68851, 0xd9a3, 0x49fd, \
    { 0x93, 0x88, 0x58, 0x58, 0x6d, 0xad, 0x80, 0x44 } }

class nsILineBreaker : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ILINEBREAKER_IID)
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
