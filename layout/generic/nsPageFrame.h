



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

  




  virtual nsIAtom* GetType() const;
  
#ifdef DEBUG
  NS_IMETHOD  GetFrameName(nsAString& aResult) const;
#endif

  
  
  

  
  virtual void  SetPageNumInfo(int32_t aPageNumber, int32_t aTotalPages);

  virtual void SetSharedPageData(nsSharedPageData* aPD);

  
  
  virtual bool HonorPrintBackgroundSettings() { return false; }

  void PaintHeaderFooter(nsRenderingContext& aRenderingContext,
                         nsPoint aPt);
  void PaintPageContent(nsRenderingContext& aRenderingContext,
                        const nsRect&        aDirtyRect,
                        nsPoint              aPt);

protected:
  nsPageFrame(nsStyleContext* aContext);
  virtual ~nsPageFrame();

  typedef enum {
    eHeader,
    eFooter
  } nsHeaderFooterEnum;

  nscoord GetXPosition(nsRenderingContext& aRenderingContext, 
                       const nsRect&        aRect, 
                       int32_t              aJust,
                       const nsString&      aStr);

  void DrawHeaderFooter(nsRenderingContext& aRenderingContext,
                        nsHeaderFooterEnum   aHeaderFooter,
                        int32_t              aJust,
                        const nsString&      sStr,
                        const nsRect&        aRect,
                        nscoord              aHeight,
                        nscoord              aAscent,
                        nscoord              aWidth);

  void DrawHeaderFooter(nsRenderingContext& aRenderingContext,
                        nsHeaderFooterEnum   aHeaderFooter,
                        const nsString&      aStrLeft,
                        const nsString&      aStrRight,
                        const nsString&      aStrCenter,
                        const nsRect&        aRect,
                        nscoord              aAscent,
                        nscoord              aHeight);

  void ProcessSpecialCodes(const nsString& aStr, nsString& aNewStr);

  int32_t     mPageNum;
  int32_t     mTotNumPages;

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

#ifdef DEBUG
  NS_IMETHOD  GetFrameName(nsAString& aResult) const;
#endif

protected:

  virtual nscoord GetIntrinsicWidth();
  virtual nscoord GetIntrinsicHeight();

    bool mHaveReflowed;

    friend nsIFrame* NS_NewPageBreakFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
};

#endif 

