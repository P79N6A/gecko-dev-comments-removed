



































#include "nsIDOMHTMLImageElement.h"
#include "nsIDOMEventTarget.h"
#include "nsGenericHTMLElement.h"
#include "nsImageLoadingContent.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsMappedAttributes.h"
#include "nsIJSNativeInitializer.h"
#include "nsSize.h"
#include "nsIDocument.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDOMDocument.h"
#include "nsIScriptContext.h"
#include "nsIURL.h"
#include "nsIIOService.h"
#include "nsIServiceManager.h"
#include "nsNetUtil.h"
#include "nsContentUtils.h"
#include "nsIFrame.h"
#include "nsNodeInfoManager.h"
#include "nsGUIEvent.h"
#include "nsContentPolicyUtils.h"
#include "nsIDOMWindow.h"
#include "nsFocusManager.h"

#include "imgIContainer.h"
#include "imgILoader.h"
#include "imgIRequest.h"
#include "imgIDecoderObserver.h"

#include "nsILoadGroup.h"

#include "nsRuleData.h"

#include "nsIJSContextStack.h"
#include "nsImageMapUtils.h"
#include "nsIDOMHTMLMapElement.h"
#include "nsEventDispatcher.h"

#include "nsLayoutUtils.h"
#include "mozilla/Preferences.h"

using namespace mozilla;
using namespace mozilla::dom;



class nsHTMLImageElement : public nsGenericHTMLElement,
                           public nsImageLoadingContent,
                           public nsIDOMHTMLImageElement,
                           public nsIJSNativeInitializer
{
public:
  nsHTMLImageElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLImageElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLIMAGEELEMENT

  
  NS_IMETHOD GetDraggable(PRBool* aDraggable);

  
  NS_IMETHOD Initialize(nsISupports* aOwner, JSContext* aContext,
                        JSObject* aObj, PRUint32 argc, jsval* argv);

  
  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              PRInt32 aModType) const;
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;

  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);

  PRBool IsHTMLFocusable(PRBool aWithMouse, PRBool *aIsFocusable, PRInt32 *aTabIndex);

  
  
  nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, PRBool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nsnull, aValue, aNotify);
  }
  virtual nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           PRBool aNotify);
  virtual nsresult UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                             PRBool aNotify);

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);

  virtual nsEventStates IntrinsicState() const;
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  nsresult CopyInnerTo(nsGenericElement* aDest) const;

  void MaybeLoadImage();
  virtual nsXPCClassInfo* GetClassInfo();
protected:
  nsSize GetWidthHeight();
};

nsGenericHTMLElement*
NS_NewHTMLImageElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                       FromParser aFromParser)
{
  




  nsCOMPtr<nsINodeInfo> nodeInfo(aNodeInfo);
  if (!nodeInfo) {
    nsCOMPtr<nsIDocument> doc =
      do_QueryInterface(nsContentUtils::GetDocumentFromCaller());
    NS_ENSURE_TRUE(doc, nsnull);

    nodeInfo = doc->NodeInfoManager()->GetNodeInfo(nsGkAtoms::img, nsnull,
                                                   kNameSpaceID_XHTML);
    NS_ENSURE_TRUE(nodeInfo, nsnull);
  }

  return new nsHTMLImageElement(nodeInfo.forget());
}

nsHTMLImageElement::nsHTMLImageElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

nsHTMLImageElement::~nsHTMLImageElement()
{
  DestroyImageLoadingContent();
}


NS_IMPL_ADDREF_INHERITED(nsHTMLImageElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLImageElement, nsGenericElement)


DOMCI_NODE_DATA(HTMLImageElement, nsHTMLImageElement)


NS_INTERFACE_TABLE_HEAD(nsHTMLImageElement)
  NS_HTML_CONTENT_INTERFACE_TABLE5(nsHTMLImageElement,
                                   nsIDOMHTMLImageElement,
                                   nsIJSNativeInitializer,
                                   imgIDecoderObserver,
                                   nsIImageLoadingContent,
                                   imgIContainerObserver)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLImageElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLImageElement)


