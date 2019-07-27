






#ifndef nsImageFrame_h___
#define nsImageFrame_h___

#include "nsSplittableFrame.h"
#include "nsIIOService.h"
#include "nsIObserver.h"

#include "imgINotificationObserver.h"

#include "nsDisplayList.h"
#include "imgIContainer.h"
#include "mozilla/Attributes.h"
#include "mozilla/DebugOnly.h"
#include "nsIReflowCallback.h"
#include "nsTObserverArray.h"

class nsImageMap;
class nsIURI;
class nsILoadGroup;
struct nsHTMLReflowState;
class nsHTMLReflowMetrics;
class nsDisplayImage;
class nsPresContext;
class nsImageFrame;
class nsTransform2D;
class nsImageLoadingContent;

namespace mozilla {
namespace layers {
  class ImageContainer;
  class ImageLayer;
  class LayerManager;
}
}

class nsImageListener : public imgINotificationObserver
{
protected:
  virtual ~nsImageListener();

public:
  explicit nsImageListener(nsImageFrame *aFrame);

  NS_DECL_ISUPPORTS
  NS_DECL_IMGINOTIFICATIONOBSERVER

  void SetFrame(nsImageFrame *frame) { mFrame = frame; }

private:
  nsImageFrame *mFrame;
};

typedef nsSplittableFrame ImageFrameSuper;

