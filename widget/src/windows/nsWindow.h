












































#ifndef Window_h__
#define Window_h__





#include "nsAutoPtr.h"
#include "nsBaseWidget.h"
#include "nsdefs.h"
#include "nsIdleService.h"
#include "nsToolkit.h"
#include "nsString.h"
#include "nsTArray.h"
#include "gfxWindowsSurface.h"
#include "nsWindowDbg.h"
#include "cairo.h"
#include "nsITimer.h"
#ifdef CAIRO_HAS_D2D_SURFACE
#include "gfxD2DSurface.h"
#endif

#if !defined(WINCE)
#include "nsWinGesture.h"
#endif

#if defined(WINCE)
#include "nsWindowCE.h"
#endif

#include "WindowHook.h"
#include "TaskbarWindowPreview.h"

#ifdef ACCESSIBILITY
#include "OLEACC.H"
#include "nsAccessible.h"
#endif

#if !defined(WINCE)
#include "nsUXThemeData.h"
#endif 




class nsNativeDragTarget;
class nsIRollupListener;
class nsIFile;
class imgIContainer;





class nsWindow : public nsBaseWidget
{
  typedef mozilla::widget::WindowHook WindowHook;
#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_WIN7
  typedef mozilla::widget::TaskbarWindowPreview TaskbarWindowPreview;
#endif
public:
  nsWindow();
  virtual ~nsWindow();

  NS_DECL_ISUPPORTS_INHERITED

  friend class nsWindowGfx;

  


  NS_IMETHOD              Create(nsIWidget *aParent,
                                 nsNativeWidget aNativeParent,
                                 const nsIntRect &aRect,
                                 EVENT_CALLBACK aHandleEventFunction,
                                 nsIDeviceContext *aContext,
                                 nsIAppShell *aAppShell = nsnull,
                                 nsIToolkit *aToolkit = nsnull,
                                 nsWidgetInitData *aInitData = nsnull);
  NS_IMETHOD              Destroy();
  NS_IMETHOD              SetParent(nsIWidget *aNewParent);
  virtual nsIWidget*      GetParent(void);
  virtual float           GetDPI();
  NS_IMETHOD              Show(PRBool bState);
  NS_IMETHOD              IsVisible(PRBool & aState);
  NS_IMETHOD              ConstrainPosition(PRBool aAllowSlop, PRInt32 *aX, PRInt32 *aY);
  NS_IMETHOD              Move(PRInt32 aX, PRInt32 aY);
  NS_IMETHOD              Resize(PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint);
  NS_IMETHOD              Resize(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint);
  NS_IMETHOD              ResizeClient(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint);
#if !defined(WINCE)
  NS_IMETHOD              BeginResizeDrag(nsGUIEvent* aEvent, PRInt32 aHorizontal, PRInt32 aVertical);
#endif
  NS_IMETHOD              PlaceBehind(nsTopLevelWidgetZPlacement aPlacement, nsIWidget *aWidget, PRBool aActivate);
  NS_IMETHOD              SetSizeMode(PRInt32 aMode);
  NS_IMETHOD              Enable(PRBool aState);
  NS_IMETHOD              IsEnabled(PRBool *aState);
  NS_IMETHOD              SetFocus(PRBool aRaise);
  NS_IMETHOD              GetBounds(nsIntRect &aRect);
  NS_IMETHOD              GetScreenBounds(nsIntRect &aRect);
  NS_IMETHOD              GetClientBounds(nsIntRect &aRect);
  virtual nsIntPoint      GetClientOffset();
  NS_IMETHOD              SetBackgroundColor(const nscolor &aColor);
  NS_IMETHOD              SetCursor(imgIContainer* aCursor,
                                    PRUint32 aHotspotX, PRUint32 aHotspotY);
  NS_IMETHOD              SetCursor(nsCursor aCursor);
  virtual nsresult        ConfigureChildren(const nsTArray<Configuration>& aConfigurations);
  NS_IMETHOD              MakeFullScreen(PRBool aFullScreen);
  NS_IMETHOD              HideWindowChrome(PRBool aShouldHide);
  NS_IMETHOD              Invalidate(PRBool aIsSynchronous);
  NS_IMETHOD              Invalidate(const nsIntRect & aRect, PRBool aIsSynchronous);
  NS_IMETHOD              Update();
  virtual void*           GetNativeData(PRUint32 aDataType);
  virtual void            FreeNativeData(void * data, PRUint32 aDataType);
  NS_IMETHOD              SetTitle(const nsAString& aTitle);
  NS_IMETHOD              SetIcon(const nsAString& aIconSpec);
  virtual nsIntPoint      WidgetToScreenOffset();
  virtual nsIntSize       ClientToWindowSize(const nsIntSize& aClientSize);
  NS_IMETHOD              DispatchEvent(nsGUIEvent* event, nsEventStatus & aStatus);
  NS_IMETHOD              EnableDragDrop(PRBool aEnable);
  NS_IMETHOD              CaptureMouse(PRBool aCapture);
  NS_IMETHOD              CaptureRollupEvents(nsIRollupListener * aListener, nsIMenuRollup * aMenuRollup,
                                              PRBool aDoCapture, PRBool aConsumeRollupEvent);
  NS_IMETHOD              GetAttention(PRInt32 aCycleCount);
  virtual PRBool          HasPendingInputEvent();
  virtual LayerManager*   GetLayerManager(LayerManagerPersistence aPersistence = LAYER_MANAGER_CURRENT, bool* aAllowRetaining = nsnull);
  gfxASurface             *GetThebesSurface();
  NS_IMETHOD              OnDefaultButtonLoaded(const nsIntRect &aButtonRect);
  NS_IMETHOD              OverrideSystemMouseScrollSpeed(PRInt32 aOriginalDelta, PRBool aIsHorizontal, PRInt32 &aOverriddenDelta);

