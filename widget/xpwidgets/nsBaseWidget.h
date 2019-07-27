



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
}
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
  typedef mozilla::ScreenRotation ScreenRotation;

  virtual ~nsBaseWidget();

public:
  nsBaseWidget();

  NS_DECL_ISUPPORTS

  
  NS_IMETHOD              CaptureMouse(bool aCapture);
  virtual nsIWidgetListener*  GetWidgetListener();
  virtual void            SetWidgetListener(nsIWidgetListener* alistener);
  NS_IMETHOD              Destroy();
  NS_IMETHOD              SetParent(nsIWidget* aNewParent);
  virtual nsIWidget*      GetParent(void);
  virtual nsIWidget*      GetTopLevelWidget();
  virtual nsIWidget*      GetSheetWindowParent(void);
  virtual float           GetDPI();
  virtual void            AddChild(nsIWidget* aChild);
  virtual void            RemoveChild(nsIWidget* aChild);

  void                    SetZIndex(int32_t aZIndex);
  NS_IMETHOD              PlaceBehind(nsTopLevelWidgetZPlacement aPlacement,
                                      nsIWidget *aWidget, bool aActivate);

  NS_IMETHOD              SetSizeMode(int32_t aMode);
  virtual int32_t         SizeMode() MOZ_OVERRIDE
  {
    return mSizeMode;
  }

  virtual nsCursor        GetCursor();
  NS_IMETHOD              SetCursor(nsCursor aCursor);
  NS_IMETHOD              SetCursor(imgIContainer* aCursor,
                                    uint32_t aHotspotX, uint32_t aHotspotY);
  virtual void            ClearCachedCursor() { mUpdateCursor = true; }
  virtual void            SetTransparencyMode(nsTransparencyMode aMode);
  virtual nsTransparencyMode GetTransparencyMode();
  virtual void            GetWindowClipRegion(nsTArray<nsIntRect>* aRects);
  NS_IMETHOD              SetWindowShadowStyle(int32_t aStyle);
  virtual void            SetShowsToolbarButton(bool aShow) {}
  virtual void            SetShowsFullScreenButton(bool aShow) {}
  virtual void            SetWindowAnimationType(WindowAnimationType aType) {}
  NS_IMETHOD              HideWindowChrome(bool aShouldHide);
  NS_IMETHOD              MakeFullScreen(bool aFullScreen);
  virtual nsDeviceContext* GetDeviceContext();
  virtual LayerManager*   GetLayerManager(PLayerTransactionChild* aShadowManager = nullptr,
                                          LayersBackend aBackendHint = mozilla::layers::LayersBackend::LAYERS_NONE,
                                          LayerManagerPersistence aPersistence = LAYER_MANAGER_CURRENT,
                                          bool* aAllowRetaining = nullptr);

  virtual CompositorParent* NewCompositorParent(int aSurfaceWidth, int aSurfaceHeight);
  virtual void            CreateCompositor();
  virtual void            CreateCompositor(int aWidth, int aHeight);
  virtual void            PrepareWindowEffects() {}
  virtual void            CleanupWindowEffects() {}
  virtual bool            PreRender(LayerManagerComposite* aManager) { return true; }
  virtual void            PostRender(LayerManagerComposite* aManager) {}
  virtual void            DrawWindowUnderlay(LayerManagerComposite* aManager, nsIntRect aRect) {}
  virtual void            DrawWindowOverlay(LayerManagerComposite* aManager, nsIntRect aRect) {}
  virtual mozilla::TemporaryRef<mozilla::gfx::DrawTarget> StartRemoteDrawing();
  virtual void            EndRemoteDrawing() { };
  virtual void            CleanupRemoteDrawing() { };
  virtual void            UpdateThemeGeometries(const nsTArray<ThemeGeometry>& aThemeGeometries) {}
  NS_IMETHOD              SetModal(bool aModal);
  virtual uint32_t        GetMaxTouchPoints() const;
  NS_IMETHOD              SetWindowClass(const nsAString& xulWinType);
  
  
  
  
  
  
  
  bool                    BoundsUseDisplayPixels() const {
    return mWindowType <= eWindowType_popup;
  }
  NS_IMETHOD              MoveClient(double aX, double aY);
  NS_IMETHOD              ResizeClient(double aWidth, double aHeight, bool aRepaint);
  NS_IMETHOD              ResizeClient(double aX, double aY, double aWidth, double aHeight, bool aRepaint);
  NS_IMETHOD              GetBounds(nsIntRect &aRect);
  NS_IMETHOD              GetClientBounds(nsIntRect &aRect);
  NS_IMETHOD              GetScreenBounds(nsIntRect &aRect);
  NS_IMETHOD              GetRestoredBounds(nsIntRect &aRect) MOZ_OVERRIDE;
  NS_IMETHOD              GetNonClientMargins(nsIntMargin &margins);
  NS_IMETHOD              SetNonClientMargins(nsIntMargin &margins);
  virtual nsIntPoint      GetClientOffset();
  NS_IMETHOD              EnableDragDrop(bool aEnable);
  NS_IMETHOD              GetAttention(int32_t aCycleCount);
  virtual bool            HasPendingInputEvent();
  NS_IMETHOD              SetIcon(const nsAString &anIconSpec);
  NS_IMETHOD              SetWindowTitlebarColor(nscolor aColor, bool aActive);
  virtual void            SetDrawsInTitlebar(bool aState) {}
  virtual bool            ShowsResizeIndicator(nsIntRect* aResizerRect);
  virtual void            FreeNativeData(void * data, uint32_t aDataType) {}
  NS_IMETHOD              BeginResizeDrag(mozilla::WidgetGUIEvent* aEvent,
                                          int32_t aHorizontal,
                                          int32_t aVertical);
  NS_IMETHOD              BeginMoveDrag(mozilla::WidgetMouseEvent* aEvent);
  virtual nsresult        ActivateNativeMenuItemAt(const nsAString& indexString) { return NS_ERROR_NOT_IMPLEMENTED; }
  virtual nsresult        ForceUpdateNativeMenuAt(const nsAString& indexString) { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              NotifyIME(const IMENotification& aIMENotification) MOZ_OVERRIDE { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              AttachNativeKeyEvent(mozilla::WidgetKeyboardEvent& aEvent) MOZ_OVERRIDE { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD_(bool)       ExecuteNativeKeyBinding(
                            NativeKeyBindingsType aType,
                            const mozilla::WidgetKeyboardEvent& aEvent,
                            DoCommandCallback aCallback,
                            void* aCallbackData) MOZ_OVERRIDE { return false; }
  NS_IMETHOD              SetLayersAcceleration(bool aEnabled);
  virtual bool            GetLayersAcceleration() { return mUseLayersAcceleration; }
  virtual bool            ComputeShouldAccelerate(bool aDefault);
  NS_IMETHOD              GetToggledKeyState(uint32_t aKeyCode, bool* aLEDState) { return NS_ERROR_NOT_IMPLEMENTED; }
  virtual nsIMEUpdatePreference GetIMEUpdatePreference() MOZ_OVERRIDE { return nsIMEUpdatePreference(); }
  NS_IMETHOD              OnDefaultButtonLoaded(const nsIntRect &aButtonRect) { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              OverrideSystemMouseScrollSpeed(double aOriginalDeltaX,
                                                         double aOriginalDeltaY,
                                                         double& aOverriddenDeltaX,
                                                         double& aOverriddenDeltaY);
  virtual already_AddRefed<nsIWidget>
  CreateChild(const nsIntRect  &aRect,
              nsDeviceContext *aContext,
              nsWidgetInitData *aInitData = nullptr,
              bool             aForceUseIWidgetParent = false);
  NS_IMETHOD              AttachViewToTopLevel(bool aUseAttachedEvents, nsDeviceContext *aContext);
  virtual nsIWidgetListener* GetAttachedWidgetListener();
  virtual void               SetAttachedWidgetListener(nsIWidgetListener* aListener);
  NS_IMETHOD              RegisterTouchWindow();
  NS_IMETHOD              UnregisterTouchWindow();

  void NotifyWindowDestroyed();
  void NotifySizeMoveDone();
  void NotifyWindowMoved(int32_t aX, int32_t aY);

  
  
  void NotifySysColorChanged();
  void NotifyThemeChanged();
  void NotifyUIStateChanged(UIStateChangeType aShowAccelerators,
                            UIStateChangeType aShowFocusRings);

#ifdef ACCESSIBILITY
  
  mozilla::a11y::Accessible* GetRootAccessible();
#endif

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

  virtual uint32_t GetGLFrameBufferFormat() MOZ_OVERRIDE;

  virtual const SizeConstraints& GetSizeConstraints() const;
  virtual void SetSizeConstraints(const SizeConstraints& aConstraints);

  









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

  virtual void            ResolveIconName(const nsAString &aIconName,
                                          const nsAString &aIconSuffix,
                                          nsIFile **aResult);
  virtual void            OnDestroy();
  virtual void            BaseCreate(nsIWidget *aParent,
                                     const nsIntRect &aRect,
                                     nsDeviceContext *aContext,
                                     nsWidgetInitData *aInitData);

  virtual nsIContent* GetLastRollup()
  {
    return mLastRollup;
  }

  virtual nsresult SynthesizeNativeKeyEvent(int32_t aNativeKeyboardLayout,
                                            int32_t aNativeKeyCode,
                                            uint32_t aModifierFlags,
                                            const nsAString& aCharacters,
                                            const nsAString& aUnmodifiedCharacters)
  { return NS_ERROR_UNEXPECTED; }

  virtual nsresult SynthesizeNativeMouseEvent(nsIntPoint aPoint,
                                              uint32_t aNativeMessage,
                                              uint32_t aModifierFlags)
  { return NS_ERROR_UNEXPECTED; }

  virtual nsresult SynthesizeNativeMouseMove(nsIntPoint aPoint)
  { return NS_ERROR_UNEXPECTED; }

  virtual nsresult SynthesizeNativeMouseScrollEvent(nsIntPoint aPoint,
                                                    uint32_t aNativeMessage,
                                                    double aDeltaX,
                                                    double aDeltaY,
                                                    double aDeltaZ,
                                                    uint32_t aModifierFlags,
                                                    uint32_t aAdditionalFlags)
  { return NS_ERROR_UNEXPECTED; }

  virtual nsresult SynthesizeNativeTouchPoint(uint32_t aPointerId,
                                              TouchPointerState aPointerState,
                                              nsIntPoint aPointerScreenPoint,
                                              double aPointerPressure,
                                              uint32_t aPointerOrientation)
  { return NS_ERROR_UNEXPECTED; }

protected:
  
  
  bool StoreWindowClipRegion(const nsTArray<nsIntRect>& aRects);

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

protected:
  








  void DestroyCompositor();

  nsIWidgetListener* mWidgetListener;
  nsIWidgetListener* mAttachedWidgetListener;
  nsDeviceContext* mContext;
  nsRefPtr<LayerManager> mLayerManager;
  nsRefPtr<LayerManager> mBasicLayerManager;
  nsRefPtr<CompositorChild> mCompositorChild;
  nsRefPtr<CompositorParent> mCompositorParent;
  nsRefPtr<WidgetShutdownObserver> mShutdownObserver;
  nsCursor          mCursor;
  bool              mUpdateCursor;
  nsBorderStyle     mBorderStyle;
  bool              mUseLayersAcceleration;
  bool              mForceLayersAcceleration;
  bool              mTemporarilyUseBasicLayerManager;
  
  
  bool              mRequireOffMainThreadCompositing;
  bool              mUseAttachedEvents;
  bool              mContextInitialized;
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
