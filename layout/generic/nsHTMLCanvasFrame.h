






#ifndef nsHTMLCanvasFrame_h___
#define nsHTMLCanvasFrame_h___

#include "mozilla/Attributes.h"
#include "nsContainerFrame.h"
#include "nsString.h"
#include "nsAString.h"
#include "nsIIOService.h"

namespace mozilla {
namespace layers {
class Layer;
class LayerManager;
}
}

class nsPresContext;
class nsDisplayItem;

nsIFrame* NS_NewHTMLCanvasFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);

class nsHTMLCanvasFrame : public nsContainerFrame
{
public:
  typedef mozilla::layers::Layer Layer;
  typedef mozilla::layers::LayerManager LayerManager;

  NS_DECL_FRAMEARENA_HELPERS

  nsHTMLCanvasFrame(nsStyleContext* aContext) : nsContainerFrame(aContext) {}

  NS_IMETHOD Init(nsIContent* aContent,
                  nsIFrame*   aParent,
                  nsIFrame*   aPrevInFlow) MOZ_OVERRIDE;

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                     LayerManager* aManager,
                                     nsDisplayItem* aItem);

  
  nsIntSize GetCanvasSize();

  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
  virtual nscoord GetPrefWidth(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
  virtual nsSize GetIntrinsicRatio() MOZ_OVERRIDE;

  virtual nsSize ComputeSize(nsRenderingContext *aRenderingContext,
                             nsSize aCBSize, nscoord aAvailableWidth,
                             nsSize aMargin, nsSize aBorder, nsSize aPadding,
                             uint32_t aFlags) MOZ_OVERRIDE;

  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus) MOZ_OVERRIDE;

  nsRect GetInnerArea() const;

#ifdef ACCESSIBILITY
  virtual already_AddRefed<Accessible> CreateAccessible() MOZ_OVERRIDE;
#endif

  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

  virtual bool IsFrameOfType(uint32_t aFlags) const
  {
    return nsSplittableFrame::IsFrameOfType(aFlags & ~(nsIFrame::eReplaced));
  }

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

  
  virtual nsIFrame* GetContentInsertionFrame() {
    return GetFirstPrincipalChild()->GetContentInsertionFrame();
  }

protected:
  virtual ~nsHTMLCanvasFrame();

  nscoord GetContinuationOffset(nscoord* aWidth = 0) const;

  nsMargin mBorderPadding;
};

#endif 
