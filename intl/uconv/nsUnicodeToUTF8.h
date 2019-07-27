




#ifndef nsUnicodeToUTF8_h___
#define nsUnicodeToUTF8_h___

#include "mozilla/Attributes.h"
#include "nsIUnicodeEncoder.h"



#define NS_UNICODETOUTF8_CID \
  { 0x7c657d18, 0xec5e, 0x11d2, {0x8a, 0xac, 0x0, 0x60, 0x8, 0x11, 0xa8, 0x36}}

#define NS_UNICODETOUTF8_CONTRACTID "@mozilla.org/intl/unicode/encoder;1?charset=UTF-8"













class nsUnicodeToUTF8 MOZ_FINAL : public nsIUnicodeEncoder
{
  ~nsUnicodeToUTF8() {}

  NS_DECL_ISUPPORTS

public:

  


  nsUnicodeToUTF8() {mHighSurrogate = 0;}

  NS_IMETHOD Convert(const char16_t * aSrc, 
                     int32_t * aSrcLength, 
                     char * aDest, 
                     int32_t * aDestLength) MOZ_OVERRIDE;

  NS_IMETHOD Finish(char * aDest, int32_t * aDestLength) MOZ_OVERRIDE;

  NS_IMETHOD GetMaxLength(const char16_t * aSrc, int32_t aSrcLength, 
      int32_t * aDestLength) MOZ_OVERRIDE;

  NS_IMETHOD Reset() MOZ_OVERRIDE {mHighSurrogate = 0; return NS_OK;}

  NS_IMETHOD SetOutputErrorBehavior(int32_t aBehavior, 
    nsIUnicharEncoder * aEncoder, char16_t aChar) MOZ_OVERRIDE {return NS_OK;}

protected:
  char16_t mHighSurrogate;

};

#endif 
