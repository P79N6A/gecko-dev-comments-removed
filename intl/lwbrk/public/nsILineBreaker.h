



#ifndef nsILineBreaker_h__
#define nsILineBreaker_h__

#include "nsISupports.h"

#include "nscore.h"

#define NS_LINEBREAKER_NEED_MORE_TEXT -1


#define NS_ILINEBREAKER_IID \
{0x4b0b9e04, 0x6ffb, 0x4647, \
    {0xaa, 0x5f, 0x2f, 0xa2, 0xeb, 0xd8, 0x83, 0xe8}}

class nsILineBreaker : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ILINEBREAKER_IID)

  enum {
    kWordBreak_Normal   = 0, 
    kWordBreak_BreakAll = 1, 
    kWordBreak_KeepAll  = 2  
  };

  virtual PRInt32 Next( const PRUnichar* aText, PRUint32 aLen, 
                        PRUint32 aPos) = 0;

  virtual PRInt32 Prev( const PRUnichar* aText, PRUint32 aLen, 
                        PRUint32 aPos) = 0;

  
  
  
  
  
  
  virtual void GetJISx4051Breaks(const PRUnichar* aText, PRUint32 aLength,
                                 PRUint8 aWordBreak,
                                 PRUint8* aBreakBefore) = 0;
  virtual void GetJISx4051Breaks(const PRUint8* aText, PRUint32 aLength,
                                 PRUint8 aWordBreak,
                                 PRUint8* aBreakBefore) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsILineBreaker, NS_ILINEBREAKER_IID)

static inline bool
NS_IsSpace(PRUnichar u)
{
  return u == 0x0020 ||                  
         u == 0x0009 ||                  
         u == 0x000D ||                  
         (0x2000 <= u && u <= 0x2006) || 
                                         
                                         
         (0x2008 <= u && u <= 0x200B);   
                                         
}

static inline bool
NS_NeedsPlatformNativeHandling(PRUnichar aChar)
{
  return (0x0e01 <= aChar && aChar <= 0x0fff); 
}

#endif  
