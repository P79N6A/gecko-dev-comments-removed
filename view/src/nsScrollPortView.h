




































#ifndef nsScrollPortView_h___
#define nsScrollPortView_h___

#include "nsView.h"
#include "nsIScrollableView.h"
#include "nsCOMPtr.h"
#include "nsITimer.h"

class nsISupportsArray;
class AsyncScroll;




class nsScrollPortView : public nsView, public nsIScrollableView
{
public:
  nsScrollPortView(nsViewManager* aViewManager = nsnull);

  virtual nsIScrollableView* ToScrollableView() { return this; }

  NS_IMETHOD QueryInterface(REFNSIID aIID,
                            void** aInstancePtr);

  
  NS_IMETHOD  CreateScrollControls(nsNativeWidget aNative = nsnull);
  NS_IMETHOD  GetContainerSize(nscoord *aWidth, nscoord *aHeight) const;
  NS_IMETHOD  SetScrolledView(nsIView *aScrolledView);
  NS_IMETHOD  GetScrolledView(nsIView *&aScrolledView) const;

  NS_IMETHOD  GetScrollPosition(nscoord &aX, nscoord &aY) const;
  NS_IMETHOD  ScrollTo(nscoord aX, nscoord aY, PRUint32 aUpdateFlags);
  NS_IMETHOD  SetScrollProperties(PRUint32 aProperties);
  NS_IMETHOD  GetScrollProperties(PRUint32 *aProperties);
  NS_IMETHOD  SetLineHeight(nscoord aHeight);
  NS_IMETHOD  GetLineHeight(nscoord *aHeight);
  NS_IMETHOD  ScrollByLines(PRInt32 aNumLinesX, PRInt32 aNumLinesY,
                            PRUint32 aUpdateFlags = 0);
  NS_IMETHOD  GetPageScrollDistances(nsSize *aDistances);
  NS_IMETHOD  ScrollByPages(PRInt32 aNumPagesX, PRInt32 aNumPagesY,
                            PRUint32 aUpdateFlags = 0);
  NS_IMETHOD  ScrollByWhole(PRBool aTop, PRUint32 aUpdateFlags = 0);
  NS_IMETHOD  ScrollByPixels(PRInt32 aNumPixelsX, PRInt32 aNumPixelsY,
                             PRUint32 aUpdateFlags = 0);
  NS_IMETHOD  CanScroll(PRBool aHorizontal, PRBool aForward, PRBool &aResult);

  NS_IMETHOD_(nsIView*) View();

  NS_IMETHOD  AddScrollPositionListener(nsIScrollPositionListener* aListener);
  NS_IMETHOD  RemoveScrollPositionListener(nsIScrollPositionListener* aListener);

  

  nsView*     GetScrolledView() const { return GetFirstChild(); }

private:
  NS_IMETHOD  ScrollToImpl(nscoord aX, nscoord aY);

  
  AsyncScroll* mAsyncScroll;

  
  void        IncrementalScroll();
  PRBool      IsSmoothScrollingEnabled();
  static void AsyncScrollCallback(nsITimer *aTimer, void* aSPV);

protected:
  virtual ~nsScrollPortView();

  
  void Scroll(nsView *aScrolledView, nsPoint aTwipsDelta, nsIntPoint aPixDelta, nscoord aP2A);
  PRBool CannotBitBlt(nsView* aScrolledView);

  nscoord             mOffsetX, mOffsetY;
  nscoord             mDestinationX, mDestinationY;
  PRUint32            mScrollProperties;
  nscoord             mLineHeight;
  nsISupportsArray   *mListeners;
};

#endif
