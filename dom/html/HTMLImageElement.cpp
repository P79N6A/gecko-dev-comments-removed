




#include "mozilla/dom/HTMLImageElement.h"
#include "mozilla/dom/HTMLImageElementBinding.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsMappedAttributes.h"
#include "nsSize.h"
#include "nsIDocument.h"
#include "nsIDOMMutationEvent.h"
#include "nsIScriptContext.h"
#include "nsIURL.h"
#include "nsIIOService.h"
#include "nsIServiceManager.h"
#include "nsIAppShell.h"
#include "nsWidgetsCID.h"
#include "nsNetUtil.h"
#include "nsContentUtils.h"
#include "nsContainerFrame.h"
#include "nsNodeInfoManager.h"
#include "mozilla/MouseEvents.h"
#include "nsContentPolicyUtils.h"
#include "nsIDOMWindow.h"
#include "nsFocusManager.h"
#include "mozilla/dom/HTMLFormElement.h"
#include "nsAttrValueOrString.h"
#include "imgLoader.h"


#include "mozilla/dom/HTMLSourceElement.h"
#include "mozilla/dom/ResponsiveImageSelector.h"

#include "imgIContainer.h"
#include "imgILoader.h"
#include "imgINotificationObserver.h"
#include "imgRequestProxy.h"

#include "nsILoadGroup.h"

#include "nsRuleData.h"

#include "nsIDOMHTMLMapElement.h"
#include "mozilla/EventDispatcher.h"
#include "mozilla/EventStates.h"

#include "nsLayoutUtils.h"

#include "mozilla/Preferences.h"
static const char *kPrefSrcsetEnabled = "dom.image.srcset.enabled";

static NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);

NS_IMPL_NS_NEW_HTML_ELEMENT(Image)

#ifdef DEBUG

static bool IsPreviousSibling(nsINode *aSubject, nsINode *aNode)
{
  if (aSubject == aNode) {
    return false;
  }

  nsINode *parent = aSubject->GetParentNode();
  if (parent && parent == aNode->GetParentNode()) {
    return parent->IndexOf(aSubject) < parent->IndexOf(aNode);
  }

  return false;
}
#endif

namespace mozilla {
namespace dom {




class ImageLoadTask : public nsRunnable
{
public:
  explicit ImageLoadTask(HTMLImageElement *aElement) :
    mElement(aElement)
  {}

