




#include "mozilla/ArrayUtils.h"
#include "mozilla/TextEventDispatcher.h"

#include "mozilla/layers/CompositorChild.h"
#include "mozilla/layers/CompositorParent.h"
#include "mozilla/layers/ImageBridgeChild.h"
#include "nsBaseWidget.h"
#include "nsDeviceContext.h"
#include "nsCOMPtr.h"
#include "nsGfxCIID.h"
#include "nsWidgetsCID.h"
#include "nsServiceManagerUtils.h"
#include "nsIScreenManager.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsISimpleEnumerator.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsIServiceManager.h"
#include "mozilla/Preferences.h"
#include "BasicLayers.h"
#include "ClientLayerManager.h"
#include "mozilla/layers/Compositor.h"
#include "nsIXULRuntime.h"
#include "nsIXULWindow.h"
#include "nsIBaseWindow.h"
#include "nsXULPopupManager.h"
#include "nsIWidgetListener.h"
#include "nsIGfxInfo.h"
#include "npapi.h"
#include "base/thread.h"
#include "prdtoa.h"
#include "prenv.h"
#include "mozilla/Attributes.h"
#include "mozilla/unused.h"
#include "nsContentUtils.h"
#include "gfxPrefs.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/MouseEvents.h"
#include "GLConsts.h"
#include "mozilla/unused.h"
#include "mozilla/VsyncDispatcher.h"
#include "mozilla/layers/APZCTreeManager.h"
#include "mozilla/layers/APZEventState.h"
#include "mozilla/layers/APZThreadUtils.h"
#include "mozilla/layers/ChromeProcessController.h"
#include "mozilla/layers/InputAPZContext.h"
#include "mozilla/layers/APZCCallbackHelper.h"
#include "mozilla/dom/TabParent.h"
#include "nsRefPtrHashtable.h"
#include "TouchEvents.h"
#ifdef ACCESSIBILITY
#include "nsAccessibilityService.h"
#endif

#ifdef DEBUG
#include "nsIObserver.h"

static void debug_RegisterPrefCallbacks();

#endif

#ifdef NOISY_WIDGET_LEAKS
static int32_t gNumWidgets;
#endif

#ifdef XP_MACOSX
#include "nsCocoaFeatures.h"
#endif

#if defined(XP_WIN) || defined(MOZ_WIDGET_GTK)
static nsRefPtrHashtable<nsVoidPtrHashKey, nsIWidget>* sPluginWidgetList;
#endif
nsIRollupListener* nsBaseWidget::gRollupListener = nullptr;

using namespace mozilla::layers;
using namespace mozilla::ipc;
using namespace mozilla::widget;
using namespace mozilla;
using base::Thread;

nsIContent* nsBaseWidget::mLastRollup = nullptr;


bool            gDisableNativeTheme               = false;


#define TOUCH_INJECT_PUMP_TIMER_MSEC 50
#define TOUCH_INJECT_LONG_TAP_DEFAULT_MSEC 1500
int32_t nsIWidget::sPointerIdCounter = 0;


 uint64_t AutoObserverNotifier::sObserverId = 0;
 nsDataHashtable<nsUint64HashKey, nsCOMPtr<nsIObserver>> AutoObserverNotifier::sSavedObservers;

nsAutoRollup::nsAutoRollup()
{
  
  
  
  wasClear = !nsBaseWidget::mLastRollup;
}

nsAutoRollup::~nsAutoRollup()
{
  if (nsBaseWidget::mLastRollup && wasClear) {
    NS_RELEASE(nsBaseWidget::mLastRollup);
  }
}

NS_IMPL_ISUPPORTS(nsBaseWidget, nsIWidget, nsISupportsWeakReference)







nsBaseWidget::nsBaseWidget()
: mWidgetListener(nullptr)
, mAttachedWidgetListener(nullptr)
, mCompositorVsyncDispatcher(nullptr)
, mCursor(eCursor_standard)
, mUpdateCursor(true)
, mBorderStyle(eBorderStyle_none)
, mUseLayersAcceleration(false)
, mForceLayersAcceleration(false)
, mTemporarilyUseBasicLayerManager(false)
, mUseAttachedEvents(false)
, mBounds(0,0,0,0)
, mOriginalBounds(nullptr)
, mClipRectCount(0)
, mSizeMode(nsSizeMode_Normal)
, mPopupLevel(ePopupLevelTop)
, mPopupType(ePopupTypeAny)
{
#ifdef NOISY_WIDGET_LEAKS
  gNumWidgets++;
  printf("WIDGETS+ = %d\n", gNumWidgets);
#endif

#ifdef DEBUG
  debug_RegisterPrefCallbacks();
#endif

#if defined(XP_WIN) || defined(MOZ_WIDGET_GTK)
  if (!sPluginWidgetList) {
    sPluginWidgetList = new nsRefPtrHashtable<nsVoidPtrHashKey, nsIWidget>();
  }
#endif
  mShutdownObserver = new WidgetShutdownObserver(this);
}

NS_IMPL_ISUPPORTS(WidgetShutdownObserver, nsIObserver)

WidgetShutdownObserver::WidgetShutdownObserver(nsBaseWidget* aWidget) :
  mWidget(aWidget),
  mRegistered(false)
{
  Register();
}

WidgetShutdownObserver::~WidgetShutdownObserver()
{
  
  
  
}

NS_IMETHODIMP
WidgetShutdownObserver::Observe(nsISupports *aSubject,
                                const char *aTopic,
                                const char16_t *aData)
{
  if (mWidget && !strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {
    nsCOMPtr<nsIWidget> kungFuDeathGrip(mWidget);
    mWidget->Shutdown();
  }
  return NS_OK;
}

void
WidgetShutdownObserver::Register()
{
  if (!mRegistered) {
    mRegistered = true;
    nsContentUtils::RegisterShutdownObserver(this);
  }
}

void
WidgetShutdownObserver::Unregister()
{
  if (mRegistered) {
    nsContentUtils::UnregisterShutdownObserver(this);
    mRegistered = false;
  }
}

void
nsBaseWidget::Shutdown()
{
  DestroyCompositor();
  FreeShutdownObserver();
#if defined(XP_WIN) || defined(MOZ_WIDGET_GTK)
  if (sPluginWidgetList) {
    delete sPluginWidgetList;
    sPluginWidgetList = nullptr;
  }
#endif
}

void nsBaseWidget::DestroyCompositor()
{
  if (mCompositorChild) {
    
    
    nsRefPtr<CompositorChild> compositorChild = mCompositorChild;
    nsRefPtr<CompositorParent> compositorParent = mCompositorParent;
    mCompositorChild->Destroy();
  }
}

void nsBaseWidget::DestroyLayerManager()
{
  if (mLayerManager) {
    mLayerManager->Destroy();
    mLayerManager = nullptr;
  }
  DestroyCompositor();
}

void
nsBaseWidget::FreeShutdownObserver()
{
  if (mShutdownObserver) {
    mShutdownObserver->Unregister();
  }
  mShutdownObserver = nullptr;
}






nsBaseWidget::~nsBaseWidget()
{
  if (mLayerManager &&
      mLayerManager->GetBackendType() == LayersBackend::LAYERS_BASIC) {
    static_cast<BasicLayerManager*>(mLayerManager.get())->ClearRetainerWidget();
  }

  FreeShutdownObserver();
  DestroyLayerManager();

#ifdef NOISY_WIDGET_LEAKS
  gNumWidgets--;
  printf("WIDGETS- = %d\n", gNumWidgets);
#endif

  delete mOriginalBounds;

  
  if (mCompositorVsyncDispatcher) {
    mCompositorVsyncDispatcher->Shutdown();
  }
}






void nsBaseWidget::BaseCreate(nsIWidget *aParent,
                              const nsIntRect &aRect,
                              nsWidgetInitData *aInitData)
{
  static bool gDisableNativeThemeCached = false;
  if (!gDisableNativeThemeCached) {
    Preferences::AddBoolVarCache(&gDisableNativeTheme,
                                 "mozilla.widget.disable-native-theme",
                                 gDisableNativeTheme);
    gDisableNativeThemeCached = true;
  }

  
  if (nullptr != aInitData) {
    mWindowType = aInitData->mWindowType;
    mBorderStyle = aInitData->mBorderStyle;
    mPopupLevel = aInitData->mPopupLevel;
    mPopupType = aInitData->mPopupHint;
    mRequireOffMainThreadCompositing = aInitData->mRequireOffMainThreadCompositing;
  }

  if (aParent) {
    aParent->AddChild(this);
  }
}

NS_IMETHODIMP nsBaseWidget::CaptureMouse(bool aCapture)
{
  return NS_OK;
}







nsIWidgetListener* nsBaseWidget::GetWidgetListener()
{
  return mWidgetListener;
}

void nsBaseWidget::SetWidgetListener(nsIWidgetListener* aWidgetListener)
{
  mWidgetListener = aWidgetListener;
}

already_AddRefed<nsIWidget>
nsBaseWidget::CreateChild(const nsIntRect  &aRect,
                          nsWidgetInitData *aInitData,
                          bool             aForceUseIWidgetParent)
{
  nsIWidget* parent = this;
  nsNativeWidget nativeParent = nullptr;

  if (!aForceUseIWidgetParent) {
    
    
    
    nativeParent = parent ? parent->GetNativeData(NS_NATIVE_WIDGET) : nullptr;
    parent = nativeParent ? nullptr : parent;
    MOZ_ASSERT(!parent || !nativeParent, "messed up logic");
  }

  nsCOMPtr<nsIWidget> widget;
  if (aInitData && aInitData->mWindowType == eWindowType_popup) {
    widget = AllocateChildPopupWidget();
  } else {
    static NS_DEFINE_IID(kCChildCID, NS_CHILD_CID);
    widget = do_CreateInstance(kCChildCID);
  }

  if (widget &&
      NS_SUCCEEDED(widget->Create(parent, nativeParent, aRect, aInitData))) {
    return widget.forget();
  }

  return nullptr;
}


NS_IMETHODIMP
nsBaseWidget::AttachViewToTopLevel(bool aUseAttachedEvents)
{
  NS_ASSERTION((mWindowType == eWindowType_toplevel ||
                mWindowType == eWindowType_dialog ||
                mWindowType == eWindowType_invisible ||
                mWindowType == eWindowType_child),
               "Can't attach to window of that type");

  mUseAttachedEvents = aUseAttachedEvents;

  return NS_OK;
}

nsIWidgetListener* nsBaseWidget::GetAttachedWidgetListener()
 {
   return mAttachedWidgetListener;
 }

void nsBaseWidget::SetAttachedWidgetListener(nsIWidgetListener* aListener)
 {
   mAttachedWidgetListener = aListener;
 }






NS_METHOD nsBaseWidget::Destroy()
{
  
  nsCOMPtr<nsIWidget> kungFuDeathGrip(this);
  
  nsIWidget *parent = GetParent();
  if (parent) {
    parent->RemoveChild(this);
  }

  return NS_OK;
}







NS_IMETHODIMP nsBaseWidget::SetParent(nsIWidget* aNewParent)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}







