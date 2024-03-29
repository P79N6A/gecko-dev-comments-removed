











#include "mozilla/dom/ElementInlines.h"

#include "AnimationCommon.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/dom/Animation.h"
#include "mozilla/dom/Attr.h"
#include "nsDOMAttributeMap.h"
#include "nsIAtom.h"
#include "nsIContentInlines.h"
#include "mozilla/dom/NodeInfo.h"
#include "nsIDocumentInlines.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMDocument.h"
#include "nsIContentIterator.h"
#include "nsFocusManager.h"
#include "nsFrameManager.h"
#include "nsILinkHandler.h"
#include "nsIScriptGlobalObject.h"
#include "nsIURL.h"
#include "nsContainerFrame.h"
#include "nsIAnonymousContentCreator.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsStyleConsts.h"
#include "nsString.h"
#include "nsUnicharUtils.h"
#include "nsIDOMEvent.h"
#include "nsDOMCID.h"
#include "nsIServiceManager.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsDOMCSSAttrDeclaration.h"
#include "nsNameSpaceManager.h"
#include "nsContentList.h"
#include "nsVariant.h"
#include "nsDOMSettableTokenList.h"
#include "nsDOMTokenList.h"
#include "nsXBLPrototypeBinding.h"
#include "nsError.h"
#include "nsDOMString.h"
#include "nsIScriptSecurityManager.h"
#include "nsIDOMMutationEvent.h"
#include "mozilla/AnimationComparator.h"
#include "mozilla/AsyncEventDispatcher.h"
#include "mozilla/ContentEvents.h"
#include "mozilla/EventDispatcher.h"
#include "mozilla/EventListenerManager.h"
#include "mozilla/EventStateManager.h"
#include "mozilla/EventStates.h"
#include "mozilla/InternalMutationEvent.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/TextEvents.h"
#include "nsNodeUtils.h"
#include "mozilla/dom/DirectionalityUtils.h"
#include "nsDocument.h"
#include "nsAttrValueOrString.h"
#include "nsAttrValueInlines.h"
#ifdef MOZ_XUL
#include "nsXULElement.h"
#endif 
#include "nsSVGElement.h"
#include "nsFrameSelection.h"
#ifdef DEBUG
#include "nsRange.h"
#endif

#include "nsBindingManager.h"
#include "nsXBLBinding.h"
#include "nsPIDOMWindow.h"
#include "nsPIBoxObject.h"
#include "mozilla/dom/DOMRect.h"
#include "nsSVGUtils.h"
#include "nsLayoutUtils.h"
#include "nsGkAtoms.h"
#include "nsContentUtils.h"
#include "ChildIterator.h"

#include "nsIDOMEventListener.h"
#include "nsIWebNavigation.h"
#include "nsIBaseWindow.h"
#include "nsIWidget.h"

#include "nsNodeInfoManager.h"
#include "nsICategoryManager.h"
#include "nsIDOMDocumentType.h"
#include "nsGenericHTMLElement.h"
#include "nsIEditor.h"
#include "nsIEditorIMESupport.h"
#include "nsContentCreatorFunctions.h"
#include "nsIControllers.h"
#include "nsView.h"
#include "nsViewManager.h"
#include "nsIScrollableFrame.h"
#include "mozilla/css/StyleRule.h" 
#include "nsCSSRuleProcessor.h"
#include "nsRuleProcessorData.h"
#include "nsTextNode.h"

#ifdef MOZ_XUL
#include "nsIXULDocument.h"
#endif 

#include "nsCycleCollectionParticipant.h"
#include "nsCCUncollectableMarker.h"

#include "mozAutoDocUpdate.h"

#include "nsCSSParser.h"
#include "prprf.h"
#include "nsDOMMutationObserver.h"
#include "nsSVGFeatures.h"
#include "nsWrapperCacheInlines.h"
#include "xpcpublic.h"
#include "nsIScriptError.h"
#include "mozilla/Telemetry.h"

#include "mozilla/CORSMode.h"
#include "mozilla/dom/ShadowRoot.h"
#include "mozilla/dom/NodeListBinding.h"

#include "nsStyledElement.h"
#include "nsXBLService.h"
#include "nsITextControlElement.h"
#include "nsITextControlFrame.h"
#include "nsISupportsImpl.h"
#include "mozilla/dom/DocumentFragment.h"
#include "mozilla/IntegerPrintfMacros.h"
#include "mozilla/dom/WindowBinding.h"
#include "mozilla/dom/ElementBinding.h"
#include "mozilla/dom/VRDevice.h"
#include "nsComputedDOMStyle.h"
#include "mozilla/Preferences.h"

using namespace mozilla;
using namespace mozilla::dom;

nsIAtom*
nsIContent::DoGetID() const
{
  MOZ_ASSERT(HasID(), "Unexpected call");
  MOZ_ASSERT(IsElement(), "Only elements can have IDs");

  return AsElement()->GetParsedAttr(nsGkAtoms::id)->GetAtomValue();
}

const nsAttrValue*
nsIContent::DoGetClasses() const
{
  MOZ_ASSERT(HasFlag(NODE_MAY_HAVE_CLASS), "Unexpected call");
  MOZ_ASSERT(IsElement(), "Only elements can have classes");

  if (IsSVGElement()) {
    const nsAttrValue* animClass =
      static_cast<const nsSVGElement*>(this)->GetAnimatedClassName();
    if (animClass) {
      return animClass;
    }
  }

  return AsElement()->GetParsedAttr(nsGkAtoms::_class);
}

NS_IMETHODIMP
Element::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  NS_ASSERTION(aInstancePtr,
               "QueryInterface requires a non-NULL destination!");
  nsresult rv = FragmentOrElement::QueryInterface(aIID, aInstancePtr);
  if (NS_SUCCEEDED(rv)) {
    return NS_OK;
  }

  
  return OwnerDoc()->BindingManager()->GetBindingImplementation(this, aIID,
                                                                aInstancePtr);
}

EventStates
Element::IntrinsicState() const
{
  return IsEditable() ? NS_EVENT_STATE_MOZ_READWRITE :
                        NS_EVENT_STATE_MOZ_READONLY;
}

void
Element::NotifyStateChange(EventStates aStates)
{
  nsIDocument* doc = GetCrossShadowCurrentDoc();
  if (doc) {
    nsAutoScriptBlocker scriptBlocker;
    doc->ContentStateChanged(this, aStates);
  }
}

void
Element::UpdateLinkState(EventStates aState)
{
  MOZ_ASSERT(!aState.HasAtLeastOneOfStates(~(NS_EVENT_STATE_VISITED |
                                             NS_EVENT_STATE_UNVISITED)),
             "Unexpected link state bits");
  mState =
    (mState & ~(NS_EVENT_STATE_VISITED | NS_EVENT_STATE_UNVISITED)) |
    aState;
}

void
Element::UpdateState(bool aNotify)
{
  EventStates oldState = mState;
  mState = IntrinsicState() | (oldState & ESM_MANAGED_STATES);
  if (aNotify) {
    EventStates changedStates = oldState ^ mState;
    if (!changedStates.IsEmpty()) {
      nsIDocument* doc = GetCrossShadowCurrentDoc();
      if (doc) {
        nsAutoScriptBlocker scriptBlocker;
        doc->ContentStateChanged(this, changedStates);
      }
    }
  }
}

void
nsIContent::UpdateEditableState(bool aNotify)
{
  
  NS_ASSERTION(!IsElement(), "What happened here?");
  nsIContent *parent = GetParent();

  
  
  bool isUnknownNativeAnon = false;
  if (IsInNativeAnonymousSubtree()) {
    isUnknownNativeAnon = true;
    nsCOMPtr<nsIContent> root = this;
    while (root && !root->IsRootOfNativeAnonymousSubtree()) {
      root = root->GetParent();
    }
    
    if (root) {
      nsIFrame* rootFrame = root->GetPrimaryFrame();
      if (rootFrame) {
        nsContainerFrame* parentFrame = rootFrame->GetParent();
        nsITextControlFrame* textCtrl = do_QueryFrame(parentFrame);
        isUnknownNativeAnon = !textCtrl;
      }
    }
  }

  SetEditableFlag(parent && parent->HasFlag(NODE_IS_EDITABLE) &&
                  !isUnknownNativeAnon);
}

void
Element::UpdateEditableState(bool aNotify)
{
  nsIContent *parent = GetParent();

  SetEditableFlag(parent && parent->HasFlag(NODE_IS_EDITABLE));
  if (aNotify) {
    UpdateState(aNotify);
  } else {
    
    
    
    
    if (IsEditable()) {
      RemoveStatesSilently(NS_EVENT_STATE_MOZ_READONLY);
      AddStatesSilently(NS_EVENT_STATE_MOZ_READWRITE);
    } else {
      RemoveStatesSilently(NS_EVENT_STATE_MOZ_READWRITE);
      AddStatesSilently(NS_EVENT_STATE_MOZ_READONLY);
    }
  }
}

EventStates
Element::StyleStateFromLocks() const
{
  EventStates locks = LockedStyleStates();
  EventStates state = mState | locks;

  if (locks.HasState(NS_EVENT_STATE_VISITED)) {
    return state & ~NS_EVENT_STATE_UNVISITED;
  }
  if (locks.HasState(NS_EVENT_STATE_UNVISITED)) {
    return state & ~NS_EVENT_STATE_VISITED;
  }
  return state;
}

EventStates
Element::LockedStyleStates() const
{
  EventStates* locks =
    static_cast<EventStates*>(GetProperty(nsGkAtoms::lockedStyleStates));
  if (locks) {
    return *locks;
  }
  return EventStates();
}

void
Element::NotifyStyleStateChange(EventStates aStates)
{
  nsIDocument* doc = GetComposedDoc();
  if (doc) {
    nsIPresShell *presShell = doc->GetShell();
    if (presShell) {
      nsAutoScriptBlocker scriptBlocker;
      presShell->ContentStateChanged(doc, this, aStates);
    }
  }
}

void
Element::LockStyleStates(EventStates aStates)
{
  EventStates* locks = new EventStates(LockedStyleStates());

  *locks |= aStates;

  if (aStates.HasState(NS_EVENT_STATE_VISITED)) {
    *locks &= ~NS_EVENT_STATE_UNVISITED;
  }
  if (aStates.HasState(NS_EVENT_STATE_UNVISITED)) {
    *locks &= ~NS_EVENT_STATE_VISITED;
  }

  SetProperty(nsGkAtoms::lockedStyleStates, locks,
              nsINode::DeleteProperty<EventStates>);
  SetHasLockedStyleStates();

  NotifyStyleStateChange(aStates);
}

void
Element::UnlockStyleStates(EventStates aStates)
{
  EventStates* locks = new EventStates(LockedStyleStates());

  *locks &= ~aStates;

  if (locks->IsEmpty()) {
    DeleteProperty(nsGkAtoms::lockedStyleStates);
    ClearHasLockedStyleStates();
    delete locks;
  }
  else {
    SetProperty(nsGkAtoms::lockedStyleStates, locks,
                nsINode::DeleteProperty<EventStates>);
  }

  NotifyStyleStateChange(aStates);
}

void
Element::ClearStyleStateLocks()
{
  EventStates locks = LockedStyleStates();

  DeleteProperty(nsGkAtoms::lockedStyleStates);
  ClearHasLockedStyleStates();

  NotifyStyleStateChange(locks);
}

bool
Element::GetBindingURL(nsIDocument *aDocument, css::URLValue **aResult)
{
  
  
  
  
  bool isXULorPluginElement = (IsXULElement() ||
                               IsHTMLElement(nsGkAtoms::object) ||
                               IsHTMLElement(nsGkAtoms::embed) ||
                               IsHTMLElement(nsGkAtoms::applet));
  nsCOMPtr<nsIPresShell> shell = aDocument->GetShell();
  if (!shell || GetPrimaryFrame() || !isXULorPluginElement) {
    *aResult = nullptr;

    return true;
  }

  
  nsRefPtr<nsStyleContext> sc =
    nsComputedDOMStyle::GetStyleContextForElementNoFlush(this, nullptr, shell);
  NS_ENSURE_TRUE(sc, false);

  *aResult = sc->StyleDisplay()->mBinding;

  return true;
}

