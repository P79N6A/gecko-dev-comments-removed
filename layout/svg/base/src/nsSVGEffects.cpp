




































#include "nsSVGEffects.h"
#include "nsISupportsImpl.h"
#include "nsSVGOuterSVGFrame.h"
#include "nsSVGFilterFrame.h"
#include "nsSVGClipPathFrame.h"
#include "nsSVGMaskFrame.h"

NS_IMPL_ISUPPORTS1(nsSVGRenderingObserver, nsIMutationObserver)

nsSVGRenderingObserver::nsSVGRenderingObserver(nsIURI *aURI,
                                               nsIFrame *aFrame)
  : mElement(this), mFrame(aFrame),
    mFramePresShell(aFrame->PresContext()->PresShell()),
    mReferencedFrame(nsnull),
    mReferencedFramePresShell(nsnull)
{
  
  mElement.Reset(aFrame->GetContent(), aURI);
  if (mElement.get()) {
    mElement.get()->AddMutationObserver(this);
  }
}

nsSVGRenderingObserver::~nsSVGRenderingObserver()
{
  if (mElement.get()) {
    mElement.get()->RemoveMutationObserver(this);
  }
  if (mReferencedFrame && !mReferencedFramePresShell->IsDestroying()) {
    nsSVGEffects::RemoveRenderingObserver(mReferencedFrame, this);
  }
}

