





#include "nsSVGEffects.h"


#include "nsCSSFrameConstructor.h"
#include "nsISupportsImpl.h"
#include "nsSVGClipPathFrame.h"
#include "nsSVGPaintServerFrame.h"
#include "nsSVGPathGeometryElement.h"
#include "nsSVGFilterFrame.h"
#include "nsSVGMaskFrame.h"
#include "nsIReflowCallback.h"
#include "RestyleManager.h"

using namespace mozilla;
using namespace mozilla::dom;


NS_IMPL_ISUPPORTS(nsSVGRenderingObserver, nsIMutationObserver)

void
nsSVGRenderingObserver::StartListening()
{
  Element* target = GetTarget();
  if (target) {
    target->AddMutationObserver(this);
  }
}

void
nsSVGRenderingObserver::StopListening()
{
  Element* target = GetTarget();

  if (target) {
    target->RemoveMutationObserver(this);
    if (mInObserverList) {
      nsSVGEffects::RemoveRenderingObserver(target, this);
      mInObserverList = false;
    }
  }
  NS_ASSERTION(!mInObserverList, "still in an observer list?");
}

static nsSVGRenderingObserverList *
GetObserverList(Element *aElement)
{
  return static_cast<nsSVGRenderingObserverList*>
    (aElement->GetProperty(nsGkAtoms::renderingobserverlist));
}

Element*
nsSVGRenderingObserver::GetReferencedElement()
{
  Element* target = GetTarget();
#ifdef DEBUG
  if (target) {
    nsSVGRenderingObserverList *observerList = GetObserverList(target);
    bool inObserverList = observerList && observerList->Contains(this);
    NS_ASSERTION(inObserverList == mInObserverList, "failed to track whether we're in our referenced element's observer list!");
  } else {
    NS_ASSERTION(!mInObserverList, "In whose observer list are we, then?");
  }
#endif
  if (target && !mInObserverList) {
    nsSVGEffects::AddRenderingObserver(target, this);
    mInObserverList = true;
  }
  return target;
}

nsIFrame*
nsSVGRenderingObserver::GetReferencedFrame()
{
  Element* referencedElement = GetReferencedElement();
  return referencedElement ? referencedElement->GetPrimaryFrame() : nullptr;
}

nsIFrame*
nsSVGRenderingObserver::GetReferencedFrame(nsIAtom* aFrameType, bool* aOK)
{
  nsIFrame* frame = GetReferencedFrame();
  if (frame) {
    if (frame->GetType() == aFrameType)
      return frame;
    if (aOK) {
      *aOK = false;
    }
  }
  return nullptr;
}

void
nsSVGRenderingObserver::InvalidateViaReferencedElement()
{
  mInObserverList = false;
  DoUpdate();
}

void
nsSVGRenderingObserver::NotifyEvictedFromRenderingObserverList()
{
  mInObserverList = false; 
  StopListening();            
}

void
nsSVGRenderingObserver::AttributeChanged(nsIDocument* aDocument,
                                         dom::Element* aElement,
                                         int32_t aNameSpaceID,
                                         nsIAtom* aAttribute,
                                         int32_t aModType)
{
  
  
  
  
  
  
  
  
  
  
  
  

  DoUpdate();
}

void
nsSVGRenderingObserver::ContentAppended(nsIDocument *aDocument,
                                        nsIContent *aContainer,
                                        nsIContent *aFirstNewContent,
                                        int32_t )
{
  DoUpdate();
}

void
nsSVGRenderingObserver::ContentInserted(nsIDocument *aDocument,
                                        nsIContent *aContainer,
                                        nsIContent *aChild,
                                        int32_t )
{
  DoUpdate();
}

void
nsSVGRenderingObserver::ContentRemoved(nsIDocument *aDocument,
                                       nsIContent *aContainer,
                                       nsIContent *aChild,
                                       int32_t aIndexInContainer,
                                       nsIContent *aPreviousSibling)
{
  DoUpdate();
}


