JSObject*
Element::WrapObject(JSContext *aCx, JS::Handle<JSObject*> aGivenProto)
{
  JS::Rooted<JSObject*> givenProto(aCx, aGivenProto);
  JS::Rooted<JSObject*> customProto(aCx);

  if (!givenProto) {
    
    CustomElementData* data = GetCustomElementData();
    if (data) {
      
      nsDocument* document = static_cast<nsDocument*>(OwnerDoc());
      document->GetCustomPrototype(NodeInfo()->NamespaceID(), data->mType, &customProto);
      if (customProto &&
          NodePrincipal()->SubsumesConsideringDomain(nsContentUtils::ObjectPrincipal(customProto))) {
        
        
        
        givenProto = customProto;
        customProto = nullptr;
      }
    }
  }

  JS::Rooted<JSObject*> obj(aCx, nsINode::WrapObject(aCx, givenProto));
  if (!obj) {
    return nullptr;
  }

  if (customProto) {
    
    
    
    
    
    JSAutoCompartment ac(aCx, customProto);
    JS::Rooted<JSObject*> wrappedObj(aCx, obj);
    if (!JS_WrapObject(aCx, &wrappedObj) ||
        !JS_SetPrototype(aCx, wrappedObj, customProto)) {
      return nullptr;
    }
  }

  nsIDocument* doc;
  if (HasFlag(NODE_FORCE_XBL_BINDINGS)) {
    doc = OwnerDoc();
  }
  else {
    doc = GetComposedDoc();
  }

  if (!doc) {
    
    
    return obj;
  }

  
  

  if (HasFlag(NODE_MAY_BE_IN_BINDING_MNGR) && GetXBLBinding()) {
    
    

    
    
    
    
    return obj;
  }

  
  
  mozilla::css::URLValue *bindingURL;
  bool ok = GetBindingURL(doc, &bindingURL);
  if (!ok) {
    dom::Throw(aCx, NS_ERROR_FAILURE);
    return nullptr;
  }

  if (!bindingURL) {
    
    return obj;
  }

  nsCOMPtr<nsIURI> uri = bindingURL->GetURI();
  nsCOMPtr<nsIPrincipal> principal = bindingURL->mOriginPrincipal;

  
  bool dummy;

  nsXBLService* xblService = nsXBLService::GetInstance();
  if (!xblService) {
    dom::Throw(aCx, NS_ERROR_NOT_AVAILABLE);
    return nullptr;
  }

  {
    
    nsRefPtr<nsXBLBinding> binding;
    xblService->LoadBindings(this, uri, principal, getter_AddRefs(binding), &dummy);

    if (binding) {
      if (nsContentUtils::IsSafeToRunScript()) {
        binding->ExecuteAttachedHandler();
      }
      else {
        nsContentUtils::AddScriptRunner(
          NS_NewRunnableMethod(binding, &nsXBLBinding::ExecuteAttachedHandler));
      }
    }
  }

  return obj;
}


nsINode*
Element::GetScopeChainParent() const
{
  return OwnerDoc();
}

nsDOMTokenList*
Element::ClassList()
{
  Element::nsDOMSlots* slots = DOMSlots();

  if (!slots->mClassList) {
    slots->mClassList = new nsDOMTokenList(this, nsGkAtoms::_class);
  }

  return slots->mClassList;
}

void
Element::GetClassList(nsISupports** aClassList)
{
  NS_ADDREF(*aClassList = ClassList());
}

already_AddRefed<nsIHTMLCollection>
Element::GetElementsByTagName(const nsAString& aLocalName)
{
  return NS_GetContentList(this, kNameSpaceID_Unknown, aLocalName);
}

void
Element::GetElementsByTagName(const nsAString& aLocalName,
                              nsIDOMHTMLCollection** aResult)
{
  *aResult = GetElementsByTagName(aLocalName).take();
}

nsIFrame*
Element::GetStyledFrame()
{
  nsIFrame *frame = GetPrimaryFrame(Flush_Layout);
  return frame ? nsLayoutUtils::GetStyleFrame(frame) : nullptr;
}

nsIScrollableFrame*
Element::GetScrollFrame(nsIFrame **aStyledFrame, bool aFlushLayout)
{
  
  if (IsSVGElement()) {
    if (aStyledFrame) {
      *aStyledFrame = nullptr;
    }
    return nullptr;
  }

  
  nsIFrame* frame = GetPrimaryFrame(aFlushLayout ? Flush_Layout : Flush_None);
  if (frame) {
    frame = nsLayoutUtils::GetStyleFrame(frame);
  }

  if (aStyledFrame) {
    *aStyledFrame = frame;
  }
  if (!frame) {
    return nullptr;
  }

  
  
  nsIAtom* type = frame->GetType();
  if (type != nsGkAtoms::menuFrame && type != nsGkAtoms::comboboxControlFrame) {
    nsIScrollableFrame *scrollFrame = frame->GetScrollTargetFrame();
    if (scrollFrame)
      return scrollFrame;
  }

  nsIDocument* doc = OwnerDoc();
  bool quirksMode = doc->GetCompatibilityMode() == eCompatibility_NavQuirks;
  Element* elementWithRootScrollInfo =
    quirksMode ? doc->GetBodyElement() : doc->GetRootElement();
  if (this == elementWithRootScrollInfo) {
    
    
    
    
    return frame->PresContext()->PresShell()->GetRootScrollFrameAsScrollable();
  }

  return nullptr;
}

void
Element::ScrollIntoView()
{
  ScrollIntoView(ScrollIntoViewOptions());
}

void
Element::ScrollIntoView(bool aTop)
{
  ScrollIntoViewOptions options;
  if (!aTop) {
    options.mBlock = ScrollLogicalPosition::End;
  }
  ScrollIntoView(options);
}

void
Element::ScrollIntoView(const ScrollIntoViewOptions &aOptions)
{
  nsIDocument *document = GetComposedDoc();
  if (!document) {
    return;
  }

  
  nsCOMPtr<nsIPresShell> presShell = document->GetShell();
  if (!presShell) {
    return;
  }

  int16_t vpercent = (aOptions.mBlock == ScrollLogicalPosition::Start)
                       ? nsIPresShell::SCROLL_TOP
                       : nsIPresShell::SCROLL_BOTTOM;

  uint32_t flags = nsIPresShell::SCROLL_OVERFLOW_HIDDEN;
  if (aOptions.mBehavior == ScrollBehavior::Smooth) {
    flags |= nsIPresShell::SCROLL_SMOOTH;
  } else if (aOptions.mBehavior == ScrollBehavior::Auto) {
    flags |= nsIPresShell::SCROLL_SMOOTH_AUTO;
  }

  presShell->ScrollContentIntoView(this,
                                   nsIPresShell::ScrollAxis(
                                     vpercent,
                                     nsIPresShell::SCROLL_ALWAYS),
                                   nsIPresShell::ScrollAxis(),
                                   flags);
}

void
Element::Scroll(const CSSIntPoint& aScroll, const ScrollOptions& aOptions)
{
  nsIScrollableFrame* sf = GetScrollFrame();
  if (sf) {
    nsIScrollableFrame::ScrollMode scrollMode = nsIScrollableFrame::INSTANT;
    if (aOptions.mBehavior == ScrollBehavior::Smooth) {
      scrollMode = nsIScrollableFrame::SMOOTH_MSD;
    } else if (aOptions.mBehavior == ScrollBehavior::Auto) {
      ScrollbarStyles styles = sf->GetScrollbarStyles();
      if (styles.mScrollBehavior == NS_STYLE_SCROLL_BEHAVIOR_SMOOTH) {
        scrollMode = nsIScrollableFrame::SMOOTH_MSD;
      }
    }

    sf->ScrollToCSSPixels(aScroll, scrollMode);
  }
}

void
Element::Scroll(double aXScroll, double aYScroll)
{
  
  CSSIntPoint scrollPos(mozilla::ToZeroIfNonfinite(aXScroll),
                        mozilla::ToZeroIfNonfinite(aYScroll));

  Scroll(scrollPos, ScrollOptions());
}

void
Element::Scroll(const ScrollToOptions& aOptions)
{
  nsIScrollableFrame *sf = GetScrollFrame();
  if (sf) {
    CSSIntPoint scrollPos = sf->GetScrollPositionCSSPixels();
    if (aOptions.mLeft.WasPassed()) {
      scrollPos.x = mozilla::ToZeroIfNonfinite(aOptions.mLeft.Value());
    }
    if (aOptions.mTop.WasPassed()) {
      scrollPos.y = mozilla::ToZeroIfNonfinite(aOptions.mTop.Value());
    }
    Scroll(scrollPos, aOptions);
  }
}

void
Element::ScrollTo(double aXScroll, double aYScroll)
{
  Scroll(aXScroll, aYScroll);
}

void
Element::ScrollTo(const ScrollToOptions& aOptions)
{
  Scroll(aOptions);
}

void
Element::ScrollBy(double aXScrollDif, double aYScrollDif)
{
  nsIScrollableFrame *sf = GetScrollFrame();
  if (sf) {
    CSSIntPoint scrollPos = sf->GetScrollPositionCSSPixels();
    scrollPos += CSSIntPoint(mozilla::ToZeroIfNonfinite(aXScrollDif),
                             mozilla::ToZeroIfNonfinite(aYScrollDif));
    Scroll(scrollPos, ScrollOptions());
  }
}

void
Element::ScrollBy(const ScrollToOptions& aOptions)
{
  nsIScrollableFrame *sf = GetScrollFrame();
  if (sf) {
    CSSIntPoint scrollPos = sf->GetScrollPositionCSSPixels();
    if (aOptions.mLeft.WasPassed()) {
      scrollPos.x += mozilla::ToZeroIfNonfinite(aOptions.mLeft.Value());
    }
    if (aOptions.mTop.WasPassed()) {
      scrollPos.y += mozilla::ToZeroIfNonfinite(aOptions.mTop.Value());
    }
    Scroll(scrollPos, aOptions);
  }
}

int32_t
Element::ScrollTop()
{
  nsIScrollableFrame* sf = GetScrollFrame();
  return sf ? sf->GetScrollPositionCSSPixels().y : 0;
}

void
Element::SetScrollTop(int32_t aScrollTop)
{
  nsIScrollableFrame* sf = GetScrollFrame();
  if (sf) {
    nsIScrollableFrame::ScrollMode scrollMode = nsIScrollableFrame::INSTANT;
    if (sf->GetScrollbarStyles().mScrollBehavior == NS_STYLE_SCROLL_BEHAVIOR_SMOOTH) {
      scrollMode = nsIScrollableFrame::SMOOTH_MSD;
    }
    sf->ScrollToCSSPixels(CSSIntPoint(sf->GetScrollPositionCSSPixels().x,
                                      aScrollTop),
                          scrollMode);
  }
}

int32_t
Element::ScrollLeft()
{
  nsIScrollableFrame* sf = GetScrollFrame();
  return sf ? sf->GetScrollPositionCSSPixels().x : 0;
}

void
Element::SetScrollLeft(int32_t aScrollLeft)
{
  nsIScrollableFrame* sf = GetScrollFrame();
  if (sf) {
    nsIScrollableFrame::ScrollMode scrollMode = nsIScrollableFrame::INSTANT;
    if (sf->GetScrollbarStyles().mScrollBehavior == NS_STYLE_SCROLL_BEHAVIOR_SMOOTH) {
      scrollMode = nsIScrollableFrame::SMOOTH_MSD;
    }

    sf->ScrollToCSSPixels(CSSIntPoint(aScrollLeft,
                                      sf->GetScrollPositionCSSPixels().y),
                          scrollMode);
  }
}


bool
Element::ScrollByNoFlush(int32_t aDx, int32_t aDy)
{
  nsIScrollableFrame* sf = GetScrollFrame(nullptr, false);
  if (!sf) {
    return false;
  }

  nsWeakFrame weakRef(sf->GetScrolledFrame());

  CSSIntPoint before = sf->GetScrollPositionCSSPixels();
  sf->ScrollToCSSPixelsApproximate(CSSIntPoint(before.x + aDx, before.y + aDy));

  
  if (!weakRef.IsAlive()) {
    return false;
  }

  CSSIntPoint after = sf->GetScrollPositionCSSPixels();
  return (before != after);
}

void
Element::MozScrollSnap()
{
  nsIScrollableFrame* sf = GetScrollFrame(nullptr, false);
  if (sf) {
    sf->ScrollSnap();
  }
}

static nsSize GetScrollRectSizeForOverflowVisibleFrame(nsIFrame* aFrame)
{
  if (!aFrame) {
    return nsSize(0,0);
  }

  nsRect paddingRect = aFrame->GetPaddingRectRelativeToSelf();
  nsOverflowAreas overflowAreas(paddingRect, paddingRect);
  
  
  
  
  nsLayoutUtils::UnionChildOverflow(aFrame, overflowAreas);
  
  
  nsRect overflowRect =
    overflowAreas.ScrollableOverflow().UnionEdges(paddingRect);
  return nsLayoutUtils::GetScrolledRect(aFrame,
      overflowRect, paddingRect.Size(),
      aFrame->StyleVisibility()->mDirection).Size();
}

int32_t
Element::ScrollHeight()
{
  if (IsSVGElement())
    return 0;

  nsIScrollableFrame* sf = GetScrollFrame();
  nscoord height;
  if (sf) {
    height = sf->GetScrollRange().height + sf->GetScrollPortRect().height;
  } else {
    height = GetScrollRectSizeForOverflowVisibleFrame(GetStyledFrame()).height;
  }

  return nsPresContext::AppUnitsToIntCSSPixels(height);
}

int32_t
Element::ScrollWidth()
{
  if (IsSVGElement())
    return 0;

  nsIScrollableFrame* sf = GetScrollFrame();
  nscoord width;
  if (sf) {
    width = sf->GetScrollRange().width + sf->GetScrollPortRect().width;
  } else {
    width = GetScrollRectSizeForOverflowVisibleFrame(GetStyledFrame()).width;
  }

  return nsPresContext::AppUnitsToIntCSSPixels(width);
}

