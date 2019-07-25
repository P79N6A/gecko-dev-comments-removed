



































#ifndef nsBaseWidget_h__
#define nsBaseWidget_h__

#include "nsRect.h"
#include "nsIWidget.h"
#include "nsWidgetsCID.h"
#include "nsILocalFile.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsGUIEvent.h"
#include "nsAutoPtr.h"
#include "BasicLayers.h"

class nsIContent;
class nsAutoRollup;
class gfxContext;










class nsBaseWidget : public nsIWidget
{
  friend class nsAutoRollup;

protected:
  typedef mozilla::layers::BasicLayerManager BasicLayerManager;

public:
  nsBaseWidget();
  virtual ~nsBaseWidget();
  
  NS_DECL_ISUPPORTS
  
  
  NS_IMETHOD              CaptureMouse(bool aCapture);
  NS_IMETHOD              GetClientData(void*& aClientData);
  NS_IMETHOD              SetClientData(void* aClientData);
  NS_IMETHOD              Destroy();
  NS_IMETHOD              SetParent(nsIWidget* aNewParent);
  virtual nsIWidget*      GetParent(void);
  virtual nsIWidget*      GetTopLevelWidget();
  virtual nsIWidget*      GetSheetWindowParent(void);
  virtual float           GetDPI();
  virtual double          GetDefaultScale();
  virtual void            AddChild(nsIWidget* aChild);
  virtual void            RemoveChild(nsIWidget* aChild);

  NS_IMETHOD              SetZIndex(PRInt32 aZIndex);
  NS_IMETHOD              GetZIndex(PRInt32* aZIndex);
  NS_IMETHOD              PlaceBehind(nsTopLevelWidgetZPlacement aPlacement,
                                      nsIWidget *aWidget, bool aActivate);

  NS_IMETHOD              SetSizeMode(PRInt32 aMode);
  NS_IMETHOD              GetSizeMode(PRInt32* aMode);

  virtual nscolor         GetForegroundColor(void);
  NS_IMETHOD              SetForegroundColor(const nscolor &aColor);
  virtual nscolor         GetBackgroundColor(void);
  NS_IMETHOD              SetBackgroundColor(const nscolor &aColor);
  virtual nsCursor        GetCursor();
  NS_IMETHOD              SetCursor(nsCursor aCursor);
  NS_IMETHOD              SetCursor(imgIContainer* aCursor,
                                    PRUint32 aHotspotX, PRUint32 aHotspotY);
  NS_IMETHOD              GetWindowType(nsWindowType& aWindowType);
  virtual void            SetTransparencyMode(nsTransparencyMode aMode);
  virtual nsTransparencyMode GetTransparencyMode();
  virtual void            GetWindowClipRegion(nsTArray<nsIntRect>* aRects);
  NS_IMETHOD              SetWindowShadowStyle(PRInt32 aStyle);
  virtual void            SetShowsToolbarButton(bool aShow) {}
  NS_IMETHOD              HideWindowChrome(bool aShouldHide);
  NS_IMETHOD              MakeFullScreen(bool aFullScreen);
  virtual nsDeviceContext* GetDeviceContext();
  virtual LayerManager*   GetLayerManager(PLayersChild* aShadowManager = nsnull,
                                          LayersBackend aBackendHint = LayerManager::LAYERS_NONE,
                                          LayerManagerPersistence aPersistence = LAYER_MANAGER_CURRENT,
                                          bool* aAllowRetaining = nsnull);

