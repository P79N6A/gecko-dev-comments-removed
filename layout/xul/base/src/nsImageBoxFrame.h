



































#ifndef nsImageBoxFrame_h___
#define nsImageBoxFrame_h___

#include "nsLeafBoxFrame.h"

#include "imgILoader.h"
#include "imgIRequest.h"
#include "imgIContainer.h"
#include "nsStubImageDecoderObserver.h"

class nsImageBoxFrame;

class nsImageBoxListener : public nsStubImageDecoderObserver
{
public:
  nsImageBoxListener();
  virtual ~nsImageBoxListener();

  NS_DECL_ISUPPORTS
  
  NS_IMETHOD OnStartContainer(imgIRequest *request, imgIContainer *image);
  NS_IMETHOD OnStopContainer(imgIRequest *request, imgIContainer *image);
  NS_IMETHOD OnStopDecode(imgIRequest *request, nsresult status,
                          const PRUnichar *statusArg);
  
  NS_IMETHOD FrameChanged(imgIContainer *container, nsIntRect *dirtyRect);

  void SetFrame(nsImageBoxFrame *frame) { mFrame = frame; }

private:
  nsImageBoxFrame *mFrame;
};

class nsImageBoxFrame : public nsLeafBoxFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  
  virtual nsSize GetPrefSize(nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetMinSize(nsBoxLayoutState& aBoxLayoutState);
  virtual nscoord GetBoxAscent(nsBoxLayoutState& aBoxLayoutState);
  virtual void MarkIntrinsicWidthsDirty();

  friend nsIFrame* NS_NewImageBoxFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  

  NS_IMETHOD  Init(nsIContent*      aContent,
                   nsIFrame*        aParent,
                   nsIFrame*        asPrevInFlow);

  NS_IMETHOD AttributeChanged(PRInt32 aNameSpaceID,
                              nsIAtom* aAttribute,
                              PRInt32 aModType);

  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext);

  virtual void Destroy();

  virtual nsIAtom* GetType() const;
#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

  




  void UpdateImage();

  



  void UpdateLoadFlags();

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  NS_IMETHOD OnStartContainer(imgIRequest *request, imgIContainer *image);
  NS_IMETHOD OnStopContainer(imgIRequest *request, imgIContainer *image);
  NS_IMETHOD OnStopDecode(imgIRequest *request,
                          nsresult status,
                          const PRUnichar *statusArg);
  NS_IMETHOD FrameChanged(imgIContainer *container, nsIntRect *dirtyRect);

  virtual ~nsImageBoxFrame();

  void  PaintImage(nsIRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect,
                   nsPoint aPt);

protected:
  nsImageBoxFrame(nsIPresShell* aShell, nsStyleContext* aContext);

  virtual void GetImageSize();

private:

  nsRect mSubRect; 
  nsSize mIntrinsicSize;
  nsSize mImageSize;

  nsCOMPtr<imgIRequest> mImageRequest;
  nsCOMPtr<imgIDecoderObserver> mListener;

  PRInt32 mLoadFlags;

  PRPackedBool mUseSrcAttr; 
  PRPackedBool mSuppressStyleCheck;
}; 

#endif 
