





#include "nsContentUtils.h"
#include "nsFrame.h"
#include "nsGkAtoms.h"
#include "nsLiteralString.h"
#include "nsSVGEffects.h"
#include "nsSVGFilters.h"

using namespace mozilla;

typedef nsFrame SVGFEImageFrameBase;

class SVGFEImageFrame : public SVGFEImageFrameBase
{
  friend nsIFrame*
  NS_NewSVGFEImageFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  SVGFEImageFrame(nsStyleContext* aContext)
    : SVGFEImageFrameBase(aContext)
  {
    AddStateBits(NS_FRAME_SVG_LAYOUT | NS_STATE_SVG_NONDISPLAY_CHILD);
  }

public:
  NS_DECL_FRAMEARENA_HELPERS

  NS_IMETHOD Init(nsIContent* aContent,
                  nsIFrame*   aParent,
                  nsIFrame*   aPrevInFlow);
  virtual void DestroyFrom(nsIFrame* aDestructRoot);

  virtual bool IsFrameOfType(uint32_t aFlags) const
  {
    return SVGFEImageFrameBase::IsFrameOfType(aFlags & ~(nsIFrame::eSVG));
  }

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGFEImage"), aResult);
  }
#endif

  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext);

  




  virtual nsIAtom* GetType() const;

  NS_IMETHOD AttributeChanged(int32_t  aNameSpaceID,
                              nsIAtom* aAttribute,
                              int32_t  aModType);

  virtual bool UpdateOverflow() {
    
    return false;
  }
};

nsIFrame*
NS_NewSVGFEImageFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) SVGFEImageFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(SVGFEImageFrame)

 void
SVGFEImageFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  SVGFEImageFrameBase::DidSetStyleContext(aOldStyleContext);
  nsSVGEffects::InvalidateRenderingObservers(this);
}

 void
SVGFEImageFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  nsCOMPtr<nsIImageLoadingContent> imageLoader =
    do_QueryInterface(SVGFEImageFrameBase::mContent);

  if (imageLoader) {
    imageLoader->FrameDestroyed(this);
  }

  SVGFEImageFrameBase::DestroyFrom(aDestructRoot);
}

NS_IMETHODIMP
SVGFEImageFrame::Init(nsIContent* aContent,
                        nsIFrame* aParent,
                        nsIFrame* aPrevInFlow)
{
  NS_ASSERTION(aContent->IsSVG(nsGkAtoms::feImage),
               "Trying to construct an SVGFEImageFrame for a "
               "content element that doesn't support the right interfaces");

  SVGFEImageFrameBase::Init(aContent, aParent, aPrevInFlow);
  nsCOMPtr<nsIImageLoadingContent> imageLoader =
    do_QueryInterface(SVGFEImageFrameBase::mContent);

  if (imageLoader) {
    imageLoader->FrameCreated(this);
  }

  return NS_OK;
}

nsIAtom *
SVGFEImageFrame::GetType() const
{
  return nsGkAtoms::svgFEImageFrame;
}

NS_IMETHODIMP
SVGFEImageFrame::AttributeChanged(int32_t  aNameSpaceID,
                                  nsIAtom* aAttribute,
                                  int32_t  aModType)
{
  nsSVGFEImageElement *element = static_cast<nsSVGFEImageElement*>(mContent);
  if (element->AttributeAffectsRendering(aNameSpaceID, aAttribute)) {
    nsSVGEffects::InvalidateRenderingObservers(this);
  }
  if (aNameSpaceID == kNameSpaceID_XLink &&
      aAttribute == nsGkAtoms::href) {

    
    if (nsContentUtils::IsImageSrcSetDisabled()) {
      return NS_OK;
    }

    if (element->mStringAttributes[nsSVGFEImageElement::HREF].IsExplicitlySet()) {
      element->LoadSVGImage(true, true);
    } else {
      element->CancelImageRequests(true);
    }
  }

  return SVGFEImageFrameBase::AttributeChanged(aNameSpaceID,
                                                 aAttribute, aModType);
}