  virtual nsresult        SynthesizeNativeKeyEvent(PRInt32 aNativeKeyboardLayout,
                                                   PRInt32 aNativeKeyCode,
                                                   PRUint32 aModifierFlags,
                                                   const nsAString& aCharacters,
                                                   const nsAString& aUnmodifiedCharacters);
  virtual nsresult        SynthesizeNativeMouseEvent(nsIntPoint aPoint,
                                                     PRUint32 aNativeMessage,
                                                     PRUint32 aModifierFlags);
  NS_IMETHOD              ResetInputState();
  NS_IMETHOD              SetIMEOpenState(PRBool aState);
  NS_IMETHOD              GetIMEOpenState(PRBool* aState);
  NS_IMETHOD              SetInputMode(const IMEContext& aContext);
  NS_IMETHOD              GetInputMode(IMEContext& aContext);
  NS_IMETHOD              CancelIMEComposition();
  NS_IMETHOD              GetToggledKeyState(PRUint32 aKeyCode, PRBool* aLEDState);
  NS_IMETHOD              RegisterTouchWindow();
  NS_IMETHOD              UnregisterTouchWindow();
#ifdef MOZ_XUL
  virtual void            SetTransparencyMode(nsTransparencyMode aMode);
  virtual nsTransparencyMode GetTransparencyMode();
  virtual void            UpdateTransparentRegion(const nsIntRegion& aTransparentRegion);
#endif 
#ifdef NS_ENABLE_TSF
  NS_IMETHOD              OnIMEFocusChange(PRBool aFocus);
  NS_IMETHOD              OnIMETextChange(PRUint32 aStart, PRUint32 aOldEnd, PRUint32 aNewEnd);
  NS_IMETHOD              OnIMESelectionChange(void);
#endif 
  NS_IMETHOD              GetNonClientMargins(nsIntMargin &margins);
  NS_IMETHOD              SetNonClientMargins(nsIntMargin &margins);
  void                    SetDrawsInTitlebar(PRBool aState);

  


  static PRInt32          GetWindowsVersion();

  


