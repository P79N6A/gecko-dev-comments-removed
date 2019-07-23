





































#ifndef nsAppShell_h__
#define nsAppShell_h__

#include "nsIAppShell.h"
#include "nsIEventQueueService.h"
#include "nsWidget.h"
#include "prtime.h"
#include "xlibrgb.h"
#include <X11/Intrinsic.h>

class nsAppShell : public nsIAppShell
{
public:
  nsAppShell(); 
  virtual ~nsAppShell();

  NS_DECL_ISUPPORTS

  
  
  NS_IMETHOD            Create(int* argc, char ** argv);
  virtual nsresult      Run(); 
  NS_IMETHOD            Spinup();
  NS_IMETHOD            Spindown();
  NS_IMETHOD            ListenToEventQueue(nsIEventQueue *aQueue, PRBool aListen);
  NS_IMETHOD            GetNativeEvent(PRBool &aRealEvent, void *&aEvent);
  NS_IMETHOD            DispatchNativeEvent(PRBool aRealEvent, void *aEvent);
  
  NS_IMETHOD            Exit();
  virtual void *        GetNativeData(PRUint32 aDataType);
  static void           DispatchXEvent(XEvent *event);
  


  static XtAppContext   mAppContext; 
  static XlibRgbHandle *GetXlibRgbHandle() { return mXlib_rgb_handle; }
  static Display       *mDisplay;
 private:
  static XlibRgbHandle *mXlib_rgb_handle;
  
  static void HandleButtonEvent(XEvent *event, nsWidget *aWidget);
  static void HandleMotionNotifyEvent(XEvent *event, nsWidget *aWidget);
  static void HandleExposeEvent(XEvent *event, nsWidget *aWidget);
  static void HandleConfigureNotifyEvent(XEvent *event, nsWidget *aWidget);
  static void HandleKeyPressEvent(XEvent *event, nsWidget *aWidget);
  static void HandleKeyReleaseEvent(XEvent *event, nsWidget *aWidget);
  static void HandleFocusInEvent(XEvent *event, nsWidget *aWidget);
  static void HandleFocusOutEvent(XEvent *event, nsWidget *aWidget);
  static void HandleVisibilityNotifyEvent(XEvent *event, nsWidget *aWidget);
  static void HandleMapNotifyEvent(XEvent *event, nsWidget *aWidget);
  static void HandleUnmapNotifyEvent(XEvent *event, nsWidget *aWidget);
  static void HandleEnterEvent(XEvent *event, nsWidget *aWidget);
  static void HandleLeaveEvent(XEvent *event, nsWidget *aWidget);
  static void HandleClientMessageEvent(XEvent *event, nsWidget *aWidget);
  static void HandleSelectionRequestEvent(XEvent *event, nsWidget *aWidget);
  static void HandleDragMotionEvent(XEvent *event, nsWidget *aWidget);
  static void HandleDragEnterEvent(XEvent *event, nsWidget *aWidget);
  static void HandleDragLeaveEvent(XEvent *event, nsWidget *aWidget);
  static void HandleDragDropEvent(XEvent *event, nsWidget *aWidget);
  static void ForwardEvent(XEvent *event, nsWidget *aWidget);
  static PRInt16      mClicks;
  static PRUint16     mClickedButton;
  static PRTime       mClickTime;
  static PRPackedBool mClicked;
  static PRPackedBool mDragging;
  static PRPackedBool mAltDown;
  static PRPackedBool mShiftDown;
  static PRPackedBool mCtrlDown;
  static PRPackedBool mMetaDown;
  static PRPackedBool DieAppShellDie;

protected:
  nsCOMPtr<nsIEventQueue>  mEventQueue;
};

#endif 