NS_IMPL_ELEMENT_CLONE(nsHTMLImageElement)


NS_IMPL_STRING_ATTR(nsHTMLImageElement, Name, name)
NS_IMPL_STRING_ATTR(nsHTMLImageElement, Align, align)
NS_IMPL_STRING_ATTR(nsHTMLImageElement, Alt, alt)
NS_IMPL_STRING_ATTR(nsHTMLImageElement, Border, border)
NS_IMPL_INT_ATTR(nsHTMLImageElement, Hspace, hspace)
NS_IMPL_BOOL_ATTR(nsHTMLImageElement, IsMap, ismap)
NS_IMPL_URI_ATTR(nsHTMLImageElement, LongDesc, longdesc)
NS_IMPL_STRING_ATTR(nsHTMLImageElement, Lowsrc, lowsrc)
NS_IMPL_URI_ATTR(nsHTMLImageElement, Src, src)
NS_IMPL_STRING_ATTR(nsHTMLImageElement, UseMap, usemap)
NS_IMPL_INT_ATTR(nsHTMLImageElement, Vspace, vspace)

NS_IMETHODIMP
nsHTMLImageElement::GetDraggable(PRBool* aDraggable)
{
  
  *aDraggable = !AttrValueIs(kNameSpaceID_None, nsGkAtoms::draggable,
                             nsGkAtoms::_false, eIgnoreCase);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLImageElement::GetComplete(PRBool* aComplete)
{
  NS_PRECONDITION(aComplete, "Null out param!");
  *aComplete = PR_TRUE;

  if (!mCurrentRequest) {
    return NS_OK;
  }

  PRUint32 status;
  mCurrentRequest->GetImageStatus(&status);
  *aComplete =
    (status &
     (imgIRequest::STATUS_LOAD_COMPLETE | imgIRequest::STATUS_ERROR)) != 0;

  return NS_OK;
}

nsSize
nsHTMLImageElement::GetWidthHeight()
{
  nsSize size(0,0);

  nsIFrame* frame = GetPrimaryFrame(Flush_Layout);

  if (frame) {
    size = frame->GetContentRect().Size();

    size.width = nsPresContext::AppUnitsToIntCSSPixels(size.width);
    size.height = nsPresContext::AppUnitsToIntCSSPixels(size.height);
  } else {
    const nsAttrValue* value;
    nsCOMPtr<imgIContainer> image;
    if (mCurrentRequest) {
      mCurrentRequest->GetImage(getter_AddRefs(image));
    }

    if ((value = GetParsedAttr(nsGkAtoms::width)) &&
        value->Type() == nsAttrValue::eInteger) {
      size.width = value->GetIntegerValue();
    } else if (image) {
      image->GetWidth(&size.width);
    }

    if ((value = GetParsedAttr(nsGkAtoms::height)) &&
        value->Type() == nsAttrValue::eInteger) {
      size.height = value->GetIntegerValue();
    } else if (image) {
      image->GetHeight(&size.height);
    }
  }

  return size;
}

NS_IMETHODIMP
nsHTMLImageElement::GetHeight(PRInt32* aHeight)
{
  *aHeight = GetWidthHeight().height;

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLImageElement::SetHeight(PRInt32 aHeight)
{
  nsAutoString val;
  val.AppendInt(aHeight);

  return nsGenericHTMLElement::SetAttr(kNameSpaceID_None, nsGkAtoms::height,
                                       val, PR_TRUE);
}

NS_IMETHODIMP
nsHTMLImageElement::GetWidth(PRInt32* aWidth)
{
  *aWidth = GetWidthHeight().width;

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLImageElement::SetWidth(PRInt32 aWidth)
{
  nsAutoString val;
  val.AppendInt(aWidth);

  return nsGenericHTMLElement::SetAttr(kNameSpaceID_None, nsGkAtoms::width,
                                       val, PR_TRUE);
}

PRBool
nsHTMLImageElement::ParseAttribute(PRInt32 aNamespaceID,
                                   nsIAtom* aAttribute,
                                   const nsAString& aValue,
                                   nsAttrValue& aResult)
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
MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                      nsRuleData* aData)
{
  nsGenericHTMLElement::MapImageAlignAttributeInto(aAttributes, aData);
  nsGenericHTMLElement::MapImageBorderAttributeInto(aAttributes, aData);
  nsGenericHTMLElement::MapImageMarginAttributeInto(aAttributes, aData);
  nsGenericHTMLElement::MapImageSizeAttributesInto(aAttributes, aData);
  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
}

nsChangeHint
nsHTMLImageElement::GetAttributeChangeHint(const nsIAtom* aAttribute,
                                           PRInt32 aModType) const
{
  nsChangeHint retval =
    nsGenericHTMLElement::GetAttributeChangeHint(aAttribute, aModType);
  if (aAttribute == nsGkAtoms::usemap ||
      aAttribute == nsGkAtoms::ismap) {
    NS_UpdateHint(retval, NS_STYLE_HINT_FRAMECHANGE);
  }
  return retval;
}

NS_IMETHODIMP_(PRBool)
nsHTMLImageElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry* const map[] = {
    sCommonAttributeMap,
    sImageMarginSizeAttributeMap,
    sImageBorderAttributeMap,
    sImageAlignAttributeMap
  };

  return FindAttributeDependence(aAttribute, map, NS_ARRAY_LENGTH(map));
}


nsMapRuleToAttributesFunc
nsHTMLImageElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}


