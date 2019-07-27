





















#include "nsPrimitiveHelpers.h"
#include "nsCOMPtr.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsITransferable.h"
#include "nsIComponentManager.h"
#include "nsLinebreakConverter.h"
#include "nsReadableUtils.h"

#include "nsIServiceManager.h"

#include "nsIPlatformCharset.h"
#include "nsIUnicodeDecoder.h"
#include "nsISaveAsCharset.h"
#include "nsAutoPtr.h"
#include "mozilla/Likely.h"
#include "mozilla/dom/EncodingUtils.h"

using mozilla::dom::EncodingUtils;










void
nsPrimitiveHelpers :: CreatePrimitiveForData ( const char* aFlavor, const void* aDataBuff,
                                                 uint32_t aDataLen, nsISupports** aPrimitive )
{
  if ( !aPrimitive )
    return;

  if ( strcmp(aFlavor,kTextMime) == 0 || strcmp(aFlavor,kNativeHTMLMime) == 0 ) {
    nsCOMPtr<nsISupportsCString> primitive =
        do_CreateInstance(NS_SUPPORTS_CSTRING_CONTRACTID);
    if ( primitive ) {
      const char * start = reinterpret_cast<const char*>(aDataBuff);
      primitive->SetData(Substring(start, start + aDataLen));
      NS_ADDREF(*aPrimitive = primitive);
    }
  }
  else {
    nsCOMPtr<nsISupportsString> primitive =
        do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID);
    if (primitive ) {
      if (aDataLen % 2) {
        nsAutoArrayPtr<char> buffer(new char[aDataLen + 1]);
        if (!MOZ_LIKELY(buffer))
          return;

        memcpy(buffer, aDataBuff, aDataLen);
        buffer[aDataLen] = 0;
        const char16_t* start = reinterpret_cast<const char16_t*>(buffer.get());
        
        primitive->SetData(Substring(start, start + (aDataLen + 1) / 2));
      } else {
        const char16_t* start = reinterpret_cast<const char16_t*>(aDataBuff);
        
        primitive->SetData(Substring(start, start + (aDataLen / 2)));
      }
      NS_ADDREF(*aPrimitive = primitive);
    }
  }

} 









void
nsPrimitiveHelpers :: CreateDataFromPrimitive ( const char* aFlavor, nsISupports* aPrimitive,
                                                   void** aDataBuff, uint32_t aDataLen )
{
  if ( !aDataBuff )
    return;

  *aDataBuff = nullptr;

  if ( strcmp(aFlavor,kTextMime) == 0 ) {
    nsCOMPtr<nsISupportsCString> plainText ( do_QueryInterface(aPrimitive) );
    if ( plainText ) {
      nsAutoCString data;
      plainText->GetData ( data );
      *aDataBuff = ToNewCString(data);
    }
  }
  else {
    nsCOMPtr<nsISupportsString> doubleByteText ( do_QueryInterface(aPrimitive) );
    if ( doubleByteText ) {
      nsAutoString data;
      doubleByteText->GetData ( data );
      *aDataBuff = ToNewUnicode(data);
    }
  }

}










nsresult
nsPrimitiveHelpers :: ConvertUnicodeToPlatformPlainText ( char16_t* inUnicode, int32_t inUnicodeLen,
                                                            char** outPlainTextData, int32_t* outPlainTextLen )
{
  if ( !outPlainTextData || !outPlainTextLen )
    return NS_ERROR_INVALID_ARG;

  
  nsresult rv;
  nsCOMPtr <nsIPlatformCharset> platformCharsetService = do_GetService(NS_PLATFORMCHARSET_CONTRACTID, &rv);

  nsAutoCString platformCharset;
  if (NS_SUCCEEDED(rv))
    rv = platformCharsetService->GetCharset(kPlatformCharsetSel_PlainTextInClipboard, platformCharset);
  if (NS_FAILED(rv))
    platformCharset.AssignLiteral("ISO-8859-1");

  

  nsCOMPtr<nsISaveAsCharset> converter = do_CreateInstance("@mozilla.org/intl/saveascharset;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = converter->Init(platformCharset.get(),
                  nsISaveAsCharset::attr_EntityAfterCharsetConv +
                  nsISaveAsCharset::attr_FallbackQuestionMark,
                  0);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = converter->Convert(inUnicode, outPlainTextData);
  *outPlainTextLen = *outPlainTextData ? strlen(*outPlainTextData) : 0;

  NS_ASSERTION ( NS_SUCCEEDED(rv), "Error converting unicode to plain text" );

  return rv;
} 










