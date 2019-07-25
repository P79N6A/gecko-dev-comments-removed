



































#include "nsString.h"
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
#include "nsContentUtils.h"
#include "nsIContentSink.h"
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
#include "nsParserUtils.h"
#include "nsAutoPtr.h"
#include "nsTreeSanitizer.h"
#include "nsHtml5Module.h"

#define XHTML_DIV_TAG "div xmlns=\"http://www.w3.org/1999/xhtml\""

NS_IMPL_ISUPPORTS2(nsParserUtils,
                   nsIScriptableUnescapeHTML,
                   nsIParserUtils)

static NS_DEFINE_CID(kCParserCID, NS_PARSER_CID);



NS_IMETHODIMP
nsParserUtils::ConvertToPlainText(const nsAString & aFromStr,
                                           PRUint32 aFlags,
                                           PRUint32 aWrapCol,
                                           nsAString & aToStr)
{
  return nsContentUtils::ConvertToPlainText(aFromStr,
    aToStr,
    aFlags,
    aWrapCol);
}

NS_IMETHODIMP
nsParserUtils::Unescape(const nsAString & aFromStr,
                                 nsAString & aToStr)
{
  return nsContentUtils::ConvertToPlainText(aFromStr,
    aToStr,
    nsIDocumentEncoder::OutputSelectionOnly |
    nsIDocumentEncoder::OutputAbsoluteLinks,
    0);
}




NS_IMETHODIMP
nsParserUtils::ParseFragment(const nsAString &aFragment,
                                      bool aIsXML,
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
  bool scripts_enabled = false;
  if (document) {
    loader = document->ScriptLoader();
    scripts_enabled = loader->GetEnabled();
  }
  if (scripts_enabled) {
    loader->SetEnabled(false);
  }

  
  
  nsAutoTArray<nsString, 2> tagStack;
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
      tagStack.AppendElement(NS_ConvertUTF8toUTF16(base));
    }  else {
      tagStack.AppendElement(NS_LITERAL_STRING(XHTML_DIV_TAG));
    }
  }

  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIContent> fragment;
    if (aIsXML) {
      rv = nsContentUtils::ParseFragmentXML(aFragment,
                                            document,
                                            tagStack,
                                            true,
                                            aReturn);
      fragment = do_QueryInterface(*aReturn);
    } else {
      NS_NewDocumentFragment(aReturn,
                             document->NodeInfoManager());
      fragment = do_QueryInterface(*aReturn);
      rv = nsContentUtils::ParseFragmentHTML(aFragment,
                                             fragment,
                                             nsGkAtoms::body,
                                             kNameSpaceID_XHTML,
                                             false,
                                             true);
      
      if (aBaseURI) {
        aBaseURI->GetSpec(spec);
        nsAutoString spec16;
        CopyUTF8toUTF16(spec, spec16);
        nsIContent* node = fragment->GetFirstChild();
        while (node) {
          if (node->IsElement()) {
            node->SetAttr(kNameSpaceID_XML,
                          nsGkAtoms::base,
                          nsGkAtoms::xml,
                          spec16,
                          false);
          }
          node = node->GetNextSibling();
        }
      }
    }
    if (fragment) {
      nsTreeSanitizer sanitizer(false, false);
      sanitizer.Sanitize(fragment);
    }
  }

  if (scripts_enabled)
      loader->SetEnabled(true);
  
  return rv;
}
