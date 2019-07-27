



#ifndef nsBaseWidget_h__
#define nsBaseWidget_h__

#include "mozilla/EventForwards.h"
#include "mozilla/WidgetUtils.h"
#include "nsRect.h"
#include "nsIWidget.h"
#include "nsWidgetsCID.h"
#include "nsIFile.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsIRollupListener.h"
#include "nsIObserver.h"
#include "nsIWidgetListener.h"
#include "nsPIDOMWindow.h"
#include "nsWeakReference.h"
#include <algorithm>
class nsIContent;
class nsAutoRollup;
class gfxContext;

namespace mozilla {
#ifdef ACCESSIBILITY
namespace a11y {
class Accessible;
}
#endif

namespace layers {
class BasicLayerManager;
class CompositorChild;
class CompositorParent;
class APZCTreeManager;
class GeckoContentController;
class APZEventState;
struct ScrollableLayerGuid;
struct SetAllowedTouchBehaviorCallback;
}

class CompositorVsyncDispatcher;
}

namespace base {
class Thread;
}




#define TOUCH_INJECT_MAX_POINTS 256

class nsBaseWidget;


class WidgetShutdownObserver final : public nsIObserver
{
  ~WidgetShutdownObserver();

public:
  explicit WidgetShutdownObserver(nsBaseWidget* aWidget);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  void Register();
  void Unregister();

  nsBaseWidget *mWidget;
  bool mRegistered;
};










class nsBaseWidget : public nsIWidget, public nsSupportsWeakReference
{
  friend class nsAutoRollup;

protected:
  typedef base::Thread Thread;
  typedef mozilla::layers::BasicLayerManager BasicLayerManager;
  typedef mozilla::layers::BufferMode BufferMode;
  typedef mozilla::layers::CompositorChild CompositorChild;
  typedef mozilla::layers::CompositorParent CompositorParent;
  typedef mozilla::layers::APZCTreeManager APZCTreeManager;
  typedef mozilla::layers::GeckoContentController GeckoContentController;
  typedef mozilla::layers::ScrollableLayerGuid ScrollableLayerGuid;
  typedef mozilla::layers::APZEventState APZEventState;
  typedef mozilla::layers::SetAllowedTouchBehaviorCallback SetAllowedTouchBehaviorCallback;
  typedef mozilla::ScreenRotation ScreenRotation;

  virtual ~nsBaseWidget();

public:
  nsBaseWidget();

  NS_DECL_ISUPPORTS

  
  NS_IMETHOD              CaptureMouse(bool aCapture) override;
  virtual nsIWidgetListener*  GetWidgetListener() override;
  virtual void            SetWidgetListener(nsIWidgetListener* alistener) override;
  NS_IMETHOD              Destroy() override;
  NS_IMETHOD              SetParent(nsIWidget* aNewParent) override;
  virtual nsIWidget*      GetParent(void) override;
  virtual nsIWidget*      GetTopLevelWidget() override;
  virtual nsIWidget*      GetSheetWindowParent(void) override;
  virtual float           GetDPI() override;
  virtual void            AddChild(nsIWidget* aChild) override;
  virtual void            RemoveChild(nsIWidget* aChild) override;

  void                    SetZIndex(int32_t aZIndex) override;
  NS_IMETHOD              PlaceBehind(nsTopLevelWidgetZPlacement aPlacement,
                                      nsIWidget *aWidget, bool aActivate) override;

  NS_IMETHOD              SetSizeMode(int32_t aMode) override;
  virtual int32_t         SizeMode() override
  {
    return mSizeMode;
  }