nsRect
Element::GetClientAreaRect()
{
  nsIFrame* styledFrame;
  nsIScrollableFrame* sf = GetScrollFrame(&styledFrame);

  if (sf) {
    return sf->GetScrollPortRect();
  }

  if (styledFrame &&
      (styledFrame->StyleDisplay()->mDisplay != NS_STYLE_DISPLAY_INLINE ||
       styledFrame->IsFrameOfType(nsIFrame::eReplaced))) {
    
    
    return styledFrame->GetPaddingRect() - styledFrame->GetPositionIgnoringScrolling();
  }

  
  return nsRect(0, 0, 0, 0);
}

already_AddRefed<DOMRect>
Element::GetBoundingClientRect()
{
  nsRefPtr<DOMRect> rect = new DOMRect(this);
  
  nsIFrame* frame = GetPrimaryFrame(Flush_Layout);
  if (!frame) {
    
    return rect.forget();
  }

  nsRect r = nsLayoutUtils::GetAllInFlowRectsUnion(frame,
          nsLayoutUtils::GetContainingBlockForClientRect(frame),
          nsLayoutUtils::RECTS_ACCOUNT_FOR_TRANSFORMS);
  rect->SetLayoutRect(r);
  return rect.forget();
}

already_AddRefed<DOMRectList>
Element::GetClientRects()
{
  nsRefPtr<DOMRectList> rectList = new DOMRectList(this);

  nsIFrame* frame = GetPrimaryFrame(Flush_Layout);
  if (!frame) {
    
    return rectList.forget();
  }

  nsLayoutUtils::RectListBuilder builder(rectList);
  nsLayoutUtils::GetAllInFlowRects(frame,
          nsLayoutUtils::GetContainingBlockForClientRect(frame), &builder,
          nsLayoutUtils::RECTS_ACCOUNT_FOR_TRANSFORMS);
  return rectList.forget();
}




void
Element::AddToIdTable(nsIAtom* aId)
{
  NS_ASSERTION(HasID(), "Node doesn't have an ID?");
  if (IsInShadowTree()) {
    ShadowRoot* containingShadow = GetContainingShadow();
    containingShadow->AddToIdTable(this, aId);
  } else {
    nsIDocument* doc = GetUncomposedDoc();
    if (doc && (!IsInAnonymousSubtree() || doc->IsXULDocument())) {
      doc->AddToIdTable(this, aId);
    }
  }
}

void
Element::RemoveFromIdTable()
{
  if (!HasID()) {
    return;
  }

  nsIAtom* id = DoGetID();
  if (IsInShadowTree()) {
    ShadowRoot* containingShadow = GetContainingShadow();
    
    
    if (containingShadow) {
      containingShadow->RemoveFromIdTable(this, id);
    }
  } else {
    nsIDocument* doc = GetUncomposedDoc();
    if (doc && (!IsInAnonymousSubtree() || doc->IsXULDocument())) {
      doc->RemoveFromIdTable(this, id);
    }
  }
}

already_AddRefed<ShadowRoot>
Element::CreateShadowRoot(ErrorResult& aError)
{
  nsAutoScriptBlocker scriptBlocker;

  nsRefPtr<mozilla::dom::NodeInfo> nodeInfo;
  nodeInfo = mNodeInfo->NodeInfoManager()->GetNodeInfo(
    nsGkAtoms::documentFragmentNodeName, nullptr, kNameSpaceID_None,
    nsIDOMNode::DOCUMENT_FRAGMENT_NODE);

  nsRefPtr<nsXBLDocumentInfo> docInfo = new nsXBLDocumentInfo(OwnerDoc());

  nsXBLPrototypeBinding* protoBinding = new nsXBLPrototypeBinding();
  aError = protoBinding->Init(NS_LITERAL_CSTRING("shadowroot"),
                              docInfo, nullptr, true);
  if (aError.Failed()) {
    delete protoBinding;
    return nullptr;
  }

  nsIDocument* doc = GetCrossShadowCurrentDoc();
  nsIContent* destroyedFramesFor = nullptr;
  if (doc) {
    nsIPresShell* shell = doc->GetShell();
    if (shell) {
      shell->DestroyFramesFor(this, &destroyedFramesFor);
      MOZ_ASSERT(!shell->FrameManager()->GetDisplayContentsStyleFor(this));
    }
  }
  MOZ_ASSERT(!GetPrimaryFrame());

  
  protoBinding->SetInheritsStyle(false);

  
  docInfo->SetPrototypeBinding(NS_LITERAL_CSTRING("shadowroot"), protoBinding);

  nsRefPtr<ShadowRoot> shadowRoot = new ShadowRoot(this, nodeInfo.forget(),
                                                   protoBinding);

  shadowRoot->SetIsComposedDocParticipant(IsInComposedDoc());

  
  
  
  
  ShadowRoot* olderShadow = GetShadowRoot();
  SetShadowRoot(shadowRoot);
  if (olderShadow) {
    olderShadow->SetYoungerShadow(shadowRoot);

    
    
    for (nsIContent* child = olderShadow->GetFirstChild(); child;
         child = child->GetNextSibling()) {
      child->UnbindFromTree(true, false);
    }

    olderShadow->SetIsComposedDocParticipant(false);
  }

  
  nsRefPtr<nsXBLBinding> xblBinding = new nsXBLBinding(shadowRoot, protoBinding);
  shadowRoot->SetAssociatedBinding(xblBinding);
  xblBinding->SetBoundElement(this);

  SetXBLBinding(xblBinding);

  
  
  if (doc) {
    MOZ_ASSERT(doc == GetCrossShadowCurrentDoc());
    nsIPresShell* shell = doc->GetShell();
    if (shell) {
      shell->CreateFramesFor(destroyedFramesFor);
    }
  }

  return shadowRoot.forget();
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(DestinationInsertionPointList, mParent,
                                      mDestinationPoints)

NS_INTERFACE_TABLE_HEAD(DestinationInsertionPointList)
  NS_WRAPPERCACHE_INTERFACE_TABLE_ENTRY
  NS_INTERFACE_TABLE(DestinationInsertionPointList, nsINodeList, nsIDOMNodeList)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE_CYCLE_COLLECTION(DestinationInsertionPointList)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(DestinationInsertionPointList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DestinationInsertionPointList)

DestinationInsertionPointList::DestinationInsertionPointList(Element* aElement)
  : mParent(aElement)
{
  nsTArray<nsIContent*>* destPoints = aElement->GetExistingDestInsertionPoints();
  if (destPoints) {
    for (uint32_t i = 0; i < destPoints->Length(); i++) {
      mDestinationPoints.AppendElement(destPoints->ElementAt(i));
    }
  }
}

DestinationInsertionPointList::~DestinationInsertionPointList()
{
}

nsIContent*
DestinationInsertionPointList::Item(uint32_t aIndex)
{
  return mDestinationPoints.SafeElementAt(aIndex);
}

NS_IMETHODIMP
DestinationInsertionPointList::Item(uint32_t aIndex, nsIDOMNode** aReturn)
{
  nsIContent* item = Item(aIndex);
  if (!item) {
    return NS_ERROR_FAILURE;
  }

  return CallQueryInterface(item, aReturn);
}

uint32_t
DestinationInsertionPointList::Length() const
{
  return mDestinationPoints.Length();
}

NS_IMETHODIMP
DestinationInsertionPointList::GetLength(uint32_t* aLength)
{
  *aLength = Length();
  return NS_OK;
}

int32_t
DestinationInsertionPointList::IndexOf(nsIContent* aContent)
{
  return mDestinationPoints.IndexOf(aContent);
}

JSObject*
DestinationInsertionPointList::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return NodeListBinding::Wrap(aCx, this, aGivenProto);
}

already_AddRefed<DestinationInsertionPointList>
Element::GetDestinationInsertionPoints()
{
  nsRefPtr<DestinationInsertionPointList> list =
    new DestinationInsertionPointList(this);
  return list.forget();
}

void
Element::GetAttribute(const nsAString& aName, DOMString& aReturn)
{
  const nsAttrValue* val =
    mAttrsAndChildren.GetAttr(aName,
                              IsHTMLElement() && IsInHTMLDocument() ?
                                eIgnoreCase : eCaseMatters);
  if (val) {
    val->ToString(aReturn);
  } else {
    if (IsXULElement()) {
      
      
      
    } else {
      aReturn.SetNull();
    }
  }
}

void
Element::SetAttribute(const nsAString& aName,
                      const nsAString& aValue,
                      ErrorResult& aError)
{
  const nsAttrName* name = InternalGetExistingAttrNameFromQName(aName);

  if (!name) {
    aError = nsContentUtils::CheckQName(aName, false);
    if (aError.Failed()) {
      return;
    }

    nsCOMPtr<nsIAtom> nameAtom;
    if (IsHTMLElement() && IsInHTMLDocument()) {
      nsAutoString lower;
      nsContentUtils::ASCIIToLower(aName, lower);
      nameAtom = do_GetAtom(lower);
    }
    else {
      nameAtom = do_GetAtom(aName);
    }
    if (!nameAtom) {
      aError.Throw(NS_ERROR_OUT_OF_MEMORY);
      return;
    }
    aError = SetAttr(kNameSpaceID_None, nameAtom, aValue, true);
    return;
  }

  aError = SetAttr(name->NamespaceID(), name->LocalName(), name->GetPrefix(),
                   aValue, true);
  return;
}

void
Element::RemoveAttribute(const nsAString& aName, ErrorResult& aError)
{
  const nsAttrName* name = InternalGetExistingAttrNameFromQName(aName);

  if (!name) {
    
    
    
    return;
  }

  
  
  
  nsAttrName tmp(*name);

  aError = UnsetAttr(name->NamespaceID(), name->LocalName(), true);
}

Attr*
Element::GetAttributeNode(const nsAString& aName)
{
  OwnerDoc()->WarnOnceAbout(nsIDocument::eGetAttributeNode);
  return Attributes()->GetNamedItem(aName);
}

already_AddRefed<Attr>
Element::SetAttributeNode(Attr& aNewAttr, ErrorResult& aError)
{
  OwnerDoc()->WarnOnceAbout(nsIDocument::eSetAttributeNode);

  return Attributes()->SetNamedItem(aNewAttr, aError);
}

already_AddRefed<Attr>
Element::RemoveAttributeNode(Attr& aAttribute,
                             ErrorResult& aError)
{
  Element *elem = aAttribute.GetElement();
  if (elem != this) {
    aError.Throw(NS_ERROR_DOM_NOT_FOUND_ERR);
    return nullptr;
  }

  OwnerDoc()->WarnOnceAbout(nsIDocument::eRemoveAttributeNode);
  return Attributes()->RemoveNamedItem(aAttribute.NodeName(), aError);
}

void
Element::GetAttributeNS(const nsAString& aNamespaceURI,
                        const nsAString& aLocalName,
                        nsAString& aReturn)
{
  int32_t nsid =
    nsContentUtils::NameSpaceManager()->GetNameSpaceID(aNamespaceURI);

  if (nsid == kNameSpaceID_Unknown) {
    
    SetDOMStringToNull(aReturn);
    return;
  }

  nsCOMPtr<nsIAtom> name = do_GetAtom(aLocalName);
  bool hasAttr = GetAttr(nsid, name, aReturn);
  if (!hasAttr) {
    SetDOMStringToNull(aReturn);
  }
}

void
Element::SetAttributeNS(const nsAString& aNamespaceURI,
                        const nsAString& aQualifiedName,
                        const nsAString& aValue,
                        ErrorResult& aError)
{
  nsRefPtr<mozilla::dom::NodeInfo> ni;
  aError =
    nsContentUtils::GetNodeInfoFromQName(aNamespaceURI, aQualifiedName,
                                         mNodeInfo->NodeInfoManager(),
                                         nsIDOMNode::ATTRIBUTE_NODE,
                                         getter_AddRefs(ni));
  if (aError.Failed()) {
    return;
  }

  aError = SetAttr(ni->NamespaceID(), ni->NameAtom(), ni->GetPrefixAtom(),
                   aValue, true);
}

void
Element::RemoveAttributeNS(const nsAString& aNamespaceURI,
                           const nsAString& aLocalName,
                           ErrorResult& aError)
{
  nsCOMPtr<nsIAtom> name = do_GetAtom(aLocalName);
  int32_t nsid =
    nsContentUtils::NameSpaceManager()->GetNameSpaceID(aNamespaceURI);

  if (nsid == kNameSpaceID_Unknown) {
    
    
    
    return;
  }

  aError = UnsetAttr(nsid, name, true);
}

Attr*
Element::GetAttributeNodeNS(const nsAString& aNamespaceURI,
                            const nsAString& aLocalName)
{
  OwnerDoc()->WarnOnceAbout(nsIDocument::eGetAttributeNodeNS);

  return GetAttributeNodeNSInternal(aNamespaceURI, aLocalName);
}

Attr*
Element::GetAttributeNodeNSInternal(const nsAString& aNamespaceURI,
                                    const nsAString& aLocalName)
{
  return Attributes()->GetNamedItemNS(aNamespaceURI, aLocalName);
}

already_AddRefed<Attr>
Element::SetAttributeNodeNS(Attr& aNewAttr,
                            ErrorResult& aError)
{
  OwnerDoc()->WarnOnceAbout(nsIDocument::eSetAttributeNodeNS);
  return Attributes()->SetNamedItemNS(aNewAttr, aError);
}