nsIWidget* nsBaseWidget::GetParent(void)
{
  return nullptr;
}






nsIWidget* nsBaseWidget::GetTopLevelWidget()
{
  nsIWidget *topLevelWidget = nullptr, *widget = this;
  while (widget) {
    topLevelWidget = widget;
    widget = widget->GetParent();
  }
  return topLevelWidget;
}






nsIWidget* nsBaseWidget::GetSheetWindowParent(void)
{
  return nullptr;
}

float nsBaseWidget::GetDPI()
{
  return 96.0f;
}

CSSToLayoutDeviceScale nsIWidget::GetDefaultScale()
{
  double devPixelsPerCSSPixel = DefaultScaleOverride();

  if (devPixelsPerCSSPixel <= 0.0) {
    devPixelsPerCSSPixel = GetDefaultScaleInternal();
  }

  return CSSToLayoutDeviceScale(devPixelsPerCSSPixel);
}


double nsIWidget::DefaultScaleOverride()
{
  
  
  
  double devPixelsPerCSSPixel = -1.0;

  nsAdoptingCString prefString = Preferences::GetCString("layout.css.devPixelsPerPx");
  if (!prefString.IsEmpty()) {
    devPixelsPerCSSPixel = PR_strtod(prefString, nullptr);
  }

  return devPixelsPerCSSPixel;
}






void nsBaseWidget::AddChild(nsIWidget* aChild)
{
  NS_PRECONDITION(!aChild->GetNextSibling() && !aChild->GetPrevSibling(),
                  "aChild not properly removed from its old child list");

  if (!mFirstChild) {
    mFirstChild = mLastChild = aChild;
  } else {
    
    NS_ASSERTION(mLastChild, "Bogus state");
    NS_ASSERTION(!mLastChild->GetNextSibling(), "Bogus state");
    mLastChild->SetNextSibling(aChild);
    aChild->SetPrevSibling(mLastChild);
    mLastChild = aChild;
  }
}







void nsBaseWidget::RemoveChild(nsIWidget* aChild)
{
#ifdef DEBUG
#ifdef XP_MACOSX
  
  
  nsIWidget* parent = aChild->GetParent();
  NS_ASSERTION(!parent || parent == this, "Not one of our kids!");
#else
  NS_ASSERTION(aChild->GetParent() == this, "Not one of our kids!");
#endif
#endif

  if (mLastChild == aChild) {
    mLastChild = mLastChild->GetPrevSibling();
  }
  if (mFirstChild == aChild) {
    mFirstChild = mFirstChild->GetNextSibling();
  }

  
  
  nsIWidget* prev = aChild->GetPrevSibling();
  nsIWidget* next = aChild->GetNextSibling();
  if (prev) {
    prev->SetNextSibling(next);
  }
  if (next) {
    next->SetPrevSibling(prev);
  }

  aChild->SetNextSibling(nullptr);
  aChild->SetPrevSibling(nullptr);
}







void nsBaseWidget::SetZIndex(int32_t aZIndex)
{
  
  
  nsCOMPtr<nsIWidget> kungFuDeathGrip(this);

  mZIndex = aZIndex;

  
  nsBaseWidget* parent = static_cast<nsBaseWidget*>(GetParent());
  if (parent) {
    parent->RemoveChild(this);
    
    nsIWidget* sib = parent->GetFirstChild();
    for ( ; sib; sib = sib->GetNextSibling()) {
      int32_t childZIndex = GetZIndex();
      if (aZIndex < childZIndex) {
        
        nsIWidget* prev = sib->GetPrevSibling();
        mNextSibling = sib;
        mPrevSibling = prev;
        sib->SetPrevSibling(this);
        if (prev) {
          prev->SetNextSibling(this);
        } else {
          NS_ASSERTION(sib == parent->mFirstChild, "Broken child list");
          
          
          parent->mFirstChild = this;
        }
        PlaceBehind(eZPlacementBelow, sib, false);
        break;
      }
    }
    
    if (!sib) {
      parent->AddChild(this);
    }
  }
}






NS_IMETHODIMP nsBaseWidget::PlaceBehind(nsTopLevelWidgetZPlacement aPlacement,
                                        nsIWidget *aWidget, bool aActivate)
{
  return NS_OK;
}







NS_IMETHODIMP nsBaseWidget::SetSizeMode(int32_t aMode)
{
  if (aMode == nsSizeMode_Normal ||
      aMode == nsSizeMode_Minimized ||
      aMode == nsSizeMode_Maximized ||
      aMode == nsSizeMode_Fullscreen) {

    mSizeMode = (nsSizeMode) aMode;
    return NS_OK;
  }
  return NS_ERROR_ILLEGAL_VALUE;
}






nsCursor nsBaseWidget::GetCursor()
{
  return mCursor;
}

NS_METHOD nsBaseWidget::SetCursor(nsCursor aCursor)
{
  mCursor = aCursor;
  return NS_OK;
}

NS_IMETHODIMP nsBaseWidget::SetCursor(imgIContainer* aCursor,
                                      uint32_t aHotspotX, uint32_t aHotspotY)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}







void nsBaseWidget::SetTransparencyMode(nsTransparencyMode aMode) {
}

nsTransparencyMode nsBaseWidget::GetTransparencyMode() {
  return eTransparencyOpaque;
}

