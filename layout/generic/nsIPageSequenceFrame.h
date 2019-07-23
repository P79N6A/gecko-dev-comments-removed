



































#ifndef nsIPageSequenceFrame_h___
#define nsIPageSequenceFrame_h___

#include "nsQueryFrame.h"
#include "nsRect.h"

class nsPresContext;
class nsIPrintSettings;






class nsIPageSequenceFrame : public nsQueryFrame
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsIPageSequenceFrame)

  












  NS_IMETHOD StartPrint(nsPresContext*  aPresContext,
                        nsIPrintSettings* aPrintOptions,
                        PRUnichar* aDocTitle,
                        PRUnichar* aDocURL) = 0;
  NS_IMETHOD PrintNextPage() = 0;
  NS_IMETHOD GetCurrentPageNum(PRInt32* aPageNum) = 0;
  NS_IMETHOD GetNumPages(PRInt32* aNumPages) = 0;
  NS_IMETHOD IsDoingPrintRange(PRBool* aDoing) = 0;
  NS_IMETHOD GetPrintRange(PRInt32* aFromPage, PRInt32* aToPage) = 0;

  NS_IMETHOD DoPageEnd() = 0;
  NS_IMETHOD SetSelectionHeight(nscoord aYOffset, nscoord aHeight) = 0;

  NS_IMETHOD SetTotalNumPages(PRInt32 aTotal) = 0;

  
  NS_IMETHOD GetDeadSpaceValue(nscoord* aValue) = 0;

  
  NS_IMETHOD GetSTFPercent(float& aSTFPercent) = 0;
};

#endif 


