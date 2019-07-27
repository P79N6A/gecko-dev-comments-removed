





#ifdef MOZ_X11
#include <cairo-xlib.h>
#include "gfxXlibSurface.h"

enum { XKeyPress = KeyPress };
#include "mozilla/X11Util.h"
using mozilla::DefaultXDisplay;
#endif

#include "nsPluginInstanceOwner.h"

#include "gfxUtils.h"
#include "nsIRunnable.h"
#include "nsContentUtils.h"
#include "nsRect.h"
#include "nsSize.h"
#include "nsDisplayList.h"
#include "ImageLayers.h"
#include "GLImages.h"
#include "nsPluginFrame.h"
#include "nsIPluginDocument.h"
#include "nsIStringStream.h"
#include "nsNetUtil.h"
#include "mozilla/Preferences.h"
#include "nsILinkHandler.h"
#include "nsIDocShellTreeItem.h"
#include "nsIWebBrowserChrome.h"
#include "nsLayoutUtils.h"
#include "nsIPluginWidget.h"
#include "nsViewManager.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIDOMHTMLObjectElement.h"
#include "nsIAppShell.h"
#include "nsIDOMHTMLAppletElement.h"
#include "nsIObjectLoadingContent.h"
#include "nsObjectLoadingContent.h"
#include "nsAttrName.h"
#include "nsIFocusManager.h"
#include "nsFocusManager.h"
#include "nsIDOMDragEvent.h"
#include "nsIScrollableFrame.h"
#include "nsIDocShell.h"
#include "ImageContainer.h"
#include "nsIDOMHTMLCollection.h"
#include "GLContext.h"
#include "EGLUtils.h"
#include "nsIContentInlines.h"
#include "mozilla/MiscEvents.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/TextEvents.h"
#include "mozilla/dom/HTMLObjectElementBinding.h"
#include "mozilla/dom/TabChild.h"
#include "nsFrameSelection.h"
#include "PuppetWidget.h"
#include "nsPIWindowRoot.h"

#include "nsContentCID.h"
#include "nsWidgetsCID.h"
static NS_DEFINE_CID(kWidgetCID, NS_CHILD_CID);
static NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);

#ifdef XP_WIN
#include <wtypes.h>
#include <winuser.h>
#endif

#ifdef XP_MACOSX
#include "ComplexTextInputPanel.h"
#include "nsIDOMXULDocument.h"
#include "nsIDOMXULCommandDispatcher.h"
#endif

#ifdef MOZ_WIDGET_GTK
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#endif

#ifdef MOZ_WIDGET_ANDROID
#include "ANPBase.h"
#include "AndroidBridge.h"
#include "nsWindow.h"

static nsPluginInstanceOwner* sFullScreenInstance = nullptr;

using namespace mozilla::dom;

#include <android/log.h>
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "GeckoPlugins" , ## args)
#endif

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::layers;

static inline nsPoint AsNsPoint(const nsIntPoint &p) {
  return nsPoint(p.x, p.y);
}




class nsPluginDOMContextMenuListener : public nsIDOMEventListener
{
  virtual ~nsPluginDOMContextMenuListener();

public:
  explicit nsPluginDOMContextMenuListener(nsIContent* aContent);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMEVENTLISTENER

  void Destroy(nsIContent* aContent);

  nsEventStatus ProcessEvent(const WidgetGUIEvent& anEvent)
  {
    return nsEventStatus_eConsumeNoDefault;
  }
};

class AsyncPaintWaitEvent : public nsRunnable
{
public:
  AsyncPaintWaitEvent(nsIContent* aContent, bool aFinished) :
    mContent(aContent), mFinished(aFinished)
  {
  }

  NS_IMETHOD Run()
  {
    nsContentUtils::DispatchTrustedEvent(mContent->OwnerDoc(), mContent,
        mFinished ? NS_LITERAL_STRING("MozPaintWaitFinished") : NS_LITERAL_STRING("MozPaintWait"),
        true, true);
    return NS_OK;
  }

private:
  nsCOMPtr<nsIContent> mContent;
  bool                 mFinished;
};

void
nsPluginInstanceOwner::NotifyPaintWaiter(nsDisplayListBuilder* aBuilder)
{
  
  if (!mWaitingForPaint && !IsUpToDate() && aBuilder->ShouldSyncDecodeImages()) {
    nsCOMPtr<nsIRunnable> event = new AsyncPaintWaitEvent(mContent, false);
    
    
    mWaitingForPaint = nsContentUtils::AddScriptRunner(event);
  }
}

#if MOZ_WIDGET_ANDROID
static void
AttachToContainerAsEGLImage(ImageContainer* container,
                            nsNPAPIPluginInstance* instance,
                            const LayoutDeviceRect& rect,
                            nsRefPtr<Image>* out_image)
{
  MOZ_ASSERT(out_image);
  MOZ_ASSERT(!*out_image);

  EGLImage image = instance->AsEGLImage();
  if (!image) {
    return;
  }

  nsRefPtr<Image> img = container->CreateImage(ImageFormat::EGLIMAGE);

  EGLImageImage::Data data;
  data.mImage = image;
  data.mSize = gfx::IntSize(rect.width, rect.height);
  data.mOriginPos = instance->OriginPos();

  EGLImageImage* typedImg = static_cast<EGLImageImage*>(img.get());
  typedImg->SetData(data);

  *out_image = img;
}

static void
AttachToContainerAsSurfaceTexture(ImageContainer* container,
                                  nsNPAPIPluginInstance* instance,
                                  const LayoutDeviceRect& rect,
                                  nsRefPtr<Image>* out_image)
{
  MOZ_ASSERT(out_image);
  MOZ_ASSERT(!*out_image);

  mozilla::gl::AndroidSurfaceTexture* surfTex = instance->AsSurfaceTexture();
  if (!surfTex) {
    return;
  }

  nsRefPtr<Image> img = container->CreateImage(ImageFormat::SURFACE_TEXTURE);

  SurfaceTextureImage::Data data;
  data.mSurfTex = surfTex;
  data.mSize = gfx::IntSize(rect.width, rect.height);
  data.mOriginPos = instance->OriginPos();

  SurfaceTextureImage* typedImg = static_cast<SurfaceTextureImage*>(img.get());
  typedImg->SetData(data);

  *out_image = img;
}
#endif

already_AddRefed<ImageContainer>
nsPluginInstanceOwner::GetImageContainer()
{
  if (!mInstance)
    return nullptr;

  nsRefPtr<ImageContainer> container;

#if MOZ_WIDGET_ANDROID
  
  
  if (AndroidBridge::Bridge()->GetAPIVersion() < 11)
    return nullptr;

  LayoutDeviceRect r = GetPluginRect();

  
  
  gfxSize resolution = mPluginFrame->PresContext()->PresShell()->GetCumulativeResolution();
  ScreenSize screenSize = (r * LayoutDeviceToScreenScale(resolution.width, resolution.height)).Size();
  mInstance->NotifySize(nsIntSize(screenSize.width, screenSize.height));

  container = LayerManager::CreateImageContainer();

  
  nsRefPtr<Image> img;
  AttachToContainerAsEGLImage(container, mInstance, r, &img);
  if (!img) {
    AttachToContainerAsSurfaceTexture(container, mInstance, r, &img);
  }
  MOZ_ASSERT(img);

  container->SetCurrentImageInTransaction(img);
#else
  mInstance->GetImageContainer(getter_AddRefs(container));
#endif

  return container.forget();
}

void
nsPluginInstanceOwner::SetBackgroundUnknown()
{
  if (mInstance) {
    mInstance->SetBackgroundUnknown();
  }
}

already_AddRefed<gfxContext>
nsPluginInstanceOwner::BeginUpdateBackground(const nsIntRect& aRect)
{
  nsIntRect rect = aRect;
  nsRefPtr<gfxContext> ctx;
  if (mInstance &&
      NS_SUCCEEDED(mInstance->BeginUpdateBackground(&rect, getter_AddRefs(ctx)))) {
    return ctx.forget();
  }
  return nullptr;
}

void
nsPluginInstanceOwner::EndUpdateBackground(gfxContext* aContext,
                                           const nsIntRect& aRect)
{
  nsIntRect rect = aRect;
  if (mInstance) {
    mInstance->EndUpdateBackground(aContext, &rect);
  }
}

bool
nsPluginInstanceOwner::UseAsyncRendering()
{
#ifdef XP_MACOSX
  if (mUseAsyncRendering) {
    return true;
  }
#endif

  bool isOOP;
  bool result = (mInstance &&
          NS_SUCCEEDED(mInstance->GetIsOOP(&isOOP)) && isOOP
#ifndef XP_MACOSX
          && (!mPluginWindow ||
           mPluginWindow->type == NPWindowTypeDrawable)
#endif
          );

#ifdef XP_MACOSX
  if (result) {
    mUseAsyncRendering = true;
  }
#endif

  return result;
}

nsIntSize
nsPluginInstanceOwner::GetCurrentImageSize()
{
  nsIntSize size(0,0);
  if (mInstance) {
    mInstance->GetImageSize(&size);
  }
  return size;
}

nsPluginInstanceOwner::nsPluginInstanceOwner()
{
  
  
  nsCOMPtr<nsIPluginHost> pluginHostCOM = do_GetService(MOZ_PLUGIN_HOST_CONTRACTID);
  mPluginHost = static_cast<nsPluginHost*>(pluginHostCOM.get());
  if (mPluginHost)
    mPluginHost->NewPluginNativeWindow(&mPluginWindow);
  else
    mPluginWindow = nullptr;

  mPluginFrame = nullptr;
  mContent = nullptr;
  mWidgetCreationComplete = false;
#ifdef XP_MACOSX
  memset(&mCGPluginPortCopy, 0, sizeof(NP_CGContext));
  mInCGPaintLevel = 0;
  mSentInitialTopLevelWindowEvent = false;
  mLastWindowIsActive = false;
  mLastContentFocused = false;
  mLastScaleFactor = 1.0;
  mColorProfile = nullptr;
  mShouldBlurOnActivate = false;
#endif
  mContentFocused = false;
  mWidgetVisible = true;
  mPluginWindowVisible = false;
  mPluginDocumentActiveState = true;
  mLastMouseDownButtonType = -1;

#ifdef XP_MACOSX
#ifndef NP_NO_CARBON
  
  mEventModel = NPEventModelCarbon;
#else
  mEventModel = NPEventModelCocoa;
#endif
  mUseAsyncRendering = false;
#endif

  mWaitingForPaint = false;

#ifdef MOZ_WIDGET_ANDROID
  mFullScreen = false;
  mJavaView = nullptr;
#endif
}

nsPluginInstanceOwner::~nsPluginInstanceOwner()
{
  if (mWaitingForPaint) {
    
    
    nsCOMPtr<nsIRunnable> event = new AsyncPaintWaitEvent(mContent, true);
    NS_DispatchToMainThread(event);
  }

  mPluginFrame = nullptr;

  PLUG_DeletePluginNativeWindow(mPluginWindow);
  mPluginWindow = nullptr;

#ifdef MOZ_WIDGET_ANDROID
  RemovePluginView();
#endif

  if (mInstance) {
    mInstance->SetOwner(nullptr);
  }
}

NS_IMPL_ISUPPORTS(nsPluginInstanceOwner,
                  nsIPluginInstanceOwner,
                  nsIDOMEventListener,
                  nsIPrivacyTransitionObserver,
                  nsISupportsWeakReference)

