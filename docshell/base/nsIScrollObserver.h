





#ifndef nsIScrollObserver_h___
#define nsIScrollObserver_h___

#include "nsISupports.h"
#include "Units.h"

#define NS_ISCROLLOBSERVER_IID \
  { 0xaa5026eb, 0x2f88, 0x4026, \
    { 0xa4, 0x6b, 0xf4, 0x59, 0x6b, 0x4e, 0xdf, 0x00 } }

class nsIScrollObserver : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCROLLOBSERVER_IID)

  


  virtual void ScrollPositionChanged() = 0;

  



  virtual void AsyncPanZoomStarted() {};

  



  virtual void AsyncPanZoomStopped() {};
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScrollObserver, NS_ISCROLLOBSERVER_IID)

#endif 