nsIFrame*
nsSVGRenderingObserver::GetReferencedFrame()
{
  if (mReferencedFrame && !mReferencedFramePresShell->IsDestroying()) {
    NS_ASSERTION(mElement.get() &&
                 static_cast<nsGenericElement*>(mElement.get())->GetPrimaryFrame() == mReferencedFrame,
                 "Cached frame is incorrect!");
    return mReferencedFrame;
  }

  if (mElement.get()) {
    nsIFrame *frame =
      static_cast<nsGenericElement*>(mElement.get())->GetPrimaryFrame();
    if (frame) {
      mReferencedFrame = frame;
      mReferencedFramePresShell = mReferencedFrame->PresContext()->PresShell();
      nsSVGEffects::AddRenderingObserver(mReferencedFrame, this);
      return frame;
    }
  }
  return nsnull;
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
nsSVGRenderingObserver::DoUpdate()
{
  if (mFramePresShell->IsDestroying()) {
    
    mFrame = nsnull;
    return;
  }
  if (mReferencedFrame) {
    nsSVGEffects::RemoveRenderingObserver(mReferencedFrame, this);
    mReferencedFrame = nsnull;
    mReferencedFramePresShell = nsnull;
  }
  if (mFrame && mFrame->IsFrameOfType(nsIFrame::eSVG)) {
    
    
    nsSVGEffects::InvalidateRenderingObservers(mFrame);
  }
}

void
nsSVGRenderingObserver::InvalidateViaReferencedFrame()
{
  
  
  mReferencedFrame = nsnull;
  mReferencedFramePresShell = nsnull;
  DoUpdate();
}

void
nsSVGRenderingObserver::AttributeChanged(nsIDocument *aDocument,
                                         nsIContent *aContent,
                                         PRInt32 aNameSpaceID,
                                         nsIAtom *aAttribute,
                                         PRInt32 aModType,
                                         PRUint32 aStateMask)
{
  DoUpdate();
}

void
nsSVGRenderingObserver::ContentAppended(nsIDocument *aDocument,
                                        nsIContent *aContainer,
                                        PRInt32 aNewIndexInContainer)
{
  DoUpdate();
}

void
nsSVGRenderingObserver::ContentInserted(nsIDocument *aDocument,
                                        nsIContent *aContainer,
                                        nsIContent *aChild,
                                        PRInt32 aIndexInContainer)
{
  DoUpdate();
}

void
nsSVGRenderingObserver::ContentRemoved(nsIDocument *aDocument,
                                       nsIContent *aContainer,
                                       nsIContent *aChild,
                                       PRInt32 aIndexInContainer)
{
  DoUpdate();
}

NS_IMPL_ISUPPORTS_INHERITED1(nsSVGFilterProperty,
                             nsSVGRenderingObserver,
                             nsISVGFilterProperty)

nsSVGFilterProperty::nsSVGFilterProperty(nsIURI *aURI,
                                         nsIFrame *aFilteredFrame)
  : nsSVGRenderingObserver(aURI, aFilteredFrame)
{
  UpdateRect();
}

nsSVGFilterFrame *
nsSVGFilterProperty::GetFilterFrame()
{
  return static_cast<nsSVGFilterFrame *>
    (GetReferencedFrame(nsGkAtoms::svgFilterFrame, nsnull));
}

void
nsSVGFilterProperty::UpdateRect()
{
  nsSVGFilterFrame *filter = GetFilterFrame();
  if (filter) {
    mFilterRect = filter->GetFilterBBox(mFrame, nsnull);
    mFilterRect.ScaleRoundOut(filter->PresContext()->AppUnitsPerDevPixel());
  } else {
    mFilterRect = nsRect();
  }
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
  nsSVGRenderingObserver::DoUpdate();
  if (!mFrame)
    return;

  if (mFrame->IsFrameOfType(nsIFrame::eSVG)) {
    nsSVGOuterSVGFrame *outerSVGFrame = nsSVGUtils::GetOuterSVGFrame(mFrame);
    if (outerSVGFrame) {
      outerSVGFrame->Invalidate(mFilterRect);
      UpdateRect();
      outerSVGFrame->Invalidate(mFilterRect);
    }
  } else {
    InvalidateAllContinuations(mFrame);
    
    mFramePresShell->FrameNeedsReflow(
         mFrame, nsIPresShell::eResize, NS_FRAME_IS_DIRTY);
  }
}

void
nsSVGMarkerProperty::DoUpdate()
{
  nsSVGRenderingObserver::DoUpdate();
  if (!mFrame)
    return;

  if (mFrame->IsFrameOfType(nsIFrame::eSVG)) {
    nsSVGOuterSVGFrame *outerSVGFrame = nsSVGUtils::GetOuterSVGFrame(mFrame);
    if (outerSVGFrame) {
      
      outerSVGFrame->UpdateAndInvalidateCoveredRegion(mFrame);
    }
  } else {
    InvalidateAllContinuations(mFrame);
  }
}

void
nsSVGPaintingProperty::DoUpdate()
{
  nsSVGRenderingObserver::DoUpdate();
  if (!mFrame)
    return;

  if (mFrame->IsFrameOfType(nsIFrame::eSVG)) {
    nsSVGOuterSVGFrame *outerSVGFrame = nsSVGUtils::GetOuterSVGFrame(mFrame);
    if (outerSVGFrame) {
      outerSVGFrame->InvalidateCoveredRegion(mFrame);
    }
  } else {
    InvalidateAllContinuations(mFrame);
  }
}

static nsSVGRenderingObserver *
CreateFilterProperty(nsIURI *aURI, nsIFrame *aFrame)
{ return new nsSVGFilterProperty(aURI, aFrame); }

static nsSVGRenderingObserver *
CreateMarkerProperty(nsIURI *aURI, nsIFrame *aFrame)
{ return new nsSVGMarkerProperty(aURI, aFrame); }

static nsSVGRenderingObserver *
CreatePaintingProperty(nsIURI *aURI, nsIFrame *aFrame)
{ return new nsSVGPaintingProperty(aURI, aFrame); }

static nsSVGRenderingObserver *
GetEffectProperty(nsIURI *aURI, nsIFrame *aFrame, nsIAtom *aProp,
                  nsSVGRenderingObserver * (* aCreate)(nsIURI *, nsIFrame *))
{
  if (!aURI)
    return nsnull;
  nsSVGRenderingObserver *prop =
    static_cast<nsSVGRenderingObserver*>(aFrame->GetProperty(aProp));
  if (prop)
    return prop;
  prop = aCreate(aURI, aFrame);
  if (!prop)
    return nsnull;
  NS_ADDREF(prop);
  aFrame->SetProperty(aProp,
                      static_cast<nsISupports*>(prop),
                      nsPropertyTable::SupportsDtorFunc);
  return prop;
}

nsSVGMarkerProperty *
nsSVGEffects::GetMarkerProperty(nsIURI *aURI, nsIFrame *aFrame, nsIAtom *aProp)
{
  return static_cast<nsSVGMarkerProperty*>(
          GetEffectProperty(aURI, aFrame, aProp, CreateMarkerProperty));
}

nsSVGPaintingProperty *
nsSVGEffects::GetPaintingProperty(nsIURI *aURI, nsIFrame *aFrame, nsIAtom *aProp)
{
  return static_cast<nsSVGPaintingProperty*>(
          GetEffectProperty(aURI, aFrame, aProp, CreatePaintingProperty));
}

nsSVGEffects::EffectProperties
nsSVGEffects::GetEffectProperties(nsIFrame *aFrame)
{
  NS_ASSERTION(!aFrame->GetPrevContinuation(), "aFrame should be first continuation");

  EffectProperties result;
  const nsStyleSVGReset *style = aFrame->GetStyleSVGReset();
  result.mFilter = static_cast<nsSVGFilterProperty*>
    (GetEffectProperty(style->mFilter, aFrame, nsGkAtoms::filter, CreateFilterProperty));
  result.mClipPath = GetPaintingProperty(style->mClipPath, aFrame, nsGkAtoms::clipPath);
  result.mMask = GetPaintingProperty(style->mMask, aFrame, nsGkAtoms::mask);
  return result;
}

nsSVGClipPathFrame *
nsSVGEffects::EffectProperties::GetClipPathFrame(PRBool *aOK)
{
  if (!mClipPath)
    return nsnull;
  return static_cast<nsSVGClipPathFrame *>
    (mClipPath->GetReferencedFrame(nsGkAtoms::svgClipPathFrame, aOK));
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
  aFrame->DeleteProperty(nsGkAtoms::filter);
  aFrame->DeleteProperty(nsGkAtoms::mask);
  aFrame->DeleteProperty(nsGkAtoms::clipPath);

  aFrame->DeleteProperty(nsGkAtoms::marker_start);
  aFrame->DeleteProperty(nsGkAtoms::marker_mid);
  aFrame->DeleteProperty(nsGkAtoms::marker_end);

  aFrame->DeleteProperty(nsGkAtoms::stroke);
  aFrame->DeleteProperty(nsGkAtoms::fill);
}

nsSVGFilterProperty *
nsSVGEffects::GetFilterProperty(nsIFrame *aFrame)
{
  NS_ASSERTION(!aFrame->GetPrevContinuation(), "aFrame should be first continuation");

  if (!aFrame->GetStyleSVGReset()->mFilter)
    return nsnull;

  return static_cast<nsSVGFilterProperty *>(aFrame->GetProperty(nsGkAtoms::filter));
}

static PLDHashOperator PR_CALLBACK
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
    observers[i]->InvalidateViaReferencedFrame();
  }
}

