















#include "nsIAtom.h"
#include "nsHTMLTokenizer.h"
#include "nsParserConstants.h"
#include "nsIHTMLContentSink.h"








NS_IMPL_ISUPPORTS1(nsHTMLTokenizer, nsITokenizer)








nsHTMLTokenizer::nsHTMLTokenizer(nsDTDMode aParseMode,
                                 eParserDocType aDocType,
                                 eParserCommands aCommand,
                                 uint32_t aFlags)
{
  
  MOZ_ASSERT(!(aFlags & NS_IPARSER_FLAG_XML));
}




nsHTMLTokenizer::~nsHTMLTokenizer()
{
}

 uint32_t
nsHTMLTokenizer::GetFlags(const nsIContentSink* aSink)
{
  uint32_t flags = 0;
  nsCOMPtr<nsIHTMLContentSink> sink =
    do_QueryInterface(const_cast<nsIContentSink*>(aSink));
  if (sink) {
    bool enabled = true;
    sink->IsEnabled(eHTMLTag_frameset, &enabled);
    if (enabled) {
      flags |= NS_IPARSER_FLAG_FRAMES_ENABLED;
    }
    sink->IsEnabled(eHTMLTag_script, &enabled);
    if (enabled) {
      flags |= NS_IPARSER_FLAG_SCRIPT_ENABLED;
    }
  }
  return flags;
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
