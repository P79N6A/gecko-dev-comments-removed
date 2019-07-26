




#ifndef nsIUnicodeDecoder_h___
#define nsIUnicodeDecoder_h___

#include "nscore.h"
#include "nsISupports.h"






#define NS_IUNICODEDECODER_IID	\
	{ 0x25359602, 0xfc70, 0x4d13,	\
		{ 0xa9, 0xab, 0x80, 0x86, 0xd3, 0x82, 0x7c, 0xd }}


#define NS_UNICODEDECODER_CONTRACTID_BASE "@mozilla.org/intl/unicode/decoder;1?charset="







class nsIUnicodeDecoder : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IUNICODEDECODER_IID)

  enum {
    kOnError_Recover,       
    kOnError_Signal         
  };

  





















































  NS_IMETHOD Convert(const char * aSrc, int32_t * aSrcLength, 
      char16_t * aDest, int32_t * aDestLength) = 0;

  










  NS_IMETHOD GetMaxLength(const char * aSrc, int32_t aSrcLength, 
      int32_t * aDestLength) = 0;

  



  NS_IMETHOD Reset() = 0;

  






  virtual void SetInputErrorBehavior(int32_t aBehavior) = 0;

  


  virtual char16_t GetCharacterForUnMapped() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIUnicodeDecoder, NS_IUNICODEDECODER_IID)

#endif 