static nsSVGRenderingObserverList *
GetObserverList(nsIFrame *aFrame)
{
  if (!(aFrame->GetStateBits() & NS_FRAME_MAY_BE_TRANSFORMED_OR_HAVE_RENDERING_OBSERVERS))
    return nsnull;
  return static_cast<nsSVGRenderingObserverList*>(aFrame->GetProperty(nsGkAtoms::observer));
}

void
nsSVGEffects::AddRenderingObserver(nsIFrame *aFrame, nsSVGRenderingObserver *aObserver)
{
  NS_ASSERTION(!aFrame->GetPrevContinuation(), "aFrame must be first continuation");

  nsSVGRenderingObserverList *observerList = GetObserverList(aFrame);
  if (!observerList) {
    observerList = new nsSVGRenderingObserverList();
    if (!observerList)
      return;
    for (nsIFrame* f = aFrame; f; f = f->GetNextContinuation()) {
      f->AddStateBits(NS_FRAME_MAY_BE_TRANSFORMED_OR_HAVE_RENDERING_OBSERVERS);
    }
    aFrame->SetProperty(nsGkAtoms::observer, observerList);
  }
  observerList->Add(aObserver);
}

void
nsSVGEffects::RemoveRenderingObserver(nsIFrame *aFrame, nsSVGRenderingObserver *aObserver)
{
  NS_ASSERTION(!aFrame->GetPrevContinuation(), "aFrame must be first continuation");

  nsSVGRenderingObserverList *observerList = GetObserverList(aFrame);
  if (observerList) {
    observerList->Remove(aObserver);
    
    
    
  }
}

void
nsSVGEffects::InvalidateRenderingObservers(nsIFrame *aFrame)
{
  NS_ASSERTION(!aFrame->GetPrevContinuation(), "aFrame must be first continuation");

  nsSVGRenderingObserverList *observerList = GetObserverList(aFrame);
  if (observerList) {
    observerList->InvalidateAll();
    return;
  }

  
  
  for (nsIFrame *f = aFrame->GetParent();
       f->IsFrameOfType(nsIFrame::eSVGContainer); f = f->GetParent()) {
    observerList = GetObserverList(f);
    if (observerList) {
      observerList->InvalidateAll();
      return;
    }
  }
}

void
nsSVGEffects::InvalidateDirectRenderingObservers(nsIFrame *aFrame)
{
  nsSVGRenderingObserverList *observerList = GetObserverList(aFrame);
  if (observerList) {
    observerList->InvalidateAll();
  }
}
