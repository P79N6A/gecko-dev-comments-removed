





































#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsICharsetAlias.h"
#include "nsIServiceManager.h"
#include "nsICategoryManager.h"
#include "nsICharsetConverterManager.h"
#include "nsEncoderDecoderUtils.h"
#include "nsIStringBundle.h"
#include "nsUConvDll.h"
#include "prmem.h"
#include "nsCRT.h"
#include "nsTArray.h"
#include "nsStringEnumerator.h"
#include "nsThreadUtils.h"
#include "nsIProxyObjectManager.h"

#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"


#include "nsCharsetConverterManager.h"

#ifdef MOZ_USE_NATIVE_UCONV
#include "nsNativeUConvService.h"
#endif


#define NS_1BYTE_CODER_PATTERN "ISO-8859"
#define NS_1BYTE_CODER_PATTERN_LEN 8



NS_IMPL_THREADSAFE_ISUPPORTS1(nsCharsetConverterManager,
                              nsICharsetConverterManager)

nsCharsetConverterManager::nsCharsetConverterManager() 
  :mDataBundle(NULL), mTitleBundle(NULL)
{
#ifdef MOZ_USE_NATIVE_UCONV
  mNativeUC = do_GetService(NS_NATIVE_UCONV_SERVICE_CONTRACT_ID);
#endif
}

nsCharsetConverterManager::~nsCharsetConverterManager() 
{
  NS_IF_RELEASE(mDataBundle);
  NS_IF_RELEASE(mTitleBundle);
}

nsresult nsCharsetConverterManager::Init()
{
  if (!mDecoderHash.Init())
    return NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}

nsresult nsCharsetConverterManager::RegisterConverterManagerData()
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> catman = do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    return rv;

  RegisterConverterCategory(catman, NS_TITLE_BUNDLE_CATEGORY,
                            "chrome://global/locale/charsetTitles.properties");
  RegisterConverterCategory(catman, NS_DATA_BUNDLE_CATEGORY,
                            "resource://gre/res/charsetData.properties");

  return NS_OK;
}

nsresult
nsCharsetConverterManager::RegisterConverterCategory(nsICategoryManager* catman,
                                                     const char* aCategory,
                                                     const char* aURL)
{
  return catman->AddCategoryEntry(aCategory, aURL, "",
                                  PR_TRUE, PR_TRUE, nsnull);
}

nsresult nsCharsetConverterManager::LoadExtensibleBundle(
                                    const char* aCategory, 
                                    nsIStringBundle ** aResult)
{
  nsresult rv = NS_OK;

  nsCOMPtr<nsIStringBundleService> sbServ = 
           do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    return rv;

  return sbServ->CreateExtensibleBundle(aCategory, aResult);
}

nsresult nsCharsetConverterManager::GetBundleValue(nsIStringBundle * aBundle, 
                                                   const char * aName, 
                                                   const nsAFlatString& aProp, 
                                                   PRUnichar ** aResult)
{
  nsAutoString key; 

  key.AssignWithConversion(aName);
  ToLowerCase(key); 
  key.Append(aProp);

  return aBundle->GetStringFromName(key.get(), aResult);
}

nsresult nsCharsetConverterManager::GetBundleValue(nsIStringBundle * aBundle, 
                                                   const char * aName, 
                                                   const nsAFlatString& aProp, 
                                                   nsAString& aResult)
{
  nsresult rv = NS_OK;

  nsXPIDLString value;
  rv = GetBundleValue(aBundle, aName, aProp, getter_Copies(value));
  if (NS_FAILED(rv))
    return rv;

  aResult = value;

  return NS_OK;
}





NS_IMETHODIMP
nsCharsetConverterManager::GetUnicodeEncoder(const char * aDest, 
                                             nsIUnicodeEncoder ** aResult)
{
  
  nsCAutoString charset;
  
  
  nsCharsetConverterManager::GetCharsetAlias(aDest, charset);

  return nsCharsetConverterManager::GetUnicodeEncoderRaw(charset.get(),
                                                         aResult);
}


