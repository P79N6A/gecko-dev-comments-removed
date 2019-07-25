





































#include "nsAutoPtr.h"
#include "nsGenericHTMLElement.h"
#include "nsObjectLoadingContent.h"
#include "nsGkAtoms.h"
#include "nsDOMError.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMSVGDocument.h"
#include "nsIDOMGetSVGDocument.h"
#include "nsIDOMHTMLObjectElement.h"
#include "nsFormSubmission.h"
#include "nsIObjectFrame.h"
#include "nsNPAPIPluginInstance.h"
#include "nsIConstraintValidation.h"

using namespace mozilla::dom;

class nsHTMLObjectElement : public nsGenericHTMLFormElement
                          , public nsObjectLoadingContent
                          , public nsIDOMHTMLObjectElement
                          , public nsIConstraintValidation
                          , public nsIDOMGetSVGDocument
{
public:
  using nsIConstraintValidation::GetValidationMessage;

  nsHTMLObjectElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                      mozilla::dom::FromParser aFromParser = mozilla::dom::NOT_FROM_PARSER);
  virtual ~nsHTMLObjectElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLFormElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLFormElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLFormElement::)

  
  NS_DECL_NSIDOMHTMLOBJECTELEMENT

  
  NS_DECL_NSIDOMGETSVGDOCUMENT

  virtual nsresult BindToTree(nsIDocument *aDocument, nsIContent *aParent,
                              nsIContent *aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);
  virtual nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom *aName,
                           nsIAtom *aPrefix, const nsAString &aValue,
                           PRBool aNotify);
  virtual nsresult UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                             PRBool aNotify);

  virtual PRBool IsHTMLFocusable(PRBool aWithMouse, PRBool *aIsFocusable, PRInt32 *aTabIndex);
  virtual PRUint32 GetDesiredIMEState();

  
  NS_IMETHOD_(PRUint32) GetType() const
  {
    return NS_FORM_OBJECT;
  }

  NS_IMETHOD Reset();
  NS_IMETHOD SubmitNamesValues(nsFormSubmission *aFormSubmission);

  virtual bool IsDisabled() const { return PR_FALSE; }

  virtual nsresult DoneAddingChildren(PRBool aHaveNotified);
  virtual PRBool IsDoneAddingChildren();

  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom *aAttribute,
                                const nsAString &aValue,
                                nsAttrValue &aResult);
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom *aAttribute) const;
  virtual nsEventStates IntrinsicState() const;
  virtual void DestroyContent();

  
  virtual PRUint32 GetCapabilities() const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  nsresult CopyInnerTo(nsGenericElement* aDest) const;

  void StartObjectLoad() { StartObjectLoad(PR_TRUE); }

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(nsHTMLObjectElement,
                                                     nsGenericHTMLFormElement)

  virtual nsXPCClassInfo* GetClassInfo();
private:
  


  NS_HIDDEN_(void) StartObjectLoad(PRBool aNotify);

  



  bool IsFocusableForTabIndex();

  PRPackedBool mIsDoneAddingChildren;
};


NS_IMPL_NS_NEW_HTML_ELEMENT_CHECK_PARSER(Object)


nsHTMLObjectElement::nsHTMLObjectElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                                         FromParser aFromParser)
  : nsGenericHTMLFormElement(aNodeInfo),
    mIsDoneAddingChildren(!aFromParser)
{
  RegisterFreezableElement();
  SetIsNetworkCreated(aFromParser == FROM_PARSER_NETWORK);

  
  SetBarredFromConstraintValidation(PR_TRUE);

  
  AddStatesSilently(NS_EVENT_STATE_LOADING);
}

nsHTMLObjectElement::~nsHTMLObjectElement()
{
  UnregisterFreezableElement();
  DestroyImageLoadingContent();
}

PRBool
nsHTMLObjectElement::IsDoneAddingChildren()
{
  return mIsDoneAddingChildren;
}