nsSVGIDRenderingObserver::nsSVGIDRenderingObserver(nsIURI *aURI,
                                                   nsIFrame *aFrame,
                                                   bool aReferenceImage)
  : mElement(MOZ_THIS_IN_INITIALIZER_LIST()), mFrame(aFrame),
    mFramePresShell(aFrame->PresContext()->PresShell())
{
  
  mElement.Reset(aFrame->GetContent(), aURI, true, aReferenceImage);
  StartListening();
}

nsSVGIDRenderingObserver::~nsSVGIDRenderingObserver()
{
  StopListening();
}

void
nsSVGIDRenderingObserver::DoUpdate()
{
  if (mFramePresShell->IsDestroying()) {
    
    mFrame = nullptr;
    return;
  }
  if (mElement.get() && mInObserverList) {
    nsSVGEffects::RemoveRenderingObserver(mElement.get(), this);
    mInObserverList = false;
  }
  if (mFrame && mFrame->IsFrameOfType(nsIFrame::eSVG)) {
    
    
    nsSVGEffects::InvalidateRenderingObservers(mFrame);
  }
}

NS_IMPL_ISUPPORTS_INHERITED(nsSVGFilterReference,
                            nsSVGIDRenderingObserver,
                            nsISVGFilterReference);

nsSVGFilterFrame *
nsSVGFilterReference::GetFilterFrame()
{
  return static_cast<nsSVGFilterFrame *>
    (GetReferencedFrame(nsGkAtoms::svgFilterFrame, nullptr));
}

void
nsSVGFilterReference::DoUpdate()
{
  nsSVGIDRenderingObserver::DoUpdate();
  if (!mFrame)
    return;

  
  nsChangeHint changeHint =
    nsChangeHint(nsChangeHint_RepaintFrame);

  
  if (!(mFrame->GetStateBits() & NS_FRAME_IN_REFLOW)) {
    NS_UpdateHint(changeHint, nsChangeHint_UpdateOverflow);
  }
  mFramePresShell->GetPresContext()->RestyleManager()->PostRestyleEvent(
    mFrame->GetContent()->AsElement(), nsRestyleHint(0), changeHint);
}

NS_IMPL_ISUPPORTS(nsSVGFilterProperty, nsISupports)

nsSVGFilterProperty::nsSVGFilterProperty(const nsTArray<nsStyleFilter> &aFilters,
                                         nsIFrame *aFilteredFrame)
{
  for (uint32_t i = 0; i < aFilters.Length(); i++) {
    if (aFilters[i].GetType() != NS_STYLE_FILTER_URL)
      continue;

    nsRefPtr<nsSVGFilterReference> reference =
      new nsSVGFilterReference(aFilters[i].GetURL(), aFilteredFrame);
    mReferences.AppendElement(reference);
  }
}

bool
nsSVGFilterProperty::ReferencesValidResources()
{
  for (uint32_t i = 0; i < mReferences.Length(); i++) {
    if (!mReferences[i]->ReferencesValidResource())
      return false;
  }
  return true;
}

bool
nsSVGFilterProperty::IsInObserverLists() const
{
  for (uint32_t i = 0; i < mReferences.Length(); i++) {
    if (!mReferences[i]->IsInObserverList())
      return false;
  }
  return true;
}

void
nsSVGFilterProperty::Invalidate()
{
  for (uint32_t i = 0; i < mReferences.Length(); i++) {
    mReferences[i]->Invalidate();
  }
}

void
nsSVGMarkerProperty::DoUpdate()
{
  nsSVGIDRenderingObserver::DoUpdate();
  if (!mFrame)
    return;

  NS_ASSERTION(mFrame->IsFrameOfType(nsIFrame::eSVG), "SVG frame expected");

  
  nsChangeHint changeHint =
    nsChangeHint(nsChangeHint_RepaintFrame);

  
  if (!(mFrame->GetStateBits() & NS_FRAME_IN_REFLOW)) {
    
    
    nsSVGEffects::InvalidateRenderingObservers(mFrame);
    
    nsSVGUtils::ScheduleReflowSVG(mFrame);
  }
  mFramePresShell->GetPresContext()->RestyleManager()->PostRestyleEvent(
    mFrame->GetContent()->AsElement(), nsRestyleHint(0), changeHint);
}

