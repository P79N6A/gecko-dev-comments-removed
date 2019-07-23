







































#ifndef nsVideoFrame_h___
#define nsVideoFrame_h___

#include "nsContainerFrame.h"
#include "nsString.h"
#include "nsAString.h"
#include "nsPresContext.h"
#include "nsIIOService.h"
#include "nsITimer.h"
#include "nsTArray.h"
#include "nsIAnonymousContentCreator.h"

nsIFrame* NS_NewVideoFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);

class nsVideoFrame : public nsContainerFrame, public nsIAnonymousContentCreator
{
public:
  using nsFrame::GetIntrinsicSize;

  nsVideoFrame(nsStyleContext* aContext);

  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  NS_IMETHOD AttributeChanged(PRInt32 aNameSpaceID,
                              nsIAtom* aAttribute,
                              PRInt32 aModType);

  void PaintVideo(nsIRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect, nsPoint aPt);
                              
  
  nsSize GetIntrinsicSize(nsIRenderingContext *aRenderingContext);
  virtual nsSize GetIntrinsicRatio();
  virtual nsSize ComputeSize(nsIRenderingContext *aRenderingContext,
                             nsSize aCBSize, nscoord aAvailableWidth,
                             nsSize aMargin, nsSize aBorder, nsSize aPadding,
                             PRBool aShrinkWrap);
  virtual nscoord GetMinWidth(nsIRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsIRenderingContext *aRenderingContext);
  virtual void Destroy();
  virtual PRBool IsLeaf() const;

  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

#ifdef ACCESSIBILITY
  NS_IMETHOD GetAccessible(nsIAccessible** aAccessible);
#endif

  virtual nsIAtom* GetType() const;

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsSplittableFrame::IsFrameOfType(aFlags & ~(nsIFrame::eReplaced));
  }
  
  virtual nsresult CreateAnonymousContent(nsTArray<nsIContent*>& aElements);

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

protected:

  
  
  PRBool HasVideoElement();

  
  
  
  PRBool HasVideoData();
  
  
  
  PRBool ShouldDisplayPoster();

  
  
  
  nsresult UpdatePosterSource(PRBool aNotify);

  virtual ~nsVideoFrame();

  nsMargin mBorderPadding;
  
  
  nsCOMPtr<nsIContent> mVideoControls;
  
  
  nsCOMPtr<nsIContent> mPosterImage;
};

#endif 
