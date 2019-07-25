





































#include "mozilla/Util.h"

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
#include "nsIServiceManager.h"
#include "mozilla/Preferences.h"
#include "BasicLayers.h"
#include "LayerManagerOGL.h"
#include "nsIXULRuntime.h"
#include "nsIGfxInfo.h"
#include "npapi.h"

#ifdef DEBUG
#include "nsIObserver.h"

static void debug_RegisterPrefCallbacks();

static bool debug_InSecureKeyboardInputMode = false;
#endif

#ifdef NOISY_WIDGET_LEAKS
static PRInt32 gNumWidgets;
#endif

using namespace mozilla::layers;
using namespace mozilla;

nsIContent* nsBaseWidget::mLastRollup = nsnull;


NS_IMPL_ISUPPORTS1(nsBaseWidget, nsIWidget)


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







nsBaseWidget::nsBaseWidget()
: mClientData(nsnull)
, mViewWrapperPtr(nsnull)
, mEventCallback(nsnull)
, mViewCallback(nsnull)
, mContext(nsnull)
, mCursor(eCursor_standard)
, mWindowType(eWindowType_child)
, mBorderStyle(eBorderStyle_none)
, mOnDestroyCalled(false)
, mUseAcceleratedRendering(false)
, mTemporarilyUseBasicLayerManager(false)
, mBounds(0,0,0,0)
, mOriginalBounds(nsnull)
, mClipRectCount(0)
, mZIndex(0)
, mSizeMode(nsSizeMode_Normal)
, mPopupLevel(ePopupLevelTop)
{
#ifdef NOISY_WIDGET_LEAKS
  gNumWidgets++;
  printf("WIDGETS+ = %d\n", gNumWidgets);
#endif

#ifdef DEBUG
    debug_RegisterPrefCallbacks();
#endif
}







nsBaseWidget::~nsBaseWidget()
{
  if (mLayerManager &&
      mLayerManager->GetBackendType() == LayerManager::LAYERS_BASIC) {
    static_cast<BasicLayerManager*>(mLayerManager.get())->ClearRetainerWidget();
  }

  if (mLayerManager) {
    mLayerManager->Destroy();
  }

#ifdef NOISY_WIDGET_LEAKS
  gNumWidgets--;
  printf("WIDGETS- = %d\n", gNumWidgets);
#endif

  NS_IF_RELEASE(mContext);
  delete mOriginalBounds;
}







void nsBaseWidget::BaseCreate(nsIWidget *aParent,
                              const nsIntRect &aRect,
                              EVENT_CALLBACK aHandleEventFunction,
                              nsDeviceContext *aContext,
                              nsWidgetInitData *aInitData)
{
  
  mEventCallback = aHandleEventFunction;
  
  
  if (aContext) {
    mContext = aContext;
    NS_ADDREF(mContext);
  }
  else {
    mContext = new nsDeviceContext();
    NS_ADDREF(mContext);
    mContext->Init(nsnull);
  }

  if (nsnull != aInitData) {
    mWindowType = aInitData->mWindowType;
    mBorderStyle = aInitData->mBorderStyle;
    mPopupLevel = aInitData->mPopupLevel;
  }

  if (aParent) {
    aParent->AddChild(this);
  }
}

NS_IMETHODIMP nsBaseWidget::CaptureMouse(bool aCapture)
{
  return NS_OK;
}







NS_IMETHODIMP nsBaseWidget::GetClientData(void*& aClientData)
{
  aClientData = mClientData;
  return NS_OK;
}

NS_IMETHODIMP nsBaseWidget::SetClientData(void* aClientData)
{
  mClientData = aClientData;
  return NS_OK;
}

