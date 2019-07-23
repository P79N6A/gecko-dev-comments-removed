




































#ifndef nsIScrollableView_h___
#define nsIScrollableView_h___

#include "nsCoord.h"

class nsIView;
class nsIScrollPositionListener;
struct nsSize;


#define NS_ISCROLLABLEVIEW_IID    \
{ 0x1fcd151c, 0x5e26, 0x4c9d, \
{ 0xa5, 0x2c, 0x87, 0x43, 0x7d, 0x7b, 0x1c, 0xe8 } }











class nsIScrollableView {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCROLLABLEVIEW_IID)

  





  NS_IMETHOD  CreateScrollControls(nsNativeWidget aNative = nsnull) = 0;

  




  NS_IMETHOD  GetContainerSize(nscoord *aWidth, nscoord *aHeight) const = 0;

  


  NS_IMETHOD  SetScrolledView(nsIView *aScrolledView) = 0;

  



  NS_IMETHOD  GetScrolledView(nsIView *&aScrolledView) const = 0;

  


  NS_IMETHOD  GetScrollPosition(nscoord &aX, nscoord& aY) const = 0;

  








  NS_IMETHOD ScrollTo(nscoord aX, nscoord aY, PRUint32 aUpdateFlags) = 0;

  





  NS_IMETHOD SetScrollProperties(PRUint32 aProperties) = 0;

  





  NS_IMETHOD GetScrollProperties(PRUint32 *aProperties) = 0;

  





  NS_IMETHOD SetLineHeight(nscoord aHeight) = 0;

  




  NS_IMETHOD GetLineHeight(nscoord *aHeight) = 0;

  







  NS_IMETHOD ScrollByLines(PRInt32 aNumLinesX, PRInt32 aNumLinexY) = 0;

  




  NS_IMETHOD GetPageScrollDistances(nsSize *aDistances) = 0;

  








  NS_IMETHOD ScrollByPages(PRInt32 aNumPagesX, PRInt32 aNumPagesY) = 0;

  





  NS_IMETHOD ScrollByWhole(PRBool aTop) = 0;

  







  NS_IMETHOD ScrollByPixels(PRInt32 aNumPixelsX, PRInt32 aNumPixelsY) = 0;

  






  NS_IMETHOD CanScroll(PRBool aHorizontal, PRBool aForward, PRBool &aResult) = 0;

  


  NS_IMETHOD_(nsIView*) View() = 0;

  


  NS_IMETHOD AddScrollPositionListener(nsIScrollPositionListener* aListener) = 0;
  
  


  NS_IMETHOD RemoveScrollPositionListener(nsIScrollPositionListener* aListener) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScrollableView, NS_ISCROLLABLEVIEW_IID)



#define NS_SCROLL_PROPERTY_ALWAYS_BLIT    0x0001

#endif