bool
nsBaseWidget::IsWindowClipRegionEqual(const nsTArray<nsIntRect>& aRects)
{
  return mClipRects &&
         mClipRectCount == aRects.Length() &&
         memcmp(mClipRects, aRects.Elements(), sizeof(nsIntRect)*mClipRectCount) == 0;
}

void
nsBaseWidget::StoreWindowClipRegion(const nsTArray<nsIntRect>& aRects)
{
  mClipRectCount = aRects.Length();
  mClipRects = new nsIntRect[mClipRectCount];
  if (mClipRects) {
    memcpy(mClipRects, aRects.Elements(), sizeof(nsIntRect)*mClipRectCount);
  }
}

void
nsBaseWidget::GetWindowClipRegion(nsTArray<nsIntRect>* aRects)
{
  if (mClipRects) {
    aRects->AppendElements(mClipRects.get(), mClipRectCount);
  } else {
    aRects->AppendElement(nsIntRect(0, 0, mBounds.width, mBounds.height));
  }
}

const nsIntRegion
nsBaseWidget::RegionFromArray(const nsTArray<nsIntRect>& aRects)
{
  nsIntRegion region;
  for (uint32_t i = 0; i < aRects.Length(); ++i) {
    region.Or(region, aRects[i]);
  }
  return region;
}

void
nsBaseWidget::ArrayFromRegion(const nsIntRegion& aRegion, nsTArray<nsIntRect>& aRects)
{
  const nsIntRect* r;
  for (nsIntRegionRectIterator iter(aRegion); (r = iter.Next());) {
    aRects.AppendElement(*r);
  }
}

nsresult
nsBaseWidget::SetWindowClipRegion(const nsTArray<nsIntRect>& aRects,
                                  bool aIntersectWithExisting)
{
  if (!aIntersectWithExisting) {
    StoreWindowClipRegion(aRects);
  } else {
    
    nsTArray<nsIntRect> currentRects;
    GetWindowClipRegion(&currentRects);
    
    nsIntRegion currentRegion = RegionFromArray(currentRects);
    
    nsIntRegion newRegion = RegionFromArray(aRects);
    
    nsIntRegion intersection;
    intersection.And(currentRegion, newRegion);
    
    nsTArray<nsIntRect> rects;
    ArrayFromRegion(intersection, rects);
    
    StoreWindowClipRegion(rects);
  }
  return NS_OK;
}







NS_IMETHODIMP nsBaseWidget::SetWindowShadowStyle(int32_t aMode)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}






NS_IMETHODIMP nsBaseWidget::HideWindowChrome(bool aShouldHide)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}






NS_IMETHODIMP nsBaseWidget::MakeFullScreen(bool aFullScreen, nsIScreen* aScreen)
{
  HideWindowChrome(aFullScreen);

  if (aFullScreen) {
    if (!mOriginalBounds)
      mOriginalBounds = new nsIntRect();
    GetScreenBounds(*mOriginalBounds);
    
    CSSToLayoutDeviceScale scale = GetDefaultScale();
    mOriginalBounds->x = NSToIntRound(mOriginalBounds->x / scale.scale);
    mOriginalBounds->y = NSToIntRound(mOriginalBounds->y / scale.scale);
    mOriginalBounds->width = NSToIntRound(mOriginalBounds->width / scale.scale);
    mOriginalBounds->height = NSToIntRound(mOriginalBounds->height / scale.scale);

    
    nsCOMPtr<nsIScreenManager> screenManager;
    screenManager = do_GetService("@mozilla.org/gfx/screenmanager;1");
    NS_ASSERTION(screenManager, "Unable to grab screenManager.");
    if (screenManager) {
      nsCOMPtr<nsIScreen> screen = aScreen;
      if (!screen) {
        
        screenManager->ScreenForRect(mOriginalBounds->x,
                                     mOriginalBounds->y,
                                     mOriginalBounds->width,
                                     mOriginalBounds->height,
                                     getter_AddRefs(screen));
      }

      if (screen) {
        int32_t left, top, width, height;
        if (NS_SUCCEEDED(screen->GetRectDisplayPix(&left, &top, &width, &height))) {
          Resize(left, top, width, height, true);
        }
      }
    }

  } else if (mOriginalBounds) {
    Resize(mOriginalBounds->x, mOriginalBounds->y, mOriginalBounds->width,
           mOriginalBounds->height, true);
  }

  return NS_OK;
}

nsBaseWidget::AutoLayerManagerSetup::AutoLayerManagerSetup(
    nsBaseWidget* aWidget, gfxContext* aTarget,
    BufferMode aDoubleBuffering, ScreenRotation aRotation)
  : mWidget(aWidget)
{
  mLayerManager = static_cast<BasicLayerManager*>(mWidget->GetLayerManager());
  if (mLayerManager) {
    NS_ASSERTION(mLayerManager->GetBackendType() == LayersBackend::LAYERS_BASIC,
      "AutoLayerManagerSetup instantiated for non-basic layer backend!");
    mLayerManager->SetDefaultTarget(aTarget);
    mLayerManager->SetDefaultTargetConfiguration(aDoubleBuffering, aRotation);
  }
}

nsBaseWidget::AutoLayerManagerSetup::~AutoLayerManagerSetup()
{
  if (mLayerManager) {
    NS_ASSERTION(mLayerManager->GetBackendType() == LayersBackend::LAYERS_BASIC,
      "AutoLayerManagerSetup instantiated for non-basic layer backend!");
    mLayerManager->SetDefaultTarget(nullptr);
    mLayerManager->SetDefaultTargetConfiguration(mozilla::layers::BufferMode::BUFFER_NONE, ROTATION_0);
  }
}

nsBaseWidget::AutoUseBasicLayerManager::AutoUseBasicLayerManager(nsBaseWidget* aWidget)
  : mWidget(aWidget)
{
  mPreviousTemporarilyUseBasicLayerManager =
    mWidget->mTemporarilyUseBasicLayerManager;
  mWidget->mTemporarilyUseBasicLayerManager = true;
}

nsBaseWidget::AutoUseBasicLayerManager::~AutoUseBasicLayerManager()
{
  mWidget->mTemporarilyUseBasicLayerManager =
    mPreviousTemporarilyUseBasicLayerManager;
}

bool
nsBaseWidget::ComputeShouldAccelerate(bool aDefault)
{
#if defined(XP_WIN) || defined(ANDROID) || \
    defined(MOZ_GL_PROVIDER) || defined(XP_MACOSX) || defined(MOZ_WIDGET_QT)
  bool accelerateByDefault = true;
#else
  bool accelerateByDefault = false;
#endif

#ifdef XP_MACOSX
  
  
  
  
  
  accelerateByDefault = nsCocoaFeatures::AccelerateByDefault();
#endif

  
  bool disableAcceleration = gfxPrefs::LayersAccelerationDisabled();
  mForceLayersAcceleration = gfxPrefs::LayersAccelerationForceEnabled();

  const char *acceleratedEnv = PR_GetEnv("MOZ_ACCELERATED");
  accelerateByDefault = accelerateByDefault ||
                        (acceleratedEnv && (*acceleratedEnv != '0'));

  nsCOMPtr<nsIXULRuntime> xr = do_GetService("@mozilla.org/xre/runtime;1");
  bool safeMode = false;
  if (xr)
    xr->GetInSafeMode(&safeMode);

  bool whitelisted = false;

  nsCOMPtr<nsIGfxInfo> gfxInfo = do_GetService("@mozilla.org/gfx/info;1");
  if (gfxInfo) {
    
    
    
    
    gfxInfo->GetData();

    int32_t status;
    if (NS_SUCCEEDED(gfxInfo->GetFeatureStatus(nsIGfxInfo::FEATURE_OPENGL_LAYERS, &status))) {
      if (status == nsIGfxInfo::FEATURE_STATUS_OK) {
        whitelisted = true;
      }
    }
  }

  if (disableAcceleration || safeMode)
    return false;

  if (mForceLayersAcceleration)
    return true;

  if (!whitelisted) {
    static int tell_me_once = 0;
    if (!tell_me_once) {
      NS_WARNING("OpenGL-accelerated layers are not supported on this system");
      tell_me_once = 1;
    }
#ifdef MOZ_WIDGET_ANDROID
    NS_RUNTIMEABORT("OpenGL-accelerated layers are a hard requirement on this platform. "
                    "Cannot continue without support for them");
#endif
    return false;
  }

  if (accelerateByDefault)
    return true;

  
  return aDefault;
}

