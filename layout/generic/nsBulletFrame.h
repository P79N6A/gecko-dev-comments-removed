






































#ifndef nsBulletFrame_h___
#define nsBulletFrame_h___

#include "nsFrame.h"
#include "nsStyleContext.h"

#include "imgIRequest.h"
#include "imgIDecoderObserver.h"
class gfxIImageFrame;





class nsBulletFrame : public nsFrame {
public:
  nsBulletFrame(nsStyleContext* aContext) : nsFrame(aContext) {}
  virtual ~nsBulletFrame();

  
  virtual void Destroy();
  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);
  virtual nsIAtom* GetType() const;
  NS_IMETHOD DidSetStyleContext();
#ifdef NS_DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

  
  NS_IMETHOD Reflow(nsPresContext* aPresContext,
                    nsHTMLReflowMetrics& aMetrics,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus& aStatus);
  virtual nscoord GetMinWidth(nsIRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsIRenderingContext *aRenderingContext);

  
  PRInt32 SetListItemOrdinal(PRInt32 aNextOrdinal, PRBool* aChanged);


  NS_IMETHOD OnStartContainer(imgIRequest *aRequest, imgIContainer *aImage);
  NS_IMETHOD OnDataAvailable(imgIRequest *aRequest,
                             gfxIImageFrame *aFrame,
                             const nsRect * rect);
  NS_IMETHOD OnStopDecode(imgIRequest *aRequest,
                          nsresult aStatus,
                          const PRUnichar *aStatusArg);
  NS_IMETHOD FrameChanged(imgIContainer *aContainer,
                          gfxIImageFrame *aNewframe,
                          nsRect *aDirtyRect);

  
  static PRBool AppendCounterText(PRInt32 aListStyleType,
                                  PRInt32 aOrdinal,
                                  nsString& aResult);

  
  PRBool GetListItemText(const nsStyleList& aStyleList,
                         nsString& aResult);
                         
  void PaintBullet(nsIRenderingContext& aRenderingContext, nsPoint aPt,
                   const nsRect& aDirtyRect);

protected:
  void GetDesiredSize(nsPresContext* aPresContext,
                      nsIRenderingContext *aRenderingContext,
                      nsHTMLReflowMetrics& aMetrics);

  void GetLoadGroup(nsPresContext *aPresContext, nsILoadGroup **aLoadGroup);

  PRInt32 mOrdinal;
  nsMargin mPadding;
  nsCOMPtr<imgIRequest> mImageRequest;
  nsCOMPtr<imgIDecoderObserver> mListener;

  nsSize mIntrinsicSize;
  nsSize mComputedSize;
};

#endif 
