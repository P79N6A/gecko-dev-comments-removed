




#ifndef NSSVGFOREIGNOBJECTFRAME_H__
#define NSSVGFOREIGNOBJECTFRAME_H__

#include "mozilla/Attributes.h"
#include "nsContainerFrame.h"
#include "nsIPresShell.h"
#include "nsISVGChildFrame.h"
#include "nsRegion.h"
#include "nsSVGUtils.h"

class nsRenderingContext;
class nsSVGOuterSVGFrame;

typedef nsContainerFrame nsSVGForeignObjectFrameBase;

class nsSVGForeignObjectFrame : public nsSVGForeignObjectFrameBase,
                                public nsISVGChildFrame
{
  friend nsContainerFrame*
  NS_NewSVGForeignObjectFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  explicit nsSVGForeignObjectFrame(nsStyleContext* aContext);

public:
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;
  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;
  virtual nsresult  AttributeChanged(int32_t         aNameSpaceID,
                                     nsIAtom*        aAttribute,
                                     int32_t         aModType) MOZ_OVERRIDE;

  virtual nsContainerFrame* GetContentInsertionFrame() MOZ_OVERRIDE {
    return GetFirstPrincipalChild()->GetContentInsertionFrame();
  }

  virtual void Reflow(nsPresContext*           aPresContext,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus) MOZ_OVERRIDE;

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE
  {
    return nsSVGForeignObjectFrameBase::IsFrameOfType(aFlags &
      ~(nsIFrame::eSVG | nsIFrame::eSVGForeignObject));
  }

  virtual bool IsSVGTransformed(Matrix *aOwnTransform,
                                Matrix *aFromParentTransform) const MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGForeignObject"), aResult);
  }
#endif

  
  virtual nsresult PaintSVG(nsRenderingContext *aContext,
                            const gfxMatrix& aTransform,
                            const nsIntRect* aDirtyRect = nullptr) MOZ_OVERRIDE;
  virtual nsIFrame* GetFrameForPoint(const gfxPoint& aPoint) MOZ_OVERRIDE;
  virtual nsRect GetCoveredRegion() MOZ_OVERRIDE;
  virtual void ReflowSVG() MOZ_OVERRIDE;
  virtual void NotifySVGChanged(uint32_t aFlags) MOZ_OVERRIDE;
  virtual SVGBBox GetBBoxContribution(const Matrix &aToBBoxUserspace,
                                      uint32_t aFlags) MOZ_OVERRIDE;
  virtual bool IsDisplayContainer() MOZ_OVERRIDE { return true; }

  gfxMatrix GetCanvasTM();

  nsRect GetInvalidRegion();

protected:
  
  void DoReflow();
  void RequestReflow(nsIPresShell::IntrinsicDirty aType);

  
  bool IsDisabled() const { return mRect.width <= 0 || mRect.height <= 0; }

  nsAutoPtr<gfxMatrix> mCanvasTM;

  bool mInReflow;
};

#endif
