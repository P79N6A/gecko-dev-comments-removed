




#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsUnicharUtils.h"
#include "nsCharsetAlias.h"
#include "nsICategoryManager.h"
#include "nsICharsetConverterManager.h"
#include "nsEncoderDecoderUtils.h"
#include "nsIStringBundle.h"
#include "nsTArray.h"
#include "nsStringEnumerator.h"
#include "mozilla/Services.h"

#include "nsComponentManagerUtils.h"
#include "nsISupportsPrimitives.h"
#include "nsServiceManagerUtils.h"


#include "nsCharsetConverterManager.h"

static nsIStringBundle * sDataBundle;
static nsIStringBundle * sTitleBundle;



NS_IMPL_ISUPPORTS1(nsCharsetConverterManager, nsICharsetConverterManager)

nsCharsetConverterManager::nsCharsetConverterManager() 
{
}

nsCharsetConverterManager::~nsCharsetConverterManager() 
{
}


void nsCharsetConverterManager::Shutdown()
{
  NS_IF_RELEASE(sDataBundle);
  NS_IF_RELEASE(sTitleBundle);
}

static
nsresult LoadExtensibleBundle(const char* aCategory, 
                              nsIStringBundle ** aResult)
{
  nsCOMPtr<nsIStringBundleService> sbServ =
    mozilla::services::GetStringBundleService();
  if (!sbServ)
    return NS_ERROR_FAILURE;

  return sbServ->CreateExtensibleBundle(aCategory, aResult);
}

static
nsresult GetBundleValue(nsIStringBundle * aBundle, 
                        const char * aName, 
                        const nsAFlatString& aProp, 
                        char16_t ** aResult)
{
  nsAutoString key; 

  key.AssignWithConversion(aName);
  ToLowerCase(key); 
  key.Append(aProp);

  return aBundle->GetStringFromName(key.get(), aResult);
}

static
nsresult GetBundleValue(nsIStringBundle * aBundle, 
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

static
nsresult GetCharsetDataImpl(const char * aCharset, const char16_t * aProp,
                            nsAString& aResult)
{
  NS_ENSURE_ARG_POINTER(aCharset);
  

  if (!sDataBundle) {
    nsresult rv = LoadExtensibleBundle(NS_DATA_BUNDLE_CATEGORY, &sDataBundle);
    if (NS_FAILED(rv))
      return rv;
  }

  return GetBundleValue(sDataBundle, aCharset, nsDependentString(aProp), aResult);
}


bool nsCharsetConverterManager::IsInternal(const nsACString& aCharset)
{
  nsAutoString str;
  
  nsresult rv = GetCharsetDataImpl(PromiseFlatCString(aCharset).get(),
                                   MOZ_UTF16(".isInternal"),
                                   str);

  return NS_SUCCEEDED(rv);
}





NS_IMETHODIMP
nsCharsetConverterManager::GetUnicodeEncoder(const char * aDest, 
                                             nsIUnicodeEncoder ** aResult)
{
  
  nsAutoCString charset;
  
  
  nsCharsetConverterManager::GetCharsetAlias(aDest, charset);

  return nsCharsetConverterManager::GetUnicodeEncoderRaw(charset.get(),
                                                         aResult);
}


NS_IMETHODIMP
nsCharsetConverterManager::GetUnicodeEncoderRaw(const char * aDest, 
                                                nsIUnicodeEncoder ** aResult)
{
  *aResult= nullptr;
  nsCOMPtr<nsIUnicodeEncoder> encoder;

  nsresult rv = NS_OK;

  nsAutoCString
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
  
  nsAutoCString charset;

  
  if (NS_FAILED(nsCharsetConverterManager::GetCharsetAlias(aSrc, charset)))
    return NS_ERROR_UCONV_NOCONV;

  return nsCharsetConverterManager::GetUnicodeDecoderRaw(charset.get(),
                                                         aResult);
}

NS_IMETHODIMP
nsCharsetConverterManager::GetUnicodeDecoderInternal(const char * aSrc, 
                                                     nsIUnicodeDecoder ** aResult)
{
  
  nsAutoCString charset;

  nsresult rv = nsCharsetAlias::GetPreferredInternal(nsDependentCString(aSrc),
                                                     charset);
  NS_ENSURE_SUCCESS(rv, rv);

  return nsCharsetConverterManager::GetUnicodeDecoderRaw(charset.get(),
                                                         aResult);
}

NS_IMETHODIMP
nsCharsetConverterManager::GetUnicodeDecoderRaw(const char * aSrc, 
                                                nsIUnicodeDecoder ** aResult)
{
  *aResult= nullptr;
  nsCOMPtr<nsIUnicodeDecoder> decoder;

  nsresult rv = NS_OK;

  NS_NAMED_LITERAL_CSTRING(contractbase, NS_UNICODEDECODER_CONTRACTID_BASE);
  nsDependentCString src(aSrc);

  decoder = do_CreateInstance(PromiseFlatCString(contractbase + src).get(),
                              &rv);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_UCONV_NOCONV);

  decoder.forget(aResult);
  return rv;
}

