






#ifndef nsImageFrame_h___
#define nsImageFrame_h___

#include "nsSplittableFrame.h"
#include "nsIIOService.h"
#include "nsIObserver.h"

#include "imgINotificationObserver.h"

#include "nsDisplayList.h"
#include "imgIContainer.h"
#include "mozilla/Attributes.h"

class nsIFrame;
class nsImageMap;
class nsIURI;
class nsILoadGroup;
struct nsHTMLReflowState;
struct nsHTMLReflowMetrics;
struct nsSize;
class nsDisplayImage;
class nsPresContext;
class nsImageFrame;
class nsTransform2D;
class nsImageLoadingContent;
class imgRequestProxy;

namespace mozilla {
namespace layers {
  class ImageContainer;
  class ImageLayer;
  class LayerManager;
}
}

class nsImageListener : public imgINotificationObserver
{
public:
  nsImageListener(nsImageFrame *aFrame);
  virtual ~nsImageListener();

  NS_DECL_ISUPPORTS
  NS_DECL_IMGINOTIFICATIONOBSERVER

  void SetFrame(nsImageFrame *frame) { mFrame = frame; }

private:
  nsImageFrame *mFrame;
};

#define IMAGE_SIZECONSTRAINED       NS_FRAME_STATE_BIT(20)
#define IMAGE_GOTINITIALREFLOW      NS_FRAME_STATE_BIT(21)

#define ImageFrameSuper nsSplittableFrame

class nsImageFrame : public ImageFrameSuper {
public:
  typedef mozilla::layers::ImageContainer ImageContainer;
  typedef mozilla::layers::ImageLayer ImageLayer;
  typedef mozilla::layers::LayerManager LayerManager;

  NS_DECL_FRAMEARENA_HELPERS

  nsImageFrame(nsStyleContext* aContext);

  NS_DECL_QUERYFRAME_TARGET(nsImageFrame)
  NS_DECL_QUERYFRAME

  virtual void DestroyFrom(nsIFrame* aDestructRoot);
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;
  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsRenderingContext *aRenderingContext);
  virtual IntrinsicSize GetIntrinsicSize();
  virtual nsSize GetIntrinsicRatio();
  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);
  
  NS_IMETHOD  GetContentForEvent(nsEvent* aEvent,
                                 nsIContent** aContent);
  NS_IMETHOD HandleEvent(nsPresContext* aPresContext,
                        nsGUIEvent* aEvent,
                        nsEventStatus* aEventStatus);
  NS_IMETHOD GetCursor(const nsPoint& aPoint,
                       nsIFrame::Cursor& aCursor);
  NS_IMETHOD AttributeChanged(int32_t aNameSpaceID,
                              nsIAtom* aAttribute,
                              int32_t aModType);

#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() MOZ_OVERRIDE;
#endif

  virtual nsIAtom* GetType() const;

  virtual bool IsFrameOfType(uint32_t aFlags) const
  {
    return ImageFrameSuper::IsFrameOfType(aFlags & ~(nsIFrame::eReplaced));
  }

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
  NS_IMETHOD List(FILE* out, int32_t aIndent, uint32_t aFlags = 0) const;
#endif

  virtual int GetSkipSides() const MOZ_OVERRIDE;

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

  


  mozilla::dom::Element* GetMapElement() const
  {
    nsAutoString usemap;
    if (mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::usemap, usemap)) {
      return mContent->OwnerDoc()->FindImageMap(usemap);
    }
    return nullptr;
  }

  


  bool HasImageMap() const { return mImageMap || GetMapElement(); }

  nsImageMap* GetImageMap();
  nsImageMap* GetExistingImageMap() const { return mImageMap; }

  virtual void AddInlineMinWidth(nsRenderingContext *aRenderingContext,
                                 InlineMinWidthData *aData);

  void DisconnectMap();
protected:
  virtual ~nsImageFrame();

  void EnsureIntrinsicSizeAndRatio(nsPresContext* aPresContext);

  virtual nsSize ComputeSize(nsRenderingContext *aRenderingContext,
                             nsSize aCBSize, nscoord aAvailableWidth,
                             nsSize aMargin, nsSize aBorder, nsSize aPadding,
                             uint32_t aFlags) MOZ_OVERRIDE;

  bool IsServerImageMap();

  void TranslateEventCoords(const nsPoint& aPoint,
                            nsIntPoint& aResult);

  bool GetAnchorHREFTargetAndNode(nsIURI** aHref, nsString& aTarget,
                                    nsIContent** aNode);
  








  nscoord MeasureString(const PRUnichar*     aString,
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

  nsSize mComputedSize;
  nsIFrame::IntrinsicSize mIntrinsicSize;
  nsSize mIntrinsicRatio;

  bool mDisplayingIcon;
  bool mFirstFrameComplete;

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
#ifdef DEBUG
        bool rv =
#endif
            mIconObservers.RemoveElement(frame);
        NS_ABORT_IF_FALSE(rv, "Observer not in array");
    }

  private:
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
  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx) MOZ_OVERRIDE;

  



  virtual already_AddRefed<ImageContainer> GetContainer(LayerManager* aManager,
                                                        nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE;

  gfxRect GetDestRect();

  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   const ContainerParameters& aParameters) MOZ_OVERRIDE;
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) MOZ_OVERRIDE
  {
    *aSnap = true;
    return nsRect(ToReferenceFrame(), GetUnderlyingFrame()->GetSize());
  }

  virtual already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                             LayerManager* aManager,
                                             const ContainerParameters& aContainerParameters) MOZ_OVERRIDE;

  



  virtual void ConfigureLayer(ImageLayer* aLayer, const nsIntPoint& aOffset) MOZ_OVERRIDE;

  NS_DISPLAY_DECL_NAME("Image", TYPE_IMAGE)
private:
  nsCOMPtr<imgIContainer> mImage;
};

#endif 
