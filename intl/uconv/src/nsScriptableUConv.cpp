






































#include "pratom.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsIServiceManager.h"
#include "nsICharsetConverterManager.h"
#include "nsIScriptableUConv.h"
#include "nsScriptableUConv.h"
#include "nsIStringStream.h"
#include "nsCRT.h"

#include "nsIPlatformCharset.h"

static PRInt32          gInstanceCount = 0;


NS_IMPL_ISUPPORTS1(nsScriptableUnicodeConverter, nsIScriptableUnicodeConverter)

nsScriptableUnicodeConverter::nsScriptableUnicodeConverter()
{
  PR_AtomicIncrement(&gInstanceCount);
}

nsScriptableUnicodeConverter::~nsScriptableUnicodeConverter()
{
  PR_AtomicDecrement(&gInstanceCount);
}

nsresult
nsScriptableUnicodeConverter::ConvertFromUnicodeWithLength(const nsAString& aSrc,
                                                           PRInt32* aOutLen,
                                                           char **_retval)
{
  if (!mEncoder)
    return NS_ERROR_FAILURE;

  nsresult rv = NS_OK;
  PRInt32 inLength = aSrc.Length();
  const nsAFlatString& flatSrc = PromiseFlatString(aSrc);
  rv = mEncoder->GetMaxLength(flatSrc.get(), inLength, aOutLen);
  if (NS_SUCCEEDED(rv)) {
    *_retval = (char*) nsMemory::Alloc(*aOutLen+1);
    if (!*_retval)
      return NS_ERROR_OUT_OF_MEMORY;

    rv = mEncoder->Convert(flatSrc.get(), &inLength, *_retval, aOutLen);
    if (NS_SUCCEEDED(rv))
    {
      (*_retval)[*aOutLen] = '\0';
      return NS_OK;
    }
    nsMemory::Free(*_retval);
  }
  *_retval = nsnull;
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
nsScriptableUnicodeConverter::ConvertFromUnicode(const nsAString& aSrc,
                                                 nsACString& _retval)
{
  PRInt32 len;
  char* str;
  nsresult rv = ConvertFromUnicodeWithLength(aSrc, &len, &str);
  if (NS_SUCCEEDED(rv)) {
    
    _retval.Assign(str, len);
    nsMemory::Free(str);
  }
  return rv;
}

nsresult
nsScriptableUnicodeConverter::FinishWithLength(char **_retval, PRInt32* aLength)
{
  if (!mEncoder)
    return NS_ERROR_FAILURE;

  PRInt32 finLength = 32;

  *_retval = (char *) nsMemory::Alloc(finLength);
  if (!*_retval)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = mEncoder->Finish(*_retval, &finLength);
  if (NS_SUCCEEDED(rv))
    *aLength = finLength;
  else
    nsMemory::Free(*_retval);

  return rv;

}


NS_IMETHODIMP
nsScriptableUnicodeConverter::Finish(nsACString& _retval)
{
  PRInt32 len;
  char* str;
  nsresult rv = FinishWithLength(&str, &len);
  if (NS_SUCCEEDED(rv)) {
    
    _retval.Assign(str, len);
    nsMemory::Free(str);
  }
  return rv;
}


NS_IMETHODIMP
nsScriptableUnicodeConverter::ConvertToUnicode(const nsACString& aSrc, nsAString& _retval)
{
  nsACString::const_iterator i;
  aSrc.BeginReading(i);
  return ConvertFromByteArray(NS_REINTERPRET_CAST(const PRUint8*, i.get()),
                              aSrc.Length(),
                              _retval);
}




NS_IMETHODIMP
nsScriptableUnicodeConverter::ConvertFromByteArray(const PRUint8* aData,
                                                   PRUint32 aCount,
                                                   nsAString& _retval)
{
  if (!mDecoder)
    return NS_ERROR_FAILURE;

  nsresult rv = NS_OK;
  PRInt32 inLength = aCount;
  PRInt32 outLength;
  rv = mDecoder->GetMaxLength(NS_REINTERPRET_CAST(const char*, aData),
                              inLength, &outLength);
  if (NS_SUCCEEDED(rv))
  {
    PRUnichar* buf = (PRUnichar*) nsMemory::Alloc((outLength+1)*sizeof(PRUnichar));
    if (!buf)
      return NS_ERROR_OUT_OF_MEMORY;

    rv = mDecoder->Convert(NS_REINTERPRET_CAST(const char*, aData),
                           &inLength, buf, &outLength);
    if (NS_SUCCEEDED(rv))
    {
      buf[outLength] = 0;
      _retval.Assign(buf, outLength);
    }
    nsMemory::Free(buf);
    return rv;
  }
  return NS_ERROR_FAILURE;

}





NS_IMETHODIMP
nsScriptableUnicodeConverter::ConvertToByteArray(const nsAString& aString,
                                                 PRUint32* aLen,
                                                 PRUint8** _aData)
{
  char* data;
  PRInt32 len;
  nsresult rv = ConvertFromUnicodeWithLength(aString, &len, &data);
  if (NS_FAILED(rv))
    return rv;
  nsXPIDLCString str;
  str.Adopt(data, len); 

  rv = FinishWithLength(&data, &len);
  if (NS_FAILED(rv))
    return rv;

  str.Append(data, len);
  nsMemory::Free(data);
  
  *_aData = NS_REINTERPRET_CAST(PRUint8*,
                                nsMemory::Clone(str.get(), str.Length()));
  if (!*_aData)
    return NS_ERROR_OUT_OF_MEMORY;
  *aLen = str.Length();
  return NS_OK;
}


NS_IMETHODIMP
nsScriptableUnicodeConverter::ConvertToInputStream(const nsAString& aString,
                                                   nsIInputStream** _retval)
{
  nsresult rv;
  nsCOMPtr<nsIStringInputStream> inputStream =
    do_CreateInstance("@mozilla.org/io/string-input-stream;1", &rv);
  if (NS_FAILED(rv))
    return rv;

  PRUint8* data;
  PRUint32 dataLen;
  rv = ConvertToByteArray(aString, &dataLen, &data);
  if (NS_FAILED(rv))
    return rv;

  rv = inputStream->AdoptData(NS_REINTERPRET_CAST(char*, data), dataLen);
  if (NS_FAILED(rv)) {
    nsMemory::Free(data);
    return rv;
  }

  NS_ADDREF(*_retval = inputStream);
  return rv;
}


NS_IMETHODIMP
nsScriptableUnicodeConverter::GetCharset(char * *aCharset)
{
  *aCharset = ToNewCString(mCharset);
  if (!*aCharset)
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

NS_IMETHODIMP
nsScriptableUnicodeConverter::SetCharset(const char * aCharset)
{
  mCharset.Assign(aCharset);
  return InitConverter();
}

nsresult
nsScriptableUnicodeConverter::InitConverter()
{
  nsresult rv = NS_OK;
  mEncoder = NULL ;

  nsCOMPtr<nsICharsetConverterManager> ccm = do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);

  if (NS_SUCCEEDED( rv) && (nsnull != ccm)) {
    
    
    
    rv = ccm->GetUnicodeEncoder(mCharset.get(), getter_AddRefs(mEncoder));
    if(NS_SUCCEEDED(rv)) {
      rv = mEncoder->SetOutputErrorBehavior(nsIUnicodeEncoder::kOnError_Replace, nsnull, (PRUnichar)'?');
      if(NS_SUCCEEDED(rv)) {
        rv = ccm->GetUnicodeDecoder(mCharset.get(), getter_AddRefs(mDecoder));
      }
    }
  }

  return rv ;
}
