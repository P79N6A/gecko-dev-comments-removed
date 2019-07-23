




































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

#define NS_OK_UENC_EXACTLENGTH      \
  NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_UCONV, 0x21)

#define NS_OK_UENC_MOREOUTPUT       \
  NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_UCONV, 0x22)

#define NS_ERROR_UENC_NOMAPPING     \
  NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_UCONV, 0x23)

#define NS_OK_UENC_MOREINPUT       \
  NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_UCONV, 0x24)


#define NS_UNICODEENCODER_CONTRACTID_BASE "@mozilla.org/intl/unicode/encoder;1?charset="








class nsIUnicharEncoder : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IUNICHARENCODER_IID)

  


  NS_IMETHOD Convert(PRUnichar aChar, char * aDest, PRInt32 * aDestLength) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIUnicharEncoder, NS_IUNICHARENCODER_IID)













#define ENCODER_BUFFER_ALLOC_IF_NEEDED(p,e,s,l,sb,sbl,al) \
  PR_BEGIN_MACRO                                          \
    if (e                                                 \
        && NS_SUCCEEDED((e)->GetMaxLength((s), (l), &(al)))\
        && ((al) > (PRInt32)(sbl))                        \
        && (nsnull!=((p)=(char*)nsMemory::Alloc((al)+1))) \
        ) {                                               \
    }                                                     \
    else {                                                \
      (p) = (char*)(sb);                                  \
      (al) = (sbl);                                       \
    }                                                     \
  PR_END_MACRO 




#define ENCODER_BUFFER_FREE_IF_NEEDED(p,sb) \
  PR_BEGIN_MACRO                            \
    if ((p) != (char*)(sb))                 \
      nsMemory::Free(p);                    \
  PR_END_MACRO 







class nsIUnicodeEncoder : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IUNICODEENCODER_IID)

  enum {
    kOnError_Signal,        
    kOnError_CallBack,      
    kOnError_Replace       
  };

  


































  NS_IMETHOD Convert(const PRUnichar * aSrc, PRInt32 * aSrcLength, 
      char * aDest, PRInt32 * aDestLength) = 0;

  









  NS_IMETHOD Finish(char * aDest, PRInt32 * aDestLength) = 0;

  










  NS_IMETHOD GetMaxLength(const PRUnichar * aSrc, PRInt32 aSrcLength, 
      PRInt32 * aDestLength) = 0;

  



  NS_IMETHOD Reset() = 0;

  




  NS_IMETHOD SetOutputErrorBehavior(PRInt32 aBehavior, 
      nsIUnicharEncoder * aEncoder, PRUnichar aChar) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIUnicodeEncoder, NS_IUNICODEENCODER_IID)

#endif 
