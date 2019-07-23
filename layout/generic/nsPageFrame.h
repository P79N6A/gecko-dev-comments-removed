



































#ifndef nsPageFrame_h___
#define nsPageFrame_h___

#include "nsContainerFrame.h"
#include "nsLeafFrame.h"

class nsSharedPageData;


class nsPageFrame : public nsContainerFrame {

public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewPageFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD  Reflow(nsPresContext*      aPresContext,
                     nsHTMLReflowMetrics& aDesiredSize,
                     const nsHTMLReflowState& aMaxSize,
                     nsReflowStatus&      aStatus);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  virtual PRBool IsContainingBlock() const;

  




  virtual nsIAtom* GetType() const;
  
#ifdef NS_DEBUG
  NS_IMETHOD  GetFrameName(nsAString& aResult) const;
#endif

  
  
  

  
  virtual void  SetPageNumInfo(PRInt32 aPageNumber, PRInt32 aTotalPages);

  virtual void SetSharedPageData(nsSharedPageData* aPD);

  void PaintPrintPreviewBackground(nsIRenderingContext& aRenderingContext,
                                   nsPoint aPt);
  void PaintHeaderFooter(nsIRenderingContext& aRenderingContext,
                         nsPoint aPt);
  void PaintPageContent(nsIRenderingContext& aRenderingContext,
                        const nsRect&        aDirtyRect,
                        nsPoint              aPt);

protected:
  nsPageFrame(nsStyleContext* aContext);
  virtual ~nsPageFrame();

  typedef enum {
    eHeader,
    eFooter
  } nsHeaderFooterEnum;

  nscoord GetXPosition(nsIRenderingContext& aRenderingContext, 
                       const nsRect&        aRect, 
                       PRInt32              aJust,
                       const nsString&      aStr);

  void DrawHeaderFooter(nsIRenderingContext& aRenderingContext,
                        nsHeaderFooterEnum   aHeaderFooter,
                        PRInt32              aJust,
                        const nsString&      sStr,
                        const nsRect&        aRect,
                        nscoord              aHeight,
                        nscoord              aAscent,
                        nscoord              aWidth);

  void DrawHeaderFooter(nsIRenderingContext& aRenderingContext,
                        nsHeaderFooterEnum   aHeaderFooter,
                        const nsString&      aStrLeft,
                        const nsString&      aStrRight,
                        const nsString&      aStrCenter,
                        const nsRect&        aRect,
                        nscoord              aAscent,
                        nscoord              aHeight);

  void ProcessSpecialCodes(const nsString& aStr, nsString& aNewStr);

  PRInt32     mPageNum;
  PRInt32     mTotNumPages;

  nsSharedPageData* mPD;
};


class nsPageBreakFrame : public nsLeafFrame
{
  NS_DECL_FRAMEARENA_HELPERS

  nsPageBreakFrame(nsStyleContext* aContext);
  ~nsPageBreakFrame();

  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  virtual nsIAtom* GetType() const;

#ifdef NS_DEBUG
  NS_IMETHOD  GetFrameName(nsAString& aResult) const;
#endif

protected:

  virtual nscoord GetIntrinsicWidth();
  virtual nscoord GetIntrinsicHeight();

    PRBool mHaveReflowed;

    friend nsIFrame* NS_NewPageBreakFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
};

#endif 