bool
nsSVGTextPathProperty::TargetIsValid()
{
  Element* target = GetTarget();
  return target && target->IsSVG(nsGkAtoms::path);
}

void
nsSVGTextPathProperty::DoUpdate()
{
  nsSVGIDRenderingObserver::DoUpdate();
  if (!mFrame)
    return;

  NS_ASSERTION(mFrame->IsFrameOfType(nsIFrame::eSVG) || mFrame->IsSVGText(),
               "SVG frame expected");

  
  
  
  
  
  
  
  
  
  
  bool nowValid = TargetIsValid();
  if (!mValid && !nowValid) {
    
    return;
  }
  mValid = nowValid;

  
  nsChangeHint changeHint =
    nsChangeHint(nsChangeHint_RepaintFrame | nsChangeHint_UpdateTextPath);
  mFramePresShell->GetPresContext()->RestyleManager()->PostRestyleEvent(
    mFrame->GetContent()->AsElement(), nsRestyleHint(0), changeHint);
}

static void
InvalidateAllContinuations(nsIFrame* aFrame)
{
  for (nsIFrame* f = aFrame; f;
       f = nsLayoutUtils::GetNextContinuationOrIBSplitSibling(f)) {
    f->InvalidateFrame();
  }
}

void
nsSVGPaintingProperty::DoUpdate()
{
  nsSVGIDRenderingObserver::DoUpdate();
  if (!mFrame)
    return;

  if (mFrame->GetStateBits() & NS_FRAME_SVG_LAYOUT) {
    nsSVGEffects::InvalidateRenderingObservers(mFrame);
    mFrame->InvalidateFrameSubtree();
  } else {
    InvalidateAllContinuations(mFrame);
  }
}

static nsSVGRenderingObserver *
CreateMarkerProperty(nsIURI *aURI, nsIFrame *aFrame, bool aReferenceImage)
{ return new nsSVGMarkerProperty(aURI, aFrame, aReferenceImage); }

static nsSVGRenderingObserver *
CreateTextPathProperty(nsIURI *aURI, nsIFrame *aFrame, bool aReferenceImage)
{ return new nsSVGTextPathProperty(aURI, aFrame, aReferenceImage); }

static nsSVGRenderingObserver *
CreatePaintingProperty(nsIURI *aURI, nsIFrame *aFrame, bool aReferenceImage)
{ return new nsSVGPaintingProperty(aURI, aFrame, aReferenceImage); }

static nsSVGRenderingObserver *
GetEffectProperty(nsIURI *aURI, nsIFrame *aFrame,
                  const FramePropertyDescriptor *aProperty,
                  nsSVGRenderingObserver * (* aCreate)(nsIURI *, nsIFrame *, bool))
{
  if (!aURI)
    return nullptr;

  FrameProperties props = aFrame->Properties();
  nsSVGRenderingObserver *prop =
    static_cast<nsSVGRenderingObserver*>(props.Get(aProperty));
  if (prop)
    return prop;
  prop = aCreate(aURI, aFrame, false);
  if (!prop)
    return nullptr;
  NS_ADDREF(prop);
  props.Set(aProperty, static_cast<nsISupports*>(prop));
  return prop;
}

static nsSVGFilterProperty*
GetOrCreateFilterProperty(nsIFrame *aFrame)
{
  const nsStyleSVGReset* style = aFrame->StyleSVGReset();
  if (!style->HasFilters())
    return nullptr;

  FrameProperties props = aFrame->Properties();
  nsSVGFilterProperty *prop =
    static_cast<nsSVGFilterProperty*>(props.Get(nsSVGEffects::FilterProperty()));
  if (prop)
    return prop;
  prop = new nsSVGFilterProperty(style->mFilters, aFrame);
  if (!prop)
    return nullptr;
  NS_ADDREF(prop);
  props.Set(nsSVGEffects::FilterProperty(), static_cast<nsISupports*>(prop));
  return prop;
}