CompositorParent* nsBaseWidget::NewCompositorParent(int aSurfaceWidth,
                                                    int aSurfaceHeight)
{
  return new CompositorParent(this, false, aSurfaceWidth, aSurfaceHeight);
}

void nsBaseWidget::CreateCompositor()
{
  nsIntRect rect;
  GetBounds(rect);
  CreateCompositor(rect.width, rect.height);
}

already_AddRefed<GeckoContentController>
nsBaseWidget::CreateRootContentController()
{
  nsRefPtr<GeckoContentController> controller = new ChromeProcessController(this, mAPZEventState);
  return controller.forget();
}

class ChromeProcessSetAllowedTouchBehaviorCallback : public SetAllowedTouchBehaviorCallback {
public:
  explicit ChromeProcessSetAllowedTouchBehaviorCallback(APZCTreeManager* aTreeManager)
    : mTreeManager(aTreeManager)
  {}

  void Run(uint64_t aInputBlockId, const nsTArray<TouchBehaviorFlags>& aFlags) const override {
    MOZ_ASSERT(NS_IsMainThread());
    APZThreadUtils::RunOnControllerThread(NewRunnableMethod(
        mTreeManager.get(), &APZCTreeManager::SetAllowedTouchBehavior,
        aInputBlockId, aFlags));
  }

private:
  nsRefPtr<APZCTreeManager> mTreeManager;
};

class ChromeProcessContentReceivedInputBlockCallback : public ContentReceivedInputBlockCallback {
public:
  explicit ChromeProcessContentReceivedInputBlockCallback(APZCTreeManager* aTreeManager)
    : mTreeManager(aTreeManager)
  {}

  void Run(const ScrollableLayerGuid& aGuid, uint64_t aInputBlockId, bool aPreventDefault) const override {
    MOZ_ASSERT(NS_IsMainThread());
    APZThreadUtils::RunOnControllerThread(NewRunnableMethod(
        mTreeManager.get(), &APZCTreeManager::ContentReceivedInputBlock,
        aInputBlockId, aPreventDefault));
  }

private:
  nsRefPtr<APZCTreeManager> mTreeManager;
};


void nsBaseWidget::ConfigureAPZCTreeManager()
{
  uint64_t rootLayerTreeId = mCompositorParent->RootLayerTreeId();
  mAPZC = CompositorParent::GetAPZCTreeManager(rootLayerTreeId);
  MOZ_ASSERT(mAPZC);

  ConfigureAPZControllerThread();

  mAPZC->SetDPI(GetDPI());
  mAPZEventState = new APZEventState(this,
      new ChromeProcessContentReceivedInputBlockCallback(mAPZC));
  mSetAllowedTouchBehaviorCallback = new ChromeProcessSetAllowedTouchBehaviorCallback(mAPZC);

  nsRefPtr<GeckoContentController> controller = CreateRootContentController();
  if (controller) {
    CompositorParent::SetControllerForLayerTree(rootLayerTreeId, controller);
  }
}

void nsBaseWidget::ConfigureAPZControllerThread()
{
  
  APZThreadUtils::SetControllerThread(MessageLoop::current());
}

void
nsBaseWidget::SetConfirmedTargetAPZC(uint64_t aInputBlockId,
                                     const nsTArray<ScrollableLayerGuid>& aTargets) const
{
  
  void (APZCTreeManager::*setTargetApzcFunc)(uint64_t, const nsTArray<ScrollableLayerGuid>&)
          = &APZCTreeManager::SetTargetAPZC;
  APZThreadUtils::RunOnControllerThread(NewRunnableMethod(
    mAPZC.get(), setTargetApzcFunc, aInputBlockId, mozilla::Move(aTargets)));
}

nsEventStatus
nsBaseWidget::ProcessUntransformedAPZEvent(WidgetInputEvent* aEvent,
                                           const ScrollableLayerGuid& aGuid,
                                           uint64_t aInputBlockId,
                                           nsEventStatus aApzResponse)
{
  MOZ_ASSERT(NS_IsMainThread());
  InputAPZContext context(aGuid, aInputBlockId);

  
  
  
  
  
  
  if (aEvent->AsTouchEvent() && aGuid.mLayersId == mCompositorParent->RootLayerTreeId()) {
    APZCCallbackHelper::ApplyCallbackTransform(*aEvent->AsTouchEvent(), aGuid,
        GetDefaultScale(), 1.0f);
  }

  nsEventStatus status;
  DispatchEvent(aEvent, status);

  if (mAPZC && !context.WasRoutedToChildProcess()) {
    
    
    
    
    if (WidgetTouchEvent* touchEvent = aEvent->AsTouchEvent()) {
      if (touchEvent->message == NS_TOUCH_START) {
        if (gfxPrefs::TouchActionEnabled()) {
          APZCCallbackHelper::SendSetAllowedTouchBehaviorNotification(this, *touchEvent,
              aInputBlockId, mSetAllowedTouchBehaviorCallback);
        }
        APZCCallbackHelper::SendSetTargetAPZCNotification(this, GetDocument(), *aEvent,
            aGuid, aInputBlockId);
      }
      mAPZEventState->ProcessTouchEvent(*touchEvent, aGuid, aInputBlockId, aApzResponse);
    } else if (WidgetWheelEvent* wheelEvent = aEvent->AsWheelEvent()) {
      APZCCallbackHelper::SendSetTargetAPZCNotification(this, GetDocument(), *aEvent,
                aGuid, aInputBlockId);
      mAPZEventState->ProcessWheelEvent(*wheelEvent, aGuid, aInputBlockId);
    }
  }

  return status;
}

nsEventStatus
nsBaseWidget::DispatchInputEvent(WidgetInputEvent* aEvent)
{
  if (mAPZC) {
    nsEventStatus result = mAPZC->ReceiveInputEvent(*aEvent, nullptr, nullptr);
    if (result == nsEventStatus_eConsumeNoDefault) {
      return result;
    }
  }

  nsEventStatus status;
  DispatchEvent(aEvent, status);
  return status;
}

nsEventStatus
nsBaseWidget::DispatchAPZAwareEvent(WidgetInputEvent* aEvent)
{
  if (mAPZC) {
    uint64_t inputBlockId = 0;
    ScrollableLayerGuid guid;

    nsEventStatus result = mAPZC->ReceiveInputEvent(*aEvent, &guid, &inputBlockId);
    if (result == nsEventStatus_eConsumeNoDefault) {
        return result;
    }
    return ProcessUntransformedAPZEvent(aEvent, guid, inputBlockId, result);
  }

  nsEventStatus status;
  DispatchEvent(aEvent, status);
  return status;
}

void
nsBaseWidget::GetPreferredCompositorBackends(nsTArray<LayersBackend>& aHints)
{
  if (mUseLayersAcceleration) {
    aHints.AppendElement(LayersBackend::LAYERS_OPENGL);
  }

  aHints.AppendElement(LayersBackend::LAYERS_BASIC);
}

nsIDocument*
nsBaseWidget::GetDocument() const
{
  if (mWidgetListener) {
    if (nsIPresShell* presShell = mWidgetListener->GetPresShell()) {
      return presShell->GetDocument();
    }
  }
  return nullptr;
}

void nsBaseWidget::CreateCompositorVsyncDispatcher()
{
  if (gfxPrefs::HardwareVsyncEnabled()) {
    
    
    
    if (XRE_IsParentProcess()) {
      mCompositorVsyncDispatcher = new CompositorVsyncDispatcher();
    }
  }
}

CompositorVsyncDispatcher*
nsBaseWidget::GetCompositorVsyncDispatcher()
{
  return mCompositorVsyncDispatcher;
}

