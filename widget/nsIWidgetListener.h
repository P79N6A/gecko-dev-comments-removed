


































#ifndef nsIWidgetListener_h__
#define nsIWidgetListener_h__

#include "nscore.h"
#include "nsGUIEvent.h"
#include "nsIXULWindow.h"

class nsIView;
class nsIPresShell;

class nsIWidgetListener
{
public:

  




  virtual nsIXULWindow* GetXULWindow() { return nullptr; }

  


  virtual nsIView* GetView() { return nullptr; }

  


  virtual nsIPresShell* GetPresShell() { return nullptr; }

  



  virtual bool WindowMoved(nsIWidget* aWidget, int32_t aX, int32_t aY) { return false; }

  



  virtual bool WindowResized(nsIWidget* aWidget, int32_t aWidth, int32_t aHeight) { return false; }

  


  virtual void SizeModeChanged(nsSizeMode sizeMode) { }

  






  virtual bool ZLevelChanged(bool aImmediate, nsWindowZ *aPlacement,
                             nsIWidget* aRequestBelow, nsIWidget** aActualBelow) { return false; }

  


  virtual void WindowActivated() { }

  


  virtual void WindowDeactivated() { }

  


  virtual void OSToolbarButtonPressed() { }

  



  virtual bool RequestWindowClose(nsIWidget* aWidget) { return false; }

  


  virtual void WillPaintWindow(nsIWidget* aWidget, bool aWillSendDidPaint) { }

  



  enum {
    SENT_WILL_PAINT = 1 << 0, 
    WILL_SEND_DID_PAINT = 1 << 1, 
  };
  virtual bool PaintWindow(nsIWidget* aWidget, nsIntRegion aRegion, uint32_t aFlags) { return false; }

  


  virtual void DidPaintWindow() { }

  


  virtual nsEventStatus HandleEvent(nsGUIEvent* event, bool useAttachedEvents)
  {
    return nsEventStatus_eIgnore;
  }
};

#endif
