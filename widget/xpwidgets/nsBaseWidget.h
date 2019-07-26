



#ifndef nsBaseWidget_h__
#define nsBaseWidget_h__

#include "mozilla/WidgetUtils.h"
#include "nsRect.h"
#include "nsIWidget.h"
#include "nsWidgetsCID.h"
#include "nsIFile.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsGUIEvent.h"
#include "nsAutoPtr.h"
#include "nsIRollupListener.h"
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

public:
  nsBaseWidget();
  virtual ~nsBaseWidget();

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

  NS_IMETHOD              SetZIndex(int32_t aZIndex);
  NS_IMETHOD              GetZIndex(int32_t* aZIndex);
  NS_IMETHOD              PlaceBehind(nsTopLevelWidgetZPlacement aPlacement,
                                      nsIWidget *aWidget, bool aActivate);

  NS_IMETHOD              SetSizeMode(int32_t aMode);
  NS_IMETHOD              GetSizeMode(int32_t* aMode);

  virtual nscolor         GetForegroundColor(void);
  NS_IMETHOD              SetForegroundColor(const nscolor &aColor);
  virtual nscolor         GetBackgroundColor(void);
  NS_IMETHOD              SetBackgroundColor(const nscolor &aColor);
  virtual nsCursor        GetCursor();
  NS_IMETHOD              SetCursor(nsCursor aCursor);
  NS_IMETHOD              SetCursor(imgIContainer* aCursor,
                                    uint32_t aHotspotX, uint32_t aHotspotY);
  NS_IMETHOD              GetWindowType(nsWindowType& aWindowType);
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
  virtual LayerManager*   GetLayerManager(PLayersChild* aShadowManager = nullptr,
                                          LayersBackend aBackendHint = mozilla::layers::LAYERS_NONE,
                                          LayerManagerPersistence aPersistence = LAYER_MANAGER_CURRENT,
                                          bool* aAllowRetaining = nullptr);

  virtual void            CreateCompositor();
  virtual void            DrawWindowUnderlay(LayerManager* aManager, nsIntRect aRect) {}
  virtual void            DrawWindowOverlay(LayerManager* aManager, nsIntRect aRect) {}
  virtual void            UpdateThemeGeometries(const nsTArray<ThemeGeometry>& aThemeGeometries) {}
  virtual gfxASurface*    GetThebesSurface();
  NS_IMETHOD              SetModal(bool aModal); 
  NS_IMETHOD              SetWindowClass(const nsAString& xulWinType);
  NS_IMETHOD              MoveClient(double aX, double aY);
  NS_IMETHOD              ResizeClient(double aWidth, double aHeight, bool aRepaint);
  NS_IMETHOD              ResizeClient(double aX, double aY, double aWidth, double aHeight, bool aRepaint);
  NS_IMETHOD              GetBounds(nsIntRect &aRect);
  NS_IMETHOD              GetClientBounds(nsIntRect &aRect);
  NS_IMETHOD              GetScreenBounds(nsIntRect &aRect);
  NS_IMETHOD              GetNonClientMargins(nsIntMargin &margins);
  NS_IMETHOD              SetNonClientMargins(nsIntMargin &margins);
  virtual nsIntPoint      GetClientOffset();
  NS_IMETHOD              EnableDragDrop(bool aEnable);
  NS_IMETHOD              GetAttention(int32_t aCycleCount);
  virtual bool            HasPendingInputEvent();
  NS_IMETHOD              SetIcon(const nsAString &anIconSpec);
  NS_IMETHOD              BeginSecureKeyboardInput();
  NS_IMETHOD              EndSecureKeyboardInput();
  NS_IMETHOD              SetWindowTitlebarColor(nscolor aColor, bool aActive);
  virtual void            SetDrawsInTitlebar(bool aState) {}
  virtual bool            ShowsResizeIndicator(nsIntRect* aResizerRect);
  virtual void            FreeNativeData(void * data, uint32_t aDataType) {}
  NS_IMETHOD              BeginResizeDrag(nsGUIEvent* aEvent, int32_t aHorizontal, int32_t aVertical);
  NS_IMETHOD              BeginMoveDrag(nsMouseEvent* aEvent);
  virtual nsresult        ActivateNativeMenuItemAt(const nsAString& indexString) { return NS_ERROR_NOT_IMPLEMENTED; }
  virtual nsresult        ForceUpdateNativeMenuAt(const nsAString& indexString) { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              NotifyIME(NotificationToIME aNotification) MOZ_OVERRIDE { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              SetLayersAcceleration(bool aEnabled);
  virtual bool            GetLayersAcceleration() { return mUseLayersAcceleration; }
  virtual bool            ComputeShouldAccelerate(bool aDefault);
  NS_IMETHOD              GetToggledKeyState(uint32_t aKeyCode, bool* aLEDState) { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              NotifyIMEOfTextChange(uint32_t aStart, uint32_t aOldEnd, uint32_t aNewEnd) MOZ_OVERRIDE { return NS_ERROR_NOT_IMPLEMENTED; }
  virtual nsIMEUpdatePreference GetIMEUpdatePreference() { return nsIMEUpdatePreference(false, false); }
  NS_IMETHOD              OnDefaultButtonLoaded(const nsIntRect &aButtonRect) { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD              OverrideSystemMouseScrollSpeed(int32_t aOriginalDelta, bool aIsHorizontal, int32_t &aOverriddenDelta);
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

  
  
  void NotifySysColorChanged();
  void NotifyThemeChanged();
  void NotifyUIStateChanged(UIStateChangeType aShowAccelerators,
                            UIStateChangeType aShowFocusRings);

#ifdef ACCESSIBILITY
  
  mozilla::a11y::Accessible* GetAccessible();
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
    AutoUseBasicLayerManager(nsBaseWidget* aWidget);
    ~AutoUseBasicLayerManager();
  private:
    nsBaseWidget* mWidget;
    bool mPreviousTemporarilyUseBasicLayerManager;
  };
  friend class AutoUseBasicLayerManager;

  nsWindowType            GetWindowType() { return mWindowType; }

  virtual bool            ShouldUseOffMainThreadCompositing();

  static nsIRollupListener* GetActiveRollupListener();

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

  
  
  bool StoreWindowClipRegion(const nsTArray<nsIntRect>& aRects);

  virtual already_AddRefed<nsIWidget>
  AllocateChildPopupWidget()
  {
    static NS_DEFINE_IID(kCPopUpCID, NS_CHILD_CID);
    nsCOMPtr<nsIWidget> widget = do_CreateInstance(kCPopUpCID);
    return widget.forget();
  }

  BasicLayerManager* CreateBasicLayerManager();

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

protected:
  








  void DestroyCompositor();

  nsIWidgetListener* mWidgetListener;
  nsIWidgetListener* mAttachedWidgetListener;
  nsDeviceContext* mContext;
  nsRefPtr<LayerManager> mLayerManager;
  nsRefPtr<LayerManager> mBasicLayerManager;
  nsRefPtr<CompositorChild> mCompositorChild;
  nsRefPtr<CompositorParent> mCompositorParent;
  nscolor           mBackground;
  nscolor           mForeground;
  nsCursor          mCursor;
  nsWindowType      mWindowType;
  nsBorderStyle     mBorderStyle;
  bool              mUseLayersAcceleration;
  bool              mForceLayersAcceleration;
  bool              mTemporarilyUseBasicLayerManager;
  bool              mUseAttachedEvents;
  bool              mContextInitialized;
  nsIntRect         mBounds;
  nsIntRect*        mOriginalBounds;
  
  nsAutoArrayPtr<nsIntRect> mClipRects;
  uint32_t          mClipRectCount;
  int32_t           mZIndex;
  nsSizeMode        mSizeMode;
  nsPopupLevel      mPopupLevel;
  nsPopupType       mPopupType;
  SizeConstraints   mSizeConstraints;

  static nsIRollupListener* gRollupListener;

  
  
  static nsIContent* mLastRollup;

#ifdef DEBUG
protected:
  static nsAutoString debug_GuiEventToString(nsGUIEvent * aGuiEvent);
  static bool debug_WantPaintFlashing();

  static void debug_DumpInvalidate(FILE *                aFileOut,
                                   nsIWidget *           aWidget,
                                   const nsIntRect *     aRect,
                                   const nsAutoCString & aWidgetName,
                                   int32_t               aWindowID);

  static void debug_DumpEvent(FILE *                aFileOut,
                              nsIWidget *           aWidget,
                              nsGUIEvent *          aGuiEvent,
                              const nsAutoCString & aWidgetName,
                              int32_t               aWindowID);

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