already_AddRefed<nsIWidget>
nsBaseWidget::CreateChild(const nsIntRect  &aRect,
                          EVENT_CALLBACK   aHandleEventFunction,
                          nsDeviceContext *aContext,
                          nsWidgetInitData *aInitData,
                          bool             aForceUseIWidgetParent)
{
  nsIWidget* parent = this;
  nsNativeWidget nativeParent = nsnull;

  if (!aForceUseIWidgetParent) {
    
    
    
    nativeParent = parent ? parent->GetNativeData(NS_NATIVE_WIDGET) : nsnull;
    parent = nativeParent ? nsnull : parent;
    NS_ABORT_IF_FALSE(!parent || !nativeParent, "messed up logic");
  }

  nsCOMPtr<nsIWidget> widget;
  if (aInitData && aInitData->mWindowType == eWindowType_popup) {
    widget = AllocateChildPopupWidget();
  } else {
    static NS_DEFINE_IID(kCChildCID, NS_CHILD_CID);
    widget = do_CreateInstance(kCChildCID);
  }

  if (widget &&
      NS_SUCCEEDED(widget->Create(parent, nativeParent, aRect,
                                  aHandleEventFunction,
                                  aContext, aInitData))) {
    return widget.forget();
  }

  return nsnull;
}

NS_IMETHODIMP
nsBaseWidget::SetEventCallback(EVENT_CALLBACK aEventFunction,
                               nsDeviceContext *aContext)
{
  mEventCallback = aEventFunction;

  if (aContext) {
    NS_IF_RELEASE(mContext);
    mContext = aContext;
    NS_ADDREF(mContext);
  }

  return NS_OK;
}


NS_IMETHODIMP
nsBaseWidget::AttachViewToTopLevel(EVENT_CALLBACK aViewEventFunction,
                                   nsDeviceContext *aContext)
{
  NS_ASSERTION((mWindowType == eWindowType_toplevel ||
                mWindowType == eWindowType_dialog ||
                mWindowType == eWindowType_invisible ||
                mWindowType == eWindowType_child),
               "Can't attach to window of that type");

  mViewCallback = aViewEventFunction;

  if (aContext) {
    if (mContext) {
      NS_IF_RELEASE(mContext);
    }
    mContext = aContext;
    NS_ADDREF(mContext);
  }

  return NS_OK;
}

ViewWrapper* nsBaseWidget::GetAttachedViewPtr()
 {
   return mViewWrapperPtr;
 }
 
NS_IMETHODIMP nsBaseWidget::SetAttachedViewPtr(ViewWrapper* aViewWrapper)
 {
   mViewWrapperPtr = aViewWrapper;
   return NS_OK;
 }

NS_METHOD nsBaseWidget::ResizeClient(PRInt32 aX,
                                     PRInt32 aY,
                                     PRInt32 aWidth,
                                     PRInt32 aHeight,
                                     bool aRepaint)
{
  return Resize(aX, aY, aWidth, aHeight, aRepaint);
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
  return nsnull;
}






nsIWidget* nsBaseWidget::GetTopLevelWidget()
{
  nsIWidget *topLevelWidget = nsnull, *widget = this;
  while (widget) {
    topLevelWidget = widget;
    widget = widget->GetParent();
  }
  return topLevelWidget;
}






nsIWidget* nsBaseWidget::GetSheetWindowParent(void)
{
  return nsnull;
}

float nsBaseWidget::GetDPI()
{
  return 96.0f;
}

double nsBaseWidget::GetDefaultScale()
{
  return 1.0;
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
  NS_ASSERTION(aChild->GetParent() == this, "Not one of our kids!");
  
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
  
  aChild->SetNextSibling(nsnull);
  aChild->SetPrevSibling(nsnull);
}







