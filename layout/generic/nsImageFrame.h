







































#ifndef nsImageFrame_h___
#define nsImageFrame_h___

#include "nsSplittableFrame.h"
#include "nsString.h"
#include "nsAString.h"
#include "nsPresContext.h"
#include "nsIImageFrame.h"
#include "nsIIOService.h"
#include "nsIObserver.h"

#include "nsTransform2D.h"
#include "imgIRequest.h"
#include "nsStubImageDecoderObserver.h"

class nsIFrame;
class nsImageMap;
class nsIURI;
class nsILoadGroup;
struct nsHTMLReflowState;
struct nsHTMLReflowMetrics;
struct nsSize;
class nsDisplayImage;

class nsImageFrame;

class nsImageListener : public nsStubImageDecoderObserver
{
public:
  nsImageListener(nsImageFrame *aFrame);
  virtual ~nsImageListener();

  NS_DECL_ISUPPORTS
  
  NS_IMETHOD OnStartContainer(imgIRequest *aRequest, imgIContainer *aImage);
  NS_IMETHOD OnDataAvailable(imgIRequest *aRequest, gfxIImageFrame *aFrame,
                             const nsRect *aRect);
  NS_IMETHOD OnStopDecode(imgIRequest *aRequest, nsresult status,
                          const PRUnichar *statusArg);
  
  NS_IMETHOD FrameChanged(imgIContainer *aContainer, gfxIImageFrame *newframe,
                          nsRect * dirtyRect);

  void SetFrame(nsImageFrame *frame) { mFrame = frame; }

private:
  nsImageFrame *mFrame;
};

#define IMAGE_SIZECONSTRAINED       0x00100000
#define IMAGE_GOTINITIALREFLOW      0x00200000

#define ImageFrameSuper nsSplittableFrame

class nsImageFrame : public ImageFrameSuper, public nsIImageFrame {
public:
  nsImageFrame(nsStyleContext* aContext);

  
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);

  virtual void Destroy();
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);
  virtual nscoord GetMinWidth(nsIRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsIRenderingContext *aRenderingContext);
  virtual nsSize GetIntrinsicRatio();
  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);
  
  NS_IMETHOD  GetContentForEvent(nsPresContext* aPresContext,
                                 nsEvent* aEvent,
                                 nsIContent** aContent);
  NS_IMETHOD HandleEvent(nsPresContext* aPresContext,
                        nsGUIEvent* aEvent,
                        nsEventStatus* aEventStatus);
  NS_IMETHOD GetCursor(const nsPoint& aPoint,
                       nsIFrame::Cursor& aCursor);
  NS_IMETHOD AttributeChanged(PRInt32 aNameSpaceID,
                              nsIAtom* aAttribute,
                              PRInt32 aModType);

#ifdef ACCESSIBILITY
  NS_IMETHOD GetAccessible(nsIAccessible** aAccessible);
#endif

  virtual nsIAtom* GetType() const;

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    return ImageFrameSuper::IsFrameOfType(aFlags & ~(nsIFrame::eReplaced));
  }

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
  NS_IMETHOD List(FILE* out, PRInt32 aIndent) const;
#endif

  virtual PRIntn GetSkipSides() const;

  NS_IMETHOD GetImageMap(nsPresContext *aPresContext, nsIImageMap **aImageMap);

  NS_IMETHOD GetIntrinsicImageSize(nsSize& aSize);

  static void ReleaseGlobals() {
    if (gIconLoad) {
      gIconLoad->Shutdown();
      NS_RELEASE(gIconLoad);
    }
    NS_IF_RELEASE(sIOService);
  }

  




  static PRBool ShouldCreateImageFrameFor(nsIContent* aContent,
                                          nsStyleContext* aStyleContext);
  
  void DisplayAltFeedback(nsIRenderingContext& aRenderingContext,
                          const nsRect&        aDirtyRect,
                          imgIRequest*         aRequest,
                          nsPoint              aPt);

  nsRect GetInnerArea() const;

  nsImageMap* GetImageMap(nsPresContext* aPresContext);

