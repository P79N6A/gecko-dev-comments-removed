





#include "gfxMatrix.h"
#include "mozilla/dom/SVGAElement.h"
#include "nsSVGContainerFrame.h"
#include "nsSVGIntegrationUtils.h"
#include "nsSVGUtils.h"
#include "SVGLengthList.h"

using namespace mozilla;

typedef nsSVGDisplayContainerFrame nsSVGAFrameBase;

class nsSVGAFrame : public nsSVGAFrameBase
{
  friend nsIFrame*
  NS_NewSVGAFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  explicit nsSVGAFrame(nsStyleContext* aContext) :
    nsSVGAFrameBase(aContext) {}

public:
  NS_DECL_FRAMEARENA_HELPERS

#ifdef DEBUG
  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;
#endif

  
  virtual nsresult  AttributeChanged(int32_t         aNameSpaceID,
                                     nsIAtom*        aAttribute,
                                     int32_t         aModType) MOZ_OVERRIDE;

  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGA"), aResult);
  }
#endif
  
  virtual void NotifySVGChanged(uint32_t aFlags) MOZ_OVERRIDE;
  
  
  virtual gfxMatrix GetCanvasTM(uint32_t aFor,
                                nsIFrame* aTransformRoot = nullptr) MOZ_OVERRIDE;

private:
  nsAutoPtr<gfxMatrix> mCanvasTM;
};




nsIFrame*
NS_NewSVGAFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGAFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGAFrame)



#ifdef DEBUG
void
nsSVGAFrame::Init(nsIContent*       aContent,
                  nsContainerFrame* aParent,
                  nsIFrame*         aPrevInFlow)
{
  NS_ASSERTION(aContent->IsSVG(nsGkAtoms::a),
               "Trying to construct an SVGAFrame for a "
               "content element that doesn't support the right interfaces");

  nsSVGAFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif 

nsresult
nsSVGAFrame::AttributeChanged(int32_t         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              int32_t         aModType)
{
  if (aNameSpaceID == kNameSpaceID_None &&
      aAttribute == nsGkAtoms::transform) {
    
    
    
    
    NotifySVGChanged(TRANSFORM_CHANGED);
  }

 return NS_OK;
}

nsIAtom *
nsSVGAFrame::GetType() const
{
  return nsGkAtoms::svgAFrame;
}




void
nsSVGAFrame::NotifySVGChanged(uint32_t aFlags)
{
  NS_ABORT_IF_FALSE(aFlags & (TRANSFORM_CHANGED | COORD_CONTEXT_CHANGED),
                    "Invalidation logic may need adjusting");

  if (aFlags & TRANSFORM_CHANGED) {
    
    mCanvasTM = nullptr;
  }

  nsSVGAFrameBase::NotifySVGChanged(aFlags);
}




gfxMatrix
nsSVGAFrame::GetCanvasTM(uint32_t aFor, nsIFrame* aTransformRoot)
{
  if (!(GetStateBits() & NS_FRAME_IS_NONDISPLAY) && !aTransformRoot) {
    if (aFor == FOR_PAINTING && NS_SVGDisplayListPaintingEnabled()) {
      return nsSVGIntegrationUtils::GetCSSPxToDevPxMatrix(this);
    }
  }
  if (!mCanvasTM) {
    NS_ASSERTION(GetParent(), "null parent");

    nsSVGContainerFrame *parent = static_cast<nsSVGContainerFrame*>(GetParent());
    dom::SVGAElement *content = static_cast<dom::SVGAElement*>(mContent);

    gfxMatrix tm = content->PrependLocalTransformsTo(
        this == aTransformRoot ? gfxMatrix() :
                                 parent->GetCanvasTM(aFor, aTransformRoot));

    mCanvasTM = new gfxMatrix(tm);
  }

  return *mCanvasTM;
}
