



#ifndef nsSimplePageSequence_h___
#define nsSimplePageSequence_h___

#include "nsIPageSequenceFrame.h"
#include "nsContainerFrame.h"
#include "nsIPrintSettings.h"
#include "nsIPrintOptions.h"
#include "nsIDateTimeFormat.h"





class nsSharedPageData {
public:
  nsSharedPageData();
  ~nsSharedPageData();

  PRUnichar * mDateTimeStr;
  nsFont *    mHeadFootFont;
  PRUnichar * mPageNumFormat;
  PRUnichar * mPageNumAndTotalsFormat;
  PRUnichar * mDocTitle;
  PRUnichar * mDocURL;

  nsSize      mReflowSize;
  nsMargin    mReflowMargin;
  
  
  nsMargin    mExtraMargin;
  
  
  
  nsMargin    mEdgePaperMargin;

  nsCOMPtr<nsIPrintSettings> mPrintSettings;
  nsCOMPtr<nsIPrintOptions> mPrintOptions;

  nscoord      mPageContentXMost;      
  nscoord      mPageContentSize;       
};


class nsSimplePageSequenceFrame : public nsContainerFrame,
                                  public nsIPageSequenceFrame {
public:
  friend nsIFrame* NS_NewSimplePageSequenceFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  NS_IMETHOD  Reflow(nsPresContext*      aPresContext,
                     nsHTMLReflowMetrics& aDesiredSize,
                     const nsHTMLReflowState& aMaxSize,
                     nsReflowStatus&      aStatus);

  NS_IMETHOD  BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                               const nsRect&           aDirtyRect,
                               const nsDisplayListSet& aLists);

  
  NS_IMETHOD SetPageNo(int32_t aPageNo) { return NS_OK;}
  NS_IMETHOD SetSelectionHeight(nscoord aYOffset, nscoord aHeight) { mYSelOffset = aYOffset; mSelectionHeight = aHeight; return NS_OK; }
  NS_IMETHOD SetTotalNumPages(int32_t aTotal) { mTotalPages = aTotal; return NS_OK; }
  
  
  NS_IMETHOD GetSTFPercent(float& aSTFPercent);

  
  NS_IMETHOD StartPrint(nsPresContext*  aPresContext,
                        nsIPrintSettings* aPrintSettings,
                        PRUnichar*        aDocTitle,
                        PRUnichar*        aDocURL);
  NS_IMETHOD PrintNextPage();
  NS_IMETHOD GetCurrentPageNum(int32_t* aPageNum);
  NS_IMETHOD GetNumPages(int32_t* aNumPages);
  NS_IMETHOD IsDoingPrintRange(bool* aDoing);
  NS_IMETHOD GetPrintRange(int32_t* aFromPage, int32_t* aToPage);
  NS_IMETHOD DoPageEnd();

  
  
  virtual bool HonorPrintBackgroundSettings() { return false; }

  




  virtual nsIAtom* GetType() const;
  
#ifdef DEBUG
  NS_IMETHOD  GetFrameName(nsAString& aResult) const;
#endif

  void PaintPageSequence(nsRenderingContext& aRenderingContext,
                         const nsRect&        aDirtyRect,
                         nsPoint              aPt);

protected:
  nsSimplePageSequenceFrame(nsStyleContext* aContext);
  virtual ~nsSimplePageSequenceFrame();

  void SetPageNumberFormat(const char* aPropName, const char* aDefPropVal, bool aPageNumOnly);

  
  void SetDateTimeStr(PRUnichar * aDateTimeStr);
  void SetPageNumberFormat(PRUnichar * aFormatStr, bool aForPageNumOnly);

  
  
  void SetDesiredSize(nsHTMLReflowMetrics& aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nscoord aWidth, nscoord aHeight);

  nsMargin mMargin;

  
  nsCOMPtr<nsIDateTimeFormat> mDateFormatter;

  nsSize       mSize;
  nsSharedPageData* mPageData; 

  
  nsIFrame *   mCurrentPageFrame;
  int32_t      mPageNum;
  int32_t      mTotalPages;
  int32_t      mPrintRangeType;
  int32_t      mFromPageNum;
  int32_t      mToPageNum;
  nsTArray<int32_t> mPageRanges;

  
  nscoord      mSelectionHeight;
  nscoord      mYSelOffset;

  
  bool mPrintThisPage;
  bool mDoingPageRange;

  bool mIsPrintingSelection;
};

#endif 