NS_IMETHODIMP nsBaseWidget::SetZIndex(PRInt32 aZIndex)
{
  
  
  nsCOMPtr<nsIWidget> kungFuDeathGrip(this);
  
  mZIndex = aZIndex;

  
  nsBaseWidget* parent = static_cast<nsBaseWidget*>(GetParent());
  if (parent) {
    parent->RemoveChild(this);
    
    nsIWidget* sib = parent->GetFirstChild();
    for ( ; sib; sib = sib->GetNextSibling()) {
      PRInt32 childZIndex;
      if (NS_SUCCEEDED(sib->GetZIndex(&childZIndex))) {
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
    }
    
    if (!sib) {
      parent->AddChild(this);
    }
  }
  return NS_OK;
}






NS_IMETHODIMP nsBaseWidget::GetZIndex(PRInt32* aZIndex)
{
  *aZIndex = mZIndex;
  return NS_OK;
}






NS_IMETHODIMP nsBaseWidget::PlaceBehind(nsTopLevelWidgetZPlacement aPlacement,
                                        nsIWidget *aWidget, bool aActivate)
{
  return NS_OK;
}







NS_IMETHODIMP nsBaseWidget::SetSizeMode(PRInt32 aMode)
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






NS_IMETHODIMP nsBaseWidget::GetSizeMode(PRInt32* aMode)
{
  *aMode = mSizeMode;
  return NS_OK;
}






nscolor nsBaseWidget::GetForegroundColor(void)
{
  return mForeground;
}

    





NS_METHOD nsBaseWidget::SetForegroundColor(const nscolor &aColor)
{
  mForeground = aColor;
  return NS_OK;
}

    





nscolor nsBaseWidget::GetBackgroundColor(void)
{
  return mBackground;
}






NS_METHOD nsBaseWidget::SetBackgroundColor(const nscolor &aColor)
{
  mBackground = aColor;
  return NS_OK;
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
                                      PRUint32 aHotspotX, PRUint32 aHotspotY)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
    





NS_IMETHODIMP nsBaseWidget::GetWindowType(nsWindowType& aWindowType)
{
  aWindowType = mWindowType;
  return NS_OK;
}







void nsBaseWidget::SetTransparencyMode(nsTransparencyMode aMode) {
}

nsTransparencyMode nsBaseWidget::GetTransparencyMode() {
  return eTransparencyOpaque;
}

bool
nsBaseWidget::StoreWindowClipRegion(const nsTArray<nsIntRect>& aRects)
{
  if (mClipRects && mClipRectCount == aRects.Length() &&
      memcmp(mClipRects, aRects.Elements(), sizeof(nsIntRect)*mClipRectCount) == 0)
    return false;

  mClipRectCount = aRects.Length();
  mClipRects = new nsIntRect[mClipRectCount];
  if (mClipRects) {
    memcpy(mClipRects, aRects.Elements(), sizeof(nsIntRect)*mClipRectCount);
  }
  return true;
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







NS_IMETHODIMP nsBaseWidget::SetWindowShadowStyle(PRInt32 aMode)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}






NS_IMETHODIMP nsBaseWidget::HideWindowChrome(bool aShouldHide)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}






