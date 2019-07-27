




#ifndef __NS_SVGINNERSVGFRAME_H__
#define __NS_SVGINNERSVGFRAME_H__

#include "mozilla/Attributes.h"
#include "nsSVGContainerFrame.h"
#include "nsISVGSVGFrame.h"

class nsRenderingContext;

typedef nsSVGDisplayContainerFrame nsSVGInnerSVGFrameBase;

class nsSVGInnerSVGFrame : public nsSVGInnerSVGFrameBase,
                           public nsISVGSVGFrame
{
  friend nsIFrame*
  NS_NewSVGInnerSVGFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  explicit nsSVGInnerSVGFrame(nsStyleContext* aContext) :
    nsSVGInnerSVGFrameBase(aContext) {}
  
public:
  NS_DECL_QUERYFRAME_TARGET(nsSVGInnerSVGFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

#ifdef DEBUG
  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;
#endif

  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGInnerSVG"), aResult);
  }
#endif

  virtual nsresult  AttributeChanged(int32_t         aNameSpaceID,
                                     nsIAtom*        aAttribute,
                                     int32_t         aModType) MOZ_OVERRIDE;

  
  virtual nsresult PaintSVG(nsRenderingContext *aContext,
                            const nsIntRect *aDirtyRect,
                            nsIFrame* aTransformRoot = nullptr) MOZ_OVERRIDE;
  virtual nsRect GetCoveredRegion() MOZ_OVERRIDE;
  virtual void ReflowSVG() MOZ_OVERRIDE;
  virtual void NotifySVGChanged(uint32_t aFlags) MOZ_OVERRIDE;
  virtual nsIFrame* GetFrameForPoint(const gfxPoint& aPoint) MOZ_OVERRIDE;

  
  virtual gfxMatrix GetCanvasTM(uint32_t aFor,
                                nsIFrame* aTransformRoot = nullptr) MOZ_OVERRIDE;

  virtual bool HasChildrenOnlyTransform(Matrix *aTransform) const MOZ_OVERRIDE;

  
  virtual void NotifyViewportOrTransformChanged(uint32_t aFlags) MOZ_OVERRIDE;

protected:

  nsAutoPtr<gfxMatrix> mCanvasTM;
};

#endif