already_AddRefed<nsIHTMLCollection>
Element::GetElementsByTagNameNS(const nsAString& aNamespaceURI,
                                const nsAString& aLocalName,
                                ErrorResult& aError)
{
  int32_t nameSpaceId = kNameSpaceID_Wildcard;

  if (!aNamespaceURI.EqualsLiteral("*")) {
    aError =
      nsContentUtils::NameSpaceManager()->RegisterNameSpace(aNamespaceURI,
                                                            nameSpaceId);
    if (aError.Failed()) {
      return nullptr;
    }
  }

  NS_ASSERTION(nameSpaceId != kNameSpaceID_Unknown, "Unexpected namespace ID!");

  return NS_GetContentList(this, nameSpaceId, aLocalName);
}

nsresult
Element::GetElementsByTagNameNS(const nsAString& namespaceURI,
                                const nsAString& localName,
                                nsIDOMHTMLCollection** aResult)
{
  mozilla::ErrorResult rv;
  nsCOMPtr<nsIHTMLCollection> list =
    GetElementsByTagNameNS(namespaceURI, localName, rv);
  if (rv.Failed()) {
    return rv.StealNSResult();
  }
  list.forget(aResult);
  return NS_OK;
}

bool
Element::HasAttributeNS(const nsAString& aNamespaceURI,
                        const nsAString& aLocalName) const
{
  int32_t nsid =
    nsContentUtils::NameSpaceManager()->GetNameSpaceID(aNamespaceURI);

  if (nsid == kNameSpaceID_Unknown) {
    
    return false;
  }

  nsCOMPtr<nsIAtom> name = do_GetAtom(aLocalName);
  return HasAttr(nsid, name);
}

already_AddRefed<nsIHTMLCollection>
Element::GetElementsByClassName(const nsAString& aClassNames)
{
  return nsContentUtils::GetElementsByClassName(this, aClassNames);
}

nsresult
Element::GetElementsByClassName(const nsAString& aClassNames,
                                nsIDOMHTMLCollection** aResult)
{
  *aResult =
    nsContentUtils::GetElementsByClassName(this, aClassNames).take();
  return NS_OK;
}