static
nsresult GetList(const nsACString& aCategory,
                 const nsACString& aPrefix,
                 nsIUTF8StringEnumerator** aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = nullptr;

  nsresult rv;

  nsCOMPtr<nsICategoryManager> catman = do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    return rv;

  nsTArray<nsCString>* array = new nsTArray<nsCString>;
  if (!array)
    return NS_ERROR_OUT_OF_MEMORY;
  
  nsCOMPtr<nsISimpleEnumerator> enumerator;
  catman->EnumerateCategory(PromiseFlatCString(aCategory).get(), 
                            getter_AddRefs(enumerator));

  bool hasMore;
  while (NS_SUCCEEDED(enumerator->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsISupports> supports;
    if (NS_FAILED(enumerator->GetNext(getter_AddRefs(supports))))
      continue;
    
    nsCOMPtr<nsISupportsCString> supStr = do_QueryInterface(supports);
    if (!supStr)
      continue;

    nsAutoCString name;
    if (NS_FAILED(supStr->GetData(name)))
      continue;

    nsAutoCString fullName(aPrefix);
    fullName.Append(name);
    NS_ENSURE_TRUE(array->AppendElement(fullName), NS_ERROR_OUT_OF_MEMORY);
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
  NS_ENSURE_ARG_POINTER(aCharset);

  
  
  nsresult rv;

  rv = nsCharsetAlias::GetPreferred(nsDependentCString(aCharset), aResult);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


NS_IMETHODIMP
nsCharsetConverterManager::GetCharsetTitle(const char * aCharset, 
                                           nsAString& aResult)
{
  NS_ENSURE_ARG_POINTER(aCharset);

  if (!sTitleBundle) {
    nsresult rv = LoadExtensibleBundle(NS_TITLE_BUNDLE_CATEGORY, &sTitleBundle);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return GetBundleValue(sTitleBundle, aCharset, NS_LITERAL_STRING(".title"), aResult);
}

NS_IMETHODIMP
nsCharsetConverterManager::GetCharsetData(const char * aCharset, 
                                          const char16_t * aProp,
                                          nsAString& aResult)
{
  return GetCharsetDataImpl(aCharset, aProp, aResult);
}

NS_IMETHODIMP
nsCharsetConverterManager::GetCharsetLangGroup(const char * aCharset, 
                                               nsIAtom** aResult)
{
  
  nsAutoCString charset;

  nsresult rv = GetCharsetAlias(aCharset, charset);
  NS_ENSURE_SUCCESS(rv, rv);

  
  return nsCharsetConverterManager::GetCharsetLangGroupRaw(charset.get(),
                                                           aResult);
}

NS_IMETHODIMP
nsCharsetConverterManager::GetCharsetLangGroupRaw(const char * aCharset, 
                                                  nsIAtom** aResult)
{

  *aResult = nullptr;
  nsAutoString langGroup;
  
  nsresult rv = nsCharsetConverterManager::GetCharsetData(
      aCharset, MOZ_UTF16(".LangGroup"), langGroup);

  if (NS_SUCCEEDED(rv)) {
    ToLowerCase(langGroup); 
    *aResult = NS_NewAtom(langGroup).get();
  }

  return rv;
}
