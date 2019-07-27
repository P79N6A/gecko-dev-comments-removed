





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
#include "nsObjectFrame.h"
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
#include "nsFrameSelection.h"

#include "nsContentCID.h"
#include "nsWidgetsCID.h"
static NS_DEFINE_CID(kWidgetCID, NS_CHILD_CID);
static NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);

#ifdef XP_WIN
#include <wtypes.h>
#include <winuser.h>
#endif

#ifdef XP_MACOSX
#include <Carbon/Carbon.h>
#include "nsPluginUtilsOSX.h"
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
  data.mInverted = instance->Inverted();

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

  nsSurfaceTexture* surfTex = instance->AsSurfaceTexture();
  if (!surfTex) {
    return;
  }

  nsRefPtr<Image> img = container->CreateImage(ImageFormat::SURFACE_TEXTURE);

  SurfaceTextureImage::Data data;
  data.mSurfTex = surfTex;
  data.mSize = gfx::IntSize(rect.width, rect.height);
  data.mInverted = instance->Inverted();

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

  
  
  gfxSize resolution = mObjectFrame->PresContext()->PresShell()->GetCumulativeResolution();
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

  mObjectFrame = nullptr;
  mContent = nullptr;
  mWidgetCreationComplete = false;
#ifdef XP_MACOSX
  memset(&mCGPluginPortCopy, 0, sizeof(NP_CGContext));
  mInCGPaintLevel = 0;
  mSentInitialTopLevelWindowEvent = false;
  mColorProfile = nullptr;
  mPluginPortChanged = false;
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

  mObjectFrame = nullptr;

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

  if (!mObjectFrame) {
    return rv;
  }
  nsCOMPtr<nsIDocShellTreeItem> docShellItem = mObjectFrame->PresContext()->GetDocShell();
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
  
  
  if (mWaitingForPaint && (!mObjectFrame || IsUpToDate())) {
    
    
    nsCOMPtr<nsIRunnable> event = new AsyncPaintWaitEvent(mContent, true);
    NS_DispatchToMainThread(event);
    mWaitingForPaint = false;
  }

  if (!mObjectFrame || !invalidRect || !mWidgetVisible)
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
  mObjectFrame->InvalidateLayer(nsDisplayItem::TYPE_PLUGIN, &rect);
  return NS_OK;
}

NS_IMETHODIMP nsPluginInstanceOwner::InvalidateRegion(NPRegion invalidRegion)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsPluginInstanceOwner::RedrawPlugin()
{
  if (mObjectFrame) {
    mObjectFrame->InvalidateLayer(nsDisplayItem::TYPE_PLUGIN);
  }
  return NS_OK;
}