class nsImageFrame : public ImageFrameSuper,
                     public nsIReflowCallback {
public:
  typedef mozilla::layers::ImageContainer ImageContainer;
  typedef mozilla::layers::ImageLayer ImageLayer;
  typedef mozilla::layers::LayerManager LayerManager;

  NS_DECL_FRAMEARENA_HELPERS

  explicit nsImageFrame(nsStyleContext* aContext);

  NS_DECL_QUERYFRAME_TARGET(nsImageFrame)
  NS_DECL_QUERYFRAME

  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;
  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext) MOZ_OVERRIDE;

  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;
  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;
  virtual nscoord GetMinISize(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
  virtual nscoord GetPrefISize(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
  virtual mozilla::IntrinsicSize GetIntrinsicSize() MOZ_OVERRIDE;
  virtual nsSize GetIntrinsicRatio() MOZ_OVERRIDE;
  virtual void Reflow(nsPresContext*           aPresContext,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus) MOZ_OVERRIDE;
  
  virtual nsresult  GetContentForEvent(mozilla::WidgetEvent* aEvent,
                                       nsIContent** aContent) MOZ_OVERRIDE;
  virtual nsresult HandleEvent(nsPresContext* aPresContext,
                               mozilla::WidgetGUIEvent* aEvent,
                               nsEventStatus* aEventStatus) MOZ_OVERRIDE;
  virtual nsresult GetCursor(const nsPoint& aPoint,
                             nsIFrame::Cursor& aCursor) MOZ_OVERRIDE;
  virtual nsresult AttributeChanged(int32_t aNameSpaceID,
                                    nsIAtom* aAttribute,
                                    int32_t aModType) MOZ_OVERRIDE;

#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() MOZ_OVERRIDE;
#endif

  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE
  {
    return ImageFrameSuper::IsFrameOfType(aFlags & ~(nsIFrame::eReplaced));
  }

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
  void List(FILE* out = stderr, const char* aPrefix = "", 
            uint32_t aFlags = 0) const MOZ_OVERRIDE;
#endif

  virtual LogicalSides GetLogicalSkipSides(const nsHTMLReflowState* aReflowState = nullptr) const MOZ_OVERRIDE;

  nsresult GetIntrinsicImageSize(nsSize& aSize);

  static void ReleaseGlobals() {
    if (gIconLoad) {
      gIconLoad->Shutdown();
      NS_RELEASE(gIconLoad);
    }
    NS_IF_RELEASE(sIOService);
  }

  nsresult Notify(imgIRequest *aRequest, int32_t aType, const nsIntRect* aData);

  




  static bool ShouldCreateImageFrameFor(mozilla::dom::Element* aElement,
                                          nsStyleContext* aStyleContext);
  
  void DisplayAltFeedback(nsRenderingContext& aRenderingContext,
                          const nsRect&        aDirtyRect,
                          imgIRequest*         aRequest,
                          nsPoint              aPt);

  nsRect GetInnerArea() const;

  


  mozilla::dom::Element* GetMapElement() const;

  


  bool HasImageMap() const { return mImageMap || GetMapElement(); }

  nsImageMap* GetImageMap();
  nsImageMap* GetExistingImageMap() const { return mImageMap; }

  virtual void AddInlineMinISize(nsRenderingContext *aRenderingContext,
                                 InlineMinISizeData *aData) MOZ_OVERRIDE;

  void DisconnectMap();

  
  virtual bool ReflowFinished() MOZ_OVERRIDE;
  virtual void ReflowCallbackCanceled() MOZ_OVERRIDE;

protected:
  virtual ~nsImageFrame();

  void EnsureIntrinsicSizeAndRatio();

  virtual mozilla::LogicalSize
  ComputeSize(nsRenderingContext *aRenderingContext,
              mozilla::WritingMode aWritingMode,
              const mozilla::LogicalSize& aCBSize,
              nscoord aAvailableISize,
              const mozilla::LogicalSize& aMargin,
              const mozilla::LogicalSize& aBorder,
              const mozilla::LogicalSize& aPadding,
              uint32_t aFlags) MOZ_OVERRIDE;

  bool IsServerImageMap();

  void TranslateEventCoords(const nsPoint& aPoint,
                            nsIntPoint& aResult);

  bool GetAnchorHREFTargetAndNode(nsIURI** aHref, nsString& aTarget,
                                    nsIContent** aNode);
  








  nscoord MeasureString(const char16_t*     aString,
                        int32_t              aLength,
                        nscoord              aMaxWidth,
                        uint32_t&            aMaxFit,
                        nsRenderingContext& aContext);

  void DisplayAltText(nsPresContext*      aPresContext,
                      nsRenderingContext& aRenderingContext,
                      const nsString&      aAltText,
                      const nsRect&        aRect);

  void PaintImage(nsRenderingContext& aRenderingContext, nsPoint aPt,
                  const nsRect& aDirtyRect, imgIContainer* aImage,
                  uint32_t aFlags);

protected:
  friend class nsImageListener;
  friend class nsImageLoadingContent;
  nsresult OnStartContainer(imgIRequest *aRequest, imgIContainer *aImage);
  nsresult OnDataAvailable(imgIRequest *aRequest, const nsIntRect *rect);
  nsresult OnStopRequest(imgIRequest *aRequest,
                         nsresult aStatus);
  nsresult FrameChanged(imgIRequest *aRequest,
                        imgIContainer *aContainer);
  


  void NotifyNewCurrentRequest(imgIRequest *aRequest, nsresult aStatus);

private:
  
  inline void SpecToURI(const nsAString& aSpec, nsIIOService *aIOService,
                        nsIURI **aURI);

  inline void GetLoadGroup(nsPresContext *aPresContext,
                           nsILoadGroup **aLoadGroup);
  nscoord GetContinuationOffset() const;
  void GetDocumentCharacterSet(nsACString& aCharset) const;
  bool ShouldDisplaySelection();

  





  bool UpdateIntrinsicSize(imgIContainer* aImage);

  





  bool UpdateIntrinsicRatio(imgIContainer* aImage);

  








  bool GetSourceToDestTransform(nsTransform2D& aTransform);

  




  bool IsPendingLoad(imgIRequest* aRequest) const;
  bool IsPendingLoad(imgIContainer* aContainer) const;

  



  nsRect SourceRectToDest(const nsIntRect & aRect);

  nsImageMap*         mImageMap;

  nsCOMPtr<imgINotificationObserver> mListener;

  nsCOMPtr<imgIContainer> mImage;
  nsSize mComputedSize;
  mozilla::IntrinsicSize mIntrinsicSize;
  nsSize mIntrinsicRatio;

  bool mDisplayingIcon;
  bool mFirstFrameComplete;
  bool mReflowCallbackPosted;

  static nsIIOService* sIOService;
  
  

  
  
  

  
  
  nsresult LoadIcons(nsPresContext *aPresContext);
  nsresult LoadIcon(const nsAString& aSpec, nsPresContext *aPresContext,
                    imgRequestProxy **aRequest);

  class IconLoad MOZ_FINAL : public nsIObserver,
                             public imgINotificationObserver {
    
    
  public:
    IconLoad();

    void Shutdown();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
    NS_DECL_IMGINOTIFICATIONOBSERVER

    void AddIconObserver(nsImageFrame *frame) {
        NS_ABORT_IF_FALSE(!mIconObservers.Contains(frame),
                          "Observer shouldn't aleady be in array");
        mIconObservers.AppendElement(frame);
    }

    void RemoveIconObserver(nsImageFrame *frame) {
      mozilla::DebugOnly<bool> didRemove = mIconObservers.RemoveElement(frame);
      NS_ABORT_IF_FALSE(didRemove, "Observer not in array");
    }

  private:
    ~IconLoad() {}

    void GetPrefs();
    nsTObserverArray<nsImageFrame*> mIconObservers;


  public:
    nsRefPtr<imgRequestProxy> mLoadingImage;
    nsRefPtr<imgRequestProxy> mBrokenImage;
    bool             mPrefForceInlineAltText;
    bool             mPrefShowPlaceholders;
  };
  
public:
  static IconLoad* gIconLoad; 
  
  friend class nsDisplayImage;
};







class nsDisplayImage : public nsDisplayImageContainer {
public:
  typedef mozilla::layers::LayerManager LayerManager;

  nsDisplayImage(nsDisplayListBuilder* aBuilder, nsImageFrame* aFrame,
                 imgIContainer* aImage)
    : nsDisplayImageContainer(aBuilder, aFrame), mImage(aImage) {
    MOZ_COUNT_CTOR(nsDisplayImage);
  }
  virtual ~nsDisplayImage() {
    MOZ_COUNT_DTOR(nsDisplayImage);
  }
  virtual void ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                         const nsDisplayItemGeometry* aGeometry,
                                         nsRegion* aInvalidRegion) MOZ_OVERRIDE;
  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx) MOZ_OVERRIDE;

  



  virtual already_AddRefed<ImageContainer> GetContainer(LayerManager* aManager,
                                                        nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE;

  gfxRect GetDestRect();

  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   const ContainerLayerParameters& aParameters) MOZ_OVERRIDE;
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) MOZ_OVERRIDE
  {
    *aSnap = true;
    return nsRect(ToReferenceFrame(), Frame()->GetSize());
  }

  virtual already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                             LayerManager* aManager,
                                             const ContainerLayerParameters& aContainerParameters) MOZ_OVERRIDE;

  



  virtual void ConfigureLayer(ImageLayer* aLayer, const nsIntPoint& aOffset) MOZ_OVERRIDE;

  NS_DISPLAY_DECL_NAME("Image", TYPE_IMAGE)
private:
  nsCOMPtr<imgIContainer> mImage;
};

#endif 
