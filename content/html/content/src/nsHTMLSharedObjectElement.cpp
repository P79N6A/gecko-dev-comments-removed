





































#include "nsGenericHTMLElement.h"
#include "nsObjectLoadingContent.h"
#include "nsGkAtoms.h"
#include "nsDOMError.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMHTMLAppletElement.h"
#include "nsIDOMHTMLEmbedElement.h"
#include "nsThreadUtils.h"
#ifdef MOZ_SVG
#include "nsIDOMGetSVGDocument.h"
#include "nsIDOMSVGDocument.h"
#endif



#ifdef XP_WIN
#undef GetClassName
#undef GetObject
#endif

class nsHTMLSharedObjectElement : public nsGenericHTMLElement,
                                  public nsObjectLoadingContent,
                                  public nsIDOMHTMLAppletElement,
                                  public nsIDOMHTMLEmbedElement
#ifdef MOZ_SVG
                                  , public nsIDOMGetSVGDocument
#endif
{
public:
  nsHTMLSharedObjectElement(nsINodeInfo *aNodeInfo,
                            PRBool aFromParser = PR_FALSE);
  virtual ~nsHTMLSharedObjectElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLAPPLETELEMENT

  
  

  
  NS_IMETHOD GetSrc(nsAString &aSrc);
  NS_IMETHOD SetSrc(const nsAString &aSrc);
  NS_IMETHOD GetType(nsAString &aType);
  NS_IMETHOD SetType(const nsAString &aType);

#ifdef MOZ_SVG
  
  NS_DECL_NSIDOMGETSVGDOCUMENT
#endif

  virtual nsresult BindToTree(nsIDocument *aDocument, nsIContent *aParent,
                              nsIContent *aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);
  virtual nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom *aName,
                           nsIAtom *aPrefix, const nsAString &aValue,
                           PRBool aNotify);

  NS_IMETHOD GetTabIndex(PRInt32 *aTabIndex);
  NS_IMETHOD SetTabIndex(PRInt32 aTabIndex);
  virtual PRBool IsHTMLFocusable(PRBool *aIsFocusable, PRInt32 *aTabIndex);
  virtual PRUint32 GetDesiredIMEState();

  virtual nsresult DoneAddingChildren(PRBool aHaveNotified);
  virtual PRBool IsDoneAddingChildren();

  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom *aAttribute,
                                const nsAString &aValue,
                                nsAttrValue &aResult);
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom *aAttribute) const;
  virtual PRInt32 IntrinsicState() const;
  virtual void DestroyContent();

  
  virtual PRUint32 GetCapabilities() const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  nsresult CopyInnerTo(nsGenericElement* aDest) const;

  void StartObjectLoad() { StartObjectLoad(PR_TRUE); }

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(nsHTMLSharedObjectElement,
                                                     nsGenericHTMLElement)

private:
  


  NS_HIDDEN_(void) StartObjectLoad(PRBool aNotify);

  void GetTypeAttrValue(nsCString &aValue) const
  {
    if (mNodeInfo->Equals(nsGkAtoms::applet)) {
      aValue.AppendLiteral("application/x-java-vm");
    }
    else {
      nsAutoString type;
      GetAttr(kNameSpaceID_None, nsGkAtoms::type, type);

      CopyUTF16toUTF8(type, aValue);
    }
  }

  nsIAtom *URIAttrName() const
  {
    return mNodeInfo->Equals(nsGkAtoms::applet) ?
           nsGkAtoms::code :
           nsGkAtoms::src;
  }

  
  
  PRPackedBool mIsDoneAddingChildren;
};


NS_IMPL_NS_NEW_HTML_ELEMENT_CHECK_PARSER(SharedObject)


nsHTMLSharedObjectElement::nsHTMLSharedObjectElement(nsINodeInfo *aNodeInfo,
                                                     PRBool aFromParser)
  : nsGenericHTMLElement(aNodeInfo),
    mIsDoneAddingChildren(aNodeInfo->Equals(nsGkAtoms::embed) || !aFromParser)
{
  RegisterFreezableElement();
}

nsHTMLSharedObjectElement::~nsHTMLSharedObjectElement()
{
  UnregisterFreezableElement();
  DestroyImageLoadingContent();
}

PRBool
nsHTMLSharedObjectElement::IsDoneAddingChildren()
{
  return mIsDoneAddingChildren;
}

