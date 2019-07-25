



































 
#include "nsICharsetConverterManager.h"
#include "nsServiceManagerUtils.h"
#include "nsICharsetAlias.h"
#include "nsEncoderDecoderUtils.h"
#include "nsTraceRefcnt.h"

static NS_DEFINE_CID(kCharsetAliasCID, NS_CHARSETALIAS_CID);

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

bool
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
  encoding.Trim(" \t\r\n\f");
  if (encoding.LowerCaseEqualsLiteral("utf-16") ||
      encoding.LowerCaseEqualsLiteral("utf-16be") ||
      encoding.LowerCaseEqualsLiteral("utf-16le")) {
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
  if (preferred.LowerCaseEqualsLiteral("utf-16") ||
      preferred.LowerCaseEqualsLiteral("utf-16be") ||
      preferred.LowerCaseEqualsLiteral("utf-16le") ||
      preferred.LowerCaseEqualsLiteral("utf-7") ||
      preferred.LowerCaseEqualsLiteral("jis_x0212-1990") ||
      preferred.LowerCaseEqualsLiteral("x-jis0208") ||
      preferred.LowerCaseEqualsLiteral("x-imap4-modified-utf7") ||
      preferred.LowerCaseEqualsLiteral("x-user-defined")) {
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