nsresult
nsPrimitiveHelpers :: ConvertPlatformPlainTextToUnicode ( const char* inText, int32_t inTextLen,
                                                            char16_t** outUnicode, int32_t* outUnicodeLen )
{
  if ( !outUnicode || !outUnicodeLen )
    return NS_ERROR_INVALID_ARG;

  
  
  nsresult rv = NS_OK;
  static nsCOMPtr<nsIUnicodeDecoder> decoder;
  static bool hasConverter = false;
  if ( !hasConverter ) {
    
    nsAutoCString platformCharset;
    nsCOMPtr <nsIPlatformCharset> platformCharsetService = do_GetService(NS_PLATFORMCHARSET_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv))
      rv = platformCharsetService->GetCharset(kPlatformCharsetSel_PlainTextInClipboard, platformCharset);
    if (NS_FAILED(rv))
      platformCharset.AssignLiteral("windows-1252");

    decoder = EncodingUtils::DecoderForEncoding(platformCharset);

    hasConverter = true;
  }

  
  
  decoder->GetMaxLength(inText, inTextLen, outUnicodeLen);   
  if ( *outUnicodeLen ) {
    *outUnicode = reinterpret_cast<char16_t*>(moz_xmalloc((*outUnicodeLen + 1) * sizeof(char16_t)));
    if ( *outUnicode ) {
      rv = decoder->Convert(inText, &inTextLen, *outUnicode, outUnicodeLen);
      (*outUnicode)[*outUnicodeLen] = '\0';                   
    }
  } 

  NS_ASSERTION ( NS_SUCCEEDED(rv), "Error converting plain text to unicode" );

  return rv;
} 












nsresult
nsLinebreakHelpers :: ConvertPlatformToDOMLinebreaks ( const char* inFlavor, void** ioData,
                                                          int32_t* ioLengthInBytes )
{
  NS_ASSERTION ( ioData && *ioData && ioLengthInBytes, "Bad Params");
  if ( !(ioData && *ioData && ioLengthInBytes) )
    return NS_ERROR_INVALID_ARG;

  nsresult retVal = NS_OK;

  if ( strcmp(inFlavor, "text/plain") == 0 ) {
    char* buffAsChars = reinterpret_cast<char*>(*ioData);
    char* oldBuffer = buffAsChars;
    retVal = nsLinebreakConverter::ConvertLineBreaksInSitu ( &buffAsChars, nsLinebreakConverter::eLinebreakAny,
                                                              nsLinebreakConverter::eLinebreakContent,
                                                              *ioLengthInBytes, ioLengthInBytes );
    if ( NS_SUCCEEDED(retVal) ) {
      if ( buffAsChars != oldBuffer )             
        free ( oldBuffer );
      *ioData = buffAsChars;
    }
  }
  else if ( strcmp(inFlavor, "image/jpeg") == 0 ) {
    
  }
  else {
    char16_t* buffAsUnichar = reinterpret_cast<char16_t*>(*ioData);
    char16_t* oldBuffer = buffAsUnichar;
    int32_t newLengthInChars;
    retVal = nsLinebreakConverter::ConvertUnicharLineBreaksInSitu ( &buffAsUnichar, nsLinebreakConverter::eLinebreakAny,
                                                                     nsLinebreakConverter::eLinebreakContent,
                                                                     *ioLengthInBytes / sizeof(char16_t), &newLengthInChars );
    if ( NS_SUCCEEDED(retVal) ) {
      if ( buffAsUnichar != oldBuffer )           
        free ( oldBuffer );
      *ioData = buffAsUnichar;
      *ioLengthInBytes = newLengthInChars * sizeof(char16_t);
    }
  }

  return retVal;

} 
