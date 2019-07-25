












































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
#include "mozilla/TimeStamp.h"

#ifdef CAIRO_HAS_D2D_SURFACE
#include "gfxD2DSurface.h"
#endif

#include "nsWinGesture.h"

#include "WindowHook.h"
#include "TaskbarWindowPreview.h"

#ifdef ACCESSIBILITY
#include "OLEACC.H"
#include "nsAccessible.h"
#endif

#include "nsUXThemeData.h"

#include "nsIDOMMouseEvent.h"





class nsNativeDragTarget;
class nsIRollupListener;
class nsIFile;
class imgIContainer;





class nsWindow : public nsBaseWidget
{
  typedef mozilla::TimeStamp TimeStamp;
  typedef mozilla::TimeDuration TimeDuration;
  typedef mozilla::widget::WindowHook WindowHook;
  typedef mozilla::widget::TaskbarWindowPreview TaskbarWindowPreview;
public:
  nsWindow();
  virtual ~nsWindow();

  NS_DECL_ISUPPORTS_INHERITED

  friend class nsWindowGfx;

  


  NS_IMETHOD              Create(nsIWidget *aParent,
                                 nsNativeWidget aNativeParent,
                                 const nsIntRect &aRect,
                                 EVENT_CALLBACK aHandleEventFunction,
                                 nsDeviceContext *aContext,
                                 nsWidgetInitData *aInitData = nsnull);
  NS_IMETHOD              Destroy();
  NS_IMETHOD              SetParent(nsIWidget *aNewParent);
  virtual nsIWidget*      GetParent(void);
  virtual float           GetDPI();
  NS_IMETHOD              Show(bool bState);
  NS_IMETHOD              IsVisible(bool & aState);
  NS_IMETHOD              ConstrainPosition(bool aAllowSlop, PRInt32 *aX, PRInt32 *aY);
  NS_IMETHOD              Move(PRInt32 aX, PRInt32 aY);
  NS_IMETHOD              Resize(PRInt32 aWidth, PRInt32 aHeight, bool aRepaint);
  NS_IMETHOD              Resize(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight, bool aRepaint);
  NS_IMETHOD              BeginResizeDrag(nsGUIEvent* aEvent, PRInt32 aHorizontal, PRInt32 aVertical);
  NS_IMETHOD              PlaceBehind(nsTopLevelWidgetZPlacement aPlacement, nsIWidget *aWidget, bool aActivate);
  NS_IMETHOD              SetSizeMode(PRInt32 aMode);
  NS_IMETHOD              Enable(bool aState);
  NS_IMETHOD              IsEnabled(bool *aState);
  NS_IMETHOD              SetFocus(bool aRaise);
  NS_IMETHOD              GetBounds(nsIntRect &aRect);
  NS_IMETHOD              GetScreenBounds(nsIntRect &aRect);
  NS_IMETHOD              GetClientBounds(nsIntRect &aRect);
  virtual nsIntPoint      GetClientOffset();
  NS_IMETHOD              SetBackgroundColor(const nscolor &aColor);
  NS_IMETHOD              SetCursor(imgIContainer* aCursor,
                                    PRUint32 aHotspotX, PRUint32 aHotspotY);
  NS_IMETHOD              SetCursor(nsCursor aCursor);
  virtual nsresult        ConfigureChildren(const nsTArray<Configuration>& aConfigurations);
  NS_IMETHOD              MakeFullScreen(bool aFullScreen);
  NS_IMETHOD              HideWindowChrome(bool aShouldHide);
  NS_IMETHOD              Invalidate(bool aEraseBackground = false,
                                     bool aUpdateNCArea = false,
                                     bool aIncludeChildren = false);
  NS_IMETHOD              Invalidate(const nsIntRect & aRect);
  virtual void*           GetNativeData(PRUint32 aDataType);
  virtual void            FreeNativeData(void * data, PRUint32 aDataType);
  NS_IMETHOD              SetTitle(const nsAString& aTitle);
  NS_IMETHOD              SetIcon(const nsAString& aIconSpec);
  virtual nsIntPoint      WidgetToScreenOffset();
  virtual nsIntSize       ClientToWindowSize(const nsIntSize& aClientSize);
  NS_IMETHOD              DispatchEvent(nsGUIEvent* event, nsEventStatus & aStatus);
  NS_IMETHOD              EnableDragDrop(bool aEnable);
  NS_IMETHOD              CaptureMouse(bool aCapture);
  NS_IMETHOD              CaptureRollupEvents(nsIRollupListener * aListener,
                                              bool aDoCapture, bool aConsumeRollupEvent);
  NS_IMETHOD              GetAttention(PRInt32 aCycleCount);
  virtual bool            HasPendingInputEvent();
  virtual LayerManager*   GetLayerManager(PLayersChild* aShadowManager = nsnull,
                                          LayersBackend aBackendHint = LayerManager::LAYERS_NONE,
                                          LayerManagerPersistence aPersistence = LAYER_MANAGER_CURRENT,
                                          bool* aAllowRetaining = nsnull);
  gfxASurface             *GetThebesSurface();
  NS_IMETHOD              OnDefaultButtonLoaded(const nsIntRect &aButtonRect);
  NS_IMETHOD              OverrideSystemMouseScrollSpeed(PRInt32 aOriginalDelta, bool aIsHorizontal, PRInt32 &aOverriddenDelta);

