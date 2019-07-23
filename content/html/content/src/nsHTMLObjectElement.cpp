





































#include "nsGenericHTMLElement.h"
#include "nsObjectLoadingContent.h"
#include "nsGkAtoms.h"
#include "nsDOMError.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMHTMLObjectElement.h"
#include "nsIFormSubmission.h"
#include "nsIObjectFrame.h"
#include "nsIPluginInstance.h"
#include "nsIPluginInstanceInternal.h"

class nsHTMLObjectElement : public nsGenericHTMLFormElement,
                            public nsObjectLoadingContent,
                            public nsIDOMHTMLObjectElement
{
public:
  nsHTMLObjectElement(nsINodeInfo *aNodeInfo, PRBool aFromParser = PR_FALSE);
  virtual ~nsHTMLObjectElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLFormElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLFormElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLFormElement::)

  
  NS_DECL_NSIDOMHTMLOBJECTELEMENT

  virtual nsresult BindToTree(nsIDocument *aDocument, nsIContent *aParent,
                              nsIContent *aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);
  virtual nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom *aName,
                           nsIAtom *aPrefix, const nsAString &aValue,
                           PRBool aNotify);

  virtual PRBool IsFocusable(PRInt32 *aTabIndex = nsnull);
  virtual PRUint32 GetDesiredIMEState();

  
  NS_IMETHOD_(PRInt32) GetType() const
  {
    return NS_FORM_OBJECT;
  }

  NS_IMETHOD Reset();
  NS_IMETHOD SubmitNamesValues(nsIFormSubmission *aFormSubmission,
                               nsIContent *aSubmitElement);

  virtual nsresult DoneAddingChildren(PRBool aHaveNotified);
  virtual PRBool IsDoneAddingChildren();

  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom *aAttribute,
                                const nsAString &aValue,
                                nsAttrValue &aResult);
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom *aAttribute) const;
  virtual PRInt32 IntrinsicState() const;

  
  virtual PRUint32 GetCapabilities() const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(nsHTMLObjectElement,
                                                     nsGenericHTMLElement)

private:
  


  NS_HIDDEN_(void) StartObjectLoad(PRBool aNotify);

  PRPackedBool mIsDoneAddingChildren;
};


NS_IMPL_NS_NEW_HTML_ELEMENT_CHECK_PARSER(Object)


nsHTMLObjectElement::nsHTMLObjectElement(nsINodeInfo *aNodeInfo,
                                         PRBool aFromParser)
  : nsGenericHTMLFormElement(aNodeInfo),
    mIsDoneAddingChildren(!aFromParser)
{
}

nsHTMLObjectElement::~nsHTMLObjectElement()
{
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

NS_HTML_CONTENT_CC_INTERFACE_MAP_BEGIN(nsHTMLObjectElement,
                                       nsGenericHTMLFormElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHTMLObjectElement)
  NS_INTERFACE_MAP_ENTRY(imgIDecoderObserver)
  NS_INTERFACE_MAP_ENTRY(nsIRequestObserver)
  NS_INTERFACE_MAP_ENTRY(nsIStreamListener)
  NS_INTERFACE_MAP_ENTRY(nsIFrameLoaderOwner)
  NS_INTERFACE_MAP_ENTRY(nsIObjectLoadingContent)
  NS_INTERFACE_MAP_ENTRY(nsIImageLoadingContent)
  NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
  NS_INTERFACE_MAP_ENTRY(nsIChannelEventSink)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(HTMLObjectElement)
NS_HTML_CONTENT_INTERFACE_MAP_END

NS_IMPL_ELEMENT_CLONE(nsHTMLObjectElement)


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
    
    
    StartObjectLoad(PR_FALSE);
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

PRBool
nsHTMLObjectElement::IsFocusable(PRInt32 *aTabIndex)
{
  if (Type() == eType_Plugin) {
    
    
    if (aTabIndex) {
      GetTabIndex(aTabIndex);
    }
  
    return PR_TRUE;
  }

  return nsGenericHTMLFormElement::IsFocusable(aTabIndex);
}

PRUint32
nsHTMLObjectElement::GetDesiredIMEState()
{
  if (Type() == eType_Plugin) {
    return nsIContent::IME_STATUS_ENABLE;
  }
   
  return nsGenericHTMLFormElement::GetDesiredIMEState();
}

NS_IMETHODIMP
nsHTMLObjectElement::Reset()
{
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLObjectElement::SubmitNamesValues(nsIFormSubmission *aFormSubmission,
                                       nsIContent *aSubmitElement)
{
  nsAutoString name;
  if (!GetAttr(kNameSpaceID_None, nsGkAtoms::name, name)) {
    

    return NS_OK;
  }

  nsIFrame* frame = GetPrimaryFrame();

  nsIObjectFrame *objFrame = nsnull;
  if (frame) {
    CallQueryInterface(frame, &objFrame);
  }

  if (!objFrame) {
    

    return NS_OK;
  }

  nsCOMPtr<nsIPluginInstance> pi;
  objFrame->GetPluginInstance(*getter_AddRefs(pi));

  nsCOMPtr<nsIPluginInstanceInternal> pi_internal(do_QueryInterface(pi));

  if (!pi_internal) {
    

    return NS_OK;
  }

  nsAutoString value;
  nsresult rv = pi_internal->GetFormValue(value);
  NS_ENSURE_SUCCESS(rv, rv);

  return aFormSubmission->AddNameValuePair(this, name, value);
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
NS_IMPL_INT_ATTR(nsHTMLObjectElement, TabIndex, tabindex)
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
}

PRInt32
nsHTMLObjectElement::IntrinsicState() const
{
  return nsGenericHTMLFormElement::IntrinsicState() | ObjectState();
}

PRUint32
nsHTMLObjectElement::GetCapabilities() const
{
  return nsObjectLoadingContent::GetCapabilities() | eSupportClassID;
}
