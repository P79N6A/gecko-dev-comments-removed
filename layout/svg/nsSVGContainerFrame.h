




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
  nsSVGContainerFrame(nsStyleContext* aContext)
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

  






  virtual bool HasChildrenOnlyTransform(mozilla::gfx::Matrix *aTransform) const {
    return false;
  }

  
  NS_IMETHOD AppendFrames(ChildListID     aListID,
                          nsFrameList&    aFrameList) MOZ_OVERRIDE;
  NS_IMETHOD InsertFrames(ChildListID     aListID,
                          nsIFrame*       aPrevFrame,
                          nsFrameList&    aFrameList) MOZ_OVERRIDE;
  NS_IMETHOD RemoveFrame(ChildListID     aListID,
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
  nsSVGDisplayContainerFrame(nsStyleContext* aContext)
    : nsSVGContainerFrame(aContext)
  {
     AddStateBits(NS_FRAME_MAY_BE_TRANSFORMED);
  }

public:
  NS_DECL_QUERYFRAME_TARGET(nsSVGDisplayContainerFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  NS_IMETHOD InsertFrames(ChildListID     aListID,
                          nsIFrame*       aPrevFrame,
                          nsFrameList&    aFrameList) MOZ_OVERRIDE;
  NS_IMETHOD RemoveFrame(ChildListID     aListID,
                         nsIFrame*       aOldFrame) MOZ_OVERRIDE;
 virtual void Init(nsIContent*      aContent,
                   nsIFrame*        aParent,
                   nsIFrame*        aPrevInFlow) MOZ_OVERRIDE;

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  virtual bool IsSVGTransformed(mozilla::gfx::Matrix *aOwnTransform = nullptr,
                                mozilla::gfx::Matrix *aFromParentTransform = nullptr) const MOZ_OVERRIDE;

  
  NS_IMETHOD PaintSVG(nsRenderingContext* aContext,
                      const nsIntRect *aDirtyRect,
                      nsIFrame* aTransformRoot = nullptr) MOZ_OVERRIDE;
  NS_IMETHOD_(nsIFrame*) GetFrameForPoint(const nsPoint &aPoint) MOZ_OVERRIDE;
  NS_IMETHOD_(nsRect) GetCoveredRegion() MOZ_OVERRIDE;
  virtual void ReflowSVG() MOZ_OVERRIDE;
  virtual void NotifySVGChanged(uint32_t aFlags) MOZ_OVERRIDE;
  virtual SVGBBox GetBBoxContribution(const mozilla::gfx::Matrix &aToBBoxUserspace,
                                      uint32_t aFlags) MOZ_OVERRIDE;
  NS_IMETHOD_(bool) IsDisplayContainer() MOZ_OVERRIDE { return true; }
};

#endif
