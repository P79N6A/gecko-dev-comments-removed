







#ifndef nsVideoFrame_h___
#define nsVideoFrame_h___

#include "mozilla/Attributes.h"
#include "nsContainerFrame.h"
#include "nsString.h"
#include "nsAString.h"
#include "nsIIOService.h"
#include "nsITimer.h"
#include "nsTArray.h"
#include "nsIAnonymousContentCreator.h"
#include "FrameLayerBuilder.h"

namespace mozilla {
namespace layers {
class Layer;
class LayerManager;
}
}

class nsPresContext;
class nsDisplayItem;

nsIFrame* NS_NewVideoFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);

class nsVideoFrame : public nsContainerFrame, public nsIAnonymousContentCreator
{
public:
  typedef mozilla::layers::Layer Layer;
  typedef mozilla::layers::LayerManager LayerManager;
  typedef mozilla::FrameLayerBuilder::ContainerParameters ContainerParameters;

  nsVideoFrame(nsStyleContext* aContext);

  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  NS_IMETHOD AttributeChanged(int32_t aNameSpaceID,
                              nsIAtom* aAttribute,
                              int32_t aModType);

  
  nsSize GetVideoIntrinsicSize(nsRenderingContext *aRenderingContext);
  virtual nsSize GetIntrinsicRatio();
  virtual nsSize ComputeSize(nsRenderingContext *aRenderingContext,
                             nsSize aCBSize, nscoord aAvailableWidth,
                             nsSize aMargin, nsSize aBorder, nsSize aPadding,
                             uint32_t aFlags) MOZ_OVERRIDE;
  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsRenderingContext *aRenderingContext);
  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;
  virtual bool IsLeaf() const MOZ_OVERRIDE;

  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() MOZ_OVERRIDE;
#endif

  virtual nsIAtom* GetType() const;

  virtual bool IsFrameOfType(uint32_t aFlags) const
  {
    return nsSplittableFrame::IsFrameOfType(aFlags & ~(nsIFrame::eReplaced));
  }
  
  virtual nsresult CreateAnonymousContent(nsTArray<ContentInfo>& aElements) MOZ_OVERRIDE;
  virtual void AppendAnonymousContentTo(nsBaseContentList& aElements,
                                        uint32_t aFilters) MOZ_OVERRIDE;

  nsIContent* GetPosterImage() { return mPosterImage; }

  
  
  bool ShouldDisplayPoster();

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

  already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                     LayerManager* aManager,
                                     nsDisplayItem* aItem,
                                     const ContainerParameters& aContainerParameters);

protected:

  
  
  bool HasVideoElement();

  
  
  
  bool HasVideoData();

  
  
  
  nsresult UpdatePosterSource(bool aNotify);

  virtual ~nsVideoFrame();

  nsMargin mBorderPadding;

  
  nsCOMPtr<nsIContent> mVideoControls;

  
  nsCOMPtr<nsIContent> mPosterImage;
};

#endif 