nsresult
nsHTMLObjectElement::DoneAddingChildren(PRBool aHaveNotified)
{
  mIsDoneAddingChildren = PR_TRUE;

  
  
  if (IsInDoc()) {
    StartObjectLoad(aHaveNotified);
  }
  return NS_OK;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsHTMLObjectElement)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsHTMLObjectElement,
                                                  nsGenericHTMLFormElement)
  tmp->Traverse(cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(nsHTMLObjectElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLObjectElement, nsGenericElement) 

DOMCI_NODE_DATA(HTMLObjectElement, nsHTMLObjectElement)

NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(nsHTMLObjectElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_BEGIN(nsHTMLObjectElement)
    NS_INTERFACE_TABLE_ENTRY(nsHTMLObjectElement, nsIDOMHTMLObjectElement)
    NS_INTERFACE_TABLE_ENTRY(nsHTMLObjectElement, imgIDecoderObserver)
    NS_INTERFACE_TABLE_ENTRY(nsHTMLObjectElement, nsIRequestObserver)
    NS_INTERFACE_TABLE_ENTRY(nsHTMLObjectElement, nsIStreamListener)
    NS_INTERFACE_TABLE_ENTRY(nsHTMLObjectElement, nsIFrameLoaderOwner)
    NS_INTERFACE_TABLE_ENTRY(nsHTMLObjectElement, nsIObjectLoadingContent)
    NS_INTERFACE_TABLE_ENTRY(nsHTMLObjectElement, nsIImageLoadingContent)
    NS_INTERFACE_TABLE_ENTRY(nsHTMLObjectElement, imgIContainerObserver)
    NS_INTERFACE_TABLE_ENTRY(nsHTMLObjectElement, nsIInterfaceRequestor)
    NS_INTERFACE_TABLE_ENTRY(nsHTMLObjectElement, nsIChannelEventSink)
    NS_INTERFACE_TABLE_ENTRY(nsHTMLObjectElement, nsIConstraintValidation)
    NS_INTERFACE_TABLE_ENTRY(nsHTMLObjectElement, nsIDOMGetSVGDocument)
  NS_OFFSET_AND_INTERFACE_TABLE_END
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLObjectElement,
                                               nsGenericHTMLFormElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLObjectElement)

NS_IMPL_ELEMENT_CLONE(nsHTMLObjectElement)


NS_IMPL_NSICONSTRAINTVALIDATION(nsHTMLObjectElement)

NS_IMETHODIMP
nsHTMLObjectElement::GetForm(nsIDOMHTMLFormElement **aForm)
{
  return nsGenericHTMLFormElement::GetForm(aForm);
}

nsresult
nsHTMLObjectElement::BindToTree(nsIDocument *aDocument,
                                nsIContent *aParent,
                                nsIContent *aBindingParent,
                                PRBool aCompileEventHandlers)
{
  nsresult rv = nsGenericHTMLFormElement::BindToTree(aDocument, aParent,
                                                     aBindingParent,
                                                     aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (mIsDoneAddingChildren) {
    void (nsHTMLObjectElement::*start)() = &nsHTMLObjectElement::StartObjectLoad;
    nsContentUtils::AddScriptRunner(NS_NewRunnableMethod(this, start));
  }

  return NS_OK;
}

void
nsHTMLObjectElement::UnbindFromTree(PRBool aDeep,
                                    PRBool aNullParent)
{
  RemovedFromDocument();
  nsGenericHTMLFormElement::UnbindFromTree(aDeep, aNullParent);
}



nsresult
nsHTMLObjectElement::SetAttr(PRInt32 aNameSpaceID, nsIAtom *aName,
                             nsIAtom *aPrefix, const nsAString &aValue,
                             PRBool aNotify)
{
  
  
  
  
  
  
  
  
  
  if (aNotify && IsInDoc() && mIsDoneAddingChildren &&
      aNameSpaceID == kNameSpaceID_None && aName == nsGkAtoms::data) {
    nsAutoString type;
    GetAttr(kNameSpaceID_None, nsGkAtoms::type, type);
    LoadObject(aValue, aNotify, NS_ConvertUTF16toUTF8(type), PR_TRUE);
  }

  return nsGenericHTMLFormElement::SetAttr(aNameSpaceID, aName, aPrefix,
                                           aValue, aNotify);
}

nsresult
nsHTMLObjectElement::UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                               PRBool aNotify)
{
  if (aNameSpaceID == kNameSpaceID_None && aAttribute == nsGkAtoms::data) {
    Fallback(aNotify);
  }

  return nsGenericHTMLFormElement::UnsetAttr(aNameSpaceID, aAttribute, aNotify);
}

bool
nsHTMLObjectElement::IsFocusableForTabIndex()
{
  nsIDocument* doc = GetCurrentDoc();
  if (!doc || doc->HasFlag(NODE_IS_EDITABLE)) {
    return false;
  }

  return Type() == eType_Plugin || IsEditableRoot() ||
         (Type() == eType_Document && nsContentUtils::IsSubDocumentTabbable(this));
}

PRBool
nsHTMLObjectElement::IsHTMLFocusable(PRBool aWithMouse,
                                     PRBool *aIsFocusable, PRInt32 *aTabIndex)
{
  
  
  nsIDocument *doc = GetCurrentDoc();
  if (!doc || doc->HasFlag(NODE_IS_EDITABLE)) {
    if (aTabIndex) {
      GetIntAttr(nsGkAtoms::tabindex, -1, aTabIndex);
    }

    *aIsFocusable = PR_FALSE;

    return PR_FALSE;
  }

  
  
  if (Type() == eType_Plugin || IsEditableRoot() ||
      (Type() == eType_Document && nsContentUtils::IsSubDocumentTabbable(this))) {
    
    
    if (aTabIndex) {
      GetIntAttr(nsGkAtoms::tabindex, 0, aTabIndex);
    }

    *aIsFocusable = PR_TRUE;

    return PR_FALSE;
  }

  
  
  const nsAttrValue* attrVal = mAttrsAndChildren.GetAttr(nsGkAtoms::tabindex);

  *aIsFocusable = attrVal && attrVal->Type() == nsAttrValue::eInteger;

  if (aTabIndex && *aIsFocusable) {
    *aTabIndex = attrVal->GetIntegerValue();
  }

  return PR_FALSE;
}

PRUint32
nsHTMLObjectElement::GetDesiredIMEState()
{
  if (Type() == eType_Plugin) {
    return nsIContent::IME_STATUS_PLUGIN;
  }
   
  return nsGenericHTMLFormElement::GetDesiredIMEState();
}

NS_IMETHODIMP
nsHTMLObjectElement::Reset()
{
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLObjectElement::SubmitNamesValues(nsFormSubmission *aFormSubmission)
{
  nsAutoString name;
  if (!GetAttr(kNameSpaceID_None, nsGkAtoms::name, name)) {
    

    return NS_OK;
  }

  nsIFrame* frame = GetPrimaryFrame();

  nsIObjectFrame *objFrame = do_QueryFrame(frame);
  if (!objFrame) {
    

    return NS_OK;
  }

  nsRefPtr<nsNPAPIPluginInstance> pi;
  objFrame->GetPluginInstance(getter_AddRefs(pi));
  if (!pi)
    return NS_OK;

  nsAutoString value;
  nsresult rv = pi->GetFormValue(value);
  NS_ENSURE_SUCCESS(rv, rv);

  return aFormSubmission->AddNameValuePair(name, value);
}

NS_IMPL_STRING_ATTR(nsHTMLObjectElement, Align, align)
NS_IMPL_STRING_ATTR(nsHTMLObjectElement, Archive, archive)
NS_IMPL_STRING_ATTR(nsHTMLObjectElement, Border, border)
NS_IMPL_URI_ATTR_WITH_BASE(nsHTMLObjectElement, Code, code, codebase)
NS_IMPL_URI_ATTR(nsHTMLObjectElement, CodeBase, codebase)
NS_IMPL_STRING_ATTR(nsHTMLObjectElement, CodeType, codetype)
NS_IMPL_URI_ATTR_WITH_BASE(nsHTMLObjectElement, Data, data, codebase)
NS_IMPL_BOOL_ATTR(nsHTMLObjectElement, Declare, declare)
NS_IMPL_STRING_ATTR(nsHTMLObjectElement, Height, height)
NS_IMPL_INT_ATTR(nsHTMLObjectElement, Hspace, hspace)
NS_IMPL_STRING_ATTR(nsHTMLObjectElement, Name, name)
NS_IMPL_STRING_ATTR(nsHTMLObjectElement, Standby, standby)
NS_IMPL_INT_ATTR_DEFAULT_VALUE(nsHTMLObjectElement, TabIndex, tabindex,
                               IsFocusableForTabIndex() ? 0 : -1)
NS_IMPL_STRING_ATTR(nsHTMLObjectElement, Type, type)
NS_IMPL_STRING_ATTR(nsHTMLObjectElement, UseMap, usemap)
NS_IMPL_INT_ATTR(nsHTMLObjectElement, Vspace, vspace)
NS_IMPL_STRING_ATTR(nsHTMLObjectElement, Width, width)

NS_IMETHODIMP
nsHTMLObjectElement::GetContentDocument(nsIDOMDocument **aContentDocument)
{
  NS_ENSURE_ARG_POINTER(aContentDocument);

  *aContentDocument = nsnull;

  if (!IsInDoc()) {
    return NS_OK;
  }

  
  nsIDocument *sub_doc = GetOwnerDoc()->GetSubDocumentFor(this);
  if (!sub_doc) {
    return NS_OK;
  }

  return CallQueryInterface(sub_doc, aContentDocument);
}

NS_IMETHODIMP
nsHTMLObjectElement::GetSVGDocument(nsIDOMDocument **aResult)
{
  return GetContentDocument(aResult);
}

PRBool
nsHTMLObjectElement::ParseAttribute(PRInt32 aNamespaceID,
                                    nsIAtom *aAttribute,
                                    const nsAString &aValue,
                                    nsAttrValue &aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::align) {
      return ParseAlignValue(aValue, aResult);
    }
    if (ParseImageAttribute(aAttribute, aValue, aResult)) {
      return PR_TRUE;
    }
  }

  return nsGenericHTMLFormElement::ParseAttribute(aNamespaceID, aAttribute,
                                                  aValue, aResult);
}