nsSVGMarkerProperty *
nsSVGEffects::GetMarkerProperty(nsIURI *aURI, nsIFrame *aFrame,
                                const FramePropertyDescriptor *aProp)
{
  NS_ABORT_IF_FALSE(aFrame->GetType() == nsGkAtoms::svgPathGeometryFrame &&
                      static_cast<nsSVGPathGeometryElement*>(aFrame->GetContent())->IsMarkable(),
                    "Bad frame");
  return static_cast<nsSVGMarkerProperty*>(
          GetEffectProperty(aURI, aFrame, aProp, CreateMarkerProperty));
}

nsSVGTextPathProperty *
nsSVGEffects::GetTextPathProperty(nsIURI *aURI, nsIFrame *aFrame,
                                  const FramePropertyDescriptor *aProp)
{
  return static_cast<nsSVGTextPathProperty*>(
          GetEffectProperty(aURI, aFrame, aProp, CreateTextPathProperty));
}

nsSVGPaintingProperty *
nsSVGEffects::GetPaintingProperty(nsIURI *aURI, nsIFrame *aFrame,
                                  const FramePropertyDescriptor *aProp)
{
  return static_cast<nsSVGPaintingProperty*>(
          GetEffectProperty(aURI, aFrame, aProp, CreatePaintingProperty));
}

static nsSVGRenderingObserver *
GetEffectPropertyForURI(nsIURI *aURI, nsIFrame *aFrame,
                        const FramePropertyDescriptor *aProperty,
                        nsSVGRenderingObserver * (* aCreate)(nsIURI *, nsIFrame *, bool))
{
  if (!aURI)
    return nullptr;

  FrameProperties props = aFrame->Properties();
  nsSVGEffects::URIObserverHashtable *hashtable =
    static_cast<nsSVGEffects::URIObserverHashtable*>(props.Get(aProperty));
  if (!hashtable) {
    hashtable = new nsSVGEffects::URIObserverHashtable();
    props.Set(aProperty, hashtable);
  }
  nsSVGRenderingObserver* prop =
    static_cast<nsSVGRenderingObserver*>(hashtable->GetWeak(aURI));
  if (!prop) {
    bool watchImage = aProperty == nsSVGEffects::BackgroundImageProperty();
    prop = aCreate(aURI, aFrame, watchImage);
    hashtable->Put(aURI, prop);
  }
  return prop;
}

nsSVGPaintingProperty *
nsSVGEffects::GetPaintingPropertyForURI(nsIURI *aURI, nsIFrame *aFrame,
                                        const FramePropertyDescriptor *aProp)
{
  return static_cast<nsSVGPaintingProperty*>(
          GetEffectPropertyForURI(aURI, aFrame, aProp, CreatePaintingProperty));
}

nsSVGEffects::EffectProperties
nsSVGEffects::GetEffectProperties(nsIFrame *aFrame)
{
  NS_ASSERTION(!aFrame->GetPrevContinuation(), "aFrame should be first continuation");

  EffectProperties result;
  const nsStyleSVGReset *style = aFrame->StyleSVGReset();
  result.mFilter = GetOrCreateFilterProperty(aFrame);
  result.mClipPath =
    GetPaintingProperty(style->mClipPath, aFrame, ClipPathProperty());
  result.mMask =
    GetPaintingProperty(style->mMask, aFrame, MaskProperty());
  return result;
}

nsSVGPaintServerFrame *
nsSVGEffects::GetPaintServer(nsIFrame *aTargetFrame, const nsStyleSVGPaint *aPaint,
                             const FramePropertyDescriptor *aType)
{
  if (aPaint->mType != eStyleSVGPaintType_Server)
    return nullptr;

  nsIFrame *frame = aTargetFrame->GetContent()->IsNodeOfType(nsINode::eTEXT) ?
                      aTargetFrame->GetParent() : aTargetFrame;
  nsSVGPaintingProperty *property =
    nsSVGEffects::GetPaintingProperty(aPaint->mPaint.mPaintServer, frame, aType);
  if (!property)
    return nullptr;
  nsIFrame *result = property->GetReferencedFrame();
  if (!result)
    return nullptr;

  nsIAtom *type = result->GetType();
  if (type != nsGkAtoms::svgLinearGradientFrame &&
      type != nsGkAtoms::svgRadialGradientFrame &&
      type != nsGkAtoms::svgPatternFrame)
    return nullptr;

  return static_cast<nsSVGPaintServerFrame*>(result);
}

