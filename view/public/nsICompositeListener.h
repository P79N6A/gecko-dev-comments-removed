






































#ifndef nsICompositeListener_h___
#define nsICompositeListener_h___

#include "nsISupports.h"


class nsIView;
class nsIViewManager;
class nsIRenderingContext;
class nsIRegion;
struct nsRect;



#define NS_ICOMPOSITELISTENER_IID \
{ 0x5661ce55, 0x7c42, 0x11d3, { 0x9d, 0x1d, 0x0, 0x60, 0xb0, 0xf8, 0xba, 0xff } }





class nsICompositeListener : public nsISupports {
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICOMPOSITELISTENER_IID)

  









  NS_IMETHOD WillRefreshRegion(nsIViewManager *aViewManager,
                               nsIView *aView,
                               nsIRenderingContext *aContext,
                               nsIRegion *aRegion,
                               PRUint32 aUpdateFlags) = 0;

  









  NS_IMETHOD DidRefreshRegion(nsIViewManager *aViewManager,
                              nsIView *aView,
                              nsIRenderingContext *aContext,
                              nsIRegion *aRegion,
                              PRUint32 aUpdateFlags) = 0;

  









  NS_IMETHOD WillRefreshRect(nsIViewManager *aViewManager,
                             nsIView *aView,
                             nsIRenderingContext *aContext,
                             const nsRect *aRect,
                             PRUint32 aUpdateFlags) = 0;

  









  NS_IMETHOD DidRefreshRect(nsIViewManager *aViewManager,
                            nsIView *aView,
                            nsIRenderingContext *aContext,
                            const nsRect *aRect,
                            PRUint32 aUpdateFlags) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICompositeListener, NS_ICOMPOSITELISTENER_IID)

#endif 

