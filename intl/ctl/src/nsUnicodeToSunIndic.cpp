







































#include "nsCOMPtr.h"
#include "nsCtlCIID.h"
#include "nsILE.h"
#include "nsULE.h"
#include "nsUnicodeToSunIndic.h"

NS_IMPL_ISUPPORTS2(nsUnicodeToSunIndic, nsIUnicodeEncoder, nsICharRepresentable)

NS_IMETHODIMP
nsUnicodeToSunIndic::SetOutputErrorBehavior(PRInt32           aBehavior,
                                            nsIUnicharEncoder *aEncoder,
                                            PRUnichar         aChar)
{
  if (aBehavior == kOnError_CallBack && aEncoder == nsnull)
     return NS_ERROR_NULL_POINTER;
  mErrEncoder = aEncoder;
  mErrBehavior = aBehavior;
  mErrChar = aChar;
  return NS_OK;
}


nsUnicodeToSunIndic::nsUnicodeToSunIndic()
{
  static   NS_DEFINE_CID(kLECID, NS_ULE_CID);
  nsresult rv;

  mCtlObj = do_CreateInstance(kLECID, &rv);
  if (NS_FAILED(rv)) {

#ifdef DEBUG_prabhath
    
    
    printf("ERROR: Cannot create instance of component " NS_ULE_PROGID " [%x].\n", rv);
#endif

    NS_WARNING("Indian Text Shaping Will Not Be Supported\n");
    mCtlObj = nsnull;
  }
}

nsUnicodeToSunIndic::~nsUnicodeToSunIndic()
{
  
  
}






NS_IMETHODIMP nsUnicodeToSunIndic::Convert(const PRUnichar* input,
                                           PRInt32*         aSrcLength,
                                           char*            output,
                                           PRInt32*         aDestLength)
{
  PRSize outLen;

  if (mCtlObj == nsnull) {
#ifdef DEBUG_prabhath
  printf("Debug/Test Case of No Hindi pango shaper Object\n");
  
  printf("ERROR: No Hindi Text Layout Implementation");
#endif

    NS_WARNING("cannot get default converter for Hindi");
    return NS_ERROR_FAILURE;
  }

  mCharOff = mByteOff = 0;
  mCtlObj->GetPresentationForm(input, *aSrcLength, "sun.unicode.india-0",
                               &output[mByteOff], &outLen, PR_TRUE);
  *aDestLength = outLen;
  return NS_OK;
}

NS_IMETHODIMP nsUnicodeToSunIndic::Finish(char *output, PRInt32 *aDestLength)
{
  
  
  mByteOff = mCharOff = 0;
  return NS_OK;
}


NS_IMETHODIMP nsUnicodeToSunIndic::Reset()
{
  mByteOff = mCharOff = 0;
  return NS_OK;
}


NS_IMETHODIMP nsUnicodeToSunIndic::GetMaxLength(const PRUnichar * aSrc,
                                                PRInt32 aSrcLength,
                                                PRInt32 * aDestLength)
{
  *aDestLength = (aSrcLength + 1) *  2; 
                                        
  return NS_OK;
}


NS_IMETHODIMP nsUnicodeToSunIndic::FillInfo(PRUint32* aInfo)
{
  PRUint16 i;

  
  for (i = 0;i <= 0x7f; i++)
    SET_REPRESENTABLE(aInfo, i);

  
  for (i = 0x0901; i <= 0x0903; i++)
    SET_REPRESENTABLE(aInfo, i);

  for (i = 0x0905; i <= 0x0939; i++)
    SET_REPRESENTABLE(aInfo, i);

  for (i = 0x093c; i <= 0x094d; i++)
    SET_REPRESENTABLE(aInfo, i);

  for (i = 0x0950; i <= 0x0954; i++)
    SET_REPRESENTABLE(aInfo, i);

  for (i = 0x0958; i <= 0x0970; i++)
    SET_REPRESENTABLE(aInfo, i);
 
  
  return NS_OK;
}