nsresult
Element::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                    nsIContent* aBindingParent,
                    bool aCompileEventHandlers)
{
  NS_PRECONDITION(aParent || aDocument, "Must have document if no parent!");
  NS_PRECONDITION((NODE_FROM(aParent, aDocument)->OwnerDoc() == OwnerDoc()),
                  "Must have the same owner document");
  NS_PRECONDITION(!aParent || aDocument == aParent->GetUncomposedDoc(),
                  "aDocument must be current doc of aParent");
  NS_PRECONDITION(!GetUncomposedDoc(), "Already have a document.  Unbind first!");
  
  
  NS_PRECONDITION(!GetParent() || aParent == GetParent(),
                  "Already have a parent.  Unbind first!");
  NS_PRECONDITION(!GetBindingParent() ||
                  aBindingParent == GetBindingParent() ||
                  (!aBindingParent && aParent &&
                   aParent->GetBindingParent() == GetBindingParent()),
                  "Already have a binding parent.  Unbind first!");
  NS_PRECONDITION(!aParent || !aDocument ||
                  !aParent->HasFlag(NODE_FORCE_XBL_BINDINGS),
                  "Parent in document but flagged as forcing XBL");
  NS_PRECONDITION(aBindingParent != this,
                  "Content must not be its own binding parent");
  NS_PRECONDITION(!IsRootOfNativeAnonymousSubtree() ||
                  aBindingParent == aParent,
                  "Native anonymous content must have its parent as its "
                  "own binding parent");
  NS_PRECONDITION(aBindingParent || !aParent ||
                  aBindingParent == aParent->GetBindingParent(),
                  "We should be passed the right binding parent");

#ifdef MOZ_XUL
  
  nsXULElement* xulElem = nsXULElement::FromContent(this);
  if (xulElem) {
    xulElem->SetXULBindingParent(aBindingParent);
  }
  else 
#endif
  {
    if (aBindingParent) {
      nsDOMSlots *slots = DOMSlots();

      slots->mBindingParent = aBindingParent; 
    }
  }
  NS_ASSERTION(!aBindingParent || IsRootOfNativeAnonymousSubtree() ||
               !HasFlag(NODE_IS_IN_NATIVE_ANONYMOUS_SUBTREE) ||
               (aParent && aParent->IsInNativeAnonymousSubtree()),
               "Trying to re-bind content from native anonymous subtree to "
               "non-native anonymous parent!");
  if (aParent) {
    if (aParent->IsInNativeAnonymousSubtree()) {
      SetFlags(NODE_IS_IN_NATIVE_ANONYMOUS_SUBTREE);
    }
    if (aParent->HasFlag(NODE_CHROME_ONLY_ACCESS)) {
      SetFlags(NODE_CHROME_ONLY_ACCESS);
    }
    if (aParent->IsInShadowTree()) {
      ClearSubtreeRootPointer();
      SetFlags(NODE_IS_IN_SHADOW_TREE);
    }
    ShadowRoot* parentContainingShadow = aParent->GetContainingShadow();
    if (parentContainingShadow) {
      DOMSlots()->mContainingShadow = parentContainingShadow;
    }
  }

  bool hadForceXBL = HasFlag(NODE_FORCE_XBL_BINDINGS);

  
  if (aParent) {
    if (!GetParent()) {
      NS_ADDREF(aParent);
    }
    mParent = aParent;

    if (aParent->HasFlag(NODE_FORCE_XBL_BINDINGS)) {
      SetFlags(NODE_FORCE_XBL_BINDINGS);
    }
  }
  else {
    mParent = aDocument;
  }
  SetParentIsContent(aParent);

  

  
  if (aDocument) {
    
    
    
    
    
    
    
    

    
    
    ClearSubtreeRootPointer();

    
    SetInDocument();

    
    UnsetFlags(NODE_FORCE_XBL_BINDINGS |
               
               NODE_NEEDS_FRAME | NODE_DESCENDANTS_NEED_FRAMES |
               
               ELEMENT_ALL_RESTYLE_FLAGS);
  } else if (IsInShadowTree()) {
    
    
    
    
    
    
    UnsetFlags(NODE_FORCE_XBL_BINDINGS |
               NODE_NEEDS_FRAME | NODE_DESCENDANTS_NEED_FRAMES |
               ELEMENT_ALL_RESTYLE_FLAGS);
  } else {
    
    
    SetSubtreeRootPointer(aParent->SubtreeRoot());
  }

  nsIDocument* composedDoc = GetComposedDoc();
  if (composedDoc) {
    
    
    if (GetCustomElementData() && composedDoc->GetDocShell()) {
      
      composedDoc->EnqueueLifecycleCallback(nsIDocument::eAttached, this);
    }
  }

  
  if (mParent->IsContent()) {
    nsIContent* parent;
    ShadowRoot* shadowRootParent = ShadowRoot::FromNode(mParent);
    if (shadowRootParent) {
      parent = shadowRootParent->GetHost();
    } else {
      parent = mParent->AsContent();
    }

    bool inStyleScope = parent->IsElementInStyleScope();

    SetIsElementInStyleScope(inStyleScope);
    SetIsElementInStyleScopeFlagOnShadowTree(inStyleScope);
  }

  
  
  
  if (IsHTMLElement()) {
    SetDirOnBind(this, aParent);
  }

  
  
  nsresult rv;
  if (hadForceXBL) {
    nsBindingManager* bmgr = OwnerDoc()->BindingManager();

    nsXBLBinding* contBinding = bmgr->GetBindingWithContent(this);
    
    if (contBinding) {
      nsCOMPtr<nsIContent> anonRoot = contBinding->GetAnonymousContent();
      bool allowScripts = contBinding->AllowScripts();
      for (nsCOMPtr<nsIContent> child = anonRoot->GetFirstChild();
           child;
           child = child->GetNextSibling()) {
        rv = child->BindToTree(aDocument, this, this, allowScripts);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }

  UpdateEditableState(false);

  
  for (nsIContent* child = GetFirstChild(); child;
       child = child->GetNextSibling()) {
    rv = child->BindToTree(aDocument, this, aBindingParent,
                           aCompileEventHandlers);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsNodeUtils::ParentChainChanged(this);

  if (HasID()) {
    AddToIdTable(DoGetID());
  }

  if (MayHaveStyle() && !IsXULElement()) {
    
    
    
    static_cast<nsStyledElement*>(this)->ReparseStyleAttribute(false);
  }

  if (aDocument) {
    
    
    
    
    nsHTMLStyleSheet* sheet = aDocument->GetAttributeStyleSheet();
    if (sheet) {
      mAttrsAndChildren.SetMappedAttrStyleSheet(sheet);
    }
  }

  
  ShadowRoot* shadowRoot = GetShadowRoot();
  if (shadowRoot) {
    shadowRoot->SetIsComposedDocParticipant(IsInComposedDoc());
    for (nsIContent* child = shadowRoot->GetFirstChild(); child;
         child = child->GetNextSibling()) {
      rv = child->BindToTree(nullptr, shadowRoot,
                             shadowRoot->GetBindingParent(),
                             aCompileEventHandlers);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  
  
  
  NS_POSTCONDITION(aDocument == GetUncomposedDoc(), "Bound to wrong document");
  NS_POSTCONDITION(aParent == GetParent(), "Bound to wrong parent");
  NS_POSTCONDITION(aBindingParent == GetBindingParent(),
                   "Bound to wrong binding parent");

  return NS_OK;
}

RemoveFromBindingManagerRunnable::RemoveFromBindingManagerRunnable(nsBindingManager* aManager,
                                                                   nsIContent* aContent,
                                                                   nsIDocument* aDoc):
  mManager(aManager), mContent(aContent), mDoc(aDoc)
{}

RemoveFromBindingManagerRunnable::~RemoveFromBindingManagerRunnable() {}

NS_IMETHODIMP
RemoveFromBindingManagerRunnable::Run()
{
  
  
  
  
  
  
  
  if (!mContent->IsInComposedDoc()) {
    mManager->RemovedFromDocumentInternal(mContent, mDoc);
  }

  return NS_OK;
}


void
Element::UnbindFromTree(bool aDeep, bool aNullParent)
{
  NS_PRECONDITION(aDeep || (!GetUncomposedDoc() && !GetBindingParent()),
                  "Shallow unbind won't clear document and binding parent on "
                  "kids!");

  RemoveFromIdTable();

  
  nsIDocument* document =
    HasFlag(NODE_FORCE_XBL_BINDINGS) ? OwnerDoc() : GetComposedDoc();

  if (aNullParent) {
    if (IsFullScreenAncestor()) {
      
      
      nsContentUtils::ReportToConsole(nsIScriptError::warningFlag,
                                      NS_LITERAL_CSTRING("DOM"), OwnerDoc(),
                                      nsContentUtils::eDOM_PROPERTIES,
                                      "RemovedFullScreenElement");
      
      nsIDocument::ExitFullscreenInDocTree(OwnerDoc());
    }
    if (HasPointerLock()) {
      nsIDocument::UnlockPointer();
    }
    if (GetParent()) {
      nsINode* p = mParent;
      mParent = nullptr;
      NS_RELEASE(p);
    } else {
      mParent = nullptr;
    }
    SetParentIsContent(false);
  }
  ClearInDocument();

  if (aNullParent || !mParent->IsInShadowTree()) {
    UnsetFlags(NODE_IS_IN_SHADOW_TREE);

    
    SetSubtreeRootPointer(aNullParent ? this : mParent->SubtreeRoot());
  }

  if (document) {
    
    
    
    
    if (HasFlag(NODE_MAY_BE_IN_BINDING_MNGR) && !GetShadowRoot()) {
      nsContentUtils::AddScriptRunner(
        new RemoveFromBindingManagerRunnable(document->BindingManager(), this,
                                             document));
    }

    document->ClearBoxObjectFor(this);

    
    
    if (GetCustomElementData() && document->GetDocShell()) {
      
      document->EnqueueLifecycleCallback(nsIDocument::eDetached, this);
    }
  }

  
  
  
  
  if (HasFlag(NODE_HAS_PROPERTIES)) {
    DeleteProperty(nsGkAtoms::transitionsOfBeforeProperty);
    DeleteProperty(nsGkAtoms::transitionsOfAfterProperty);
    DeleteProperty(nsGkAtoms::transitionsProperty);
    DeleteProperty(nsGkAtoms::animationsOfBeforeProperty);
    DeleteProperty(nsGkAtoms::animationsOfAfterProperty);
    DeleteProperty(nsGkAtoms::animationsProperty);
  }

  
  UnsetFlags(NODE_FORCE_XBL_BINDINGS);
  bool clearBindingParent = true;

#ifdef MOZ_XUL
  nsXULElement* xulElem = nsXULElement::FromContent(this);
  if (xulElem) {
    xulElem->SetXULBindingParent(nullptr);
    clearBindingParent = false;
  }
#endif

  nsDOMSlots* slots = GetExistingDOMSlots();
  if (slots) {
    if (clearBindingParent) {
      slots->mBindingParent = nullptr;
    }
    if (aNullParent || !mParent->IsInShadowTree()) {
      slots->mContainingShadow = nullptr;
    }
  }

  
  
  
  if (IsHTMLElement()) {
    ResetDir(this);
  }

  if (aDeep) {
    
    
    
    uint32_t i, n = mAttrsAndChildren.ChildCount();

    for (i = 0; i < n; ++i) {
      
      
      
      mAttrsAndChildren.ChildAt(i)->UnbindFromTree(true, false);
    }
  }

  nsNodeUtils::ParentChainChanged(this);

  
  ShadowRoot* shadowRoot = GetShadowRoot();
  if (shadowRoot) {
    for (nsIContent* child = shadowRoot->GetFirstChild(); child;
         child = child->GetNextSibling()) {
      child->UnbindFromTree(true, false);
    }

    shadowRoot->SetIsComposedDocParticipant(false);
  }
}

nsICSSDeclaration*
Element::GetSMILOverrideStyle()
{
  Element::nsDOMSlots *slots = DOMSlots();

  if (!slots->mSMILOverrideStyle) {
    slots->mSMILOverrideStyle = new nsDOMCSSAttributeDeclaration(this, true);
  }

  return slots->mSMILOverrideStyle;
}

css::StyleRule*
Element::GetSMILOverrideStyleRule()
{
  Element::nsDOMSlots *slots = GetExistingDOMSlots();
  return slots ? slots->mSMILOverrideStyleRule.get() : nullptr;
}

nsresult
Element::SetSMILOverrideStyleRule(css::StyleRule* aStyleRule,
                                           bool aNotify)
{
  Element::nsDOMSlots *slots = DOMSlots();

  slots->mSMILOverrideStyleRule = aStyleRule;

  if (aNotify) {
    nsIDocument* doc = GetComposedDoc();
    
    
    
    if (doc) {
      nsCOMPtr<nsIPresShell> shell = doc->GetShell();
      if (shell) {
        
        
        
        
        shell->RestyleForAnimation(this, eRestyle_StyleAttribute |
                                         eRestyle_StyleAttribute_Animations);
      }
    }
  }

  return NS_OK;
}

bool
Element::IsLabelable() const
{
  return false;
}

bool
Element::IsInteractiveHTMLContent(bool aIgnoreTabindex) const
{
  return false;
}

css::StyleRule*
Element::GetInlineStyleRule()
{
  return nullptr;
}

nsresult
Element::SetInlineStyleRule(css::StyleRule* aStyleRule,
                            const nsAString* aSerialized,
                            bool aNotify)
{
  NS_NOTYETIMPLEMENTED("Element::SetInlineStyleRule");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP_(bool)
Element::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  return false;
}

nsChangeHint
Element::GetAttributeChangeHint(const nsIAtom* aAttribute,
                                int32_t aModType) const
{
  return nsChangeHint(0);
}

bool
Element::FindAttributeDependence(const nsIAtom* aAttribute,
                                 const MappedAttributeEntry* const aMaps[],
                                 uint32_t aMapCount)
{
  for (uint32_t mapindex = 0; mapindex < aMapCount; ++mapindex) {
    for (const MappedAttributeEntry* map = aMaps[mapindex];
         map->attribute; ++map) {
      if (aAttribute == *map->attribute) {
        return true;
      }
    }
  }

  return false;
}

already_AddRefed<mozilla::dom::NodeInfo>
Element::GetExistingAttrNameFromQName(const nsAString& aStr) const
{
  const nsAttrName* name = InternalGetExistingAttrNameFromQName(aStr);
  if (!name) {
    return nullptr;
  }

  nsRefPtr<mozilla::dom::NodeInfo> nodeInfo;
  if (name->IsAtom()) {
    nodeInfo = mNodeInfo->NodeInfoManager()->
      GetNodeInfo(name->Atom(), nullptr, kNameSpaceID_None,
                  nsIDOMNode::ATTRIBUTE_NODE);
  }
  else {
    nodeInfo = name->NodeInfo();
  }

  return nodeInfo.forget();
}


bool
Element::ShouldBlur(nsIContent *aContent)
{
  
  
  nsIDocument* document = aContent->GetComposedDoc();
  if (!document)
    return false;

  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(document->GetWindow());
  if (!window)
    return false;

  nsCOMPtr<nsPIDOMWindow> focusedFrame;
  nsIContent* contentToBlur =
    nsFocusManager::GetFocusedDescendant(window, false, getter_AddRefs(focusedFrame));
  if (contentToBlur == aContent)
    return true;

  
  
  return (contentToBlur && nsFocusManager::GetRedirectedFocus(aContent) == contentToBlur);
}

bool
Element::IsNodeOfType(uint32_t aFlags) const
{
  return !(aFlags & ~eCONTENT);
}


nsresult
Element::DispatchEvent(nsPresContext* aPresContext,
                       WidgetEvent* aEvent,
                       nsIContent* aTarget,
                       bool aFullDispatch,
                       nsEventStatus* aStatus)
{
  NS_PRECONDITION(aTarget, "Must have target");
  NS_PRECONDITION(aEvent, "Must have source event");
  NS_PRECONDITION(aStatus, "Null out param?");

  if (!aPresContext) {
    return NS_OK;
  }

  nsCOMPtr<nsIPresShell> shell = aPresContext->GetPresShell();
  if (!shell) {
    return NS_OK;
  }

  if (aFullDispatch) {
    return shell->HandleEventWithTarget(aEvent, nullptr, aTarget, aStatus);
  }

  return shell->HandleDOMEventWithTarget(aTarget, aEvent, aStatus);
}


nsresult
Element::DispatchClickEvent(nsPresContext* aPresContext,
                            WidgetInputEvent* aSourceEvent,
                            nsIContent* aTarget,
                            bool aFullDispatch,
                            const EventFlags* aExtraEventFlags,
                            nsEventStatus* aStatus)
{
  NS_PRECONDITION(aTarget, "Must have target");
  NS_PRECONDITION(aSourceEvent, "Must have source event");
  NS_PRECONDITION(aStatus, "Null out param?");

  WidgetMouseEvent event(aSourceEvent->mFlags.mIsTrusted, NS_MOUSE_CLICK,
                         aSourceEvent->widget, WidgetMouseEvent::eReal);
  event.refPoint = aSourceEvent->refPoint;
  uint32_t clickCount = 1;
  float pressure = 0;
  uint16_t inputSource = 0;
  WidgetMouseEvent* sourceMouseEvent = aSourceEvent->AsMouseEvent();
  if (sourceMouseEvent) {
    clickCount = sourceMouseEvent->clickCount;
    pressure = sourceMouseEvent->pressure;
    inputSource = sourceMouseEvent->inputSource;
  } else if (aSourceEvent->mClass == eKeyboardEventClass) {
    inputSource = nsIDOMMouseEvent::MOZ_SOURCE_KEYBOARD;
  }
  event.pressure = pressure;
  event.clickCount = clickCount;
  event.inputSource = inputSource;
  event.modifiers = aSourceEvent->modifiers;
  if (aExtraEventFlags) {
    
    event.mFlags.Union(*aExtraEventFlags);
  }

  return DispatchEvent(aPresContext, &event, aTarget, aFullDispatch, aStatus);
}

nsIFrame*
Element::GetPrimaryFrame(mozFlushType aType)
{
  nsIDocument* doc = GetComposedDoc();
  if (!doc) {
    return nullptr;
  }

  
  
  if (aType != Flush_None) {
    doc->FlushPendingNotifications(aType);
  }

  return GetPrimaryFrame();
}


nsresult
Element::LeaveLink(nsPresContext* aPresContext)
{
  nsILinkHandler *handler = aPresContext->GetLinkHandler();
  if (!handler) {
    return NS_OK;
  }

  return handler->OnLeaveLink();
}

nsresult
Element::SetEventHandler(nsIAtom* aEventName,
                         const nsAString& aValue,
                         bool aDefer)
{
  nsIDocument *ownerDoc = OwnerDoc();
  if (ownerDoc->IsLoadedAsData()) {
    
    
    return NS_OK;
  }

  NS_PRECONDITION(aEventName, "Must have event name!");
  bool defer = true;
  EventListenerManager* manager =
    GetEventListenerManagerForAttr(aEventName, &defer);
  if (!manager) {
    return NS_OK;
  }

  defer = defer && aDefer; 
  manager->SetEventHandler(aEventName, aValue,
                           defer, !nsContentUtils::IsChromeDoc(ownerDoc),
                           this);
  return NS_OK;
}




const nsAttrName*
Element::InternalGetExistingAttrNameFromQName(const nsAString& aStr) const
{
  return mAttrsAndChildren.GetExistingAttrNameFromQName(aStr);
}

bool
Element::MaybeCheckSameAttrVal(int32_t aNamespaceID,
                               nsIAtom* aName,
                               nsIAtom* aPrefix,
                               const nsAttrValueOrString& aValue,
                               bool aNotify,
                               nsAttrValue& aOldValue,
                               uint8_t* aModType,
                               bool* aHasListeners)
{
  bool modification = false;
  *aHasListeners = aNotify &&
    nsContentUtils::HasMutationListeners(this,
                                         NS_EVENT_BITS_MUTATION_ATTRMODIFIED,
                                         this);

  
  
  
  
  
  if (*aHasListeners || aNotify) {
    nsAttrInfo info(GetAttrInfo(aNamespaceID, aName));
    if (info.mValue) {
      
      
      
      
      if (*aHasListeners || GetCustomElementData()) {
        
        
        
        
        
        
        
        
        
        aOldValue.SetToSerialized(*info.mValue);
      }
      bool valueMatches = aValue.EqualsAsStrings(*info.mValue);
      if (valueMatches && aPrefix == info.mName->GetPrefix()) {
        return true;
      }
      modification = true;
    }
  }
  *aModType = modification ?
    static_cast<uint8_t>(nsIDOMMutationEvent::MODIFICATION) :
    static_cast<uint8_t>(nsIDOMMutationEvent::ADDITION);
  return false;
}

bool
Element::OnlyNotifySameValueSet(int32_t aNamespaceID, nsIAtom* aName,
                                nsIAtom* aPrefix,
                                const nsAttrValueOrString& aValue,
                                bool aNotify, nsAttrValue& aOldValue,
                                uint8_t* aModType, bool* aHasListeners)
{
  if (!MaybeCheckSameAttrVal(aNamespaceID, aName, aPrefix, aValue, aNotify,
                             aOldValue, aModType, aHasListeners)) {
    return false;
  }

  nsAutoScriptBlocker scriptBlocker;
  nsNodeUtils::AttributeSetToCurrentValue(this, aNamespaceID, aName);
  return true;
}

nsresult
Element::SetAttr(int32_t aNamespaceID, nsIAtom* aName,
                 nsIAtom* aPrefix, const nsAString& aValue,
                 bool aNotify)
{
  

  NS_ENSURE_ARG_POINTER(aName);
  NS_ASSERTION(aNamespaceID != kNameSpaceID_Unknown,
               "Don't call SetAttr with unknown namespace");

  if (!mAttrsAndChildren.CanFitMoreAttrs()) {
    return NS_ERROR_FAILURE;
  }

  uint8_t modType;
  bool hasListeners;
  nsAttrValueOrString value(aValue);
  nsAttrValue oldValue;

  if (OnlyNotifySameValueSet(aNamespaceID, aName, aPrefix, value, aNotify,
                             oldValue, &modType, &hasListeners)) {
    return NS_OK;
  }

  nsresult rv = BeforeSetAttr(aNamespaceID, aName, &value, aNotify);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aNotify) {
    nsNodeUtils::AttributeWillChange(this, aNamespaceID, aName, modType);
  }

  
  
  nsAutoScriptBlocker scriptBlocker;

  nsAttrValue attrValue;
  if (!ParseAttribute(aNamespaceID, aName, aValue, attrValue)) {
    attrValue.SetTo(aValue);
  }

  return SetAttrAndNotify(aNamespaceID, aName, aPrefix, oldValue,
                          attrValue, modType, hasListeners, aNotify,
                          kCallAfterSetAttr);
}

nsresult
Element::SetParsedAttr(int32_t aNamespaceID, nsIAtom* aName,
                       nsIAtom* aPrefix, nsAttrValue& aParsedValue,
                       bool aNotify)
{
  

  NS_ENSURE_ARG_POINTER(aName);
  NS_ASSERTION(aNamespaceID != kNameSpaceID_Unknown,
               "Don't call SetAttr with unknown namespace");

  if (!mAttrsAndChildren.CanFitMoreAttrs()) {
    return NS_ERROR_FAILURE;
  }


  uint8_t modType;
  bool hasListeners;
  nsAttrValueOrString value(aParsedValue);
  nsAttrValue oldValue;

  if (OnlyNotifySameValueSet(aNamespaceID, aName, aPrefix, value, aNotify,
                             oldValue, &modType, &hasListeners)) {
    return NS_OK;
  }

  nsresult rv = BeforeSetAttr(aNamespaceID, aName, &value, aNotify);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aNotify) {
    nsNodeUtils::AttributeWillChange(this, aNamespaceID, aName, modType);
  }

  return SetAttrAndNotify(aNamespaceID, aName, aPrefix, oldValue,
                          aParsedValue, modType, hasListeners, aNotify,
                          kCallAfterSetAttr);
}

nsresult
Element::SetAttrAndNotify(int32_t aNamespaceID,
                          nsIAtom* aName,
                          nsIAtom* aPrefix,
                          const nsAttrValue& aOldValue,
                          nsAttrValue& aParsedValue,
                          uint8_t aModType,
                          bool aFireMutation,
                          bool aNotify,
                          bool aCallAfterSetAttr)
{
  nsresult rv;

  nsIDocument* document = GetComposedDoc();
  mozAutoDocUpdate updateBatch(document, UPDATE_CONTENT_MODEL, aNotify);

  nsMutationGuard::DidMutate();

  
  
  nsAttrValue aValueForAfterSetAttr;
  if (aCallAfterSetAttr) {
    aValueForAfterSetAttr.SetTo(aParsedValue);
  }

  bool hadValidDir = false;
  bool hadDirAuto = false;

  if (aNamespaceID == kNameSpaceID_None) {
    if (aName == nsGkAtoms::dir) {
      hadValidDir = HasValidDir() || IsHTMLElement(nsGkAtoms::bdi);
      hadDirAuto = HasDirAuto(); 
    }

    
    
    if (!IsAttributeMapped(aName) ||
        !SetMappedAttribute(document, aName, aParsedValue, &rv)) {
      rv = mAttrsAndChildren.SetAndTakeAttr(aName, aParsedValue);
    }
  }
  else {
    nsRefPtr<mozilla::dom::NodeInfo> ni;
    ni = mNodeInfo->NodeInfoManager()->GetNodeInfo(aName, aPrefix,
                                                   aNamespaceID,
                                                   nsIDOMNode::ATTRIBUTE_NODE);

    rv = mAttrsAndChildren.SetAndTakeAttr(ni, aParsedValue);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  if (document || HasFlag(NODE_FORCE_XBL_BINDINGS)) {
    nsRefPtr<nsXBLBinding> binding = GetXBLBinding();
    if (binding) {
      binding->AttributeChanged(aName, aNamespaceID, false, aNotify);
    }
  }

  UpdateState(aNotify);

  nsIDocument* ownerDoc = OwnerDoc();
  if (ownerDoc && GetCustomElementData()) {
    nsCOMPtr<nsIAtom> oldValueAtom = aOldValue.GetAsAtom();
    nsCOMPtr<nsIAtom> newValueAtom = aValueForAfterSetAttr.GetAsAtom();
    LifecycleCallbackArgs args = {
      nsDependentAtomString(aName),
      aModType == nsIDOMMutationEvent::ADDITION ?
        NullString() : nsDependentAtomString(oldValueAtom),
      nsDependentAtomString(newValueAtom)
    };

    ownerDoc->EnqueueLifecycleCallback(nsIDocument::eAttributeChanged, this, &args);
  }

  if (aCallAfterSetAttr) {
    rv = AfterSetAttr(aNamespaceID, aName, &aValueForAfterSetAttr, aNotify);
    NS_ENSURE_SUCCESS(rv, rv);

    if (aNamespaceID == kNameSpaceID_None && aName == nsGkAtoms::dir) {
      OnSetDirAttr(this, &aValueForAfterSetAttr,
                   hadValidDir, hadDirAuto, aNotify);
    }
  }

  if (aNotify) {
    nsNodeUtils::AttributeChanged(this, aNamespaceID, aName, aModType);
  }

  if (aFireMutation) {
    InternalMutationEvent mutation(true, NS_MUTATION_ATTRMODIFIED);

    nsAutoString ns;
    nsContentUtils::NameSpaceManager()->GetNameSpaceURI(aNamespaceID, ns);
    Attr* attrNode =
      GetAttributeNodeNSInternal(ns, nsDependentAtomString(aName));
    mutation.mRelatedNode = attrNode;

    mutation.mAttrName = aName;
    nsAutoString newValue;
    GetAttr(aNamespaceID, aName, newValue);
    if (!newValue.IsEmpty()) {
      mutation.mNewAttrValue = do_GetAtom(newValue);
    }
    if (!aOldValue.IsEmptyString()) {
      mutation.mPrevAttrValue = aOldValue.GetAsAtom();
    }
    mutation.mAttrChange = aModType;

    mozAutoSubtreeModified subtree(OwnerDoc(), this);
    (new AsyncEventDispatcher(this, mutation))->RunDOMEventWhenSafe();
  }

  return NS_OK;
}

bool
Element::ParseAttribute(int32_t aNamespaceID,
                        nsIAtom* aAttribute,
                        const nsAString& aValue,
                        nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::_class) {
      SetFlags(NODE_MAY_HAVE_CLASS);
      aResult.ParseAtomArray(aValue);
      return true;
    }
    if (aAttribute == nsGkAtoms::id) {
      
      
      RemoveFromIdTable();
      if (aValue.IsEmpty()) {
        ClearHasID();
        return false;
      }
      aResult.ParseAtom(aValue);
      SetHasID();
      AddToIdTable(aResult.GetAtomValue());
      return true;
    }
  }

  return false;
}

bool
Element::SetMappedAttribute(nsIDocument* aDocument,
                            nsIAtom* aName,
                            nsAttrValue& aValue,
                            nsresult* aRetval)
{
  *aRetval = NS_OK;
  return false;
}

EventListenerManager*
Element::GetEventListenerManagerForAttr(nsIAtom* aAttrName,
                                        bool* aDefer)
{
  *aDefer = true;
  return GetOrCreateListenerManager();
}

Element::nsAttrInfo
Element::GetAttrInfo(int32_t aNamespaceID, nsIAtom* aName) const
{
  NS_ASSERTION(nullptr != aName, "must have attribute name");
  NS_ASSERTION(aNamespaceID != kNameSpaceID_Unknown,
               "must have a real namespace ID!");

  int32_t index = mAttrsAndChildren.IndexOfAttr(aName, aNamespaceID);
  if (index >= 0) {
    return nsAttrInfo(mAttrsAndChildren.AttrNameAt(index),
                      mAttrsAndChildren.AttrAt(index));
  }

  return nsAttrInfo(nullptr, nullptr);
}
  

bool
Element::GetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                 nsAString& aResult) const
{
  DOMString str;
  bool haveAttr = GetAttr(aNameSpaceID, aName, str);
  str.ToString(aResult);
  return haveAttr;
}

int32_t
Element::FindAttrValueIn(int32_t aNameSpaceID,
                         nsIAtom* aName,
                         AttrValuesArray* aValues,
                         nsCaseTreatment aCaseSensitive) const
{
  NS_ASSERTION(aName, "Must have attr name");
  NS_ASSERTION(aNameSpaceID != kNameSpaceID_Unknown, "Must have namespace");
  NS_ASSERTION(aValues, "Null value array");
  
  const nsAttrValue* val = mAttrsAndChildren.GetAttr(aName, aNameSpaceID);
  if (val) {
    for (int32_t i = 0; aValues[i]; ++i) {
      if (val->Equals(*aValues[i], aCaseSensitive)) {
        return i;
      }
    }
    return ATTR_VALUE_NO_MATCH;
  }
  return ATTR_MISSING;
}

nsresult
Element::UnsetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                   bool aNotify)
{
  NS_ASSERTION(nullptr != aName, "must have attribute name");

  int32_t index = mAttrsAndChildren.IndexOfAttr(aName, aNameSpaceID);
  if (index < 0) {
    return NS_OK;
  }

  nsresult rv = BeforeSetAttr(aNameSpaceID, aName, nullptr, aNotify);
  NS_ENSURE_SUCCESS(rv, rv);

  nsIDocument *document = GetComposedDoc();
  mozAutoDocUpdate updateBatch(document, UPDATE_CONTENT_MODEL, aNotify);

  if (aNotify) {
    nsNodeUtils::AttributeWillChange(this, aNameSpaceID, aName,
                                     nsIDOMMutationEvent::REMOVAL);
  }

  bool hasMutationListeners = aNotify &&
    nsContentUtils::HasMutationListeners(this,
                                         NS_EVENT_BITS_MUTATION_ATTRMODIFIED,
                                         this);

  
  nsRefPtr<Attr> attrNode;
  if (hasMutationListeners) {
    nsAutoString ns;
    nsContentUtils::NameSpaceManager()->GetNameSpaceURI(aNameSpaceID, ns);
    attrNode = GetAttributeNodeNSInternal(ns, nsDependentAtomString(aName));
  }

  
  nsDOMSlots *slots = GetExistingDOMSlots();
  if (slots && slots->mAttributeMap) {
    slots->mAttributeMap->DropAttribute(aNameSpaceID, aName);
  }

  
  
  nsMutationGuard::DidMutate();

  if (aName == nsGkAtoms::id && aNameSpaceID == kNameSpaceID_None) {
    
    RemoveFromIdTable();
    ClearHasID();
  }

  bool hadValidDir = false;
  bool hadDirAuto = false;

  if (aNameSpaceID == kNameSpaceID_None && aName == nsGkAtoms::dir) {
    hadValidDir = HasValidDir() || IsHTMLElement(nsGkAtoms::bdi);
    hadDirAuto = HasDirAuto(); 
  }

  nsAttrValue oldValue;
  rv = mAttrsAndChildren.RemoveAttrAt(index, oldValue);
  NS_ENSURE_SUCCESS(rv, rv);

  if (document || HasFlag(NODE_FORCE_XBL_BINDINGS)) {
    nsRefPtr<nsXBLBinding> binding = GetXBLBinding();
    if (binding) {
      binding->AttributeChanged(aName, aNameSpaceID, true, aNotify);
    }
  }

  UpdateState(aNotify);

  nsIDocument* ownerDoc = OwnerDoc();
  if (ownerDoc && GetCustomElementData()) {
    nsCOMPtr<nsIAtom> oldValueAtom = oldValue.GetAsAtom();
    LifecycleCallbackArgs args = {
      nsDependentAtomString(aName),
      nsDependentAtomString(oldValueAtom),
      NullString()
    };

    ownerDoc->EnqueueLifecycleCallback(nsIDocument::eAttributeChanged, this, &args);
  }

  if (aNotify) {
    nsNodeUtils::AttributeChanged(this, aNameSpaceID, aName,
                                  nsIDOMMutationEvent::REMOVAL);
  }

  rv = AfterSetAttr(aNameSpaceID, aName, nullptr, aNotify);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aNameSpaceID == kNameSpaceID_None && aName == nsGkAtoms::dir) {
    OnSetDirAttr(this, nullptr, hadValidDir, hadDirAuto, aNotify);
  }

  if (hasMutationListeners) {
    InternalMutationEvent mutation(true, NS_MUTATION_ATTRMODIFIED);

    mutation.mRelatedNode = attrNode;
    mutation.mAttrName = aName;

    nsAutoString value;
    oldValue.ToString(value);
    if (!value.IsEmpty())
      mutation.mPrevAttrValue = do_GetAtom(value);
    mutation.mAttrChange = nsIDOMMutationEvent::REMOVAL;

    mozAutoSubtreeModified subtree(OwnerDoc(), this);
    (new AsyncEventDispatcher(this, mutation))->RunDOMEventWhenSafe();
  }

  return NS_OK;
}