nsresult
nsPluginInstanceOwner::SetInstance(nsNPAPIPluginInstance *aInstance)
{
  NS_ASSERTION(!mInstance || !aInstance, "mInstance should only be set or unset!");

  
  
  
  if (mInstance && !aInstance) {
    mInstance->SetOwner(nullptr);

#ifdef MOZ_WIDGET_ANDROID
    RemovePluginView();
#endif
  }

  mInstance = aInstance;

  nsCOMPtr<nsIDocument> doc;
  GetDocument(getter_AddRefs(doc));
  if (doc) {
    nsCOMPtr<nsPIDOMWindow> domWindow = doc->GetWindow();
    if (domWindow) {
      nsCOMPtr<nsIDocShell> docShell = domWindow->GetDocShell();
      if (docShell)
        docShell->AddWeakPrivacyTransitionObserver(this);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP nsPluginInstanceOwner::GetWindow(NPWindow *&aWindow)
{
  NS_ASSERTION(mPluginWindow, "the plugin window object being returned is null");
  aWindow = mPluginWindow;
  return NS_OK;
}

NS_IMETHODIMP nsPluginInstanceOwner::GetMode(int32_t *aMode)
{
  nsCOMPtr<nsIDocument> doc;
  nsresult rv = GetDocument(getter_AddRefs(doc));
  nsCOMPtr<nsIPluginDocument> pDoc (do_QueryInterface(doc));

  if (pDoc) {
    *aMode = NP_FULL;
  } else {
    *aMode = NP_EMBED;
  }

  return rv;
}

void nsPluginInstanceOwner::GetAttributes(nsTArray<MozPluginParameter>& attributes)
{
  nsCOMPtr<nsIObjectLoadingContent> content = do_QueryInterface(mContent);
  nsObjectLoadingContent *loadingContent =
    static_cast<nsObjectLoadingContent*>(content.get());

  loadingContent->GetPluginAttributes(attributes);
}

NS_IMETHODIMP nsPluginInstanceOwner::GetDOMElement(nsIDOMElement* *result)
{
  return CallQueryInterface(mContent, result);
}

nsresult nsPluginInstanceOwner::GetInstance(nsNPAPIPluginInstance **aInstance)
{
  NS_ENSURE_ARG_POINTER(aInstance);

  *aInstance = mInstance;
  NS_IF_ADDREF(*aInstance);
  return NS_OK;
}

NS_IMETHODIMP nsPluginInstanceOwner::GetURL(const char *aURL,
                                            const char *aTarget,
                                            nsIInputStream *aPostStream,
                                            void *aHeadersData,
                                            uint32_t aHeadersDataLen)
{
  NS_ENSURE_TRUE(mContent, NS_ERROR_NULL_POINTER);

  if (mContent->IsEditable()) {
    return NS_OK;
  }

  nsIDocument *doc = mContent->GetCurrentDoc();
  if (!doc) {
    return NS_ERROR_FAILURE;
  }

  nsIPresShell *presShell = doc->GetShell();
  if (!presShell) {
    return NS_ERROR_FAILURE;
  }

  nsPresContext *presContext = presShell->GetPresContext();
  if (!presContext) {
    return NS_ERROR_FAILURE;
  }

  
  nsCOMPtr<nsISupports> container = presContext->GetContainerWeak();
  NS_ENSURE_TRUE(container,NS_ERROR_FAILURE);
  nsCOMPtr<nsILinkHandler> lh = do_QueryInterface(container);
  NS_ENSURE_TRUE(lh, NS_ERROR_FAILURE);

  nsAutoString  unitarget;
  unitarget.AssignASCII(aTarget); 

  nsCOMPtr<nsIURI> baseURI = GetBaseURI();

  
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aURL, baseURI);

  NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

  nsCOMPtr<nsIInputStream> headersDataStream;
  if (aPostStream && aHeadersData) {
    if (!aHeadersDataLen)
      return NS_ERROR_UNEXPECTED;

    nsCOMPtr<nsIStringInputStream> sis = do_CreateInstance("@mozilla.org/io/string-input-stream;1");
    if (!sis)
      return NS_ERROR_OUT_OF_MEMORY;

    rv = sis->SetData((char *)aHeadersData, aHeadersDataLen);
    NS_ENSURE_SUCCESS(rv, rv);
    headersDataStream = do_QueryInterface(sis);
  }

  int32_t blockPopups =
    Preferences::GetInt("privacy.popups.disable_from_plugins");
  nsAutoPopupStatePusher popupStatePusher((PopupControlState)blockPopups);

  rv = lh->OnLinkClick(mContent, uri, unitarget.get(), NullString(),
                       aPostStream, headersDataStream, true);

  return rv;
}

NS_IMETHODIMP nsPluginInstanceOwner::ShowStatus(const char *aStatusMsg)
{
  nsresult  rv = NS_ERROR_FAILURE;

  rv = this->ShowStatus(NS_ConvertUTF8toUTF16(aStatusMsg).get());

  return rv;
}

NS_IMETHODIMP nsPluginInstanceOwner::ShowStatus(const char16_t *aStatusMsg)
{
  nsresult  rv = NS_ERROR_FAILURE;

  if (!mPluginFrame) {
    return rv;
  }
  nsCOMPtr<nsIDocShellTreeItem> docShellItem = mPluginFrame->PresContext()->GetDocShell();
  if (NS_FAILED(rv) || !docShellItem) {
    return rv;
  }

  nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
  rv = docShellItem->GetTreeOwner(getter_AddRefs(treeOwner));
  if (NS_FAILED(rv) || !treeOwner) {
    return rv;
  }

  nsCOMPtr<nsIWebBrowserChrome> browserChrome(do_GetInterface(treeOwner, &rv));
  if (NS_FAILED(rv) || !browserChrome) {
    return rv;
  }
  rv = browserChrome->SetStatus(nsIWebBrowserChrome::STATUS_SCRIPT,
                                aStatusMsg);

  return rv;
}

NS_IMETHODIMP nsPluginInstanceOwner::GetDocument(nsIDocument* *aDocument)
{
  if (!aDocument)
    return NS_ERROR_NULL_POINTER;

  
  
  NS_IF_ADDREF(*aDocument = mContent->OwnerDoc());
  return NS_OK;
}

NS_IMETHODIMP nsPluginInstanceOwner::InvalidateRect(NPRect *invalidRect)
{
  
  
  if (mWaitingForPaint && (!mPluginFrame || IsUpToDate())) {
    
    
    nsCOMPtr<nsIRunnable> event = new AsyncPaintWaitEvent(mContent, true);
    NS_DispatchToMainThread(event);
    mWaitingForPaint = false;
  }

  if (!mPluginFrame || !invalidRect || !mWidgetVisible)
    return NS_ERROR_FAILURE;

#if defined(XP_MACOSX) || defined(MOZ_WIDGET_ANDROID)
  
  
  
  nsRefPtr<ImageContainer> container;
  mInstance->GetImageContainer(getter_AddRefs(container));
#endif

#ifndef XP_MACOSX
  
  
  if (mWidget) {
    mWidget->Invalidate(nsIntRect(invalidRect->left, invalidRect->top,
                                  invalidRect->right - invalidRect->left,
                                  invalidRect->bottom - invalidRect->top));
    return NS_OK;
  }
#endif
  nsIntRect rect(invalidRect->left,
                 invalidRect->top,
                 invalidRect->right - invalidRect->left,
                 invalidRect->bottom - invalidRect->top);
  
  
  
  double scaleFactor = 1.0;
  GetContentsScaleFactor(&scaleFactor);
  rect.ScaleRoundOut(scaleFactor);
  mPluginFrame->InvalidateLayer(nsDisplayItem::TYPE_PLUGIN, &rect);
  return NS_OK;
}

NS_IMETHODIMP nsPluginInstanceOwner::InvalidateRegion(NPRegion invalidRegion)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsPluginInstanceOwner::RedrawPlugin()
{
  if (mPluginFrame) {
    mPluginFrame->InvalidateLayer(nsDisplayItem::TYPE_PLUGIN);
  }
  return NS_OK;
}

NS_IMETHODIMP nsPluginInstanceOwner::GetNetscapeWindow(void *value)
{
  if (!mPluginFrame) {
    NS_WARNING("plugin owner has no owner in getting doc's window handle");
    return NS_ERROR_FAILURE;
  }

#if defined(XP_WIN)
  void** pvalue = (void**)value;
  nsViewManager* vm = mPluginFrame->PresContext()->GetPresShell()->GetViewManager();
  if (!vm)
    return NS_ERROR_FAILURE;
  
  
  

  
  

  
  

  
  
  
  

  
  
  
  if (XRE_GetProcessType() != GeckoProcessType_Content &&
      mPluginWindow && mPluginWindow->type == NPWindowTypeDrawable) {
    
    
    
    
    
    
    
    

    nsIWidget* win = mPluginFrame->GetNearestWidget();
    if (win) {
      nsView *view = nsView::GetViewFor(win);
      NS_ASSERTION(view, "No view for widget");
      nsPoint offset = view->GetOffsetTo(nullptr);

      if (offset.x || offset.y) {
        
        
        *pvalue = (void*)win->GetNativeData(NS_NATIVE_WINDOW);
        if (*pvalue)
          return NS_OK;
      }
    }
  }
  
  nsCOMPtr<nsIWidget> widget;
  vm->GetRootWidget(getter_AddRefs(widget));
  if (widget) {
    *pvalue = (void*)widget->GetNativeData(NS_NATIVE_SHAREABLE_WINDOW);
  } else {
    NS_ASSERTION(widget, "couldn't get doc's widget in getting doc's window handle");
  }

  return NS_OK;
#elif (defined(MOZ_WIDGET_GTK) || defined(MOZ_WIDGET_QT)) && defined(MOZ_X11)
  
  nsIWidget* win = mPluginFrame->GetNearestWidget();
  if (!win)
    return NS_ERROR_FAILURE;
  *static_cast<Window*>(value) = (long unsigned int)win->GetNativeData(NS_NATIVE_SHAREABLE_WINDOW);
  return NS_OK;
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif
}

NS_IMETHODIMP nsPluginInstanceOwner::SetEventModel(int32_t eventModel)
{
#ifdef XP_MACOSX
  mEventModel = static_cast<NPEventModel>(eventModel);
  return NS_OK;
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif
}



NPError nsPluginInstanceOwner::ShowNativeContextMenu(NPMenu* menu, void* event)
{
  return NPERR_GENERIC_ERROR;
}

#ifdef XP_MACOSX
NPBool nsPluginInstanceOwner::ConvertPointPuppet(PuppetWidget *widget,
                                                 nsPluginFrame* pluginFrame,
                                                 double sourceX, double sourceY,
                                                 NPCoordinateSpace sourceSpace,
                                                 double *destX, double *destY,
                                                 NPCoordinateSpace destSpace)
{
  NS_ENSURE_TRUE(widget && widget->GetOwningTabChild() && pluginFrame, false);
  
  NS_ENSURE_TRUE(destX || destY, false);

  if (sourceSpace == destSpace) {
    if (destX) {
      *destX = sourceX;
    }
    if (destY) {
      *destY = sourceY;
    }
    return true;
  }

  nsPresContext* presContext = pluginFrame->PresContext();
  double scaleFactor = double(nsPresContext::AppUnitsPerCSSPixel())/
    presContext->DeviceContext()->AppUnitsPerDevPixelAtUnitFullZoom();

  PuppetWidget *puppetWidget = static_cast<PuppetWidget*>(widget);
  PuppetWidget *rootWidget = static_cast<PuppetWidget*>(widget->GetTopLevelWidget());
  if (!rootWidget) {
    return false;
  }
  nsPoint chromeSize = AsNsPoint(rootWidget->GetChromeDimensions()) / scaleFactor;
  nsIntSize intScreenDims = rootWidget->GetScreenDimensions();
  nsSize screenDims = nsSize(intScreenDims.width / scaleFactor,
                             intScreenDims.height / scaleFactor);
  int32_t screenH = screenDims.height;
  nsPoint windowPosition = AsNsPoint(rootWidget->GetWindowPosition()) / scaleFactor;

  
  nsIntRect tabContentBounds;
  NS_ENSURE_SUCCESS(puppetWidget->GetBounds(tabContentBounds), false);
  tabContentBounds.ScaleInverseRoundOut(scaleFactor);
  int32_t windowH = tabContentBounds.height + int(chromeSize.y);

  
  nsPoint pluginPosition = AsNsPoint(pluginFrame->GetScreenRect().TopLeft());

  
  
  
  nsPoint sourcePoint(sourceX, sourceY);
  nsPoint screenPoint;
  switch (sourceSpace) {
    case NPCoordinateSpacePlugin:
      screenPoint = sourcePoint + pluginFrame->GetContentRectRelativeToSelf().TopLeft() +
        chromeSize + pluginPosition + windowPosition;
      break;
    case NPCoordinateSpaceWindow:
      screenPoint = nsPoint(sourcePoint.x, windowH-sourcePoint.y) +
        windowPosition;
      break;
    case NPCoordinateSpaceFlippedWindow:
      screenPoint = sourcePoint + windowPosition;
      break;
    case NPCoordinateSpaceScreen:
      screenPoint = nsPoint(sourcePoint.x, screenH-sourcePoint.y);
      break;
    case NPCoordinateSpaceFlippedScreen:
      screenPoint = sourcePoint;
      break;
    default:
      return false;
  }

  
  nsPoint destPoint;
  switch (destSpace) {
    case NPCoordinateSpacePlugin:
      destPoint = screenPoint - pluginFrame->GetContentRectRelativeToSelf().TopLeft() -
        chromeSize - pluginPosition - windowPosition;
      break;
    case NPCoordinateSpaceWindow:
      destPoint = screenPoint - windowPosition;
      destPoint.y = windowH - destPoint.y;
      break;
    case NPCoordinateSpaceFlippedWindow:
      destPoint = screenPoint - windowPosition;
      break;
    case NPCoordinateSpaceScreen:
      destPoint = nsPoint(screenPoint.x, screenH-screenPoint.y);
      break;
    case NPCoordinateSpaceFlippedScreen:
      destPoint = screenPoint;
      break;
    default:
      return false;
  }

  if (destX) {
    *destX = destPoint.x;
  }
  if (destY) {
    *destY = destPoint.y;
  }

  return true;
}

NPBool nsPluginInstanceOwner::ConvertPointNoPuppet(nsIWidget *widget,
                                                   nsPluginFrame* pluginFrame,
                                                   double sourceX, double sourceY,
                                                   NPCoordinateSpace sourceSpace,
                                                   double *destX, double *destY,
                                                   NPCoordinateSpace destSpace)
{
  NS_ENSURE_TRUE(widget && pluginFrame, false);
  
  NS_ENSURE_TRUE(destX || destY, false);

  if (sourceSpace == destSpace) {
    if (destX) {
      *destX = sourceX;
    }
    if (destY) {
      *destY = sourceY;
    }
    return true;
  }

  nsPresContext* presContext = pluginFrame->PresContext();
  double scaleFactor = double(nsPresContext::AppUnitsPerCSSPixel())/
    presContext->DeviceContext()->AppUnitsPerDevPixelAtUnitFullZoom();

  nsCOMPtr<nsIScreenManager> screenMgr = do_GetService("@mozilla.org/gfx/screenmanager;1");
  if (!screenMgr) {
    return false;
  }
  nsCOMPtr<nsIScreen> screen;
  screenMgr->ScreenForNativeWidget(widget->GetNativeData(NS_NATIVE_WINDOW), getter_AddRefs(screen));
  if (!screen) {
    return false;
  }

  int32_t screenX, screenY, screenWidth, screenHeight;
  screen->GetRect(&screenX, &screenY, &screenWidth, &screenHeight);
  screenHeight /= scaleFactor;

  nsIntRect windowScreenBounds;
  NS_ENSURE_SUCCESS(widget->GetScreenBounds(windowScreenBounds), false);
  windowScreenBounds.ScaleInverseRoundOut(scaleFactor);
  int32_t windowX = windowScreenBounds.x;
  int32_t windowY = windowScreenBounds.y;
  int32_t windowHeight = windowScreenBounds.height;

  nsIntRect pluginScreenRect = pluginFrame->GetScreenRect();

  double screenXGecko, screenYGecko;
  switch (sourceSpace) {
    case NPCoordinateSpacePlugin:
      screenXGecko = pluginScreenRect.x + sourceX;
      screenYGecko = pluginScreenRect.y + sourceY;
      break;
    case NPCoordinateSpaceWindow:
      screenXGecko = windowX + sourceX;
      screenYGecko = windowY + (windowHeight - sourceY);
      break;
    case NPCoordinateSpaceFlippedWindow:
      screenXGecko = windowX + sourceX;
      screenYGecko = windowY + sourceY;
      break;
    case NPCoordinateSpaceScreen:
      screenXGecko = sourceX;
      screenYGecko = screenHeight - sourceY;
      break;
    case NPCoordinateSpaceFlippedScreen:
      screenXGecko = sourceX;
      screenYGecko = sourceY;
      break;
    default:
      return false;
  }

  double destXCocoa, destYCocoa;
  switch (destSpace) {
    case NPCoordinateSpacePlugin:
      destXCocoa = screenXGecko - pluginScreenRect.x;
      destYCocoa = screenYGecko - pluginScreenRect.y;
      break;
    case NPCoordinateSpaceWindow:
      destXCocoa = screenXGecko - windowX;
      destYCocoa = windowHeight - (screenYGecko - windowY);
      break;
    case NPCoordinateSpaceFlippedWindow:
      destXCocoa = screenXGecko - windowX;
      destYCocoa = screenYGecko - windowY;
      break;
    case NPCoordinateSpaceScreen:
      destXCocoa = screenXGecko;
      destYCocoa = screenHeight - screenYGecko;
      break;
    case NPCoordinateSpaceFlippedScreen:
      destXCocoa = screenXGecko;
      destYCocoa = screenYGecko;
      break;
    default:
      return false;
  }

  if (destX) {
    *destX = destXCocoa;
  }
  if (destY) {
    *destY = destYCocoa;
  }

  return true;
}
#endif 

NPBool nsPluginInstanceOwner::ConvertPoint(double sourceX, double sourceY, NPCoordinateSpace sourceSpace,
                                           double *destX, double *destY, NPCoordinateSpace destSpace)
{
#ifdef XP_MACOSX
  if (!mPluginFrame) {
    return false;
  }

  MOZ_ASSERT(mPluginFrame->GetNearestWidget());

  if (nsIWidget::UsePuppetWidgets()) {
    return ConvertPointPuppet(static_cast<PuppetWidget*>(mPluginFrame->GetNearestWidget()),
                               mPluginFrame, sourceX, sourceY, sourceSpace,
                               destX, destY, destSpace);
  }

  return ConvertPointNoPuppet(mPluginFrame->GetNearestWidget(),
                              mPluginFrame, sourceX, sourceY, sourceSpace,
                              destX, destY, destSpace);
#else
  return false;
#endif
}

NS_IMETHODIMP nsPluginInstanceOwner::GetTagType(nsPluginTagType *result)
{
  NS_ENSURE_ARG_POINTER(result);

  *result = nsPluginTagType_Unknown;

  nsIAtom *atom = mContent->Tag();

  if (atom == nsGkAtoms::applet)
    *result = nsPluginTagType_Applet;
  else if (atom == nsGkAtoms::embed)
    *result = nsPluginTagType_Embed;
  else if (atom == nsGkAtoms::object)
    *result = nsPluginTagType_Object;

  return NS_OK;
}

void nsPluginInstanceOwner::GetParameters(nsTArray<MozPluginParameter>& parameters)
{
  nsCOMPtr<nsIObjectLoadingContent> content = do_QueryInterface(mContent);
  nsObjectLoadingContent *loadingContent =
    static_cast<nsObjectLoadingContent*>(content.get());

  loadingContent->GetPluginParameters(parameters);
}

#ifdef XP_MACOSX

static void InitializeNPCocoaEvent(NPCocoaEvent* event)
{
  memset(event, 0, sizeof(NPCocoaEvent));
}

NPDrawingModel nsPluginInstanceOwner::GetDrawingModel()
{
#ifndef NP_NO_QUICKDRAW
  
  
  NPDrawingModel drawingModel = NPDrawingModelQuickDraw;
#else
  NPDrawingModel drawingModel = NPDrawingModelCoreGraphics;
#endif

  if (!mInstance)
    return drawingModel;

  mInstance->GetDrawingModel((int32_t*)&drawingModel);
  return drawingModel;
}

bool nsPluginInstanceOwner::IsRemoteDrawingCoreAnimation()
{
  if (!mInstance)
    return false;

  bool coreAnimation;
  if (!NS_SUCCEEDED(mInstance->IsRemoteDrawingCoreAnimation(&coreAnimation)))
    return false;

  return coreAnimation;
}

nsresult nsPluginInstanceOwner::ContentsScaleFactorChanged(double aContentsScaleFactor)
{
  if (!mInstance) {
    return NS_ERROR_NULL_POINTER;
  }
  return mInstance->ContentsScaleFactorChanged(aContentsScaleFactor);
}

NPEventModel nsPluginInstanceOwner::GetEventModel()
{
  return mEventModel;
}

#define DEFAULT_REFRESH_RATE 20 // 50 FPS

nsCOMPtr<nsITimer>               *nsPluginInstanceOwner::sCATimer = nullptr;
nsTArray<nsPluginInstanceOwner*> *nsPluginInstanceOwner::sCARefreshListeners = nullptr;

void nsPluginInstanceOwner::CARefresh(nsITimer *aTimer, void *aClosure) {
  if (!sCARefreshListeners) {
    return;
  }
  for (size_t i = 0; i < sCARefreshListeners->Length(); i++) {
    nsPluginInstanceOwner* instanceOwner = (*sCARefreshListeners)[i];
    NPWindow *window;
    instanceOwner->GetWindow(window);
    if (!window) {
      continue;
    }
    NPRect r;
    r.left = 0;
    r.top = 0;
    r.right = window->width;
    r.bottom = window->height;
    instanceOwner->InvalidateRect(&r);
  }
}

void nsPluginInstanceOwner::AddToCARefreshTimer() {
  if (!mInstance) {
    return;
  }

  
  const char* mime = nullptr;
  if (NS_SUCCEEDED(mInstance->GetMIMEType(&mime)) && mime) {
    if (strcmp(mime, "application/x-shockwave-flash") == 0) {
      return;
    }
  }

  if (!sCARefreshListeners) {
    sCARefreshListeners = new nsTArray<nsPluginInstanceOwner*>();
    if (!sCARefreshListeners) {
      return;
    }
  }

  if (sCARefreshListeners->Contains(this)) {
    return;
  }

  sCARefreshListeners->AppendElement(this);

  if (!sCATimer) {
    sCATimer = new nsCOMPtr<nsITimer>();
    if (!sCATimer) {
      return;
    }
  }

  if (sCARefreshListeners->Length() == 1) {
    *sCATimer = do_CreateInstance("@mozilla.org/timer;1");
    (*sCATimer)->InitWithFuncCallback(CARefresh, nullptr,
                   DEFAULT_REFRESH_RATE, nsITimer::TYPE_REPEATING_SLACK);
  }
}

void nsPluginInstanceOwner::RemoveFromCARefreshTimer() {
  if (!sCARefreshListeners || sCARefreshListeners->Contains(this) == false) {
    return;
  }

  sCARefreshListeners->RemoveElement(this);

  if (sCARefreshListeners->Length() == 0) {
    if (sCATimer) {
      (*sCATimer)->Cancel();
      delete sCATimer;
      sCATimer = nullptr;
    }
    delete sCARefreshListeners;
    sCARefreshListeners = nullptr;
  }
}

void nsPluginInstanceOwner::RenderCoreAnimation(CGContextRef aCGContext,
                                                int aWidth, int aHeight)
{
  if (aWidth == 0 || aHeight == 0)
    return;

  if (!mCARenderer) {
    mCARenderer = new nsCARenderer();
  }

  
  
  
  double scaleFactor = 1.0;
  GetContentsScaleFactor(&scaleFactor);

  if (!mIOSurface ||
      (mIOSurface->GetWidth() != (size_t)aWidth ||
       mIOSurface->GetHeight() != (size_t)aHeight ||
       mIOSurface->GetContentsScaleFactor() != scaleFactor)) {
    mIOSurface = nullptr;

    
    mIOSurface = MacIOSurface::CreateIOSurface(aWidth, aHeight, scaleFactor);
    if (mIOSurface) {
      RefPtr<MacIOSurface> attachSurface = MacIOSurface::LookupSurface(
                                              mIOSurface->GetIOSurfaceID(),
                                              scaleFactor);
      if (attachSurface) {
        mCARenderer->AttachIOSurface(attachSurface);
      } else {
        NS_ERROR("IOSurface attachment failed");
        mIOSurface = nullptr;
      }
    }
  }

  if (!mColorProfile) {
    mColorProfile = CreateSystemColorSpace();
  }

  if (mCARenderer->isInit() == false) {
    void *caLayer = nullptr;
    nsresult rv = mInstance->GetValueFromPlugin(NPPVpluginCoreAnimationLayer, &caLayer);
    if (NS_FAILED(rv) || !caLayer) {
      return;
    }

    
    
    mCARenderer->SetupRenderer(caLayer, aWidth, aHeight, scaleFactor,
                               DISALLOW_OFFLINE_RENDERER);

    
    
    FixUpPluginWindow(ePluginPaintDisable);
    FixUpPluginWindow(ePluginPaintEnable);
  }

  CGImageRef caImage = nullptr;
  nsresult rt = mCARenderer->Render(aWidth, aHeight, scaleFactor, &caImage);
  if (rt == NS_OK && mIOSurface && mColorProfile) {
    nsCARenderer::DrawSurfaceToCGContext(aCGContext, mIOSurface, mColorProfile,
                                         0, 0, aWidth, aHeight);
  } else if (rt == NS_OK && caImage != nullptr) {
    
    ::CGContextSetInterpolationQuality(aCGContext, kCGInterpolationNone );
    ::CGContextTranslateCTM(aCGContext, 0, (double) aHeight * scaleFactor);
    ::CGContextScaleCTM(aCGContext, scaleFactor, -scaleFactor);

    ::CGContextDrawImage(aCGContext, CGRectMake(0,0,aWidth,aHeight), caImage);
  } else {
    NS_NOTREACHED("nsCARenderer::Render failure");
  }
}

void* nsPluginInstanceOwner::GetPluginPortCopy()
{
  if (GetDrawingModel() == NPDrawingModelCoreGraphics ||
      GetDrawingModel() == NPDrawingModelCoreAnimation ||
      GetDrawingModel() == NPDrawingModelInvalidatingCoreAnimation)
    return &mCGPluginPortCopy;
  return nullptr;
}

void nsPluginInstanceOwner::SetPluginPort()
{
  void* pluginPort = GetPluginPort();
  if (!pluginPort || !mPluginWindow)
    return;
  mPluginWindow->window = pluginPort;
}

void nsPluginInstanceOwner::BeginCGPaint()
{
  ++mInCGPaintLevel;
}

void nsPluginInstanceOwner::EndCGPaint()
{
  --mInCGPaintLevel;
  NS_ASSERTION(mInCGPaintLevel >= 0, "Mismatched call to nsPluginInstanceOwner::EndCGPaint()!");
}

#endif


uint32_t
nsPluginInstanceOwner::GetEventloopNestingLevel()
{
  nsCOMPtr<nsIAppShell> appShell = do_GetService(kAppShellCID);
  uint32_t currentLevel = 0;
  if (appShell) {
    appShell->GetEventloopNestingLevel(&currentLevel);
#ifdef XP_MACOSX
    
    
    currentLevel++;
#endif
  }

  
  
  
  
  if (!currentLevel) {
    currentLevel++;
  }

  return currentLevel;
}

#ifdef MOZ_WIDGET_ANDROID




static nsPoint
GetOffsetRootContent(nsIFrame* aFrame)
{
  
  
  
  nsPoint offset(0, 0), docOffset(0, 0);
  const nsIFrame* f = aFrame;
  int32_t currAPD = aFrame->PresContext()->AppUnitsPerDevPixel();
  int32_t apd = currAPD;
  nsRect displayPort;
  while (f) {
    if (f->GetContent() && nsLayoutUtils::GetDisplayPort(f->GetContent(), &displayPort))
      break;

    docOffset += f->GetPosition();
    nsIFrame* parent = f->GetParent();
    if (parent) {
      f = parent;
    } else {
      nsPoint newOffset(0, 0);
      f = nsLayoutUtils::GetCrossDocParentFrame(f, &newOffset);
      int32_t newAPD = f ? f->PresContext()->AppUnitsPerDevPixel() : 0;
      if (!f || newAPD != currAPD) {
        
        offset += docOffset.ConvertAppUnits(currAPD, apd);
        docOffset.x = docOffset.y = 0;
      }
      currAPD = newAPD;
      docOffset += newOffset;
    }
  }

  offset += docOffset.ConvertAppUnits(currAPD, apd);

  return offset;
}

LayoutDeviceRect nsPluginInstanceOwner::GetPluginRect()
{
  
  nsRect bounds = mPluginFrame->GetContentRectRelativeToSelf() + GetOffsetRootContent(mPluginFrame);
  LayoutDeviceIntRect rect = LayoutDeviceIntRect::FromAppUnitsToNearest(bounds, mPluginFrame->PresContext()->AppUnitsPerDevPixel());
  return LayoutDeviceRect(rect);
}

bool nsPluginInstanceOwner::AddPluginView(const LayoutDeviceRect& aRect )
{
  if (!mJavaView) {
    mJavaView = mInstance->GetJavaSurface();

    if (!mJavaView)
      return false;

    mJavaView = (void*)AndroidBridge::GetJNIEnv()->NewGlobalRef((jobject)mJavaView);
  }

  if (AndroidBridge::Bridge())
    AndroidBridge::Bridge()->AddPluginView((jobject)mJavaView, aRect, mFullScreen);

  if (mFullScreen)
    sFullScreenInstance = this;

  return true;
}

void nsPluginInstanceOwner::RemovePluginView()
{
  if (!mInstance || !mJavaView)
    return;

  widget::GeckoAppShell::RemovePluginView(
      jni::Object::Ref::From(jobject(mJavaView)), mFullScreen);
  AndroidBridge::GetJNIEnv()->DeleteGlobalRef((jobject)mJavaView);
  mJavaView = nullptr;

  if (mFullScreen)
    sFullScreenInstance = nullptr;
}

void
nsPluginInstanceOwner::GetVideos(nsTArray<nsNPAPIPluginInstance::VideoInfo*>& aVideos)
{
  if (!mInstance)
    return;

  mInstance->GetVideos(aVideos);
}

already_AddRefed<ImageContainer>
nsPluginInstanceOwner::GetImageContainerForVideo(nsNPAPIPluginInstance::VideoInfo* aVideoInfo)
{
  nsRefPtr<ImageContainer> container = LayerManager::CreateImageContainer();

  nsRefPtr<Image> img = container->CreateImage(ImageFormat::SURFACE_TEXTURE);

  SurfaceTextureImage::Data data;

  data.mSurfTex = aVideoInfo->mSurfaceTexture;

  
  
  data.mOriginPos = AndroidBridge::Bridge()->IsHoneycomb() ? gl::OriginPos::BottomLeft
                                                           : mInstance->OriginPos();
  data.mSize = gfx::IntSize(aVideoInfo->mDimensions.width, aVideoInfo->mDimensions.height);

  SurfaceTextureImage* typedImg = static_cast<SurfaceTextureImage*>(img.get());
  typedImg->SetData(data);

  container->SetCurrentImageInTransaction(img);

  return container.forget();
}

void nsPluginInstanceOwner::Invalidate() {
  NPRect rect;
  rect.left = rect.top = 0;
  rect.right = mPluginWindow->width;
  rect.bottom = mPluginWindow->height;
  InvalidateRect(&rect);
}

void nsPluginInstanceOwner::RequestFullScreen() {
  if (mFullScreen)
    return;

  
  RemovePluginView();

  mFullScreen = true;
  AddPluginView();

  mInstance->NotifyFullScreen(mFullScreen);
}

void nsPluginInstanceOwner::ExitFullScreen() {
  if (!mFullScreen)
    return;

  RemovePluginView();

  mFullScreen = false;

  int32_t model = mInstance->GetANPDrawingModel();

  if (model == kSurface_ANPDrawingModel) {
    
    
    AddPluginView(GetPluginRect());
  }

  mInstance->NotifyFullScreen(mFullScreen);

  
  
  Invalidate();
}

void nsPluginInstanceOwner::ExitFullScreen(jobject view) {
  JNIEnv* env = AndroidBridge::GetJNIEnv();

  if (sFullScreenInstance && sFullScreenInstance->mInstance &&
      env->IsSameObject(view, (jobject)sFullScreenInstance->mInstance->GetJavaSurface())) {
    sFullScreenInstance->ExitFullScreen();
  }
}

#endif

void
nsPluginInstanceOwner::NotifyHostAsyncInitFailed()
{
  nsCOMPtr<nsIObjectLoadingContent> content = do_QueryInterface(mContent);
  content->StopPluginInstance();
}

void
nsPluginInstanceOwner::NotifyHostCreateWidget()
{
  mPluginHost->CreateWidget(this);
#ifdef XP_MACOSX
  FixUpPluginWindow(ePluginPaintEnable);
#else
  if (mPluginFrame) {
    mPluginFrame->InvalidateFrame();
  } else {
    CallSetWindow();
  }
#endif
}

nsresult nsPluginInstanceOwner::DispatchFocusToPlugin(nsIDOMEvent* aFocusEvent)
{
#ifdef MOZ_WIDGET_ANDROID
  if (mInstance) {
    ANPEvent event;
    event.inSize = sizeof(ANPEvent);
    event.eventType = kLifecycle_ANPEventType;

    nsAutoString eventType;
    aFocusEvent->GetType(eventType);
    if (eventType.EqualsLiteral("focus")) {
      event.data.lifecycle.action = kGainFocus_ANPLifecycleAction;
    }
    else if (eventType.EqualsLiteral("blur")) {
      event.data.lifecycle.action = kLoseFocus_ANPLifecycleAction;
    }
    else {
      NS_ASSERTION(false, "nsPluginInstanceOwner::DispatchFocusToPlugin, wierd eventType");
    }
    mInstance->HandleEvent(&event, nullptr);
  }
#endif

#ifndef XP_MACOSX
  if (!mPluginWindow || (mPluginWindow->type == NPWindowTypeWindow)) {
    
    return aFocusEvent->PreventDefault(); 
  }
#endif

  WidgetEvent* theEvent = aFocusEvent->GetInternalNSEvent();
  if (theEvent) {
    WidgetGUIEvent focusEvent(theEvent->mFlags.mIsTrusted, theEvent->message,
                              nullptr);
    nsEventStatus rv = ProcessEvent(focusEvent);
    if (nsEventStatus_eConsumeNoDefault == rv) {
      aFocusEvent->PreventDefault();
      aFocusEvent->StopPropagation();
    }
  }

  return NS_OK;
}

nsresult nsPluginInstanceOwner::ProcessKeyPress(nsIDOMEvent* aKeyEvent)
{
#ifdef XP_MACOSX
  return DispatchKeyToPlugin(aKeyEvent);
#else
  if (SendNativeEvents())
    DispatchKeyToPlugin(aKeyEvent);

  if (mInstance) {
    
    
    aKeyEvent->PreventDefault();
    aKeyEvent->StopPropagation();
  }
  return NS_OK;
#endif
}

nsresult nsPluginInstanceOwner::DispatchKeyToPlugin(nsIDOMEvent* aKeyEvent)
{
#if !defined(XP_MACOSX)
  if (!mPluginWindow || (mPluginWindow->type == NPWindowTypeWindow))
    return aKeyEvent->PreventDefault(); 
  
#endif

  if (mInstance) {
    WidgetKeyboardEvent* keyEvent =
      aKeyEvent->GetInternalNSEvent()->AsKeyboardEvent();
    if (keyEvent && keyEvent->mClass == eKeyboardEventClass) {
      nsEventStatus rv = ProcessEvent(*keyEvent);
      if (nsEventStatus_eConsumeNoDefault == rv) {
        aKeyEvent->PreventDefault();
        aKeyEvent->StopPropagation();
      }
    }
  }

  return NS_OK;
}

nsresult
nsPluginInstanceOwner::ProcessMouseDown(nsIDOMEvent* aMouseEvent)
{
#if !defined(XP_MACOSX)
  if (!mPluginWindow || (mPluginWindow->type == NPWindowTypeWindow))
    return aMouseEvent->PreventDefault(); 
  
#endif

  
  
  if (mPluginFrame && mPluginWindow &&
      mPluginWindow->type == NPWindowTypeDrawable) {

    nsIFocusManager* fm = nsFocusManager::GetFocusManager();
    if (fm) {
      nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(mContent);
      fm->SetFocus(elem, 0);
    }
  }

  WidgetMouseEvent* mouseEvent =
    aMouseEvent->GetInternalNSEvent()->AsMouseEvent();
  if (mouseEvent && mouseEvent->mClass == eMouseEventClass) {
    mLastMouseDownButtonType = mouseEvent->button;
    nsEventStatus rv = ProcessEvent(*mouseEvent);
    if (nsEventStatus_eConsumeNoDefault == rv) {
      return aMouseEvent->PreventDefault(); 
    }
  }

  return NS_OK;
}

nsresult nsPluginInstanceOwner::DispatchMouseToPlugin(nsIDOMEvent* aMouseEvent,
                                                      bool aAllowPropagate)
{
#if !defined(XP_MACOSX)
  if (!mPluginWindow || (mPluginWindow->type == NPWindowTypeWindow))
    return aMouseEvent->PreventDefault(); 
  
#endif
  
  if (!mWidgetVisible)
    return NS_OK;

  WidgetMouseEvent* mouseEvent =
    aMouseEvent->GetInternalNSEvent()->AsMouseEvent();
  if (mouseEvent && mouseEvent->mClass == eMouseEventClass) {
    nsEventStatus rv = ProcessEvent(*mouseEvent);
    if (nsEventStatus_eConsumeNoDefault == rv) {
      aMouseEvent->PreventDefault();
      if (!aAllowPropagate) {
        aMouseEvent->StopPropagation();
      }
    }
    if (mouseEvent->message == NS_MOUSE_BUTTON_UP) {
      mLastMouseDownButtonType = -1;
    }
  }
  return NS_OK;
}

nsresult
nsPluginInstanceOwner::HandleEvent(nsIDOMEvent* aEvent)
{
  NS_ASSERTION(mInstance, "Should have a valid plugin instance or not receive events.");

  nsAutoString eventType;
  aEvent->GetType(eventType);

#ifdef XP_MACOSX
  if (eventType.EqualsLiteral("activate") ||
      eventType.EqualsLiteral("deactivate")) {
    WindowFocusMayHaveChanged();
    return NS_OK;
  }
  if (eventType.EqualsLiteral("MozPerformDelayedBlur")) {
    if (mShouldBlurOnActivate) {
      WidgetGUIEvent blurEvent(true, NS_BLUR_CONTENT, nullptr);
      ProcessEvent(blurEvent);
      mShouldBlurOnActivate = false;
    }
    return NS_OK;
  }
#endif

  if (eventType.EqualsLiteral("focus")) {
    mContentFocused = true;
    return DispatchFocusToPlugin(aEvent);
  }
  if (eventType.EqualsLiteral("blur")) {
    mContentFocused = false;
    return DispatchFocusToPlugin(aEvent);
  }
  if (eventType.EqualsLiteral("mousedown")) {
    return ProcessMouseDown(aEvent);
  }
  if (eventType.EqualsLiteral("mouseup")) {
    return DispatchMouseToPlugin(aEvent);
  }
  if (eventType.EqualsLiteral("mousemove")) {
    return DispatchMouseToPlugin(aEvent, true);
  }
  if (eventType.EqualsLiteral("click") ||
      eventType.EqualsLiteral("dblclick") ||
      eventType.EqualsLiteral("mouseover") ||
      eventType.EqualsLiteral("mouseout")) {
    return DispatchMouseToPlugin(aEvent);
  }
  if (eventType.EqualsLiteral("keydown") ||
      eventType.EqualsLiteral("keyup")) {
    return DispatchKeyToPlugin(aEvent);
  }
  if (eventType.EqualsLiteral("keypress")) {
    return ProcessKeyPress(aEvent);
  }

  nsCOMPtr<nsIDOMDragEvent> dragEvent(do_QueryInterface(aEvent));
  if (dragEvent && mInstance) {
    WidgetEvent* ievent = aEvent->GetInternalNSEvent();
    if ((ievent && ievent->mFlags.mIsTrusted) &&
         ievent->message != NS_DRAGDROP_ENTER && ievent->message != NS_DRAGDROP_OVER) {
      aEvent->PreventDefault();
    }

    
    aEvent->StopPropagation();
  }
  return NS_OK;
}

#ifdef MOZ_X11
static unsigned int XInputEventState(const WidgetInputEvent& anEvent)
{
  unsigned int state = 0;
  if (anEvent.IsShift()) state |= ShiftMask;
  if (anEvent.IsControl()) state |= ControlMask;
  if (anEvent.IsAlt()) state |= Mod1Mask;
  if (anEvent.IsMeta()) state |= Mod4Mask;
  return state;
}
#endif

#ifdef XP_MACOSX



static bool
ContentIsFocusedWithinWindow(nsIContent* aContent)
{
  nsPIDOMWindow* outerWindow = aContent->OwnerDoc()->GetWindow();
  if (!outerWindow) {
    return false;
  }

  nsPIDOMWindow* rootWindow = outerWindow->GetPrivateRoot();
  if (!rootWindow) {
    return false;
  }

  nsFocusManager* fm = nsFocusManager::GetFocusManager();
  if (!fm) {
    return false;
  }

  nsCOMPtr<nsPIDOMWindow> focusedFrame;
  nsCOMPtr<nsIContent> focusedContent = fm->GetFocusedDescendant(rootWindow, true, getter_AddRefs(focusedFrame));
  return (focusedContent.get() == aContent);
}

static NPCocoaEventType
CocoaEventTypeForEvent(const WidgetGUIEvent& anEvent, nsIFrame* aObjectFrame)
{
  const NPCocoaEvent* event = static_cast<const NPCocoaEvent*>(anEvent.mPluginEvent);
  if (event) {
    return event->type;
  }

  switch (anEvent.message) {
    case NS_MOUSE_ENTER_SYNTH:
      return NPCocoaEventMouseEntered;
    case NS_MOUSE_EXIT_SYNTH:
      return NPCocoaEventMouseExited;
    case NS_MOUSE_MOVE:
    {
      
      
      
      if (nsIPresShell::GetCapturingContent()) {
        return NPCocoaEventMouseDragged;
      }

      return NPCocoaEventMouseMoved;
    }
    case NS_MOUSE_BUTTON_DOWN:
      return NPCocoaEventMouseDown;
    case NS_MOUSE_BUTTON_UP:
      return NPCocoaEventMouseUp;
    case NS_KEY_DOWN:
      return NPCocoaEventKeyDown;
    case NS_KEY_UP:
      return NPCocoaEventKeyUp;
    case NS_FOCUS_CONTENT:
    case NS_BLUR_CONTENT:
      return NPCocoaEventFocusChanged;
    case NS_MOUSE_SCROLL:
      return NPCocoaEventScrollWheel;
    default:
      return (NPCocoaEventType)0;
  }
}

static NPCocoaEvent
TranslateToNPCocoaEvent(WidgetGUIEvent* anEvent, nsIFrame* aObjectFrame)
{
  NPCocoaEvent cocoaEvent;
  InitializeNPCocoaEvent(&cocoaEvent);
  cocoaEvent.type = CocoaEventTypeForEvent(*anEvent, aObjectFrame);

  if (anEvent->message == NS_MOUSE_MOVE ||
      anEvent->message == NS_MOUSE_BUTTON_DOWN ||
      anEvent->message == NS_MOUSE_BUTTON_UP ||
      anEvent->message == NS_MOUSE_SCROLL ||
      anEvent->message == NS_MOUSE_ENTER_SYNTH ||
      anEvent->message == NS_MOUSE_EXIT_SYNTH)
  {
    nsPoint pt = nsLayoutUtils::GetEventCoordinatesRelativeTo(anEvent, aObjectFrame) -
                 aObjectFrame->GetContentRectRelativeToSelf().TopLeft();
    nsPresContext* presContext = aObjectFrame->PresContext();
    
    
    double scaleFactor = double(nsPresContext::AppUnitsPerCSSPixel())/
      aObjectFrame->PresContext()->DeviceContext()->AppUnitsPerDevPixelAtUnitFullZoom();
    size_t intScaleFactor = ceil(scaleFactor);
    nsIntPoint ptPx(presContext->AppUnitsToDevPixels(pt.x) / intScaleFactor,
                    presContext->AppUnitsToDevPixels(pt.y) / intScaleFactor);
    cocoaEvent.data.mouse.pluginX = double(ptPx.x);
    cocoaEvent.data.mouse.pluginY = double(ptPx.y);
  }

  switch (anEvent->message) {
    case NS_MOUSE_BUTTON_DOWN:
    case NS_MOUSE_BUTTON_UP:
    {
      WidgetMouseEvent* mouseEvent = anEvent->AsMouseEvent();
      if (mouseEvent) {
        switch (mouseEvent->button) {
          case WidgetMouseEvent::eLeftButton:
            cocoaEvent.data.mouse.buttonNumber = 0;
            break;
          case WidgetMouseEvent::eRightButton:
            cocoaEvent.data.mouse.buttonNumber = 1;
            break;
          case WidgetMouseEvent::eMiddleButton:
            cocoaEvent.data.mouse.buttonNumber = 2;
            break;
          default:
            NS_WARNING("Mouse button we don't know about?");
        }
        cocoaEvent.data.mouse.clickCount = mouseEvent->clickCount;
      } else {
        NS_WARNING("NS_MOUSE_BUTTON_UP/DOWN is not a WidgetMouseEvent?");
      }
      break;
    }
    case NS_MOUSE_SCROLL:
    {
      WidgetWheelEvent* wheelEvent = anEvent->AsWheelEvent();
      if (wheelEvent) {
        cocoaEvent.data.mouse.deltaX = wheelEvent->lineOrPageDeltaX;
        cocoaEvent.data.mouse.deltaY = wheelEvent->lineOrPageDeltaY;
      } else {
        NS_WARNING("NS_MOUSE_SCROLL is not a WidgetWheelEvent? (could be, haven't checked)");
      }
      break;
    }
    case NS_KEY_DOWN:
    case NS_KEY_UP:
    {
      WidgetKeyboardEvent* keyEvent = anEvent->AsKeyboardEvent();

      cocoaEvent.data.key.keyCode = keyEvent->mNativeKeyCode;
      cocoaEvent.data.key.isARepeat = keyEvent->mIsRepeat;
      cocoaEvent.data.key.modifierFlags = keyEvent->mNativeModifierFlags;
      const char16_t* nativeChars = keyEvent->mNativeCharacters.get();
      cocoaEvent.data.key.characters =
        (NPNSString*)::CFStringCreateWithCharacters(NULL,
                                                    reinterpret_cast<const UniChar*>(nativeChars),
                                                    keyEvent->mNativeCharacters.Length());
      const char16_t* nativeCharsIgnoringModifiers = keyEvent->mNativeCharactersIgnoringModifiers.get();
      cocoaEvent.data.key.charactersIgnoringModifiers =
        (NPNSString*)::CFStringCreateWithCharacters(NULL,
                                                    reinterpret_cast<const UniChar*>(nativeCharsIgnoringModifiers),
                                                    keyEvent->mNativeCharactersIgnoringModifiers.Length());
      break;
    }
    case NS_FOCUS_CONTENT:
    case NS_BLUR_CONTENT:
      cocoaEvent.data.focus.hasFocus = (anEvent->message == NS_FOCUS_CONTENT);
      break;
    default:
      break;
  }
  return cocoaEvent;
}

void nsPluginInstanceOwner::PerformDelayedBlurs()
{
  nsCOMPtr<EventTarget> windowRoot = mContent->OwnerDoc()->GetWindow()->GetTopWindowRoot();
  nsContentUtils::DispatchTrustedEvent(mContent->OwnerDoc(),
                                       windowRoot,
                                       NS_LITERAL_STRING("MozPerformDelayedBlur"),
                                       false, false, nullptr);
}

#endif

nsEventStatus nsPluginInstanceOwner::ProcessEvent(const WidgetGUIEvent& anEvent)
{
  nsEventStatus rv = nsEventStatus_eIgnore;

  if (!mInstance || !mPluginFrame) {
    return nsEventStatus_eIgnore;
  }

#ifdef XP_MACOSX
  NPEventModel eventModel = GetEventModel();
  if (eventModel != NPEventModelCocoa) {
    return nsEventStatus_eIgnore;
  }

  
  
  
  if (anEvent.message == NS_BLUR_CONTENT &&
      ContentIsFocusedWithinWindow(mContent)) {
    mShouldBlurOnActivate = true;
    return nsEventStatus_eIgnore;
  }

  
  
  
  
  if (anEvent.message == NS_FOCUS_CONTENT &&
      mLastContentFocused == true) {
    mShouldBlurOnActivate = false;
    return nsEventStatus_eIgnore;
  }

  
  
  
  if (anEvent.message == NS_FOCUS_CONTENT ||
      anEvent.message == NS_BLUR_CONTENT) {
    mLastContentFocused = (anEvent.message == NS_FOCUS_CONTENT);
    mShouldBlurOnActivate = false;
    PerformDelayedBlurs();
  }

  NPCocoaEvent cocoaEvent = TranslateToNPCocoaEvent(const_cast<WidgetGUIEvent*>(&anEvent), mPluginFrame);
  if (cocoaEvent.type == (NPCocoaEventType)0) {
    return nsEventStatus_eIgnore;
  }

  if (cocoaEvent.type == NPCocoaEventKeyDown) {
    ComplexTextInputPanel* ctiPanel = ComplexTextInputPanel::GetSharedComplexTextInputPanel();
    if (ctiPanel && ctiPanel->IsInComposition()) {
      nsAutoString outText;
      ctiPanel->InterpretKeyEvent(&cocoaEvent, outText);
      if (!outText.IsEmpty()) {
        CFStringRef cfString = ::CFStringCreateWithCharacters(kCFAllocatorDefault,
                                                              reinterpret_cast<const UniChar*>(outText.get()),
                                                              outText.Length());

        NPCocoaEvent textEvent;
        InitializeNPCocoaEvent(&textEvent);
        textEvent.type = NPCocoaEventTextInput;
        textEvent.data.text.text = (NPNSString*)cfString;

        mInstance->HandleEvent(&textEvent, nullptr);
      }
      return nsEventStatus_eConsumeNoDefault;
    }
  }

  int16_t response = kNPEventNotHandled;
  mInstance->HandleEvent(&cocoaEvent,
                         &response,
                         NS_PLUGIN_CALL_SAFE_TO_REENTER_GECKO);
  if (response == kNPEventStartIME) {
    nsAutoString outText;
    ComplexTextInputPanel* ctiPanel = ComplexTextInputPanel::GetSharedComplexTextInputPanel();

    
    
    double screenX, screenY;
    ConvertPoint(0.0, mPluginFrame->GetScreenRect().height, NPCoordinateSpacePlugin,
                 &screenX, &screenY, NPCoordinateSpaceScreen);

    ctiPanel->PlacePanel(screenX, screenY);
    ctiPanel->InterpretKeyEvent(&cocoaEvent, outText);

    if (!outText.IsEmpty()) {
      CFStringRef cfString = ::CFStringCreateWithCharacters(kCFAllocatorDefault,
                                                            reinterpret_cast<const UniChar*>(outText.get()),
                                                            outText.Length());

      NPCocoaEvent textEvent;
      InitializeNPCocoaEvent(&textEvent);
      textEvent.type = NPCocoaEventTextInput;
      textEvent.data.text.text = (NPNSString*)cfString;

      mInstance->HandleEvent(&textEvent, nullptr);
     }
  }

  bool handled = (response == kNPEventHandled || response == kNPEventStartIME);
  bool leftMouseButtonDown = (anEvent.message == NS_MOUSE_BUTTON_DOWN) &&
                             (anEvent.AsMouseEvent()->button == WidgetMouseEvent::eLeftButton);
  if (handled && !(leftMouseButtonDown && !mContentFocused)) {
    rv = nsEventStatus_eConsumeNoDefault;
  }
#endif

#ifdef XP_WIN
  
  const NPEvent *pPluginEvent = static_cast<const NPEvent*>(anEvent.mPluginEvent);
  
  
  NPEvent pluginEvent;
  if (anEvent.mClass == eMouseEventClass) {
    if (!pPluginEvent) {
      
      
      pluginEvent.event = 0;
      const WidgetMouseEvent* mouseEvent = anEvent.AsMouseEvent();
      switch (anEvent.message) {
      case NS_MOUSE_MOVE:
        pluginEvent.event = WM_MOUSEMOVE;
        break;
      case NS_MOUSE_BUTTON_DOWN: {
        static const int downMsgs[] =
          { WM_LBUTTONDOWN, WM_MBUTTONDOWN, WM_RBUTTONDOWN };
        static const int dblClickMsgs[] =
          { WM_LBUTTONDBLCLK, WM_MBUTTONDBLCLK, WM_RBUTTONDBLCLK };
        if (mouseEvent->clickCount == 2) {
          pluginEvent.event = dblClickMsgs[mouseEvent->button];
        } else {
          pluginEvent.event = downMsgs[mouseEvent->button];
        }
        break;
      }
      case NS_MOUSE_BUTTON_UP: {
        static const int upMsgs[] =
          { WM_LBUTTONUP, WM_MBUTTONUP, WM_RBUTTONUP };
        pluginEvent.event = upMsgs[mouseEvent->button];
        break;
      }
      
      
      
      default:
        break;
      }
      if (pluginEvent.event) {
        pPluginEvent = &pluginEvent;
        pluginEvent.wParam =
          (::GetKeyState(VK_CONTROL) ? MK_CONTROL : 0) |
          (::GetKeyState(VK_SHIFT) ? MK_SHIFT : 0) |
          (::GetKeyState(VK_LBUTTON) ? MK_LBUTTON : 0) |
          (::GetKeyState(VK_MBUTTON) ? MK_MBUTTON : 0) |
          (::GetKeyState(VK_RBUTTON) ? MK_RBUTTON : 0) |
          (::GetKeyState(VK_XBUTTON1) ? MK_XBUTTON1 : 0) |
          (::GetKeyState(VK_XBUTTON2) ? MK_XBUTTON2 : 0);
      }
    }
    if (pPluginEvent) {
      
      
      
      
      NS_ASSERTION(anEvent.message == NS_MOUSE_BUTTON_DOWN ||
                   anEvent.message == NS_MOUSE_BUTTON_UP ||
                   anEvent.message == NS_MOUSE_DOUBLECLICK ||
                   anEvent.message == NS_MOUSE_ENTER_SYNTH ||
                   anEvent.message == NS_MOUSE_EXIT_SYNTH ||
                   anEvent.message == NS_MOUSE_MOVE,
                   "Incorrect event type for coordinate translation");
      nsPoint pt =
        nsLayoutUtils::GetEventCoordinatesRelativeTo(&anEvent, mPluginFrame) -
        mPluginFrame->GetContentRectRelativeToSelf().TopLeft();
      nsPresContext* presContext = mPluginFrame->PresContext();
      nsIntPoint ptPx(presContext->AppUnitsToDevPixels(pt.x),
                      presContext->AppUnitsToDevPixels(pt.y));
      nsIntPoint widgetPtPx = ptPx + mPluginFrame->GetWindowOriginInPixels(true);
      const_cast<NPEvent*>(pPluginEvent)->lParam = MAKELPARAM(widgetPtPx.x, widgetPtPx.y);
    }
  }
  else if (!pPluginEvent) {
    switch (anEvent.message) {
      case NS_FOCUS_CONTENT:
        pluginEvent.event = WM_SETFOCUS;
        pluginEvent.wParam = 0;
        pluginEvent.lParam = 0;
        pPluginEvent = &pluginEvent;
        break;
      case NS_BLUR_CONTENT:
        pluginEvent.event = WM_KILLFOCUS;
        pluginEvent.wParam = 0;
        pluginEvent.lParam = 0;
        pPluginEvent = &pluginEvent;
        break;
    }
  }

  if (pPluginEvent && !pPluginEvent->event) {
    
    NS_WARNING("nsPluginFrame ProcessEvent: trying to send null event to plugin.");
    return rv;
  }

  if (pPluginEvent) {
    int16_t response = kNPEventNotHandled;
    mInstance->HandleEvent(const_cast<NPEvent*>(pPluginEvent),
                           &response,
                           NS_PLUGIN_CALL_SAFE_TO_REENTER_GECKO);
    if (response == kNPEventHandled)
      rv = nsEventStatus_eConsumeNoDefault;
  }
#endif

#ifdef MOZ_X11
  
  nsIWidget* widget = anEvent.widget;
  XEvent pluginEvent = XEvent();
  pluginEvent.type = 0;

  switch(anEvent.mClass) {
    case eMouseEventClass:
      {
        switch (anEvent.message)
          {
          case NS_MOUSE_CLICK:
          case NS_MOUSE_DOUBLECLICK:
            
            return rv;
          }

        
        const nsPresContext* presContext = mPluginFrame->PresContext();
        nsPoint appPoint =
          nsLayoutUtils::GetEventCoordinatesRelativeTo(&anEvent, mPluginFrame) -
          mPluginFrame->GetContentRectRelativeToSelf().TopLeft();
        nsIntPoint pluginPoint(presContext->AppUnitsToDevPixels(appPoint.x),
                               presContext->AppUnitsToDevPixels(appPoint.y));
        const WidgetMouseEvent& mouseEvent = *anEvent.AsMouseEvent();
        
        LayoutDeviceIntPoint rootPoint(-1, -1);
        if (widget)
          rootPoint = anEvent.refPoint +
            LayoutDeviceIntPoint::FromUntyped(widget->WidgetToScreenOffset());
#ifdef MOZ_WIDGET_GTK
        Window root = GDK_ROOT_WINDOW();
#elif defined(MOZ_WIDGET_QT)
        Window root = RootWindowOfScreen(DefaultScreenOfDisplay(mozilla::DefaultXDisplay()));
#else
        Window root = None; 
#endif

        switch (anEvent.message)
          {
          case NS_MOUSE_ENTER_SYNTH:
          case NS_MOUSE_EXIT_SYNTH:
            {
              XCrossingEvent& event = pluginEvent.xcrossing;
              event.type = anEvent.message == NS_MOUSE_ENTER_SYNTH ?
                EnterNotify : LeaveNotify;
              event.root = root;
              event.time = anEvent.time;
              event.x = pluginPoint.x;
              event.y = pluginPoint.y;
              event.x_root = rootPoint.x;
              event.y_root = rootPoint.y;
              event.state = XInputEventState(mouseEvent);
              
              event.subwindow = None;
              event.mode = -1;
              event.detail = NotifyDetailNone;
              event.same_screen = True;
              event.focus = mContentFocused;
            }
            break;
          case NS_MOUSE_MOVE:
            {
              XMotionEvent& event = pluginEvent.xmotion;
              event.type = MotionNotify;
              event.root = root;
              event.time = anEvent.time;
              event.x = pluginPoint.x;
              event.y = pluginPoint.y;
              event.x_root = rootPoint.x;
              event.y_root = rootPoint.y;
              event.state = XInputEventState(mouseEvent);
              
              event.subwindow = None;
              event.is_hint = NotifyNormal;
              event.same_screen = True;
            }
            break;
          case NS_MOUSE_BUTTON_DOWN:
          case NS_MOUSE_BUTTON_UP:
            {
              XButtonEvent& event = pluginEvent.xbutton;
              event.type = anEvent.message == NS_MOUSE_BUTTON_DOWN ?
                ButtonPress : ButtonRelease;
              event.root = root;
              event.time = anEvent.time;
              event.x = pluginPoint.x;
              event.y = pluginPoint.y;
              event.x_root = rootPoint.x;
              event.y_root = rootPoint.y;
              event.state = XInputEventState(mouseEvent);
              switch (mouseEvent.button)
                {
                case WidgetMouseEvent::eMiddleButton:
                  event.button = 2;
                  break;
                case WidgetMouseEvent::eRightButton:
                  event.button = 3;
                  break;
                default: 
                  event.button = 1;
                  break;
                }
              
              event.subwindow = None;
              event.same_screen = True;
            }
            break;
          }
      }
      break;

   

   case eKeyboardEventClass:
      if (anEvent.mPluginEvent)
        {
          XKeyEvent &event = pluginEvent.xkey;
#ifdef MOZ_WIDGET_GTK
          event.root = GDK_ROOT_WINDOW();
          event.time = anEvent.time;
          const GdkEventKey* gdkEvent =
            static_cast<const GdkEventKey*>(anEvent.mPluginEvent);
          event.keycode = gdkEvent->hardware_keycode;
          event.state = gdkEvent->state;
          switch (anEvent.message)
            {
            case NS_KEY_DOWN:
              
              
              if (gdkEvent->is_modifier)
                event.type = XKeyPress;
              break;
            case NS_KEY_PRESS:
              event.type = XKeyPress;
              break;
            case NS_KEY_UP:
              event.type = KeyRelease;
              break;
            }
#endif

          
          
          event.subwindow = None;
          event.x = 0;
          event.y = 0;
          event.x_root = -1;
          event.y_root = -1;
          event.same_screen = False;
        }
      else
        {
          
          
          
          
          NS_WARNING("Synthesized key event not sent to plugin");
        }
      break;

    default:
      switch (anEvent.message)
        {
        case NS_FOCUS_CONTENT:
        case NS_BLUR_CONTENT:
          {
            XFocusChangeEvent &event = pluginEvent.xfocus;
            event.type =
              anEvent.message == NS_FOCUS_CONTENT ? FocusIn : FocusOut;
            
            event.mode = -1;
            event.detail = NotifyDetailNone;
          }
          break;
        }
    }

  if (!pluginEvent.type) {
    return rv;
  }

  
  XAnyEvent& event = pluginEvent.xany;
  event.display = widget ?
    static_cast<Display*>(widget->GetNativeData(NS_NATIVE_DISPLAY)) : nullptr;
  event.window = None; 
  
  event.serial = 0;
  event.send_event = False;

  int16_t response = kNPEventNotHandled;
  mInstance->HandleEvent(&pluginEvent, &response, NS_PLUGIN_CALL_SAFE_TO_REENTER_GECKO);
  if (response == kNPEventHandled)
    rv = nsEventStatus_eConsumeNoDefault;
#endif

#ifdef MOZ_WIDGET_ANDROID
  
  {
    
    nsIFocusManager* fm = nsFocusManager::GetFocusManager();
    if (fm) {
      nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(mContent);
      fm->SetFocus(elem, 0);
    }
  }
  switch(anEvent.mClass) {
    case eMouseEventClass:
      {
        switch (anEvent.message)
          {
          case NS_MOUSE_CLICK:
          case NS_MOUSE_DOUBLECLICK:
            
            return rv;
          }

        
        const nsPresContext* presContext = mPluginFrame->PresContext();
        nsPoint appPoint =
          nsLayoutUtils::GetEventCoordinatesRelativeTo(&anEvent, mPluginFrame) -
          mPluginFrame->GetContentRectRelativeToSelf().TopLeft();
        nsIntPoint pluginPoint(presContext->AppUnitsToDevPixels(appPoint.x),
                               presContext->AppUnitsToDevPixels(appPoint.y));

        switch (anEvent.message)
          {
          case NS_MOUSE_MOVE:
            {
              
              
              
            }
            break;
          case NS_MOUSE_BUTTON_DOWN:
            {
              ANPEvent event;
              event.inSize = sizeof(ANPEvent);
              event.eventType = kMouse_ANPEventType;
              event.data.mouse.action = kDown_ANPMouseAction;
              event.data.mouse.x = pluginPoint.x;
              event.data.mouse.y = pluginPoint.y;
              mInstance->HandleEvent(&event, nullptr, NS_PLUGIN_CALL_SAFE_TO_REENTER_GECKO);
            }
            break;
          case NS_MOUSE_BUTTON_UP:
            {
              ANPEvent event;
              event.inSize = sizeof(ANPEvent);
              event.eventType = kMouse_ANPEventType;
              event.data.mouse.action = kUp_ANPMouseAction;
              event.data.mouse.x = pluginPoint.x;
              event.data.mouse.y = pluginPoint.y;
              mInstance->HandleEvent(&event, nullptr, NS_PLUGIN_CALL_SAFE_TO_REENTER_GECKO);
            }
            break;
          }
      }
      break;

    case eKeyboardEventClass:
     {
       const WidgetKeyboardEvent& keyEvent = *anEvent.AsKeyboardEvent();
       LOG("Firing eKeyboardEventClass %d %d\n",
           keyEvent.keyCode, keyEvent.charCode);
       
       const ANPEvent* pluginEvent = static_cast<const ANPEvent*>(keyEvent.mPluginEvent);
       if (pluginEvent) {
         MOZ_ASSERT(pluginEvent->inSize == sizeof(ANPEvent));
         MOZ_ASSERT(pluginEvent->eventType == kKey_ANPEventType);
         mInstance->HandleEvent(const_cast<ANPEvent*>(pluginEvent),
                                nullptr,
                                NS_PLUGIN_CALL_SAFE_TO_REENTER_GECKO);
       }
     }
     break;

    default:
      break;
    }
    rv = nsEventStatus_eConsumeNoDefault;
#endif

  return rv;
}

nsresult
nsPluginInstanceOwner::Destroy()
{
  SetFrame(nullptr);

#ifdef XP_MACOSX
  RemoveFromCARefreshTimer();
  if (mColorProfile)
    ::CGColorSpaceRelease(mColorProfile);
#endif

  
  if (mCXMenuListener) {
    mCXMenuListener->Destroy(mContent);
    mCXMenuListener = nullptr;
  }

  mContent->RemoveEventListener(NS_LITERAL_STRING("focus"), this, false);
  mContent->RemoveEventListener(NS_LITERAL_STRING("blur"), this, false);
  mContent->RemoveEventListener(NS_LITERAL_STRING("mouseup"), this, false);
  mContent->RemoveEventListener(NS_LITERAL_STRING("mousedown"), this, false);
  mContent->RemoveEventListener(NS_LITERAL_STRING("mousemove"), this, false);
  mContent->RemoveEventListener(NS_LITERAL_STRING("click"), this, false);
  mContent->RemoveEventListener(NS_LITERAL_STRING("dblclick"), this, false);
  mContent->RemoveEventListener(NS_LITERAL_STRING("mouseover"), this, false);
  mContent->RemoveEventListener(NS_LITERAL_STRING("mouseout"), this, false);
  mContent->RemoveEventListener(NS_LITERAL_STRING("keypress"), this, true);
  mContent->RemoveEventListener(NS_LITERAL_STRING("keydown"), this, true);
  mContent->RemoveEventListener(NS_LITERAL_STRING("keyup"), this, true);
  mContent->RemoveEventListener(NS_LITERAL_STRING("drop"), this, true);
  mContent->RemoveEventListener(NS_LITERAL_STRING("dragdrop"), this, true);
  mContent->RemoveEventListener(NS_LITERAL_STRING("drag"), this, true);
  mContent->RemoveEventListener(NS_LITERAL_STRING("dragenter"), this, true);
  mContent->RemoveEventListener(NS_LITERAL_STRING("dragover"), this, true);
  mContent->RemoveEventListener(NS_LITERAL_STRING("dragleave"), this, true);
  mContent->RemoveEventListener(NS_LITERAL_STRING("dragexit"), this, true);
  mContent->RemoveEventListener(NS_LITERAL_STRING("dragstart"), this, true);
  mContent->RemoveEventListener(NS_LITERAL_STRING("draggesture"), this, true);
  mContent->RemoveEventListener(NS_LITERAL_STRING("dragend"), this, true);

#if MOZ_WIDGET_ANDROID
  RemovePluginView();
#endif

  if (mWidget) {
    if (mPluginWindow) {
      mPluginWindow->SetPluginWidget(nullptr);
    }

    nsCOMPtr<nsIPluginWidget> pluginWidget = do_QueryInterface(mWidget);
    if (pluginWidget) {
      pluginWidget->SetPluginInstanceOwner(nullptr);
    }
    mWidget->Destroy();
  }

  return NS_OK;
}



#ifdef XP_MACOSX
void nsPluginInstanceOwner::Paint(const gfxRect& aDirtyRect, CGContextRef cgContext)
{
  if (!mInstance || !mPluginFrame)
    return;

  gfxRect dirtyRectCopy = aDirtyRect;
  double scaleFactor = 1.0;
  GetContentsScaleFactor(&scaleFactor);
  if (scaleFactor != 1.0) {
    ::CGContextScaleCTM(cgContext, scaleFactor, scaleFactor);
    
    
    dirtyRectCopy.ScaleRoundOut(1.0 / scaleFactor);
  }

  DoCocoaEventDrawRect(dirtyRectCopy, cgContext);
}

void nsPluginInstanceOwner::DoCocoaEventDrawRect(const gfxRect& aDrawRect, CGContextRef cgContext)
{
  if (!mInstance || !mPluginFrame)
    return;

  
  NPCocoaEvent updateEvent;
  InitializeNPCocoaEvent(&updateEvent);
  updateEvent.type = NPCocoaEventDrawRect;
  updateEvent.data.draw.context = cgContext;
  updateEvent.data.draw.x = aDrawRect.X();
  updateEvent.data.draw.y = aDrawRect.Y();
  updateEvent.data.draw.width = aDrawRect.Width();
  updateEvent.data.draw.height = aDrawRect.Height();

  mInstance->HandleEvent(&updateEvent, nullptr);
}
#endif

#ifdef XP_WIN
void nsPluginInstanceOwner::Paint(const RECT& aDirty, HDC aDC)
{
  if (!mInstance || !mPluginFrame)
    return;

  NPEvent pluginEvent;
  pluginEvent.event = WM_PAINT;
  pluginEvent.wParam = WPARAM(aDC);
  pluginEvent.lParam = LPARAM(&aDirty);
  mInstance->HandleEvent(&pluginEvent, nullptr);
}
#endif

#ifdef MOZ_WIDGET_ANDROID

void nsPluginInstanceOwner::Paint(gfxContext* aContext,
                                  const gfxRect& aFrameRect,
                                  const gfxRect& aDirtyRect)
{
  if (!mInstance || !mPluginFrame || !mPluginDocumentActiveState || mFullScreen)
    return;

  int32_t model = mInstance->GetANPDrawingModel();

  if (model == kSurface_ANPDrawingModel) {
    if (!AddPluginView(GetPluginRect())) {
      Invalidate();
    }
    return;
  }

  if (model != kBitmap_ANPDrawingModel)
    return;

#ifdef ANP_BITMAP_DRAWING_MODEL
  static nsRefPtr<gfxImageSurface> pluginSurface;

  if (pluginSurface == nullptr ||
      aFrameRect.width  != pluginSurface->Width() ||
      aFrameRect.height != pluginSurface->Height()) {

    pluginSurface = new gfxImageSurface(gfxIntSize(aFrameRect.width, aFrameRect.height),
                                        gfxImageFormat::ARGB32);
    if (!pluginSurface)
      return;
  }

  
  gfxUtils::ClearThebesSurface(pluginSurface);

  ANPEvent event;
  event.inSize = sizeof(ANPEvent);
  event.eventType = 4;
  event.data.draw.model = 1;

  event.data.draw.clip.top     = 0;
  event.data.draw.clip.left    = 0;
  event.data.draw.clip.bottom  = aFrameRect.width;
  event.data.draw.clip.right   = aFrameRect.height;

  event.data.draw.data.bitmap.format   = kRGBA_8888_ANPBitmapFormat;
  event.data.draw.data.bitmap.width    = aFrameRect.width;
  event.data.draw.data.bitmap.height   = aFrameRect.height;
  event.data.draw.data.bitmap.baseAddr = pluginSurface->Data();
  event.data.draw.data.bitmap.rowBytes = aFrameRect.width * 4;

  if (!mInstance)
    return;

  mInstance->HandleEvent(&event, nullptr);

  aContext->SetOperator(gfxContext::OPERATOR_SOURCE);
  aContext->SetSource(pluginSurface, gfxPoint(aFrameRect.x, aFrameRect.y));
  aContext->Clip(aFrameRect);
  aContext->Paint();
#endif
}
#endif

#if defined(MOZ_X11)
void nsPluginInstanceOwner::Paint(gfxContext* aContext,
                                  const gfxRect& aFrameRect,
                                  const gfxRect& aDirtyRect)
{
  if (!mInstance || !mPluginFrame)
    return;

  
  gfxRect pluginRect = aFrameRect;
  if (aContext->UserToDevicePixelSnapped(pluginRect)) {
    pluginRect = aContext->DeviceToUser(pluginRect);
  }

  
  
  gfxRect dirtyRect = aDirtyRect - pluginRect.TopLeft();
  dirtyRect.RoundOut();

  
  
  
  
  
  
  
  
  
  
  nsIntSize pluginSize(NS_lround(pluginRect.width),
                       NS_lround(pluginRect.height));

  
  nsIntRect pluginDirtyRect(int32_t(dirtyRect.x),
                            int32_t(dirtyRect.y),
                            int32_t(dirtyRect.width),
                            int32_t(dirtyRect.height));
  if (!pluginDirtyRect.
      IntersectRect(nsIntRect(0, 0, pluginSize.width, pluginSize.height),
                    pluginDirtyRect))
    return;

  NPWindow* window;
  GetWindow(window);

  uint32_t rendererFlags = 0;
  if (!mFlash10Quirks) {
    rendererFlags |=
      Renderer::DRAW_SUPPORTS_CLIP_RECT |
      Renderer::DRAW_SUPPORTS_ALTERNATE_VISUAL;
  }

  bool transparent;
  mInstance->IsTransparent(&transparent);
  if (!transparent)
    rendererFlags |= Renderer::DRAW_IS_OPAQUE;

  
  gfxContextAutoSaveRestore autoSR(aContext);
  aContext->SetMatrix(
    aContext->CurrentMatrix().Translate(pluginRect.TopLeft()));

  Renderer renderer(window, this, pluginSize, pluginDirtyRect);

  Display* dpy = mozilla::DefaultXDisplay();
  Screen* screen = DefaultScreenOfDisplay(dpy);
  Visual* visual = DefaultVisualOfScreen(screen);

  renderer.Draw(aContext, nsIntSize(window->width, window->height),
                rendererFlags, screen, visual);
}
nsresult
nsPluginInstanceOwner::Renderer::DrawWithXlib(cairo_surface_t* xsurface,
                                              nsIntPoint offset,
                                              nsIntRect *clipRects,
                                              uint32_t numClipRects)
{
  Screen *screen = cairo_xlib_surface_get_screen(xsurface);
  Colormap colormap;
  Visual* visual;
  if (!gfxXlibSurface::GetColormapAndVisual(xsurface, &colormap, &visual)) {
    NS_ERROR("Failed to get visual and colormap");
    return NS_ERROR_UNEXPECTED;
  }

  nsNPAPIPluginInstance *instance = mInstanceOwner->mInstance;
  if (!instance)
    return NS_ERROR_FAILURE;

  
  bool doupdatewindow = false;

  if (mWindow->x != offset.x || mWindow->y != offset.y) {
    mWindow->x = offset.x;
    mWindow->y = offset.y;
    doupdatewindow = true;
  }

  if (nsIntSize(mWindow->width, mWindow->height) != mPluginSize) {
    mWindow->width = mPluginSize.width;
    mWindow->height = mPluginSize.height;
    doupdatewindow = true;
  }

  
  NS_ASSERTION(numClipRects <= 1, "We don't support multiple clip rectangles!");
  nsIntRect clipRect;
  if (numClipRects) {
    clipRect.x = clipRects[0].x;
    clipRect.y = clipRects[0].y;
    clipRect.width  = clipRects[0].width;
    clipRect.height = clipRects[0].height;
    
    
    NS_ASSERTION(clipRect.x >= 0 && clipRect.y >= 0,
                 "Clip rectangle offsets are negative!");
  }
  else {
    clipRect.x = offset.x;
    clipRect.y = offset.y;
    clipRect.width  = mWindow->width;
    clipRect.height = mWindow->height;
    
    
    clipRect.IntersectRect(clipRect,
                           nsIntRect(0, 0,
                                     cairo_xlib_surface_get_width(xsurface),
                                     cairo_xlib_surface_get_height(xsurface)));
  }

  NPRect newClipRect;
  newClipRect.left = clipRect.x;
  newClipRect.top = clipRect.y;
  newClipRect.right = clipRect.XMost();
  newClipRect.bottom = clipRect.YMost();
  if (mWindow->clipRect.left    != newClipRect.left   ||
      mWindow->clipRect.top     != newClipRect.top    ||
      mWindow->clipRect.right   != newClipRect.right  ||
      mWindow->clipRect.bottom  != newClipRect.bottom) {
    mWindow->clipRect = newClipRect;
    doupdatewindow = true;
  }

  NPSetWindowCallbackStruct* ws_info =
    static_cast<NPSetWindowCallbackStruct*>(mWindow->ws_info);
#ifdef MOZ_X11
  if (ws_info->visual != visual || ws_info->colormap != colormap) {
    ws_info->visual = visual;
    ws_info->colormap = colormap;
    ws_info->depth = gfxXlibSurface::DepthOfVisual(screen, visual);
    doupdatewindow = true;
  }
#endif

  {
    if (doupdatewindow)
      instance->SetWindow(mWindow);
  }

  
  nsIntRect dirtyRect = mDirtyRect + offset;
  if (mInstanceOwner->mFlash10Quirks) {
    
    
    
    
    dirtyRect.SetRect(offset.x, offset.y,
                      mDirtyRect.XMost(), mDirtyRect.YMost());
  }
  
  
  if (!dirtyRect.IntersectRect(dirtyRect, clipRect))
    return NS_OK;

  {
    XEvent pluginEvent = XEvent();
    XGraphicsExposeEvent& exposeEvent = pluginEvent.xgraphicsexpose;
    
    exposeEvent.type = GraphicsExpose;
    exposeEvent.display = DisplayOfScreen(screen);
    exposeEvent.drawable = cairo_xlib_surface_get_drawable(xsurface);
    exposeEvent.x = dirtyRect.x;
    exposeEvent.y = dirtyRect.y;
    exposeEvent.width  = dirtyRect.width;
    exposeEvent.height = dirtyRect.height;
    exposeEvent.count = 0;
    
    exposeEvent.serial = 0;
    exposeEvent.send_event = False;
    exposeEvent.major_code = 0;
    exposeEvent.minor_code = 0;

    instance->HandleEvent(&pluginEvent, nullptr);
  }
  return NS_OK;
}
#endif

nsresult nsPluginInstanceOwner::Init(nsIContent* aContent)
{
  mLastEventloopNestingLevel = GetEventloopNestingLevel();

  mContent = aContent;

  
  
  nsIFrame* frame = aContent->GetPrimaryFrame();
  nsIObjectFrame* iObjFrame = do_QueryFrame(frame);
  nsPluginFrame* objFrame =  static_cast<nsPluginFrame*>(iObjFrame);
  if (objFrame) {
    SetFrame(objFrame);
    
    
    
    
    objFrame->PresContext()->EnsureVisible();
  } else {
    NS_NOTREACHED("Should not be initializing plugin without a frame");
    return NS_ERROR_FAILURE;
  }

  
  mCXMenuListener = new nsPluginDOMContextMenuListener(aContent);

  mContent->AddEventListener(NS_LITERAL_STRING("focus"), this, false,
                             false);
  mContent->AddEventListener(NS_LITERAL_STRING("blur"), this, false,
                             false);
  mContent->AddEventListener(NS_LITERAL_STRING("mouseup"), this, false,
                             false);
  mContent->AddEventListener(NS_LITERAL_STRING("mousedown"), this, false,
                             false);
  mContent->AddEventListener(NS_LITERAL_STRING("mousemove"), this, false,
                             false);
  mContent->AddEventListener(NS_LITERAL_STRING("click"), this, false,
                             false);
  mContent->AddEventListener(NS_LITERAL_STRING("dblclick"), this, false,
                             false);
  mContent->AddEventListener(NS_LITERAL_STRING("mouseover"), this, false,
                             false);
  mContent->AddEventListener(NS_LITERAL_STRING("mouseout"), this, false,
                             false);
  mContent->AddEventListener(NS_LITERAL_STRING("keypress"), this, true);
  mContent->AddEventListener(NS_LITERAL_STRING("keydown"), this, true);
  mContent->AddEventListener(NS_LITERAL_STRING("keyup"), this, true);
  mContent->AddEventListener(NS_LITERAL_STRING("drop"), this, true);
  mContent->AddEventListener(NS_LITERAL_STRING("dragdrop"), this, true);
  mContent->AddEventListener(NS_LITERAL_STRING("drag"), this, true);
  mContent->AddEventListener(NS_LITERAL_STRING("dragenter"), this, true);
  mContent->AddEventListener(NS_LITERAL_STRING("dragover"), this, true);
  mContent->AddEventListener(NS_LITERAL_STRING("dragleave"), this, true);
  mContent->AddEventListener(NS_LITERAL_STRING("dragexit"), this, true);
  mContent->AddEventListener(NS_LITERAL_STRING("dragstart"), this, true);
  mContent->AddEventListener(NS_LITERAL_STRING("draggesture"), this, true);
  mContent->AddEventListener(NS_LITERAL_STRING("dragend"), this, true);

  return NS_OK;
}

void* nsPluginInstanceOwner::GetPluginPort()
{


  void* result = nullptr;
  if (mWidget) {
#ifdef XP_WIN
    if (mPluginWindow && (mPluginWindow->type == NPWindowTypeDrawable))
      result = mWidget->GetNativeData(NS_NATIVE_GRAPHIC); 
    else
#endif
      result = mWidget->GetNativeData(NS_NATIVE_PLUGIN_PORT); 
  }

  return result;
}

void nsPluginInstanceOwner::ReleasePluginPort(void * pluginPort)
{
#ifdef XP_WIN
  if (mWidget && mPluginWindow &&
      mPluginWindow->type == NPWindowTypeDrawable) {
    mWidget->FreeNativeData((HDC)pluginPort, NS_NATIVE_GRAPHIC);
  }
#endif
}

NS_IMETHODIMP nsPluginInstanceOwner::CreateWidget(void)
{
  NS_ENSURE_TRUE(mPluginWindow, NS_ERROR_NULL_POINTER);

  nsresult rv = NS_ERROR_FAILURE;

  
  if (mWidget) {
    NS_WARNING("Trying to create a plugin widget twice!");
    return NS_ERROR_FAILURE;
  }

  bool windowless = false;
  mInstance->IsWindowless(&windowless);
  if (!windowless) {
    
    
    nsCOMPtr<nsIWidget> parentWidget;
    nsIDocument *doc = nullptr;
    if (mContent) {
      doc = mContent->OwnerDoc();
      parentWidget = nsContentUtils::WidgetForDocument(doc);
#ifndef XP_MACOSX
      
      if (XRE_GetProcessType() == GeckoProcessType_Content) {
        nsCOMPtr<nsIDOMWindow> window = doc->GetWindow();
        if (window) {
          nsCOMPtr<nsIDOMWindow> topWindow;
          window->GetTop(getter_AddRefs(topWindow));
          if (topWindow) {
            dom::TabChild* tc = dom::TabChild::GetFrom(topWindow);
            if (tc) {
              
              mWidget = tc->CreatePluginWidget(parentWidget.get());
            }
          }
        }
      }
#endif 
    }
    if (!mWidget) {
      
      mWidget = do_CreateInstance(kWidgetCID, &rv);
      nsWidgetInitData initData;
      initData.mWindowType = eWindowType_plugin;
      initData.mUnicode = false;
      initData.clipChildren = true;
      initData.clipSiblings = true;
      rv = mWidget->Create(parentWidget.get(), nullptr, nsIntRect(0,0,0,0),
                           nullptr, &initData);
      if (NS_FAILED(rv)) {
        mWidget->Destroy();
        mWidget = nullptr;
        return rv;
      }
    }


    mWidget->EnableDragDrop(true);
    mWidget->Show(false);
    mWidget->Enable(false);
  }

  if (mPluginFrame) {
    
    mPluginFrame->PrepForDrawing(mWidget);
  }

  if (windowless) {
    mPluginWindow->type = NPWindowTypeDrawable;

    
    
    
    
    mPluginWindow->window = nullptr;
#ifdef MOZ_X11
    
    NPSetWindowCallbackStruct* ws_info =
    static_cast<NPSetWindowCallbackStruct*>(mPluginWindow->ws_info);
    ws_info->display = DefaultXDisplay();

    nsAutoCString description;
    GetPluginDescription(description);
    NS_NAMED_LITERAL_CSTRING(flash10Head, "Shockwave Flash 10.");
    mFlash10Quirks = StringBeginsWith(description, flash10Head);
#endif
  } else if (mWidget) {
    
    
    mPluginWindow->type = NPWindowTypeWindow;
    mPluginWindow->window = GetPluginPort();
    
    mPluginWindow->SetPluginWidget(mWidget);

    
    nsCOMPtr<nsIPluginWidget> pluginWidget = do_QueryInterface(mWidget);
    if (pluginWidget) {
      pluginWidget->SetPluginInstanceOwner(this);
    }
  }

#ifdef XP_MACOSX
  if (GetDrawingModel() == NPDrawingModelCoreAnimation) {
    AddToCARefreshTimer();
  }
#endif

  mWidgetCreationComplete = true;

  return NS_OK;
}


#ifdef XP_MACOSX

void nsPluginInstanceOwner::FixUpPluginWindow(int32_t inPaintState)
{
  if (!mPluginWindow || !mInstance || !mPluginFrame) {
    return;
  }

  
  
  if (mInCGPaintLevel < 1) {
    SetPluginPort();
  }

  nsIntSize widgetClip = mPluginFrame->GetWidgetlessClipRect().Size();

  mPluginWindow->x = 0;
  mPluginWindow->y = 0;

  NPRect oldClipRect = mPluginWindow->clipRect;

  
  mPluginWindow->clipRect.top  = 0;
  mPluginWindow->clipRect.left = 0;

  if (inPaintState == ePluginPaintDisable) {
    mPluginWindow->clipRect.bottom = mPluginWindow->clipRect.top;
    mPluginWindow->clipRect.right  = mPluginWindow->clipRect.left;
  }
  else if (XRE_GetProcessType() != GeckoProcessType_Default)
  {
    
    
    
    
    mPluginWindow->clipRect.bottom = mPluginWindow->clipRect.top + mPluginWindow->height;
    mPluginWindow->clipRect.right  = mPluginWindow->clipRect.left + mPluginWindow->width;
  }
  else if (inPaintState == ePluginPaintEnable)
  {
    mPluginWindow->clipRect.bottom = mPluginWindow->clipRect.top + widgetClip.height;
    mPluginWindow->clipRect.right  = mPluginWindow->clipRect.left + widgetClip.width;
  }

  
  
  if (mPluginWindow->clipRect.left    != oldClipRect.left   ||
      mPluginWindow->clipRect.top     != oldClipRect.top    ||
      mPluginWindow->clipRect.right   != oldClipRect.right  ||
      mPluginWindow->clipRect.bottom  != oldClipRect.bottom)
  {
    if (UseAsyncRendering()) {
      mInstance->AsyncSetWindow(mPluginWindow);
    }
    else {
      mPluginWindow->CallSetWindow(mInstance);
    }
  }

  
  
  if (!mSentInitialTopLevelWindowEvent) {
    
    mSentInitialTopLevelWindowEvent = true;

    bool isActive = WindowIsActive();
    SendWindowFocusChanged(isActive);
    mLastWindowIsActive = isActive;
  }
}

void
nsPluginInstanceOwner::WindowFocusMayHaveChanged()
{
  if (!mSentInitialTopLevelWindowEvent) {
    return;
  }

  bool isActive = WindowIsActive();
  if (isActive != mLastWindowIsActive) {
    SendWindowFocusChanged(isActive);
    mLastWindowIsActive = isActive;
  }
}

bool
nsPluginInstanceOwner::WindowIsActive()
{
  if (!mPluginFrame) {
    return false;
  }

  EventStates docState = mPluginFrame->GetContent()->OwnerDoc()->GetDocumentState();
  return !docState.HasState(NS_DOCUMENT_STATE_WINDOW_INACTIVE);
}

void
nsPluginInstanceOwner::SendWindowFocusChanged(bool aIsActive)
{
  if (!mInstance) {
    return;
  }

  NPCocoaEvent cocoaEvent;
  InitializeNPCocoaEvent(&cocoaEvent);
  cocoaEvent.type = NPCocoaEventWindowFocusChanged;
  cocoaEvent.data.focus.hasFocus = aIsActive;
  mInstance->HandleEvent(&cocoaEvent,
                         nullptr,
                         NS_PLUGIN_CALL_SAFE_TO_REENTER_GECKO);
}

void
nsPluginInstanceOwner::ResolutionMayHaveChanged()
{
  double scaleFactor = 1.0;
  GetContentsScaleFactor(&scaleFactor);
  if (scaleFactor != mLastScaleFactor) {
    ContentsScaleFactorChanged(scaleFactor);
    mLastScaleFactor = scaleFactor;
   }
}

void
nsPluginInstanceOwner::HidePluginWindow()
{
  if (!mPluginWindow || !mInstance) {
    return;
  }

  mPluginWindow->clipRect.bottom = mPluginWindow->clipRect.top;
  mPluginWindow->clipRect.right  = mPluginWindow->clipRect.left;
  mWidgetVisible = false;
  if (UseAsyncRendering()) {
    mInstance->AsyncSetWindow(mPluginWindow);
  } else {
    mInstance->SetWindow(mPluginWindow);
  }
}

#else 

void nsPluginInstanceOwner::UpdateWindowPositionAndClipRect(bool aSetWindow)
{
  if (!mPluginWindow)
    return;

  
  
  
  if (aSetWindow && !mWidget && mPluginWindowVisible && !UseAsyncRendering())
    return;

  const NPWindow oldWindow = *mPluginWindow;

  bool windowless = (mPluginWindow->type == NPWindowTypeDrawable);
  nsIntPoint origin = mPluginFrame->GetWindowOriginInPixels(windowless);

  mPluginWindow->x = origin.x;
  mPluginWindow->y = origin.y;

  mPluginWindow->clipRect.left = 0;
  mPluginWindow->clipRect.top = 0;

  if (mPluginWindowVisible && mPluginDocumentActiveState) {
    mPluginWindow->clipRect.right = mPluginWindow->width;
    mPluginWindow->clipRect.bottom = mPluginWindow->height;
  } else {
    mPluginWindow->clipRect.right = 0;
    mPluginWindow->clipRect.bottom = 0;
  }

  if (!aSetWindow)
    return;

  if (mPluginWindow->x               != oldWindow.x               ||
      mPluginWindow->y               != oldWindow.y               ||
      mPluginWindow->clipRect.left   != oldWindow.clipRect.left   ||
      mPluginWindow->clipRect.top    != oldWindow.clipRect.top    ||
      mPluginWindow->clipRect.right  != oldWindow.clipRect.right  ||
      mPluginWindow->clipRect.bottom != oldWindow.clipRect.bottom) {
    CallSetWindow();
  }
}

void
nsPluginInstanceOwner::UpdateWindowVisibility(bool aVisible)
{
  mPluginWindowVisible = aVisible;
  UpdateWindowPositionAndClipRect(true);
}
#endif 

void
nsPluginInstanceOwner::UpdateDocumentActiveState(bool aIsActive)
{
  mPluginDocumentActiveState = aIsActive;
#ifndef XP_MACOSX
  UpdateWindowPositionAndClipRect(true);

#ifdef MOZ_WIDGET_ANDROID
  if (mInstance) {
    if (!mPluginDocumentActiveState) {
      RemovePluginView();
    }

    mInstance->NotifyOnScreen(mPluginDocumentActiveState);

    
    
    
    
    
    
    mInstance->NotifyForeground(mPluginDocumentActiveState);
  }
#endif 

  
  
  
  
  
  if (mWidget && XRE_GetProcessType() == GeckoProcessType_Content) {
    mWidget->Show(aIsActive);
    mWidget->Enable(aIsActive);
  }
#endif 
}

NS_IMETHODIMP
nsPluginInstanceOwner::CallSetWindow()
{
  if (!mWidgetCreationComplete) {
    
    return NS_OK;
  }
  if (mPluginFrame) {
    mPluginFrame->CallSetWindow(false);
  } else if (mInstance) {
    if (UseAsyncRendering()) {
      mInstance->AsyncSetWindow(mPluginWindow);
    } else {
      mInstance->SetWindow(mPluginWindow);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPluginInstanceOwner::GetContentsScaleFactor(double *result)
{
  NS_ENSURE_ARG_POINTER(result);
  double scaleFactor = 1.0;
  
  
  
#if defined(XP_MACOSX)
  nsIPresShell* presShell = nsContentUtils::FindPresShellForDocument(mContent->OwnerDoc());
  if (presShell) {
    scaleFactor = double(nsPresContext::AppUnitsPerCSSPixel())/
      presShell->GetPresContext()->DeviceContext()->AppUnitsPerDevPixelAtUnitFullZoom();
  }
#endif
  *result = scaleFactor;
  return NS_OK;
}

void nsPluginInstanceOwner::SetFrame(nsPluginFrame *aFrame)
{
  
  if (mPluginFrame == aFrame) {
    return;
  }

  
  if (mPluginFrame) {
    if (mContent && mContent->OwnerDoc() && mContent->OwnerDoc()->GetWindow()) {
      nsCOMPtr<EventTarget> windowRoot = mContent->OwnerDoc()->GetWindow()->GetTopWindowRoot();
      if (windowRoot) {
        windowRoot->RemoveEventListener(NS_LITERAL_STRING("activate"),
                                              this, false);
        windowRoot->RemoveEventListener(NS_LITERAL_STRING("deactivate"),
                                              this, false);
        windowRoot->RemoveEventListener(NS_LITERAL_STRING("MozPerformDelayedBlur"),
                                              this, false);
      }
    }

    
    mPluginFrame->SetInstanceOwner(nullptr);
  }

  
  mPluginFrame = aFrame;

  
  if (mPluginFrame) {
    mPluginFrame->SetInstanceOwner(this);
    
    
    if (mWidgetCreationComplete) {
      mPluginFrame->PrepForDrawing(mWidget);
    }
    mPluginFrame->FixupWindow(mPluginFrame->GetContentRectRelativeToSelf().Size());
    mPluginFrame->InvalidateFrame();

    nsFocusManager* fm = nsFocusManager::GetFocusManager();
    const nsIContent* content = aFrame->GetContent();
    if (fm && content) {
      mContentFocused = (content == fm->GetFocusedContent());
    }

    
    if (mContent && mContent->OwnerDoc() && mContent->OwnerDoc()->GetWindow()) {
      nsCOMPtr<EventTarget> windowRoot = mContent->OwnerDoc()->GetWindow()->GetTopWindowRoot();
      if (windowRoot) {
        windowRoot->AddEventListener(NS_LITERAL_STRING("activate"),
                                           this, false, false);
        windowRoot->AddEventListener(NS_LITERAL_STRING("deactivate"),
                                           this, false, false);
        windowRoot->AddEventListener(NS_LITERAL_STRING("MozPerformDelayedBlur"),
                                           this, false, false);
      }
    }
  }
}

nsPluginFrame* nsPluginInstanceOwner::GetFrame()
{
  return mPluginFrame;
}

NS_IMETHODIMP nsPluginInstanceOwner::PrivateModeChanged(bool aEnabled)
{
  return mInstance ? mInstance->PrivateModeStateChanged(aEnabled) : NS_OK;
}

already_AddRefed<nsIURI> nsPluginInstanceOwner::GetBaseURI() const
{
  if (!mContent) {
    return nullptr;
  }
  return mContent->GetBaseURI();
}



nsPluginDOMContextMenuListener::nsPluginDOMContextMenuListener(nsIContent* aContent)
{
  aContent->AddEventListener(NS_LITERAL_STRING("contextmenu"), this, true);
}

nsPluginDOMContextMenuListener::~nsPluginDOMContextMenuListener()
{
}

NS_IMPL_ISUPPORTS(nsPluginDOMContextMenuListener,
                  nsIDOMEventListener)

NS_IMETHODIMP
nsPluginDOMContextMenuListener::HandleEvent(nsIDOMEvent* aEvent)
{
  aEvent->PreventDefault(); 

  return NS_OK;
}

void nsPluginDOMContextMenuListener::Destroy(nsIContent* aContent)
{
  
  aContent->RemoveEventListener(NS_LITERAL_STRING("contextmenu"), this, true);
}