  virtual void            DrawOver(LayerManager* aManager, nsIntRect aRect) {}
  virtual void            UpdateThemeGeometries(const nsTArray<ThemeGeometry>& aThemeGeometries) {}
  virtual gfxASurface*    GetThebesSurface();
  NS_IMETHOD              SetModal(bool aModal); 
  NS_IMETHOD              SetWindowClass(const nsAString& xulWinType);
  NS_IMETHOD              MoveClient(PRInt32 aX, PRInt32 aY);
  NS_IMETHOD              ResizeClient(PRInt32 aWidth, PRInt32 aHeight, bool aRepaint);
  NS_IMETHOD              ResizeClient(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight, bool aRepaint);
  NS_IMETHOD              SetBounds(const nsIntRect &aRect);
  NS_IMETHOD              GetBounds(nsIntRect &aRect);
  NS_IMETHOD              GetClientBounds(nsIntRect &aRect);
  NS_IMETHOD              GetScreenBounds(nsIntRect &aRect);
  NS_IMETHOD              GetNonClientMargins(nsIntMargin &margins);
  NS_IMETHOD              SetNonClientMargins(nsIntMargin &margins);
  virtual nsIntPoint      GetClientOffset();
  NS_IMETHOD              EnableDragDrop(bool aEnable);
  NS_IMETHOD              GetAttention(PRInt32 aCycleCount);
  virtual bool            HasPendingInputEvent();
  NS_IMETHOD              SetIcon(const nsAString &anIconSpec);
  NS_IMETHOD              BeginSecureKeyboardInput();
  NS_IMETHOD              EndSecureKeyboardInput();
  NS_IMETHOD              SetWindowTitlebarColor(nscolor aColor, bool aActive);
  virtual void            SetDrawsInTitlebar(bool aState) {}
  virtual bool            ShowsResizeIndicator(nsIntRect* aResizerRect);
  virtual void            FreeNativeData(void * data, PRUint32 aDataType) {}
  NS_IMETHOD              BeginResizeDrag(nsGUIEvent* aEvent, PRInt32 aHorizontal, PRInt32 aVertical);
  NS_IMETHOD              BeginMoveDrag(nsMouseEvent* aEvent);
  virtual nsresult        ActivateNativeMenuItemAt(const nsAString& indexString) { return NS_ERROR_NOT_IMPLEMENTED; }
  virtual nsresult        ForceUpdateNativeMenuAt(const nsAString& indexString) { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              ResetInputState() { return NS_OK; }
  NS_IMETHOD              CancelIMEComposition() { return NS_OK; }
  NS_IMETHOD              SetAcceleratedRendering(bool aEnabled);
  virtual bool            GetAcceleratedRendering();
  virtual bool            GetShouldAccelerate();
  NS_IMETHOD              GetToggledKeyState(PRUint32 aKeyCode, bool* aLEDState) { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              OnIMEFocusChange(bool aFocus) { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              OnIMETextChange(PRUint32 aStart, PRUint32 aOldEnd, PRUint32 aNewEnd) { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              OnIMESelectionChange(void) { return NS_ERROR_NOT_IMPLEMENTED; }
  virtual nsIMEUpdatePreference GetIMEUpdatePreference() { return nsIMEUpdatePreference(false, false); }
  NS_IMETHOD              OnDefaultButtonLoaded(const nsIntRect &aButtonRect) { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              OverrideSystemMouseScrollSpeed(PRInt32 aOriginalDelta, bool aIsHorizontal, PRInt32 &aOverriddenDelta);
  virtual already_AddRefed<nsIWidget>
  CreateChild(const nsIntRect  &aRect,
              EVENT_CALLBACK   aHandleEventFunction,
              nsDeviceContext *aContext,
              nsWidgetInitData *aInitData = nsnull,
              bool             aForceUseIWidgetParent = false);
  NS_IMETHOD              SetEventCallback(EVENT_CALLBACK aEventFunction, nsDeviceContext *aContext);
  NS_IMETHOD              AttachViewToTopLevel(EVENT_CALLBACK aViewEventFunction, nsDeviceContext *aContext);
  virtual ViewWrapper*    GetAttachedViewPtr();
  NS_IMETHOD              SetAttachedViewPtr(ViewWrapper* aViewWrapper);
  NS_IMETHOD              RegisterTouchWindow();
  NS_IMETHOD              UnregisterTouchWindow();

  nsPopupLevel PopupLevel() { return mPopupLevel; }

  virtual nsIntSize       ClientToWindowSize(const nsIntSize& aClientSize)
  {
    return aClientSize;
  }

  
  bool IsPopupWithTitleBar() const
  {
    return (mWindowType == eWindowType_popup && 
            mBorderStyle != eBorderStyle_default &&
            mBorderStyle & eBorderStyle_title);
  }

  NS_IMETHOD              ReparentNativeWidget(nsIWidget* aNewParent) = 0;
  




  class AutoLayerManagerSetup {
  public:
    AutoLayerManagerSetup(nsBaseWidget* aWidget, gfxContext* aTarget,
                          BasicLayerManager::BufferMode aDoubleBuffering);
    ~AutoLayerManagerSetup();
  private:
    nsBaseWidget* mWidget;
  };
  friend class AutoLayerManagerSetup;

  class AutoUseBasicLayerManager {
  public:
    AutoUseBasicLayerManager(nsBaseWidget* aWidget);
    ~AutoUseBasicLayerManager();
  private:
    nsBaseWidget* mWidget;
  };
  friend class AutoUseBasicLayerManager;

  bool HasDestroyStarted() const 
  {
    return mOnDestroyCalled;
  }

  bool                    Destroyed() { return mOnDestroyCalled; }

protected:

  virtual void            ResolveIconName(const nsAString &aIconName,
                                          const nsAString &aIconSuffix,
                                          nsILocalFile **aResult);
  virtual void            OnDestroy();
  virtual void            BaseCreate(nsIWidget *aParent,
                                     const nsIntRect &aRect,
                                     EVENT_CALLBACK aHandleEventFunction,
                                     nsDeviceContext *aContext,
                                     nsWidgetInitData *aInitData);

  virtual nsIContent* GetLastRollup()
  {
    return mLastRollup;
  }

  virtual nsresult SynthesizeNativeKeyEvent(PRInt32 aNativeKeyboardLayout,
                                            PRInt32 aNativeKeyCode,
                                            PRUint32 aModifierFlags,
                                            const nsAString& aCharacters,
                                            const nsAString& aUnmodifiedCharacters)
  { return NS_ERROR_UNEXPECTED; }

  virtual nsresult SynthesizeNativeMouseEvent(nsIntPoint aPoint,
                                              PRUint32 aNativeMessage,
                                              PRUint32 aModifierFlags)
  { return NS_ERROR_UNEXPECTED; }

  
  
  bool StoreWindowClipRegion(const nsTArray<nsIntRect>& aRects);

  virtual already_AddRefed<nsIWidget>
  AllocateChildPopupWidget()
  {
    static NS_DEFINE_IID(kCPopUpCID, NS_CHILD_CID);
    nsCOMPtr<nsIWidget> widget = do_CreateInstance(kCPopUpCID);
    return widget.forget();
  }

  BasicLayerManager* CreateBasicLayerManager();

protected:
  void*             mClientData;
  ViewWrapper*      mViewWrapperPtr;
  EVENT_CALLBACK    mEventCallback;
  EVENT_CALLBACK    mViewCallback;
  nsDeviceContext* mContext;
  nsRefPtr<LayerManager> mLayerManager;
  nsRefPtr<LayerManager> mBasicLayerManager;
  nscolor           mBackground;
  nscolor           mForeground;
  nsCursor          mCursor;
  nsWindowType      mWindowType;
  nsBorderStyle     mBorderStyle;
  bool              mOnDestroyCalled;
  bool              mUseAcceleratedRendering;
  bool              mTemporarilyUseBasicLayerManager;
  nsIntRect         mBounds;
  nsIntRect*        mOriginalBounds;
  
  nsAutoArrayPtr<nsIntRect> mClipRects;
  PRUint32          mClipRectCount;
  PRInt32           mZIndex;
  nsSizeMode        mSizeMode;
  nsPopupLevel      mPopupLevel;

  
  
  static nsIContent* mLastRollup;

#ifdef DEBUG
protected:
  static nsAutoString debug_GuiEventToString(nsGUIEvent * aGuiEvent);
  static bool debug_WantPaintFlashing();

  static void debug_DumpInvalidate(FILE *                aFileOut,
                                   nsIWidget *           aWidget,
                                   const nsIntRect *     aRect,
                                   bool                  aIsSynchronous,
                                   const nsCAutoString & aWidgetName,
                                   PRInt32               aWindowID);

  static void debug_DumpEvent(FILE *                aFileOut,
                              nsIWidget *           aWidget,
                              nsGUIEvent *          aGuiEvent,
                              const nsCAutoString & aWidgetName,
                              PRInt32               aWindowID);

  static void debug_DumpPaintEvent(FILE *                aFileOut,
                                   nsIWidget *           aWidget,
                                   nsPaintEvent *        aPaintEvent,
                                   const nsCAutoString & aWidgetName,
                                   PRInt32               aWindowID);

  static bool debug_GetCachedBoolPref(const char* aPrefName);
#endif
};













class nsAutoRollup
{
  bool wasClear;

  public:

  nsAutoRollup();
  ~nsAutoRollup();
};

#endif 
