



#ifndef nsIPageSequenceFrame_h___
#define nsIPageSequenceFrame_h___

#include "nsQueryFrame.h"
#include "nsRect.h"

class nsPresContext;
class nsIPrintSettings;
class nsITimerCallback;






class nsIPageSequenceFrame : public nsQueryFrame
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsIPageSequenceFrame)

  












  NS_IMETHOD StartPrint(nsPresContext*  aPresContext,
                        nsIPrintSettings* aPrintOptions,
                        PRUnichar* aDocTitle,
                        PRUnichar* aDocURL) = 0;

  NS_IMETHOD PrePrintNextPage(nsITimerCallback* aCallback, bool* aDone) = 0;
  NS_IMETHOD PrintNextPage() = 0;
  NS_IMETHOD ResetPrintCanvasList() = 0;
  NS_IMETHOD GetCurrentPageNum(int32_t* aPageNum) = 0;
  NS_IMETHOD GetNumPages(int32_t* aNumPages) = 0;
  NS_IMETHOD IsDoingPrintRange(bool* aDoing) = 0;
  NS_IMETHOD GetPrintRange(int32_t* aFromPage, int32_t* aToPage) = 0;

  NS_IMETHOD DoPageEnd() = 0;
  NS_IMETHOD SetSelectionHeight(nscoord aYOffset, nscoord aHeight) = 0;

  NS_IMETHOD SetTotalNumPages(int32_t aTotal) = 0;

  
  NS_IMETHOD GetSTFPercent(float& aSTFPercent) = 0;
};

#endif 


