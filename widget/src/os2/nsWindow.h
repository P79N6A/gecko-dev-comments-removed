














































































#ifndef _nswindow_h
#define _nswindow_h

#include "nsBaseWidget.h"
#include "gfxASurface.h"

#define INCL_DOS
#define INCL_WIN
#define INCL_NLS
#define INCL_GPI
#include <os2.h>





#ifndef WM_MOUSEENTER
#define WM_MOUSEENTER   0x041E
#endif

#ifndef WM_MOUSELEAVE
#define WM_MOUSELEAVE   0x041F
#endif

#ifndef WM_FOCUSCHANGED
#define WM_FOCUSCHANGED 0x000E
#endif

extern "C" {
  PVOID  APIENTRY WinQueryProperty(HWND hwnd, PCSZ pszNameOrAtom);
  PVOID  APIENTRY WinRemoveProperty(HWND hwnd, PCSZ pszNameOrAtom);
  BOOL   APIENTRY WinSetProperty(HWND hwnd, PCSZ pszNameOrAtom,
                                 PVOID pvData, ULONG ulFlags);
  APIRET APIENTRY DosQueryModFromEIP(HMODULE* phMod, ULONG* pObjNum,
                                     ULONG BuffLen,  PCHAR pBuff,
                                     ULONG* pOffset, ULONG Address);
}





#define kWindowClassName            "MozillaWindowClass"
#define QWL_NSWINDOWPTR             (QWL_USER+4)


#define kIsInitialized              0x0001
#define kIsDBCS                     0x0002
#define kIsTrackPoint               0x0004


#define nsWindowState_ePrecreate    0x0001      // Create() not called yet
#define nsWindowState_eInCreate     0x0002      // processing Create() method
#define nsWindowState_eLive         0x0004      // active, existing window
#define nsWindowState_eClosing      0x0008      // processing Close() method
#define nsWindowState_eDoingDelete  0x0010      // object destructor running
#define nsWindowState_eDead         0x0100      // window destroyed









class imgIContainer;
class gfxOS2Surface;
class os2FrameWindow;

