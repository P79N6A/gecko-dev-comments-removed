






#ifndef nsHTMLCanvasFrame_h___
#define nsHTMLCanvasFrame_h___

#include "nsContainerFrame.h"
#include "nsString.h"
#include "nsAString.h"
#include "nsIIOService.h"
#include "Layers.h"
#include "ImageLayers.h"

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
                  nsIFrame*   aPrevInFlow);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                     LayerManager* aManager,
                                     nsDisplayItem* aItem);

  
  nsIntSize GetCanvasSize();

  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsRenderingContext *aRenderingContext);
  virtual nsSize GetIntrinsicRatio();

  virtual nsSize ComputeSize(nsRenderingContext *aRenderingContext,
                             nsSize aCBSize, nscoord aAvailableWidth,
                             nsSize aMargin, nsSize aBorder, nsSize aPadding,
                             PRUint32 aFlags) MOZ_OVERRIDE;

  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);
  
  nsRect GetInnerArea() const;

#ifdef ACCESSIBILITY
  virtual already_AddRefed<Accessible> CreateAccessible();
#endif

  virtual nsIAtom* GetType() const;

  virtual bool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsSplittableFrame::IsFrameOfType(aFlags & ~(nsIFrame::eReplaced));
  }

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
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