protected:
  
  NS_IMETHOD_(nsrefcnt) AddRef(void);
  NS_IMETHOD_(nsrefcnt) Release(void);

  virtual ~nsImageFrame();

  void EnsureIntrinsicSize(nsPresContext* aPresContext);

  virtual nsSize ComputeSize(nsIRenderingContext *aRenderingContext,
                             nsSize aCBSize, nscoord aAvailableWidth,
                             nsSize aMargin, nsSize aBorder, nsSize aPadding,
                             PRBool aShrinkWrap);

  void TriggerLink(nsPresContext* aPresContext,
                   nsIURI* aURI,
                   const nsString& aTargetSpec,
                   nsINode* aTriggerNode,
                   PRBool aClick);

  PRBool IsServerImageMap();

  void TranslateEventCoords(const nsPoint& aPoint,
                            nsIntPoint& aResult);

  PRBool GetAnchorHREFTargetAndNode(nsIURI** aHref, nsString& aTarget,
                                    nsINode** aNode);
  








  nscoord MeasureString(const PRUnichar*     aString,
                        PRInt32              aLength,
                        nscoord              aMaxWidth,
                        PRUint32&            aMaxFit,
                        nsIRenderingContext& aContext);

  void DisplayAltText(nsPresContext*      aPresContext,
                      nsIRenderingContext& aRenderingContext,
                      const nsString&      aAltText,
                      const nsRect&        aRect);

  void PaintImage(nsIRenderingContext& aRenderingContext, nsPoint aPt,
                  const nsRect& aDirtyRect, imgIContainer* aImage);
                  
protected:
  friend class nsImageListener;
  nsresult OnStartContainer(imgIRequest *aRequest, imgIContainer *aImage);
  nsresult OnDataAvailable(imgIRequest *aRequest,
                           gfxIImageFrame *aFrame,
                           const nsRect * rect);
  nsresult OnStopDecode(imgIRequest *aRequest,
                        nsresult aStatus,
                        const PRUnichar *aStatusArg);
  nsresult FrameChanged(imgIContainer *aContainer,
                        gfxIImageFrame *aNewframe,
                        nsRect *aDirtyRect);

private:
  
  inline void SpecToURI(const nsAString& aSpec, nsIIOService *aIOService,
                        nsIURI **aURI);

  inline void GetLoadGroup(nsPresContext *aPresContext,
                           nsILoadGroup **aLoadGroup);
  nscoord GetContinuationOffset() const;
  void GetDocumentCharacterSet(nsACString& aCharset) const;

  





  PRBool UpdateIntrinsicSize(imgIContainer* aImage);

  


  void RecalculateTransform();

  




  PRBool IsPendingLoad(imgIRequest* aRequest) const;
  PRBool IsPendingLoad(imgIContainer* aContainer) const;

  



  nsRect SourceRectToDest(const nsRect & aRect);

  nsImageMap*         mImageMap;

  nsCOMPtr<imgIDecoderObserver> mListener;

  nsSize mComputedSize;
  nsSize mIntrinsicSize;
  nsTransform2D mTransform;
  
  static nsIIOService* sIOService;

  

  
  
  

  
  
  nsresult LoadIcons(nsPresContext *aPresContext);
  nsresult LoadIcon(const nsAString& aSpec, nsPresContext *aPresContext,
                    imgIRequest **aRequest);
  
  
  
  
  PRBool HandleIconLoads(imgIRequest* aRequest, PRBool aCompleted);

  class IconLoad : public nsIObserver {
    
    
  public:
    IconLoad(imgIDecoderObserver* aObserver);

    void Shutdown()
    {
      
      if (mLoadingImage) {
        mLoadingImage->Cancel(NS_ERROR_FAILURE);
        mLoadingImage = nsnull;
      }
      if (mBrokenImage) {
        mBrokenImage->Cancel(NS_ERROR_FAILURE);
        mBrokenImage = nsnull;
      }
    }

    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER

  private:
    void GetPrefs();

  public:
    nsCOMPtr<imgIRequest> mLoadingImage;
    nsCOMPtr<imgIRequest> mBrokenImage;
    nsCOMPtr<imgIDecoderObserver> mLoadObserver; 
    PRUint8          mIconsLoaded;
    PRPackedBool     mPrefForceInlineAltText;
    PRPackedBool     mPrefShowPlaceholders;
  };
  
public:
  static IconLoad* gIconLoad; 
  
  friend class nsDisplayImage;
};

#endif 