void nsBaseWidget::CreateCompositor(int aWidth, int aHeight)
{
  
  gfxPlatform::GetPlatform();

  MOZ_ASSERT(gfxPlatform::UsesOffMainThreadCompositing(),
             "This function assumes OMTC");

  MOZ_ASSERT(!mCompositorParent && !mCompositorChild,
    "Should have properly cleaned up the previous PCompositor pair beforehand");

  if (mCompositorChild) {
    mCompositorChild->Destroy();
  }

  
  

  
  
  if (!mShutdownObserver) {
    return;
  }

  CreateCompositorVsyncDispatcher();
  mCompositorParent = NewCompositorParent(aWidth, aHeight);
  nsRefPtr<ClientLayerManager> lm = new ClientLayerManager(this);
  mCompositorChild = new CompositorChild(lm);
  mCompositorChild->OpenSameProcess(mCompositorParent);

  
  mCompositorParent->SetOtherProcessId(base::GetCurrentProcId());

  if (gfxPrefs::AsyncPanZoomEnabled() &&
      (WindowType() == eWindowType_toplevel || WindowType() == eWindowType_child)) {
    ConfigureAPZCTreeManager();
  }

  TextureFactoryIdentifier textureFactoryIdentifier;
  PLayerTransactionChild* shadowManager = nullptr;
  nsTArray<LayersBackend> backendHints;
  GetPreferredCompositorBackends(backendHints);

  bool success = false;
  if (!backendHints.IsEmpty()) {
    shadowManager = mCompositorChild->SendPLayerTransactionConstructor(
      backendHints, 0, &textureFactoryIdentifier, &success);
  }

  ShadowLayerForwarder* lf = lm->AsShadowForwarder();

  if (!success || !lf) {
    NS_WARNING("Failed to create an OMT compositor.");
    DestroyCompositor();
    return;
  }

  lf->SetShadowManager(shadowManager);
  lf->IdentifyTextureHost(textureFactoryIdentifier);
  ImageBridgeChild::IdentifyCompositorTextureHost(textureFactoryIdentifier);
  WindowUsesOMTC();

  mLayerManager = lm.forget();
}

bool nsBaseWidget::ShouldUseOffMainThreadCompositing()
{
  return gfxPlatform::UsesOffMainThreadCompositing();
}

LayerManager* nsBaseWidget::GetLayerManager(PLayerTransactionChild* aShadowManager,
                                            LayersBackend aBackendHint,
                                            LayerManagerPersistence aPersistence,
                                            bool* aAllowRetaining)
{
  if (!mLayerManager) {

    mUseLayersAcceleration = ComputeShouldAccelerate(mUseLayersAcceleration);

    
    if (ShouldUseOffMainThreadCompositing()) {
      
      
      NS_ASSERTION(aShadowManager == nullptr, "Async Compositor not supported with e10s");
      CreateCompositor();
    }

    if (!mLayerManager) {
      mLayerManager = CreateBasicLayerManager();
    }
  }
  if (mTemporarilyUseBasicLayerManager && !mBasicLayerManager) {
    mBasicLayerManager = CreateBasicLayerManager();
  }
  LayerManager* usedLayerManager = mTemporarilyUseBasicLayerManager ?
                                     mBasicLayerManager : mLayerManager;
  if (aAllowRetaining) {
    *aAllowRetaining = (usedLayerManager == mLayerManager);
  }
  return usedLayerManager;
}

LayerManager* nsBaseWidget::CreateBasicLayerManager()
{
  return new BasicLayerManager(this);
}

CompositorChild* nsBaseWidget::GetRemoteRenderer()
{
  return mCompositorChild;
}

TemporaryRef<mozilla::gfx::DrawTarget> nsBaseWidget::StartRemoteDrawing()
{
  return nullptr;
}






void nsBaseWidget::OnDestroy()
{
  if (mTextEventDispatcher) {
    mTextEventDispatcher->OnDestroyWidget();
    
    
  }
}

NS_METHOD nsBaseWidget::SetWindowClass(const nsAString& xulWinType)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_METHOD nsBaseWidget::MoveClient(double aX, double aY)
{
  nsIntPoint clientOffset(GetClientOffset());

  
  
  CSSToLayoutDeviceScale scale = BoundsUseDisplayPixels()
                                    ? GetDefaultScale()
                                    : CSSToLayoutDeviceScale(1.0);
  aX -= clientOffset.x * 1.0 / scale.scale;
  aY -= clientOffset.y * 1.0 / scale.scale;

  return Move(aX, aY);
}

NS_METHOD nsBaseWidget::ResizeClient(double aWidth,
                                     double aHeight,
                                     bool aRepaint)
{
  NS_ASSERTION((aWidth >=0) , "Negative width passed to ResizeClient");
  NS_ASSERTION((aHeight >=0), "Negative height passed to ResizeClient");

  nsIntRect clientBounds;
  GetClientBounds(clientBounds);

  
  
  CSSToLayoutDeviceScale scale = BoundsUseDisplayPixels()
                                    ? GetDefaultScale()
                                    : CSSToLayoutDeviceScale(1.0);
  double invScale = 1.0 / scale.scale;
  aWidth = mBounds.width * invScale + (aWidth - clientBounds.width * invScale);
  aHeight = mBounds.height * invScale + (aHeight - clientBounds.height * invScale);

  return Resize(aWidth, aHeight, aRepaint);
}

NS_METHOD nsBaseWidget::ResizeClient(double aX,
                                     double aY,
                                     double aWidth,
                                     double aHeight,
                                     bool aRepaint)
{
  NS_ASSERTION((aWidth >=0) , "Negative width passed to ResizeClient");
  NS_ASSERTION((aHeight >=0), "Negative height passed to ResizeClient");

  nsIntRect clientBounds;
  GetClientBounds(clientBounds);

  double scale = BoundsUseDisplayPixels() ? 1.0 / GetDefaultScale().scale : 1.0;
  aWidth = mBounds.width * scale + (aWidth - clientBounds.width * scale);
  aHeight = mBounds.height * scale + (aHeight - clientBounds.height * scale);

  nsIntPoint clientOffset(GetClientOffset());
  aX -= clientOffset.x * scale;
  aY -= clientOffset.y * scale;

  return Resize(aX, aY, aWidth, aHeight, aRepaint);
}











NS_METHOD nsBaseWidget::GetClientBounds(nsIntRect &aRect)
{
  return GetBounds(aRect);
}





NS_METHOD nsBaseWidget::GetBounds(nsIntRect &aRect)
{
  aRect = mBounds;
  return NS_OK;
}






NS_METHOD nsBaseWidget::GetScreenBounds(nsIntRect &aRect)
{
  return GetBounds(aRect);
}

NS_METHOD nsBaseWidget::GetRestoredBounds(nsIntRect &aRect)
{
  if (SizeMode() != nsSizeMode_Normal) {
    return NS_ERROR_FAILURE;
  }
  return GetScreenBounds(aRect);
}

nsIntPoint nsBaseWidget::GetClientOffset()
{
  return nsIntPoint(0, 0);
}

NS_IMETHODIMP
nsBaseWidget::GetNonClientMargins(nsIntMargin &margins)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsBaseWidget::SetNonClientMargins(nsIntMargin &margins)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_METHOD nsBaseWidget::EnableDragDrop(bool aEnable)
{
  return NS_OK;
}

uint32_t nsBaseWidget::GetMaxTouchPoints() const
{
  return 0;
}