  void                    InitEvent(nsGUIEvent& event, nsIntPoint* aPoint = nsnull);
  virtual PRBool          DispatchMouseEvent(PRUint32 aEventType, WPARAM wParam,
                                             LPARAM lParam,
                                             PRBool aIsContextMenuKey = PR_FALSE,
                                             PRInt16 aButton = nsMouseEvent::eLeftButton,
                                             PRUint16 aInputSource = nsIDOMNSMouseEvent::MOZ_SOURCE_MOUSE);
  virtual PRBool          DispatchWindowEvent(nsGUIEvent* event);
  virtual PRBool          DispatchWindowEvent(nsGUIEvent*event, nsEventStatus &aStatus);
  virtual PRBool          DispatchKeyEvent(PRUint32 aEventType, WORD aCharCode,
                                           const nsTArray<nsAlternativeCharCode>* aAlternativeChars,
                                           UINT aVirtualCharCode, const MSG *aMsg,
                                           const nsModifierKeyState &aModKeyState,
                                           PRUint32 aFlags = 0);
  void                    DispatchPendingEvents();
  PRBool                  DispatchPluginEvent(UINT aMessage,
                                              WPARAM aWParam,
                                              LPARAM aLParam,
                                              PRBool aDispatchPendingEvents);

  void                    SuppressBlurEvents(PRBool aSuppress); 
  PRBool                  BlurEventsSuppressed();
#ifdef ACCESSIBILITY
  nsAccessible* DispatchAccessibleEvent(PRUint32 aEventType);
  nsAccessible* GetRootAccessible();
#endif 

  


  static void             GlobalMsgWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
  nsWindow*               GetTopLevelWindow(PRBool aStopOnDialogOrPopup);
  static HWND             GetTopLevelHWND(HWND aWnd, PRBool aStopOnDialogOrPopup = PR_FALSE);
  HWND                    GetWindowHandle() { return mWnd; }
  WNDPROC                 GetPrevWindowProc() { return mPrevWndProc; }
  static nsWindow*        GetNSWindowPtr(HWND aWnd);
  WindowHook&             GetWindowHook() { return mWindowHook; }
  nsWindow*               GetParentWindow(PRBool aIncludeOwner);
  
  typedef void            (WindowEnumCallback)(nsWindow*);
  static void             EnumAllWindows(WindowEnumCallback aCallback);

  


  virtual PRBool          AutoErase(HDC dc);
  nsIntPoint*             GetLastPoint() { return &mLastPoint; }
  
  PRBool                  PluginHasFocus() { return mIMEContext.mStatus == nsIWidget::IME_STATUS_PLUGIN; }
  PRBool                  IsTopLevelWidget() { return mIsTopWidgetWindow; }
  







  static void             StartAllowingD3D9(bool aReinitialize);

#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_WIN7
  PRBool HasTaskbarIconBeenCreated() { return mHasTaskbarIconBeenCreated; }
  
  
  void SetHasTaskbarIconBeenCreated(PRBool created = PR_TRUE) { mHasTaskbarIconBeenCreated = created; }

  
  already_AddRefed<nsITaskbarWindowPreview> GetTaskbarPreview() {
    nsCOMPtr<nsITaskbarWindowPreview> preview(do_QueryReferent(mTaskbarPreview));
    return preview.forget();
  }
  void SetTaskbarPreview(nsITaskbarWindowPreview *preview) { mTaskbarPreview = do_GetWeakReference(preview); }
#endif

  NS_IMETHOD              ReparentNativeWidget(nsIWidget* aNewParent);
protected:

  
  
  enum { eFakeTrackPointScrollableID = 0x46545053 };

  