const nsAttrName*
Element::GetAttrNameAt(uint32_t aIndex) const
{
  return mAttrsAndChildren.GetSafeAttrNameAt(aIndex);
}

uint32_t
Element::GetAttrCount() const
{
  return mAttrsAndChildren.AttrCount();
}

void
Element::DescribeAttribute(uint32_t index, nsAString& aOutDescription) const
{
  
  mAttrsAndChildren.AttrNameAt(index)->GetQualifiedName(aOutDescription);

  
  aOutDescription.AppendLiteral("=\"");
  nsAutoString value;
  mAttrsAndChildren.AttrAt(index)->ToString(value);
  for (int i = value.Length(); i >= 0; --i) {
    if (value[i] == char16_t('"'))
      value.Insert(char16_t('\\'), uint32_t(i));
  }
  aOutDescription.Append(value);
  aOutDescription.Append('"');
}

#ifdef DEBUG
void
Element::ListAttributes(FILE* out) const
{
  uint32_t index, count = mAttrsAndChildren.AttrCount();
  for (index = 0; index < count; index++) {
    nsAutoString attributeDescription;
    DescribeAttribute(index, attributeDescription);

    fputs(" ", out);
    fputs(NS_LossyConvertUTF16toASCII(attributeDescription).get(), out);
  }
}

