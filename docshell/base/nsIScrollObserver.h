




#ifndef nsIScrollObserver_h___
#define nsIScrollObserver_h___

#include "nsISupports.h"

#define NS_ISCROLLOBSERVER_IID \
  { 0x03465b77, 0x9ce2, 0x4d19, \
    { 0xb2, 0xf6, 0x82, 0xae, 0xee, 0x85, 0xc3, 0xbf } }

class nsIScrollObserver : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCROLLOBSERVER_IID)

  


  virtual void ScrollPositionChanged() = 0;

  


  virtual void AsyncPanZoomStarted(){};

  


  virtual void AsyncPanZoomStopped(){};
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScrollObserver, NS_ISCROLLOBSERVER_IID)

#endif 