nsSVGClipPathFrame *
nsSVGEffects::EffectProperties::GetClipPathFrame(bool *aOK)
{
  if (!mClipPath)
    return nullptr;
  nsSVGClipPathFrame *frame = static_cast<nsSVGClipPathFrame *>
    (mClipPath->GetReferencedFrame(nsGkAtoms::svgClipPathFrame, aOK));
  if (frame && aOK && *aOK) {
    *aOK = frame->IsValid();
  }
  return frame;
}

nsSVGMaskFrame *
nsSVGEffects::EffectProperties::GetMaskFrame(bool *aOK)
{
  if (!mMask)
    return nullptr;
  return static_cast<nsSVGMaskFrame *>
    (mMask->GetReferencedFrame(nsGkAtoms::svgMaskFrame, aOK));
}

void
nsSVGEffects::UpdateEffects(nsIFrame *aFrame)
{
  NS_ASSERTION(aFrame->GetContent()->IsElement(),
               "aFrame's content should be an element");

  FrameProperties props = aFrame->Properties();
  props.Delete(FilterProperty());
  props.Delete(MaskProperty());
  props.Delete(ClipPathProperty());
  props.Delete(MarkerBeginProperty());
  props.Delete(MarkerMiddleProperty());
  props.Delete(MarkerEndProperty());
  props.Delete(FillProperty());
  props.Delete(StrokeProperty());
  props.Delete(BackgroundImageProperty());

  
  
  GetOrCreateFilterProperty(aFrame);

  if (aFrame->GetType() == nsGkAtoms::svgPathGeometryFrame &&
      static_cast<nsSVGPathGeometryElement*>(aFrame->GetContent())->IsMarkable()) {
    
    const nsStyleSVG *style = aFrame->StyleSVG();
    GetEffectProperty(style->mMarkerStart, aFrame, MarkerBeginProperty(),
                      CreateMarkerProperty);
    GetEffectProperty(style->mMarkerMid, aFrame, MarkerMiddleProperty(),
                      CreateMarkerProperty);
    GetEffectProperty(style->mMarkerEnd, aFrame, MarkerEndProperty(),
                      CreateMarkerProperty);
  }
}

nsSVGFilterProperty *
nsSVGEffects::GetFilterProperty(nsIFrame *aFrame)
{
  NS_ASSERTION(!aFrame->GetPrevContinuation(), "aFrame should be first continuation");

  if (!aFrame->StyleSVGReset()->HasFilters())
    return nullptr;

  return static_cast<nsSVGFilterProperty *>
    (aFrame->Properties().Get(FilterProperty()));
}

static PLDHashOperator
GatherEnumerator(nsPtrHashKey<nsSVGRenderingObserver>* aEntry, void* aArg)
{
  nsTArray<nsSVGRenderingObserver*>* array =
    static_cast<nsTArray<nsSVGRenderingObserver*>*>(aArg);
  array->AppendElement(aEntry->GetKey());

  return PL_DHASH_REMOVE;
}

static PLDHashOperator
GatherEnumeratorForReflow(nsPtrHashKey<nsSVGRenderingObserver>* aEntry, void* aArg)
{
  if (!aEntry->GetKey()->ObservesReflow()) {
    return PL_DHASH_NEXT;
  }

  nsTArray<nsSVGRenderingObserver*>* array =
    static_cast<nsTArray<nsSVGRenderingObserver*>*>(aArg);
  array->AppendElement(aEntry->GetKey());

  return PL_DHASH_REMOVE;
}