  virtual nsCursor        GetCursor() override;
  NS_IMETHOD              SetCursor(nsCursor aCursor) override;
  NS_IMETHOD              SetCursor(imgIContainer* aCursor,
                                    uint32_t aHotspotX, uint32_t aHotspotY) override;
  virtual void            ClearCachedCursor() override { mUpdateCursor = true; }
  virtual void            SetTransparencyMode(nsTransparencyMode aMode) override;
  virtual nsTransparencyMode GetTransparencyMode() override;
  virtual void            GetWindowClipRegion(nsTArray<nsIntRect>* aRects) override;
  NS_IMETHOD              SetWindowShadowStyle(int32_t aStyle) override;
  virtual void            SetShowsToolbarButton(bool aShow) override {}
  virtual void            SetShowsFullScreenButton(bool aShow) override {}
  virtual void            SetWindowAnimationType(WindowAnimationType aType) override {}
  NS_IMETHOD              HideWindowChrome(bool aShouldHide) override;
  NS_IMETHOD              MakeFullScreen(bool aFullScreen, nsIScreen* aScreen = nullptr) override;
  virtual LayerManager*   GetLayerManager(PLayerTransactionChild* aShadowManager = nullptr,
                                          LayersBackend aBackendHint = mozilla::layers::LayersBackend::LAYERS_NONE,
                                          LayerManagerPersistence aPersistence = LAYER_MANAGER_CURRENT,
                                          bool* aAllowRetaining = nullptr) override;

  CompositorVsyncDispatcher* GetCompositorVsyncDispatcher() override;
  void            CreateCompositorVsyncDispatcher();
  virtual CompositorParent* NewCompositorParent(int aSurfaceWidth, int aSurfaceHeight);
  virtual void            CreateCompositor();
  virtual void            CreateCompositor(int aWidth, int aHeight);
  virtual void            PrepareWindowEffects() override {}
  virtual void            CleanupWindowEffects() override {}
  virtual bool            PreRender(LayerManagerComposite* aManager) override { return true; }
  virtual void            PostRender(LayerManagerComposite* aManager) override {}
  virtual void            DrawWindowUnderlay(LayerManagerComposite* aManager, nsIntRect aRect) override {}
  virtual void            DrawWindowOverlay(LayerManagerComposite* aManager, nsIntRect aRect) override {}
  virtual mozilla::TemporaryRef<mozilla::gfx::DrawTarget> StartRemoteDrawing() override;
  virtual void            EndRemoteDrawing() override { };
  virtual void            CleanupRemoteDrawing() override { };
  virtual void            UpdateThemeGeometries(const nsTArray<ThemeGeometry>& aThemeGeometries) override {}
  NS_IMETHOD              SetModal(bool aModal) override;
  virtual uint32_t        GetMaxTouchPoints() const override;
  NS_IMETHOD              SetWindowClass(const nsAString& xulWinType) override;
  virtual nsresult        SetWindowClipRegion(const nsTArray<nsIntRect>& aRects, bool aIntersectWithExisting) override;
  
  
  
  
  
  
  
