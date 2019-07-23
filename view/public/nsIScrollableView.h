




































#ifndef nsIScrollableView_h___
#define nsIScrollableView_h___

#include "nsCoord.h"
#include "nsNativeWidget.h"

class nsIView;
class nsIScrollPositionListener;
struct nsSize;


#define NS_ISCROLLABLEVIEW_IID    \
  { 0x74254899, 0xccc9, 0x4e67, \
    { 0xa6, 0x78, 0x6b, 0x79, 0xfa, 0x72, 0xb4, 0x86 } }











class nsIScrollableView {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCROLLABLEVIEW_IID)

  




  NS_IMETHOD  GetContainerSize(nscoord *aWidth, nscoord *aHeight) const = 0;

  


  NS_IMETHOD  SetScrolledView(nsIView *aScrolledView) = 0;

  



  NS_IMETHOD  GetScrolledView(nsIView *&aScrolledView) const = 0;

  


  NS_IMETHOD  GetScrollPosition(nscoord &aX, nscoord& aY) const = 0;

  








  NS_IMETHOD ScrollTo(nscoord aX, nscoord aY, PRUint32 aUpdateFlags) = 0;

  





  NS_IMETHOD SetLineHeight(nscoord aHeight) = 0;

  




  NS_IMETHOD GetLineHeight(nscoord *aHeight) = 0;

  








  NS_IMETHOD ScrollByLines(PRInt32 aNumLinesX, PRInt32 aNumLinexY,
                           PRUint32 aUpdateFlags = 0) = 0;
  










  NS_IMETHOD ScrollByLinesWithOverflow(PRInt32 aNumLinesX, PRInt32 aNumLinexY,
                                       PRInt32& aOverflowX, PRInt32& aOverflowY,
                                       PRUint32 aUpdateFlags = 0) = 0;

  




  NS_IMETHOD GetPageScrollDistances(nsSize *aDistances) = 0;

  









  NS_IMETHOD ScrollByPages(PRInt32 aNumPagesX, PRInt32 aNumPagesY,
                           PRUint32 aUpdateFlags = 0) = 0;

  






  NS_IMETHOD ScrollByWhole(PRBool aTop, PRUint32 aUpdateFlags = 0) = 0;

  












  NS_IMETHOD ScrollByPixels(PRInt32 aNumPixelsX, PRInt32 aNumPixelsY,
                            PRInt32& aOverflowX, PRInt32& aOverflowY,
                            PRUint32 aUpdateFlags = 0) = 0;

  






  NS_IMETHOD CanScroll(PRBool aHorizontal, PRBool aForward, PRBool &aResult) = 0;

  


  NS_IMETHOD_(nsIView*) View() = 0;

  


  NS_IMETHOD AddScrollPositionListener(nsIScrollPositionListener* aListener) = 0;
  
  


  NS_IMETHOD RemoveScrollPositionListener(nsIScrollPositionListener* aListener) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScrollableView, NS_ISCROLLABLEVIEW_IID)

#endif