NS_METHOD nsBaseWidget::SetModal(bool aModal)
{
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsBaseWidget::GetAttention(int32_t aCycleCount) {
    return NS_OK;
}

bool
nsBaseWidget::HasPendingInputEvent()
{
  return false;
}

NS_IMETHODIMP
nsBaseWidget::SetIcon(const nsAString&)
{
  return NS_OK;
}

NS_IMETHODIMP
nsBaseWidget::SetWindowTitlebarColor(nscolor aColor, bool aActive)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

bool
nsBaseWidget::ShowsResizeIndicator(nsIntRect* aResizerRect)
{
  return false;
}

NS_IMETHODIMP
nsBaseWidget::OverrideSystemMouseScrollSpeed(double aOriginalDeltaX,
                                             double aOriginalDeltaY,
                                             double& aOverriddenDeltaX,
                                             double& aOverriddenDeltaY)
{
  aOverriddenDeltaX = aOriginalDeltaX;
  aOverriddenDeltaY = aOriginalDeltaY;

  static bool sInitialized = false;
  static bool sIsOverrideEnabled = false;
  static int32_t sIntFactorX = 0;
  static int32_t sIntFactorY = 0;

  if (!sInitialized) {
    Preferences::AddBoolVarCache(&sIsOverrideEnabled,
      "mousewheel.system_scroll_override_on_root_content.enabled", false);
    Preferences::AddIntVarCache(&sIntFactorX,
      "mousewheel.system_scroll_override_on_root_content.horizontal.factor", 0);
    Preferences::AddIntVarCache(&sIntFactorY,
      "mousewheel.system_scroll_override_on_root_content.vertical.factor", 0);
    sIntFactorX = std::max(sIntFactorX, 0);
    sIntFactorY = std::max(sIntFactorY, 0);
    sInitialized = true;
  }

  if (!sIsOverrideEnabled) {
    return NS_OK;
  }

  
  
  if (sIntFactorX > 100) {
    double factor = static_cast<double>(sIntFactorX) / 100;
    aOverriddenDeltaX *= factor;
  }
  if (sIntFactorY > 100) {
    double factor = static_cast<double>(sIntFactorY) / 100;
    aOverriddenDeltaY *= factor;
  }

  return NS_OK;
}







static bool
ResolveIconNameHelper(nsIFile *aFile,
                      const nsAString &aIconName,
                      const nsAString &aIconSuffix)
{
  aFile->Append(NS_LITERAL_STRING("icons"));
  aFile->Append(NS_LITERAL_STRING("default"));
  aFile->Append(aIconName + aIconSuffix);

  bool readable;
  return NS_SUCCEEDED(aFile->IsReadable(&readable)) && readable;
}








void
nsBaseWidget::ResolveIconName(const nsAString &aIconName,
                              const nsAString &aIconSuffix,
                              nsIFile **aResult)
{
  *aResult = nullptr;

  nsCOMPtr<nsIProperties> dirSvc = do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID);
  if (!dirSvc)
    return;

  

  nsCOMPtr<nsISimpleEnumerator> dirs;
  dirSvc->Get(NS_APP_CHROME_DIR_LIST, NS_GET_IID(nsISimpleEnumerator),
              getter_AddRefs(dirs));
  if (dirs) {
    bool hasMore;
    while (NS_SUCCEEDED(dirs->HasMoreElements(&hasMore)) && hasMore) {
      nsCOMPtr<nsISupports> element;
      dirs->GetNext(getter_AddRefs(element));
      if (!element)
        continue;
      nsCOMPtr<nsIFile> file = do_QueryInterface(element);
      if (!file)
        continue;
      if (ResolveIconNameHelper(file, aIconName, aIconSuffix)) {
        NS_ADDREF(*aResult = file);
        return;
      }
    }
  }

  

  nsCOMPtr<nsIFile> file;
  dirSvc->Get(NS_APP_CHROME_DIR, NS_GET_IID(nsIFile),
              getter_AddRefs(file));
  if (file && ResolveIconNameHelper(file, aIconName, aIconSuffix))
    NS_ADDREF(*aResult = file);
}

NS_IMETHODIMP
nsBaseWidget::BeginResizeDrag(WidgetGUIEvent* aEvent,
                              int32_t aHorizontal,
                              int32_t aVertical)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsBaseWidget::BeginMoveDrag(WidgetMouseEvent* aEvent)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

uint32_t
nsBaseWidget::GetGLFrameBufferFormat()
{
  return LOCAL_GL_RGBA;
}

void nsBaseWidget::SetSizeConstraints(const SizeConstraints& aConstraints)
{
  mSizeConstraints = aConstraints;
  
  
}

const widget::SizeConstraints& nsBaseWidget::GetSizeConstraints() const
{
  return mSizeConstraints;
}


nsIRollupListener*
nsBaseWidget::GetActiveRollupListener()
{
  
  if (gRollupListener)
    return gRollupListener;

  return nsXULPopupManager::GetInstance();
}

void
nsBaseWidget::NotifyWindowDestroyed()
{
  if (!mWidgetListener)
    return;

  nsCOMPtr<nsIXULWindow> window = mWidgetListener->GetXULWindow();
  nsCOMPtr<nsIBaseWindow> xulWindow(do_QueryInterface(window));
  if (xulWindow) {
    xulWindow->Destroy();
  }
}

void
nsBaseWidget::NotifySizeMoveDone()
{
  if (!mWidgetListener || mWidgetListener->GetXULWindow())
    return;

  nsIPresShell* presShell = mWidgetListener->GetPresShell();
  if (presShell) {
    presShell->WindowSizeMoveDone();
  }
}

void
nsBaseWidget::NotifyWindowMoved(int32_t aX, int32_t aY)
{
  if (mWidgetListener) {
    mWidgetListener->WindowMoved(this, aX, aY);
  }

  if (GetIMEUpdatePreference().WantPositionChanged()) {
    NotifyIME(IMENotification(IMEMessage::NOTIFY_IME_OF_POSITION_CHANGE));
  }
}

void
nsBaseWidget::NotifySysColorChanged()
{
  if (!mWidgetListener || mWidgetListener->GetXULWindow())
    return;

  nsIPresShell* presShell = mWidgetListener->GetPresShell();
  if (presShell) {
    presShell->SysColorChanged();
  }
}

void
nsBaseWidget::NotifyThemeChanged()
{
  if (!mWidgetListener || mWidgetListener->GetXULWindow())
    return;

  nsIPresShell* presShell = mWidgetListener->GetPresShell();
  if (presShell) {
    presShell->ThemeChanged();
  }
}

void
nsBaseWidget::NotifyUIStateChanged(UIStateChangeType aShowAccelerators,
                                   UIStateChangeType aShowFocusRings)
{
  if (nsIDocument* doc = GetDocument()) {
    nsPIDOMWindow* win = doc->GetWindow();
    if (win) {
      win->SetKeyboardIndicators(aShowAccelerators, aShowFocusRings);
    }
  }
}

NS_IMETHODIMP
nsBaseWidget::NotifyIME(const IMENotification& aIMENotification)
{
  switch (aIMENotification.mMessage) {
    case REQUEST_TO_COMMIT_COMPOSITION:
    case REQUEST_TO_CANCEL_COMPOSITION:
      
      
      
      
      if (mTextEventDispatcher && mTextEventDispatcher->IsComposing()) {
        return mTextEventDispatcher->NotifyIME(aIMENotification);
      }
      
      return NotifyIMEInternal(aIMENotification);
    case NOTIFY_IME_OF_FOCUS:
    case NOTIFY_IME_OF_BLUR:
      
      
      
      if (mTextEventDispatcher) {
        mTextEventDispatcher->NotifyIME(aIMENotification);
      }
      return NotifyIMEInternal(aIMENotification);
    default:
      
      return NotifyIMEInternal(aIMENotification);
  }
}

NS_IMETHODIMP_(nsIWidget::TextEventDispatcher*)
nsBaseWidget::GetTextEventDispatcher()
{
  if (!mTextEventDispatcher) {
    mTextEventDispatcher = new TextEventDispatcher(this);
  }
  return mTextEventDispatcher;
}

#ifdef ACCESSIBILITY

a11y::Accessible*
nsBaseWidget::GetRootAccessible()
{
  NS_ENSURE_TRUE(mWidgetListener, nullptr);

  nsIPresShell* presShell = mWidgetListener->GetPresShell();
  NS_ENSURE_TRUE(presShell, nullptr);

  
  
  nsPresContext* presContext = presShell->GetPresContext();
  NS_ENSURE_TRUE(presContext->GetContainerWeak(), nullptr);

  
  
  nsCOMPtr<nsIAccessibilityService> accService =
    services::GetAccessibilityService();
  if (accService) {
    return accService->GetRootDocumentAccessible(presShell, nsContentUtils::IsSafeToRunScript());
  }

  return nullptr;
}

#endif 

