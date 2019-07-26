




#include "mozilla/dom/Element.h"
#include "nsIDOMLinkStyle.h"
#include "nsIDOMStyleSheet.h"
#include "nsIDocument.h"
#include "nsIStyleSheet.h"
#include "nsIURI.h"
#include "nsStyleLinkElement.h"
#include "nsNetUtil.h"
#include "mozilla/dom/ProcessingInstruction.h"
#include "nsUnicharUtils.h"
#include "nsGkAtoms.h"
#include "nsThreadUtils.h"
#include "nsContentUtils.h"

using namespace mozilla;
using namespace dom;

class nsXMLStylesheetPI : public ProcessingInstruction,
                          public nsStyleLinkElement
{
public:
  nsXMLStylesheetPI(already_AddRefed<nsINodeInfo> aNodeInfo, const nsAString& aData);
  virtual ~nsXMLStylesheetPI();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsXMLStylesheetPI,
                                           ProcessingInstruction)

  
  virtual void SetNodeValueInternal(const nsAString& aNodeValue,
                                    mozilla::ErrorResult& aError);

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true);

  
  virtual void OverrideBaseURI(nsIURI* aNewBaseURI);

  
  NS_IMETHOD GetCharset(nsAString& aCharset);

  virtual nsXPCClassInfo* GetClassInfo();
protected:
  nsCOMPtr<nsIURI> mOverriddenBaseURI;

  already_AddRefed<nsIURI> GetStyleSheetURL(bool* aIsInline);
  void GetStyleSheetInfo(nsAString& aTitle,
                         nsAString& aType,
                         nsAString& aMedia,
                         bool* aIsScoped,
                         bool* aIsAlternate);
  virtual nsGenericDOMDataNode* CloneDataNode(nsINodeInfo *aNodeInfo,
                                              bool aCloneText) const;
};



DOMCI_NODE_DATA(XMLStylesheetProcessingInstruction, nsXMLStylesheetPI)

NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(nsXMLStylesheetPI)
  NS_NODE_INTERFACE_TABLE4(nsXMLStylesheetPI, nsIDOMNode,
                           nsIDOMProcessingInstruction, nsIDOMLinkStyle,
                           nsIStyleSheetLinkingElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(XMLStylesheetProcessingInstruction)
NS_INTERFACE_MAP_END_INHERITING(ProcessingInstruction)

NS_IMPL_ADDREF_INHERITED(nsXMLStylesheetPI, ProcessingInstruction)
NS_IMPL_RELEASE_INHERITED(nsXMLStylesheetPI, ProcessingInstruction)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsXMLStylesheetPI,
                                                  ProcessingInstruction)
  tmp->nsStyleLinkElement::Traverse(cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsXMLStylesheetPI,
                                                ProcessingInstruction)
  tmp->nsStyleLinkElement::Unlink();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END


nsXMLStylesheetPI::nsXMLStylesheetPI(already_AddRefed<nsINodeInfo> aNodeInfo,
                                     const nsAString& aData)
  : ProcessingInstruction(aNodeInfo, aData)
{
}

nsXMLStylesheetPI::~nsXMLStylesheetPI()
{
}



nsresult
nsXMLStylesheetPI::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers)
{
  nsresult rv = ProcessingInstruction::BindToTree(aDocument, aParent,
                                                  aBindingParent,
                                                  aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  void (nsXMLStylesheetPI::*update)() = &nsXMLStylesheetPI::UpdateStyleSheetInternal;
  nsContentUtils::AddScriptRunner(NS_NewRunnableMethod(this, update));

  return rv;  
}

void
nsXMLStylesheetPI::UnbindFromTree(bool aDeep, bool aNullParent)
{
  nsCOMPtr<nsIDocument> oldDoc = GetCurrentDoc();

  ProcessingInstruction::UnbindFromTree(aDeep, aNullParent);
  UpdateStyleSheetInternal(oldDoc);
}



void
nsXMLStylesheetPI::SetNodeValueInternal(const nsAString& aNodeValue,
                                        ErrorResult& aError)
{
  nsGenericDOMDataNode::SetNodeValueInternal(aNodeValue, aError);
  if (!aError.Failed()) {
    UpdateStyleSheetInternal(nullptr, true);
  }
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
nsXMLStylesheetPI::GetStyleSheetURL(bool* aIsInline)
{
  *aIsInline = false;

  nsAutoString href;
  if (!GetAttrValue(nsGkAtoms::href, href)) {
    return nullptr;
  }

  nsIURI *baseURL;
  nsAutoCString charset;
  nsIDocument *document = OwnerDoc();
  baseURL = mOverriddenBaseURI ?
            mOverriddenBaseURI.get() :
            document->GetDocBaseURI();
  charset = document->GetDocumentCharacterSet();

  nsCOMPtr<nsIURI> aURI;
  NS_NewURI(getter_AddRefs(aURI), href, charset.get(), baseURL);
  return aURI.forget();
}

void
nsXMLStylesheetPI::GetStyleSheetInfo(nsAString& aTitle,
                                     nsAString& aType,
                                     nsAString& aMedia,
                                     bool* aIsScoped,
                                     bool* aIsAlternate)
{
  aTitle.Truncate();
  aType.Truncate();
  aMedia.Truncate();
  *aIsScoped = false;
  *aIsAlternate = false;

  
  if (!nsContentUtils::InProlog(this)) {
    return;
  }

  nsAutoString data;
  GetData(data);

  nsContentUtils::GetPseudoAttributeValue(data, nsGkAtoms::title, aTitle);

  nsAutoString alternate;
  nsContentUtils::GetPseudoAttributeValue(data,
                                          nsGkAtoms::alternate,
                                          alternate);

  
  if (alternate.EqualsLiteral("yes")) {
    if (aTitle.IsEmpty()) { 
      return;
    }

    *aIsAlternate = true;
  }

  nsContentUtils::GetPseudoAttributeValue(data, nsGkAtoms::media, aMedia);

  nsAutoString type;
  nsContentUtils::GetPseudoAttributeValue(data, nsGkAtoms::type, type);

  nsAutoString mimeType, notUsed;
  nsContentUtils::SplitMimeType(type, mimeType, notUsed);
  if (!mimeType.IsEmpty() && !mimeType.LowerCaseEqualsLiteral("text/css")) {
    aType.Assign(type);
    return;
  }

  
  
  aType.AssignLiteral("text/css");

  return;
}

nsGenericDOMDataNode*
nsXMLStylesheetPI::CloneDataNode(nsINodeInfo *aNodeInfo, bool aCloneText) const
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

  *aInstancePtrResult = nullptr;
  
  nsCOMPtr<nsINodeInfo> ni;
  ni = aNodeInfoManager->GetNodeInfo(nsGkAtoms::processingInstructionTagName,
                                     nullptr, kNameSpaceID_None,
                                     nsIDOMNode::PROCESSING_INSTRUCTION_NODE,
                                     nsGkAtoms::xml_stylesheet);
  NS_ENSURE_TRUE(ni, NS_ERROR_OUT_OF_MEMORY);

  nsXMLStylesheetPI *instance = new nsXMLStylesheetPI(ni.forget(), aData);
  if (!instance) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(*aInstancePtrResult = instance);

  return NS_OK;
}
