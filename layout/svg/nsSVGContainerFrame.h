




#ifndef NS_SVGCONTAINERFRAME_H
#define NS_SVGCONTAINERFRAME_H

#include "mozilla/Attributes.h"
#include "nsContainerFrame.h"
#include "nsFrame.h"
#include "nsIFrame.h"
#include "nsISVGChildFrame.h"
#include "nsQueryFrame.h"
#include "nsRect.h"
#include "nsSVGUtils.h"

class nsFrameList;
class nsIContent;
class nsIPresShell;
class nsRenderingContext;
class nsStyleContext;

struct nsPoint;
struct nsRect;
struct nsIntRect;

typedef nsContainerFrame nsSVGContainerFrameBase;













class nsSVGContainerFrame : public nsSVGContainerFrameBase
{
  friend nsIFrame* NS_NewSVGContainerFrame(nsIPresShell* aPresShell,
                                           nsStyleContext* aContext);
protected:
  explicit nsSVGContainerFrame(nsStyleContext* aContext)
    : nsSVGContainerFrameBase(aContext)
  {
    AddStateBits(NS_FRAME_SVG_LAYOUT);
  }

public:
  NS_DECL_QUERYFRAME_TARGET(nsSVGContainerFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  virtual gfxMatrix GetCanvasTM(uint32_t aFor,
                                nsIFrame* aTransformRoot = nullptr) {
    return gfxMatrix();
  }

  






  virtual bool HasChildrenOnlyTransform(Matrix *aTransform) const {
    return false;
  }

  
  virtual void AppendFrames(ChildListID     aListID,
                            nsFrameList&    aFrameList) MOZ_OVERRIDE;
  virtual void InsertFrames(ChildListID     aListID,
                            nsIFrame*       aPrevFrame,
                            nsFrameList&    aFrameList) MOZ_OVERRIDE;
  virtual void RemoveFrame(ChildListID     aListID,
                           nsIFrame*       aOldFrame) MOZ_OVERRIDE;

  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE
  {
    return nsSVGContainerFrameBase::IsFrameOfType(
            aFlags & ~(nsIFrame::eSVG | nsIFrame::eSVGContainer));
  }

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE {}

  virtual bool UpdateOverflow() MOZ_OVERRIDE;

protected:
  



  static void ReflowSVGNonDisplayText(nsIFrame* aContainer);
};












class nsSVGDisplayContainerFrame : public nsSVGContainerFrame,
                                   public nsISVGChildFrame
{
protected:
  explicit nsSVGDisplayContainerFrame(nsStyleContext* aContext)
    : nsSVGContainerFrame(aContext)
  {
     AddStateBits(NS_FRAME_MAY_BE_TRANSFORMED);
  }

public:
  NS_DECL_QUERYFRAME_TARGET(nsSVGDisplayContainerFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  virtual void InsertFrames(ChildListID     aListID,
                                nsIFrame*       aPrevFrame,
                                nsFrameList&    aFrameList) MOZ_OVERRIDE;
  virtual void RemoveFrame(ChildListID     aListID,
                               nsIFrame*       aOldFrame) MOZ_OVERRIDE;
 virtual void Init(nsIContent*       aContent,
                   nsContainerFrame* aParent,
                   nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  virtual bool IsSVGTransformed(Matrix *aOwnTransform = nullptr,
                                Matrix *aFromParentTransform = nullptr) const MOZ_OVERRIDE;

  
  virtual nsresult PaintSVG(nsRenderingContext* aContext,
                            const nsIntRect *aDirtyRect,
                            nsIFrame* aTransformRoot = nullptr) MOZ_OVERRIDE;
  virtual nsIFrame* GetFrameForPoint(const gfxPoint& aPoint) MOZ_OVERRIDE;
  virtual nsRect GetCoveredRegion() MOZ_OVERRIDE;
  virtual void ReflowSVG() MOZ_OVERRIDE;
  virtual void NotifySVGChanged(uint32_t aFlags) MOZ_OVERRIDE;
  virtual SVGBBox GetBBoxContribution(const Matrix &aToBBoxUserspace,
                                      uint32_t aFlags) MOZ_OVERRIDE;
  virtual bool IsDisplayContainer() MOZ_OVERRIDE { return true; }
};

#endif