nsresult
nsIWidget::SynthesizeNativeTouchTap(nsIntPoint aPointerScreenPoint, bool aLongTap,
                                    nsIObserver* aObserver)
{
  AutoObserverNotifier notifier(aObserver, "touchtap");

  if (sPointerIdCounter > TOUCH_INJECT_MAX_POINTS) {
    sPointerIdCounter = 0;
  }
  int pointerId = sPointerIdCounter;
  sPointerIdCounter++;
  nsresult rv = SynthesizeNativeTouchPoint(pointerId, TOUCH_CONTACT,
                                           aPointerScreenPoint, 1.0, 90, nullptr);
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (!aLongTap) {
    nsresult rv = SynthesizeNativeTouchPoint(pointerId, TOUCH_REMOVE,
                                             aPointerScreenPoint, 0, 0, nullptr);
    return rv;
  }

  
  int elapse = Preferences::GetInt("ui.click_hold_context_menus.delay",
                                   TOUCH_INJECT_LONG_TAP_DEFAULT_MSEC);
  if (!mLongTapTimer) {
    mLongTapTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
    if (NS_FAILED(rv)) {
      SynthesizeNativeTouchPoint(pointerId, TOUCH_CANCEL,
                                 aPointerScreenPoint, 0, 0, nullptr);
      return NS_ERROR_UNEXPECTED;
    }
    
    
    int timeout = elapse;
    if (timeout > TOUCH_INJECT_PUMP_TIMER_MSEC) {
      timeout = TOUCH_INJECT_PUMP_TIMER_MSEC;
    }
    mLongTapTimer->InitWithFuncCallback(OnLongTapTimerCallback, this,
                                        timeout,
                                        nsITimer::TYPE_REPEATING_SLACK);
  }

  
  
  if (mLongTapTouchPoint) {
    SynthesizeNativeTouchPoint(mLongTapTouchPoint->mPointerId, TOUCH_CANCEL,
                               mLongTapTouchPoint->mPosition, 0, 0, nullptr);
  }

  mLongTapTouchPoint = new LongTapInfo(pointerId, aPointerScreenPoint,
                                       TimeDuration::FromMilliseconds(elapse),
                                       aObserver);
  notifier.SkipNotification();  
  return NS_OK;
}


void
nsIWidget::OnLongTapTimerCallback(nsITimer* aTimer, void* aClosure)
{
  nsIWidget *self = static_cast<nsIWidget *>(aClosure);

  if ((self->mLongTapTouchPoint->mStamp + self->mLongTapTouchPoint->mDuration) >
      TimeStamp::Now()) {
#ifdef XP_WIN
    
    
    self->SynthesizeNativeTouchPoint(self->mLongTapTouchPoint->mPointerId,
                                     TOUCH_CONTACT,
                                     self->mLongTapTouchPoint->mPosition,
                                     1.0, 90, nullptr);
#endif
    return;
  }

  AutoObserverNotifier notiifer(self->mLongTapTouchPoint->mObserver, "touchtap");

  
  self->mLongTapTimer->Cancel();
  self->mLongTapTimer = nullptr;
  self->SynthesizeNativeTouchPoint(self->mLongTapTouchPoint->mPointerId,
                                   TOUCH_REMOVE,
                                   self->mLongTapTouchPoint->mPosition,
                                   0, 0, nullptr);
  self->mLongTapTouchPoint = nullptr;
}

nsresult
nsIWidget::ClearNativeTouchSequence(nsIObserver* aObserver)
{
  AutoObserverNotifier notifier(aObserver, "cleartouch");

  if (!mLongTapTimer) {
    return NS_OK;
  }
  mLongTapTimer->Cancel();
  mLongTapTimer = nullptr;
  SynthesizeNativeTouchPoint(mLongTapTouchPoint->mPointerId, TOUCH_CANCEL,
                             mLongTapTouchPoint->mPosition, 0, 0, nullptr);
  mLongTapTouchPoint = nullptr;
  return NS_OK;
}

void
nsBaseWidget::RegisterPluginWindowForRemoteUpdates()
{
#if !defined(XP_WIN) && !defined(MOZ_WIDGET_GTK)
  NS_NOTREACHED("nsBaseWidget::RegisterPluginWindowForRemoteUpdates not implemented!");
  return;
#else
  MOZ_ASSERT(NS_IsMainThread());
  void* id = GetNativeData(NS_NATIVE_PLUGIN_ID);
  if (!id) {
    NS_WARNING("This is not a valid native widget!");
    return;
  }
  MOZ_ASSERT(sPluginWidgetList);
  sPluginWidgetList->Put(id, this);
#endif
}

void
nsBaseWidget::UnregisterPluginWindowForRemoteUpdates()
{
#if !defined(XP_WIN) && !defined(MOZ_WIDGET_GTK)
  NS_NOTREACHED("nsBaseWidget::UnregisterPluginWindowForRemoteUpdates not implemented!");
  return;
#else
  MOZ_ASSERT(NS_IsMainThread());
  void* id = GetNativeData(NS_NATIVE_PLUGIN_ID);
  if (!id) {
    NS_WARNING("This is not a valid native widget!");
    return;
  }
  MOZ_ASSERT(sPluginWidgetList);
  sPluginWidgetList->Remove(id);
#endif
}


nsIWidget*
nsIWidget::LookupRegisteredPluginWindow(uintptr_t aWindowID)
{
#if !defined(XP_WIN) && !defined(MOZ_WIDGET_GTK)
  NS_NOTREACHED("nsBaseWidget::LookupRegisteredPluginWindow not implemented!");
  return nullptr;
#else
  MOZ_ASSERT(NS_IsMainThread());
  nsIWidget* widget = nullptr;
  MOZ_ASSERT(sPluginWidgetList);
  sPluginWidgetList->Get((void*)aWindowID, &widget);
  return widget;
#endif
}

#if defined(XP_WIN) || defined(MOZ_WIDGET_GTK)
static PLDHashOperator
RegisteredPluginEnumerator(const void* aWindowId, nsIWidget* aWidget, void* aUserArg)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aWindowId);
  MOZ_ASSERT(aWidget);
  MOZ_ASSERT(aUserArg);
  const nsTArray<uintptr_t>* visible = static_cast<const nsTArray<uintptr_t>*>(aUserArg);
  if (!visible->Contains((uintptr_t)aWindowId) && !aWidget->Destroyed()) {
    aWidget->Show(false);
  }
  return PLDHashOperator::PL_DHASH_NEXT;
}
#endif


void
nsIWidget::UpdateRegisteredPluginWindowVisibility(nsTArray<uintptr_t>& aVisibleList)
{
#if !defined(XP_WIN) && !defined(MOZ_WIDGET_GTK)
  NS_NOTREACHED("nsBaseWidget::UpdateRegisteredPluginWindowVisibility not implemented!");
  return;
#else
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(sPluginWidgetList);
  sPluginWidgetList->EnumerateRead(RegisteredPluginEnumerator, static_cast<void*>(&aVisibleList));
#endif
}

#ifdef DEBUG










 nsAutoString