void
nsSVGRenderingObserverList::InvalidateAll()
{
  if (mObservers.Count() == 0)
    return;

  nsAutoTArray<nsSVGRenderingObserver*,10> observers;

  
  mObservers.EnumerateEntries(GatherEnumerator, &observers);

  for (uint32_t i = 0; i < observers.Length(); ++i) {
    observers[i]->InvalidateViaReferencedElement();
  }
}

void
nsSVGRenderingObserverList::InvalidateAllForReflow()
{
  if (mObservers.Count() == 0)
    return;

  nsAutoTArray<nsSVGRenderingObserver*,10> observers;

  
  mObservers.EnumerateEntries(GatherEnumeratorForReflow, &observers);

  for (uint32_t i = 0; i < observers.Length(); ++i) {
    observers[i]->InvalidateViaReferencedElement();
  }
}

void
nsSVGRenderingObserverList::RemoveAll()
{
  nsAutoTArray<nsSVGRenderingObserver*,10> observers;

  
  mObservers.EnumerateEntries(GatherEnumerator, &observers);

  
  
  for (uint32_t i = 0; i < observers.Length(); ++i) {
    observers[i]->NotifyEvictedFromRenderingObserverList();
  }
}

void
nsSVGEffects::AddRenderingObserver(Element *aElement, nsSVGRenderingObserver *aObserver)
{
  nsSVGRenderingObserverList *observerList = GetObserverList(aElement);
  if (!observerList) {
    observerList = new nsSVGRenderingObserverList();
    if (!observerList)
      return;
    aElement->SetProperty(nsGkAtoms::renderingobserverlist, observerList,
                          nsINode::DeleteProperty<nsSVGRenderingObserverList>);
  }
  aElement->SetHasRenderingObservers(true);
  observerList->Add(aObserver);
}

void
nsSVGEffects::RemoveRenderingObserver(Element *aElement, nsSVGRenderingObserver *aObserver)
{
  nsSVGRenderingObserverList *observerList = GetObserverList(aElement);
  if (observerList) {
    NS_ASSERTION(observerList->Contains(aObserver),
                 "removing observer from an element we're not observing?");
    observerList->Remove(aObserver);
    if (observerList->IsEmpty()) {
      aElement->SetHasRenderingObservers(false);
    }
  }
}

void
nsSVGEffects::RemoveAllRenderingObservers(Element *aElement)
{
  nsSVGRenderingObserverList *observerList = GetObserverList(aElement);
  if (observerList) {
    observerList->RemoveAll();
    aElement->SetHasRenderingObservers(false);
  }
}

void
nsSVGEffects::InvalidateRenderingObservers(nsIFrame *aFrame)
{
  NS_ASSERTION(!aFrame->GetPrevContinuation(), "aFrame must be first continuation");

  if (!aFrame->GetContent()->IsElement())
    return;

  nsSVGRenderingObserverList *observerList =
    GetObserverList(aFrame->GetContent()->AsElement());
  if (observerList) {
    observerList->InvalidateAll();
    return;
  }

  
  
  for (nsIFrame *f = aFrame->GetParent();
       f->IsFrameOfType(nsIFrame::eSVGContainer); f = f->GetParent()) {
    if (f->GetContent()->IsElement()) {
      observerList = GetObserverList(f->GetContent()->AsElement());
      if (observerList) {
        observerList->InvalidateAll();
        return;
      }
    }
  }
}

void
nsSVGEffects::InvalidateDirectRenderingObservers(Element *aElement, uint32_t aFlags )
{
  if (aElement->HasRenderingObservers()) {
    nsSVGRenderingObserverList *observerList = GetObserverList(aElement);
    if (observerList) {
      if (aFlags & INVALIDATE_REFLOW) {
        observerList->InvalidateAllForReflow();
      } else {
        observerList->InvalidateAll();
      }
    }
  }
}

void
nsSVGEffects::InvalidateDirectRenderingObservers(nsIFrame *aFrame, uint32_t aFlags )
{
  if (aFrame->GetContent() && aFrame->GetContent()->IsElement()) {
    InvalidateDirectRenderingObservers(aFrame->GetContent()->AsElement(), aFlags);
  }
}
