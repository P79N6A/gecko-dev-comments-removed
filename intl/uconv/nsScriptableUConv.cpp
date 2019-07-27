




#include "nsString.h"
#include "nsIScriptableUConv.h"
#include "nsScriptableUConv.h"
#include "nsIStringStream.h"
#include "nsComponentManagerUtils.h"
#include "nsIUnicodeDecoder.h"
#include "nsIUnicodeEncoder.h"
#include "mozilla/dom/EncodingUtils.h"

using mozilla::dom::EncodingUtils;


NS_IMPL_ISUPPORTS(nsScriptableUnicodeConverter, nsIScriptableUnicodeConverter)

nsScriptableUnicodeConverter::nsScriptableUnicodeConverter()
: mIsInternal(false)
{
}

nsScriptableUnicodeConverter::~nsScriptableUnicodeConverter()
{
}

nsresult
nsScriptableUnicodeConverter::ConvertFromUnicodeWithLength(const nsAString& aSrc,
                                                           int32_t* aOutLen,
                                                           char **_retval)
{
  if (!mEncoder)
    return NS_ERROR_FAILURE;

  nsresult rv = NS_OK;
  int32_t inLength = aSrc.Length();
  const nsAFlatString& flatSrc = PromiseFlatString(aSrc);
  rv = mEncoder->GetMaxLength(flatSrc.get(), inLength, aOutLen);
  if (NS_SUCCEEDED(rv)) {
    *_retval = (char*)moz_malloc(*aOutLen+1);
    if (!*_retval)
      return NS_ERROR_OUT_OF_MEMORY;

    rv = mEncoder->Convert(flatSrc.get(), &inLength, *_retval, aOutLen);
    if (NS_SUCCEEDED(rv))
    {
      (*_retval)[*aOutLen] = '\0';
      return NS_OK;
    }
    moz_free(*_retval);
  }
  *_retval = nullptr;
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
nsScriptableUnicodeConverter::ConvertFromUnicode(const nsAString& aSrc,
                                                 nsACString& _retval)
{
  int32_t len;
  char* str;
  nsresult rv = ConvertFromUnicodeWithLength(aSrc, &len, &str);
  if (NS_SUCCEEDED(rv)) {
    
    if (!_retval.Assign(str, len, mozilla::fallible_t())) {
      rv = NS_ERROR_OUT_OF_MEMORY;
    }
    moz_free(str);
  }
  return rv;
}

nsresult
nsScriptableUnicodeConverter::FinishWithLength(char **_retval, int32_t* aLength)
{
  if (!mEncoder)
    return NS_ERROR_FAILURE;

  int32_t finLength = 32;

  *_retval = (char *)moz_malloc(finLength);
  if (!*_retval)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = mEncoder->Finish(*_retval, &finLength);
  if (NS_SUCCEEDED(rv))
    *aLength = finLength;
  else
    moz_free(*_retval);

  return rv;

}


NS_IMETHODIMP
nsScriptableUnicodeConverter::Finish(nsACString& _retval)
{
  
  
  
  
  
  
  if (!mEncoder) {
    _retval.Truncate();
    return NS_OK;
  }
  int32_t len;
  char* str;
  nsresult rv = FinishWithLength(&str, &len);
  if (NS_SUCCEEDED(rv)) {
    
    if (!_retval.Assign(str, len, mozilla::fallible_t())) {
      rv = NS_ERROR_OUT_OF_MEMORY;
    }
    moz_free(str);
  }
  return rv;
}


NS_IMETHODIMP
nsScriptableUnicodeConverter::ConvertToUnicode(const nsACString& aSrc, nsAString& _retval)
{
  nsACString::const_iterator i;
  aSrc.BeginReading(i);
  return ConvertFromByteArray(reinterpret_cast<const uint8_t*>(i.get()),
                              aSrc.Length(),
                              _retval);
}




NS_IMETHODIMP
nsScriptableUnicodeConverter::ConvertFromByteArray(const uint8_t* aData,
                                                   uint32_t aCount,
                                                   nsAString& _retval)
{
  if (!mDecoder)
    return NS_ERROR_FAILURE;

  nsresult rv = NS_OK;
  int32_t inLength = aCount;
  int32_t outLength;
  rv = mDecoder->GetMaxLength(reinterpret_cast<const char*>(aData),
                              inLength, &outLength);
  if (NS_SUCCEEDED(rv))
  {
    char16_t* buf = (char16_t*)moz_malloc((outLength+1)*sizeof(char16_t));
    if (!buf)
      return NS_ERROR_OUT_OF_MEMORY;

    rv = mDecoder->Convert(reinterpret_cast<const char*>(aData),
                           &inLength, buf, &outLength);
    if (NS_SUCCEEDED(rv))
    {
      buf[outLength] = 0;
      if (!_retval.Assign(buf, outLength, mozilla::fallible_t())) {
        rv = NS_ERROR_OUT_OF_MEMORY;
      }
    }
    moz_free(buf);
    return rv;
  }
  return NS_ERROR_FAILURE;

}





NS_IMETHODIMP
nsScriptableUnicodeConverter::ConvertToByteArray(const nsAString& aString,
                                                 uint32_t* aLen,
                                                 uint8_t** _aData)
{
  char* data;
  int32_t len;
  nsresult rv = ConvertFromUnicodeWithLength(aString, &len, &data);
  if (NS_FAILED(rv))
    return rv;
  nsXPIDLCString str;
  str.Adopt(data, len); 

  rv = FinishWithLength(&data, &len);
  if (NS_FAILED(rv))
    return rv;

  str.Append(data, len);
  moz_free(data);
  
  *_aData = reinterpret_cast<uint8_t*>(moz_malloc(str.Length()));
  if (!*_aData)
    return NS_ERROR_OUT_OF_MEMORY;
  memcpy(*_aData, str.get(), str.Length());
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

  uint8_t* data;
  uint32_t dataLen;
  rv = ConvertToByteArray(aString, &dataLen, &data);
  if (NS_FAILED(rv))
    return rv;

  rv = inputStream->AdoptData(reinterpret_cast<char*>(data), dataLen);
  if (NS_FAILED(rv)) {
    moz_free(data);
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

NS_IMETHODIMP
nsScriptableUnicodeConverter::GetIsInternal(bool *aIsInternal)
{
  *aIsInternal = mIsInternal;
  return NS_OK;
}

NS_IMETHODIMP
nsScriptableUnicodeConverter::SetIsInternal(const bool aIsInternal)
{
  mIsInternal = aIsInternal;
  return NS_OK;
}

nsresult
nsScriptableUnicodeConverter::InitConverter()
{
  mEncoder = nullptr;
  mDecoder = nullptr;

  nsAutoCString encoding;
  if (mIsInternal) {
    
    
    
    nsAutoCString contractId;
    nsAutoCString label(mCharset);
    EncodingUtils::TrimSpaceCharacters(label);
    
    
    ToLowerCase(label);
    if (label.EqualsLiteral("replacement")) {
      
      return NS_ERROR_UCONV_NOCONV;
    }
    contractId.AssignLiteral(NS_UNICODEENCODER_CONTRACTID_BASE);
    contractId.Append(label);
    mEncoder = do_CreateInstance(contractId.get());
    contractId.AssignLiteral(NS_UNICODEDECODER_CONTRACTID_BASE);
    contractId.Append(label);
    mDecoder = do_CreateInstance(contractId.get());
    if (!mDecoder) {
      
      
      
      
      
      ToUpperCase(label);
      contractId.AssignLiteral(NS_UNICODEENCODER_CONTRACTID_BASE);
      contractId.Append(label);
      mEncoder = do_CreateInstance(contractId.get());
      contractId.AssignLiteral(NS_UNICODEDECODER_CONTRACTID_BASE);
      contractId.Append(label);
      mDecoder = do_CreateInstance(contractId.get());
      
    }
  }

  if (!mDecoder) {
    if (!EncodingUtils::FindEncodingForLabelNoReplacement(mCharset, encoding)) {
      return NS_ERROR_UCONV_NOCONV;
    }
    mEncoder = EncodingUtils::EncoderForEncoding(encoding);
    mDecoder = EncodingUtils::DecoderForEncoding(encoding);
  }

  
  
  
  
  if (encoding.EqualsLiteral("UTF-8")) {
    mDecoder->SetInputErrorBehavior(nsIUnicodeDecoder::kOnError_Signal);
  }

  if (!mEncoder) {
    return NS_OK;
  }

  return mEncoder->SetOutputErrorBehavior(nsIUnicodeEncoder::kOnError_Replace,
                                          nullptr,
                                          (char16_t)'?');
}
