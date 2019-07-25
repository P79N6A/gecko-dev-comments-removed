




































#include "nsSVGEffects.h"
#include "nsISupportsImpl.h"
#include "nsSVGOuterSVGFrame.h"
#include "nsSVGFilterFrame.h"
#include "nsSVGClipPathFrame.h"
#include "nsSVGMaskFrame.h"
#include "nsSVGTextPathFrame.h"
#include "nsCSSFrameConstructor.h"
#include "nsFrameManager.h"

using namespace mozilla;
using namespace mozilla::dom;


NS_IMPL_ISUPPORTS1(nsSVGRenderingObserver, nsIMutationObserver)

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
      mInObserverList = PR_FALSE;
    }
  }
  NS_ASSERTION(!mInObserverList, "still in an observer list?");
}




















#ifdef _MSC_VER



#pragma warning(push)
#pragma warning(disable:4355)
#endif
nsSVGIDRenderingObserver::nsSVGIDRenderingObserver(nsIURI *aURI,
                                                   nsIFrame *aFrame,
                                                   PRBool aReferenceImage)
  : mElement(this), mFrame(aFrame),
    mFramePresShell(aFrame->PresContext()->PresShell())
#ifdef _MSC_VER
#pragma warning(pop)
#endif
{
  
  mElement.Reset(aFrame->GetContent(), aURI, PR_TRUE, aReferenceImage);
  StartListening();
}

nsSVGIDRenderingObserver::~nsSVGIDRenderingObserver()
{
  StopListening();
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
    PRBool inObserverList = observerList && observerList->Contains(this);
    NS_ASSERTION(inObserverList == mInObserverList, "failed to track whether we're in our referenced element's observer list!");
  } else {
    NS_ASSERTION(!mInObserverList, "In whose observer list are we, then?");
  }
#endif
  if (target && !mInObserverList) {
    nsSVGEffects::AddRenderingObserver(target, this);
    mInObserverList = PR_TRUE;
  }
  return target;
}

nsIFrame*
nsSVGRenderingObserver::GetReferencedFrame()
{
  Element* referencedElement = GetReferencedElement();
  return referencedElement ? referencedElement->GetPrimaryFrame() : nsnull;
}

nsIFrame*
nsSVGRenderingObserver::GetReferencedFrame(nsIAtom* aFrameType, PRBool* aOK)
{
  nsIFrame* frame = GetReferencedFrame();
  if (frame && frame->GetType() == aFrameType)
    return frame;
  if (aOK) {
    *aOK = PR_FALSE;
  }
  return nsnull;
}

void
nsSVGIDRenderingObserver::DoUpdate()
{
  if (mFramePresShell->IsDestroying()) {
    
    mFrame = nsnull;
    return;
  }
  if (mElement.get() && mInObserverList) {
    nsSVGEffects::RemoveRenderingObserver(mElement.get(), this);
    mInObserverList = PR_FALSE;
  }
  if (mFrame && mFrame->IsFrameOfType(nsIFrame::eSVG)) {
    
    
    nsSVGEffects::InvalidateRenderingObservers(mFrame);
  }
}

void
nsSVGRenderingObserver::InvalidateViaReferencedElement()
{
  mInObserverList = PR_FALSE;
  DoUpdate();
}

void
nsSVGRenderingObserver::AttributeChanged(nsIDocument* aDocument,
                                         dom::Element* aElement,
                                         PRInt32 aNameSpaceID,
                                         nsIAtom* aAttribute,
                                         PRInt32 aModType)
{
  
  
  
  
  
  
  
  
  
  
  
  

  DoUpdate();
}

void
nsSVGRenderingObserver::ContentAppended(nsIDocument *aDocument,
                                        nsIContent *aContainer,
                                        nsIContent *aFirstNewContent,
                                        PRInt32 )
{
  DoUpdate();
}

void
nsSVGRenderingObserver::ContentInserted(nsIDocument *aDocument,
                                        nsIContent *aContainer,
                                        nsIContent *aChild,
                                        PRInt32 )
{
  DoUpdate();
}

void
nsSVGRenderingObserver::ContentRemoved(nsIDocument *aDocument,
                                       nsIContent *aContainer,
                                       nsIContent *aChild,
                                       PRInt32 aIndexInContainer,
                                       nsIContent *aPreviousSibling)
{
  DoUpdate();
}

NS_IMPL_ISUPPORTS_INHERITED1(nsSVGFilterProperty,
                             nsSVGIDRenderingObserver,
                             nsISVGFilterProperty)

nsSVGFilterFrame *
nsSVGFilterProperty::GetFilterFrame()
{
  return static_cast<nsSVGFilterFrame *>
    (GetReferencedFrame(nsGkAtoms::svgFilterFrame, nsnull));
}

