




































#include "TestHarness.h"

#include "nsIParser.h"
#include "nsIHTMLToTextSink.h"
#include "nsIParser.h"
#include "nsIContentSink.h"
#include "nsIParserService.h"
#include "nsServiceManagerUtils.h"
#include "nsStringGlue.h"
#include "nsParserCIID.h"
#include "nsIDocumentEncoder.h"
#include "nsCRT.h"

static NS_DEFINE_CID(kCParserCID, NS_PARSER_CID);

void
ConvertBufToPlainText(nsString &aConBuf, int aFlag)
{
  nsCOMPtr<nsIParser> parser = do_CreateInstance(kCParserCID);
  if (parser) {
    nsCOMPtr<nsIContentSink> sink;
    sink = do_CreateInstance(NS_PLAINTEXTSINK_CONTRACTID);
    if (sink) {
      nsCOMPtr<nsIHTMLToTextSink> textSink(do_QueryInterface(sink));
      if (textSink) {
        nsAutoString convertedText;
        textSink->Initialize(&convertedText, aFlag, 72);
        parser->SetContentSink(sink);
        parser->Parse(aConBuf, 0, NS_LITERAL_CSTRING("text/html"), true);
        aConBuf = convertedText;
      }
    }
  }
}


nsresult
TestASCIIWithFlowedDelSp()
{
  nsString test;
  nsString result;

  test.AssignLiteral("<html><body>"
                     "Firefox Firefox Firefox Firefox "
                     "Firefox Firefox Firefox Firefox "
                     "Firefox Firefox Firefox Firefox"
                     "</body></html>");

  ConvertBufToPlainText(test, nsIDocumentEncoder::OutputFormatted |
                              nsIDocumentEncoder::OutputCRLineBreak |
                              nsIDocumentEncoder::OutputLFLineBreak |
                              nsIDocumentEncoder::OutputFormatFlowed |
                              nsIDocumentEncoder::OutputFormatDelSp);

  
  result.AssignLiteral("Firefox Firefox Firefox Firefox "
                       "Firefox Firefox Firefox Firefox "
                       "Firefox  \r\nFirefox Firefox Firefox\r\n");

  if (!test.Equals(result)) {
    fail("Wrong HTML to ASCII text serialization with format=flowed; delsp=yes");
    return NS_ERROR_FAILURE;
  }

  passed("HTML to ASCII text serialization with format=flowed; delsp=yes");

  return NS_OK;
}


nsresult
TestCJKWithFlowedDelSp()
{
  nsString test;
  nsString result;

  test.AssignLiteral("<html><body>");
  for (PRUint32 i = 0; i < 40; i++) {
    
    test.Append(0x5341);
  }
  test.AppendLiteral("</body></html>");

  ConvertBufToPlainText(test, nsIDocumentEncoder::OutputFormatted |
                              nsIDocumentEncoder::OutputCRLineBreak |
                              nsIDocumentEncoder::OutputLFLineBreak |
                              nsIDocumentEncoder::OutputFormatFlowed |
                              nsIDocumentEncoder::OutputFormatDelSp);

  
  for (PRUint32 i = 0; i < 36; i++) {
    result.Append(0x5341);
  }
  result.Append(NS_LITERAL_STRING(" \r\n"));
  for (PRUint32 i = 0; i < 4; i++) {
    result.Append(0x5341);
  }
  result.Append(NS_LITERAL_STRING("\r\n"));

  if (!test.Equals(result)) {
    fail("Wrong HTML to CJK text serialization with format=flowed; delsp=yes");
    return NS_ERROR_FAILURE;
  }

  passed("HTML to CJK text serialization with format=flowed; delsp=yes");

  return NS_OK;
}

nsresult
TestPrettyPrintedHtml()
{
  nsString test;
  test.AppendLiteral(
    "<html>" NS_LINEBREAK
    "<body>" NS_LINEBREAK
    "  first<br>" NS_LINEBREAK
    "  second<br>" NS_LINEBREAK
    "</body>" NS_LINEBREAK "</html>");

  ConvertBufToPlainText(test, 0);
  if (!test.EqualsLiteral("first" NS_LINEBREAK "second" NS_LINEBREAK)) {
    fail("Wrong prettyprinted html to text serialization");
    return NS_ERROR_FAILURE;
  }

  passed("prettyprinted HTML to text serialization test");
  return NS_OK;
}

nsresult
TestPlainTextSerializer()
{
  nsString test;
  test.AppendLiteral("<html><base>base</base><head><span>span</span></head>"
                     "<body>body</body></html>");
  ConvertBufToPlainText(test, 0);
  if (!test.EqualsLiteral("basespanbody")) {
    fail("Wrong html to text serialization");
    return NS_ERROR_FAILURE;
  }

  passed("HTML to text serialization test");

  nsresult rv = TestASCIIWithFlowedDelSp();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = TestCJKWithFlowedDelSp();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = TestPrettyPrintedHtml();
  NS_ENSURE_SUCCESS(rv, rv);

  
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
