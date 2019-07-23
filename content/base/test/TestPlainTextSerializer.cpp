




































#include "TestHarness.h"

#include "nsIParser.h"
#include "nsIHTMLToTextSink.h"
#include "nsIParser.h"
#include "nsIContentSink.h"
#include "nsIParserService.h"
#include "nsServiceManagerUtils.h"
#include "nsStringGlue.h"
#include "nsParserCIID.h"

static NS_DEFINE_CID(kCParserCID, NS_PARSER_CID);

void
ConvertBufToPlainText(nsString &aConBuf)
{
  nsCOMPtr<nsIParser> parser = do_CreateInstance(kCParserCID);
  if (parser) {
    nsCOMPtr<nsIContentSink> sink;
    sink = do_CreateInstance(NS_PLAINTEXTSINK_CONTRACTID);
    if (sink) {
      nsCOMPtr<nsIHTMLToTextSink> textSink(do_QueryInterface(sink));
      if (textSink) {
        nsAutoString convertedText;
        textSink->Initialize(&convertedText, 0, 72);
        parser->SetContentSink(sink);
        parser->Parse(aConBuf, 0, NS_LITERAL_CSTRING("text/html"), PR_TRUE);
        aConBuf = convertedText;
      }
    }
  }
}

nsresult
TestPlainTextSerializer()
{
  nsString test;
  test.AppendLiteral("<html><base>base</base><head><span>span</span></head>"
                     "<body>body</body></html>");
  ConvertBufToPlainText(test);
  if (!test.EqualsLiteral("basespanbody")) {
    fail("Wrong html to text serialization");
    return NS_ERROR_FAILURE;
  }

  passed("HTML to text serialization test");

  
  return NS_OK;
}

int main(int argc, char** argv)
{
  ScopedXPCOM xpcom("PlainTextSerializer");
  if (xpcom.failed())
    return 1;

  int retval = 0;
  if (NS_FAILED(TestPlainTextSerializer())) {
    retval = 1;
  }

  return retval;
}