static void
InvalidateAllContinuations(nsIFrame* aFrame)
{
  for (nsIFrame* f = aFrame; f; f = f->GetNextContinuation()) {
    f->InvalidateOverflowRect();
  }
}

void
nsSVGFilterProperty::DoUpdate()
{
  nsSVGIDRenderingObserver::DoUpdate();
  if (!mFrame)
    return;

  
  nsChangeHint changeHint =
    nsChangeHint(nsChangeHint_RepaintFrame | nsChangeHint_UpdateEffects);

  if (!mFrame->IsFrameOfType(nsIFrame::eSVG)) {
    NS_UpdateHint(changeHint, nsChangeHint_ReflowFrame);
  }
  mFramePresShell->FrameConstructor()->PostRestyleEvent(
    mFrame->GetContent()->AsElement(), nsRestyleHint(0), changeHint);
}

void
nsSVGMarkerProperty::DoUpdate()
{
  nsSVGIDRenderingObserver::DoUpdate();
  if (!mFrame)
    return;

  NS_ASSERTION(mFrame->IsFrameOfType(nsIFrame::eSVG), "SVG frame expected");

  
  nsChangeHint changeHint =
    nsChangeHint(nsChangeHint_RepaintFrame | nsChangeHint_UpdateEffects);

  mFramePresShell->FrameConstructor()->PostRestyleEvent(
    mFrame->GetContent()->AsElement(), nsRestyleHint(0), changeHint);
}

void
nsSVGTextPathProperty::DoUpdate()
{
  nsSVGIDRenderingObserver::DoUpdate();
  if (!mFrame)
    return;

  NS_ASSERTION(mFrame->IsFrameOfType(nsIFrame::eSVG), "SVG frame expected");

  if (mFrame->GetType() == nsGkAtoms::svgTextPathFrame) {
    nsSVGTextPathFrame* textPathFrame = static_cast<nsSVGTextPathFrame*>(mFrame);
    textPathFrame->NotifyGlyphMetricsChange();
  }
}

void
nsSVGPaintingProperty::DoUpdate()
{
  nsSVGIDRenderingObserver::DoUpdate();
  if (!mFrame)
    return;

  if (mFrame->IsFrameOfType(nsIFrame::eSVG)) {
    nsSVGUtils::InvalidateCoveredRegion(mFrame);
  } else {
    InvalidateAllContinuations(mFrame);
  }
}

static nsSVGRenderingObserver *
CreateFilterProperty(nsIURI *aURI, nsIFrame *aFrame, PRBool aReferenceImage)
{ return new nsSVGFilterProperty(aURI, aFrame, aReferenceImage); }

static nsSVGRenderingObserver *
CreateMarkerProperty(nsIURI *aURI, nsIFrame *aFrame, PRBool aReferenceImage)
{ return new nsSVGMarkerProperty(aURI, aFrame, aReferenceImage); }

static nsSVGRenderingObserver *
CreateTextPathProperty(nsIURI *aURI, nsIFrame *aFrame, PRBool aReferenceImage)
{ return new nsSVGTextPathProperty(aURI, aFrame, aReferenceImage); }

static nsSVGRenderingObserver *
CreatePaintingProperty(nsIURI *aURI, nsIFrame *aFrame, PRBool aReferenceImage)
{ return new nsSVGPaintingProperty(aURI, aFrame, aReferenceImage); }

static nsSVGRenderingObserver *
GetEffectProperty(nsIURI *aURI, nsIFrame *aFrame,
                  const FramePropertyDescriptor *aProperty,
                  nsSVGRenderingObserver * (* aCreate)(nsIURI *, nsIFrame *, PRBool))
{
  if (!aURI)
    return nsnull;

  FrameProperties props = aFrame->Properties();
  nsSVGRenderingObserver *prop =
    static_cast<nsSVGRenderingObserver*>(props.Get(aProperty));
  if (prop)
    return prop;
  prop = aCreate(aURI, aFrame, PR_FALSE);
  if (!prop)
    return nsnull;
  NS_ADDREF(prop);
  props.Set(aProperty, static_cast<nsISupports*>(prop));
  return prop;
}