  NS_IMETHOD Run()
  {
    if (mElement->mPendingImageLoadTask == this) {
      mElement->mPendingImageLoadTask = nullptr;
      mElement->LoadSelectedImage(true, true);
    }
    return NS_OK;
  }

private:
  ~ImageLoadTask() {}
  nsRefPtr<HTMLImageElement> mElement;
};

HTMLImageElement::HTMLImageElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
  , mForm(nullptr)
{
  
  AddStatesSilently(NS_EVENT_STATE_BROKEN);
}

HTMLImageElement::~HTMLImageElement()
{
  DestroyImageLoadingContent();
}


NS_IMPL_ADDREF_INHERITED(HTMLImageElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLImageElement, Element)

NS_IMPL_CYCLE_COLLECTION_INHERITED(HTMLImageElement,
                                   nsGenericHTMLElement,
                                   mResponsiveSelector)


NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(HTMLImageElement)
  NS_INTERFACE_TABLE_INHERITED(HTMLImageElement,
                               nsIDOMHTMLImageElement,
                               nsIImageLoadingContent,
                               imgIOnloadBlocker,
                               imgINotificationObserver)
NS_INTERFACE_TABLE_TAIL_INHERITING(nsGenericHTMLElement)


NS_IMPL_ELEMENT_CLONE(HTMLImageElement)


NS_IMPL_STRING_ATTR(HTMLImageElement, Name, name)
NS_IMPL_STRING_ATTR(HTMLImageElement, Align, align)
NS_IMPL_STRING_ATTR(HTMLImageElement, Alt, alt)
NS_IMPL_STRING_ATTR(HTMLImageElement, Border, border)
NS_IMPL_INT_ATTR(HTMLImageElement, Hspace, hspace)
NS_IMPL_BOOL_ATTR(HTMLImageElement, IsMap, ismap)
NS_IMPL_URI_ATTR(HTMLImageElement, LongDesc, longdesc)
NS_IMPL_STRING_ATTR(HTMLImageElement, Sizes, sizes)
NS_IMPL_STRING_ATTR(HTMLImageElement, Lowsrc, lowsrc)
NS_IMPL_URI_ATTR(HTMLImageElement, Src, src)
NS_IMPL_STRING_ATTR(HTMLImageElement, Srcset, srcset)
NS_IMPL_STRING_ATTR(HTMLImageElement, UseMap, usemap)
NS_IMPL_INT_ATTR(HTMLImageElement, Vspace, vspace)

bool
HTMLImageElement::IsInteractiveHTMLContent(bool aIgnoreTabindex) const
{
  return HasAttr(kNameSpaceID_None, nsGkAtoms::usemap) ||
          nsGenericHTMLElement::IsInteractiveHTMLContent(aIgnoreTabindex);
}

bool
HTMLImageElement::IsSrcsetEnabled()
{
  return Preferences::GetBool(kPrefSrcsetEnabled, false);
}

nsresult
HTMLImageElement::GetCurrentSrc(nsAString& aValue)
{
  if (!IsSrcsetEnabled()) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIURI> currentURI;
  GetCurrentURI(getter_AddRefs(currentURI));
  if (currentURI) {
    nsAutoCString spec;
    currentURI->GetSpec(spec);
    CopyUTF8toUTF16(spec, aValue);
  } else {
    SetDOMStringToNull(aValue);
  }

  return NS_OK;
}

void
HTMLImageElement::GetItemValueText(DOMString& aValue)
{
  GetSrc(aValue);
}

void
HTMLImageElement::SetItemValueText(const nsAString& aValue)
{
  SetSrc(aValue);
}

bool
HTMLImageElement::Draggable() const
{
  
  return !AttrValueIs(kNameSpaceID_None, nsGkAtoms::draggable,
                      nsGkAtoms::_false, eIgnoreCase);
}

bool
HTMLImageElement::Complete()
{
  if (!mCurrentRequest) {
    return true;
  }

  if (mPendingRequest) {
    return false;
  }

  uint32_t status;
  mCurrentRequest->GetImageStatus(&status);
  return
    (status &
     (imgIRequest::STATUS_LOAD_COMPLETE | imgIRequest::STATUS_ERROR)) != 0;
}

NS_IMETHODIMP
HTMLImageElement::GetComplete(bool* aComplete)
{
  NS_PRECONDITION(aComplete, "Null out param!");

  *aComplete = Complete();

  return NS_OK;
}

CSSIntPoint
HTMLImageElement::GetXY()
{
  nsIFrame* frame = GetPrimaryFrame(Flush_Layout);
  if (!frame) {
    return CSSIntPoint(0, 0);
  }

  nsIFrame* layer = nsLayoutUtils::GetClosestLayer(frame->GetParent());
  return CSSIntPoint::FromAppUnitsRounded(frame->GetOffsetTo(layer));
}

int32_t
HTMLImageElement::X()
{
  return GetXY().x;
}

int32_t
HTMLImageElement::Y()
{
  return GetXY().y;
}

NS_IMETHODIMP
HTMLImageElement::GetX(int32_t* aX)
{
  *aX = X();
  return NS_OK;
}

NS_IMETHODIMP
HTMLImageElement::GetY(int32_t* aY)
{
  *aY = Y();
  return NS_OK;
}

NS_IMETHODIMP
HTMLImageElement::GetHeight(uint32_t* aHeight)
{
  *aHeight = Height();

  return NS_OK;
}

NS_IMETHODIMP
HTMLImageElement::SetHeight(uint32_t aHeight)
{
  ErrorResult rv;
  SetHeight(aHeight, rv);
  return rv.StealNSResult();
}

NS_IMETHODIMP
HTMLImageElement::GetWidth(uint32_t* aWidth)
{
  *aWidth = Width();

  return NS_OK;
}

NS_IMETHODIMP
HTMLImageElement::SetWidth(uint32_t aWidth)
{
  ErrorResult rv;
  SetWidth(aWidth, rv);
  return rv.StealNSResult();
}

bool
HTMLImageElement::ParseAttribute(int32_t aNamespaceID,
                                 nsIAtom* aAttribute,
                                 const nsAString& aValue,
                                 nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::align) {
      return ParseAlignValue(aValue, aResult);
    }
    if (aAttribute == nsGkAtoms::crossorigin) {
      ParseCORSValue(aValue, aResult);
      return true;
    }
    if (ParseImageAttribute(aAttribute, aValue, aResult)) {
      return true;
    }
  }

  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}