MRESULT EXPENTRY fnwpNSWindow(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
MRESULT EXPENTRY fnwpFrame(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);





class nsWindow : public nsBaseWidget
{
public:
  nsWindow();
  virtual ~nsWindow();

  
  NS_IMETHOD            Create(nsIWidget* aParent,
                               nsNativeWidget aNativeParent,
                               const nsIntRect& aRect,
                               EVENT_CALLBACK aHandleEventFunction,
                               nsDeviceContext* aContext,
                               nsWidgetInitData* aInitData = nsnull);
  NS_IMETHOD            Destroy();
  virtual nsIWidget*    GetParent();
  virtual float         GetDPI();
  NS_IMETHOD            Enable(bool aState);
  NS_IMETHOD            IsEnabled(bool* aState);
  NS_IMETHOD            Show(bool aState);
  NS_IMETHOD            IsVisible(bool& aState);
  NS_IMETHOD            SetFocus(bool aRaise);
  NS_IMETHOD            Invalidate(const nsIntRect& aRect,
                                   bool aIsSynchronous);
  NS_IMETHOD            Update();
  gfxASurface*          GetThebesSurface();
  virtual void*         GetNativeData(PRUint32 aDataType);
  virtual void          FreeNativeData(void* aDatum, PRUint32 aDataType);
  NS_IMETHOD            CaptureMouse(bool aCapture);
  virtual bool          HasPendingInputEvent();
  NS_IMETHOD            GetBounds(nsIntRect& aRect);
  NS_IMETHOD            GetClientBounds(nsIntRect& aRect);
  virtual nsIntPoint    WidgetToScreenOffset();
  NS_IMETHOD            Move(PRInt32 aX, PRInt32 aY);
  NS_IMETHOD            Resize(PRInt32 aWidth, PRInt32 aHeight,
                               bool    aRepaint);
  NS_IMETHOD            Resize(PRInt32 aX, PRInt32 aY,
                               PRInt32 aWidth, PRInt32 aHeight,
                               bool    aRepaint);
  NS_IMETHOD            PlaceBehind(nsTopLevelWidgetZPlacement aPlacement,
                                    nsIWidget* aWidget, bool aActivate);
  NS_IMETHOD            SetZIndex(PRInt32 aZIndex);
  virtual nsresult      ConfigureChildren(const nsTArray<Configuration>& aConfigurations);
  NS_IMETHOD            SetSizeMode(PRInt32 aMode);
  NS_IMETHOD            HideWindowChrome(bool aShouldHide);
  NS_IMETHOD            SetTitle(const nsAString& aTitle); 
  NS_IMETHOD            SetIcon(const nsAString& aIconSpec); 
  NS_IMETHOD            ConstrainPosition(bool aAllowSlop,
                                          PRInt32* aX, PRInt32* aY);
  NS_IMETHOD            SetCursor(nsCursor aCursor);
  NS_IMETHOD            SetCursor(imgIContainer* aCursor,
                                  PRUint32 aHotspotX, PRUint32 aHotspotY);
  NS_IMETHOD            CaptureRollupEvents(nsIRollupListener* aListener,
                                            bool aDoCapture, bool aConsumeRollupEvent);
  NS_IMETHOD            GetToggledKeyState(PRUint32 aKeyCode,
                                           bool* aLEDState);
  NS_IMETHOD            DispatchEvent(nsGUIEvent* event,
                                      nsEventStatus& aStatus);
  NS_IMETHOD            ReparentNativeWidget(nsIWidget* aNewParent);

  
  static void           ReleaseGlobals();
protected:
  
  virtual void          OnDestroy();

  
  static void           InitGlobals();
  nsresult              CreateWindow(nsWindow* aParent,
                                     HWND aParentWnd,
                                     const nsIntRect& aRect,
                                     nsWidgetInitData* aInitData);
  gfxASurface*          ConfirmThebesSurface();
  HWND                  GetMainWindow();
  static nsWindow*      GetNSWindowPtr(HWND aWnd);
  static bool           SetNSWindowPtr(HWND aWnd, nsWindow* aPtr);
  void                  NS2PM(POINTL& ptl);
  void                  NS2PM(RECTL& rcl);
  void                  NS2PM_PARENT(POINTL& ptl);
  void                  ActivatePlugin(HWND aWnd);
  void                  SetPluginClipRegion(const Configuration& aConfiguration);
  HWND                  GetPluginClipWindow(HWND aParentWnd);
  void                  ActivateTopLevelWidget();
  HBITMAP               DataToBitmap(PRUint8* aImageData, PRUint32 aWidth,
                                     PRUint32 aHeight, PRUint32 aDepth);
  HBITMAP               CreateBitmapRGB(PRUint8* aImageData,
                                        PRUint32 aWidth, PRUint32 aHeight);
  HBITMAP               CreateTransparencyMask(gfxASurface::gfxImageFormat format,
                                               PRUint8* aImageData,
                                               PRUint32 aWidth, PRUint32 aHeight);
  static bool           EventIsInsideWindow(nsWindow* aWindow); 
  static bool           RollupOnButtonDown(ULONG aMsg);
  static void           RollupOnFocusLost(HWND aFocus);
  MRESULT               ProcessMessage(ULONG msg, MPARAM mp1, MPARAM mp2);
  bool                  OnReposition(PSWP pNewSwp);
  bool                  OnPaint();
  bool                  OnMouseChord(MPARAM mp1, MPARAM mp2);
  bool                  OnDragDropMsg(ULONG msg, MPARAM mp1, MPARAM mp2,
                                      MRESULT& mr);
  bool                  CheckDragStatus(PRUint32 aAction, HPS* aHps);
  bool                  ReleaseIfDragHPS(HPS aHps);
  bool                  OnTranslateAccelerator(PQMSG pQmsg);
  bool                  DispatchKeyEvent(MPARAM mp1, MPARAM mp2);
  void                  InitEvent(nsGUIEvent& event, nsIntPoint* pt = 0);
  bool                  DispatchWindowEvent(nsGUIEvent* event);
  bool                  DispatchWindowEvent(nsGUIEvent* event,
                                            nsEventStatus& aStatus);
  bool                  DispatchCommandEvent(PRUint32 aEventCommand);
  bool                  DispatchDragDropEvent(PRUint32 aMsg);
  bool                  DispatchMoveEvent(PRInt32 aX, PRInt32 aY);
  bool                  DispatchResizeEvent(PRInt32 aClientX, 
                                            PRInt32 aClientY);
  bool                  DispatchMouseEvent(PRUint32 aEventType,
                                           MPARAM mp1, MPARAM mp2, 
                                           bool aIsContextMenuKey = false,
                                           PRInt16 aButton = nsMouseEvent::eLeftButton);
  bool                  DispatchActivationEvent(PRUint32 aEventType);
  bool                  DispatchScrollEvent(ULONG msg, MPARAM mp1, MPARAM mp2);

  friend MRESULT EXPENTRY fnwpNSWindow(HWND hwnd, ULONG msg,
                                       MPARAM mp1, MPARAM mp2);
  friend MRESULT EXPENTRY fnwpFrame(HWND hwnd, ULONG msg,
                                    MPARAM mp1, MPARAM mp2);
  friend class os2FrameWindow;

  HWND          mWnd;               
  nsWindow*     mParent;            
  os2FrameWindow* mFrame;           
  PRInt32       mWindowState;       
  bool          mIsDestroying;      
  bool          mInSetFocus;        
  bool          mNoPaint;           
  HPS           mDragHps;           
  PRUint32      mDragStatus;        
  HWND          mClipWnd;           
  HPOINTER      mCssCursorHPtr;     
  nsCOMPtr<imgIContainer> mCssCursorImg;
  nsRefPtr<gfxOS2Surface> mThebesSurface;
#ifdef DEBUG_FOCUS
  int           mWindowIdentifier;  
#endif
};








class nsChildWindow : public nsWindow {
public:
  nsChildWindow()       {}
  ~nsChildWindow()      {}
};

#endif 



