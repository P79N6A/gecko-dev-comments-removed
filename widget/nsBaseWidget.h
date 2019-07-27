



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
struct SetTargetAPZCCallback;
}

class CompositorVsyncDispatcher;
}

namespace base {
class Thread;
}




#define TOUCH_INJECT_MAX_POINTS 256

class nsBaseWidget;

class WidgetShutdownObserver MOZ_FINAL : public nsIObserver
{
  ~WidgetShutdownObserver() {}

public:
  explicit WidgetShutdownObserver(nsBaseWidget* aWidget)
    : mWidget(aWidget)
  { }

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  nsBaseWidget *mWidget;
};










class nsBaseWidget : public nsIWidget
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
  typedef mozilla::layers::SetTargetAPZCCallback SetTargetAPZCCallback;
  typedef mozilla::ScreenRotation ScreenRotation;

  virtual ~nsBaseWidget();

public:
  nsBaseWidget();

  NS_DECL_ISUPPORTS

  
  NS_IMETHOD              CaptureMouse(bool aCapture) MOZ_OVERRIDE;
  virtual nsIWidgetListener*  GetWidgetListener() MOZ_OVERRIDE;
  virtual void            SetWidgetListener(nsIWidgetListener* alistener) MOZ_OVERRIDE;
  NS_IMETHOD              Destroy() MOZ_OVERRIDE;
  NS_IMETHOD              SetParent(nsIWidget* aNewParent) MOZ_OVERRIDE;
  virtual nsIWidget*      GetParent(void) MOZ_OVERRIDE;
  virtual nsIWidget*      GetTopLevelWidget() MOZ_OVERRIDE;
  virtual nsIWidget*      GetSheetWindowParent(void) MOZ_OVERRIDE;
  virtual float           GetDPI() MOZ_OVERRIDE;
  virtual void            AddChild(nsIWidget* aChild) MOZ_OVERRIDE;
  virtual void            RemoveChild(nsIWidget* aChild) MOZ_OVERRIDE;

  void                    SetZIndex(int32_t aZIndex) MOZ_OVERRIDE;
  NS_IMETHOD              PlaceBehind(nsTopLevelWidgetZPlacement aPlacement,
                                      nsIWidget *aWidget, bool aActivate) MOZ_OVERRIDE;

  NS_IMETHOD              SetSizeMode(int32_t aMode) MOZ_OVERRIDE;
  virtual int32_t         SizeMode() MOZ_OVERRIDE
  {
    return mSizeMode;
  }

  virtual nsCursor        GetCursor() MOZ_OVERRIDE;
  NS_IMETHOD              SetCursor(nsCursor aCursor) MOZ_OVERRIDE;
  NS_IMETHOD              SetCursor(imgIContainer* aCursor,
                                    uint32_t aHotspotX, uint32_t aHotspotY) MOZ_OVERRIDE;
  virtual void            ClearCachedCursor() MOZ_OVERRIDE { mUpdateCursor = true; }
  virtual void            SetTransparencyMode(nsTransparencyMode aMode) MOZ_OVERRIDE;
  virtual nsTransparencyMode GetTransparencyMode() MOZ_OVERRIDE;
  virtual void            GetWindowClipRegion(nsTArray<nsIntRect>* aRects) MOZ_OVERRIDE;
  NS_IMETHOD              SetWindowShadowStyle(int32_t aStyle) MOZ_OVERRIDE;
  virtual void            SetShowsToolbarButton(bool aShow) MOZ_OVERRIDE {}
  virtual void            SetShowsFullScreenButton(bool aShow) MOZ_OVERRIDE {}
  virtual void            SetWindowAnimationType(WindowAnimationType aType) MOZ_OVERRIDE {}
  NS_IMETHOD              HideWindowChrome(bool aShouldHide) MOZ_OVERRIDE;
  NS_IMETHOD              MakeFullScreen(bool aFullScreen, nsIScreen* aScreen = nullptr) MOZ_OVERRIDE;
  virtual LayerManager*   GetLayerManager(PLayerTransactionChild* aShadowManager = nullptr,
                                          LayersBackend aBackendHint = mozilla::layers::LayersBackend::LAYERS_NONE,
                                          LayerManagerPersistence aPersistence = LAYER_MANAGER_CURRENT,
                                          bool* aAllowRetaining = nullptr) MOZ_OVERRIDE;

  CompositorVsyncDispatcher* GetCompositorVsyncDispatcher() MOZ_OVERRIDE;
  void            CreateCompositorVsyncDispatcher();
  virtual CompositorParent* NewCompositorParent(int aSurfaceWidth, int aSurfaceHeight);
  virtual void            CreateCompositor();
  virtual void            CreateCompositor(int aWidth, int aHeight);
  virtual void            PrepareWindowEffects() MOZ_OVERRIDE {}
  virtual void            CleanupWindowEffects() MOZ_OVERRIDE {}
  virtual bool            PreRender(LayerManagerComposite* aManager) MOZ_OVERRIDE { return true; }
  virtual void            PostRender(LayerManagerComposite* aManager) MOZ_OVERRIDE {}
  virtual void            DrawWindowUnderlay(LayerManagerComposite* aManager, nsIntRect aRect) MOZ_OVERRIDE {}
  virtual void            DrawWindowOverlay(LayerManagerComposite* aManager, nsIntRect aRect) MOZ_OVERRIDE {}
  virtual mozilla::TemporaryRef<mozilla::gfx::DrawTarget> StartRemoteDrawing() MOZ_OVERRIDE;
  virtual void            EndRemoteDrawing() MOZ_OVERRIDE { };
  virtual void            CleanupRemoteDrawing() MOZ_OVERRIDE { };
  virtual void            UpdateThemeGeometries(const nsTArray<ThemeGeometry>& aThemeGeometries) MOZ_OVERRIDE {}
  NS_IMETHOD              SetModal(bool aModal) MOZ_OVERRIDE;
  virtual uint32_t        GetMaxTouchPoints() const MOZ_OVERRIDE;
  NS_IMETHOD              SetWindowClass(const nsAString& xulWinType) MOZ_OVERRIDE;
  virtual nsresult        SetWindowClipRegion(const nsTArray<nsIntRect>& aRects, bool aIntersectWithExisting) MOZ_OVERRIDE;
  
  
  
  
  
  
  
  bool                    BoundsUseDisplayPixels() const {
    return mWindowType <= eWindowType_popup;
  }
  NS_IMETHOD              MoveClient(double aX, double aY) MOZ_OVERRIDE;
  NS_IMETHOD              ResizeClient(double aWidth, double aHeight, bool aRepaint) MOZ_OVERRIDE;
  NS_IMETHOD              ResizeClient(double aX, double aY, double aWidth, double aHeight, bool aRepaint) MOZ_OVERRIDE;
  NS_IMETHOD              GetBounds(nsIntRect &aRect) MOZ_OVERRIDE;
  NS_IMETHOD              GetClientBounds(nsIntRect &aRect) MOZ_OVERRIDE;
  NS_IMETHOD              GetScreenBounds(nsIntRect &aRect) MOZ_OVERRIDE;
  NS_IMETHOD              GetRestoredBounds(nsIntRect &aRect) MOZ_OVERRIDE;
  NS_IMETHOD              GetNonClientMargins(nsIntMargin &margins) MOZ_OVERRIDE;
  NS_IMETHOD              SetNonClientMargins(nsIntMargin &margins) MOZ_OVERRIDE;
  virtual nsIntPoint      GetClientOffset() MOZ_OVERRIDE;
  NS_IMETHOD              EnableDragDrop(bool aEnable) MOZ_OVERRIDE;
  NS_IMETHOD              GetAttention(int32_t aCycleCount) MOZ_OVERRIDE;
  virtual bool            HasPendingInputEvent() MOZ_OVERRIDE;
  NS_IMETHOD              SetIcon(const nsAString &anIconSpec) MOZ_OVERRIDE;
  NS_IMETHOD              SetWindowTitlebarColor(nscolor aColor, bool aActive) MOZ_OVERRIDE;
  virtual void            SetDrawsInTitlebar(bool aState) MOZ_OVERRIDE {}
  virtual bool            ShowsResizeIndicator(nsIntRect* aResizerRect) MOZ_OVERRIDE;
  virtual void            FreeNativeData(void * data, uint32_t aDataType) MOZ_OVERRIDE {}
  NS_IMETHOD              BeginResizeDrag(mozilla::WidgetGUIEvent* aEvent,
                                          int32_t aHorizontal,
                                          int32_t aVertical) MOZ_OVERRIDE;
  NS_IMETHOD              BeginMoveDrag(mozilla::WidgetMouseEvent* aEvent) MOZ_OVERRIDE;
  virtual nsresult        ActivateNativeMenuItemAt(const nsAString& indexString) MOZ_OVERRIDE { return NS_ERROR_NOT_IMPLEMENTED; }
  virtual nsresult        ForceUpdateNativeMenuAt(const nsAString& indexString) MOZ_OVERRIDE { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              NotifyIME(const IMENotification& aIMENotification) MOZ_OVERRIDE MOZ_FINAL;
  NS_IMETHOD              AttachNativeKeyEvent(mozilla::WidgetKeyboardEvent& aEvent) MOZ_OVERRIDE { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD_(bool)       ExecuteNativeKeyBinding(
                            NativeKeyBindingsType aType,
                            const mozilla::WidgetKeyboardEvent& aEvent,
                            DoCommandCallback aCallback,
                            void* aCallbackData) MOZ_OVERRIDE { return false; }
  NS_IMETHOD              SetLayersAcceleration(bool aEnabled) MOZ_OVERRIDE;
  virtual bool            ComputeShouldAccelerate(bool aDefault);
  NS_IMETHOD              GetToggledKeyState(uint32_t aKeyCode, bool* aLEDState) MOZ_OVERRIDE { return NS_ERROR_NOT_IMPLEMENTED; }
  virtual nsIMEUpdatePreference GetIMEUpdatePreference() MOZ_OVERRIDE { return nsIMEUpdatePreference(); }
  NS_IMETHOD              OnDefaultButtonLoaded(const nsIntRect &aButtonRect) MOZ_OVERRIDE { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              OverrideSystemMouseScrollSpeed(double aOriginalDeltaX,
                                                         double aOriginalDeltaY,
                                                         double& aOverriddenDeltaX,
                                                         double& aOverriddenDeltaY) MOZ_OVERRIDE;
  virtual already_AddRefed<nsIWidget>
  CreateChild(const nsIntRect  &aRect,
              nsWidgetInitData *aInitData = nullptr,
              bool             aForceUseIWidgetParent = false) MOZ_OVERRIDE;
  NS_IMETHOD              AttachViewToTopLevel(bool aUseAttachedEvents) MOZ_OVERRIDE;
  virtual nsIWidgetListener* GetAttachedWidgetListener() MOZ_OVERRIDE;
  virtual void               SetAttachedWidgetListener(nsIWidgetListener* aListener) MOZ_OVERRIDE;
  NS_IMETHOD              RegisterTouchWindow() MOZ_OVERRIDE;
  NS_IMETHOD              UnregisterTouchWindow() MOZ_OVERRIDE;
  NS_IMETHOD_(TextEventDispatcher*) GetTextEventDispatcher() MOZ_OVERRIDE MOZ_FINAL;

  void NotifyWindowDestroyed();
  void NotifySizeMoveDone();
  void NotifyWindowMoved(int32_t aX, int32_t aY);

  
  virtual void RegisterPluginWindowForRemoteUpdates() MOZ_OVERRIDE;
  virtual void UnregisterPluginWindowForRemoteUpdates() MOZ_OVERRIDE;

  virtual void SetNativeData(uint32_t aDataType, uintptr_t aVal) MOZ_OVERRIDE {};

  
  
  void NotifySysColorChanged();
  void NotifyThemeChanged();
  void NotifyUIStateChanged(UIStateChangeType aShowAccelerators,
                            UIStateChangeType aShowFocusRings);

#ifdef ACCESSIBILITY
  
  mozilla::a11y::Accessible* GetRootAccessible();
#endif

  nsPopupLevel PopupLevel() { return mPopupLevel; }

  virtual nsIntSize       ClientToWindowSize(const nsIntSize& aClientSize) MOZ_OVERRIDE
  {
    return aClientSize;
  }

  
  bool IsPopupWithTitleBar() const
  {
    return (mWindowType == eWindowType_popup && 
            mBorderStyle != eBorderStyle_default &&
            mBorderStyle & eBorderStyle_title);
  }

  NS_IMETHOD              ReparentNativeWidget(nsIWidget* aNewParent) MOZ_OVERRIDE = 0;

  virtual uint32_t GetGLFrameBufferFormat() MOZ_OVERRIDE;

  virtual const SizeConstraints& GetSizeConstraints() const MOZ_OVERRIDE;
  virtual void SetSizeConstraints(const SizeConstraints& aConstraints) MOZ_OVERRIDE;

  









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
  virtual already_AddRefed<GeckoContentController> CreateRootContentController();

  
  
  nsEventStatus DispatchEventForAPZ(mozilla::WidgetGUIEvent* aEvent,
                                    const ScrollableLayerGuid& aGuid,
                                    uint64_t aInputBlockId);

  const nsIntRegion RegionFromArray(const nsTArray<nsIntRect>& aRects);
  void ArrayFromRegion(const nsIntRegion& aRegion, nsTArray<nsIntRect>& aRects);

  virtual nsIContent* GetLastRollup() MOZ_OVERRIDE
  {
    return mLastRollup;
  }

  virtual nsresult SynthesizeNativeKeyEvent(int32_t aNativeKeyboardLayout,
                                            int32_t aNativeKeyCode,
                                            uint32_t aModifierFlags,
                                            const nsAString& aCharacters,
                                            const nsAString& aUnmodifiedCharacters) MOZ_OVERRIDE
  { return NS_ERROR_UNEXPECTED; }

  virtual nsresult SynthesizeNativeMouseEvent(mozilla::LayoutDeviceIntPoint aPoint,
                                              uint32_t aNativeMessage,
                                              uint32_t aModifierFlags) MOZ_OVERRIDE
  { return NS_ERROR_UNEXPECTED; }

  virtual nsresult SynthesizeNativeMouseMove(mozilla::LayoutDeviceIntPoint aPoint) MOZ_OVERRIDE
  { return NS_ERROR_UNEXPECTED; }

  virtual nsresult SynthesizeNativeMouseScrollEvent(mozilla::LayoutDeviceIntPoint aPoint,
                                                    uint32_t aNativeMessage,
                                                    double aDeltaX,
                                                    double aDeltaY,
                                                    double aDeltaZ,
                                                    uint32_t aModifierFlags,
                                                    uint32_t aAdditionalFlags) MOZ_OVERRIDE
  { return NS_ERROR_UNEXPECTED; }

  virtual nsresult SynthesizeNativeTouchPoint(uint32_t aPointerId,
                                              TouchPointerState aPointerState,
                                              nsIntPoint aPointerScreenPoint,
                                              double aPointerPressure,
                                              uint32_t aPointerOrientation) MOZ_OVERRIDE
  { return NS_ERROR_UNEXPECTED; }

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

  virtual CompositorChild* GetRemoteRenderer() MOZ_OVERRIDE;

  virtual void GetPreferredCompositorBackends(nsTArray<mozilla::layers::LayersBackend>& aHints);

  


  virtual void WindowUsesOMTC() {}

  nsIDocument* GetDocument() const;

protected:
  








  void DestroyCompositor();

  nsIWidgetListener* mWidgetListener;
  nsIWidgetListener* mAttachedWidgetListener;
  nsRefPtr<LayerManager> mLayerManager;
  nsRefPtr<LayerManager> mBasicLayerManager;
  nsRefPtr<CompositorChild> mCompositorChild;
  nsRefPtr<CompositorParent> mCompositorParent;
  nsRefPtr<mozilla::CompositorVsyncDispatcher> mCompositorVsyncDispatcher;
  nsRefPtr<APZCTreeManager> mAPZC;
  nsRefPtr<APZEventState> mAPZEventState;
  nsRefPtr<SetTargetAPZCCallback> mSetTargetAPZCCallback;
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
