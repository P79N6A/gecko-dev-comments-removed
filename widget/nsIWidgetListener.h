



#ifndef nsIWidgetListener_h__
#define nsIWidgetListener_h__

#include "nscore.h"
#include "nsIXULWindow.h"
#include "nsRegion.h"
#include "mozilla/BasicEvents.h"

class nsView;
class nsIPresShell;




enum nsSizeMode
{
  nsSizeMode_Normal = 0,
  nsSizeMode_Minimized,
  nsSizeMode_Maximized,
  nsSizeMode_Fullscreen
};




enum nsWindowZ
{
  nsWindowZTop = 0,   
  nsWindowZBottom,    
  nsWindowZRelative   
};

class nsIWidgetListener
{
public:

  




  virtual nsIXULWindow* GetXULWindow() { return nullptr; }

  


  virtual nsView* GetView() { return nullptr; }

  


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

  




  virtual void WillPaintWindow(nsIWidget* aWidget) { }

  





  virtual bool PaintWindow(nsIWidget* aWidget, nsIntRegion aRegion) { return false; }

  





  virtual void DidPaintWindow() { }

  


  virtual void RequestRepaint() { }

  


  virtual nsEventStatus HandleEvent(nsGUIEvent* event, bool useAttachedEvents)
  {
    return nsEventStatus_eIgnore;
  }
};

#endif
