







#ifndef nsVideoFrame_h___
#define nsVideoFrame_h___

#include "mozilla/Attributes.h"
#include "nsContainerFrame.h"
#include "nsIAnonymousContentCreator.h"
#include "nsTArrayForwardDeclare.h"
#include "FrameLayerBuilder.h"

namespace mozilla {
namespace layers {
class Layer;
class LayerManager;
} 
} 

class nsAString;
class nsPresContext;
class nsDisplayItem;

class nsVideoFrame : public nsContainerFrame, public nsIAnonymousContentCreator
{
public:
  typedef mozilla::layers::Layer Layer;
  typedef mozilla::layers::LayerManager LayerManager;
  typedef mozilla::ContainerLayerParameters ContainerLayerParameters;

  explicit nsVideoFrame(nsStyleContext* aContext);

  NS_DECL_QUERYFRAME
  NS_DECL_QUERYFRAME_TARGET(nsVideoFrame)
  NS_DECL_FRAMEARENA_HELPERS

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) override;

  virtual nsresult AttributeChanged(int32_t aNameSpaceID,
                                    nsIAtom* aAttribute,
                                    int32_t aModType) override;

  
  nsSize GetVideoIntrinsicSize(nsRenderingContext *aRenderingContext);
  virtual nsSize GetIntrinsicRatio() override;
  virtual mozilla::LogicalSize
  ComputeSize(nsRenderingContext *aRenderingContext,
              mozilla::WritingMode aWritingMode,
              const mozilla::LogicalSize& aCBSize,
              nscoord aAvailableISize,
              const mozilla::LogicalSize& aMargin,
              const mozilla::LogicalSize& aBorder,
              const mozilla::LogicalSize& aPadding,
              ComputeSizeFlags aFlags) override;
  virtual nscoord GetMinISize(nsRenderingContext *aRenderingContext) override;
  virtual nscoord GetPrefISize(nsRenderingContext *aRenderingContext) override;
  virtual void DestroyFrom(nsIFrame* aDestructRoot) override;
  virtual bool IsLeaf() const override;

  virtual void Reflow(nsPresContext*           aPresContext,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus) override;

#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() override;
#endif

  virtual nsIAtom* GetType() const override;

  virtual bool IsFrameOfType(uint32_t aFlags) const override
  {
    return nsSplittableFrame::IsFrameOfType(aFlags & ~(nsIFrame::eReplaced));
  }
  
  virtual nsresult CreateAnonymousContent(nsTArray<ContentInfo>& aElements) override;
  virtual void AppendAnonymousContentTo(nsTArray<nsIContent*>& aElements,
                                        uint32_t aFilters) override;

  nsIContent* GetPosterImage() { return mPosterImage; }

  
  
  bool ShouldDisplayPoster();

  nsIContent *GetCaptionOverlay() { return mCaptionDiv; }

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override;
#endif

  already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                     LayerManager* aManager,
                                     nsDisplayItem* aItem,
                                     const ContainerLayerParameters& aContainerParameters);

protected:

  
  
  bool HasVideoElement();

  
  
  
  bool HasVideoData();

  
  
  
  void UpdatePosterSource(bool aNotify);

  virtual ~nsVideoFrame();

  nsMargin mBorderPadding;

  
  nsCOMPtr<nsIContent> mVideoControls;

  
  nsCOMPtr<nsIContent> mPosterImage;

  
  nsCOMPtr<nsIContent> mCaptionDiv;

};

#endif 