void
Element::List(FILE* out, int32_t aIndent,
              const nsCString& aPrefix) const
{
  int32_t indent;
  for (indent = aIndent; --indent >= 0; ) fputs("  ", out);

  fputs(aPrefix.get(), out);

  fputs(NS_LossyConvertUTF16toASCII(mNodeInfo->QualifiedName()).get(), out);

  fprintf(out, "@%p", (void *)this);

  ListAttributes(out);

  fprintf(out, " state=[%llx]",
          static_cast<unsigned long long>(State().GetInternalValue()));
  fprintf(out, " flags=[%08x]", static_cast<unsigned int>(GetFlags()));
  if (IsCommonAncestorForRangeInSelection()) {
    nsRange::RangeHashTable* ranges =
      static_cast<nsRange::RangeHashTable*>(GetProperty(nsGkAtoms::range));
    fprintf(out, " ranges:%d", ranges ? ranges->Count() : 0);
  }
  fprintf(out, " primaryframe=%p", static_cast<void*>(GetPrimaryFrame()));
  fprintf(out, " refcount=%" PRIuPTR "<", mRefCnt.get());

  nsIContent* child = GetFirstChild();
  if (child) {
    fputs("\n", out);
    
    for (; child; child = child->GetNextSibling()) {
      child->List(out, aIndent + 1);
    }

    for (indent = aIndent; --indent >= 0; ) fputs("  ", out);
  }

  fputs(">\n", out);
  
  Element* nonConstThis = const_cast<Element*>(this);

  
  nsIDocument *document = OwnerDoc();

  

  nsBindingManager* bindingManager = document->BindingManager();
  nsCOMPtr<nsIDOMNodeList> anonymousChildren;
  bindingManager->GetAnonymousNodesFor(nonConstThis,
                                       getter_AddRefs(anonymousChildren));

  if (anonymousChildren) {
    uint32_t length = 0;
    anonymousChildren->GetLength(&length);

    for (indent = aIndent; --indent >= 0; ) fputs("  ", out);
    fputs("anonymous-children<\n", out);

    for (uint32_t i = 0; i < length; ++i) {
      nsCOMPtr<nsIDOMNode> node;
      anonymousChildren->Item(i, getter_AddRefs(node));
      nsCOMPtr<nsIContent> child = do_QueryInterface(node);
      child->List(out, aIndent + 1);
    }

    for (indent = aIndent; --indent >= 0; ) fputs("  ", out);
    fputs(">\n", out);

    bool outHeader = false;
    ExplicitChildIterator iter(nonConstThis);
    for (nsIContent* child = iter.GetNextChild(); child; child = iter.GetNextChild()) {
      if (!outHeader) {
        outHeader = true;

        for (indent = aIndent; --indent >= 0; ) fputs("  ", out);
        fputs("content-list<\n", out);
      }

      child->List(out, aIndent + 1);
    }

    if (outHeader) {
      for (indent = aIndent; --indent >= 0; ) fputs("  ", out);
      fputs(">\n", out);
    }
  }
}

void
Element::DumpContent(FILE* out, int32_t aIndent,
                     bool aDumpAll) const
{
  int32_t indent;
  for (indent = aIndent; --indent >= 0; ) fputs("  ", out);

  const nsString& buf = mNodeInfo->QualifiedName();
  fputs("<", out);
  fputs(NS_LossyConvertUTF16toASCII(buf).get(), out);

  if(aDumpAll) ListAttributes(out);

  fputs(">", out);

  if(aIndent) fputs("\n", out);

  for (nsIContent* child = GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    int32_t indent = aIndent ? aIndent + 1 : 0;
    child->DumpContent(out, indent, aDumpAll);
  }
  for (indent = aIndent; --indent >= 0; ) fputs("  ", out);
  fputs("</", out);
  fputs(NS_LossyConvertUTF16toASCII(buf).get(), out);
  fputs(">", out);

  if(aIndent) fputs("\n", out);
}
#endif

void
Element::Describe(nsAString& aOutDescription) const
{
  aOutDescription.Append(mNodeInfo->QualifiedName());
  aOutDescription.AppendPrintf("@%p", (void *)this);

  uint32_t index, count = mAttrsAndChildren.AttrCount();
  for (index = 0; index < count; index++) {
    aOutDescription.Append(' ');
    nsAutoString attributeDescription;
    DescribeAttribute(index, attributeDescription);
    aOutDescription.Append(attributeDescription);
  }
}

bool
Element::CheckHandleEventForLinksPrecondition(EventChainVisitor& aVisitor,
                                              nsIURI** aURI) const
{
  if (aVisitor.mEventStatus == nsEventStatus_eConsumeNoDefault ||
      (!aVisitor.mEvent->mFlags.mIsTrusted &&
       (aVisitor.mEvent->message != NS_MOUSE_CLICK) &&
       (aVisitor.mEvent->message != NS_KEY_PRESS) &&
       (aVisitor.mEvent->message != NS_UI_ACTIVATE)) ||
      !aVisitor.mPresContext ||
      aVisitor.mEvent->mFlags.mMultipleActionsPrevented) {
    return false;
  }

  
  return IsLink(aURI);
}

nsresult
Element::PreHandleEventForLinks(EventChainPreVisitor& aVisitor)
{
  
  
  switch (aVisitor.mEvent->message) {
  case NS_MOUSE_OVER:
  case NS_FOCUS_CONTENT:
  case NS_MOUSE_OUT:
  case NS_BLUR_CONTENT:
    break;
  default:
    return NS_OK;
  }

  
  nsCOMPtr<nsIURI> absURI;
  if (!CheckHandleEventForLinksPrecondition(aVisitor, getter_AddRefs(absURI))) {
    return NS_OK;
  }

  nsresult rv = NS_OK;

  
  
  switch (aVisitor.mEvent->message) {
  
  case NS_MOUSE_OVER:
    aVisitor.mEventStatus = nsEventStatus_eConsumeNoDefault;
    
  case NS_FOCUS_CONTENT: {
    InternalFocusEvent* focusEvent = aVisitor.mEvent->AsFocusEvent();
    if (!focusEvent || !focusEvent->isRefocus) {
      nsAutoString target;
      GetLinkTarget(target);
      nsContentUtils::TriggerLink(this, aVisitor.mPresContext, absURI, target,
                                  false, true, true);
      
      aVisitor.mEvent->mFlags.mMultipleActionsPrevented = true;
    }
    break;
  }
  case NS_MOUSE_OUT:
    aVisitor.mEventStatus = nsEventStatus_eConsumeNoDefault;
    
  case NS_BLUR_CONTENT:
    rv = LeaveLink(aVisitor.mPresContext);
    if (NS_SUCCEEDED(rv)) {
      aVisitor.mEvent->mFlags.mMultipleActionsPrevented = true;
    }
    break;

  default:
    
    NS_NOTREACHED("switch statements not in sync");
    return NS_ERROR_UNEXPECTED;
  }

  return rv;
}

nsresult
Element::PostHandleEventForLinks(EventChainPostVisitor& aVisitor)
{
  
  
  switch (aVisitor.mEvent->message) {
  case NS_MOUSE_BUTTON_DOWN:
  case NS_MOUSE_CLICK:
  case NS_UI_ACTIVATE:
  case NS_KEY_PRESS:
    break;
  default:
    return NS_OK;
  }

  
  nsCOMPtr<nsIURI> absURI;
  if (!CheckHandleEventForLinksPrecondition(aVisitor, getter_AddRefs(absURI))) {
    return NS_OK;
  }

  nsresult rv = NS_OK;

  switch (aVisitor.mEvent->message) {
  case NS_MOUSE_BUTTON_DOWN:
    {
      if (aVisitor.mEvent->AsMouseEvent()->button ==
            WidgetMouseEvent::eLeftButton) {
        
        nsILinkHandler *handler = aVisitor.mPresContext->GetLinkHandler();
        nsIDocument *document = GetComposedDoc();
        if (handler && document) {
          nsIFocusManager* fm = nsFocusManager::GetFocusManager();
          if (fm) {
            aVisitor.mEvent->mFlags.mMultipleActionsPrevented = true;
            nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(this);
            fm->SetFocus(elem, nsIFocusManager::FLAG_BYMOUSE |
                               nsIFocusManager::FLAG_NOSCROLL);
          }

          EventStateManager::SetActiveManager(
            aVisitor.mPresContext->EventStateManager(), this);
        }
      }
    }
    break;

  case NS_MOUSE_CLICK: {
    WidgetMouseEvent* mouseEvent = aVisitor.mEvent->AsMouseEvent();
    if (mouseEvent->IsLeftClickEvent()) {
      if (mouseEvent->IsControl() || mouseEvent->IsMeta() ||
          mouseEvent->IsAlt() ||mouseEvent->IsShift()) {
        break;
      }

      
      nsCOMPtr<nsIPresShell> shell = aVisitor.mPresContext->GetPresShell();
      if (shell) {
        
        nsEventStatus status = nsEventStatus_eIgnore;
        InternalUIEvent actEvent(mouseEvent->mFlags.mIsTrusted, NS_UI_ACTIVATE);
        actEvent.detail = 1;

        rv = shell->HandleDOMEventWithTarget(this, &actEvent, &status);
        if (NS_SUCCEEDED(rv)) {
          aVisitor.mEventStatus = nsEventStatus_eConsumeNoDefault;
        }
      }
    }
    break;
  }
  case NS_UI_ACTIVATE:
    {
      if (aVisitor.mEvent->originalTarget == this) {
        nsAutoString target;
        GetLinkTarget(target);
        nsContentUtils::TriggerLink(this, aVisitor.mPresContext, absURI, target,
                                    true, true,
                                    aVisitor.mEvent->mFlags.mIsTrusted);
        aVisitor.mEventStatus = nsEventStatus_eConsumeNoDefault;
      }
    }
    break;

  case NS_KEY_PRESS:
    {
      WidgetKeyboardEvent* keyEvent = aVisitor.mEvent->AsKeyboardEvent();
      if (keyEvent && keyEvent->keyCode == NS_VK_RETURN) {
        nsEventStatus status = nsEventStatus_eIgnore;
        rv = DispatchClickEvent(aVisitor.mPresContext, keyEvent, this,
                                false, nullptr, &status);
        if (NS_SUCCEEDED(rv)) {
          aVisitor.mEventStatus = nsEventStatus_eConsumeNoDefault;
        }
      }
    }
    break;

  default:
    
    NS_NOTREACHED("switch statements not in sync");
    return NS_ERROR_UNEXPECTED;
  }

  return rv;
}

void
Element::GetLinkTarget(nsAString& aTarget)
{
  aTarget.Truncate();
}

static void
nsDOMSettableTokenListPropertyDestructor(void *aObject, nsIAtom *aProperty,
                                         void *aPropertyValue, void *aData)
{
  nsDOMSettableTokenList* list =
    static_cast<nsDOMSettableTokenList*>(aPropertyValue);
  NS_RELEASE(list);
}

static nsIAtom** sPropertiesToTraverseAndUnlink[] =
  {
    &nsGkAtoms::microdataProperties,
    &nsGkAtoms::itemtype,
    &nsGkAtoms::itemref,
    &nsGkAtoms::itemprop,
    &nsGkAtoms::sandbox,
    &nsGkAtoms::sizes,
    nullptr
  };


nsIAtom***
Element::HTMLSVGPropertiesToTraverseAndUnlink()
{
  return sPropertiesToTraverseAndUnlink;
}

nsDOMSettableTokenList*
Element::GetTokenList(nsIAtom* aAtom)
{
#ifdef DEBUG
  nsIAtom*** props =
    HTMLSVGPropertiesToTraverseAndUnlink();
  bool found = false;
  for (uint32_t i = 0; props[i]; ++i) {
    if (*props[i] == aAtom) {
      found = true;
      break;
    }
  }
  MOZ_ASSERT(found, "Trying to use an unknown tokenlist!");
#endif

  nsDOMSettableTokenList* list = nullptr;
  if (HasProperties()) {
    list = static_cast<nsDOMSettableTokenList*>(GetProperty(aAtom));
  }
  if (!list) {
    list = new nsDOMSettableTokenList(this, aAtom);
    NS_ADDREF(list);
    SetProperty(aAtom, list, nsDOMSettableTokenListPropertyDestructor);
  }
  return list;
}

void
Element::GetTokenList(nsIAtom* aAtom, nsIVariant** aResult)
{
  nsISupports* itemType = GetTokenList(aAtom);
  nsCOMPtr<nsIWritableVariant> out = new nsVariant();
  out->SetAsInterface(NS_GET_IID(nsISupports), itemType);
  out.forget(aResult);
}

nsresult
Element::SetTokenList(nsIAtom* aAtom, nsIVariant* aValue)
{
  nsDOMSettableTokenList* itemType = GetTokenList(aAtom);
  nsAutoString string;
  aValue->GetAsAString(string);
  ErrorResult rv;
  itemType->SetValue(string, rv);
  return rv.StealNSResult();
}

Element*
Element::Closest(const nsAString& aSelector, ErrorResult& aResult)
{
  nsCSSSelectorList* selectorList = ParseSelectorList(aSelector, aResult);
  if (!selectorList) {
    
    
    return nullptr;
  }
  OwnerDoc()->FlushPendingLinkUpdates();
  TreeMatchContext matchingContext(false,
                                   nsRuleWalker::eRelevantLinkUnvisited,
                                   OwnerDoc(),
                                   TreeMatchContext::eNeverMatchVisited);
  matchingContext.SetHasSpecifiedScope();
  matchingContext.AddScopeElement(this);
  for (nsINode* node = this; node; node = node->GetParentNode()) {
    if (node->IsElement() &&
        nsCSSRuleProcessor::SelectorListMatches(node->AsElement(),
                                                matchingContext,
                                                selectorList)) {
      return node->AsElement();
    }
  }
  return nullptr;
}

bool
Element::Matches(const nsAString& aSelector, ErrorResult& aError)
{
  nsCSSSelectorList* selectorList = ParseSelectorList(aSelector, aError);
  if (!selectorList) {
    
    
    return false;
  }

  OwnerDoc()->FlushPendingLinkUpdates();
  TreeMatchContext matchingContext(false,
                                   nsRuleWalker::eRelevantLinkUnvisited,
                                   OwnerDoc(),
                                   TreeMatchContext::eNeverMatchVisited);
  matchingContext.SetHasSpecifiedScope();
  matchingContext.AddScopeElement(this);
  return nsCSSRuleProcessor::SelectorListMatches(this, matchingContext,
                                                 selectorList);
}

