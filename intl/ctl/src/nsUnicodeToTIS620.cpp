






































#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsICharsetConverterManager.h"
#include "nsILanguageAtomService.h"
#include "nsCtlCIID.h"
#include "nsILE.h"
#include "nsULE.h"
#include "nsUnicodeToTIS620.h"

static NS_DEFINE_CID(kCharSetManagerCID, NS_ICHARSETCONVERTERMANAGER_CID);

NS_IMPL_ISUPPORTS2(nsUnicodeToTIS620, nsIUnicodeEncoder, nsICharRepresentable)

NS_IMETHODIMP nsUnicodeToTIS620::SetOutputErrorBehavior(PRInt32 aBehavior,
                                                        nsIUnicharEncoder * aEncoder, 
                                                        PRUnichar aChar)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsUnicodeToTIS620::nsUnicodeToTIS620()
{
  static   NS_DEFINE_CID(kLECID, NS_ULE_CID);
  nsresult rv;

  mCtlObj = do_CreateInstance(kLECID, &rv);
  if (NS_FAILED(rv)) {
#ifdef DEBUG_prabhath
    
    
    printf("ERROR: Cannot create instance of component " NS_ULE_PROGID " [%x].\n", rv);
#endif
    NS_WARNING("Thai Text Layout Will Not Be Supported\n");
    mCtlObj = nsnull;
  }
}

nsUnicodeToTIS620::~nsUnicodeToTIS620()
{
  
  
}






NS_IMETHODIMP nsUnicodeToTIS620::Convert(const PRUnichar* input,
                                         PRInt32*         aSrcLength,
                                         char*            output,
                                         PRInt32*         aDestLength)
{
  PRSize outLen = 0;
#ifdef DEBUG_prabhath_no_shaper
  printf("Debug/Test Case of No thai pango shaper Object\n");
  
#endif

  if (mCtlObj == nsnull) {
#ifdef DEBUG_prabhath
    printf("ERROR: No CTL IMPLEMENTATION - Default Thai Conversion");
    
    
#endif

    nsCOMPtr<nsICharsetConverterManager> charsetMgr =
        do_GetService(kCharSetManagerCID);
    if (!charsetMgr)
      return NS_ERROR_FAILURE;
    
    nsCOMPtr<nsIUnicodeEncoder> encoder;
    charsetMgr->GetUnicodeEncoderRaw("TIS-620", getter_AddRefs(encoder));
    if (!encoder) {
      NS_WARNING("cannot get default converter for tis-620");
      return NS_ERROR_FAILURE;
    }

    encoder->Convert(input, aSrcLength, output, aDestLength);
    return NS_OK;
  }

  
  
  mCharOff = mByteOff = 0;

  
  

  
  
  
  mCtlObj->GetPresentationForm(input, *aSrcLength, "tis620-2",
                               &output[mByteOff], &outLen);

  *aDestLength = outLen;
  return NS_OK;
}

NS_IMETHODIMP nsUnicodeToTIS620::Finish(char * output, PRInt32 * aDestLength)
{
  
  
  mByteOff = mCharOff = 0;
  return NS_OK;
}


NS_IMETHODIMP nsUnicodeToTIS620::Reset()
{
  mByteOff = mCharOff = 0;
  return NS_OK;
}


NS_IMETHODIMP nsUnicodeToTIS620::GetMaxLength(const PRUnichar * aSrc,
                                              PRInt32 aSrcLength,
                                              PRInt32 * aDestLength)
{
  *aDestLength = (aSrcLength + 1) * 2; 
                                        
  return NS_OK;
}


NS_IMETHODIMP nsUnicodeToTIS620::FillInfo(PRUint32* aInfo)
{
  PRUint16 i;

  
  for (i = 0;i <= 0x7f; i++)
    SET_REPRESENTABLE(aInfo, i);

  
  for (i = 0x0e01; i <= 0xe3a; i++)
    SET_REPRESENTABLE(aInfo, i);

  
  
  for (i = 0x0e3f; i <= 0x0e5b; i++)
    SET_REPRESENTABLE(aInfo, i);

  
  return NS_OK;
}
