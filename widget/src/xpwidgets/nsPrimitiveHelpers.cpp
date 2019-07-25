






















































#include "nsPrimitiveHelpers.h"
#include "nsCOMPtr.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsITransferable.h"
#include "nsIComponentManager.h"
#include "nsLinebreakConverter.h"
#include "nsReadableUtils.h"

#include "nsIServiceManager.h"
#include "nsICharsetConverterManager.h"

#  include "nsIPlatformCharset.h"
#include "nsISaveAsCharset.h"
#include "nsAutoPtr.h"










void
nsPrimitiveHelpers :: CreatePrimitiveForData ( const char* aFlavor, void* aDataBuff, 
                                                 PRUint32 aDataLen, nsISupports** aPrimitive )
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
        if (!NS_LIKELY(buffer))
          return;
      
        memcpy(buffer, aDataBuff, aDataLen);
        buffer[aDataLen] = 0;
        const PRUnichar* start = reinterpret_cast<const PRUnichar*>(buffer.get());
        
        primitive->SetData(Substring(start, start + (aDataLen + 1) / 2));
      } else {
        const PRUnichar* start = reinterpret_cast<const PRUnichar*>(aDataBuff);
        
        primitive->SetData(Substring(start, start + (aDataLen / 2)));
      }
      NS_ADDREF(*aPrimitive = primitive);
    }  
  }

} 









void
nsPrimitiveHelpers :: CreateDataFromPrimitive ( const char* aFlavor, nsISupports* aPrimitive, 
                                                   void** aDataBuff, PRUint32 aDataLen )
{
  if ( !aDataBuff )
    return;

  *aDataBuff = nsnull;

  if ( strcmp(aFlavor,kTextMime) == 0 ) {
    nsCOMPtr<nsISupportsCString> plainText ( do_QueryInterface(aPrimitive) );
    if ( plainText ) {
      nsCAutoString data;
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
nsPrimitiveHelpers :: ConvertUnicodeToPlatformPlainText ( PRUnichar* inUnicode, PRInt32 inUnicodeLen, 
                                                            char** outPlainTextData, PRInt32* outPlainTextLen )
{
  if ( !outPlainTextData || !outPlainTextLen )
    return NS_ERROR_INVALID_ARG;

  
  nsresult rv;
  nsCOMPtr <nsIPlatformCharset> platformCharsetService = do_GetService(NS_PLATFORMCHARSET_CONTRACTID, &rv);

  nsCAutoString platformCharset;
  if (NS_SUCCEEDED(rv))
    rv = platformCharsetService->GetCharset(kPlatformCharsetSel_PlainTextInClipboard, platformCharset);
  if (NS_FAILED(rv))
    platformCharset.AssignLiteral("ISO-8859-1");

  

  nsCOMPtr<nsISaveAsCharset> converter = do_CreateInstance("@mozilla.org/intl/saveascharset;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = converter->Init(platformCharset.get(),
                  nsISaveAsCharset::attr_EntityAfterCharsetConv +
                  nsISaveAsCharset::attr_FallbackQuestionMark,
                  nsIEntityConverter::transliterate);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = converter->Convert(inUnicode, outPlainTextData);
  *outPlainTextLen = *outPlainTextData ? strlen(*outPlainTextData) : 0;

  NS_ASSERTION ( NS_SUCCEEDED(rv), "Error converting unicode to plain text" );
  
  return rv;
} 










nsresult
nsPrimitiveHelpers :: ConvertPlatformPlainTextToUnicode ( const char* inText, PRInt32 inTextLen, 
                                                            PRUnichar** outUnicode, PRInt32* outUnicodeLen )
{
  if ( !outUnicode || !outUnicodeLen )
    return NS_ERROR_INVALID_ARG;

  
  
  nsresult rv = NS_OK;
  static nsCOMPtr<nsIUnicodeDecoder> decoder;
  static PRBool hasConverter = PR_FALSE;
  if ( !hasConverter ) {
    
    nsCAutoString platformCharset;
    nsCOMPtr <nsIPlatformCharset> platformCharsetService = do_GetService(NS_PLATFORMCHARSET_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv))
      rv = platformCharsetService->GetCharset(kPlatformCharsetSel_PlainTextInClipboard, platformCharset);
    if (NS_FAILED(rv))
      platformCharset.AssignLiteral("ISO-8859-1");
      
    
    nsCOMPtr<nsICharsetConverterManager> ccm = 
             do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);  
    rv = ccm->GetUnicodeDecoderRaw(platformCharset.get(),
                                   getter_AddRefs(decoder));

    NS_ASSERTION(NS_SUCCEEDED(rv), "GetUnicodeEncoderRaw failed.");
    if (NS_FAILED(rv))
      return NS_ERROR_FAILURE;

    hasConverter = PR_TRUE;
  }
  
  
  
  decoder->GetMaxLength(inText, inTextLen, outUnicodeLen);   
  if ( *outUnicodeLen ) {
    *outUnicode = reinterpret_cast<PRUnichar*>(nsMemory::Alloc((*outUnicodeLen + 1) * sizeof(PRUnichar)));
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
                                                          PRInt32* ioLengthInBytes )
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
        nsMemory::Free ( oldBuffer );
      *ioData = buffAsChars;
    }
  }
  else if ( strcmp(inFlavor, "image/jpeg") == 0 ) {
    
  }
  else {       
    PRUnichar* buffAsUnichar = reinterpret_cast<PRUnichar*>(*ioData);
    PRUnichar* oldBuffer = buffAsUnichar;
    PRInt32 newLengthInChars;
    retVal = nsLinebreakConverter::ConvertUnicharLineBreaksInSitu ( &buffAsUnichar, nsLinebreakConverter::eLinebreakAny, 
                                                                     nsLinebreakConverter::eLinebreakContent, 
                                                                     *ioLengthInBytes / sizeof(PRUnichar), &newLengthInChars );
    if ( NS_SUCCEEDED(retVal) ) {
      if ( buffAsUnichar != oldBuffer )           
        nsMemory::Free ( oldBuffer );
      *ioData = buffAsUnichar;
      *ioLengthInBytes = newLengthInChars * sizeof(PRUnichar);
    }
  }
  
  return retVal;

} 