static const nsAttrValue::EnumTable kCORSAttributeTable[] = {
  
  
  { "anonymous",       CORS_ANONYMOUS       },
  { "use-credentials", CORS_USE_CREDENTIALS },
  { 0 }
};

 void
Element::ParseCORSValue(const nsAString& aValue,
                        nsAttrValue& aResult)
{
  DebugOnly<bool> success =
    aResult.ParseEnumValue(aValue, kCORSAttributeTable, false,
                           
                           
                           &kCORSAttributeTable[0]);
  MOZ_ASSERT(success);
}

 CORSMode
Element::StringToCORSMode(const nsAString& aValue)
{
  if (aValue.IsVoid()) {
    return CORS_NONE;
  }

  nsAttrValue val;
  Element::ParseCORSValue(aValue, val);
  return CORSMode(val.GetEnumValue());
}

 CORSMode
Element::AttrValueToCORSMode(const nsAttrValue* aValue)
{
  if (!aValue) {
    return CORS_NONE;
  }

  return CORSMode(aValue->GetEnumValue());
}

static const char*
GetFullScreenError(nsIDocument* aDoc)
{
  nsCOMPtr<nsPIDOMWindow> win = aDoc->GetWindow();
  if (aDoc->NodePrincipal()->GetAppStatus() >= nsIPrincipal::APP_STATUS_INSTALLED) {
    
    
    
    
    return nullptr;
  }

  if (!nsContentUtils::IsRequestFullScreenAllowed()) {
    return "FullScreenDeniedNotInputDriven";
  }

  return nullptr;
}

void
Element::MozRequestFullScreen(JSContext* aCx, JS::Handle<JS::Value> aOptions,
                              ErrorResult& aError)
{
  MOZ_ASSERT_IF(!aCx, aOptions.isNullOrUndefined());
  
  
  
  
  
  
  
  const char* error = GetFullScreenError(OwnerDoc());
  if (error) {
    nsContentUtils::ReportToConsole(nsIScriptError::warningFlag,
                                    NS_LITERAL_CSTRING("DOM"), OwnerDoc(),
                                    nsContentUtils::eDOM_PROPERTIES,
                                    error);
    nsRefPtr<AsyncEventDispatcher> asyncDispatcher =
      new AsyncEventDispatcher(OwnerDoc(),
                               NS_LITERAL_STRING("mozfullscreenerror"),
                               true,
                               false);
    asyncDispatcher->PostDOMEvent();
    return;
  }

  auto request = MakeUnique<FullscreenRequest>(this);
  request->mIsCallerChrome = nsContentUtils::IsCallerChrome();

  RequestFullscreenOptions fsOptions;
  
  
  
  if (aCx && IsConvertibleToDictionary(aCx, aOptions)) {
    if (!fsOptions.Init(aCx, aOptions)) {
      aError.Throw(NS_ERROR_FAILURE);
      return;
    }

    if (fsOptions.mVrDisplay) {
      request->mVRHMDDevice = fsOptions.mVrDisplay->GetHMD();
    }
  }

  OwnerDoc()->AsyncRequestFullScreen(Move(request));
}

void
Element::MozRequestPointerLock()
{
  OwnerDoc()->RequestPointerLock(this);
}

void
Element::GetAnimations(nsTArray<nsRefPtr<Animation>>& aAnimations)
{
  nsIDocument* doc = GetComposedDoc();
  if (doc) {
    doc->FlushPendingNotifications(Flush_Style);
  }

  nsIAtom* properties[] = { nsGkAtoms::transitionsProperty,
                            nsGkAtoms::animationsProperty };
  for (size_t propIdx = 0; propIdx < MOZ_ARRAY_LENGTH(properties);
       propIdx++) {
    AnimationCollection* collection =
      static_cast<AnimationCollection*>(
        GetProperty(properties[propIdx]));
    if (!collection) {
      continue;
    }
    for (size_t animIdx = 0;
         animIdx < collection->mAnimations.Length();
         animIdx++) {
      Animation* anim = collection->mAnimations[animIdx];
      if (anim->IsRelevant()) {
        aAnimations.AppendElement(anim);
      }
    }
  }

  aAnimations.Sort(AnimationPtrComparator<nsRefPtr<Animation>>());
}

NS_IMETHODIMP
Element::GetInnerHTML(nsAString& aInnerHTML)
{
  GetMarkup(false, aInnerHTML);
  return NS_OK;
}

void
Element::SetInnerHTML(const nsAString& aInnerHTML, ErrorResult& aError)
{
  SetInnerHTMLInternal(aInnerHTML, aError);
}

void
Element::GetOuterHTML(nsAString& aOuterHTML)
{
  GetMarkup(true, aOuterHTML);
}

void
Element::SetOuterHTML(const nsAString& aOuterHTML, ErrorResult& aError)
{
  nsCOMPtr<nsINode> parent = GetParentNode();
  if (!parent) {
    return;
  }

  if (parent->NodeType() == nsIDOMNode::DOCUMENT_NODE) {
    aError.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  if (OwnerDoc()->IsHTMLDocument()) {
    nsIAtom* localName;
    int32_t namespaceID;
    if (parent->IsElement()) {
      localName = parent->NodeInfo()->NameAtom();
      namespaceID = parent->NodeInfo()->NamespaceID();
    } else {
      NS_ASSERTION(parent->NodeType() == nsIDOMNode::DOCUMENT_FRAGMENT_NODE,
        "How come the parent isn't a document, a fragment or an element?");
      localName = nsGkAtoms::body;
      namespaceID = kNameSpaceID_XHTML;
    }
    nsRefPtr<DocumentFragment> fragment =
      new DocumentFragment(OwnerDoc()->NodeInfoManager());
    nsContentUtils::ParseFragmentHTML(aOuterHTML,
                                      fragment,
                                      localName,
                                      namespaceID,
                                      OwnerDoc()->GetCompatibilityMode() ==
                                        eCompatibility_NavQuirks,
                                      true);
    parent->ReplaceChild(*fragment, *this, aError);
    return;
  }

  nsCOMPtr<nsINode> context;
  if (parent->IsElement()) {
    context = parent;
  } else {
    NS_ASSERTION(parent->NodeType() == nsIDOMNode::DOCUMENT_FRAGMENT_NODE,
      "How come the parent isn't a document, a fragment or an element?");
    nsRefPtr<mozilla::dom::NodeInfo> info =
      OwnerDoc()->NodeInfoManager()->GetNodeInfo(nsGkAtoms::body,
                                                 nullptr,
                                                 kNameSpaceID_XHTML,
                                                 nsIDOMNode::ELEMENT_NODE);
    context = NS_NewHTMLBodyElement(info.forget(), FROM_PARSER_FRAGMENT);
  }

  nsCOMPtr<nsIDOMDocumentFragment> df;
  aError = nsContentUtils::CreateContextualFragment(context,
                                                    aOuterHTML,
                                                    true,
                                                    getter_AddRefs(df));
  if (aError.Failed()) {
    return;
  }
  nsCOMPtr<nsINode> fragment = do_QueryInterface(df);
  parent->ReplaceChild(*fragment, *this, aError);
}

enum nsAdjacentPosition {
  eBeforeBegin,
  eAfterBegin,
  eBeforeEnd,
  eAfterEnd
};

void
Element::InsertAdjacentHTML(const nsAString& aPosition, const nsAString& aText,
                            ErrorResult& aError)
{
  nsAdjacentPosition position;
  if (aPosition.LowerCaseEqualsLiteral("beforebegin")) {
    position = eBeforeBegin;
  } else if (aPosition.LowerCaseEqualsLiteral("afterbegin")) {
    position = eAfterBegin;
  } else if (aPosition.LowerCaseEqualsLiteral("beforeend")) {
    position = eBeforeEnd;
  } else if (aPosition.LowerCaseEqualsLiteral("afterend")) {
    position = eAfterEnd;
  } else {
    aError.Throw(NS_ERROR_DOM_SYNTAX_ERR);
    return;
  }

  nsCOMPtr<nsIContent> destination;
  if (position == eBeforeBegin || position == eAfterEnd) {
    destination = GetParent();
    if (!destination) {
      aError.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
      return;
    }
  } else {
    destination = this;
  }

  nsIDocument* doc = OwnerDoc();

  
  mozAutoDocUpdate updateBatch(doc, UPDATE_CONTENT_MODEL, true);
  nsAutoScriptLoaderDisabler sld(doc);
  
  
  mozAutoSubtreeModified subtree(doc, nullptr);

  
  if (doc->IsHTMLDocument() && !OwnerDoc()->MayHaveDOMMutationObservers() &&
      (position == eBeforeEnd ||
       (position == eAfterEnd && !GetNextSibling()) ||
       (position == eAfterBegin && !GetFirstChild()))) {
    int32_t oldChildCount = destination->GetChildCount();
    int32_t contextNs = destination->GetNameSpaceID();
    nsIAtom* contextLocal = destination->NodeInfo()->NameAtom();
    if (contextLocal == nsGkAtoms::html && contextNs == kNameSpaceID_XHTML) {
      
      
      
      contextLocal = nsGkAtoms::body;
    }
    aError = nsContentUtils::ParseFragmentHTML(aText,
                                               destination,
                                               contextLocal,
                                               contextNs,
                                               doc->GetCompatibilityMode() ==
                                                 eCompatibility_NavQuirks,
                                               true);
    
    nsContentUtils::FireMutationEventsForDirectParsing(doc, destination,
                                                       oldChildCount);
    return;
  }

  
  nsCOMPtr<nsIDOMDocumentFragment> df;
  aError = nsContentUtils::CreateContextualFragment(destination,
                                                    aText,
                                                    true,
                                                    getter_AddRefs(df));
  if (aError.Failed()) {
    return;
  }

  nsCOMPtr<nsINode> fragment = do_QueryInterface(df);

  
  
  
  nsAutoScriptBlockerSuppressNodeRemoved scriptBlocker;

  nsAutoMutationBatch mb(destination, true, false);
  switch (position) {
    case eBeforeBegin:
      destination->InsertBefore(*fragment, this, aError);
      break;
    case eAfterBegin:
      static_cast<nsINode*>(this)->InsertBefore(*fragment, GetFirstChild(),
                                                aError);
      break;
    case eBeforeEnd:
      static_cast<nsINode*>(this)->AppendChild(*fragment, aError);
      break;
    case eAfterEnd:
      destination->InsertBefore(*fragment, GetNextSibling(), aError);
      break;
  }
}

nsIEditor*
Element::GetEditorInternal()
{
  nsCOMPtr<nsITextControlElement> textCtrl = do_QueryInterface(this);
  return textCtrl ? textCtrl->GetTextEditor() : nullptr;
}

nsresult
Element::SetBoolAttr(nsIAtom* aAttr, bool aValue)
{
  if (aValue) {
    return SetAttr(kNameSpaceID_None, aAttr, EmptyString(), true);
  }

  return UnsetAttr(kNameSpaceID_None, aAttr, true);
}

void
Element::GetEnumAttr(nsIAtom* aAttr,
                     const char* aDefault,
                     nsAString& aResult) const
{
  GetEnumAttr(aAttr, aDefault, aDefault, aResult);
}

void
Element::GetEnumAttr(nsIAtom* aAttr,
                     const char* aDefaultMissing,
                     const char* aDefaultInvalid,
                     nsAString& aResult) const
{
  const nsAttrValue* attrVal = mAttrsAndChildren.GetAttr(aAttr);

  aResult.Truncate();

  if (!attrVal) {
    if (aDefaultMissing) {
      AppendASCIItoUTF16(nsDependentCString(aDefaultMissing), aResult);
    } else {
      SetDOMStringToNull(aResult);
    }
  } else {
    if (attrVal->Type() == nsAttrValue::eEnum) {
      attrVal->GetEnumString(aResult, true);
    } else if (aDefaultInvalid) {
      AppendASCIItoUTF16(nsDependentCString(aDefaultInvalid), aResult);
    }
  }
}

void
Element::SetOrRemoveNullableStringAttr(nsIAtom* aName, const nsAString& aValue,
                                       ErrorResult& aError)
{
  if (DOMStringIsNull(aValue)) {
    UnsetAttr(aName, aError);
  } else {
    SetAttr(aName, aValue, aError);
  }
}

Directionality
Element::GetComputedDirectionality() const
{
  nsIFrame* frame = GetPrimaryFrame();
  if (frame) {
    return frame->StyleVisibility()->mDirection == NS_STYLE_DIRECTION_LTR
             ? eDir_LTR : eDir_RTL;
  }

  return GetDirectionality();
}

float
Element::FontSizeInflation()
{
  nsIFrame* frame = GetPrimaryFrame();
  if (!frame) {
    return -1.0;
  }

  if (nsLayoutUtils::FontSizeInflationEnabled(frame->PresContext())) {
    return nsLayoutUtils::FontSizeInflationFor(frame);
  }

  return 1.0;
}

net::ReferrerPolicy
Element::GetReferrerPolicy()
{
  if (Preferences::GetBool("network.http.enablePerElementReferrer", false) &&
      IsHTMLElement()) {
    const nsAttrValue* referrerValue = GetParsedAttr(nsGkAtoms::referrer);
    if (referrerValue && referrerValue->Type() == nsAttrValue::eEnum) {
      return net::ReferrerPolicy(referrerValue->GetEnumValue());
    }
  }
  return net::RP_Unset;
}