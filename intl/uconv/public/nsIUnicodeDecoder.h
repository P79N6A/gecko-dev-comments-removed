




































#ifndef nsIUnicodeDecoder_h___
#define nsIUnicodeDecoder_h___

#include "nscore.h"
#include "nsISupports.h"






#define NS_IUNICODEDECODER_IID	\
	{ 0x25359602, 0xfc70, 0x4d13,	\
		{ 0xa9, 0xab, 0x80, 0x86, 0xd3, 0x82, 0x7c, 0xd }}


 
#define NS_EXACT_LENGTH \
  NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_UCONV, 11)

#define NS_PARTIAL_MORE_INPUT \
  NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_UCONV, 12)

#define NS_PARTIAL_MORE_OUTPUT \
  NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_UCONV, 13)

#define NS_ERROR_ILLEGAL_INPUT \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_UCONV, 14)
 


#define NS_OK_UDEC_EXACTLENGTH      \
  NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_UCONV, 11)

#define NS_OK_UDEC_MOREINPUT        \
  NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_UCONV, 12)

#define NS_OK_UDEC_MOREOUTPUT       \
  NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_UCONV, 13)

#define NS_ERROR_UDEC_ILLEGALINPUT  \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_UCONV, 14)

#define NS_OK_UDEC_NOBOMFOUND       \
  NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_UCONV, 14)


#define NS_UNICODEDECODER_CONTRACTID_BASE "@mozilla.org/intl/unicode/decoder;1?charset="







class nsIUnicodeDecoder : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IUNICODEDECODER_IID)

  enum {
    kOnError_Recover,       
    kOnError_Signal         
  };

  





















































  NS_IMETHOD Convert(const char * aSrc, PRInt32 * aSrcLength, 
      PRUnichar * aDest, PRInt32 * aDestLength) = 0;

  










  NS_IMETHOD GetMaxLength(const char * aSrc, PRInt32 aSrcLength, 
      PRInt32 * aDestLength) = 0;

  



  NS_IMETHOD Reset() = 0;

  






  virtual void SetInputErrorBehavior(PRInt32 aBehavior) = 0;

  


  virtual PRUnichar GetCharacterForUnMapped() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIUnicodeDecoder, NS_IUNICODEDECODER_IID)

#endif 