  virtual nsresult        SynthesizeNativeKeyEvent(PRInt32 aNativeKeyboardLayout,
                                                   PRInt32 aNativeKeyCode,
                                                   PRUint32 aModifierFlags,
                                                   const nsAString& aCharacters,
                                                   const nsAString& aUnmodifiedCharacters);
  virtual nsresult        SynthesizeNativeMouseEvent(nsIntPoint aPoint,
                                                     PRUint32 aNativeMessage,
                                                     PRUint32 aModifierFlags);

  virtual nsresult        SynthesizeNativeMouseMove(nsIntPoint aPoint)
                          { return SynthesizeNativeMouseEvent(aPoint, MOUSEEVENTF_MOVE, 0); }

  virtual nsresult        SynthesizeNativeMouseScrollEvent(nsIntPoint aPoint,
                                                           PRUint32 aNativeMessage,
                                                           double aDeltaX,
                                                           double aDeltaY,
                                                           double aDeltaZ,
                                                           PRUint32 aModifierFlags,
                                                           PRUint32 aAdditionalFlags);
  NS_IMETHOD              ResetInputState();
  NS_IMETHOD_(void)       SetInputContext(const InputContext& aContext,
                                          const InputContextAction& aAction);
  NS_IMETHOD_(InputContext) GetInputContext();
  NS_IMETHOD              CancelIMEComposition();
  NS_IMETHOD              GetToggledKeyState(PRUint32 aKeyCode, bool* aLEDState);
  NS_IMETHOD              RegisterTouchWindow();
  NS_IMETHOD              UnregisterTouchWindow();
#ifdef MOZ_XUL
  virtual void            SetTransparencyMode(nsTransparencyMode aMode);
  virtual nsTransparencyMode GetTransparencyMode();
  virtual void            UpdateOpaqueRegion(const nsIntRegion& aOpaqueRegion);
#endif 
#ifdef NS_ENABLE_TSF
  NS_IMETHOD              OnIMEFocusChange(bool aFocus);
  NS_IMETHOD              OnIMETextChange(PRUint32 aStart, PRUint32 aOldEnd, PRUint32 aNewEnd);
  NS_IMETHOD              OnIMESelectionChange(void);
#endif 
  NS_IMETHOD              GetNonClientMargins(nsIntMargin &margins);
  NS_IMETHOD              SetNonClientMargins(nsIntMargin &margins);
  void                    SetDrawsInTitlebar(bool aState);

  


  void                    InitEvent(nsGUIEvent& event, nsIntPoint* aPoint = nsnull);
  virtual bool            DispatchMouseEvent(PRUint32 aEventType, WPARAM wParam,
                                             LPARAM lParam,
                                             bool aIsContextMenuKey = false,
                                             PRInt16 aButton = nsMouseEvent::eLeftButton,
                                             PRUint16 aInputSource = nsIDOMMouseEvent::MOZ_SOURCE_MOUSE);
  virtual bool            DispatchWindowEvent(nsGUIEvent* event);
  virtual bool            DispatchWindowEvent(nsGUIEvent*event, nsEventStatus &aStatus);
  virtual bool            DispatchKeyEvent(PRUint32 aEventType, WORD aCharCode,
                                           const nsTArray<nsAlternativeCharCode>* aAlternativeChars,
                                           UINT aVirtualCharCode, const MSG *aMsg,
                                           const nsModifierKeyState &aModKeyState,
                                           PRUint32 aFlags = 0);
  void                    DispatchPendingEvents();
  bool                    DispatchPluginEvent(UINT aMessage,
                                              WPARAM aWParam,
                                              LPARAM aLParam,
                                              bool aDispatchPendingEvents);

