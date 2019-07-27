



#ifndef nsIWidgetListener_h__
#define nsIWidgetListener_h__

#include <stdint.h>

#include "mozilla/EventForwards.h"

class nsView;
class nsIntRegion;
class nsIPresShell;
class nsIWidget;
class nsIXULWindow;




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

  




  virtual nsIXULWindow* GetXULWindow();

  


  virtual nsView* GetView();

  


  virtual nsIPresShell* GetPresShell();

  



  virtual bool WindowMoved(nsIWidget* aWidget, int32_t aX, int32_t aY);

  



  virtual bool WindowResized(nsIWidget* aWidget,
                             int32_t aWidth, int32_t aHeight);

  


  virtual void SizeModeChanged(nsSizeMode aSizeMode);

  






  virtual bool ZLevelChanged(bool aImmediate, nsWindowZ* aPlacement,
                             nsIWidget* aRequestBelow,
                             nsIWidget** aActualBelow);

  


  virtual void FullscreenChanged(bool aInFullscreen);

  


  virtual void WindowActivated();

  


  virtual void WindowDeactivated();

  


  virtual void OSToolbarButtonPressed();

  



  virtual bool RequestWindowClose(nsIWidget* aWidget);

  




  virtual void WillPaintWindow(nsIWidget* aWidget);

  





  virtual bool PaintWindow(nsIWidget* aWidget, nsIntRegion aRegion);

  





  virtual void DidPaintWindow();

  virtual void DidCompositeWindow();

  


  virtual void RequestRepaint();

  


  virtual nsEventStatus HandleEvent(mozilla::WidgetGUIEvent* aEvent,
                                    bool aUseAttachedEvents);
};

#endif
