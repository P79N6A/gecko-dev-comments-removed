



































#ifndef _nsframewindow_h
#define _nsframewindow_h









#include "nsWindow.h"
#include "nssize.h"

class nsFrameWindow : public nsWindow
{
 public:
   nsFrameWindow();
   virtual ~nsFrameWindow();

   
   HWND GetMainWindow() const { return mFrameWnd; }

 protected:
   PFNWP  fnwpDefFrame;
   nsSize mSizeClient;
   nsSize mSizeBorder;
   PRBool mNeedActivation;

   
   virtual void ActivateTopLevelWidget();

   
   virtual void RealDoCreate( HWND hwndP, nsWindow *aParent,
                              const nsIntRect &aRect,
                              EVENT_CALLBACK aHandleEventFunction,
                              nsIDeviceContext *aContext,
                              nsIAppShell *aAppShell,
                              nsWidgetInitData *aInitData, HWND hwndO);

   
   PRBool OnReposition( PSWP pSwp);

   
   void    UpdateClientSize();
   PRInt32 GetClientHeight() { return mSizeClient.height; }

   
   MRESULT FrameMessage( ULONG msg, MPARAM mp1, MPARAM mp2);

   NS_IMETHOD Show( PRBool bState);
   void SetWindowListVisibility( PRBool bState);

   
   NS_IMETHOD GetClientBounds( nsIntRect &aRect);

   friend MRESULT EXPENTRY fnwpFrame( HWND, ULONG, MPARAM, MPARAM);
   static BOOL fHiddenWindowCreated;
};

#endif
