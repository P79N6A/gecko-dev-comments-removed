











































#ifndef Window_h__
#define Window_h__





#include "nsBaseWidget.h"
#include "nsdefs.h"
#include "nsSwitchToUIThread.h"
#include "nsToolkit.h"
#include "nsIEventListener.h"
#include "nsString.h"
#include "nsTArray.h"
#include "gfxWindowsSurface.h"
#include "nsWindowDbg.h"
#include "cairo.h"

#if !defined(WINCE)
#include "nsWinGesture.h"
#endif

#if defined(WINCE)
#include "nsWindowCE.h"
#endif

#ifdef ACCESSIBILITY
#include "OLEACC.H"
#include "nsIAccessible.h"
#endif


#if !defined(WINCE)
#define NS_ENABLE_TSF
#endif 





class nsNativeDragTarget;
class nsIRollupListener;
class nsIFile;
class imgIContainer;





class nsWindow : public nsSwitchToUIThread,
                 public nsBaseWidget
{
public:
  nsWindow();
  virtual ~nsWindow();

  NS_DECL_ISUPPORTS_INHERITED

  friend class nsWindowGfx;

  


  NS_IMETHOD              Create(nsIWidget *aParent,
                                 const nsIntRect &aRect,
                                 EVENT_CALLBACK aHandleEventFunction,
                                 nsIDeviceContext *aContext,
                                 nsIAppShell *aAppShell = nsnull,
                                 nsIToolkit *aToolkit = nsnull,
                                 nsWidgetInitData *aInitData = nsnull);
  NS_IMETHOD              Create(nsNativeWidget aParent,
                                 const nsIntRect &aRect,
                                 EVENT_CALLBACK aHandleEventFunction,
                                 nsIDeviceContext *aContext,
                                 nsIAppShell *aAppShell = nsnull,
                                 nsIToolkit *aToolkit = nsnull,
                                 nsWidgetInitData *aInitData = nsnull);
  NS_IMETHOD              Destroy();
  NS_IMETHOD              SetParent(nsIWidget *aNewParent);
  virtual nsIWidget*      GetParent(void);
  NS_IMETHOD              Show(PRBool bState);
  NS_IMETHOD              IsVisible(PRBool & aState);
  NS_IMETHOD              ConstrainPosition(PRBool aAllowSlop, PRInt32 *aX, PRInt32 *aY);
  NS_IMETHOD              Move(PRInt32 aX, PRInt32 aY);
  NS_IMETHOD              Resize(PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint);
  NS_IMETHOD              Resize(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint);
  NS_IMETHOD              PlaceBehind(nsTopLevelWidgetZPlacement aPlacement, nsIWidget *aWidget, PRBool aActivate);
  NS_IMETHOD              SetSizeMode(PRInt32 aMode);
  NS_IMETHOD              Enable(PRBool aState);
  NS_IMETHOD              IsEnabled(PRBool *aState);
  NS_IMETHOD              SetFocus(PRBool aRaise);
  NS_IMETHOD              GetBounds(nsIntRect &aRect);
  NS_IMETHOD              GetScreenBounds(nsIntRect &aRect);
  NS_IMETHOD              GetClientBounds(nsIntRect &aRect);
  NS_IMETHOD              SetBackgroundColor(const nscolor &aColor);
  NS_IMETHOD              SetCursor(imgIContainer* aCursor,
                                    PRUint32 aHotspotX, PRUint32 aHotspotY);
  NS_IMETHOD              SetCursor(nsCursor aCursor);
  NS_IMETHOD              MakeFullScreen(PRBool aFullScreen);
  NS_IMETHOD              HideWindowChrome(PRBool aShouldHide);
  NS_IMETHOD              Validate();
  NS_IMETHOD              Invalidate(PRBool aIsSynchronous);
  NS_IMETHOD              Invalidate(const nsIntRect & aRect, PRBool aIsSynchronous);
  NS_IMETHOD              Update();
  NS_IMETHOD              Scroll(PRInt32 aDx, PRInt32 aDy, nsIntRect *aClipRect);
  virtual void*           GetNativeData(PRUint32 aDataType);
  virtual void            FreeNativeData(void * data, PRUint32 aDataType);
  NS_IMETHOD              SetTitle(const nsAString& aTitle);
  NS_IMETHOD              SetIcon(const nsAString& aIconSpec);
  virtual nsIntPoint      WidgetToScreenOffset();
  NS_IMETHOD              BeginResizingChildren(void);
  NS_IMETHOD              EndResizingChildren(void);
  NS_IMETHOD              DispatchEvent(nsGUIEvent* event, nsEventStatus & aStatus);
  NS_IMETHOD              EnableDragDrop(PRBool aEnable);
  NS_IMETHOD              CaptureMouse(PRBool aCapture);
  NS_IMETHOD              CaptureRollupEvents(nsIRollupListener * aListener, PRBool aDoCapture, PRBool aConsumeRollupEvent);
  NS_IMETHOD              GetAttention(PRInt32 aCycleCount);
  virtual PRBool          HasPendingInputEvent();
  gfxASurface             *GetThebesSurface();
  virtual nsresult        SynthesizeNativeKeyEvent(PRInt32 aNativeKeyboardLayout,
                                                   PRInt32 aNativeKeyCode,
                                                   PRUint32 aModifierFlags,
                                                   const nsAString& aCharacters,
                                                   const nsAString& aUnmodifiedCharacters);
  NS_IMETHOD              ResetInputState();
  NS_IMETHOD              SetIMEOpenState(PRBool aState);
  NS_IMETHOD              GetIMEOpenState(PRBool* aState);
  NS_IMETHOD              SetIMEEnabled(PRUint32 aState);
  NS_IMETHOD              GetIMEEnabled(PRUint32* aState);
  NS_IMETHOD              CancelIMEComposition();
  NS_IMETHOD              GetToggledKeyState(PRUint32 aKeyCode, PRBool* aLEDState);
#ifdef MOZ_XUL
  virtual void            SetTransparencyMode(nsTransparencyMode aMode);
  virtual nsTransparencyMode GetTransparencyMode();
#endif 
#ifdef NS_ENABLE_TSF
  NS_IMETHOD              OnIMEFocusChange(PRBool aFocus);
  NS_IMETHOD              OnIMETextChange(PRUint32 aStart, PRUint32 aOldEnd, PRUint32 aNewEnd);
  NS_IMETHOD              OnIMESelectionChange(void);
#endif 

  


  virtual BOOL            CallMethod(MethodInfo *info);

  


  static PRInt32          GetWindowsVersion();

  


  void                    InitEvent(nsGUIEvent& event, nsIntPoint* aPoint = nsnull);
  virtual PRBool          DispatchMouseEvent(PRUint32 aEventType, WPARAM wParam,
                                             LPARAM lParam,
                                             PRBool aIsContextMenuKey = PR_FALSE,
                                             PRInt16 aButton = nsMouseEvent::eLeftButton);
  virtual PRBool          DispatchWindowEvent(nsGUIEvent* event);
  virtual PRBool          DispatchWindowEvent(nsGUIEvent*event, nsEventStatus &aStatus);
  virtual PRBool          DispatchKeyEvent(PRUint32 aEventType, WORD aCharCode,
                                           const nsTArray<nsAlternativeCharCode>* aAlternativeChars,
                                           UINT aVirtualCharCode, const MSG *aMsg,
                                           const nsModifierKeyState &aModKeyState,
                                           PRUint32 aFlags = 0);
  void                    SuppressBlurEvents(PRBool aSuppress); 
  PRBool                  BlurEventsSuppressed();
#ifdef ACCESSIBILITY
  virtual PRBool          DispatchAccessibleEvent(PRUint32 aEventType, nsIAccessible** aAccessible, nsIntPoint* aPoint = nsnull);
  already_AddRefed<nsIAccessible> GetRootAccessible();
#endif 

  


  static void             GlobalMsgWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
  nsWindow*               GetTopLevelWindow(PRBool aStopOnDialogOrPopup);
  static HWND             GetTopLevelHWND(HWND aWnd, PRBool aStopOnDialogOrPopup = PR_FALSE);
  HWND                    GetWindowHandle() { return mWnd; }
  WNDPROC                 GetPrevWindowProc() { return mPrevWndProc; }

  


  virtual PRBool          AutoErase();
  nsIntPoint*             GetLastPoint() { return &mLastPoint; }
  PRInt32                 GetNewCmdMenuId() { mMenuCmdId++; return mMenuCmdId; }
  PRBool                  GetIMEEnabled() { return mIMEEnabled; }
  
  PRBool                  PluginHasFocus() { return mIMEEnabled == nsIWidget::IME_STATUS_PLUGIN; }
  virtual void            SetUpForPaint(HDC aHDC);

protected:

  


  static LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
  static BOOL CALLBACK    BroadcastMsgToChildren(HWND aWnd, LPARAM aMsg);
  static BOOL CALLBACK    BroadcastMsg(HWND aTopWindow, LPARAM aMsg);
  static BOOL CALLBACK    DispatchStarvedPaints(HWND aTopWindow, LPARAM aMsg);
  static BOOL CALLBACK    InvalidateForeignChildWindows(HWND aWnd, LPARAM aMsg);
  static LRESULT CALLBACK MozSpecialMsgFilter(int code, WPARAM wParam, LPARAM lParam);
  static LRESULT CALLBACK MozSpecialWndProc(int code, WPARAM wParam, LPARAM lParam);
  static LRESULT CALLBACK MozSpecialMouseProc(int code, WPARAM wParam, LPARAM lParam);
  static VOID    CALLBACK HookTimerForPopups( HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime );

  


  static nsWindow*        GetNSWindowPtr(HWND aWnd);
  static BOOL             SetNSWindowPtr(HWND aWnd, nsWindow * ptr);
  LPARAM                  lParamToScreen(LPARAM lParam);
  LPARAM                  lParamToClient(LPARAM lParam);
  nsWindow*               GetParentWindow(PRBool aIncludeOwner);
  virtual void            SubclassWindow(BOOL bState);
  void                    GetNonClientBounds(nsIntRect &aRect);
  PRBool                  CanTakeFocus();

  


  void                    DispatchPendingEvents();
  PRBool                  DispatchPluginEvent(const MSG &aMsg);
  PRBool                  DispatchFocusToTopLevelWindow(PRUint32 aEventType);
  PRBool                  DispatchFocus(PRUint32 aEventType);
  PRBool                  DispatchStandardEvent(PRUint32 aMsg);
  PRBool                  DispatchCommandEvent(PRUint32 aEventCommand);
  void                    RelayMouseEvent(UINT aMsg, WPARAM wParam, LPARAM lParam);
  void                    RemoveMessageAndDispatchPluginEvent(UINT aFirstMsg, UINT aLastMsg);
  static MSG              InitMSG(UINT aMessage, WPARAM wParam, LPARAM lParam);
  virtual PRBool          ProcessMessage(UINT msg, WPARAM &wParam,
                                         LPARAM &lParam, LRESULT *aRetValue);
  PRBool                  ProcessMessageForPlugin(const MSG &aMsg,
                                                  LRESULT *aRetValue, PRBool &aCallDefWndProc);
  LRESULT                 ProcessCharMessage(const MSG &aMsg,
                                             PRBool *aEventDispatched);
  LRESULT                 ProcessKeyUpMessage(const MSG &aMsg,
                                              PRBool *aEventDispatched);
  LRESULT                 ProcessKeyDownMessage(const MSG &aMsg,
                                                PRBool *aEventDispatched);
  static PRBool           EventIsInsideWindow(UINT Msg, nsWindow* aWindow);
  
  static PRBool           ConvertStatus(nsEventStatus aStatus);
  static void             PostSleepWakeNotification(const char* aNotification);

  


  virtual void            OnDestroy();
  virtual PRBool          OnMove(PRInt32 aX, PRInt32 aY);
  virtual PRBool          OnResize(nsIntRect &aWindowRect);
  LRESULT                 OnChar(const MSG &aMsg,
                                 nsModifierKeyState &aModKeyState,
                                 PRBool *aEventDispatched,
                                 PRUint32 aFlags = 0);
  LRESULT                 OnKeyDown(const MSG &aMsg,
                                    nsModifierKeyState &aModKeyState,
                                    PRBool *aEventDispatched,
                                    nsFakeCharMessage* aFakeCharMessage);
  LRESULT                 OnKeyUp(const MSG &aMsg,
                                  nsModifierKeyState &aModKeyState,
                                  PRBool *aEventDispatched);
  LRESULT                 OnCharRaw(UINT charCode, UINT aScanCode,
                                    nsModifierKeyState &aModKeyState,
                                    PRUint32 aFlags = 0,
                                    const MSG *aMsg = nsnull,
                                    PRBool *aEventDispatched = nsnull);
  virtual PRBool          OnScroll(UINT scrollCode, int cPos);
  virtual HBRUSH          OnControlColor();
  PRBool                  OnGesture(WPARAM wParam, LPARAM lParam);
  PRBool                  OnHotKey(WPARAM wParam, LPARAM lParam);
  BOOL                    OnInputLangChange(HKL aHKL);
  void                    OnSettingsChange(WPARAM wParam, LPARAM lParam);
  virtual PRBool          OnPaint(HDC aDC = nsnull);
#if defined(CAIRO_HAS_DDRAW_SURFACE)
  PRBool                  OnPaintImageDDraw16();
#endif 
#if !defined(WINCE)
  PRBool                  OnMouseWheel(UINT msg, WPARAM wParam, LPARAM lParam, 
                                       PRBool& result, PRBool& getWheelInfo,
                                       LRESULT *aRetValue);
  void                    OnWindowPosChanging(LPWINDOWPOS& info);
#endif 

  


  virtual PRInt32         GetHeight(PRInt32 aProposedHeight);
  virtual LPCWSTR         WindowClass();
  virtual LPCWSTR         WindowPopupClass();
  virtual DWORD           WindowStyle();
  virtual DWORD           WindowExStyle();
  virtual nsresult        StandardWindowCreate(nsIWidget *aParent,
                                               const nsIntRect &aRect,
                                               EVENT_CALLBACK aHandleEventFunction,
                                               nsIDeviceContext *aContext,
                                               nsIAppShell *aAppShell,
                                               nsIToolkit *aToolkit,
                                               nsWidgetInitData *aInitData,
                                               nsNativeWidget aNativeParent = nsnull);

  


  void                    ClearThemeRegion();
  void                    SetThemeRegion();

  


  static void             ScheduleHookTimer(HWND aWnd, UINT aMsgId);
  static void             RegisterSpecialDropdownHooks();
  static void             UnregisterSpecialDropdownHooks();
  static BOOL             DealWithPopups(HWND inWnd, UINT inMsg, WPARAM inWParam, LPARAM inLParam, LRESULT* outResult);

  


#ifdef MOZ_XUL
private:
  void                    SetWindowTranslucencyInner(nsTransparencyMode aMode);
  nsTransparencyMode      GetWindowTranslucencyInner() const { return mTransparencyMode; }
  void                    ResizeTranslucentWindow(PRInt32 aNewWidth, PRInt32 aNewHeight, PRBool force = PR_FALSE);
  nsresult                UpdateTranslucentWindow();
  void                    SetupTranslucentWindowMemoryBitmap(nsTransparencyMode aMode);
protected:
#endif 

  


  static PRUint8*         Data32BitTo1Bit(PRUint8* aImageData, PRUint32 aWidth, PRUint32 aHeight);
  static PRBool           IsCursorTranslucencySupported();
  static HBITMAP          DataToBitmap(PRUint8* aImageData, PRUint32 aWidth, PRUint32 aHeight, PRUint32 aDepth);

  


  UINT                    MapFromNativeToDOM(UINT aNativeKeyCode);
  void                    StopFlashing();
  static PRBool           IsTopLevelMouseExit(HWND aWnd);
  static void             SetupKeyModifiersSequence(nsTArray<KeyPair>* aArray, PRUint32 aModifiers);

#ifdef ACCESSIBILITY
  static STDMETHODIMP_(LRESULT) LresultFromObject(REFIID riid, WPARAM wParam, LPUNKNOWN pAcc);
#endif 

protected:

  


  enum {
    
    
    CREATE = 0x0101,
    CREATE_NATIVE,
    DESTROY,
    SET_FOCUS,
    SET_CURSOR,
    CREATE_HACK
  };

protected:
  nsIntSize             mLastSize;
  nsIntPoint            mLastPoint;
  HWND                  mWnd;
  WNDPROC               mPrevWndProc;
  HBRUSH                mBrush;
  PRPackedBool          mIsTopWidgetWindow;
  PRPackedBool          mHas3DBorder;
  PRPackedBool          mIsDestroying;
  PRPackedBool          mIsVisible;
  PRPackedBool          mIsInMouseCapture;
  PRPackedBool          mInWheelProcessing;
  PRPackedBool          mUnicodeWidget;
  PRPackedBool          mIsPluginWindow;
  PRPackedBool          mPainting;
  char                  mLeadByte;
  PRUint32              mBlurSuppressLevel;
  nsContentType         mContentType;
  PRInt32               mPreferredWidth;
  PRInt32               mPreferredHeight;
  PRInt32               mMenuCmdId;
  HDWP                  mDeferredPositioner;
  DWORD_PTR             mOldStyle;
  DWORD_PTR             mOldExStyle;
  HIMC                  mOldIMC;
  PRUint32              mIMEEnabled;
  nsNativeDragTarget*   mNativeDragTarget;
  HKL                   mLastKeyboardLayout;
  nsPopupType           mPopupType;

  static PRUint32       sInstanceCount;
  static TriStateBool   sCanQuit;
  static nsWindow*      sCurrentWindow;
  static BOOL           sIsRegistered;
  static BOOL           sIsPopupClassRegistered;
  static BOOL           sIsOleInitialized;
  static HCURSOR        sHCursor;
  static imgIContainer* sCursorImgContainer;
  static PRBool         sSwitchKeyboardLayout;
  static PRBool         sJustGotDeactivate;
  static PRBool         sJustGotActivate;
  static int            sTrimOnMinimize;

  
  
  
  static HHOOK          sMsgFilterHook;
  static HHOOK          sCallProcHook;
  static HHOOK          sCallMouseHook;
  static PRPackedBool   sProcessHook;
  static UINT           sRollupMsgId;
  static HWND           sRollupMsgWnd;
  static UINT           sHookTimerId;

  
  static nsIWidget*     sRollupWidget;
  static PRBool         sRollupConsumeEvent;
  static nsIRollupListener* sRollupListener;

  
  
  static POINT          sLastMousePoint;
  static POINT          sLastMouseMovePoint;
  static LONG           sLastMouseDownTime;
  static LONG           sLastClickCount;
  static BYTE           sLastMouseButton;

  
  HDC                   mPaintDC; 
  static WinRenderMode  sRenderMode;
  static nsAutoPtr<PRUint8> sSharedSurfaceData;
  static gfxIntSize     sSharedSurfaceSize;

  
#ifdef MOZ_XUL
  
  nsRefPtr<gfxWindowsSurface> mTransparentSurface;
  HDC                   mMemoryDC;
  nsTransparencyMode    mTransparencyMode;
#endif 

  
#if !defined(WINCE)
  nsWinGesture          mGesture;
#endif 

#if defined(WINCE_HAVE_SOFTKB)
  static PRBool         sSoftKeyMenuBar;
  static PRBool         sSoftKeyboardState;
#endif 

#ifdef ACCESSIBILITY
  static BOOL           sIsAccessibilityOn;
  static HINSTANCE      sAccLib;
  static LPFNLRESULTFROMOBJECT sLresultFromObject;
#endif 
};




class ChildWindow : public nsWindow {

public:
  ChildWindow() {}
  PRBool DispatchMouseEvent(PRUint32 aEventType, WPARAM wParam, LPARAM lParam,
                            PRBool aIsContextMenuKey = PR_FALSE,
                            PRInt16 aButton = nsMouseEvent::eLeftButton);

protected:
  virtual DWORD WindowStyle();
};

#endif 