static void
MapAttributesIntoRule(const nsMappedAttributes *aAttributes,
                      nsRuleData *aData)
{
  nsGenericHTMLFormElement::MapImageAlignAttributeInto(aAttributes, aData);
  nsGenericHTMLFormElement::MapImageBorderAttributeInto(aAttributes, aData);
  nsGenericHTMLFormElement::MapImageMarginAttributeInto(aAttributes, aData);
  nsGenericHTMLFormElement::MapImageSizeAttributesInto(aAttributes, aData);
  nsGenericHTMLFormElement::MapCommonAttributesInto(aAttributes, aData);
}

NS_IMETHODIMP_(PRBool)
nsHTMLObjectElement::IsAttributeMapped(const nsIAtom *aAttribute) const
{
  static const MappedAttributeEntry* const map[] = {
    sCommonAttributeMap,
    sImageMarginSizeAttributeMap,
    sImageBorderAttributeMap,
    sImageAlignAttributeMap,
  };

  return FindAttributeDependence(aAttribute, map, NS_ARRAY_LENGTH(map));
}


nsMapRuleToAttributesFunc
nsHTMLObjectElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}

void
nsHTMLObjectElement::StartObjectLoad(PRBool aNotify)
{
  nsAutoString type;
  GetAttr(kNameSpaceID_None, nsGkAtoms::type, type);
  NS_ConvertUTF16toUTF8 ctype(type);

  nsAutoString uri;
  if (GetAttr(kNameSpaceID_None, nsGkAtoms::data, uri)) {
    LoadObject(uri, aNotify, ctype);
  }
  else {
    
    
    
    LoadObject(nsnull, aNotify, ctype);
  }
  SetIsNetworkCreated(PR_FALSE);
}

nsEventStates
nsHTMLObjectElement::IntrinsicState() const
{
  return nsGenericHTMLFormElement::IntrinsicState() | ObjectState();
}

PRUint32
nsHTMLObjectElement::GetCapabilities() const
{
  return nsObjectLoadingContent::GetCapabilities() | eSupportClassID;
}

void
nsHTMLObjectElement::DestroyContent()
{
  RemovedFromDocument();
  nsGenericHTMLFormElement::DestroyContent();
}

nsresult
nsHTMLObjectElement::CopyInnerTo(nsGenericElement* aDest) const
{
  nsresult rv = nsGenericHTMLFormElement::CopyInnerTo(aDest);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aDest->GetOwnerDoc()->IsStaticDocument()) {
    CreateStaticClone(static_cast<nsHTMLObjectElement*>(aDest));
  }

  return rv;
}