NS_IMETHODIMP nsBaseWidget::MakeFullScreen(bool aFullScreen)
{
  HideWindowChrome(aFullScreen);

  if (aFullScreen) {
    if (!mOriginalBounds)
      mOriginalBounds = new nsIntRect();
    GetScreenBounds(*mOriginalBounds);

    
    nsCOMPtr<nsIScreenManager> screenManager;
    screenManager = do_GetService("@mozilla.org/gfx/screenmanager;1"); 
    NS_ASSERTION(screenManager, "Unable to grab screenManager.");
    if (screenManager) {
      nsCOMPtr<nsIScreen> screen;
      screenManager->ScreenForRect(mOriginalBounds->x, mOriginalBounds->y,
                                   mOriginalBounds->width, mOriginalBounds->height,
                                   getter_AddRefs(screen));
      if (screen) {
        PRInt32 left, top, width, height;
        if (NS_SUCCEEDED(screen->GetRect(&left, &top, &width, &height))) {
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
    BasicLayerManager::BufferMode aDoubleBuffering)
  : mWidget(aWidget)
{
  BasicLayerManager* manager =
    static_cast<BasicLayerManager*>(mWidget->GetLayerManager());
  if (manager) {
    NS_ASSERTION(manager->GetBackendType() == LayerManager::LAYERS_BASIC,
      "AutoLayerManagerSetup instantiated for non-basic layer backend!");
    manager->SetDefaultTarget(aTarget, aDoubleBuffering);
  }
}

nsBaseWidget::AutoLayerManagerSetup::~AutoLayerManagerSetup()
{
  BasicLayerManager* manager =
    static_cast<BasicLayerManager*>(mWidget->GetLayerManager());
  if (manager) {
    NS_ASSERTION(manager->GetBackendType() == LayerManager::LAYERS_BASIC,
      "AutoLayerManagerSetup instantiated for non-basic layer backend!");
    manager->SetDefaultTarget(nsnull, BasicLayerManager::BUFFER_NONE);
  }
}

nsBaseWidget::AutoUseBasicLayerManager::AutoUseBasicLayerManager(nsBaseWidget* aWidget)
  : mWidget(aWidget)
{
  mWidget->mTemporarilyUseBasicLayerManager = true;
}

nsBaseWidget::AutoUseBasicLayerManager::~AutoUseBasicLayerManager()
{
  mWidget->mTemporarilyUseBasicLayerManager = false;
}

bool
nsBaseWidget::GetShouldAccelerate()
{
#if defined(XP_WIN) || defined(ANDROID) || (MOZ_PLATFORM_MAEMO > 5)
  bool accelerateByDefault = true;
#elif defined(XP_MACOSX)


# if defined(NP_NO_QUICKDRAW)
  bool accelerateByDefault = true;

  
  
  
  
  
  SInt32 major, minor, bugfix;
  OSErr err1 = ::Gestalt(gestaltSystemVersionMajor, &major);
  OSErr err2 = ::Gestalt(gestaltSystemVersionMinor, &minor);
  OSErr err3 = ::Gestalt(gestaltSystemVersionBugFix, &bugfix);
  if (err1 == noErr && err2 == noErr && err3 == noErr) {
    if (major == 10 && minor == 6) {
      if (bugfix <= 2) {
        accelerateByDefault = false;
      }
    }
  }

# else
  bool accelerateByDefault = false;
# endif

#else
  bool accelerateByDefault = false;
#endif

  
  bool disableAcceleration =
    Preferences::GetBool("layers.acceleration.disabled", false);
  bool forceAcceleration =
    Preferences::GetBool("layers.acceleration.force-enabled", false);

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

    PRInt32 status;
    if (NS_SUCCEEDED(gfxInfo->GetFeatureStatus(nsIGfxInfo::FEATURE_OPENGL_LAYERS, &status))) {
      if (status == nsIGfxInfo::FEATURE_NO_INFO) {
        whitelisted = true;
      }
    }
  }

  if (disableAcceleration || safeMode)
    return false;

  if (forceAcceleration)
    return true;
  
  if (!whitelisted) {
    NS_WARNING("OpenGL-accelerated layers are not supported on this system.");
    return false;
  }

  if (accelerateByDefault)
    return true;

  
  return mUseAcceleratedRendering;
}

LayerManager* nsBaseWidget::GetLayerManager(PLayersChild* aShadowManager,
                                            LayersBackend aBackendHint,
                                            LayerManagerPersistence aPersistence,
                                            bool* aAllowRetaining)
{
  if (!mLayerManager) {

    mUseAcceleratedRendering = GetShouldAccelerate();

    if (mUseAcceleratedRendering) {
      nsRefPtr<LayerManagerOGL> layerManager = new LayerManagerOGL(this);
      






      if (layerManager->Initialize()) {
        mLayerManager = layerManager;
      }
    }
    if (!mLayerManager) {
      mBasicLayerManager = mLayerManager = CreateBasicLayerManager();
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

BasicLayerManager* nsBaseWidget::CreateBasicLayerManager()
{
      return new BasicShadowLayerManager(this);
}






nsDeviceContext* nsBaseWidget::GetDeviceContext() 
{
  return mContext; 
}






gfxASurface *nsBaseWidget::GetThebesSurface()
{
  
  
  return nsnull;
}







void nsBaseWidget::OnDestroy()
{
  
  NS_IF_RELEASE(mContext);
}

NS_METHOD nsBaseWidget::SetWindowClass(const nsAString& xulWinType)
{
  return NS_ERROR_NOT_IMPLEMENTED;
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

nsIntPoint nsBaseWidget::GetClientOffset()
{
  return nsIntPoint(0, 0);
}

NS_METHOD nsBaseWidget::SetBounds(const nsIntRect &aRect)
{
  mBounds = aRect;

  return NS_OK;
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

NS_METHOD nsBaseWidget::SetModal(bool aModal)
{
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsBaseWidget::GetAttention(PRInt32 aCycleCount) {
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
nsBaseWidget::BeginSecureKeyboardInput()
{
#ifdef DEBUG
  NS_ASSERTION(!debug_InSecureKeyboardInputMode, "Attempting to nest call to BeginSecureKeyboardInput!");
  debug_InSecureKeyboardInputMode = true;
#endif
  return NS_OK;
}

NS_IMETHODIMP
nsBaseWidget::EndSecureKeyboardInput()
{
#ifdef DEBUG
  NS_ASSERTION(debug_InSecureKeyboardInputMode, "Calling EndSecureKeyboardInput when it hasn't been enabled!");
  debug_InSecureKeyboardInputMode = false;
#endif
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
nsBaseWidget::SetAcceleratedRendering(bool aEnabled)
{
  if (mUseAcceleratedRendering == aEnabled) {
    return NS_OK;
  }
  mUseAcceleratedRendering = aEnabled;
  if (mLayerManager) {
    mLayerManager->Destroy();
  }
  mLayerManager = NULL;
  return NS_OK;
}

bool
nsBaseWidget::GetAcceleratedRendering()
{
  return mUseAcceleratedRendering;
}

NS_METHOD nsBaseWidget::RegisterTouchWindow()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_METHOD nsBaseWidget::UnregisterTouchWindow()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsBaseWidget::OverrideSystemMouseScrollSpeed(PRInt32 aOriginalDelta,
                                             bool aIsHorizontal,
                                             PRInt32 &aOverriddenDelta)
{
  aOverriddenDelta = aOriginalDelta;

  const char* kPrefNameOverrideEnabled =
    "mousewheel.system_scroll_override_on_root_content.enabled";
  bool isOverrideEnabled =
    Preferences::GetBool(kPrefNameOverrideEnabled, false);
  if (!isOverrideEnabled) {
    return NS_OK;
  }

  nsCAutoString factorPrefName(
    "mousewheel.system_scroll_override_on_root_content.");
  if (aIsHorizontal) {
    factorPrefName.AppendLiteral("horizontal.");
  } else {
    factorPrefName.AppendLiteral("vertical.");
  }
  factorPrefName.AppendLiteral("factor");
  PRInt32 iFactor = Preferences::GetInt(factorPrefName.get(), 0);
  
  
  if (iFactor <= 100) {
    return NS_OK;
  }
  double factor = (double)iFactor / 100;
  aOverriddenDelta = PRInt32(NS_round((double)aOriginalDelta * factor));

  return NS_OK;
}







static bool
ResolveIconNameHelper(nsILocalFile *aFile,
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
                              nsILocalFile **aResult)
{ 
  *aResult = nsnull;

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
      nsCOMPtr<nsILocalFile> file = do_QueryInterface(element);
      if (!file)
        continue;
      if (ResolveIconNameHelper(file, aIconName, aIconSuffix)) {
        NS_ADDREF(*aResult = file);
        return;
      }
    }
  }

  

  nsCOMPtr<nsILocalFile> file;
  dirSvc->Get(NS_APP_CHROME_DIR, NS_GET_IID(nsILocalFile),
              getter_AddRefs(file));
  if (file && ResolveIconNameHelper(file, aIconName, aIconSuffix))
    NS_ADDREF(*aResult = file);
}

NS_IMETHODIMP 
nsBaseWidget::BeginResizeDrag(nsGUIEvent* aEvent, PRInt32 aHorizontal, PRInt32 aVertical)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsBaseWidget::BeginMoveDrag(nsMouseEvent* aEvent)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

#ifdef DEBUG










 nsAutoString
nsBaseWidget::debug_GuiEventToString(nsGUIEvent * aGuiEvent)
{
  NS_ASSERTION(nsnull != aGuiEvent,"cmon, null gui event.");

  nsAutoString eventName(NS_LITERAL_STRING("UNKNOWN"));

#define _ASSIGN_eventName(_value,_name)\
case _value: eventName.AssignWithConversion(_name) ; break

  switch(aGuiEvent->message)
  {
    _ASSIGN_eventName(NS_BLUR_CONTENT,"NS_BLUR_CONTENT");
    _ASSIGN_eventName(NS_CREATE,"NS_CREATE");
    _ASSIGN_eventName(NS_DESTROY,"NS_DESTROY");
    _ASSIGN_eventName(NS_DRAGDROP_GESTURE,"NS_DND_GESTURE");
    _ASSIGN_eventName(NS_DRAGDROP_DROP,"NS_DND_DROP");
    _ASSIGN_eventName(NS_DRAGDROP_ENTER,"NS_DND_ENTER");
    _ASSIGN_eventName(NS_DRAGDROP_EXIT,"NS_DND_EXIT");
    _ASSIGN_eventName(NS_DRAGDROP_OVER,"NS_DND_OVER");
    _ASSIGN_eventName(NS_FOCUS_CONTENT,"NS_FOCUS_CONTENT");
    _ASSIGN_eventName(NS_FORM_SELECTED,"NS_FORM_SELECTED");
    _ASSIGN_eventName(NS_FORM_CHANGE,"NS_FORM_CHANGE");
    _ASSIGN_eventName(NS_FORM_INPUT,"NS_FORM_INPUT");
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
    _ASSIGN_eventName(NS_MOVE,"NS_MOVE");
    _ASSIGN_eventName(NS_LOAD,"NS_LOAD");
    _ASSIGN_eventName(NS_POPSTATE,"NS_POPSTATE");
    _ASSIGN_eventName(NS_BEFORE_SCRIPT_EXECUTE,"NS_BEFORE_SCRIPT_EXECUTE");
    _ASSIGN_eventName(NS_AFTER_SCRIPT_EXECUTE,"NS_AFTER_SCRIPT_EXECUTE");
    _ASSIGN_eventName(NS_PAGE_UNLOAD,"NS_PAGE_UNLOAD");
    _ASSIGN_eventName(NS_HASHCHANGE,"NS_HASHCHANGE");
    _ASSIGN_eventName(NS_READYSTATECHANGE,"NS_READYSTATECHANGE");
    _ASSIGN_eventName(NS_PAINT,"NS_PAINT");
    _ASSIGN_eventName(NS_XUL_BROADCAST, "NS_XUL_BROADCAST");
    _ASSIGN_eventName(NS_XUL_COMMAND_UPDATE, "NS_XUL_COMMAND_UPDATE");
    _ASSIGN_eventName(NS_SCROLLBAR_LINE_NEXT,"NS_SB_LINE_NEXT");
    _ASSIGN_eventName(NS_SCROLLBAR_LINE_PREV,"NS_SB_LINE_PREV");
    _ASSIGN_eventName(NS_SCROLLBAR_PAGE_NEXT,"NS_SB_PAGE_NEXT");
    _ASSIGN_eventName(NS_SCROLLBAR_PAGE_PREV,"NS_SB_PAGE_PREV");
    _ASSIGN_eventName(NS_SCROLLBAR_POS,"NS_SB_POS");
    _ASSIGN_eventName(NS_SIZE,"NS_SIZE");

#undef _ASSIGN_eventName

  default: 
    {
      char buf[32];
      
      sprintf(buf,"UNKNOWN: %d",aGuiEvent->message);
      
      eventName.AssignWithConversion(buf);
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
  NS_ASSERTION(nsnull != aPrefName,"cmon, pref name is null.");

  for (PRUint32 i = 0; i < ArrayLength(debug_PrefValues); i++)
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
  NS_ASSERTION(nsnull != aPrefName,"cmon, pref name is null.");

  for (PRUint32 i = 0; i < ArrayLength(debug_PrefValues); i++)
  {
    if (strcmp(debug_PrefValues[i].name, aPrefName) == 0)
    {
      debug_PrefValues[i].value = aValue;

      return;
    }
  }

  NS_ASSERTION(false, "cmon, this code is not reached dude.");
}


class Debug_PrefObserver : public nsIObserver {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
};

NS_IMPL_ISUPPORTS1(Debug_PrefObserver, nsIObserver)

NS_IMETHODIMP
Debug_PrefObserver::Observe(nsISupports* subject, const char* topic,
                            const PRUnichar* data)
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
  for (PRUint32 i = 0; i < ArrayLength(debug_PrefValues); i++) {
    
    debug_PrefValues[i].value =
      Preferences::GetBool(debug_PrefValues[i].name, false);

    if (obs) {
      
      Preferences::AddStrongObserver(obs, debug_PrefValues[i].name);
    }
  }
}

static PRInt32
_GetPrintCount()
{
  static PRInt32 sCount = 0;
  
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
                              nsGUIEvent *          aGuiEvent,
                              const nsCAutoString & aWidgetName,
                              PRInt32               aWindowID)
{
  
  if (aGuiEvent->message == NS_PAINT)
    return;

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

  nsCAutoString tempString; tempString.AssignWithConversion(debug_GuiEventToString(aGuiEvent).get());
  
  fprintf(aFileOut,
          "%4d %-26s widget=%-8p name=%-12s id=%-8p refpt=%d,%d\n",
          _GetPrintCount(),
          tempString.get(),
          (void *) aWidget,
          aWidgetName.get(),
          (void *) (aWindowID ? aWindowID : 0x0),
          aGuiEvent->refPoint.x,
          aGuiEvent->refPoint.y);
}

 void
nsBaseWidget::debug_DumpPaintEvent(FILE *                aFileOut,
                                   nsIWidget *           aWidget,
                                   nsPaintEvent *        aPaintEvent,
                                   const nsCAutoString & aWidgetName,
                                   PRInt32               aWindowID)
{
  NS_ASSERTION(nsnull != aFileOut,"cmon, null output FILE");
  NS_ASSERTION(nsnull != aWidget,"cmon, the widget is null");
  NS_ASSERTION(nsnull != aPaintEvent,"cmon, the paint event is null");

  if (!debug_GetCachedBoolPref("nglayout.debug.paint_dumping"))
    return;
  
  nsIntRect rect = aPaintEvent->region.GetBounds();
  fprintf(aFileOut,
          "%4d PAINT      widget=%p name=%-12s id=%-8p bounds-rect=%3d,%-3d %3d,%-3d", 
          _GetPrintCount(),
          (void *) aWidget,
          aWidgetName.get(),
          (void *) aWindowID,
          rect.x, rect.y, rect.width, rect.height
    );
  
  fprintf(aFileOut,"\n");
}

 void
nsBaseWidget::debug_DumpInvalidate(FILE *                aFileOut,
                                   nsIWidget *           aWidget,
                                   const nsIntRect *     aRect,
                                   bool                  aIsSynchronous,
                                   const nsCAutoString & aWidgetName,
                                   PRInt32               aWindowID)
{
  if (!debug_GetCachedBoolPref("nglayout.debug.invalidate_dumping"))
    return;

  NS_ASSERTION(nsnull != aFileOut,"cmon, null output FILE");
  NS_ASSERTION(nsnull != aWidget,"cmon, the widget is null");

  fprintf(aFileOut,
          "%4d Invalidate widget=%p name=%-12s id=%-8p",
          _GetPrintCount(),
          (void *) aWidget,
          aWidgetName.get(),
          (void *) aWindowID);

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

  fprintf(aFileOut,
          " sync=%s",
          (const char *) (aIsSynchronous ? "yes" : "no "));
  
  fprintf(aFileOut,"\n");
}


#endif 

