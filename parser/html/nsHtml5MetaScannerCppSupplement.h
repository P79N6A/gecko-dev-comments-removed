



































 
#include "nsICharsetConverterManager.h"
#include "nsServiceManagerUtils.h"
#include "nsICharsetAlias.h"
#include "nsEncoderDecoderUtils.h"
#include "nsTraceRefcnt.h"

static NS_DEFINE_CID(kCharsetAliasCID, NS_CHARSETALIAS_CID);

nsHtml5MetaScanner::nsHtml5MetaScanner()
 : readable(nsnull),
   metaState(NS_HTML5META_SCANNER_NO),
   contentIndex(-1),
   charsetIndex(-1),
   stateSave(NS_HTML5META_SCANNER_DATA),
   strBufLen(0),
   strBuf(jArray<PRUnichar,PRInt32>(36))
{
  MOZ_COUNT_CTOR(nsHtml5MetaScanner);
}

nsHtml5MetaScanner::~nsHtml5MetaScanner()
{
  MOZ_COUNT_DTOR(nsHtml5MetaScanner);
  strBuf.release();
}

void
nsHtml5MetaScanner::sniff(nsHtml5ByteReadable* bytes, nsIUnicodeDecoder** decoder, nsACString& charset)
{
  readable = bytes;
  stateLoop(stateSave);
  readable = nsnull;
  if (mUnicodeDecoder) {
    mUnicodeDecoder.forget(decoder);
    charset.Assign(mCharset);
  }
}

PRBool
nsHtml5MetaScanner::tryCharset(nsString* charset)
{
  nsresult res = NS_OK;
  nsCOMPtr<nsICharsetConverterManager> convManager = do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &res);
  if (NS_FAILED(res)) {
    NS_ERROR("Could not get CharsetConverterManager service.");
    return PR_FALSE;
  }
  nsCAutoString encoding;
  CopyUTF16toUTF8(*charset, encoding);
  
  if (encoding.LowerCaseEqualsASCII("utf-16") ||
      encoding.LowerCaseEqualsASCII("utf-16be") ||
      encoding.LowerCaseEqualsASCII("utf-16le") ||
      encoding.LowerCaseEqualsASCII("utf-32") ||
      encoding.LowerCaseEqualsASCII("utf-32be") ||
      encoding.LowerCaseEqualsASCII("utf-32le")) {
    mCharset.Assign("UTF-8");
    res = convManager->GetUnicodeDecoderRaw(mCharset.get(), getter_AddRefs(mUnicodeDecoder));
    if (NS_FAILED(res)) {
      NS_ERROR("Could not get decoder for UTF-8.");
      return PR_FALSE;
    }
    return PR_TRUE;
  }
  nsCAutoString preferred;
  nsCOMPtr<nsICharsetAlias> calias(do_GetService(kCharsetAliasCID, &res));
  if (NS_FAILED(res)) {
    NS_ERROR("Could not get CharsetAlias service.");
    return PR_FALSE;
  }
  res = calias->GetPreferred(encoding, preferred);
  if (NS_FAILED(res)) {
    return PR_FALSE;
  }
  if (preferred.LowerCaseEqualsASCII("utf-16") ||
      preferred.LowerCaseEqualsASCII("utf-16be") ||
      preferred.LowerCaseEqualsASCII("utf-16le") ||
      preferred.LowerCaseEqualsASCII("utf-32") ||
      preferred.LowerCaseEqualsASCII("utf-32be") ||
      preferred.LowerCaseEqualsASCII("utf-32le") ||
      preferred.LowerCaseEqualsASCII("utf-7") ||
      preferred.LowerCaseEqualsASCII("jis_x0212-1990") ||
      preferred.LowerCaseEqualsASCII("x-jis0208") ||
      preferred.LowerCaseEqualsASCII("x-imap4-modified-utf7") ||
      preferred.LowerCaseEqualsASCII("x-user-defined")) {
    return PR_FALSE;
  }
  res = convManager->GetUnicodeDecoderRaw(preferred.get(), getter_AddRefs(mUnicodeDecoder));
  if (res == NS_ERROR_UCONV_NOCONV) {
    return PR_FALSE;
  } else if (NS_FAILED(res)) {
    NS_ERROR("Getting an encoding decoder failed in a bad way.");
    mUnicodeDecoder = nsnull;
    return PR_FALSE;
  } else {
    NS_ASSERTION(mUnicodeDecoder, "Getter nsresult and object don't match.");
    mCharset.Assign(preferred);
    return PR_TRUE;
  }
}
