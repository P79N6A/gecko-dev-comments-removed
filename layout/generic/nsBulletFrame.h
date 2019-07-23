






































#ifndef nsBulletFrame_h___
#define nsBulletFrame_h___

#include "nsFrame.h"
#include "nsStyleContext.h"

#include "imgIRequest.h"
#include "imgIDecoderObserver.h"





class nsBulletFrame : public nsFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  nsBulletFrame(nsStyleContext* aContext) : nsFrame(aContext) {}
  virtual ~nsBulletFrame();

  
  virtual void Destroy();
  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);
  virtual nsIAtom* GetType() const;
  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext);
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
                             PRBool aCurrentFrame,
                             const nsIntRect *aRect);
  NS_IMETHOD OnStopDecode(imgIRequest *aRequest,
                          nsresult aStatus,
                          const PRUnichar *aStatusArg);
  NS_IMETHOD FrameChanged(imgIContainer *aContainer,
                          nsIntRect *aDirtyRect);

  
  static PRBool AppendCounterText(PRInt32 aListStyleType,
                                  PRInt32 aOrdinal,
                                  nsString& aResult);

  
  PRBool GetListItemText(const nsStyleList& aStyleList,
                         nsString& aResult);
                         
  void PaintBullet(nsIRenderingContext& aRenderingContext, nsPoint aPt,
                   const nsRect& aDirtyRect);
  
  virtual PRBool IsEmpty();
  virtual PRBool IsSelfEmpty();

protected:
  void GetDesiredSize(nsPresContext* aPresContext,
                      nsIRenderingContext *aRenderingContext,
                      nsHTMLReflowMetrics& aMetrics);

  void GetLoadGroup(nsPresContext *aPresContext, nsILoadGroup **aLoadGroup);

  nsMargin mPadding;
  nsCOMPtr<imgIRequest> mImageRequest;
  nsCOMPtr<imgIDecoderObserver> mListener;

  nsSize mIntrinsicSize;
  nsSize mComputedSize;
  PRInt32 mOrdinal;
  PRBool mTextIsRTL;
};

#endif 