  void                    SuppressBlurEvents(bool aSuppress); 
  bool                    BlurEventsSuppressed();
#ifdef ACCESSIBILITY
  nsAccessible* DispatchAccessibleEvent(PRUint32 aEventType);
  nsAccessible* GetRootAccessible();
#endif 

  


  nsWindow*               GetTopLevelWindow(bool aStopOnDialogOrPopup);
  HWND                    GetWindowHandle() { return mWnd; }
  WNDPROC                 GetPrevWindowProc() { return mPrevWndProc; }
  WindowHook&             GetWindowHook() { return mWindowHook; }
  nsWindow*               GetParentWindow(bool aIncludeOwner);
  
  typedef void            (WindowEnumCallback)(nsWindow*);
  static void             EnumAllWindows(WindowEnumCallback aCallback);

  


  virtual bool            AutoErase(HDC dc);
  nsIntPoint*             GetLastPoint() { return &mLastPoint; }
  
  bool                    PluginHasFocus()
  {
    return (mInputContext.mIMEState.mEnabled == IMEState::PLUGIN);
  }
  bool                    IsTopLevelWidget() { return mIsTopWidgetWindow; }
  







  static void             StartAllowingD3D9(bool aReinitialize);

  












  bool                    AssociateDefaultIMC(bool aAssociate);

  bool HasTaskbarIconBeenCreated() { return mHasTaskbarIconBeenCreated; }
  
  
  void SetHasTaskbarIconBeenCreated(bool created = true) { mHasTaskbarIconBeenCreated = created; }

  
  already_AddRefed<nsITaskbarWindowPreview> GetTaskbarPreview() {
    nsCOMPtr<nsITaskbarWindowPreview> preview(do_QueryReferent(mTaskbarPreview));
    return preview.forget();
  }
  void SetTaskbarPreview(nsITaskbarWindowPreview *preview) { mTaskbarPreview = do_GetWeakReference(preview); }

  NS_IMETHOD              ReparentNativeWidget(nsIWidget* aNewParent);

  
  void                    PickerOpen();
  void                    PickerClosed();

  bool                    const DestroyCalled() { return mDestroyCalled; }

  static void             SetupKeyModifiersSequence(nsTArray<KeyPair>* aArray, PRUint32 aModifiers);
protected:

  
  
  enum { eFakeTrackPointScrollableID = 0x46545053 };

  