nsresult
nsHTMLSharedObjectElement::DoneAddingChildren(PRBool aHaveNotified)
{
  if (!mIsDoneAddingChildren) {
    mIsDoneAddingChildren = PR_TRUE;

    
    
    if (IsInDoc()) {
      StartObjectLoad(aHaveNotified);
    }
  }

  return NS_OK;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsHTMLSharedObjectElement)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsHTMLSharedObjectElement,
                                                  nsGenericHTMLElement)
  tmp->Traverse(cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(nsHTMLSharedObjectElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLSharedObjectElement, nsGenericElement) 

NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(nsHTMLSharedObjectElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_AMBIGUOUS_BEGIN(nsHTMLSharedObjectElement,
                                                  nsIDOMHTMLAppletElement)
    NS_INTERFACE_TABLE_ENTRY(nsHTMLSharedObjectElement, nsIRequestObserver)
    NS_INTERFACE_TABLE_ENTRY(nsHTMLSharedObjectElement, nsIStreamListener)
    NS_INTERFACE_TABLE_ENTRY(nsHTMLSharedObjectElement, nsIFrameLoaderOwner)
    NS_INTERFACE_TABLE_ENTRY(nsHTMLSharedObjectElement, imgIContainerObserver)
    NS_INTERFACE_TABLE_ENTRY(nsHTMLSharedObjectElement, nsIObjectLoadingContent)
    NS_INTERFACE_TABLE_ENTRY(nsHTMLSharedObjectElement, imgIDecoderObserver)
    NS_INTERFACE_TABLE_ENTRY(nsHTMLSharedObjectElement, nsIImageLoadingContent)
    NS_INTERFACE_TABLE_ENTRY(nsHTMLSharedObjectElement, nsIInterfaceRequestor)
    NS_INTERFACE_TABLE_ENTRY(nsHTMLSharedObjectElement, nsIChannelEventSink)
  NS_OFFSET_AND_INTERFACE_TABLE_END
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE_AMBIGUOUS(nsHTMLSharedObjectElement,
                                                         nsGenericHTMLElement,
                                                         nsIDOMHTMLAppletElement)
  NS_INTERFACE_MAP_ENTRY_IF_TAG(nsIDOMHTMLAppletElement, applet)
  NS_INTERFACE_MAP_ENTRY_IF_TAG(nsIDOMHTMLEmbedElement, embed)
#ifdef MOZ_SVG
  NS_INTERFACE_MAP_ENTRY_IF_TAG(nsIDOMGetSVGDocument, embed)
#endif
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO_IF_TAG(HTMLAppletElement, applet)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO_IF_TAG(HTMLEmbedElement, embed)
NS_HTML_CONTENT_INTERFACE_MAP_END

NS_IMPL_ELEMENT_CLONE(nsHTMLSharedObjectElement)

nsresult
nsHTMLSharedObjectElement::BindToTree(nsIDocument *aDocument,
                                      nsIContent *aParent,
                                      nsIContent *aBindingParent,
                                      PRBool aCompileEventHandlers)
{
  nsresult rv = nsGenericHTMLElement::BindToTree(aDocument, aParent,
                                                 aBindingParent,
                                                 aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (mIsDoneAddingChildren) {
    nsContentUtils::AddScriptRunner(
      new nsRunnableMethod<nsHTMLSharedObjectElement>(this,
                                                      &nsHTMLSharedObjectElement::StartObjectLoad));
  }

  return NS_OK;
}

void
nsHTMLSharedObjectElement::UnbindFromTree(PRBool aDeep,
                                          PRBool aNullParent)
{
  RemovedFromDocument();
  nsGenericHTMLElement::UnbindFromTree(aDeep, aNullParent);
}



nsresult
nsHTMLSharedObjectElement::SetAttr(PRInt32 aNameSpaceID, nsIAtom *aName,
                                   nsIAtom *aPrefix, const nsAString &aValue,
                                   PRBool aNotify)
{
  
  
  
  
  
  
  
  
  
  if (aNotify && IsInDoc() && mIsDoneAddingChildren &&
      aNameSpaceID == kNameSpaceID_None && aName == URIAttrName()) {
    nsCAutoString type;
    GetTypeAttrValue(type);
    LoadObject(aValue, aNotify, type, PR_TRUE);
  }

  return nsGenericHTMLElement::SetAttr(aNameSpaceID, aName, aPrefix, aValue,
                                       aNotify);
}

PRBool
nsHTMLSharedObjectElement::IsHTMLFocusable(PRBool *aIsFocusable,
                                           PRInt32 *aTabIndex)
{
  if (mNodeInfo->Equals(nsGkAtoms::embed) || Type() == eType_Plugin) {
    
    
    if (aTabIndex) {
      GetTabIndex(aTabIndex);
    }

    *aIsFocusable = PR_TRUE;

    
    return PR_TRUE;
  }

  return nsGenericHTMLElement::IsHTMLFocusable(aIsFocusable, aTabIndex);
}

PRUint32
nsHTMLSharedObjectElement::GetDesiredIMEState()
{
  if (Type() == eType_Plugin) {
    return nsIContent::IME_STATUS_PLUGIN;
  }
   
  return nsGenericHTMLElement::GetDesiredIMEState();
}

NS_IMPL_STRING_ATTR(nsHTMLSharedObjectElement, Align, align)
NS_IMPL_STRING_ATTR(nsHTMLSharedObjectElement, Alt, alt)
NS_IMPL_STRING_ATTR(nsHTMLSharedObjectElement, Archive, archive)
NS_IMPL_STRING_ATTR(nsHTMLSharedObjectElement, Code, code)
NS_IMPL_URI_ATTR(nsHTMLSharedObjectElement, CodeBase, codebase)
NS_IMPL_STRING_ATTR(nsHTMLSharedObjectElement, Height, height)
NS_IMPL_INT_ATTR(nsHTMLSharedObjectElement, Hspace, hspace)
NS_IMPL_STRING_ATTR(nsHTMLSharedObjectElement, Name, name)
NS_IMPL_URI_ATTR_WITH_BASE(nsHTMLSharedObjectElement, Object, object, codebase)
NS_IMPL_URI_ATTR(nsHTMLSharedObjectElement, Src, src)
NS_IMPL_INT_ATTR(nsHTMLSharedObjectElement, TabIndex, tabindex)
NS_IMPL_STRING_ATTR(nsHTMLSharedObjectElement, Type, type)
NS_IMPL_INT_ATTR(nsHTMLSharedObjectElement, Vspace, vspace)
NS_IMPL_STRING_ATTR(nsHTMLSharedObjectElement, Width, width)

#ifdef MOZ_SVG
NS_IMETHODIMP
nsHTMLSharedObjectElement::GetSVGDocument(nsIDOMDocument **aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);

  *aResult = nsnull;

  if (!IsInDoc()) {
    return NS_OK;
  }

  
  nsIDocument *sub_doc = GetOwnerDoc()->GetSubDocumentFor(this);
  if (!sub_doc) {
    return NS_OK;
  }

  return CallQueryInterface(sub_doc, aResult);
}
#endif

PRBool
nsHTMLSharedObjectElement::ParseAttribute(PRInt32 aNamespaceID,
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

  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}

static void
MapAttributesIntoRule(const nsMappedAttributes *aAttributes,
                      nsRuleData *aData)
{
  nsGenericHTMLElement::MapImageBorderAttributeInto(aAttributes, aData);
  nsGenericHTMLElement::MapImageMarginAttributeInto(aAttributes, aData);
  nsGenericHTMLElement::MapImageSizeAttributesInto(aAttributes, aData);
  nsGenericHTMLElement::MapImageAlignAttributeInto(aAttributes, aData);
  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
}

NS_IMETHODIMP_(PRBool)
nsHTMLSharedObjectElement::IsAttributeMapped(const nsIAtom *aAttribute) const
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
nsHTMLSharedObjectElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}

void
nsHTMLSharedObjectElement::StartObjectLoad(PRBool aNotify)
{
  nsCAutoString type;
  GetTypeAttrValue(type);

  nsAutoString uri;
  if (!GetAttr(kNameSpaceID_None, URIAttrName(), uri)) {
    
    
    
    LoadObject(nsnull, aNotify, type);
  }
  else {
    LoadObject(uri, aNotify, type);
  }
}

PRInt32
nsHTMLSharedObjectElement::IntrinsicState() const
{
  return nsGenericHTMLElement::IntrinsicState() | ObjectState();
}

PRUint32
nsHTMLSharedObjectElement::GetCapabilities() const
{
  PRUint32 capabilities = eSupportPlugins | eOverrideServerType;
  if (mNodeInfo->Equals(nsGkAtoms::embed)) {
    capabilities |=
#ifdef MOZ_SVG
                    eSupportSVG |
#endif
                    eSupportImages;
  }

  return capabilities;
}

void
nsHTMLSharedObjectElement::DestroyContent()
{
  RemovedFromDocument();
  nsGenericHTMLElement::DestroyContent();
}

nsresult
nsHTMLSharedObjectElement::CopyInnerTo(nsGenericElement* aDest) const
{
  nsresult rv = nsGenericHTMLElement::CopyInnerTo(aDest);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aDest->GetOwnerDoc()->IsStaticDocument()) {
    CreateStaticClone(static_cast<nsHTMLSharedObjectElement*>(aDest));
  }

  return rv;
}