  static LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
  static LRESULT CALLBACK WindowProcInternal(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

  static BOOL CALLBACK    BroadcastMsgToChildren(HWND aWnd, LPARAM aMsg);
  static BOOL CALLBACK    BroadcastMsg(HWND aTopWindow, LPARAM aMsg);
  static BOOL CALLBACK    DispatchStarvedPaints(HWND aTopWindow, LPARAM aMsg);
#if !defined(WINCE)
  static BOOL CALLBACK    RegisterTouchForDescendants(HWND aTopWindow, LPARAM aMsg);
  static BOOL CALLBACK    UnregisterTouchForDescendants(HWND aTopWindow, LPARAM aMsg);
#endif
  static LRESULT CALLBACK MozSpecialMsgFilter(int code, WPARAM wParam, LPARAM lParam);
  static LRESULT CALLBACK MozSpecialWndProc(int code, WPARAM wParam, LPARAM lParam);
  static LRESULT CALLBACK MozSpecialMouseProc(int code, WPARAM wParam, LPARAM lParam);
  static VOID    CALLBACK HookTimerForPopups( HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime );
  static BOOL    CALLBACK ClearResourcesCallback(HWND aChild, LPARAM aParam);
  static BOOL    CALLBACK EnumAllChildWindProc(HWND aWnd, LPARAM aParam);
  static BOOL    CALLBACK EnumAllThreadWindowProc(HWND aWnd, LPARAM aParam);
  static void             AllowD3D9Callback(nsWindow *aWindow);
  static void             AllowD3D9WithReinitializeCallback(nsWindow *aWindow);
  static BOOL CALLBACK    FindOurWindowAtPointCallback(HWND aHWND, LPARAM aLPARAM);

  


  static BOOL             SetNSWindowPtr(HWND aWnd, nsWindow * ptr);
  static PRInt32          GetMonitorCount();
  LPARAM                  lParamToScreen(LPARAM lParam);
  LPARAM                  lParamToClient(LPARAM lParam);
  virtual void            SubclassWindow(BOOL bState);
  PRBool                  CanTakeFocus();
  PRBool                  UpdateNonClientMargins(PRInt32 aSizeMode = -1, PRBool aReflowWindow = PR_TRUE);
  void                    UpdateGetWindowInfoCaptionStatus(PRBool aActiveCaption);
  void                    ResetLayout();
  void                    InvalidateNonClientRegion();
  HRGN                    ExcludeNonClientFromPaintRegion(HRGN aRegion);
#if !defined(WINCE)
  static void             InitInputWorkaroundPrefDefaults();
#endif
  static PRBool           GetInputWorkaroundPref(const char* aPrefName, PRBool aValueIfAutomatic);
  static PRBool           UseTrackPointHack();
  static void             PerformElantechSwipeGestureHack(UINT& aVirtualKeyCode, nsModifierKeyState& aModKeyState);
  static void             GetMainWindowClass(nsAString& aClass);
  PRBool                  HasGlass() const {
    return mTransparencyMode == eTransparencyGlass ||
           mTransparencyMode == eTransparencyBorderlessGlass;
  }
  static PRBool           IsOurProcessWindow(HWND aHWND);
  static HWND             FindOurProcessWindow(HWND aHWND);
  static HWND             FindOurWindowAtPoint(const POINT& aPoint);

  


  PRBool                  DispatchPluginEvent(const MSG &aMsg);
  PRBool                  DispatchFocusToTopLevelWindow(PRUint32 aEventType);
  PRBool                  DispatchFocus(PRUint32 aEventType);
  PRBool                  DispatchStandardEvent(PRUint32 aMsg);
  PRBool                  DispatchCommandEvent(PRUint32 aEventCommand);
  void                    RelayMouseEvent(UINT aMsg, WPARAM wParam, LPARAM lParam);
  static void             RemoveNextCharMessage(HWND aWnd);
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
  PRBool                  HandleScrollingPlugins(UINT aMsg, WPARAM aWParam, 
                                                 LPARAM aLParam,
                                                 PRBool& aResult,
                                                 LRESULT* aRetValue,
                                                 PRBool& aQuitProcessing);
  PRInt32                 ClientMarginHitTestPoint(PRInt32 mx, PRInt32 my);
  static WORD             GetScanCode(LPARAM aLParam)
  {
    return (aLParam >> 16) & 0xFF;
  }
  static PRBool           IsExtendedScanCode(LPARAM aLParam)
  {
    return (aLParam & 0x1000000) != 0;
  }
  static PRBool           IsRedirectedKeyDownMessage(const MSG &aMsg);
  static void             ForgetRedirectedKeyDownMessage()
  {
    sRedirectedKeyDown.message = WM_NULL;
  }

  


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
  virtual PRBool          OnScroll(UINT aMsg, WPARAM aWParam, LPARAM aLParam);
  PRBool                  OnGesture(WPARAM wParam, LPARAM lParam);
#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_WIN7
  PRBool                  OnTouch(WPARAM wParam, LPARAM lParam);
#endif
  PRBool                  OnHotKey(WPARAM wParam, LPARAM lParam);
  BOOL                    OnInputLangChange(HKL aHKL);
  void                    OnSettingsChange(WPARAM wParam, LPARAM lParam);
  PRBool                  OnPaint(HDC aDC, PRUint32 aNestingLevel);
  void                    OnWindowPosChanged(WINDOWPOS *wp, PRBool& aResult);
  PRBool                  OnMouseWheel(UINT msg, WPARAM wParam, LPARAM lParam, 
                                       PRBool& result, PRBool& getWheelInfo,
                                       LRESULT *aRetValue);
#if !defined(WINCE)
  void                    OnWindowPosChanging(LPWINDOWPOS& info);
#endif 

  



  void                    UserActivity();

  PRInt32                 GetHeight(PRInt32 aProposedHeight);
  void                    GetWindowClass(nsString& aWindowClass);
  void                    GetWindowPopupClass(nsString& aWindowClass);
  virtual DWORD           WindowStyle();
  DWORD                   WindowExStyle();

  void                    RegisterWindowClass(const nsString& aClassName,
                                              UINT aExtraStyle,
                                              LPWSTR aIconID);

  


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
  void                    UpdateGlass();
protected:
#endif 

  static bool             IsAsyncResponseEvent(UINT aMsg, LRESULT& aResult);
  void                    IPCWindowProcHandler(UINT& msg, WPARAM& wParam, LPARAM& lParam);

  


  UINT                    MapFromNativeToDOM(UINT aNativeKeyCode);
  void                    StopFlashing();
  static PRBool           IsTopLevelMouseExit(HWND aWnd);
  static void             SetupKeyModifiersSequence(nsTArray<KeyPair>* aArray, PRUint32 aModifiers);
  nsresult                SetWindowClipRegion(const nsTArray<nsIntRect>& aRects,
                                              PRBool aIntersectWithExisting);
  nsIntRegion             GetRegionToPaint(PRBool aForceFullRepaint, 
                                           PAINTSTRUCT ps, HDC aDC);
#if !defined(WINCE)
  static void             ActivateOtherWindowHelper(HWND aWnd);
  static PRUint16         GetMouseInputSource();
#endif
#ifdef ACCESSIBILITY
  static STDMETHODIMP_(LRESULT) LresultFromObject(REFIID riid, WPARAM wParam, LPUNKNOWN pAcc);
#endif 
  void                    ClearCachedResources();

  nsPopupType PopupType() { return mPopupType; }

protected:
  nsCOMPtr<nsIWidget>   mParent;
  nsIntSize             mLastSize;
  nsIntPoint            mLastPoint;
  HWND                  mWnd;
  WNDPROC               mPrevWndProc;
  HBRUSH                mBrush;
  PRPackedBool          mIsTopWidgetWindow;
  PRPackedBool          mInDtor;
  PRPackedBool          mIsVisible;
  PRPackedBool          mUnicodeWidget;
  PRPackedBool          mPainting;
  PRPackedBool          mTouchWindow;
  PRPackedBool          mDisplayPanFeedback;
  PRPackedBool          mHideChrome;
  PRPackedBool          mIsRTL;
  PRPackedBool          mFullscreenMode;
  PRPackedBool          mMousePresent;
  PRUint32              mBlurSuppressLevel;
  DWORD_PTR             mOldStyle;
  DWORD_PTR             mOldExStyle;
  HIMC                  mOldIMC;
  IMEContext            mIMEContext;
  nsNativeDragTarget*   mNativeDragTarget;
  HKL                   mLastKeyboardLayout;
  nsPopupType           mPopupType;
  nsSizeMode            mOldSizeMode;
  WindowHook            mWindowHook;
  DWORD                 mAssumeWheelIsZoomUntil;
  static PRBool         sDropShadowEnabled;
  static PRUint32       sInstanceCount;
  static TriStateBool   sCanQuit;
  static nsWindow*      sCurrentWindow;
  static BOOL           sIsOleInitialized;
  static HCURSOR        sHCursor;
  static imgIContainer* sCursorImgContainer;
  static PRBool         sSwitchKeyboardLayout;
  static PRBool         sJustGotDeactivate;
  static PRBool         sJustGotActivate;
  static PRBool         sIsInMouseCapture;
  static int            sTrimOnMinimize;
  static PRBool         sDefaultTrackPointHack;
  static const char*    sDefaultMainWindowClass;
  static PRBool         sUseElantechGestureHacks;
  static bool           sAllowD3D9;

  
  static TriStateBool   sHasBogusPopupsDropShadowOnMultiMonitor;
  static bool           HasBogusPopupsDropShadowOnMultiMonitor();

  static PRUint32       sOOPPPluginFocusEvent;

  
  
  nsIntMargin           mNonClientOffset;
  
  nsIntMargin           mNonClientMargins;

  
  PRPackedBool          mCustomNonClient;
  
  PRInt32               mHorResizeMargin;
  PRInt32               mVertResizeMargin;
  
  PRInt32               mCaptionHeight;

  nsCOMPtr<nsIdleService> mIdleService;

  
  
  
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
  static nsIMenuRollup* sMenuRollup;

  
  
  static POINT          sLastMousePoint;
  static POINT          sLastMouseMovePoint;
  static LONG           sLastMouseDownTime;
  static LONG           sLastClickCount;
  static BYTE           sLastMouseButton;

  
  HDC                   mPaintDC; 

#ifdef CAIRO_HAS_D2D_SURFACE
  nsRefPtr<gfxD2DSurface>    mD2DWindowSurface; 
#endif

  
#ifdef MOZ_XUL
  
  nsRefPtr<gfxASurface> mTransparentSurface;
  HDC                   mMemoryDC;
  nsTransparencyMode    mTransparencyMode;
#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_LONGHORN
  nsIntRegion           mPossiblyTransparentRegion;
  MARGINS               mGlassMargins;
#endif 
#endif 

  
#if !defined(WINCE)
  nsWinGesture          mGesture;
#endif 

#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_WIN7
  
  nsWeakPtr             mTaskbarPreview;
  
  
  PRBool                mHasTaskbarIconBeenCreated;
#endif

#if defined(WINCE_HAVE_SOFTKB)
  static PRBool         sSoftKeyboardState;
#endif 

#ifdef ACCESSIBILITY
  static BOOL           sIsAccessibilityOn;
  static HINSTANCE      sAccLib;
  static LPFNLRESULTFROMOBJECT sLresultFromObject;
#endif 

  
  
  static MSG            sRedirectedKeyDown;

  
  
  
  
  
  
  
  
  struct AutoForgetRedirectedKeyDownMessage
  {
    AutoForgetRedirectedKeyDownMessage(nsWindow* aWindow, const MSG &aMsg) :
      mCancel(!nsWindow::IsRedirectedKeyDownMessage(aMsg)),
      mWindow(aWindow), mMsg(aMsg)
    {
    }

    ~AutoForgetRedirectedKeyDownMessage()
    {
      if (mCancel) {
        return;
      }
      
      if (!mWindow->mOnDestroyCalled) {
        nsWindow::RemoveNextCharMessage(mWindow->mWnd);
      }
      
      nsWindow::ForgetRedirectedKeyDownMessage();
    }

    PRBool mCancel;
    nsRefPtr<nsWindow> mWindow;
    const MSG &mMsg;
  };

};




class ChildWindow : public nsWindow {

public:
  ChildWindow() {}

protected:
  virtual DWORD WindowStyle();
};

#endif 