nsSVGMarkerProperty *
nsSVGEffects::GetMarkerProperty(nsIURI *aURI, nsIFrame *aFrame,
                                const FramePropertyDescriptor *aProp)
{
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
                        nsSVGRenderingObserver * (* aCreate)(nsIURI *, nsIFrame *, PRBool))
{
  if (!aURI)
    return nsnull;

  FrameProperties props = aFrame->Properties();
  nsSVGEffects::URIObserverHashtable *hashtable =
    static_cast<nsSVGEffects::URIObserverHashtable*>(props.Get(aProperty));
  if (!hashtable) {
    hashtable = new nsSVGEffects::URIObserverHashtable();
    hashtable->Init();
    props.Set(aProperty, hashtable);
  }
  nsSVGRenderingObserver* prop =
    static_cast<nsSVGRenderingObserver*>(hashtable->GetWeak(aURI));
  if (!prop) {
    PRBool watchImage = aProperty == nsSVGEffects::BackgroundImageProperty();
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
  const nsStyleSVGReset *style = aFrame->GetStyleSVGReset();
  result.mFilter = static_cast<nsSVGFilterProperty*>
    (GetEffectProperty(style->mFilter, aFrame, FilterProperty(),
                       CreateFilterProperty));
  result.mClipPath =
    GetPaintingProperty(style->mClipPath, aFrame, ClipPathProperty());
  result.mMask =
    GetPaintingProperty(style->mMask, aFrame, MaskProperty());
  return result;
}

nsSVGClipPathFrame *
nsSVGEffects::EffectProperties::GetClipPathFrame(PRBool *aOK)
{
  if (!mClipPath)
    return nsnull;
  nsSVGClipPathFrame *frame = static_cast<nsSVGClipPathFrame *>
    (mClipPath->GetReferencedFrame(nsGkAtoms::svgClipPathFrame, aOK));
  if (frame && aOK && *aOK) {
    *aOK = frame->IsValid();
  }
  return frame;
}

nsSVGMaskFrame *
nsSVGEffects::EffectProperties::GetMaskFrame(PRBool *aOK)
{
  if (!mMask)
    return nsnull;
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

  
  
  GetEffectProperty(aFrame->GetStyleSVGReset()->mFilter,
                    aFrame, FilterProperty(), CreateFilterProperty);

  if (aFrame->IsFrameOfType(nsIFrame::eSVG)) {
    
    const nsStyleSVG *style = aFrame->GetStyleSVG();
    GetEffectProperty(style->mMarkerStart, aFrame, MarkerBeginProperty(),
                      CreateMarkerProperty);
    GetEffectProperty(style->mMarkerMid, aFrame, MarkerMiddleProperty(),
                      CreateMarkerProperty);
    GetEffectProperty(style->mMarkerEnd, aFrame, MarkerEndProperty(),
                      CreateMarkerProperty);
  }

  nsIFrame *kid = aFrame->GetFirstChild(nsnull);
  while (kid) {
    if (kid->GetContent()->IsElement()) {
      UpdateEffects(kid);
    }
    kid = kid->GetNextSibling();
  }
}

nsSVGFilterProperty *
nsSVGEffects::GetFilterProperty(nsIFrame *aFrame)
{
  NS_ASSERTION(!aFrame->GetPrevContinuation(), "aFrame should be first continuation");

  if (!aFrame->GetStyleSVGReset()->mFilter)
    return nsnull;

  return static_cast<nsSVGFilterProperty *>
    (aFrame->Properties().Get(FilterProperty()));
}

static PLDHashOperator
GatherEnumerator(nsVoidPtrHashKey* aEntry, void* aArg)
{
  nsTArray<nsSVGRenderingObserver*>* array =
    static_cast<nsTArray<nsSVGRenderingObserver*>*>(aArg);
  array->AppendElement(static_cast<nsSVGRenderingObserver*>(
          const_cast<void*>(aEntry->GetKey())));
  return PL_DHASH_REMOVE;
}

void
nsSVGRenderingObserverList::InvalidateAll()
{
  if (mObservers.Count() == 0)
    return;

  nsAutoTArray<nsSVGRenderingObserver*,10> observers;

  
  mObservers.EnumerateEntries(GatherEnumerator, &observers);

  for (PRUint32 i = 0; i < observers.Length(); ++i) {
    observers[i]->InvalidateViaReferencedElement();
  }
}

static void
DestroyObservers(void *aObject, nsIAtom *aPropertyName,
                 void *aPropertyValue, void *aData)
{
  delete static_cast<nsSVGRenderingObserverList*>(aPropertyValue);
}

void
nsSVGEffects::AddRenderingObserver(Element *aElement, nsSVGRenderingObserver *aObserver)
{
  nsSVGRenderingObserverList *observerList = GetObserverList(aElement);
  if (!observerList) {
    observerList = new nsSVGRenderingObserverList();
    if (!observerList)
      return;
    aElement->SetProperty(nsGkAtoms::renderingobserverlist, observerList, DestroyObservers);
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
nsSVGEffects::InvalidateDirectRenderingObservers(Element *aElement)
{
  if (aElement->HasRenderingObservers()) {
    nsSVGRenderingObserverList *observerList = GetObserverList(aElement);
    if (observerList) {
      observerList->InvalidateAll();
    }
  }
}

void
nsSVGEffects::InvalidateDirectRenderingObservers(nsIFrame *aFrame)
{
  if (aFrame->GetContent() && aFrame->GetContent()->IsElement()) {
    InvalidateDirectRenderingObservers(aFrame->GetContent()->AsElement());
  }
}
