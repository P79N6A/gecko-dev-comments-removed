



#ifndef nsPageFrame_h___
#define nsPageFrame_h___

#include "mozilla/Attributes.h"
#include "nsContainerFrame.h"
#include "nsLeafFrame.h"

class nsSharedPageData;


class nsPageFrame MOZ_FINAL : public nsContainerFrame {

public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsPageFrame* NS_NewPageFrame(nsIPresShell* aPresShell,
                                      nsStyleContext* aContext);

  virtual void Reflow(nsPresContext*      aPresContext,
                      nsHTMLReflowMetrics& aDesiredSize,
                      const nsHTMLReflowState& aMaxSize,
                      nsReflowStatus&      aStatus) MOZ_OVERRIDE;

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;
  
#ifdef DEBUG_FRAME_DUMP
  virtual nsresult  GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

  
  
  

  
  virtual void  SetPageNumInfo(int32_t aPageNumber, int32_t aTotalPages);

  virtual void SetSharedPageData(nsSharedPageData* aPD);

  
  
  virtual bool HonorPrintBackgroundSettings() MOZ_OVERRIDE { return false; }

  void PaintHeaderFooter(nsRenderingContext& aRenderingContext,
                         nsPoint aPt);

protected:
  explicit nsPageFrame(nsStyleContext* aContext);
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

  explicit nsPageBreakFrame(nsStyleContext* aContext);
  ~nsPageBreakFrame();

  virtual void Reflow(nsPresContext*          aPresContext,
                          nsHTMLReflowMetrics&     aDesiredSize,
                          const nsHTMLReflowState& aReflowState,
                          nsReflowStatus&          aStatus) MOZ_OVERRIDE;

  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult  GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

protected:

  virtual nscoord GetIntrinsicISize() MOZ_OVERRIDE;
  virtual nscoord GetIntrinsicBSize() MOZ_OVERRIDE;

    bool mHaveReflowed;

    friend nsIFrame* NS_NewPageBreakFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
};

#endif 

