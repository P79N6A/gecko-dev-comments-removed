





































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
#include "nsThreadUtils.h"

class nsXMLStylesheetPI : public nsXMLProcessingInstruction,
                          public nsStyleLinkElement
{
public:
  nsXMLStylesheetPI(already_AddRefed<nsINodeInfo> aNodeInfo, const nsAString& aData);
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

  virtual nsXPCClassInfo* GetClassInfo();
protected:
  nsCOMPtr<nsIURI> mOverriddenBaseURI;

  already_AddRefed<nsIURI> GetStyleSheetURL(PRBool* aIsInline);
  void GetStyleSheetInfo(nsAString& aTitle,
                         nsAString& aType,
                         nsAString& aMedia,
                         PRBool* aIsAlternate);
  virtual nsGenericDOMDataNode* CloneDataNode(nsINodeInfo *aNodeInfo,
                                              PRBool aCloneText) const;
};



DOMCI_NODE_DATA(XMLStylesheetProcessingInstruction, nsXMLStylesheetPI)

NS_INTERFACE_TABLE_HEAD(nsXMLStylesheetPI)
  NS_NODE_INTERFACE_TABLE4(nsXMLStylesheetPI, nsIDOMNode,
                           nsIDOMProcessingInstruction, nsIDOMLinkStyle,
                           nsIStyleSheetLinkingElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(XMLStylesheetProcessingInstruction)
NS_INTERFACE_MAP_END_INHERITING(nsXMLProcessingInstruction)

NS_IMPL_ADDREF_INHERITED(nsXMLStylesheetPI, nsXMLProcessingInstruction)
NS_IMPL_RELEASE_INHERITED(nsXMLStylesheetPI, nsXMLProcessingInstruction)


nsXMLStylesheetPI::nsXMLStylesheetPI(already_AddRefed<nsINodeInfo> aNodeInfo,
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

  void (nsXMLStylesheetPI::*update)() = &nsXMLStylesheetPI::UpdateStyleSheetInternal;
  nsContentUtils::AddScriptRunner(NS_NewRunnableMethod(this, update));

  return rv;  
}

void
nsXMLStylesheetPI::UnbindFromTree(PRBool aDeep, PRBool aNullParent)
{
  nsCOMPtr<nsIDocument> oldDoc = GetCurrentDoc();

  nsXMLProcessingInstruction::UnbindFromTree(aDeep, aNullParent);
  UpdateStyleSheetInternal(oldDoc);
}



NS_IMETHODIMP
nsXMLStylesheetPI::SetNodeValue(const nsAString& aNodeValue)
{
  nsresult rv = nsGenericDOMDataNode::SetNodeValue(aNodeValue);
  if (NS_SUCCEEDED(rv)) {
    UpdateStyleSheetInternal(nsnull, PR_TRUE);
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

already_AddRefed<nsIURI>
nsXMLStylesheetPI::GetStyleSheetURL(PRBool* aIsInline)
{
  *aIsInline = PR_FALSE;

  nsAutoString href;
  if (!GetAttrValue(nsGkAtoms::href, href)) {
    return nsnull;
  }

  nsIURI *baseURL;
  nsCAutoString charset;
  nsIDocument *document = GetOwnerDoc();
  if (document) {
    baseURL = mOverriddenBaseURI ?
              mOverriddenBaseURI.get() :
              document->GetDocBaseURI();
    charset = document->GetDocumentCharacterSet();
  } else {
    baseURL = mOverriddenBaseURI;
  }

  nsCOMPtr<nsIURI> aURI;
  NS_NewURI(getter_AddRefs(aURI), href, charset.get(), baseURL);
  return aURI.forget();
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
  nsCOMPtr<nsINodeInfo> ni = aNodeInfo;
  return new nsXMLStylesheetPI(ni.forget(), data);
}

nsresult
NS_NewXMLStylesheetProcessingInstruction(nsIContent** aInstancePtrResult,
                                         nsNodeInfoManager *aNodeInfoManager,
                                         const nsAString& aData)
{
  NS_PRECONDITION(aNodeInfoManager, "Missing nodeinfo manager");

  *aInstancePtrResult = nsnull;
  
  nsCOMPtr<nsINodeInfo> ni;
  ni = aNodeInfoManager->GetNodeInfo(nsGkAtoms::processingInstructionTagName,
                                     nsnull, kNameSpaceID_None);
  NS_ENSURE_TRUE(ni, NS_ERROR_OUT_OF_MEMORY);

  nsXMLStylesheetPI *instance = new nsXMLStylesheetPI(ni.forget(), aData);
  if (!instance) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(*aInstancePtrResult = instance);

  return NS_OK;
}
