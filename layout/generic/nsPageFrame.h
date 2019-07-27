



#ifndef nsPageFrame_h___
#define nsPageFrame_h___

#include "mozilla/Attributes.h"
#include "nsContainerFrame.h"
#include "nsLeafFrame.h"

class nsFontMetrics;
class nsSharedPageData;


class nsPageFrame final : public nsContainerFrame {

public:
  NS_DECL_QUERYFRAME_TARGET(nsPageFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  friend nsPageFrame* NS_NewPageFrame(nsIPresShell* aPresShell,
                                      nsStyleContext* aContext);

  virtual void Reflow(nsPresContext*      aPresContext,
                      nsHTMLReflowMetrics& aDesiredSize,
                      const nsHTMLReflowState& aMaxSize,
                      nsReflowStatus&      aStatus) override;

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) override;

  




  virtual nsIAtom* GetType() const override;
  
#ifdef DEBUG_FRAME_DUMP
  virtual nsresult  GetFrameName(nsAString& aResult) const override;
#endif

  
  
  

  
  virtual void  SetPageNumInfo(int32_t aPageNumber, int32_t aTotalPages);

  virtual void SetSharedPageData(nsSharedPageData* aPD);

  
  
  virtual bool HonorPrintBackgroundSettings() override { return false; }

  void PaintHeaderFooter(nsRenderingContext& aRenderingContext,
                         nsPoint aPt, bool aSubpixelAA);

protected:
  explicit nsPageFrame(nsStyleContext* aContext);
  virtual ~nsPageFrame();

  typedef enum {
    eHeader,
    eFooter
  } nsHeaderFooterEnum;

  nscoord GetXPosition(nsRenderingContext& aRenderingContext,
                       nsFontMetrics&       aFontMetrics,
                       const nsRect&        aRect, 
                       int32_t              aJust,
                       const nsString&      aStr);

  void DrawHeaderFooter(nsRenderingContext& aRenderingContext,
                        nsFontMetrics&       aFontMetrics,
                        nsHeaderFooterEnum   aHeaderFooter,
                        int32_t              aJust,
                        const nsString&      sStr,
                        const nsRect&        aRect,
                        nscoord              aHeight,
                        nscoord              aAscent,
                        nscoord              aWidth);

  void DrawHeaderFooter(nsRenderingContext& aRenderingContext,
                        nsFontMetrics&       aFontMetrics,
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
                          nsReflowStatus&          aStatus) override;

  virtual nsIAtom* GetType() const override;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult  GetFrameName(nsAString& aResult) const override;
#endif

protected:

  virtual nscoord GetIntrinsicISize() override;
  virtual nscoord GetIntrinsicBSize() override;

    bool mHaveReflowed;

    friend nsIFrame* NS_NewPageBreakFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
};

#endif 

