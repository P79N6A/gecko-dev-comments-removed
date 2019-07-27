





#ifndef nsIScrollObserver_h___
#define nsIScrollObserver_h___

#include "nsISupports.h"
#include "Units.h"

#define NS_ISCROLLOBSERVER_IID \
  { 0x00bc10e3, 0xaa59, 0x4aa3, \
    { 0x88, 0xe9, 0x43, 0x0a, 0x01, 0xa3, 0x88, 0x04 } }

class nsIScrollObserver : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCROLLOBSERVER_IID)

  


  virtual void ScrollPositionChanged() = 0;

  



  virtual void AsyncPanZoomStarted(const mozilla::CSSIntPoint aScrollPos) {};

  



  virtual void AsyncPanZoomStopped(const mozilla::CSSIntPoint aScrollPos) {};
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScrollObserver, NS_ISCROLLOBSERVER_IID)

#endif 