  static LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
  static LRESULT CALLBACK WindowProcInternal(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

  static BOOL CALLBACK    BroadcastMsgToChildren(HWND aWnd, LPARAM aMsg);
  static BOOL CALLBACK    BroadcastMsg(HWND aTopWindow, LPARAM aMsg);
  static BOOL CALLBACK    DispatchStarvedPaints(HWND aTopWindow, LPARAM aMsg);
  static BOOL CALLBACK    RegisterTouchForDescendants(HWND aTopWindow, LPARAM aMsg);
  static BOOL CALLBACK    UnregisterTouchForDescendants(HWND aTopWindow, LPARAM aMsg);
  static LRESULT CALLBACK MozSpecialMsgFilter(int code, WPARAM wParam, LPARAM lParam);
  static LRESULT CALLBACK MozSpecialWndProc(int code, WPARAM wParam, LPARAM lParam);
  static LRESULT CALLBACK MozSpecialMouseProc(int code, WPARAM wParam, LPARAM lParam);
  static VOID    CALLBACK HookTimerForPopups( HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime );
  static BOOL    CALLBACK ClearResourcesCallback(HWND aChild, LPARAM aParam);
  static BOOL    CALLBACK EnumAllChildWindProc(HWND aWnd, LPARAM aParam);
  static BOOL    CALLBACK EnumAllThreadWindowProc(HWND aWnd, LPARAM aParam);
  static void             AllowD3D9Callback(nsWindow *aWindow);
  static void             AllowD3D9WithReinitializeCallback(nsWindow *aWindow);

  


  LPARAM                  lParamToScreen(LPARAM lParam);
  LPARAM                  lParamToClient(LPARAM lParam);
  virtual void            SubclassWindow(BOOL bState);
  bool                    CanTakeFocus();
  bool                    UpdateNonClientMargins(PRInt32 aSizeMode = -1, bool aReflowWindow = true);
  void                    UpdateGetWindowInfoCaptionStatus(bool aActiveCaption);
  void                    ResetLayout();
  void                    InvalidateNonClientRegion();
  HRGN                    ExcludeNonClientFromPaintRegion(HRGN aRegion);
  static void             GetMainWindowClass(nsAString& aClass);
  bool                    HasGlass() const {
    return mTransparencyMode == eTransparencyGlass ||
           mTransparencyMode == eTransparencyBorderlessGlass;
  }

  


  bool                    DispatchPluginEvent(const MSG &aMsg);
  bool                    DispatchFocusToTopLevelWindow(PRUint32 aEventType);
  bool                    DispatchFocus(PRUint32 aEventType);
  bool                    DispatchStandardEvent(PRUint32 aMsg);
  bool                    DispatchCommandEvent(PRUint32 aEventCommand);
  void                    RelayMouseEvent(UINT aMsg, WPARAM wParam, LPARAM lParam);
  static void             RemoveNextCharMessage(HWND aWnd);
  void                    RemoveMessageAndDispatchPluginEvent(UINT aFirstMsg,
                            UINT aLastMsg,
                            nsFakeCharMessage* aFakeCharMessage = nsnull);
  virtual bool            ProcessMessage(UINT msg, WPARAM &wParam,
                                         LPARAM &lParam, LRESULT *aRetValue);
  bool                    ProcessMessageForPlugin(const MSG &aMsg,
                                                  LRESULT *aRetValue, bool &aCallDefWndProc);
  LRESULT                 ProcessCharMessage(const MSG &aMsg,
                                             bool *aEventDispatched);
  LRESULT                 ProcessKeyUpMessage(const MSG &aMsg,
                                              bool *aEventDispatched);
  LRESULT                 ProcessKeyDownMessage(const MSG &aMsg,
                                                bool *aEventDispatched);
  static bool             EventIsInsideWindow(UINT Msg, nsWindow* aWindow);
  
  static bool             ConvertStatus(nsEventStatus aStatus);
  static void             PostSleepWakeNotification(const bool aIsSleepMode);
  PRInt32                 ClientMarginHitTestPoint(PRInt32 mx, PRInt32 my);
  static bool             IsRedirectedKeyDownMessage(const MSG &aMsg);
  static void             ForgetRedirectedKeyDownMessage()
  {
    sRedirectedKeyDown.message = WM_NULL;
  }

  


  virtual void            OnDestroy();
  virtual bool            OnMove(PRInt32 aX, PRInt32 aY);
  virtual bool            OnResize(nsIntRect &aWindowRect);
  LRESULT                 OnChar(const MSG &aMsg,
                                 nsModifierKeyState &aModKeyState,
                                 bool *aEventDispatched,
                                 PRUint32 aFlags = 0);
  LRESULT                 OnKeyDown(const MSG &aMsg,
                                    nsModifierKeyState &aModKeyState,
                                    bool *aEventDispatched,
                                    nsFakeCharMessage* aFakeCharMessage);
  LRESULT                 OnKeyUp(const MSG &aMsg,
                                  nsModifierKeyState &aModKeyState,
                                  bool *aEventDispatched);
  LRESULT                 OnCharRaw(UINT charCode, UINT aScanCode,
                                    nsModifierKeyState &aModKeyState,
                                    PRUint32 aFlags = 0,
                                    const MSG *aMsg = nsnull,
                                    bool *aEventDispatched = nsnull);
  bool                    OnGesture(WPARAM wParam, LPARAM lParam);
  bool                    OnTouch(WPARAM wParam, LPARAM lParam);
  bool                    OnHotKey(WPARAM wParam, LPARAM lParam);
  BOOL                    OnInputLangChange(HKL aHKL);
  bool                    OnPaint(HDC aDC, PRUint32 aNestingLevel);
  void                    OnWindowPosChanged(WINDOWPOS *wp, bool& aResult);
  void                    OnWindowPosChanging(LPWINDOWPOS& info);
  void                    OnSysColorChanged();

  



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
  void                    ResizeTranslucentWindow(PRInt32 aNewWidth, PRInt32 aNewHeight, bool force = false);
  nsresult                UpdateTranslucentWindow();
  void                    SetupTranslucentWindowMemoryBitmap(nsTransparencyMode aMode);
  void                    UpdateGlass();
protected:
#endif 

  static bool             IsAsyncResponseEvent(UINT aMsg, LRESULT& aResult);
  void                    IPCWindowProcHandler(UINT& msg, WPARAM& wParam, LPARAM& lParam);

  


  UINT                    MapFromNativeToDOM(UINT aNativeKeyCode);
  void                    StopFlashing();
  static bool             IsTopLevelMouseExit(HWND aWnd);
  nsresult                SetWindowClipRegion(const nsTArray<nsIntRect>& aRects,
                                              bool aIntersectWithExisting);
  nsIntRegion             GetRegionToPaint(bool aForceFullRepaint, 
                                           PAINTSTRUCT ps, HDC aDC);
  static void             ActivateOtherWindowHelper(HWND aWnd);
  void                    ClearCachedResources();

  nsPopupType PopupType() { return mPopupType; }

protected:
  nsCOMPtr<nsIWidget>   mParent;
  nsIntSize             mLastSize;
  nsIntPoint            mLastPoint;
  HWND                  mWnd;
  WNDPROC               mPrevWndProc;
  HBRUSH                mBrush;
  bool                  mIsTopWidgetWindow;
  bool                  mInDtor;
  bool                  mIsVisible;
  bool                  mUnicodeWidget;
  bool                  mPainting;
  bool                  mTouchWindow;
  bool                  mDisplayPanFeedback;
  bool                  mHideChrome;
  bool                  mIsRTL;
  bool                  mFullscreenMode;
  bool                  mMousePresent;
  bool                  mDestroyCalled;
  PRUint32              mBlurSuppressLevel;
  DWORD_PTR             mOldStyle;
  DWORD_PTR             mOldExStyle;
  InputContext mInputContext;
  nsNativeDragTarget*   mNativeDragTarget;
  HKL                   mLastKeyboardLayout;
  nsPopupType           mPopupType;
  nsSizeMode            mOldSizeMode;
  WindowHook            mWindowHook;
  DWORD                 mAssumeWheelIsZoomUntil;
  PRUint32              mPickerDisplayCount;
  static bool           sDropShadowEnabled;
  static PRUint32       sInstanceCount;
  static TriStateBool   sCanQuit;
  static nsWindow*      sCurrentWindow;
  static BOOL           sIsOleInitialized;
  static HCURSOR        sHCursor;
  static imgIContainer* sCursorImgContainer;
  static bool           sSwitchKeyboardLayout;
  static bool           sJustGotDeactivate;
  static bool           sJustGotActivate;
  static bool           sIsInMouseCapture;
  static int            sTrimOnMinimize;
  static const char*    sDefaultMainWindowClass;
  static bool           sAllowD3D9;

  
  static TriStateBool   sHasBogusPopupsDropShadowOnMultiMonitor;
  static bool           HasBogusPopupsDropShadowOnMultiMonitor();

  static PRUint32       sOOPPPluginFocusEvent;

  
  
  nsIntMargin           mNonClientOffset;
  
  nsIntMargin           mNonClientMargins;

  
  bool                  mCustomNonClient;
  
  PRInt32               mHorResizeMargin;
  PRInt32               mVertResizeMargin;
  
  PRInt32               mCaptionHeight;

  nsCOMPtr<nsIdleService> mIdleService;

  
  
  
  static HHOOK          sMsgFilterHook;
  static HHOOK          sCallProcHook;
  static HHOOK          sCallMouseHook;
  static bool           sProcessHook;
  static UINT           sRollupMsgId;
  static HWND           sRollupMsgWnd;
  static UINT           sHookTimerId;

  
  static nsIWidget*     sRollupWidget;
  static bool           sRollupConsumeEvent;
  static nsIRollupListener* sRollupListener;

  
  
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
  nsIntRegion           mPossiblyTransparentRegion;
  MARGINS               mGlassMargins;
#endif 

  
  nsWinGesture          mGesture;

  
  nsWeakPtr             mTaskbarPreview;
  
  
  bool                  mHasTaskbarIconBeenCreated;

  
  
  TimeStamp mLastPaintEndTime;

  
  
  static MSG            sRedirectedKeyDown;

  static bool sNeedsToInitMouseWheelSettings;
  static void InitMouseWheelScrollData();

  
  
  
  
  
  
  
  
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

    bool mCancel;
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
