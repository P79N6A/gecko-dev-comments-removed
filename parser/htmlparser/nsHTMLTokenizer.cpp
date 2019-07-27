















#include "nsHTMLTokenizer.h"
#include "nsIParser.h"
#include "nsParserConstants.h"








NS_IMPL_ISUPPORTS(nsHTMLTokenizer, nsITokenizer)




nsHTMLTokenizer::nsHTMLTokenizer()
{
  
}

nsresult
nsHTMLTokenizer::WillTokenize(bool aIsFinalChunk)
{
  return NS_OK;
}













nsresult
nsHTMLTokenizer::ConsumeToken(nsScanner& aScanner, bool& aFlushTokens)
{
  return kEOF;
}