NS_IMETHODIMP
nsCharsetConverterManager::GetUnicodeEncoderRaw(const char * aDest, 
                                                nsIUnicodeEncoder ** aResult)
{
  *aResult= nsnull;
  nsCOMPtr<nsIUnicodeEncoder> encoder;

#ifdef MOZ_USE_NATIVE_UCONV
  if (mNativeUC) {
    nsCOMPtr<nsISupports> supports;
    mNativeUC->GetNativeConverter("UCS-2", 
                                  aDest,
                                  getter_AddRefs(supports));

    encoder = do_QueryInterface(supports);

    if (encoder) {
      NS_ADDREF(*aResult = encoder);
      return NS_OK;
    }
  }
#endif  
  nsresult rv = NS_OK;

  nsCAutoString
    contractid(NS_LITERAL_CSTRING(NS_UNICODEENCODER_CONTRACTID_BASE) +
               nsDependentCString(aDest));

  
  encoder = do_CreateInstance(contractid.get(), &rv);

  if (NS_FAILED(rv))
    rv = NS_ERROR_UCONV_NOCONV;
  else
  {
    *aResult = encoder.get();
    NS_ADDREF(*aResult);
  }
  return rv;
}

NS_IMETHODIMP
nsCharsetConverterManager::GetUnicodeDecoder(const char * aSrc, 
                                             nsIUnicodeDecoder ** aResult)
{
  
  nsCAutoString charset;
  
  
  nsCharsetConverterManager::GetCharsetAlias(aSrc, charset);

  return nsCharsetConverterManager::GetUnicodeDecoderRaw(charset.get(),
                                                         aResult);
}

NS_IMETHODIMP
nsCharsetConverterManager::GetUnicodeDecoderRaw(const char * aSrc, 
                                                nsIUnicodeDecoder ** aResult)
{
  *aResult= nsnull;
  nsCOMPtr<nsIUnicodeDecoder> decoder;

#ifdef MOZ_USE_NATIVE_UCONV
  if (mNativeUC) {
    nsCOMPtr<nsISupports> supports;
    mNativeUC->GetNativeConverter(aSrc,
                                  "UCS-2", 
                                  getter_AddRefs(supports));
    
    decoder = do_QueryInterface(supports);

    if (decoder) {
      NS_ADDREF(*aResult = decoder);
      return NS_OK;
    }
  }
#endif
  nsresult rv = NS_OK;

  NS_NAMED_LITERAL_CSTRING(contractbase, NS_UNICODEDECODER_CONTRACTID_BASE);
  nsDependentCString src(aSrc);
  
  if (!strncmp(aSrc, NS_1BYTE_CODER_PATTERN, NS_1BYTE_CODER_PATTERN_LEN))
  {
    
    
    if (!mDecoderHash.Get(aSrc, getter_AddRefs(decoder))) {
      decoder = do_GetService(PromiseFlatCString(contractbase + src).get(),
                              &rv);
      if (NS_SUCCEEDED(rv))
        mDecoderHash.Put(aSrc, decoder);
    }
  }
  else
  {
    decoder = do_CreateInstance(PromiseFlatCString(contractbase + src).get(),
                                &rv);
  }
  NS_ENSURE_SUCCESS(rv, NS_ERROR_UCONV_NOCONV);

  decoder.forget(aResult);
  return rv;
}

