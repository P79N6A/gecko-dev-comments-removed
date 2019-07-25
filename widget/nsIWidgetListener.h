


































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

  



  virtual bool WindowMoved(nsIWidget* aWidget, PRInt32 aX, PRInt32 aY) { return false; }

  



  virtual bool WindowResized(nsIWidget* aWidget, PRInt32 aWidth, PRInt32 aHeight) { return false; }

  


  virtual void SizeModeChanged(nsSizeMode sizeMode) { }

  






  virtual bool ZLevelChanged(bool aImmediate, nsWindowZ *aPlacement,
                             nsIWidget* aRequestBelow, nsIWidget** aActualBelow) { return false; }

  


  virtual void WindowActivated() { }

  


  virtual void WindowDeactivated() { }

  


  virtual void OSToolbarButtonPressed() { }

  



  virtual bool RequestWindowClose(nsIWidget* aWidget) { return false; }

  


  virtual void WillPaintWindow(nsIWidget* aWidget, bool aWillSendDidPaint) { }

  





  virtual bool PaintWindow(nsIWidget* aWidget, nsIntRegion aRegion,
                           bool aSentWillPaint, bool aWillSendDidPaint) { return false; }

  


  virtual void DidPaintWindow() { }

  


  virtual nsEventStatus HandleEvent(nsGUIEvent* event, bool useAttachedEvents)
  {
    return nsEventStatus_eIgnore;
  }
};

#endif