nsresult
nsHTMLImageElement::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  
  
  
  
  if (aVisitor.mEvent->eventStructType == NS_MOUSE_EVENT &&
      aVisitor.mEvent->message == NS_MOUSE_CLICK &&
      static_cast<nsMouseEvent*>(aVisitor.mEvent)->button ==
        nsMouseEvent::eLeftButton) {
    PRBool isMap = PR_FALSE;
    GetIsMap(&isMap);
    if (isMap) {
      aVisitor.mEventStatus = nsEventStatus_eConsumeNoDefault;
    }
  }
  return nsGenericHTMLElement::PreHandleEvent(aVisitor);
}

PRBool
nsHTMLImageElement::IsHTMLFocusable(PRBool aWithMouse,
                                    PRBool *aIsFocusable, PRInt32 *aTabIndex)
{
  PRInt32 tabIndex;
  GetTabIndex(&tabIndex);

  if (IsInDoc()) {
    nsAutoString usemap;
    GetUseMap(usemap);
    
    
    
    nsCOMPtr<nsIDOMHTMLMapElement> imageMap =
      nsImageMapUtils::FindImageMap(GetOwnerDoc(), usemap);
    if (imageMap) {
      if (aTabIndex) {
        
        *aTabIndex = (sTabFocusModel & eTabFocus_linksMask)? 0 : -1;
      }
      
      
      *aIsFocusable = PR_FALSE;

      return PR_FALSE;
    }
  }

  if (aTabIndex) {
    
    *aTabIndex = (sTabFocusModel & eTabFocus_formElementsMask)? tabIndex : -1;
  }

  *aIsFocusable = 
#ifdef XP_MACOSX
    (!aWithMouse || nsFocusManager::sMouseFocusesFormControl) &&
#endif
    (tabIndex >= 0 || HasAttr(kNameSpaceID_None, nsGkAtoms::tabindex));

  return PR_FALSE;
}

nsresult
nsHTMLImageElement::SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                            nsIAtom* aPrefix, const nsAString& aValue,
                            PRBool aNotify)
{
  
  
  
  
  
  if (aNotify &&
      aNameSpaceID == kNameSpaceID_None && aName == nsGkAtoms::src) {

    
    
    if (Preferences::GetBool("dom.disable_image_src_set") &&
        !nsContentUtils::IsCallerChrome()) {
      return NS_OK;
    }

    
    mNewRequestsWillNeedAnimationReset = PR_TRUE;

    
    
    
    
    
    LoadImage(aValue, PR_TRUE, aNotify);

    mNewRequestsWillNeedAnimationReset = PR_FALSE;
  }
    
  return nsGenericHTMLElement::SetAttr(aNameSpaceID, aName, aPrefix, aValue,
                                       aNotify);
}

