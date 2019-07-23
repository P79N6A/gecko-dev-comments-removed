



































#ifndef nsIPageSequenceFrame_h___
#define nsIPageSequenceFrame_h___

#include "nsISupports.h"
#include "nsRect.h"

class nsPresContext;
class nsIPrintSettings;



#define NS_IPAGESEQUENCEFRAME_IID \
 { 0xa6cf90d2, 0x15b3, 0x11d2,{0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32}}








class nsIPageSequenceFrame : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IPAGESEQUENCEFRAME_IID)

  












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
  
private:
  NS_IMETHOD_(nsrefcnt) AddRef(void) = 0;
  NS_IMETHOD_(nsrefcnt) Release(void) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIPageSequenceFrame, NS_IPAGESEQUENCEFRAME_IID)

#endif 