  bool                    BoundsUseDisplayPixels() const {
    return mWindowType <= eWindowType_popup;
  }
  NS_IMETHOD              MoveClient(double aX, double aY) override;
  NS_IMETHOD              ResizeClient(double aWidth, double aHeight, bool aRepaint) override;
  NS_IMETHOD              ResizeClient(double aX, double aY, double aWidth, double aHeight, bool aRepaint) override;
  NS_IMETHOD              GetBounds(nsIntRect &aRect) override;
  NS_IMETHOD              GetClientBounds(nsIntRect &aRect) override;
  NS_IMETHOD              GetScreenBounds(nsIntRect &aRect) override;
  NS_IMETHOD              GetRestoredBounds(nsIntRect &aRect) override;
  NS_IMETHOD              GetNonClientMargins(nsIntMargin &margins) override;
  NS_IMETHOD              SetNonClientMargins(nsIntMargin &margins) override;
  virtual nsIntPoint      GetClientOffset() override;
  NS_IMETHOD              EnableDragDrop(bool aEnable) override;
  NS_IMETHOD              GetAttention(int32_t aCycleCount) override;
  virtual bool            HasPendingInputEvent() override;
  NS_IMETHOD              SetIcon(const nsAString &anIconSpec) override;
  NS_IMETHOD              SetWindowTitlebarColor(nscolor aColor, bool aActive) override;
  virtual void            SetDrawsInTitlebar(bool aState) override {}
  virtual bool            ShowsResizeIndicator(nsIntRect* aResizerRect) override;
  virtual void            FreeNativeData(void * data, uint32_t aDataType) override {}
  NS_IMETHOD              BeginResizeDrag(mozilla::WidgetGUIEvent* aEvent,
                                          int32_t aHorizontal,
                                          int32_t aVertical) override;
  NS_IMETHOD              BeginMoveDrag(mozilla::WidgetMouseEvent* aEvent) override;
  virtual nsresult        ActivateNativeMenuItemAt(const nsAString& indexString) override { return NS_ERROR_NOT_IMPLEMENTED; }
  virtual nsresult        ForceUpdateNativeMenuAt(const nsAString& indexString) override { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              NotifyIME(const IMENotification& aIMENotification) override final;
  NS_IMETHOD              StartPluginIME(const mozilla::WidgetKeyboardEvent& aKeyboardEvent,
                                         int32_t aPanelX, int32_t aPanelY,
                                         nsString& aCommitted) override
                          { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              SetPluginFocused(bool& aFocused) override
                          { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              AttachNativeKeyEvent(mozilla::WidgetKeyboardEvent& aEvent) override { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD_(bool)       ExecuteNativeKeyBinding(
                            NativeKeyBindingsType aType,
                            const mozilla::WidgetKeyboardEvent& aEvent,
                            DoCommandCallback aCallback,
                            void* aCallbackData) override { return false; }
  virtual bool            ComputeShouldAccelerate(bool aDefault);
  NS_IMETHOD              GetToggledKeyState(uint32_t aKeyCode, bool* aLEDState) override { return NS_ERROR_NOT_IMPLEMENTED; }
  virtual nsIMEUpdatePreference GetIMEUpdatePreference() override { return nsIMEUpdatePreference(); }
  NS_IMETHOD              OnDefaultButtonLoaded(const nsIntRect &aButtonRect) override { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              OverrideSystemMouseScrollSpeed(double aOriginalDeltaX,
                                                         double aOriginalDeltaY,
                                                         double& aOverriddenDeltaX,
                                                         double& aOverriddenDeltaY) override;
  virtual already_AddRefed<nsIWidget>
  CreateChild(const nsIntRect  &aRect,
              nsWidgetInitData *aInitData = nullptr,
              bool             aForceUseIWidgetParent = false) override;
  NS_IMETHOD              AttachViewToTopLevel(bool aUseAttachedEvents) override;
  virtual nsIWidgetListener* GetAttachedWidgetListener() override;
  virtual void               SetAttachedWidgetListener(nsIWidgetListener* aListener) override;
  NS_IMETHOD_(TextEventDispatcher*) GetTextEventDispatcher() override final;

  
  
  nsEventStatus DispatchInputEvent(mozilla::WidgetInputEvent* aEvent) override;

  
  nsEventStatus DispatchAPZAwareEvent(mozilla::WidgetInputEvent* aEvent) override;

  void SetConfirmedTargetAPZC(uint64_t aInputBlockId,
                              const nsTArray<ScrollableLayerGuid>& aTargets) const override;

  void NotifyWindowDestroyed();
  void NotifySizeMoveDone();
  void NotifyWindowMoved(int32_t aX, int32_t aY);

  
  virtual void RegisterPluginWindowForRemoteUpdates() override;
  virtual void UnregisterPluginWindowForRemoteUpdates() override;

  virtual void SetNativeData(uint32_t aDataType, uintptr_t aVal) override {};

  
  
  void NotifySysColorChanged();
  void NotifyThemeChanged();
  void NotifyUIStateChanged(UIStateChangeType aShowAccelerators,
                            UIStateChangeType aShowFocusRings);

#ifdef ACCESSIBILITY
  
  mozilla::a11y::Accessible* GetRootAccessible();
#endif

  nsPopupLevel PopupLevel() { return mPopupLevel; }

  virtual mozilla::LayoutDeviceIntSize
  ClientToWindowSize(const mozilla::LayoutDeviceIntSize& aClientSize) override
  {
    return aClientSize;
  }

  
  bool IsPopupWithTitleBar() const
  {
    return (mWindowType == eWindowType_popup && 
            mBorderStyle != eBorderStyle_default &&
            mBorderStyle & eBorderStyle_title);
  }

  NS_IMETHOD              ReparentNativeWidget(nsIWidget* aNewParent) override = 0;

  virtual uint32_t GetGLFrameBufferFormat() override;

  virtual const SizeConstraints& GetSizeConstraints() const override;
  virtual void SetSizeConstraints(const SizeConstraints& aConstraints) override;

  









  class AutoLayerManagerSetup {
  public:
    AutoLayerManagerSetup(nsBaseWidget* aWidget, gfxContext* aTarget,
                          BufferMode aDoubleBuffering,
                          ScreenRotation aRotation = mozilla::ROTATION_0);
    ~AutoLayerManagerSetup();
  private:
    nsBaseWidget* mWidget;
    nsRefPtr<BasicLayerManager> mLayerManager;
  };
  friend class AutoLayerManagerSetup;

  class AutoUseBasicLayerManager {
  public:
    explicit AutoUseBasicLayerManager(nsBaseWidget* aWidget);
    ~AutoUseBasicLayerManager();
  private:
    nsBaseWidget* mWidget;
    bool mPreviousTemporarilyUseBasicLayerManager;
  };
  friend class AutoUseBasicLayerManager;

  virtual bool            ShouldUseOffMainThreadCompositing();

  static nsIRollupListener* GetActiveRollupListener();

  void Shutdown();

protected:

  void            ResolveIconName(const nsAString &aIconName,
                                  const nsAString &aIconSuffix,
                                  nsIFile **aResult);
  virtual void    OnDestroy();
  void            BaseCreate(nsIWidget *aParent,
                             const nsIntRect &aRect,
                             nsWidgetInitData *aInitData);

  virtual void ConfigureAPZCTreeManager();
  virtual void ConfigureAPZControllerThread();
  virtual already_AddRefed<GeckoContentController> CreateRootContentController();

  
  nsEventStatus ProcessUntransformedAPZEvent(mozilla::WidgetInputEvent* aEvent,
                                             const ScrollableLayerGuid& aGuid,
                                             uint64_t aInputBlockId,
                                             nsEventStatus aApzResponse);

  const nsIntRegion RegionFromArray(const nsTArray<nsIntRect>& aRects);
  void ArrayFromRegion(const nsIntRegion& aRegion, nsTArray<nsIntRect>& aRects);

  virtual nsIContent* GetLastRollup() override
  {
    return mLastRollup;
  }

  virtual nsresult SynthesizeNativeKeyEvent(int32_t aNativeKeyboardLayout,
                                            int32_t aNativeKeyCode,
                                            uint32_t aModifierFlags,
                                            const nsAString& aCharacters,
                                            const nsAString& aUnmodifiedCharacters,
                                            nsIObserver* aObserver) override
  {
    mozilla::widget::AutoObserverNotifier notifier(aObserver, "keyevent");
    return NS_ERROR_UNEXPECTED;
  }

  virtual nsresult SynthesizeNativeMouseEvent(mozilla::LayoutDeviceIntPoint aPoint,
                                              uint32_t aNativeMessage,
                                              uint32_t aModifierFlags,
                                              nsIObserver* aObserver) override
  {
    mozilla::widget::AutoObserverNotifier notifier(aObserver, "mouseevent");
    return NS_ERROR_UNEXPECTED;
  }

  virtual nsresult SynthesizeNativeMouseMove(mozilla::LayoutDeviceIntPoint aPoint,
                                             nsIObserver* aObserver) override
  {
    mozilla::widget::AutoObserverNotifier notifier(aObserver, "mouseevent");
    return NS_ERROR_UNEXPECTED;
  }

  virtual nsresult SynthesizeNativeMouseScrollEvent(mozilla::LayoutDeviceIntPoint aPoint,
                                                    uint32_t aNativeMessage,
                                                    double aDeltaX,
                                                    double aDeltaY,
                                                    double aDeltaZ,
                                                    uint32_t aModifierFlags,
                                                    uint32_t aAdditionalFlags,
                                                    nsIObserver* aObserver) override
  {
    mozilla::widget::AutoObserverNotifier notifier(aObserver, "mousescrollevent");
    return NS_ERROR_UNEXPECTED;
  }

  virtual nsresult SynthesizeNativeTouchPoint(uint32_t aPointerId,
                                              TouchPointerState aPointerState,
                                              nsIntPoint aPointerScreenPoint,
                                              double aPointerPressure,
                                              uint32_t aPointerOrientation,
                                              nsIObserver* aObserver) override
  {
    mozilla::widget::AutoObserverNotifier notifier(aObserver, "touchpoint");
    return NS_ERROR_UNEXPECTED;
  }

  virtual nsresult NotifyIMEInternal(const IMENotification& aIMENotification)
  { return NS_ERROR_NOT_IMPLEMENTED; }

protected:
  
  
  bool IsWindowClipRegionEqual(const nsTArray<nsIntRect>& aRects);

  
  void StoreWindowClipRegion(const nsTArray<nsIntRect>& aRects);

  virtual already_AddRefed<nsIWidget>
  AllocateChildPopupWidget()
  {
    static NS_DEFINE_IID(kCPopUpCID, NS_CHILD_CID);
    nsCOMPtr<nsIWidget> widget = do_CreateInstance(kCPopUpCID);
    return widget.forget();
  }

  LayerManager* CreateBasicLayerManager();

  nsPopupType PopupType() const { return mPopupType; }

  void NotifyRollupGeometryChange()
  {
    
    
    if (gRollupListener) {
      gRollupListener->NotifyGeometryChange();
    }
  }

  





  void ConstrainSize(int32_t* aWidth, int32_t* aHeight) const
  {
    *aWidth = std::max(mSizeConstraints.mMinSize.width,
                     std::min(mSizeConstraints.mMaxSize.width, *aWidth));
    *aHeight = std::max(mSizeConstraints.mMinSize.height,
                      std::min(mSizeConstraints.mMaxSize.height, *aHeight));
  }

  virtual CompositorChild* GetRemoteRenderer() override;

  virtual void GetPreferredCompositorBackends(nsTArray<mozilla::layers::LayersBackend>& aHints);

  


  virtual void WindowUsesOMTC() {}

  nsIDocument* GetDocument() const;

protected:
  








  void DestroyCompositor();
  void DestroyLayerManager();

  void FreeShutdownObserver();

  nsIWidgetListener* mWidgetListener;
  nsIWidgetListener* mAttachedWidgetListener;
  nsRefPtr<LayerManager> mLayerManager;
  nsRefPtr<LayerManager> mBasicLayerManager;
  nsRefPtr<CompositorChild> mCompositorChild;
  nsRefPtr<CompositorParent> mCompositorParent;
  nsRefPtr<mozilla::CompositorVsyncDispatcher> mCompositorVsyncDispatcher;
  nsRefPtr<APZCTreeManager> mAPZC;
  nsRefPtr<APZEventState> mAPZEventState;
  nsRefPtr<SetAllowedTouchBehaviorCallback> mSetAllowedTouchBehaviorCallback;
  nsRefPtr<WidgetShutdownObserver> mShutdownObserver;
  nsRefPtr<TextEventDispatcher> mTextEventDispatcher;
  nsCursor          mCursor;
  bool              mUpdateCursor;
  nsBorderStyle     mBorderStyle;
  bool              mUseLayersAcceleration;
  bool              mForceLayersAcceleration;
  bool              mTemporarilyUseBasicLayerManager;
  
  
  bool              mRequireOffMainThreadCompositing;
  bool              mUseAttachedEvents;
  nsIntRect         mBounds;
  nsIntRect*        mOriginalBounds;
  
  nsAutoArrayPtr<nsIntRect> mClipRects;
  uint32_t          mClipRectCount;
  nsSizeMode        mSizeMode;
  nsPopupLevel      mPopupLevel;
  nsPopupType       mPopupType;
  SizeConstraints   mSizeConstraints;

  static nsIRollupListener* gRollupListener;

  
  
  static nsIContent* mLastRollup;

#ifdef DEBUG
protected:
  static nsAutoString debug_GuiEventToString(mozilla::WidgetGUIEvent* aGuiEvent);
  static bool debug_WantPaintFlashing();

  static void debug_DumpInvalidate(FILE *                aFileOut,
                                   nsIWidget *           aWidget,
                                   const nsIntRect *     aRect,
                                   const nsAutoCString & aWidgetName,
                                   int32_t               aWindowID);

  static void debug_DumpEvent(FILE* aFileOut,
                              nsIWidget* aWidget,
                              mozilla::WidgetGUIEvent* aGuiEvent,
                              const nsAutoCString& aWidgetName,
                              int32_t aWindowID);

  static void debug_DumpPaintEvent(FILE *                aFileOut,
                                   nsIWidget *           aWidget,
                                   const nsIntRegion &   aPaintEvent,
                                   const nsAutoCString & aWidgetName,
                                   int32_t               aWindowID);

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