void
HTMLImageElement::MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                                        nsRuleData* aData)
{
  nsGenericHTMLElement::MapImageAlignAttributeInto(aAttributes, aData);
  nsGenericHTMLElement::MapImageBorderAttributeInto(aAttributes, aData);
  nsGenericHTMLElement::MapImageMarginAttributeInto(aAttributes, aData);
  nsGenericHTMLElement::MapImageSizeAttributesInto(aAttributes, aData);
  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
}

nsChangeHint
HTMLImageElement::GetAttributeChangeHint(const nsIAtom* aAttribute,
                                         int32_t aModType) const
{
  nsChangeHint retval =
    nsGenericHTMLElement::GetAttributeChangeHint(aAttribute, aModType);
  if (aAttribute == nsGkAtoms::usemap ||
      aAttribute == nsGkAtoms::ismap) {
    NS_UpdateHint(retval, NS_STYLE_HINT_FRAMECHANGE);
  } else if (aAttribute == nsGkAtoms::alt) {
    if (aModType == nsIDOMMutationEvent::ADDITION ||
        aModType == nsIDOMMutationEvent::REMOVAL) {
      NS_UpdateHint(retval, NS_STYLE_HINT_FRAMECHANGE);
    }
  }
  return retval;
}

NS_IMETHODIMP_(bool)
HTMLImageElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry* const map[] = {
    sCommonAttributeMap,
    sImageMarginSizeAttributeMap,
    sImageBorderAttributeMap,
    sImageAlignAttributeMap
  };

  return FindAttributeDependence(aAttribute, map);
}


nsMapRuleToAttributesFunc
HTMLImageElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}


nsresult
HTMLImageElement::BeforeSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                const nsAttrValueOrString* aValue,
                                bool aNotify)
{

  if (aNameSpaceID == kNameSpaceID_None && mForm &&
      (aName == nsGkAtoms::name || aName == nsGkAtoms::id)) {
    
    nsAutoString tmp;
    GetAttr(kNameSpaceID_None, aName, tmp);

    if (!tmp.IsEmpty()) {
      mForm->RemoveImageElementFromTable(this, tmp,
                                         HTMLFormElement::AttributeUpdated);
    }
  }

  return nsGenericHTMLElement::BeforeSetAttr(aNameSpaceID, aName,
                                             aValue, aNotify);
}

nsresult
HTMLImageElement::AfterSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                               const nsAttrValue* aValue, bool aNotify)
{
  if (aNameSpaceID == kNameSpaceID_None && mForm &&
      (aName == nsGkAtoms::name || aName == nsGkAtoms::id) &&
      aValue && !aValue->IsEmptyString()) {
    
    MOZ_ASSERT(aValue->Type() == nsAttrValue::eAtom,
               "Expected atom value for name/id");
    mForm->AddImageElementToTable(this,
      nsDependentAtomString(aValue->GetAtomValue()));
  }

  
  
  

  nsAttrValueOrString attrVal(aValue);

  if (aName == nsGkAtoms::src &&
      aNameSpaceID == kNameSpaceID_None &&
      !aValue) {
    
    
    if (InResponsiveMode()) {
      if (mResponsiveSelector &&
          mResponsiveSelector->Content() == this) {
        mResponsiveSelector->SetDefaultSource(NullString());
      }
      QueueImageLoadTask();
    } else {
      
      CancelImageRequests(aNotify);
    }
  } else if (aName == nsGkAtoms::srcset &&
             aNameSpaceID == kNameSpaceID_None &&
             IsSrcsetEnabled()) {
    PictureSourceSrcsetChanged(this, attrVal.String(), aNotify);
  } else if (aName == nsGkAtoms::sizes &&
             aNameSpaceID == kNameSpaceID_None &&
             HTMLPictureElement::IsPictureEnabled()) {
    PictureSourceSizesChanged(this, attrVal.String(), aNotify);
  } else if (aName == nsGkAtoms::crossorigin &&
             aNameSpaceID == kNameSpaceID_None &&
             aNotify) {
    
    if (InResponsiveMode()) {
      
      
      QueueImageLoadTask();
    } else {
      
      
      
      ForceReload(aNotify);
    }
  }

  return nsGenericHTMLElement::AfterSetAttr(aNameSpaceID, aName,
                                            aValue, aNotify);
}