nsBaseWidget::debug_GuiEventToString(WidgetGUIEvent* aGuiEvent)
{
  NS_ASSERTION(nullptr != aGuiEvent,"cmon, null gui event.");

  nsAutoString eventName(NS_LITERAL_STRING("UNKNOWN"));

#define _ASSIGN_eventName(_value,_name)\
case _value: eventName.AssignLiteral(_name) ; break

  switch(aGuiEvent->message)
  {
    _ASSIGN_eventName(NS_BLUR_CONTENT,"NS_BLUR_CONTENT");
    _ASSIGN_eventName(NS_DRAGDROP_GESTURE,"NS_DND_GESTURE");
    _ASSIGN_eventName(NS_DRAGDROP_DROP,"NS_DND_DROP");
    _ASSIGN_eventName(NS_DRAGDROP_ENTER,"NS_DND_ENTER");
    _ASSIGN_eventName(NS_DRAGDROP_EXIT,"NS_DND_EXIT");
    _ASSIGN_eventName(NS_DRAGDROP_OVER,"NS_DND_OVER");
    _ASSIGN_eventName(NS_EDITOR_INPUT,"NS_EDITOR_INPUT");
    _ASSIGN_eventName(NS_FOCUS_CONTENT,"NS_FOCUS_CONTENT");
    _ASSIGN_eventName(NS_FORM_SELECTED,"NS_FORM_SELECTED");
    _ASSIGN_eventName(NS_FORM_CHANGE,"NS_FORM_CHANGE");
    _ASSIGN_eventName(NS_FORM_RESET,"NS_FORM_RESET");
    _ASSIGN_eventName(NS_FORM_SUBMIT,"NS_FORM_SUBMIT");
    _ASSIGN_eventName(NS_IMAGE_ABORT,"NS_IMAGE_ABORT");
    _ASSIGN_eventName(NS_LOAD_ERROR,"NS_LOAD_ERROR");
    _ASSIGN_eventName(NS_KEY_DOWN,"NS_KEY_DOWN");
    _ASSIGN_eventName(NS_KEY_PRESS,"NS_KEY_PRESS");
    _ASSIGN_eventName(NS_KEY_UP,"NS_KEY_UP");
    _ASSIGN_eventName(NS_MOUSE_ENTER,"NS_MOUSE_ENTER");
    _ASSIGN_eventName(NS_MOUSE_EXIT,"NS_MOUSE_EXIT");
    _ASSIGN_eventName(NS_MOUSE_BUTTON_DOWN,"NS_MOUSE_BUTTON_DOWN");
    _ASSIGN_eventName(NS_MOUSE_BUTTON_UP,"NS_MOUSE_BUTTON_UP");
    _ASSIGN_eventName(NS_MOUSE_CLICK,"NS_MOUSE_CLICK");
    _ASSIGN_eventName(NS_MOUSE_DOUBLECLICK,"NS_MOUSE_DBLCLICK");
    _ASSIGN_eventName(NS_MOUSE_MOVE,"NS_MOUSE_MOVE");
    _ASSIGN_eventName(NS_LOAD,"NS_LOAD");
    _ASSIGN_eventName(NS_POPSTATE,"NS_POPSTATE");
    _ASSIGN_eventName(NS_BEFORE_SCRIPT_EXECUTE,"NS_BEFORE_SCRIPT_EXECUTE");
    _ASSIGN_eventName(NS_AFTER_SCRIPT_EXECUTE,"NS_AFTER_SCRIPT_EXECUTE");
    _ASSIGN_eventName(NS_PAGE_UNLOAD,"NS_PAGE_UNLOAD");
    _ASSIGN_eventName(NS_HASHCHANGE,"NS_HASHCHANGE");
    _ASSIGN_eventName(NS_READYSTATECHANGE,"NS_READYSTATECHANGE");
    _ASSIGN_eventName(NS_XUL_BROADCAST, "NS_XUL_BROADCAST");
    _ASSIGN_eventName(NS_XUL_COMMAND_UPDATE, "NS_XUL_COMMAND_UPDATE");

#undef _ASSIGN_eventName

  default:
    {
      char buf[32];

      sprintf(buf,"UNKNOWN: %d",aGuiEvent->message);

      CopyASCIItoUTF16(buf, eventName);
    }
    break;
  }

  return nsAutoString(eventName);
}





struct PrefPair
{
  const char * name;
  bool value;
};

static PrefPair debug_PrefValues[] =
{
  { "nglayout.debug.crossing_event_dumping", false },
  { "nglayout.debug.event_dumping", false },
  { "nglayout.debug.invalidate_dumping", false },
  { "nglayout.debug.motion_event_dumping", false },
  { "nglayout.debug.paint_dumping", false },
  { "nglayout.debug.paint_flashing", false }
};


bool
nsBaseWidget::debug_GetCachedBoolPref(const char * aPrefName)
{
  NS_ASSERTION(nullptr != aPrefName,"cmon, pref name is null.");

  for (uint32_t i = 0; i < ArrayLength(debug_PrefValues); i++)
  {
    if (strcmp(debug_PrefValues[i].name, aPrefName) == 0)
    {
      return debug_PrefValues[i].value;
    }
  }

  return false;
}

static void debug_SetCachedBoolPref(const char * aPrefName,bool aValue)
{
  NS_ASSERTION(nullptr != aPrefName,"cmon, pref name is null.");

  for (uint32_t i = 0; i < ArrayLength(debug_PrefValues); i++)
  {
    if (strcmp(debug_PrefValues[i].name, aPrefName) == 0)
    {
      debug_PrefValues[i].value = aValue;

      return;
    }
  }

  NS_ASSERTION(false, "cmon, this code is not reached dude.");
}


class Debug_PrefObserver final : public nsIObserver {
    ~Debug_PrefObserver() {}

  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
};

NS_IMPL_ISUPPORTS(Debug_PrefObserver, nsIObserver)

NS_IMETHODIMP
Debug_PrefObserver::Observe(nsISupports* subject, const char* topic,
                            const char16_t* data)
{
  NS_ConvertUTF16toUTF8 prefName(data);

  bool value = Preferences::GetBool(prefName.get(), false);
  debug_SetCachedBoolPref(prefName.get(), value);
  return NS_OK;
}


 void
debug_RegisterPrefCallbacks()
{
  static bool once = true;

  if (!once) {
    return;
  }

  once = false;

  nsCOMPtr<nsIObserver> obs(new Debug_PrefObserver());
  for (uint32_t i = 0; i < ArrayLength(debug_PrefValues); i++) {
    
    debug_PrefValues[i].value =
      Preferences::GetBool(debug_PrefValues[i].name, false);

    if (obs) {
      
      Preferences::AddStrongObserver(obs, debug_PrefValues[i].name);
    }
  }
}

static int32_t
_GetPrintCount()
{
  static int32_t sCount = 0;

  return ++sCount;
}

 bool
nsBaseWidget::debug_WantPaintFlashing()
{
  return debug_GetCachedBoolPref("nglayout.debug.paint_flashing");
}

 void
nsBaseWidget::debug_DumpEvent(FILE *                aFileOut,
                              nsIWidget *           aWidget,
                              WidgetGUIEvent*       aGuiEvent,
                              const nsAutoCString & aWidgetName,
                              int32_t               aWindowID)
{
  if (aGuiEvent->message == NS_MOUSE_MOVE)
  {
    if (!debug_GetCachedBoolPref("nglayout.debug.motion_event_dumping"))
      return;
  }

  if (aGuiEvent->message == NS_MOUSE_ENTER ||
      aGuiEvent->message == NS_MOUSE_EXIT)
  {
    if (!debug_GetCachedBoolPref("nglayout.debug.crossing_event_dumping"))
      return;
  }

  if (!debug_GetCachedBoolPref("nglayout.debug.event_dumping"))
    return;

  NS_LossyConvertUTF16toASCII tempString(debug_GuiEventToString(aGuiEvent).get());

  fprintf(aFileOut,
          "%4d %-26s widget=%-8p name=%-12s id=0x%-6x refpt=%d,%d\n",
          _GetPrintCount(),
          tempString.get(),
          (void *) aWidget,
          aWidgetName.get(),
          aWindowID,
          aGuiEvent->refPoint.x,
          aGuiEvent->refPoint.y);
}

 void
nsBaseWidget::debug_DumpPaintEvent(FILE *                aFileOut,
                                   nsIWidget *           aWidget,
                                   const nsIntRegion &   aRegion,
                                   const nsAutoCString & aWidgetName,
                                   int32_t               aWindowID)
{
  NS_ASSERTION(nullptr != aFileOut,"cmon, null output FILE");
  NS_ASSERTION(nullptr != aWidget,"cmon, the widget is null");

  if (!debug_GetCachedBoolPref("nglayout.debug.paint_dumping"))
    return;

  nsIntRect rect = aRegion.GetBounds();
  fprintf(aFileOut,
          "%4d PAINT      widget=%p name=%-12s id=0x%-6x bounds-rect=%3d,%-3d %3d,%-3d",
          _GetPrintCount(),
          (void *) aWidget,
          aWidgetName.get(),
          aWindowID,
          rect.x, rect.y, rect.width, rect.height
    );

  fprintf(aFileOut,"\n");
}

 void
nsBaseWidget::debug_DumpInvalidate(FILE *                aFileOut,
                                   nsIWidget *           aWidget,
                                   const nsIntRect *     aRect,
                                   const nsAutoCString & aWidgetName,
                                   int32_t               aWindowID)
{
  if (!debug_GetCachedBoolPref("nglayout.debug.invalidate_dumping"))
    return;

  NS_ASSERTION(nullptr != aFileOut,"cmon, null output FILE");
  NS_ASSERTION(nullptr != aWidget,"cmon, the widget is null");

  fprintf(aFileOut,
          "%4d Invalidate widget=%p name=%-12s id=0x%-6x",
          _GetPrintCount(),
          (void *) aWidget,
          aWidgetName.get(),
          aWindowID);

  if (aRect)
  {
    fprintf(aFileOut,
            " rect=%3d,%-3d %3d,%-3d",
            aRect->x,
            aRect->y,
            aRect->width,
            aRect->height);
  }
  else
  {
    fprintf(aFileOut,
            " rect=%-15s",
            "none");
  }

  fprintf(aFileOut,"\n");
}


#endif 