NS_IMETHODIMP nsPluginInstanceOwner::GetNetscapeWindow(void *value)
{
  if (!mObjectFrame) {
    NS_WARNING("plugin owner has no owner in getting doc's window handle");
    return NS_ERROR_FAILURE;
  }

#if defined(XP_WIN)
  void** pvalue = (void**)value;
  nsViewManager* vm = mObjectFrame->PresContext()->GetPresShell()->GetViewManager();
  if (!vm)
    return NS_ERROR_FAILURE;
#if defined(XP_WIN)
  
  
  

  
  

  
  

  
  
  
  

  
  
  
  if (mPluginWindow && mPluginWindow->type == NPWindowTypeDrawable) {
    
    
    
    
    
    
    
    

    nsIWidget* win = mObjectFrame->GetNearestWidget();
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
#endif
  
  nsCOMPtr<nsIWidget> widget;
  vm->GetRootWidget(getter_AddRefs(widget));
  if (widget) {
    *pvalue = (void*)widget->GetNativeData(NS_NATIVE_WINDOW);
  } else {
    NS_ASSERTION(widget, "couldn't get doc's widget in getting doc's window handle");
  }

  return NS_OK;
#elif (defined(MOZ_WIDGET_GTK) || defined(MOZ_WIDGET_QT)) && defined(MOZ_X11)
  
  nsIWidget* win = mObjectFrame->GetNearestWidget();
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
  if (!menu || !event)
    return NPERR_GENERIC_ERROR;

#ifdef XP_MACOSX
  if (GetEventModel() != NPEventModelCocoa)
    return NPERR_INCOMPATIBLE_VERSION_ERROR;

  return NS_NPAPI_ShowCocoaContextMenu(static_cast<void*>(menu), mWidget,
                                       static_cast<NPCocoaEvent*>(event));
#else
  return NPERR_INCOMPATIBLE_VERSION_ERROR;
#endif
}

NPBool nsPluginInstanceOwner::ConvertPoint(double sourceX, double sourceY, NPCoordinateSpace sourceSpace,
                                           double *destX, double *destY, NPCoordinateSpace destSpace)
{
#ifdef XP_MACOSX
  if (!mWidget)
    return false;

  return NS_NPAPI_ConvertPointCocoa(mWidget->GetNativeData(NS_NATIVE_WIDGET),
                                    sourceX, sourceY, sourceSpace, destX, destY, destSpace);
#else
  
  return false;
#endif
}

NPError nsPluginInstanceOwner::InitAsyncSurface(NPSize *size, NPImageFormat format,
                                                void *initData, NPAsyncSurface *surface)
{
  return NPERR_INCOMPATIBLE_VERSION_ERROR;
}

NPError nsPluginInstanceOwner::FinalizeAsyncSurface(NPAsyncSurface *)
{
  return NPERR_INCOMPATIBLE_VERSION_ERROR;
}

void nsPluginInstanceOwner::SetCurrentAsyncSurface(NPAsyncSurface *, NPRect*)
{
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













void* nsPluginInstanceOwner::SetPluginPortAndDetectChange()
{
  if (!mPluginWindow)
    return nullptr;
  void* pluginPort = GetPluginPortFromWidget();
  if (!pluginPort)
    return nullptr;
  mPluginWindow->window = pluginPort;

  return mPluginWindow->window;
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
  
  nsRect bounds = mObjectFrame->GetContentRectRelativeToSelf() + GetOffsetRootContent(mObjectFrame);
  LayoutDeviceIntRect rect = LayoutDeviceIntRect::FromAppUnitsToNearest(bounds, mObjectFrame->PresContext()->AppUnitsPerDevPixel());
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

  mozilla::widget::android::GeckoAppShell::RemovePluginView((jobject)mJavaView, mFullScreen);
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

  
  
  data.mInverted = AndroidBridge::Bridge()->IsHoneycomb() ? true : mInstance->Inverted();
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

  
  
  if (mObjectFrame && mPluginWindow &&
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
    
    
    
    
    
    
    WidgetMouseEvent* mouseEvent = aEvent->GetInternalNSEvent()->AsMouseEvent();
    if (mouseEvent &&
        static_cast<int>(mouseEvent->button) != mLastMouseDownButtonType) {
      aEvent->PreventDefault();
      return NS_OK;
    }
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

nsEventStatus nsPluginInstanceOwner::ProcessEvent(const WidgetGUIEvent& anEvent)
{
  nsEventStatus rv = nsEventStatus_eIgnore;

  if (!mInstance || !mObjectFrame)   
    return nsEventStatus_eIgnore;

#ifdef XP_MACOSX
  if (!mWidget)
    return nsEventStatus_eIgnore;

  
  if (anEvent.message == NS_MOUSE_ENTER_SYNTH)
    return nsEventStatus_eIgnore;

  nsCOMPtr<nsIPluginWidget> pluginWidget = do_QueryInterface(mWidget);
  if (!pluginWidget || NS_FAILED(pluginWidget->StartDrawPlugin()))
    return nsEventStatus_eIgnore;

  NPEventModel eventModel = GetEventModel();

  
  NPCocoaEvent synthCocoaEvent;
  const NPCocoaEvent* event = static_cast<const NPCocoaEvent*>(anEvent.mPluginEvent);
  nsPoint pt =
  nsLayoutUtils::GetEventCoordinatesRelativeTo(&anEvent, mObjectFrame) -
  mObjectFrame->GetContentRectRelativeToSelf().TopLeft();
  nsPresContext* presContext = mObjectFrame->PresContext();
  
  
  double scaleFactor = 1.0;
  GetContentsScaleFactor(&scaleFactor);
  size_t intScaleFactor = ceil(scaleFactor);
  nsIntPoint ptPx(presContext->AppUnitsToDevPixels(pt.x) / intScaleFactor,
                  presContext->AppUnitsToDevPixels(pt.y) / intScaleFactor);

  if (!event) {
    InitializeNPCocoaEvent(&synthCocoaEvent);
    switch (anEvent.message) {
      case NS_MOUSE_MOVE:
      {
        
        
        nsRefPtr<nsFrameSelection> frameselection = mObjectFrame->GetFrameSelection();
        if (!frameselection->GetDragState() ||
          (nsIPresShell::GetCapturingContent() == mObjectFrame->GetContent())) {
          synthCocoaEvent.type = NPCocoaEventMouseMoved;
          synthCocoaEvent.data.mouse.pluginX = static_cast<double>(ptPx.x);
          synthCocoaEvent.data.mouse.pluginY = static_cast<double>(ptPx.y);
          event = &synthCocoaEvent;
        }
      }
        break;
      case NS_MOUSE_BUTTON_DOWN:
        synthCocoaEvent.type = NPCocoaEventMouseDown;
        synthCocoaEvent.data.mouse.pluginX = static_cast<double>(ptPx.x);
        synthCocoaEvent.data.mouse.pluginY = static_cast<double>(ptPx.y);
        event = &synthCocoaEvent;
        break;
      case NS_MOUSE_BUTTON_UP:
        
        
        
        if (anEvent.AsMouseEvent()->button == WidgetMouseEvent::eLeftButton &&
            (nsIPresShell::GetCapturingContent() != mObjectFrame->GetContent())) {
          synthCocoaEvent.type = NPCocoaEventMouseEntered;
          synthCocoaEvent.data.mouse.pluginX = static_cast<double>(ptPx.x);
          synthCocoaEvent.data.mouse.pluginY = static_cast<double>(ptPx.y);
          event = &synthCocoaEvent;
        } else {
          synthCocoaEvent.type = NPCocoaEventMouseUp;
          synthCocoaEvent.data.mouse.pluginX = static_cast<double>(ptPx.x);
          synthCocoaEvent.data.mouse.pluginY = static_cast<double>(ptPx.y);
          event = &synthCocoaEvent;
        }
        break;
      default:
        break;
    }

    
    if (!event) {
      pluginWidget->EndDrawPlugin();
      return nsEventStatus_eIgnore;
    }
  }

  int16_t response = kNPEventNotHandled;
  void* window = FixUpPluginWindow(ePluginPaintEnable);
  if (window || (eventModel == NPEventModelCocoa)) {
    mInstance->HandleEvent(const_cast<NPCocoaEvent*>(event),
                           &response,
                           NS_PLUGIN_CALL_SAFE_TO_REENTER_GECKO);
  }

  if (eventModel == NPEventModelCocoa && response == kNPEventStartIME) {
    pluginWidget->StartComplexTextInputForCurrentEvent();
  }

  if ((response == kNPEventHandled || response == kNPEventStartIME) &&
      !(anEvent.message == NS_MOUSE_BUTTON_DOWN &&
        anEvent.AsMouseEvent()->button == WidgetMouseEvent::eLeftButton &&
        !mContentFocused)) {
    rv = nsEventStatus_eConsumeNoDefault;
  }

  pluginWidget->EndDrawPlugin();
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
        nsLayoutUtils::GetEventCoordinatesRelativeTo(&anEvent, mObjectFrame) -
        mObjectFrame->GetContentRectRelativeToSelf().TopLeft();
      nsPresContext* presContext = mObjectFrame->PresContext();
      nsIntPoint ptPx(presContext->AppUnitsToDevPixels(pt.x),
                      presContext->AppUnitsToDevPixels(pt.y));
      nsIntPoint widgetPtPx = ptPx + mObjectFrame->GetWindowOriginInPixels(true);
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
    
    NS_WARNING("nsObjectFrame ProcessEvent: trying to send null event to plugin.");
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

        
        const nsPresContext* presContext = mObjectFrame->PresContext();
        nsPoint appPoint =
          nsLayoutUtils::GetEventCoordinatesRelativeTo(&anEvent, mObjectFrame) -
          mObjectFrame->GetContentRectRelativeToSelf().TopLeft();
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

        
        const nsPresContext* presContext = mObjectFrame->PresContext();
        nsPoint appPoint =
          nsLayoutUtils::GetEventCoordinatesRelativeTo(&anEvent, mObjectFrame) -
          mObjectFrame->GetContentRectRelativeToSelf().TopLeft();
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
  if (!mInstance || !mObjectFrame)
    return;

  gfxRect dirtyRectCopy = aDirtyRect;
  double scaleFactor = 1.0;
  GetContentsScaleFactor(&scaleFactor);
  if (scaleFactor != 1.0) {
    ::CGContextScaleCTM(cgContext, scaleFactor, scaleFactor);
    
    
    dirtyRectCopy.ScaleRoundOut(1.0 / scaleFactor);
  }

  nsCOMPtr<nsIPluginWidget> pluginWidget = do_QueryInterface(mWidget);
  if (pluginWidget && NS_SUCCEEDED(pluginWidget->StartDrawPlugin())) {
    DoCocoaEventDrawRect(dirtyRectCopy, cgContext);
    pluginWidget->EndDrawPlugin();
  }
}

void nsPluginInstanceOwner::DoCocoaEventDrawRect(const gfxRect& aDrawRect, CGContextRef cgContext)
{
  if (!mInstance || !mObjectFrame)
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
  if (!mInstance || !mObjectFrame)
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
  if (!mInstance || !mObjectFrame || !mPluginDocumentActiveState || mFullScreen)
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
  if (!mInstance || !mObjectFrame)
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
  nsObjectFrame* objFrame =  static_cast<nsObjectFrame*>(iObjFrame);
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

void* nsPluginInstanceOwner::GetPluginPortFromWidget()
{


  void* result = nullptr;
  if (mWidget) {
#ifdef XP_WIN
    if (mPluginWindow && (mPluginWindow->type == NPWindowTypeDrawable))
      result = mWidget->GetNativeData(NS_NATIVE_GRAPHIC);
    else
#endif
#ifdef XP_MACOSX
    if (GetDrawingModel() == NPDrawingModelCoreGraphics ||
        GetDrawingModel() == NPDrawingModelCoreAnimation ||
        GetDrawingModel() == NPDrawingModelInvalidatingCoreAnimation)
      result = mWidget->GetNativeData(NS_NATIVE_PLUGIN_PORT_CG);
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
  if (!windowless


#ifndef XP_MACOSX
      && !nsIWidget::UsePuppetWidgets()
#endif
      ) {
    
    
    nsCOMPtr<nsIWidget> parentWidget;
    nsIDocument *doc = nullptr;
    if (mContent) {
      doc = mContent->OwnerDoc();
      parentWidget = nsContentUtils::WidgetForDocument(doc);
    }

    mWidget = do_CreateInstance(kWidgetCID, &rv);
    if (NS_FAILED(rv)) {
      return rv;
    }

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

    mWidget->EnableDragDrop(true);
    mWidget->Show(false);
    mWidget->Enable(false);

#ifdef XP_MACOSX
    
    
    nsCOMPtr<nsIPluginWidget> pluginWidget = do_QueryInterface(mWidget);
    if (!pluginWidget) {
      return NS_ERROR_FAILURE;
    }
    pluginWidget->SetPluginEventModel(GetEventModel());
    pluginWidget->SetPluginDrawingModel(GetDrawingModel());

    if (GetDrawingModel() == NPDrawingModelCoreAnimation) {
      AddToCARefreshTimer();
    }
#endif
  }

  if (mObjectFrame) {
    
    mObjectFrame->PrepForDrawing(mWidget);
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
    mPluginWindow->window = GetPluginPortFromWidget();
    
    mPluginWindow->SetPluginWidget(mWidget);

    
    nsCOMPtr<nsIPluginWidget> pluginWidget = do_QueryInterface(mWidget);
    if (pluginWidget) {
      pluginWidget->SetPluginInstanceOwner(this);
    }
  }

  mWidgetCreationComplete = true;

  return NS_OK;
}


#ifdef XP_MACOSX

void* nsPluginInstanceOwner::FixUpPluginWindow(int32_t inPaintState)
{
  if (!mWidget || !mPluginWindow || !mInstance || !mObjectFrame)
    return nullptr;

  nsCOMPtr<nsIPluginWidget> pluginWidget = do_QueryInterface(mWidget);
  if (!pluginWidget)
    return nullptr;

  
  
  if (mInCGPaintLevel < 1) {
    SetPluginPortAndDetectChange();
  }

  
  nsIWidget* widget = mObjectFrame->GetNearestWidget();
  if (!widget)
    return nullptr;
  void *cocoaTopLevelWindow = widget->GetNativeData(NS_NATIVE_WINDOW);
  
  if (!cocoaTopLevelWindow && XRE_GetProcessType() == GeckoProcessType_Default) {
    return nullptr;
  }

  nsIntPoint pluginOrigin;
  nsIntRect widgetClip;
  bool widgetVisible;
  pluginWidget->GetPluginClipRect(widgetClip, pluginOrigin, widgetVisible);
  
  if (XRE_GetProcessType() != GeckoProcessType_Default) {
    widgetVisible = true;
  }
  mWidgetVisible = widgetVisible;

  

  
  
  
  
  
  nsIntPoint geckoScreenCoords = mWidget->WidgetToScreenOffset();

  nsRect windowRect;
  if (cocoaTopLevelWindow) {
    NS_NPAPI_CocoaWindowFrame(cocoaTopLevelWindow, windowRect);
  }

  double scaleFactor = 1.0;
  GetContentsScaleFactor(&scaleFactor);
  int intScaleFactor = ceil(scaleFactor);

  
  
  mPluginWindow->x = geckoScreenCoords.x/intScaleFactor - windowRect.x;
  mPluginWindow->y = geckoScreenCoords.y/intScaleFactor - windowRect.y;

  NPRect oldClipRect = mPluginWindow->clipRect;

  
  mPluginWindow->clipRect.top    = widgetClip.y;
  mPluginWindow->clipRect.left   = widgetClip.x;

  if (!mWidgetVisible || inPaintState == ePluginPaintDisable) {
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
      mPluginWindow->clipRect.bottom  != oldClipRect.bottom ||
      mPluginPortChanged)
  {
    if (UseAsyncRendering()) {
      mInstance->AsyncSetWindow(mPluginWindow);
    }
    else {
      mPluginWindow->CallSetWindow(mInstance);
    }
    mPluginPortChanged = false;
  }

  
  
  if (!mSentInitialTopLevelWindowEvent) {
    
    mSentInitialTopLevelWindowEvent = true;

    WidgetPluginEvent pluginEvent(true, NS_PLUGIN_FOCUS_EVENT, nullptr);
    NPCocoaEvent cocoaEvent;
    InitializeNPCocoaEvent(&cocoaEvent);
    cocoaEvent.type = NPCocoaEventWindowFocusChanged;
    cocoaEvent.data.focus.hasFocus = cocoaTopLevelWindow ? NS_NPAPI_CocoaWindowIsMain(cocoaTopLevelWindow) : true;
    pluginEvent.mPluginEvent.Copy(cocoaEvent);
    ProcessEvent(pluginEvent);
  }

  return nullptr;
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
  nsIntPoint origin = mObjectFrame->GetWindowOriginInPixels(windowless);

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

void
nsPluginInstanceOwner::UpdateDocumentActiveState(bool aIsActive)
{
  mPluginDocumentActiveState = aIsActive;
  UpdateWindowPositionAndClipRect(true);

#ifdef MOZ_WIDGET_ANDROID
  if (mInstance) {
    if (!mPluginDocumentActiveState)
      RemovePluginView();

    mInstance->NotifyOnScreen(mPluginDocumentActiveState);

    
    
    
    
    
    
    mInstance->NotifyForeground(mPluginDocumentActiveState);
  }
#endif
}
#endif 

NS_IMETHODIMP
nsPluginInstanceOwner::CallSetWindow()
{
  if (mObjectFrame) {
    mObjectFrame->CallSetWindow(false);
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
      presShell->GetPresContext()->DeviceContext()->UnscaledAppUnitsPerDevPixel();
  }
#endif
  *result = scaleFactor;
  return NS_OK;
}

void nsPluginInstanceOwner::SetFrame(nsObjectFrame *aFrame)
{
  
  if (mObjectFrame == aFrame) {
    return;
  }

  
  if (mObjectFrame) {
    
    mObjectFrame->SetInstanceOwner(nullptr);
  }

  
  mObjectFrame = aFrame;

  
  if (mObjectFrame) {
    mObjectFrame->SetInstanceOwner(this);
    
    
    if (mWidgetCreationComplete) {
      mObjectFrame->PrepForDrawing(mWidget);
    }
    mObjectFrame->FixupWindow(mObjectFrame->GetContentRectRelativeToSelf().Size());
    mObjectFrame->InvalidateFrame();

    nsFocusManager* fm = nsFocusManager::GetFocusManager();
    const nsIContent* content = aFrame->GetContent();
    if (fm && content) {
      mContentFocused = (content == fm->GetFocusedContent());
    }
  }
}

nsObjectFrame* nsPluginInstanceOwner::GetFrame()
{
  return mObjectFrame;
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
