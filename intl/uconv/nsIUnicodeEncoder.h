




#ifndef nsIUnicodeEncoder_h___
#define nsIUnicodeEncoder_h___

#include "nscore.h"
#include "nsError.h"
#include "nsISupports.h"



#define NS_IUNICODEENCODER_IID \
	{ 0x2b2ca3d0, 0xa4c9, 0x11d2, \
		{ 0x8a, 0xa1, 0x0, 0x60, 0x8, 0x11, 0xa8, 0x36 }}  



#define NS_IUNICHARENCODER_IID	\
	{ 0x299bccd0, 0xc6df, 0x11d2, \
		{0x8a, 0xa8, 0x0, 0x60, 0x8, 0x11, 0xa8, 0x36 }}


#define NS_UNICODEENCODER_CONTRACTID_BASE "@mozilla.org/intl/unicode/encoder;1?charset="








class nsIUnicharEncoder : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IUNICHARENCODER_IID)

  


  NS_IMETHOD Convert(char16_t aChar, char * aDest, int32_t * aDestLength) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIUnicharEncoder, NS_IUNICHARENCODER_IID)







class nsIUnicodeEncoder : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IUNICODEENCODER_IID)

  enum {
    kOnError_Signal,        
    kOnError_CallBack,      
    kOnError_Replace       
  };

  


































  NS_IMETHOD Convert(const char16_t * aSrc, int32_t * aSrcLength, 
      char * aDest, int32_t * aDestLength) = 0;

  









  NS_IMETHOD Finish(char * aDest, int32_t * aDestLength) = 0;

  











  MOZ_WARN_UNUSED_RESULT NS_IMETHOD GetMaxLength(const char16_t* aSrc,
                                                 int32_t aSrcLength,
                                                 int32_t* aDestLength) = 0;

  



  NS_IMETHOD Reset() = 0;

  




  NS_IMETHOD SetOutputErrorBehavior(int32_t aBehavior, 
      nsIUnicharEncoder * aEncoder, char16_t aChar) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIUnicodeEncoder, NS_IUNICODEENCODER_IID)

#endif 