nsresult
HTMLImageElement::PreHandleEvent(EventChainPreVisitor& aVisitor)
{
  
  
  
  
  WidgetMouseEvent* mouseEvent = aVisitor.mEvent->AsMouseEvent();
  if (mouseEvent && mouseEvent->IsLeftClickEvent()) {
    bool isMap = false;
    GetIsMap(&isMap);
    if (isMap) {
      aVisitor.mEventStatus = nsEventStatus_eConsumeNoDefault;
    }
  }
  return nsGenericHTMLElement::PreHandleEvent(aVisitor);
}

bool
HTMLImageElement::IsHTMLFocusable(bool aWithMouse,
                                  bool *aIsFocusable, int32_t *aTabIndex)
{
  int32_t tabIndex = TabIndex();

  if (IsInDoc()) {
    nsAutoString usemap;
    GetUseMap(usemap);
    
    
    
    if (OwnerDoc()->FindImageMap(usemap)) {
      if (aTabIndex) {
        
        *aTabIndex = (sTabFocusModel & eTabFocus_linksMask)? 0 : -1;
      }
      
      
      *aIsFocusable = false;

      return false;
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

  return false;
}

nsresult
HTMLImageElement::SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                          nsIAtom* aPrefix, const nsAString& aValue,
                          bool aNotify)
{
  
  
  
  
  
  
  
  
  
  
  if (aNameSpaceID == kNameSpaceID_None &&
      aName == nsGkAtoms::src) {

    
    
    if (nsContentUtils::IsImageSrcSetDisabled()) {
      return NS_OK;
    }

    if (InResponsiveMode()) {
      if (mResponsiveSelector &&
          mResponsiveSelector->Content() == this) {
        mResponsiveSelector->SetDefaultSource(aValue);
      }
      QueueImageLoadTask();
    } else if (aNotify) {
      
      
      

      

      
      mNewRequestsWillNeedAnimationReset = true;

      
      
      
      
      
      LoadImage(aValue, true, aNotify, eImageLoadType_Normal);

      mNewRequestsWillNeedAnimationReset = false;
    }
  }

  return nsGenericHTMLElement::SetAttr(aNameSpaceID, aName, aPrefix, aValue,
                                       aNotify);
}

nsresult
HTMLImageElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                             nsIContent* aBindingParent,
                             bool aCompileEventHandlers)
{
  nsresult rv = nsGenericHTMLElement::BindToTree(aDocument, aParent,
                                                 aBindingParent,
                                                 aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  nsImageLoadingContent::BindToTree(aDocument, aParent, aBindingParent,
                                    aCompileEventHandlers);

  if (aParent) {
    UpdateFormOwner();
  }

  bool addedToPicture = aParent && aParent->IsHTMLElement(nsGkAtoms::picture) &&
                        HTMLPictureElement::IsPictureEnabled();
  if (addedToPicture) {
    QueueImageLoadTask();
  } else if (!InResponsiveMode() &&
             HasAttr(kNameSpaceID_None, nsGkAtoms::src)) {
    
    
    
    
    

    
    
    ClearBrokenState();
    RemoveStatesSilently(NS_EVENT_STATE_BROKEN);

    
    
    

    
    
    
    if (LoadingEnabled()) {
      nsContentUtils::AddScriptRunner(
          NS_NewRunnableMethod(this, &HTMLImageElement::MaybeLoadImage));
    }
  }

  return rv;
}

void
HTMLImageElement::UnbindFromTree(bool aDeep, bool aNullParent)
{
  if (mForm) {
    if (aNullParent || !FindAncestorForm(mForm)) {
      ClearForm(true);
    } else {
      UnsetFlags(MAYBE_ORPHAN_FORM_ELEMENT);
    }
  }

  if (aNullParent && GetParent() &&
      GetParent()->IsHTMLElement(nsGkAtoms::picture) &&
      HTMLPictureElement::IsPictureEnabled()) {
    
    
    QueueImageLoadTask();
  }

  nsImageLoadingContent::UnbindFromTree(aDeep, aNullParent);
  nsGenericHTMLElement::UnbindFromTree(aDeep, aNullParent);
}

void
HTMLImageElement::UpdateFormOwner()
{
  if (!mForm) {
    mForm = FindAncestorForm();
  }

  if (mForm && !HasFlag(ADDED_TO_FORM)) {
    
    nsAutoString nameVal, idVal;
    GetAttr(kNameSpaceID_None, nsGkAtoms::name, nameVal);
    GetAttr(kNameSpaceID_None, nsGkAtoms::id, idVal);

    SetFlags(ADDED_TO_FORM);

    mForm->AddImageElement(this);

    if (!nameVal.IsEmpty()) {
      mForm->AddImageElementToTable(this, nameVal);
    }

    if (!idVal.IsEmpty()) {
      mForm->AddImageElementToTable(this, idVal);
    }
  }
}

void
HTMLImageElement::MaybeLoadImage()
{
  
  
  

  

  LoadSelectedImage(false, true);

  if (!LoadingEnabled()) {
    CancelImageRequests(true);
  }
}

EventStates
HTMLImageElement::IntrinsicState() const
{
  return nsGenericHTMLElement::IntrinsicState() |
    nsImageLoadingContent::ImageState();
}


already_AddRefed<HTMLImageElement>
HTMLImageElement::Image(const GlobalObject& aGlobal,
                        const Optional<uint32_t>& aWidth,
                        const Optional<uint32_t>& aHeight,
                        ErrorResult& aError)
{
  nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(aGlobal.GetAsSupports());
  nsIDocument* doc;
  if (!win || !(doc = win->GetExtantDoc())) {
    aError.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  already_AddRefed<mozilla::dom::NodeInfo> nodeInfo =
    doc->NodeInfoManager()->GetNodeInfo(nsGkAtoms::img, nullptr,
                                        kNameSpaceID_XHTML,
                                        nsIDOMNode::ELEMENT_NODE);

  nsRefPtr<HTMLImageElement> img = new HTMLImageElement(nodeInfo);

  if (aWidth.WasPassed()) {
    img->SetWidth(aWidth.Value(), aError);
    if (aError.Failed()) {
      return nullptr;
    }

    if (aHeight.WasPassed()) {
      img->SetHeight(aHeight.Value(), aError);
      if (aError.Failed()) {
        return nullptr;
      }
    }
  }

  return img.forget();
}

uint32_t
HTMLImageElement::NaturalHeight()
{
  uint32_t height;
  nsresult rv = nsImageLoadingContent::GetNaturalHeight(&height);

  if (NS_FAILED(rv)) {
    MOZ_ASSERT(false, "GetNaturalHeight should not fail");
    return 0;
  }

  if (mResponsiveSelector) {
    double density = mResponsiveSelector->GetSelectedImageDensity();
    MOZ_ASSERT(IsFinite(density) && density > 0.0);
    height = NSToIntRound(double(height) / density);
    height = std::max(height, 0u);
  }

  return height;
}

NS_IMETHODIMP
HTMLImageElement::GetNaturalHeight(uint32_t* aNaturalHeight)
{
  *aNaturalHeight = NaturalHeight();
  return NS_OK;
}

uint32_t
HTMLImageElement::NaturalWidth()
{
  uint32_t width;
  nsresult rv = nsImageLoadingContent::GetNaturalWidth(&width);

  if (NS_FAILED(rv)) {
    MOZ_ASSERT(false, "GetNaturalWidth should not fail");
    return 0;
  }

  if (mResponsiveSelector) {
    double density = mResponsiveSelector->GetSelectedImageDensity();
    MOZ_ASSERT(IsFinite(density) && density > 0.0);
    width = NSToIntRound(double(width) / density);
    width = std::max(width, 0u);
  }

  return width;
}

NS_IMETHODIMP
HTMLImageElement::GetNaturalWidth(uint32_t* aNaturalWidth)
{
  *aNaturalWidth = NaturalWidth();
  return NS_OK;
}

nsresult
HTMLImageElement::CopyInnerTo(Element* aDest)
{
  if (aDest->OwnerDoc()->IsStaticDocument()) {
    CreateStaticImageClone(static_cast<HTMLImageElement*>(aDest));
  }
  return nsGenericHTMLElement::CopyInnerTo(aDest);
}

CORSMode
HTMLImageElement::GetCORSMode()
{
  return AttrValueToCORSMode(GetParsedAttr(nsGkAtoms::crossorigin));
}

JSObject*
HTMLImageElement::WrapNode(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return HTMLImageElementBinding::Wrap(aCx, this, aGivenProto);
}

#ifdef DEBUG
nsIDOMHTMLFormElement*
HTMLImageElement::GetForm() const
{
  return mForm;
}
#endif

void
HTMLImageElement::SetForm(nsIDOMHTMLFormElement* aForm)
{
  NS_PRECONDITION(aForm, "Don't pass null here");
  NS_ASSERTION(!mForm,
               "We don't support switching from one non-null form to another.");

  mForm = static_cast<HTMLFormElement*>(aForm);
}

void
HTMLImageElement::ClearForm(bool aRemoveFromForm)
{
  NS_ASSERTION((mForm != nullptr) == HasFlag(ADDED_TO_FORM),
               "Form control should have had flag set correctly");

  if (!mForm) {
    return;
  }

  if (aRemoveFromForm) {
    nsAutoString nameVal, idVal;
    GetAttr(kNameSpaceID_None, nsGkAtoms::name, nameVal);
    GetAttr(kNameSpaceID_None, nsGkAtoms::id, idVal);

    mForm->RemoveImageElement(this);

    if (!nameVal.IsEmpty()) {
      mForm->RemoveImageElementFromTable(this, nameVal,
                                         HTMLFormElement::ElementRemoved);
    }

    if (!idVal.IsEmpty()) {
      mForm->RemoveImageElementFromTable(this, idVal,
                                         HTMLFormElement::ElementRemoved);
    }
  }

  UnsetFlags(ADDED_TO_FORM);
  mForm = nullptr;
}

void
HTMLImageElement::QueueImageLoadTask()
{
  
  
  if (!LoadingEnabled() || !this->OwnerDoc()->IsCurrentActiveDocument()) {
    return;
  }

  
  
  mPendingImageLoadTask = new ImageLoadTask(this);
  nsCOMPtr<nsIAppShell> appShell = do_GetService(kAppShellCID);
  if (appShell) {
    appShell->RunInStableState(mPendingImageLoadTask);
  } else {
    MOZ_ASSERT(false, "expect appshell for HTMLImageElement");
  }
}

bool
HTMLImageElement::HaveSrcsetOrInPicture()
{
  if (IsSrcsetEnabled() && HasAttr(kNameSpaceID_None, nsGkAtoms::srcset)) {
    return true;
  }

  if (!HTMLPictureElement::IsPictureEnabled()) {
    return false;
  }

  Element *parent = nsINode::GetParentElement();
  return (parent && parent->IsHTMLElement(nsGkAtoms::picture));
}

bool
HTMLImageElement::InResponsiveMode()
{
  
  
  
  return mResponsiveSelector ||
         mPendingImageLoadTask ||
         HaveSrcsetOrInPicture();
}

nsresult
HTMLImageElement::LoadSelectedImage(bool aForce, bool aNotify)
{
  nsresult rv = NS_ERROR_FAILURE;

  if (aForce) {
    
    
    
    UpdateResponsiveSource();
  }

  if (mResponsiveSelector) {
    nsCOMPtr<nsIURI> url = mResponsiveSelector->GetSelectedImageURL();
    if (url) {
      rv = LoadImage(url, aForce, aNotify, eImageLoadType_Imageset);
    }
  } else {
    nsAutoString src;
    if (!GetAttr(kNameSpaceID_None, nsGkAtoms::src, src)) {
      CancelImageRequests(aNotify);
      rv = NS_OK;
    } else {
      
      
      
      rv = LoadImage(src, aForce, aNotify,
                     HaveSrcsetOrInPicture() ? eImageLoadType_Imageset
                                             : eImageLoadType_Normal);
    }
  }

  if (NS_FAILED(rv)) {
    CancelImageRequests(aNotify);
  }
  return rv;
}

void
HTMLImageElement::PictureSourceSrcsetChanged(nsIContent *aSourceNode,
                                             const nsAString& aNewValue,
                                             bool aNotify)
{
  bool isSelf = aSourceNode == this;

  if (!IsSrcsetEnabled() ||
      (!isSelf && !HTMLPictureElement::IsPictureEnabled())) {
    return;
  }

  MOZ_ASSERT(isSelf || IsPreviousSibling(aSourceNode, this),
             "Should not be getting notifications for non-previous-siblings");

  nsIContent *currentSrc =
    mResponsiveSelector ? mResponsiveSelector->Content() : nullptr;

  if (aSourceNode == currentSrc) {
    
    
    mResponsiveSelector->SetCandidatesFromSourceSet(aNewValue);
  }

  
  
  QueueImageLoadTask();
}

void
HTMLImageElement::PictureSourceSizesChanged(nsIContent *aSourceNode,
                                            const nsAString& aNewValue,
                                            bool aNotify)
{
  if (!HTMLPictureElement::IsPictureEnabled()) {
    return;
  }

  MOZ_ASSERT(aSourceNode == this ||
             IsPreviousSibling(aSourceNode, this),
             "Should not be getting notifications for non-previous-siblings");

  nsIContent *currentSrc =
    mResponsiveSelector ? mResponsiveSelector->Content() : nullptr;

  if (aSourceNode == currentSrc) {
    
    
    mResponsiveSelector->SetSizesFromDescriptor(aNewValue);
  }

  
  
  QueueImageLoadTask();
}

void
HTMLImageElement::PictureSourceMediaOrTypeChanged(nsIContent *aSourceNode,
                                                  bool aNotify)
{
  if (!HTMLPictureElement::IsPictureEnabled()) {
    return;
  }

  MOZ_ASSERT(IsPreviousSibling(aSourceNode, this),
             "Should not be getting notifications for non-previous-siblings");

  
  
  QueueImageLoadTask();
}

void
HTMLImageElement::PictureSourceAdded(nsIContent *aSourceNode)
{
  if (!HTMLPictureElement::IsPictureEnabled()) {
    return;
  }

  QueueImageLoadTask();
}

void
HTMLImageElement::PictureSourceRemoved(nsIContent *aSourceNode)
{
  if (!HTMLPictureElement::IsPictureEnabled()) {
    return;
  }

  QueueImageLoadTask();
}

void
HTMLImageElement::UpdateResponsiveSource()
{
  if (!IsSrcsetEnabled()) {
    mResponsiveSelector = nullptr;
    return;
  }

  nsIContent *currentSource =
    mResponsiveSelector ? mResponsiveSelector->Content() : nullptr;
  bool pictureEnabled = HTMLPictureElement::IsPictureEnabled();
  Element *parent = pictureEnabled ? nsINode::GetParentElement() : nullptr;

  nsINode *candidateSource = nullptr;
  if (parent && parent->IsHTMLElement(nsGkAtoms::picture)) {
    
    candidateSource = parent->GetFirstChild();
  } else {
    candidateSource = this;
  }

  while (candidateSource) {
    if (candidateSource == currentSource) {
      
      
      mResponsiveSelector->SelectImage(true);
      if (mResponsiveSelector->NumCandidates()) {
        break;
      }

      
      mResponsiveSelector = nullptr;
      if (candidateSource == this) {
        
        break;
      }
    } else if (candidateSource == this) {
      
      if (!TryCreateResponsiveSelector(candidateSource->AsContent())) {
        
        mResponsiveSelector = nullptr;
      }
      break;
    } else if (candidateSource->IsHTMLElement(nsGkAtoms::source) &&
               TryCreateResponsiveSelector(candidateSource->AsContent())) {
      
      break;
    }
    candidateSource = candidateSource->GetNextSibling();
  }

  if (!candidateSource) {
    
    mResponsiveSelector = nullptr;
  }
}

 bool
HTMLImageElement::SupportedPictureSourceType(const nsAString& aType)
{
  return
    imgLoader::SupportImageWithMimeType(NS_ConvertUTF16toUTF8(aType).get(),
                                        AcceptedMimeTypes::IMAGES_AND_DOCUMENTS);
}

bool
HTMLImageElement::TryCreateResponsiveSelector(nsIContent *aSourceNode,
                                              const nsAString *aSrcset,
                                              const nsAString *aSizes)
{
  if (!IsSrcsetEnabled()) {
    return false;
  }

  bool pictureEnabled = HTMLPictureElement::IsPictureEnabled();
  
  bool isSourceTag = aSourceNode->IsHTMLElement(nsGkAtoms::source);
  if (isSourceTag) {
    DebugOnly<Element *> parent(nsINode::GetParentElement());
    MOZ_ASSERT(parent && parent->IsHTMLElement(nsGkAtoms::picture));
    MOZ_ASSERT(IsPreviousSibling(aSourceNode, this));
    MOZ_ASSERT(pictureEnabled);

    
    HTMLSourceElement *src = static_cast<HTMLSourceElement*>(aSourceNode);
    if (!src->MatchesCurrentMedia()) {
      return false;
    }

    nsAutoString type;
    if (aSourceNode->GetAttr(kNameSpaceID_None, nsGkAtoms::type, type) &&
        !SupportedPictureSourceType(type)) {
      return false;
    }
  } else if (aSourceNode->IsHTMLElement(nsGkAtoms::img)) {
    
    MOZ_ASSERT(aSourceNode == this);
  }

  
  nsString srcset;
  if (aSrcset) {
    srcset = *aSrcset;
  } else if (!aSourceNode->GetAttr(kNameSpaceID_None, nsGkAtoms::srcset,
                                   srcset)) {
    return false;
  }

  if (srcset.IsEmpty()) {
    return false;
  }


  
  nsRefPtr<ResponsiveImageSelector> sel = new ResponsiveImageSelector(aSourceNode);
  if (!sel->SetCandidatesFromSourceSet(srcset)) {
    
    return false;
  }

  if (pictureEnabled && aSizes) {
    sel->SetSizesFromDescriptor(*aSizes);
  } else if (pictureEnabled) {
    nsAutoString sizes;
    aSourceNode->GetAttr(kNameSpaceID_None, nsGkAtoms::sizes, sizes);
    sel->SetSizesFromDescriptor(sizes);
  }

  
  if (!isSourceTag) {
    MOZ_ASSERT(aSourceNode == this);
    nsAutoString src;
    if (GetAttr(kNameSpaceID_None, nsGkAtoms::src, src) && !src.IsEmpty()) {
      sel->SetDefaultSource(src);
    }
  }

  mResponsiveSelector = sel;
  return true;
}

 bool
HTMLImageElement::SelectSourceForTagWithAttrs(nsIDocument *aDocument,
                                              bool aIsSourceTag,
                                              const nsAString& aSrcAttr,
                                              const nsAString& aSrcsetAttr,
                                              const nsAString& aSizesAttr,
                                              const nsAString& aTypeAttr,
                                              const nsAString& aMediaAttr,
                                              nsAString& aResult)
{
  MOZ_ASSERT(aIsSourceTag || (aTypeAttr.IsEmpty() && aMediaAttr.IsEmpty()),
             "Passing type or media attrs makes no sense without aIsSourceTag");
  MOZ_ASSERT(!aIsSourceTag || aSrcAttr.IsEmpty(),
             "Passing aSrcAttr makes no sense with aIsSourceTag set");

  bool pictureEnabled = HTMLPictureElement::IsPictureEnabled();
  if (aIsSourceTag && !pictureEnabled) {
    return false;
  }

  if (!IsSrcsetEnabled() || aSrcsetAttr.IsEmpty()) {
    if (!aIsSourceTag) {
      
      aResult.Assign(aSrcAttr);
      return true;
    }
    
    return false;
  }

  
  if (aIsSourceTag &&
      ((!aMediaAttr.IsVoid() &&
       !HTMLSourceElement::WouldMatchMediaForDocument(aMediaAttr, aDocument)) ||
      (!aTypeAttr.IsVoid() &&
       !SupportedPictureSourceType(aTypeAttr)))) {
    return false;
  }

  
  nsRefPtr<ResponsiveImageSelector> sel =
    new ResponsiveImageSelector(aDocument);

  sel->SetCandidatesFromSourceSet(aSrcsetAttr);
  if (pictureEnabled && !aSizesAttr.IsEmpty()) {
    sel->SetSizesFromDescriptor(aSizesAttr);
  }
  if (!aIsSourceTag) {
    sel->SetDefaultSource(aSrcAttr);
  }

  if (sel->GetSelectedImageURLSpec(aResult)) {
    return true;
  }

  if (!aIsSourceTag) {
    
    aResult.Truncate();
    return true;
  }

  
  return false;
}

void
HTMLImageElement::DestroyContent()
{
  mResponsiveSelector = nullptr;
}

} 
} 

