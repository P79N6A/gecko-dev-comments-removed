













































#ifndef _nswindow_h
#define _nswindow_h

#include "nsBaseWidget.h"
#include "gfxASurface.h"

#define INCL_DOS
#define INCL_WIN
#define INCL_NLS
#define INCL_GPI
#include <os2.h>
#include <os2im.h>




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
                               nsWidgetInitData* aInitData = nullptr);
  NS_IMETHOD            Destroy();
  virtual nsIWidget*    GetParent();
  virtual float         GetDPI();
  NS_IMETHOD            Enable(bool aState);
  virtual bool          IsEnabled() const;
  NS_IMETHOD            Show(bool aState);
  virtual bool          IsVisible() const;
  NS_IMETHOD            SetFocus(bool aRaise);
  NS_IMETHOD            Invalidate(const nsIntRect& aRect);
  gfxASurface*          GetThebesSurface();
  virtual void*         GetNativeData(uint32_t aDataType);
  virtual void          FreeNativeData(void* aDatum, uint32_t aDataType);
  NS_IMETHOD            CaptureMouse(bool aCapture);
  virtual bool          HasPendingInputEvent();
  NS_IMETHOD            GetBounds(nsIntRect& aRect);
  NS_IMETHOD            GetClientBounds(nsIntRect& aRect);
  virtual nsIntPoint    WidgetToScreenOffset();
  NS_IMETHOD            Move(int32_t aX, int32_t aY);
  NS_IMETHOD            Resize(int32_t aWidth, int32_t aHeight,
                               bool    aRepaint);
  NS_IMETHOD            Resize(int32_t aX, int32_t aY,
                               int32_t aWidth, int32_t aHeight,
                               bool    aRepaint);
  NS_IMETHOD            PlaceBehind(nsTopLevelWidgetZPlacement aPlacement,
                                    nsIWidget* aWidget, bool aActivate);
  NS_IMETHOD            SetZIndex(int32_t aZIndex);
  virtual nsresult      ConfigureChildren(const nsTArray<Configuration>& aConfigurations);
  NS_IMETHOD            SetSizeMode(int32_t aMode);
  NS_IMETHOD            HideWindowChrome(bool aShouldHide);
  NS_IMETHOD            SetTitle(const nsAString& aTitle); 
  NS_IMETHOD            SetIcon(const nsAString& aIconSpec); 
  NS_IMETHOD            ConstrainPosition(bool aAllowSlop,
                                          int32_t* aX, int32_t* aY);
  NS_IMETHOD            SetCursor(nsCursor aCursor);
  NS_IMETHOD            SetCursor(imgIContainer* aCursor,
                                  uint32_t aHotspotX, uint32_t aHotspotY);
  NS_IMETHOD            CaptureRollupEvents(nsIRollupListener* aListener,
                                            bool aDoCapture, bool aConsumeRollupEvent);
  NS_IMETHOD            GetToggledKeyState(uint32_t aKeyCode,
                                           bool* aLEDState);
  NS_IMETHOD            DispatchEvent(nsGUIEvent* event,
                                      nsEventStatus& aStatus);
  NS_IMETHOD            ReparentNativeWidget(nsIWidget* aNewParent);

  NS_IMETHOD_(void)     SetInputContext(const InputContext& aInputContext,
                                        const InputContextAction& aAction)
  {
    mInputContext = aInputContext;
  }
  NS_IMETHOD_(InputContext) GetInputContext()
  {
    return mInputContext;
  }

  
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
  HBITMAP               DataToBitmap(uint8_t* aImageData, uint32_t aWidth,
                                     uint32_t aHeight, uint32_t aDepth);
  HBITMAP               CreateBitmapRGB(uint8_t* aImageData,
                                        uint32_t aWidth, uint32_t aHeight);
  HBITMAP               CreateTransparencyMask(gfxASurface::gfxImageFormat format,
                                               uint8_t* aImageData,
                                               uint32_t aWidth, uint32_t aHeight);
  static bool           EventIsInsideWindow(nsWindow* aWindow); 
  static bool           RollupOnButtonDown(ULONG aMsg);
  static void           RollupOnFocusLost(HWND aFocus);
  MRESULT               ProcessMessage(ULONG msg, MPARAM mp1, MPARAM mp2);
  bool                  OnReposition(PSWP pNewSwp);
  bool                  OnPaint();
  bool                  OnMouseChord(MPARAM mp1, MPARAM mp2);
  bool                  OnDragDropMsg(ULONG msg, MPARAM mp1, MPARAM mp2,
                                      MRESULT& mr);
  bool                  CheckDragStatus(uint32_t aAction, HPS* aHps);
  bool                  ReleaseIfDragHPS(HPS aHps);
  bool                  OnTranslateAccelerator(PQMSG pQmsg);
  bool                  OnQueryConvertPos(MPARAM mp1, MRESULT& mresult);
  bool                  ImeResultString(HIMI himi);
  bool                  ImeConversionString(HIMI himi);
  bool                  OnImeRequest(MPARAM mp1, MPARAM mp2);
  bool                  DispatchKeyEvent(MPARAM mp1, MPARAM mp2);
  void                  InitEvent(nsGUIEvent& event, nsIntPoint* pt = 0);
  bool                  DispatchWindowEvent(nsGUIEvent* event);
  bool                  DispatchWindowEvent(nsGUIEvent* event,
                                            nsEventStatus& aStatus);
  bool                  DispatchCommandEvent(uint32_t aEventCommand);
  bool                  DispatchDragDropEvent(uint32_t aMsg);
  bool                  DispatchMoveEvent(int32_t aX, int32_t aY);
  bool                  DispatchResizeEvent(int32_t aClientX, 
                                            int32_t aClientY);
  bool                  DispatchMouseEvent(uint32_t aEventType,
                                           MPARAM mp1, MPARAM mp2, 
                                           bool aIsContextMenuKey = false,
                                           int16_t aButton = nsMouseEvent::eLeftButton);
  bool                  DispatchActivationEvent(uint32_t aEventType);
  bool                  DispatchScrollEvent(ULONG msg, MPARAM mp1, MPARAM mp2);

  friend MRESULT EXPENTRY fnwpNSWindow(HWND hwnd, ULONG msg,
                                       MPARAM mp1, MPARAM mp2);
  friend MRESULT EXPENTRY fnwpFrame(HWND hwnd, ULONG msg,
                                    MPARAM mp1, MPARAM mp2);
  friend class os2FrameWindow;

  HWND          mWnd;               
  nsWindow*     mParent;            
  os2FrameWindow* mFrame;           
  int32_t       mWindowState;       
  bool          mIsDestroying;      
  bool          mInSetFocus;        
  bool          mNoPaint;           
  HPS           mDragHps;           
  uint32_t      mDragStatus;        
  HWND          mClipWnd;           
  HPOINTER      mCssCursorHPtr;     
  nsCOMPtr<imgIContainer> mCssCursorImg;
  nsRefPtr<gfxOS2Surface> mThebesSurface;
  bool          mIsComposing;
  nsString      mLastDispatchedCompositionString;
#ifdef DEBUG_FOCUS
  int           mWindowIdentifier;  
#endif
  InputContext  mInputContext;
};








class nsChildWindow : public nsWindow {
public:
  nsChildWindow()       {}
  ~nsChildWindow()      {}
};

#endif 