nsresult 
nsCharsetConverterManager::GetList(const nsACString& aCategory,
                                   const nsACString& aPrefix,
                                   nsIUTF8StringEnumerator** aResult)
{
  if (aResult == NULL) 
    return NS_ERROR_NULL_POINTER;
  *aResult = NULL;

  nsresult rv;
  nsCAutoString alias;

  nsCOMPtr<nsICategoryManager> catman = do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    return rv;

  nsTArray<nsCString>* array = new nsTArray<nsCString>;
  if (!array)
    return NS_ERROR_OUT_OF_MEMORY;
  
  nsCOMPtr<nsISimpleEnumerator> enumerator;
  catman->EnumerateCategory(PromiseFlatCString(aCategory).get(), 
                            getter_AddRefs(enumerator));

  PRBool hasMore;
  while (NS_SUCCEEDED(enumerator->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsISupports> supports;
    if (NS_FAILED(enumerator->GetNext(getter_AddRefs(supports))))
      continue;
    
    nsCOMPtr<nsISupportsCString> supStr = do_QueryInterface(supports);
    if (!supStr)
      continue;

    nsCAutoString fullName(aPrefix);
    
    nsCAutoString name;
    if (NS_FAILED(supStr->GetData(name)))
      continue;

    fullName += name;
    rv = GetCharsetAlias(fullName.get(), alias);
    if (NS_FAILED(rv)) 
      continue;

    rv = array->AppendElement(alias) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
  }
    
  return NS_NewAdoptingUTF8StringEnumerator(aResult, array);
}


NS_IMETHODIMP
nsCharsetConverterManager::GetDecoderList(nsIUTF8StringEnumerator ** aResult)
{
  return GetList(NS_LITERAL_CSTRING(NS_UNICODEDECODER_NAME),
                 EmptyCString(), aResult);
}

NS_IMETHODIMP
nsCharsetConverterManager::GetEncoderList(nsIUTF8StringEnumerator ** aResult)
{
  return GetList(NS_LITERAL_CSTRING(NS_UNICODEENCODER_NAME),
                 EmptyCString(), aResult);
}

NS_IMETHODIMP
nsCharsetConverterManager::GetCharsetDetectorList(nsIUTF8StringEnumerator** aResult)
{
  return GetList(NS_LITERAL_CSTRING("charset-detectors"),
                 NS_LITERAL_CSTRING("chardet."), aResult);
}





NS_IMETHODIMP
nsCharsetConverterManager::GetCharsetAlias(const char * aCharset, 
                                           nsACString& aResult)
{
  NS_PRECONDITION(aCharset, "null param");
  if (!aCharset)
    return NS_ERROR_NULL_POINTER;

  
  if (!NS_IsMainThread()) {
    nsCOMPtr<nsICharsetConverterManager> self;
    nsresult rv =
    NS_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                         NS_GET_IID(nsICharsetConverterManager),
                         this, NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                         getter_AddRefs(self));
    NS_ENSURE_SUCCESS(rv, rv);
    return self->GetCharsetAlias(aCharset, aResult);
  }

  
  
  nsDependentCString charset(aCharset);
  nsCOMPtr<nsICharsetAlias> csAlias(do_GetService(NS_CHARSETALIAS_CONTRACTID));
  NS_ASSERTION(csAlias, "failed to get the CharsetAlias service");
  if (csAlias) {
    nsAutoString pref;
    nsresult rv = csAlias->GetPreferred(charset, aResult);
    if (NS_SUCCEEDED(rv)) {
      return (!aResult.IsEmpty()) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
    }
  }

  aResult = charset;
  return NS_OK;
}


NS_IMETHODIMP
nsCharsetConverterManager::GetCharsetTitle(const char * aCharset, 
                                           nsAString& aResult)
{
  if (aCharset == NULL) return NS_ERROR_NULL_POINTER;

  if (mTitleBundle == NULL) {
    nsresult rv = LoadExtensibleBundle(NS_TITLE_BUNDLE_CATEGORY, &mTitleBundle);
    if (NS_FAILED(rv))
      return rv;
  }

  return GetBundleValue(mTitleBundle, aCharset, NS_LITERAL_STRING(".title"), aResult);
}

NS_IMETHODIMP
nsCharsetConverterManager::GetCharsetData(const char * aCharset, 
                                          const PRUnichar * aProp,
                                          nsAString& aResult)
{
  if (aCharset == NULL)
    return NS_ERROR_NULL_POINTER;
  

  if (mDataBundle == NULL) {
    nsresult rv = LoadExtensibleBundle(NS_DATA_BUNDLE_CATEGORY, &mDataBundle);
    if (NS_FAILED(rv))
      return rv;
  }

  return GetBundleValue(mDataBundle, aCharset, nsDependentString(aProp), aResult);
}

NS_IMETHODIMP
nsCharsetConverterManager::GetCharsetLangGroup(const char * aCharset, 
                                               nsIAtom** aResult)
{
  
  nsCAutoString charset;

  nsresult rv = GetCharsetAlias(aCharset, charset);
  if (NS_FAILED(rv))
    return rv;

  
  return nsCharsetConverterManager::GetCharsetLangGroupRaw(charset.get(),
                                                           aResult);
}

NS_IMETHODIMP
nsCharsetConverterManager::GetCharsetLangGroupRaw(const char * aCharset, 
                                                  nsIAtom** aResult)
{

  *aResult = nsnull;
  if (aCharset == NULL)
    return NS_ERROR_NULL_POINTER;

  nsresult rv = NS_OK;

  if (mDataBundle == NULL) {
    rv = LoadExtensibleBundle(NS_DATA_BUNDLE_CATEGORY, &mDataBundle);
    if (NS_FAILED(rv))
      return rv;
  }

  nsAutoString langGroup;
  rv = GetBundleValue(mDataBundle, aCharset, NS_LITERAL_STRING(".LangGroup"), langGroup);

  if (NS_SUCCEEDED(rv))
    *aResult = NS_NewAtom(langGroup);

  return rv;
}
