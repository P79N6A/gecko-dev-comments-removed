



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

  virtual int32_t Next( const PRUnichar* aText, uint32_t aLen, 
                        uint32_t aPos) = 0;

  virtual int32_t Prev( const PRUnichar* aText, uint32_t aLen, 
                        uint32_t aPos) = 0;

  
  
  
  
  
  
  virtual void GetJISx4051Breaks(const PRUnichar* aText, uint32_t aLength,
                                 uint8_t aWordBreak,
                                 uint8_t* aBreakBefore) = 0;
  virtual void GetJISx4051Breaks(const uint8_t* aText, uint32_t aLength,
                                 uint8_t aWordBreak,
                                 uint8_t* aBreakBefore) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsILineBreaker, NS_ILINEBREAKER_IID)

static inline bool
NS_IsSpace(PRUnichar u)
{
  return u == 0x0020 ||                  
         u == 0x0009 ||                  
         u == 0x000D ||                  
         u == 0x1680 ||                  
         (0x2000 <= u && u <= 0x2006) || 
                                         
                                         
         (0x2008 <= u && u <= 0x200B) || 
                                         
         u == 0x205F;                    
}

static inline bool
NS_NeedsPlatformNativeHandling(PRUnichar aChar)
{
  return (0x0e01 <= aChar && aChar <= 0x0fff) || 
         (0x1780 <= aChar && aChar <= 0x17ff);   
}

#endif  
