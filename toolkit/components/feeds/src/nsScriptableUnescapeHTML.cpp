



































#include "nsString.h"
#include "nsISupportsArray.h"
#include "nsIComponentManager.h"
#include "nsCOMPtr.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsXPIDLString.h"
#include "nsScriptLoader.h"
#include "nsEscape.h"
#include "nsIParser.h"
#include "nsIDTD.h"
#include "nsNetCID.h"
#include "nsNetUtil.h"
#include "nsParserCIID.h"
#include "nsParserCIID.h"
#include "nsIContentSink.h"
#include "nsIHTMLToTextSink.h"
#include "nsIDocumentEncoder.h"
#include "nsIDOMDocumentFragment.h"
#include "nsIFragmentContentSink.h"
#include "nsIDOMDocument.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMNode.h"
#include "nsIDOMElement.h"
#include "nsIDocument.h"
#include "nsIContent.h"
#include "nsAttrName.h"
#include "nsHTMLParts.h"
#include "nsContentCID.h"
#include "nsIScriptableUnescapeHTML.h"
#include "nsScriptableUnescapeHTML.h"
#include "nsAutoPtr.h"

#define XHTML_DIV_TAG "div xmlns=\"http://www.w3.org/1999/xhtml\""
#define HTML_BODY_TAG "BODY"
#define HTML_BASE_TAG "BASE"

NS_IMPL_ISUPPORTS1(nsScriptableUnescapeHTML, nsIScriptableUnescapeHTML)

static NS_DEFINE_CID(kCParserCID, NS_PARSER_CID);





NS_IMETHODIMP
nsScriptableUnescapeHTML::Unescape(const nsAString & aFromStr, 
                                   nsAString & aToStr)
{
  
  aToStr.SetLength(0);
  nsresult rv;
  nsCOMPtr<nsIParser> parser = do_CreateInstance(kCParserCID, &rv);
  if (NS_FAILED(rv)) return rv;

  
  nsCOMPtr<nsIContentSink> sink;

  sink = do_CreateInstance(NS_PLAINTEXTSINK_CONTRACTID);
  NS_ENSURE_TRUE(sink, NS_ERROR_FAILURE);

  nsCOMPtr<nsIHTMLToTextSink> textSink(do_QueryInterface(sink));
  NS_ENSURE_TRUE(textSink, NS_ERROR_FAILURE);

  textSink->Initialize(&aToStr, nsIDocumentEncoder::OutputSelectionOnly
                       | nsIDocumentEncoder::OutputAbsoluteLinks, 0);

  parser->SetContentSink(sink);

  parser->Parse(aFromStr, 0, NS_LITERAL_CSTRING("text/html"),
                PR_TRUE, eDTDMode_fragment);
  
  return NS_OK;
}




NS_IMETHODIMP
nsScriptableUnescapeHTML::ParseFragment(const nsAString &aFragment,
                                        PRBool aIsXML,
                                        nsIURI* aBaseURI,
                                        nsIDOMElement* aContextElement,
                                        nsIDOMDocumentFragment** aReturn)
{
  NS_ENSURE_ARG(aContextElement);
  *aReturn = nsnull;

  nsresult rv;
  nsCOMPtr<nsIParser> parser = do_CreateInstance(kCParserCID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDocument> document;
  nsCOMPtr<nsIDOMDocument> domDocument;
  nsCOMPtr<nsIDOMNode> contextNode;
  contextNode = do_QueryInterface(aContextElement);
  contextNode->GetOwnerDocument(getter_AddRefs(domDocument));
  document = do_QueryInterface(domDocument);
  NS_ENSURE_TRUE(document, NS_ERROR_NOT_AVAILABLE);

  
  nsRefPtr<nsScriptLoader> loader;
  PRBool scripts_enabled = PR_FALSE;
  if (document) {
    loader = document->ScriptLoader();
    scripts_enabled = loader->GetEnabled();
  }
  if (scripts_enabled) {
    loader->SetEnabled(PR_FALSE);
  }

  
  
  nsVoidArray tagStack;
  nsCAutoString base, spec;
  if (aIsXML) {
    
    if (aBaseURI) {
      base.Append(NS_LITERAL_CSTRING(XHTML_DIV_TAG));
      base.Append(NS_LITERAL_CSTRING(" xml:base=\""));
      aBaseURI->GetSpec(spec);
      
      
      char* escapedSpec = nsEscapeHTML(spec.get());
      if (escapedSpec)
        base += escapedSpec;
      NS_Free(escapedSpec);
      base.Append(NS_LITERAL_CSTRING("\""));
      tagStack.AppendElement(ToNewUnicode(base));
    }  else {
      tagStack.AppendElement(ToNewUnicode(NS_LITERAL_CSTRING(XHTML_DIV_TAG)));
    }
  } else {
    
    tagStack.AppendElement(ToNewUnicode(NS_LITERAL_CSTRING(HTML_BODY_TAG)));
    if (aBaseURI) {
      base.Append(NS_LITERAL_CSTRING((HTML_BASE_TAG)));
      base.Append(NS_LITERAL_CSTRING(" href=\""));
      aBaseURI->GetSpec(spec);
      base = base + spec;
      base.Append(NS_LITERAL_CSTRING("\""));
      tagStack.AppendElement(ToNewUnicode(base));
    }
  }

  if (NS_SUCCEEDED(rv)) {
    nsCAutoString contentType;
    nsDTDMode mode;
    nsCOMPtr<nsIFragmentContentSink> sink;
    if (aIsXML) {
      mode = eDTDMode_full_standards;
      contentType = NS_LITERAL_CSTRING("application/xhtml+xml");
      sink = do_CreateInstance(NS_XHTMLPARANOIDFRAGMENTSINK_CONTRACTID);
    } else {
      mode = eDTDMode_fragment;
      contentType = NS_LITERAL_CSTRING("text/html");
      sink = do_CreateInstance(NS_HTMLPARANOIDFRAGMENTSINK_CONTRACTID);
    }
    if (sink) {
      sink->SetTargetDocument(document);
      nsCOMPtr<nsIContentSink> contentsink(do_QueryInterface(sink));
      parser->SetContentSink(contentsink);
      rv = parser->ParseFragment(aFragment, nsnull, tagStack,
                                 aIsXML, contentType, mode);
      if (NS_SUCCEEDED(rv))
        rv = sink->GetFragment(aReturn);

    } else {
      rv = NS_ERROR_FAILURE;
    }
  }

  
  PRInt32 count = tagStack.Count();
  for (PRInt32 i = 0; i < count; i++) {
    PRUnichar* str = (PRUnichar*)tagStack.ElementAt(i);
    if (str)
      NS_Free(str);
  }

  if (scripts_enabled)
      loader->SetEnabled(PR_TRUE);
  
  return rv;
}
