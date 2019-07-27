




#ifndef nsUTF8ToUnicode_h___
#define nsUTF8ToUnicode_h___

#include "nsUCSupport.h"



#define NS_UTF8TOUNICODE_CID \
  { 0x5534ddc0, 0xdd96, 0x11d2, {0x8a, 0xac, 0x0, 0x60, 0x8, 0x11, 0xa8, 0x36}}

#define NS_UTF8TOUNICODE_CONTRACTID "@mozilla.org/intl/unicode/decoder;1?charset=UTF-8"
















class nsUTF8ToUnicode : public nsBasicDecoderSupport
{
public:

  


  nsUTF8ToUnicode();

protected:

  uint32_t mUcs4; 
  uint8_t mState; 
  uint8_t mBytes;
  bool mFirst;

  
  

  MOZ_WARN_UNUSED_RESULT NS_IMETHOD GetMaxLength(const char * aSrc,
                                                 int32_t aSrcLength,
                                                 int32_t * aDestLength) override;

  
  

  NS_IMETHOD Convert(const char * aSrc, int32_t * aSrcLength, 
      char16_t * aDest, int32_t * aDestLength);

  
  

  NS_IMETHOD Reset();

};

#endif 

