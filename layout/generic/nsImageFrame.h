






#ifndef nsImageFrame_h___
#define nsImageFrame_h___

#include "nsSplittableFrame.h"
#include "nsIIOService.h"
#include "nsIObserver.h"

#include "nsStubImageDecoderObserver.h"
#include "imgIDecoderObserver.h"

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

namespace mozilla {
namespace layers {
  class ImageContainer;
  class ImageLayer;
  class LayerManager;
}
}

class nsImageListener : public nsStubImageDecoderObserver
{
public:
  nsImageListener(nsImageFrame *aFrame);
  virtual ~nsImageListener();

  NS_DECL_ISUPPORTS
  
  NS_IMETHOD OnStartContainer(imgIRequest *aRequest, imgIContainer *aImage);
  NS_IMETHOD OnDataAvailable(imgIRequest *aRequest, bool aCurrentFrame,
                             const nsIntRect *aRect);
  NS_IMETHOD OnStopDecode(imgIRequest *aRequest, nsresult status,
                          const PRUnichar *statusArg);
  
  NS_IMETHOD FrameChanged(imgIRequest *aRequest,
                          imgIContainer *aContainer,
                          const nsIntRect *dirtyRect);

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
  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);
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
  virtual already_AddRefed<Accessible> CreateAccessible();
#endif

  virtual nsIAtom* GetType() const;

  virtual bool IsFrameOfType(uint32_t aFlags) const
  {
    return ImageFrameSuper::IsFrameOfType(aFlags & ~(nsIFrame::eReplaced));
  }

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
  NS_IMETHOD List(FILE* out, int32_t aIndent) const;
#endif

  virtual int GetSkipSides() const;

  nsresult GetIntrinsicImageSize(nsSize& aSize);

  static void ReleaseGlobals() {
    if (gIconLoad) {
      gIconLoad->Shutdown();
      NS_RELEASE(gIconLoad);
    }
    NS_IF_RELEASE(sIOService);
  }

  




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
  nsresult OnDataAvailable(imgIRequest *aRequest, bool aCurrentFrame,
                           const nsIntRect *rect);
  nsresult OnStopDecode(imgIRequest *aRequest,
                        nsresult aStatus,
                        const PRUnichar *aStatusArg);
  nsresult FrameChanged(imgIRequest *aRequest,
                        imgIContainer *aContainer,
                        const nsIntRect *aDirtyRect);
  


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

  nsCOMPtr<imgIDecoderObserver> mListener;

  nsSize mComputedSize;
  nsIFrame::IntrinsicSize mIntrinsicSize;
  nsSize mIntrinsicRatio;

  bool mDisplayingIcon;

  static nsIIOService* sIOService;
  
  

  
  
  

  
  
  nsresult LoadIcons(nsPresContext *aPresContext);
  nsresult LoadIcon(const nsAString& aSpec, nsPresContext *aPresContext,
                    imgIRequest **aRequest);

  class IconLoad MOZ_FINAL : public nsIObserver,
                             public imgIDecoderObserver {
    
    
  public:
    IconLoad();

    void Shutdown();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
    NS_DECL_IMGICONTAINEROBSERVER
    NS_DECL_IMGIDECODEROBSERVER

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
    nsCOMPtr<imgIRequest> mLoadingImage;
    nsCOMPtr<imgIRequest> mBrokenImage;
    bool             mPrefForceInlineAltText;
    bool             mPrefShowPlaceholders;
  };
  
public:
  static IconLoad* gIconLoad; 
  
  friend class nsDisplayImage;
};







class nsDisplayImage : public nsDisplayItem {
public:
  typedef mozilla::layers::ImageContainer ImageContainer;
  typedef mozilla::layers::ImageLayer ImageLayer;
  typedef mozilla::layers::LayerManager LayerManager;

  nsDisplayImage(nsDisplayListBuilder* aBuilder, nsImageFrame* aFrame,
                 imgIContainer* aImage)
    : nsDisplayItem(aBuilder, aFrame), mImage(aImage) {
    MOZ_COUNT_CTOR(nsDisplayImage);
  }
  virtual ~nsDisplayImage() {
    MOZ_COUNT_DTOR(nsDisplayImage);
  }
  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx) MOZ_OVERRIDE;

  



  already_AddRefed<ImageContainer> GetContainer();

  gfxRect GetDestRect();

  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   const ContainerParameters& aParameters) MOZ_OVERRIDE;

  virtual already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                             LayerManager* aManager,
                                             const ContainerParameters& aContainerParameters) MOZ_OVERRIDE;

  



  void ConfigureLayer(ImageLayer* aLayer);

  NS_DISPLAY_DECL_NAME("Image", TYPE_IMAGE)
private:
  nsCOMPtr<imgIContainer> mImage;
};

#endif 
