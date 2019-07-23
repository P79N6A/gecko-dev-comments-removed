





































#include "nsIPlatformCharset.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsCollation.h"
#include "nsCollationCID.h"
#include "nsUnicharUtilCIID.h"
#include "prmem.h"
#include "nsReadableUtils.h"



NS_DEFINE_CID(kCollationCID, NS_COLLATION_CID);

NS_IMPL_ISUPPORTS1(nsCollationFactory, nsICollationFactory)

nsresult nsCollationFactory::CreateCollation(nsILocale* locale, nsICollation** instancePtr)
{
  
  
  nsICollation *inst;
  nsresult res;
  
  res = CallCreateInstance(kCollationCID, &inst);
  if (NS_FAILED(res)) {
    return res;
  }

  inst->Initialize(locale);
  *instancePtr = inst;

  return res;
}



nsCollation::nsCollation()
{
  MOZ_COUNT_CTOR(nsCollation);
  nsresult res;
  mCaseConversion = do_GetService(NS_UNICHARUTIL_CONTRACTID, &res);
  NS_ASSERTION(NS_SUCCEEDED(res), "CreateInstance failed for kCaseConversionIID");
}

nsCollation::~nsCollation()
{
  MOZ_COUNT_DTOR(nsCollation);
}

nsresult nsCollation::NormalizeString(const nsAString& stringIn, nsAString& stringOut)
{
  if (!mCaseConversion) {
    stringOut = stringIn;
  }
  else {
    PRInt32 aLength = stringIn.Length();

    if (aLength <= 64) {
      PRUnichar conversionBuffer[64];
      mCaseConversion->ToLower(PromiseFlatString(stringIn).get(), conversionBuffer, aLength);
      stringOut.Assign(conversionBuffer, aLength);
    }
    else {
      PRUnichar* conversionBuffer;
      conversionBuffer = new PRUnichar[aLength];
      if (!conversionBuffer) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      mCaseConversion->ToLower(PromiseFlatString(stringIn).get(), conversionBuffer, aLength);
      stringOut.Assign(conversionBuffer, aLength);
      delete [] conversionBuffer;
    }
  }
  return NS_OK;
}

nsresult nsCollation::SetCharset(const char* aCharset)
{
  NS_ENSURE_ARG_POINTER(aCharset);

  nsresult rv;
  nsCOMPtr <nsICharsetConverterManager> charsetConverterManager = do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
  if (NS_SUCCEEDED(rv)) {
    rv = charsetConverterManager->GetUnicodeEncoder(aCharset,
                                                    getter_AddRefs(mEncoder));
  }
  return rv;
}

nsresult nsCollation::UnicodeToChar(const nsAString& aSrc, char** dst)
{
  NS_ENSURE_ARG_POINTER(dst);

  nsresult res = NS_OK;
  if (!mEncoder)
    res = SetCharset("ISO-8859-1");

  if (NS_SUCCEEDED(res)) {
    const nsPromiseFlatString& src = PromiseFlatString(aSrc);
    const PRUnichar *unichars = src.get();
    PRInt32 unicharLength = src.Length();
    PRInt32 dstLength;
    res = mEncoder->GetMaxLength(unichars, unicharLength, &dstLength);
    if (NS_SUCCEEDED(res)) {
      PRInt32 bufLength = dstLength + 1 + 32; 
      *dst = (char *) PR_Malloc(bufLength);
      if (*dst) {
        **dst = '\0';
        res = mEncoder->Convert(unichars, &unicharLength, *dst, &dstLength);

        if (NS_SUCCEEDED(res) || (NS_ERROR_UENC_NOMAPPING == res)) {
          
          
          PRInt32 finishLength = bufLength - dstLength; 
          if (finishLength > 0) {
            res = mEncoder->Finish((*dst + dstLength), &finishLength);
            if (NS_SUCCEEDED(res)) {
              (*dst)[dstLength + finishLength] = '\0';
            }
          }
        }
        if (NS_FAILED(res)) {
          PR_Free(*dst);
          *dst = nsnull;
        }
      }
      else {
        res = NS_ERROR_OUT_OF_MEMORY;
      }
    }
  }

  return res;
}