nsresult
nsHTMLImageElement::UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                              PRBool aNotify)
{
  if (aNameSpaceID == kNameSpaceID_None && aAttribute == nsGkAtoms::src) {
    CancelImageRequests(aNotify);
  }

  return nsGenericHTMLElement::UnsetAttr(aNameSpaceID, aAttribute, aNotify);
}

nsresult
nsHTMLImageElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                               nsIContent* aBindingParent,
                               PRBool aCompileEventHandlers)
{
  nsresult rv = nsGenericHTMLElement::BindToTree(aDocument, aParent,
                                                 aBindingParent,
                                                 aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  if (HasAttr(kNameSpaceID_None, nsGkAtoms::src)) {
    ClearBrokenState();
    
    
    
    if (LoadingEnabled()) {
      nsContentUtils::AddScriptRunner(
        NS_NewRunnableMethod(this, &nsHTMLImageElement::MaybeLoadImage));
    }
  }

  return rv;
}

void
nsHTMLImageElement::MaybeLoadImage()
{
  
  
  
  nsAutoString uri;
  if (GetAttr(kNameSpaceID_None, nsGkAtoms::src, uri) &&
      (NS_FAILED(LoadImage(uri, PR_FALSE, PR_TRUE)) ||
       !LoadingEnabled())) {
    CancelImageRequests(PR_TRUE);
  }
}

nsEventStates
nsHTMLImageElement::IntrinsicState() const
{
  return nsGenericHTMLElement::IntrinsicState() |
    nsImageLoadingContent::ImageState();
}

NS_IMETHODIMP
nsHTMLImageElement::Initialize(nsISupports* aOwner, JSContext* aContext,
                               JSObject *aObj, PRUint32 argc, jsval *argv)
{
  if (argc <= 0) {
    

    return NS_OK;
  }

  
  uint32 width;
  JSBool ret = JS_ValueToECMAUint32(aContext, argv[0], &width);
  NS_ENSURE_TRUE(ret, NS_ERROR_INVALID_ARG);

  nsresult rv = SetIntAttr(nsGkAtoms::width, static_cast<PRInt32>(width));

  if (NS_SUCCEEDED(rv) && (argc > 1)) {
    
    uint32 height;
    ret = JS_ValueToECMAUint32(aContext, argv[1], &height);
    NS_ENSURE_TRUE(ret, NS_ERROR_INVALID_ARG);

    rv = SetIntAttr(nsGkAtoms::height, static_cast<PRInt32>(height));
  }

  return rv;
}

NS_IMETHODIMP
nsHTMLImageElement::GetNaturalHeight(PRInt32* aNaturalHeight)
{
  NS_ENSURE_ARG_POINTER(aNaturalHeight);

  *aNaturalHeight = 0;

  if (!mCurrentRequest) {
    return NS_OK;
  }
  
  nsCOMPtr<imgIContainer> image;
  mCurrentRequest->GetImage(getter_AddRefs(image));
  if (!image) {
    return NS_OK;
  }

  image->GetHeight(aNaturalHeight);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLImageElement::GetNaturalWidth(PRInt32* aNaturalWidth)
{
  NS_ENSURE_ARG_POINTER(aNaturalWidth);

  *aNaturalWidth = 0;

  if (!mCurrentRequest) {
    return NS_OK;
  }
  
  nsCOMPtr<imgIContainer> image;
  mCurrentRequest->GetImage(getter_AddRefs(image));
  if (!image) {
    return NS_OK;
  }

  image->GetWidth(aNaturalWidth);
  return NS_OK;
}

nsresult
nsHTMLImageElement::CopyInnerTo(nsGenericElement* aDest) const
{
  if (aDest->GetOwnerDoc()->IsStaticDocument()) {
    CreateStaticImageClone(static_cast<nsHTMLImageElement*>(aDest));
  }
  return nsGenericHTMLElement::CopyInnerTo(aDest);
}
