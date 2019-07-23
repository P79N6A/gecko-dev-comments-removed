





































#include "nsIDOMLinkStyle.h"
#include "nsIDOMStyleSheet.h"
#include "nsIDocument.h"
#include "nsIStyleSheet.h"
#include "nsIURI.h"
#include "nsStyleLinkElement.h"
#include "nsNetUtil.h"
#include "nsXMLProcessingInstruction.h"
#include "nsUnicharUtils.h"
#include "nsParserUtils.h"
#include "nsGkAtoms.h"

class nsXMLStylesheetPI : public nsXMLProcessingInstruction,
                          public nsStyleLinkElement
{
public:
  nsXMLStylesheetPI(nsINodeInfo *aNodeInfo, const nsAString& aData);
  virtual ~nsXMLStylesheetPI();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD SetNodeValue(const nsAString& aData);

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);

  
  virtual void OverrideBaseURI(nsIURI* aNewBaseURI);

  
  NS_IMETHOD GetCharset(nsAString& aCharset);

protected:
  nsCOMPtr<nsIURI> mOverriddenBaseURI;

  void GetStyleSheetURL(PRBool* aIsInline,
                        nsIURI** aURI);
  void GetStyleSheetInfo(nsAString& aTitle,
                         nsAString& aType,
                         nsAString& aMedia,
                         PRBool* aIsAlternate);
  virtual nsGenericDOMDataNode* CloneDataNode(nsINodeInfo *aNodeInfo,
                                              PRBool aCloneText) const;
};



NS_INTERFACE_MAP_BEGIN(nsXMLStylesheetPI)
  NS_INTERFACE_MAP_ENTRY(nsIDOMLinkStyle)
  NS_INTERFACE_MAP_ENTRY(nsIStyleSheetLinkingElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(XMLStylesheetProcessingInstruction)
NS_INTERFACE_MAP_END_INHERITING(nsXMLProcessingInstruction)

NS_IMPL_ADDREF_INHERITED(nsXMLStylesheetPI, nsXMLProcessingInstruction)
NS_IMPL_RELEASE_INHERITED(nsXMLStylesheetPI, nsXMLProcessingInstruction)


nsXMLStylesheetPI::nsXMLStylesheetPI(nsINodeInfo *aNodeInfo,
                                     const nsAString& aData)
  : nsXMLProcessingInstruction(aNodeInfo, NS_LITERAL_STRING("xml-stylesheet"),
                               aData)
{
}

nsXMLStylesheetPI::~nsXMLStylesheetPI()
{
}



nsresult
nsXMLStylesheetPI::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers)
{
  nsresult rv = nsXMLProcessingInstruction::BindToTree(aDocument, aParent,
                                                       aBindingParent,
                                                       aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  UpdateStyleSheet(nsnull);

  return rv;  
}

void
nsXMLStylesheetPI::UnbindFromTree(PRBool aDeep, PRBool aNullParent)
{
  nsCOMPtr<nsIDocument> oldDoc = GetCurrentDoc();

  nsXMLProcessingInstruction::UnbindFromTree(aDeep, aNullParent);
  UpdateStyleSheet(oldDoc);
}



NS_IMETHODIMP
nsXMLStylesheetPI::SetNodeValue(const nsAString& aNodeValue)
{
  nsresult rv = nsGenericDOMDataNode::SetNodeValue(aNodeValue);
  if (NS_SUCCEEDED(rv)) {
    UpdateStyleSheet(nsnull, nsnull, PR_TRUE);
  }
  return rv;
}



NS_IMETHODIMP
nsXMLStylesheetPI::GetCharset(nsAString& aCharset)
{
  return GetAttrValue(nsGkAtoms::charset, aCharset) ? NS_OK : NS_ERROR_FAILURE;
}

 void
nsXMLStylesheetPI::OverrideBaseURI(nsIURI* aNewBaseURI)
{
  mOverriddenBaseURI = aNewBaseURI;
}

void
nsXMLStylesheetPI::GetStyleSheetURL(PRBool* aIsInline,
                                    nsIURI** aURI)
{
  *aIsInline = PR_FALSE;
  *aURI = nsnull;

  nsAutoString href;
  GetAttrValue(nsGkAtoms::href, href);
  if (href.IsEmpty()) {
    return;
  }

  nsIURI *baseURL;
  nsCAutoString charset;
  nsIDocument *document = GetOwnerDoc();
  if (document) {
    baseURL = mOverriddenBaseURI ? mOverriddenBaseURI.get() : document->GetBaseURI();
    charset = document->GetDocumentCharacterSet();
  } else {
    baseURL = mOverriddenBaseURI;
  }

  NS_NewURI(aURI, href, charset.get(), baseURL);
}

void
nsXMLStylesheetPI::GetStyleSheetInfo(nsAString& aTitle,
                                     nsAString& aType,
                                     nsAString& aMedia,
                                     PRBool* aIsAlternate)
{
  aTitle.Truncate();
  aType.Truncate();
  aMedia.Truncate();
  *aIsAlternate = PR_FALSE;

  
  if (!nsContentUtils::InProlog(this)) {
    return;
  }

  nsAutoString data;
  GetData(data);

  nsParserUtils::GetQuotedAttributeValue(data, nsGkAtoms::title, aTitle);

  nsAutoString alternate;
  nsParserUtils::GetQuotedAttributeValue(data, nsGkAtoms::alternate, alternate);

  
  if (alternate.EqualsLiteral("yes")) {
    if (aTitle.IsEmpty()) { 
      return;
    }

    *aIsAlternate = PR_TRUE;
  }

  nsParserUtils::GetQuotedAttributeValue(data, nsGkAtoms::media, aMedia);

  nsAutoString type;
  nsParserUtils::GetQuotedAttributeValue(data, nsGkAtoms::type, type);

  nsAutoString mimeType, notUsed;
  nsParserUtils::SplitMimeType(type, mimeType, notUsed);
  if (!mimeType.IsEmpty() && !mimeType.LowerCaseEqualsLiteral("text/css")) {
    aType.Assign(type);
    return;
  }

  
  
  aType.AssignLiteral("text/css");

  return;
}

nsGenericDOMDataNode*
nsXMLStylesheetPI::CloneDataNode(nsINodeInfo *aNodeInfo, PRBool aCloneText) const
{
  nsAutoString data;
  nsGenericDOMDataNode::GetData(data);

  return new nsXMLStylesheetPI(aNodeInfo, data);
}

nsresult
NS_NewXMLStylesheetProcessingInstruction(nsIContent** aInstancePtrResult,
                                         nsNodeInfoManager *aNodeInfoManager,
                                         const nsAString& aData)
{
  NS_PRECONDITION(aNodeInfoManager, "Missing nodeinfo manager");

  *aInstancePtrResult = nsnull;
  
  nsCOMPtr<nsINodeInfo> ni;
  nsresult rv =
    aNodeInfoManager->GetNodeInfo(nsGkAtoms::processingInstructionTagName,
                                  nsnull, kNameSpaceID_None,
                                  getter_AddRefs(ni));
  NS_ENSURE_SUCCESS(rv, rv);

  nsXMLStylesheetPI *instance = new nsXMLStylesheetPI(ni, aData);
  if (!instance) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(*aInstancePtrResult = instance);

  return NS_OK;
}
