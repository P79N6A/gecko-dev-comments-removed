




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

class gfxContext;
class nsFrameList;
class nsIContent;
class nsIPresShell;
class nsStyleContext;

struct nsPoint;
struct nsRect;

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

  
  virtual gfxMatrix GetCanvasTM() {
    return gfxMatrix();
  }

  






  virtual bool HasChildrenOnlyTransform(Matrix *aTransform) const {
    return false;
  }

  
  virtual void AppendFrames(ChildListID     aListID,
                            nsFrameList&    aFrameList) override;
  virtual void InsertFrames(ChildListID     aListID,
                            nsIFrame*       aPrevFrame,
                            nsFrameList&    aFrameList) override;
  virtual void RemoveFrame(ChildListID     aListID,
                           nsIFrame*       aOldFrame) override;

  virtual bool IsFrameOfType(uint32_t aFlags) const override
  {
    return nsSVGContainerFrameBase::IsFrameOfType(
            aFlags & ~(nsIFrame::eSVG | nsIFrame::eSVGContainer));
  }

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) override {}

  virtual bool UpdateOverflow() override;

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
                                nsFrameList&    aFrameList) override;
  virtual void RemoveFrame(ChildListID     aListID,
                               nsIFrame*       aOldFrame) override;
 virtual void Init(nsIContent*       aContent,
                   nsContainerFrame* aParent,
                   nsIFrame*         aPrevInFlow) override;

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) override;

  virtual bool IsSVGTransformed(Matrix *aOwnTransform = nullptr,
                                Matrix *aFromParentTransform = nullptr) const override;

  
  virtual nsresult PaintSVG(gfxContext& aContext,
                            const gfxMatrix& aTransform,
                            const nsIntRect *aDirtyRect = nullptr) override;
  virtual nsIFrame* GetFrameForPoint(const gfxPoint& aPoint) override;
  virtual nsRect GetCoveredRegion() override;
  virtual void ReflowSVG() override;
  virtual void NotifySVGChanged(uint32_t aFlags) override;
  virtual SVGBBox GetBBoxContribution(const Matrix &aToBBoxUserspace,
                                      uint32_t aFlags) override;
  virtual bool IsDisplayContainer() override { return true; }
};

#endif
