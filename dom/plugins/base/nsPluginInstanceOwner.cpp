





#ifdef MOZ_WIDGET_QT
#include <QWidget>
#include <QKeyEvent>
#if defined(MOZ_X11)
#if defined(Q_WS_X11)
#include <QX11Info>
#else
#include "gfxQtPlatform.h"
#endif
#endif
#undef slots
#endif

#ifdef MOZ_X11
#include <cairo-xlib.h>
#include "gfxXlibSurface.h"

enum { XKeyPress = KeyPress };
#ifdef KeyPress
#undef KeyPress
#endif
#include "mozilla/X11Util.h"
using mozilla::DefaultXDisplay;
#endif

#include "nsPluginInstanceOwner.h"
#include "nsIRunnable.h"
#include "nsContentUtils.h"
#include "nsRect.h"
#include "nsSize.h"
#include "nsDisplayList.h"
#include "ImageLayers.h"
#include "nsIDOMEventTarget.h"
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
#include "nsIViewManager.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIDOMHTMLObjectElement.h"
#include "nsIAppShell.h"
#include "nsIDOMHTMLAppletElement.h"
#include "nsAttrName.h"
#include "nsIFocusManager.h"
#include "nsFocusManager.h"
#include "nsIDOMDragEvent.h"
#include "nsIScrollableFrame.h"
#include "nsIImageLoadingContent.h"
#include "nsIObjectLoadingContent.h"
#include "nsIDocShell.h"

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

#ifdef MOZ_WIDGET_GTK2
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#endif

#ifdef MOZ_WIDGET_ANDROID
#include "ANPBase.h"
#include "AndroidBridge.h"
#include "AndroidMediaLayer.h"
#include "nsWindow.h"

static nsPluginInstanceOwner* sFullScreenInstance = nsnull;

using namespace mozilla::dom;

#include <android/log.h>
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "GeckoPlugins" , ## args)
#endif

using namespace mozilla;




class nsPluginDOMContextMenuListener : public nsIDOMEventListener
{
public:
  nsPluginDOMContextMenuListener();
  virtual ~nsPluginDOMContextMenuListener();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMEVENTLISTENER

  nsresult Init(nsIContent* aContent);
  nsresult Destroy(nsIContent* aContent);
  
  nsEventStatus ProcessEvent(const nsGUIEvent& anEvent)
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

#ifdef XP_MACOSX
static void DrawPlugin(ImageContainer* aContainer, void* aPluginInstanceOwner)
{
  nsObjectFrame* frame = static_cast<nsPluginInstanceOwner*>(aPluginInstanceOwner)->GetFrame();
  if (frame) {
    frame->UpdateImageLayer(gfxRect(0,0,0,0));
  }
}

static void OnDestroyImage(void* aPluginInstanceOwner)
{
  nsPluginInstanceOwner* owner = static_cast<nsPluginInstanceOwner*>(aPluginInstanceOwner);
  NS_IF_RELEASE(owner);
}
#endif 

already_AddRefed<ImageContainer>
nsPluginInstanceOwner::GetImageContainer()
{
  if (mInstance) {
    nsRefPtr<ImageContainer> container;
    
    
    mInstance->GetImageContainer(getter_AddRefs(container));
    if (container) {
#ifdef XP_MACOSX
      AutoLockImage autoLock(container);
      Image* image = autoLock.GetImage();
      if (image && image->GetFormat() == Image::MAC_IO_SURFACE && mObjectFrame) {
        MacIOSurfaceImage *oglImage = static_cast<MacIOSurfaceImage*>(image);
        NS_ADDREF_THIS();
        oglImage->SetUpdateCallback(&DrawPlugin, this);
        oglImage->SetDestroyCallback(&OnDestroyImage);
      }
#endif
      return container.forget();
    }
  }
  return nsnull;
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
  return nsnull;
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

  bool useAsyncRendering;
  bool result = (mInstance &&
          NS_SUCCEEDED(mInstance->UseAsyncPainting(&useAsyncRendering)) &&
          useAsyncRendering
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
    mPluginWindow = nsnull;

  mObjectFrame = nsnull;
  mContent = nsnull;
  mTagText = nsnull;
  mWidgetCreationComplete = false;
#ifdef XP_MACOSX
  memset(&mCGPluginPortCopy, 0, sizeof(NP_CGContext));
#ifndef NP_NO_QUICKDRAW
  memset(&mQDPluginPortCopy, 0, sizeof(NP_Port));
#endif
  mInCGPaintLevel = 0;
  mSentInitialTopLevelWindowEvent = false;
  mColorProfile = nsnull;
  mPluginPortChanged = false;
#endif
  mContentFocused = false;
  mWidgetVisible = true;
  mPluginWindowVisible = false;
  mPluginDocumentActiveState = true;
  mNumCachedAttrs = 0;
  mNumCachedParams = 0;
  mCachedAttrParamNames = nsnull;
  mCachedAttrParamValues = nsnull;

#ifdef XP_MACOSX
#ifndef NP_NO_QUICKDRAW
  mEventModel = NPEventModelCarbon;
#else
  mEventModel = NPEventModelCocoa;
#endif
  mUseAsyncRendering = false;
#endif

  mWaitingForPaint = false;

#ifdef MOZ_WIDGET_ANDROID
  mInverted = false;
  mFullScreen = false;
  mLayer = nsnull;
  mJavaView = nsnull;
#endif
}

nsPluginInstanceOwner::~nsPluginInstanceOwner()
{
  PRInt32 cnt;

  if (mWaitingForPaint) {
    
    
    nsCOMPtr<nsIRunnable> event = new AsyncPaintWaitEvent(mContent, true);
    NS_DispatchToMainThread(event);
  }

#ifdef MAC_CARBON_PLUGINS
  CancelTimer();
#endif

  mObjectFrame = nsnull;

  for (cnt = 0; cnt < (mNumCachedAttrs + 1 + mNumCachedParams); cnt++) {
    if (mCachedAttrParamNames && mCachedAttrParamNames[cnt]) {
      NS_Free(mCachedAttrParamNames[cnt]);
      mCachedAttrParamNames[cnt] = nsnull;
    }

    if (mCachedAttrParamValues && mCachedAttrParamValues[cnt]) {
      NS_Free(mCachedAttrParamValues[cnt]);
      mCachedAttrParamValues[cnt] = nsnull;
    }
  }

  if (mCachedAttrParamNames) {
    NS_Free(mCachedAttrParamNames);
    mCachedAttrParamNames = nsnull;
  }

  if (mCachedAttrParamValues) {
    NS_Free(mCachedAttrParamValues);
    mCachedAttrParamValues = nsnull;
  }

  if (mTagText) {
    NS_Free(mTagText);
    mTagText = nsnull;
  }

  PLUG_DeletePluginNativeWindow(mPluginWindow);
  mPluginWindow = nsnull;

#ifdef MOZ_WIDGET_ANDROID
  RemovePluginView();
#endif

  if (mInstance) {
    mInstance->InvalidateOwner();
  }
}

NS_IMPL_ISUPPORTS5(nsPluginInstanceOwner,
                   nsIPluginInstanceOwner,
                   nsIPluginTagInfo,
                   nsIDOMEventListener,
                   nsIPrivacyTransitionObserver,
                   nsISupportsWeakReference)

nsresult
nsPluginInstanceOwner::SetInstance(nsNPAPIPluginInstance *aInstance)
{
  NS_ASSERTION(!mInstance || !aInstance, "mInstance should only be set or unset!");

  
  
  
  if (mInstance && !aInstance) {
    mInstance->InvalidateOwner();

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

NS_IMETHODIMP nsPluginInstanceOwner::GetMode(PRInt32 *aMode)
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

NS_IMETHODIMP nsPluginInstanceOwner::GetAttributes(PRUint16& n,
                                                   const char*const*& names,
                                                   const char*const*& values)
{
  nsresult rv = EnsureCachedAttrParamArrays();
  NS_ENSURE_SUCCESS(rv, rv);

  n = mNumCachedAttrs;
  names  = (const char **)mCachedAttrParamNames;
  values = (const char **)mCachedAttrParamValues;

  return rv;
}

NS_IMETHODIMP nsPluginInstanceOwner::GetAttribute(const char* name, const char* *result)
{
  NS_ENSURE_ARG_POINTER(name);
  NS_ENSURE_ARG_POINTER(result);
  
  nsresult rv = EnsureCachedAttrParamArrays();
  NS_ENSURE_SUCCESS(rv, rv);

  *result = nsnull;

  for (int i = 0; i < mNumCachedAttrs; i++) {
    if (0 == PL_strcasecmp(mCachedAttrParamNames[i], name)) {
      *result = mCachedAttrParamValues[i];
      return NS_OK;
    }
  }

  return NS_ERROR_FAILURE;
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
                                            PRUint32 aHeadersDataLen)
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

  
  nsCOMPtr<nsISupports> container = presContext->GetContainer();
  NS_ENSURE_TRUE(container,NS_ERROR_FAILURE);
  nsCOMPtr<nsILinkHandler> lh = do_QueryInterface(container);
  NS_ENSURE_TRUE(lh, NS_ERROR_FAILURE);

  nsAutoString  unitarget;
  unitarget.AssignASCII(aTarget); 

  nsCOMPtr<nsIURI> baseURI = mContent->GetBaseURI();

  
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

  PRInt32 blockPopups =
    Preferences::GetInt("privacy.popups.disable_from_plugins");
  nsAutoPopupStatePusher popupStatePusher((PopupControlState)blockPopups);

  rv = lh->OnLinkClick(mContent, uri, unitarget.get(), 
                       aPostStream, headersDataStream, true);

  return rv;
}

NS_IMETHODIMP nsPluginInstanceOwner::ShowStatus(const char *aStatusMsg)
{
  nsresult  rv = NS_ERROR_FAILURE;
  
  rv = this->ShowStatus(NS_ConvertUTF8toUTF16(aStatusMsg).get());
  
  return rv;
}

NS_IMETHODIMP nsPluginInstanceOwner::ShowStatus(const PRUnichar *aStatusMsg)
{
  nsresult  rv = NS_ERROR_FAILURE;

  if (!mObjectFrame) {
    return rv;
  }
  nsCOMPtr<nsISupports> cont = mObjectFrame->PresContext()->GetContainer();
  if (!cont) {
    return NS_OK;
  }

  nsCOMPtr<nsIDocShellTreeItem> docShellItem(do_QueryInterface(cont, &rv));
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

  
  
  
  nsRefPtr<ImageContainer> container;
  mInstance->GetImageContainer(getter_AddRefs(container));
  gfxIntSize oldSize(0, 0);

#ifndef XP_MACOSX
  
  
  if (mWidget) {
    mWidget->Invalidate(nsIntRect(invalidRect->left, invalidRect->top,
                                  invalidRect->right - invalidRect->left,
                                  invalidRect->bottom - invalidRect->top));
    return NS_OK;
  }
#endif

  nsPresContext* presContext = mObjectFrame->PresContext();
  nsRect rect(presContext->DevPixelsToAppUnits(invalidRect->left),
              presContext->DevPixelsToAppUnits(invalidRect->top),
              presContext->DevPixelsToAppUnits(invalidRect->right - invalidRect->left),
              presContext->DevPixelsToAppUnits(invalidRect->bottom - invalidRect->top));
  if (container) {
    gfxIntSize newSize = container->GetCurrentSize();
    if (newSize != oldSize) {
      
      nsRect oldRect = nsRect(0, 0,
                              presContext->DevPixelsToAppUnits(oldSize.width),
                              presContext->DevPixelsToAppUnits(oldSize.height));
      rect.UnionRect(rect, oldRect);
    }
  }
  rect.MoveBy(mObjectFrame->GetContentRectRelativeToSelf().TopLeft());
  mObjectFrame->InvalidateLayer(rect, nsDisplayItem::TYPE_PLUGIN);
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
    mObjectFrame->InvalidateLayer(mObjectFrame->GetContentRectRelativeToSelf(), nsDisplayItem::TYPE_PLUGIN);
  }
  return NS_OK;
}

NS_IMETHODIMP nsPluginInstanceOwner::GetNetscapeWindow(void *value)
{
  if (!mObjectFrame) {
    NS_WARNING("plugin owner has no owner in getting doc's window handle");
    return NS_ERROR_FAILURE;
  }
  
#if defined(XP_WIN) || defined(XP_OS2)
  void** pvalue = (void**)value;
  nsIViewManager* vm = mObjectFrame->PresContext()->GetPresShell()->GetViewManager();
  if (!vm)
    return NS_ERROR_FAILURE;
#if defined(XP_WIN)
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (mPluginWindow && mPluginWindow->type == NPWindowTypeDrawable) {
    
    
    
    
    
    
    
    
    
    nsIWidget* win = mObjectFrame->GetNearestWidget();
    if (win) {
      nsIView *view = nsIView::GetViewFor(win);
      NS_ASSERTION(view, "No view for widget");
      nsPoint offset = view->GetOffsetTo(nsnull);
      
      if (offset.x || offset.y) {
        
        
        *pvalue = (void*)win->GetNativeData(NS_NATIVE_WINDOW);
        if (*pvalue)
          return NS_OK;
      }
    }
  }
#endif
  
  nsCOMPtr<nsIWidget> widget;
  nsresult rv = vm->GetRootWidget(getter_AddRefs(widget));            
  if (widget) {
    *pvalue = (void*)widget->GetNativeData(NS_NATIVE_WINDOW);
  } else {
    NS_ASSERTION(widget, "couldn't get doc's widget in getting doc's window handle");
  }

  return rv;
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

NS_IMETHODIMP nsPluginInstanceOwner::SetEventModel(PRInt32 eventModel)
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

NS_IMETHODIMP nsPluginInstanceOwner::GetTagText(const char* *result)
{
  NS_ENSURE_ARG_POINTER(result);
  if (nsnull == mTagText) {
    nsresult rv;
    nsCOMPtr<nsIDOMNode> node(do_QueryInterface(mContent, &rv));
    if (NS_FAILED(rv))
      return rv;

    nsCOMPtr<nsIDocument> document;
    rv = GetDocument(getter_AddRefs(document));
    if (NS_FAILED(rv))
      return rv;

    nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(document);
    NS_ASSERTION(domDoc, "Need a document");

    nsCOMPtr<nsIDocumentEncoder> docEncoder(do_CreateInstance(NS_DOC_ENCODER_CONTRACTID_BASE "text/html", &rv));
    if (NS_FAILED(rv))
      return rv;
    rv = docEncoder->Init(domDoc, NS_LITERAL_STRING("text/html"), nsIDocumentEncoder::OutputEncodeBasicEntities);
    if (NS_FAILED(rv))
      return rv;

    nsRefPtr<nsRange> range = new nsRange();
    rv = range->SelectNode(node);
    if (NS_FAILED(rv))
      return rv;

    docEncoder->SetRange(range);
    nsString elementHTML;
    rv = docEncoder->EncodeToString(elementHTML);
    if (NS_FAILED(rv))
      return rv;

    mTagText = ToNewUTF8String(elementHTML);
    if (!mTagText)
      return NS_ERROR_OUT_OF_MEMORY;
  }
  *result = mTagText;
  return NS_OK;
}

NS_IMETHODIMP nsPluginInstanceOwner::GetParameters(PRUint16& n, const char*const*& names, const char*const*& values)
{
  nsresult rv = EnsureCachedAttrParamArrays();
  NS_ENSURE_SUCCESS(rv, rv);

  n = mNumCachedParams;
  if (n) {
    names  = (const char **)(mCachedAttrParamNames + mNumCachedAttrs + 1);
    values = (const char **)(mCachedAttrParamValues + mNumCachedAttrs + 1);
  } else
    names = values = nsnull;

  return rv;
}

NS_IMETHODIMP nsPluginInstanceOwner::GetParameter(const char* name, const char* *result)
{
  NS_ENSURE_ARG_POINTER(name);
  NS_ENSURE_ARG_POINTER(result);
  
  nsresult rv = EnsureCachedAttrParamArrays();
  NS_ENSURE_SUCCESS(rv, rv);

  *result = nsnull;

  for (int i = mNumCachedAttrs + 1; i < (mNumCachedParams + 1 + mNumCachedAttrs); i++) {
    if (0 == PL_strcasecmp(mCachedAttrParamNames[i], name)) {
      *result = mCachedAttrParamValues[i];
      return NS_OK;
    }
  }

  return NS_ERROR_FAILURE;
}
  
NS_IMETHODIMP nsPluginInstanceOwner::GetDocumentBase(const char* *result)
{
  NS_ENSURE_ARG_POINTER(result);
  nsresult rv = NS_OK;
  if (mDocumentBase.IsEmpty()) {
    if (!mObjectFrame) {
      *result = nsnull;
      return NS_ERROR_FAILURE;
    }

    nsIDocument* doc = mContent->OwnerDoc();
    NS_ASSERTION(doc, "Must have an owner doc");
    rv = doc->GetDocBaseURI()->GetSpec(mDocumentBase);
  }
  if (NS_SUCCEEDED(rv))
    *result = ToNewCString(mDocumentBase);
  return rv;
}

static nsDataHashtable<nsDepCharHashKey, const char *> * gCharsetMap;
typedef struct {
    char mozName[16];
    char javaName[12];
} moz2javaCharset;








static const moz2javaCharset charsets[] = 
{
    {"windows-1252",    "Cp1252"},
    {"IBM850",          "Cp850"},
    {"IBM852",          "Cp852"},
    {"IBM855",          "Cp855"},
    {"IBM857",          "Cp857"},
    {"IBM828",          "Cp862"},
    {"IBM864",          "Cp864"},
    {"IBM866",          "Cp866"},
    {"windows-1250",    "Cp1250"},
    {"windows-1251",    "Cp1251"},
    {"windows-1253",    "Cp1253"},
    {"windows-1254",    "Cp1254"},
    {"windows-1255",    "Cp1255"},
    {"windows-1256",    "Cp1256"},
    {"windows-1257",    "Cp1257"},
    {"windows-1258",    "Cp1258"},
    {"EUC-JP",          "EUC_JP"},
    {"EUC-KR",          "EUC_KR"},
    {"x-euc-tw",        "EUC_TW"},
    {"gb18030",         "GB18030"},
    {"gbk",             "GBK"},
    {"ISO-2022-JP",     "ISO2022JP"},
    {"ISO-2022-KR",     "ISO2022KR"},
    {"ISO-8859-2",      "ISO8859_2"},
    {"ISO-8859-3",      "ISO8859_3"},
    {"ISO-8859-4",      "ISO8859_4"},
    {"ISO-8859-5",      "ISO8859_5"},
    {"ISO-8859-6",      "ISO8859_6"},
    {"ISO-8859-7",      "ISO8859_7"},
    {"ISO-8859-8",      "ISO8859_8"},
    {"ISO-8859-9",      "ISO8859_9"},
    {"ISO-8859-13",     "ISO8859_13"},
    {"x-johab",         "Johab"},
    {"KOI8-R",          "KOI8_R"},
    {"TIS-620",         "MS874"},
    {"x-windows-949",   "MS949"},
    {"x-mac-arabic",    "MacArabic"},
    {"x-mac-croatian",  "MacCroatia"},
    {"x-mac-cyrillic",  "MacCyrillic"},
    {"x-mac-greek",     "MacGreek"},
    {"x-mac-hebrew",    "MacHebrew"},
    {"x-mac-icelandic", "MacIceland"},
    {"x-mac-roman",     "MacRoman"},
    {"x-mac-romanian",  "MacRomania"},
    {"x-mac-ukrainian", "MacUkraine"},
    {"Shift_JIS",       "SJIS"},
    {"TIS-620",         "TIS620"}
};

NS_IMETHODIMP nsPluginInstanceOwner::GetDocumentEncoding(const char* *result)
{
  NS_ENSURE_ARG_POINTER(result);
  *result = nsnull;

  nsresult rv;
  
  nsCOMPtr<nsIDocument> doc;
  rv = GetDocument(getter_AddRefs(doc));
  NS_ASSERTION(NS_SUCCEEDED(rv), "failed to get document");
  if (NS_FAILED(rv))
    return rv;

  const nsCString &charset = doc->GetDocumentCharacterSet();

  if (charset.IsEmpty())
    return NS_OK;

  
  if (charset.EqualsLiteral("us-ascii")) {
    *result = PL_strdup("US_ASCII");
  } else if (charset.EqualsLiteral("ISO-8859-1") ||
      !nsCRT::strncmp(PromiseFlatCString(charset).get(), "UTF", 3)) {
    *result = ToNewCString(charset);
  } else {
    if (!gCharsetMap) {
      const int NUM_CHARSETS = sizeof(charsets) / sizeof(moz2javaCharset);
      gCharsetMap = new nsDataHashtable<nsDepCharHashKey, const char*>();
      if (!gCharsetMap)
        return NS_ERROR_OUT_OF_MEMORY;
      gCharsetMap->Init(NUM_CHARSETS);
      for (PRUint16 i = 0; i < NUM_CHARSETS; i++) {
        gCharsetMap->Put(charsets[i].mozName, charsets[i].javaName);
      }
    }
    
    const char *mapping;
    *result = gCharsetMap->Get(charset.get(), &mapping) ? PL_strdup(mapping) :
                                                          ToNewCString(charset);
  }

  return (*result) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP nsPluginInstanceOwner::GetAlignment(const char* *result)
{
  return GetAttribute("ALIGN", result);
}
  
NS_IMETHODIMP nsPluginInstanceOwner::GetWidth(PRUint32 *result)
{
  NS_ENSURE_ARG_POINTER(result);

  NS_ENSURE_TRUE(mPluginWindow, NS_ERROR_NULL_POINTER);

  *result = mPluginWindow->width;

  return NS_OK;
}
  
NS_IMETHODIMP nsPluginInstanceOwner::GetHeight(PRUint32 *result)
{
  NS_ENSURE_ARG_POINTER(result);

  NS_ENSURE_TRUE(mPluginWindow, NS_ERROR_NULL_POINTER);

  *result = mPluginWindow->height;

  return NS_OK;
}

  
NS_IMETHODIMP nsPluginInstanceOwner::GetBorderVertSpace(PRUint32 *result)
{
  nsresult    rv;
  const char  *vspace;

  rv = GetAttribute("VSPACE", &vspace);

  if (NS_OK == rv) {
    if (*result != 0)
      *result = (PRUint32)atol(vspace);
    else
      *result = 0;
  }
  else
    *result = 0;

  return rv;
}
  
NS_IMETHODIMP nsPluginInstanceOwner::GetBorderHorizSpace(PRUint32 *result)
{
  nsresult    rv;
  const char  *hspace;

  rv = GetAttribute("HSPACE", &hspace);

  if (NS_OK == rv) {
    if (*result != 0)
      *result = (PRUint32)atol(hspace);
    else
      *result = 0;
  }
  else
    *result = 0;

  return rv;
}






nsresult nsPluginInstanceOwner::EnsureCachedAttrParamArrays()
{
  if (mCachedAttrParamValues)
    return NS_OK;

  NS_PRECONDITION(((mNumCachedAttrs + mNumCachedParams) == 0) &&
                    !mCachedAttrParamNames,
                  "re-cache of attrs/params not implemented! use the DOM "
                    "node directy instead");

  
  
  PRUint32 cattrs = mContent->GetAttrCount();
  if (cattrs < 0x0000FFFD) {
    mNumCachedAttrs = static_cast<PRUint16>(cattrs);
  } else {
    mNumCachedAttrs = 0xFFFD;
  }

  
  
  
  
  
  
  
  
  nsCOMArray<nsIDOMElement> ourParams;

  
  nsCOMPtr<nsIDOMElement> mydomElement = do_QueryInterface(mContent);
  NS_ENSURE_TRUE(mydomElement, NS_ERROR_NO_INTERFACE);

  
  nsCOMPtr<nsIPluginInstanceOwner> kungFuDeathGrip(this);

  nsCOMPtr<nsIDOMNodeList> allParams;
  NS_NAMED_LITERAL_STRING(xhtml_ns, "http://www.w3.org/1999/xhtml");
  mydomElement->GetElementsByTagNameNS(xhtml_ns, NS_LITERAL_STRING("param"),
                                       getter_AddRefs(allParams));
  if (allParams) {
    PRUint32 numAllParams; 
    allParams->GetLength(&numAllParams);
    for (PRUint32 i = 0; i < numAllParams; i++) {
      nsCOMPtr<nsIDOMNode> pnode;
      allParams->Item(i, getter_AddRefs(pnode));
      nsCOMPtr<nsIDOMElement> domelement = do_QueryInterface(pnode);
      if (domelement) {
        
        nsAutoString name;
        domelement->GetAttribute(NS_LITERAL_STRING("name"), name);
        if (!name.IsEmpty()) {
          
          nsCOMPtr<nsIDOMNode> parent;
          nsCOMPtr<nsIDOMHTMLObjectElement> domobject;
          nsCOMPtr<nsIDOMHTMLAppletElement> domapplet;
          pnode->GetParentNode(getter_AddRefs(parent));
          while (!(domobject || domapplet) && parent) {
            domobject = do_QueryInterface(parent);
            domapplet = do_QueryInterface(parent);
            nsCOMPtr<nsIDOMNode> temp;
            parent->GetParentNode(getter_AddRefs(temp));
            parent = temp;
          }
          if (domapplet || domobject) {
            if (domapplet) {
              parent = domapplet;
            }
            else {
              parent = domobject;
            }
            nsCOMPtr<nsIDOMNode> mydomNode = do_QueryInterface(mydomElement);
            if (parent == mydomNode) {
              ourParams.AppendObject(domelement);
            }
          }
        }
      }
    }
  }

  
  PRUint32 cparams = ourParams.Count();
  if (cparams < 0x0000FFFF) {
    mNumCachedParams = static_cast<PRUint16>(cparams);
  } else {
    mNumCachedParams = 0xFFFF;
  }

  PRUint16 numRealAttrs = mNumCachedAttrs;

  
  
  
  
  
  nsAutoString data;
  if (mContent->Tag() == nsGkAtoms::object &&
      !mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::src) &&
      mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::data, data) &&
      !data.IsEmpty()) {
    mNumCachedAttrs++;
  }

  
  
  nsAdoptingCString wmodeType = Preferences::GetCString("plugins.force.wmode");
  if (!wmodeType.IsEmpty()) {
    mNumCachedAttrs++;
  }

  
  
  
  
  
  
  
  const char* mime = nsnull;
  bool addCodebase = false;
  nsCAutoString codebaseSpec;
  if (mInstance && NS_SUCCEEDED(mInstance->GetMIMEType(&mime)) && mime &&
      strcmp(mime, "application/x-java-vm") == 0) {
    addCodebase = true;
    nsCOMPtr<nsIObjectLoadingContent> objlContent = do_QueryInterface(mContent);
    nsCOMPtr<nsIURI> codebaseURI;
    objlContent->GetObjectBaseURI(nsCString(mime), getter_AddRefs(codebaseURI));
    nsresult rv = codebaseURI->GetSpec(codebaseSpec);
    NS_ENSURE_SUCCESS(rv, rv);

    
    if (!mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::codebase)) {
      mNumCachedAttrs++;
    }
  }

  mCachedAttrParamNames  = (char**)NS_Alloc(sizeof(char*) * (mNumCachedAttrs + 1 + mNumCachedParams));
  NS_ENSURE_TRUE(mCachedAttrParamNames,  NS_ERROR_OUT_OF_MEMORY);
  mCachedAttrParamValues = (char**)NS_Alloc(sizeof(char*) * (mNumCachedAttrs + 1 + mNumCachedParams));
  NS_ENSURE_TRUE(mCachedAttrParamValues, NS_ERROR_OUT_OF_MEMORY);

  
  
  
  
  
  
  PRInt32 start, end, increment;
  if (mContent->IsHTML() &&
      mContent->IsInHTMLDocument()) {
    
    start = numRealAttrs - 1;
    end = -1;
    increment = -1;
  } else {
    
    start = 0;
    end = numRealAttrs;
    increment = 1;
  }

  
  PRUint32 nextAttrParamIndex = 0;

  
  
  bool wmodeSet = false;

  
  for (PRInt32 index = start; index != end; index += increment) {
    const nsAttrName* attrName = mContent->GetAttrNameAt(index);
    nsIAtom* atom = attrName->LocalName();
    nsAutoString value;
    mContent->GetAttr(attrName->NamespaceID(), atom, value);
    nsAutoString name;
    atom->ToString(name);

    FixUpURLS(name, value);

    mCachedAttrParamNames [nextAttrParamIndex] = ToNewUTF8String(name);
    if (!wmodeType.IsEmpty() && 
        0 == PL_strcasecmp(mCachedAttrParamNames[nextAttrParamIndex], "wmode")) {
      mCachedAttrParamValues[nextAttrParamIndex] = ToNewUTF8String(NS_ConvertUTF8toUTF16(wmodeType));

      if (!wmodeSet) {
        
        mNumCachedAttrs--;
        wmodeSet = true;
      }
    } else if (addCodebase && 0 == PL_strcasecmp(mCachedAttrParamNames[nextAttrParamIndex], "codebase")) {
      mCachedAttrParamValues[nextAttrParamIndex] = ToNewUTF8String(NS_ConvertUTF8toUTF16(codebaseSpec));
      addCodebase = false;
    } else {
      mCachedAttrParamValues[nextAttrParamIndex] = ToNewUTF8String(value);
    }
    nextAttrParamIndex++;
  }

  
  if (addCodebase) {
    mCachedAttrParamNames [nextAttrParamIndex] = ToNewUTF8String(NS_LITERAL_STRING("codebase"));
    mCachedAttrParamValues[nextAttrParamIndex] = ToNewUTF8String(NS_ConvertUTF8toUTF16(codebaseSpec));
    nextAttrParamIndex++;
  }

  
  if (!wmodeType.IsEmpty() && !wmodeSet) {
    mCachedAttrParamNames [nextAttrParamIndex] = ToNewUTF8String(NS_LITERAL_STRING("wmode"));
    mCachedAttrParamValues[nextAttrParamIndex] = ToNewUTF8String(NS_ConvertUTF8toUTF16(wmodeType));
    nextAttrParamIndex++;
  }

  
  if (!data.IsEmpty()) {
    mCachedAttrParamNames [nextAttrParamIndex] = ToNewUTF8String(NS_LITERAL_STRING("SRC"));
    mCachedAttrParamValues[nextAttrParamIndex] = ToNewUTF8String(data);
    nextAttrParamIndex++;
  }

  
  mCachedAttrParamNames [nextAttrParamIndex] = ToNewUTF8String(NS_LITERAL_STRING("PARAM"));
#ifdef MOZ_WIDGET_ANDROID
  
  mCachedAttrParamValues[nextAttrParamIndex] = ToNewUTF8String(NS_LITERAL_STRING(""));
#else
  mCachedAttrParamValues[nextAttrParamIndex] = nsnull;
#endif
  nextAttrParamIndex++;

  
  for (PRUint16 i = 0; i < mNumCachedParams; i++) {
    nsIDOMElement* param = ourParams.ObjectAt(i);
    if (!param) {
      continue;
    }

    nsAutoString name;
    nsAutoString value;
    param->GetAttribute(NS_LITERAL_STRING("name"), name); 
    param->GetAttribute(NS_LITERAL_STRING("value"), value);
    
    FixUpURLS(name, value);

    









    name.Trim(" \n\r\t\b", true, true, false);
    value.Trim(" \n\r\t\b", true, true, false);
    mCachedAttrParamNames [nextAttrParamIndex] = ToNewUTF8String(name);
    mCachedAttrParamValues[nextAttrParamIndex] = ToNewUTF8String(value);
    nextAttrParamIndex++;
  }

  return NS_OK;
}

#ifdef XP_MACOSX

#ifndef NP_NO_CARBON
static void InitializeEventRecord(EventRecord* event, ::Point* aMousePosition)
{
  memset(event, 0, sizeof(EventRecord));
  if (aMousePosition) {
    event->where = *aMousePosition;
  } else {
    ::GetGlobalMouse(&event->where);
  }
  event->when = ::TickCount();
  event->modifiers = ::GetCurrentKeyModifiers();
}
#endif

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

  mInstance->GetDrawingModel((PRInt32*)&drawingModel);
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

NPEventModel nsPluginInstanceOwner::GetEventModel()
{
  return mEventModel;
}

#define DEFAULT_REFRESH_RATE 20 // 50 FPS

nsCOMPtr<nsITimer>                *nsPluginInstanceOwner::sCATimer = NULL;
nsTArray<nsPluginInstanceOwner*>  *nsPluginInstanceOwner::sCARefreshListeners = NULL;

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

  
  const char* mime = nsnull;
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
    (*sCATimer)->InitWithFuncCallback(CARefresh, NULL, 
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
      sCATimer = NULL;
    }
    delete sCARefreshListeners;
    sCARefreshListeners = NULL;
  }
}

void nsPluginInstanceOwner::RenderCoreAnimation(CGContextRef aCGContext,
                                                int aWidth, int aHeight)
{
  if (aWidth == 0 || aHeight == 0)
    return;

  if (!mIOSurface ||
      (mIOSurface->GetWidth() != (size_t)aWidth ||
       mIOSurface->GetHeight() != (size_t)aHeight)) {
    mIOSurface = nsnull;

    
    mIOSurface = nsIOSurface::CreateIOSurface(aWidth, aHeight);
    if (mIOSurface) {
      nsRefPtr<nsIOSurface> attachSurface = nsIOSurface::LookupSurface(
                                              mIOSurface->GetIOSurfaceID());
      if (attachSurface) {
        mCARenderer.AttachIOSurface(attachSurface);
      } else {
        NS_ERROR("IOSurface attachment failed");
        mIOSurface = nsnull;
      }
    }
  }

  if (!mColorProfile) {
    mColorProfile = CreateSystemColorSpace();
  }

  if (mCARenderer.isInit() == false) {
    void *caLayer = NULL;
    nsresult rv = mInstance->GetValueFromPlugin(NPPVpluginCoreAnimationLayer, &caLayer);
    if (NS_FAILED(rv) || !caLayer) {
      return;
    }

    
    
    mCARenderer.SetupRenderer(caLayer, aWidth, aHeight, DISALLOW_OFFLINE_RENDERER);

    
    
    FixUpPluginWindow(ePluginPaintDisable);
    FixUpPluginWindow(ePluginPaintEnable);
  }

  CGImageRef caImage = NULL;
  nsresult rt = mCARenderer.Render(aWidth, aHeight, &caImage);
  if (rt == NS_OK && mIOSurface && mColorProfile) {
    nsCARenderer::DrawSurfaceToCGContext(aCGContext, mIOSurface, mColorProfile,
                                         0, 0, aWidth, aHeight);
  } else if (rt == NS_OK && caImage != NULL) {
    
    ::CGContextSetInterpolationQuality(aCGContext, kCGInterpolationNone );
    ::CGContextTranslateCTM(aCGContext, 0, aHeight);
    ::CGContextScaleCTM(aCGContext, 1.0, -1.0);

    ::CGContextDrawImage(aCGContext, CGRectMake(0,0,aWidth,aHeight), caImage);
  } else {
    NS_NOTREACHED("nsCARenderer::Render failure");
  }
}

void* nsPluginInstanceOwner::GetPluginPortCopy()
{
#ifndef NP_NO_QUICKDRAW
  if (GetDrawingModel() == NPDrawingModelQuickDraw)
    return &mQDPluginPortCopy;
#endif
  if (GetDrawingModel() == NPDrawingModelCoreGraphics || 
      GetDrawingModel() == NPDrawingModelCoreAnimation ||
      GetDrawingModel() == NPDrawingModelInvalidatingCoreAnimation)
    return &mCGPluginPortCopy;
  return nsnull;
}
  












void* nsPluginInstanceOwner::SetPluginPortAndDetectChange()
{
  if (!mPluginWindow)
    return nsnull;
  void* pluginPort = GetPluginPortFromWidget();
  if (!pluginPort)
    return nsnull;
  mPluginWindow->window = pluginPort;

#ifndef NP_NO_QUICKDRAW
  NPDrawingModel drawingModel = GetDrawingModel();
  if (drawingModel == NPDrawingModelQuickDraw) {
    NP_Port* windowQDPort = static_cast<NP_Port*>(mPluginWindow->window);
    if (windowQDPort->port != mQDPluginPortCopy.port) {
      mQDPluginPortCopy.port = windowQDPort->port;
      mPluginPortChanged = true;
    }
  } else if (drawingModel == NPDrawingModelCoreGraphics || 
             drawingModel == NPDrawingModelCoreAnimation ||
             drawingModel == NPDrawingModelInvalidatingCoreAnimation)
#endif
  {
#ifndef NP_NO_CARBON
    if (GetEventModel() == NPEventModelCarbon) {
      NP_CGContext* windowCGPort = static_cast<NP_CGContext*>(mPluginWindow->window);
      if ((windowCGPort->context != mCGPluginPortCopy.context) ||
          (windowCGPort->window != mCGPluginPortCopy.window)) {
        mCGPluginPortCopy.context = windowCGPort->context;
        mCGPluginPortCopy.window = windowCGPort->window;
        mPluginPortChanged = true;
      }
    }
#endif
  }

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


PRUint32
nsPluginInstanceOwner::GetEventloopNestingLevel()
{
  nsCOMPtr<nsIAppShell> appShell = do_GetService(kAppShellCID);
  PRUint32 currentLevel = 0;
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

void nsPluginInstanceOwner::ScrollPositionWillChange(nscoord aX, nscoord aY)
{
#ifdef MAC_CARBON_PLUGINS
  if (GetEventModel() != NPEventModelCarbon)
    return;

  CancelTimer();

  if (mInstance) {
    nsCOMPtr<nsIPluginWidget> pluginWidget = do_QueryInterface(mWidget);
    if (pluginWidget && NS_SUCCEEDED(pluginWidget->StartDrawPlugin())) {
      EventRecord scrollEvent;
      InitializeEventRecord(&scrollEvent, nsnull);
      scrollEvent.what = NPEventType_ScrollingBeginsEvent;

      void* window = FixUpPluginWindow(ePluginPaintDisable);
      if (window) {
        mInstance->HandleEvent(&scrollEvent, nsnull);
      }
      pluginWidget->EndDrawPlugin();
    }
  }
#endif
}

void nsPluginInstanceOwner::ScrollPositionDidChange(nscoord aX, nscoord aY)
{
#ifdef MAC_CARBON_PLUGINS
  if (GetEventModel() != NPEventModelCarbon)
    return;

  if (mInstance) {
    nsCOMPtr<nsIPluginWidget> pluginWidget = do_QueryInterface(mWidget);
    if (pluginWidget && NS_SUCCEEDED(pluginWidget->StartDrawPlugin())) {
      EventRecord scrollEvent;
      InitializeEventRecord(&scrollEvent, nsnull);
      scrollEvent.what = NPEventType_ScrollingEndsEvent;

      void* window = FixUpPluginWindow(ePluginPaintEnable);
      if (window) {
        mInstance->HandleEvent(&scrollEvent, nsnull);
      }
      pluginWidget->EndDrawPlugin();
    }
  }
#endif
}

#ifdef MOZ_WIDGET_ANDROID




static nsPoint
GetOffsetRootContent(nsIFrame* aFrame)
{
  
  
  
  nsPoint offset(0, 0), docOffset(0, 0);
  const nsIFrame* f = aFrame;
  PRInt32 currAPD = aFrame->PresContext()->AppUnitsPerDevPixel();
  PRInt32 apd = currAPD;
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
      PRInt32 newAPD = f ? f->PresContext()->AppUnitsPerDevPixel() : 0;
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

gfxRect nsPluginInstanceOwner::GetPluginRect()
{
  
  nsRect bounds = mObjectFrame->GetContentRectRelativeToSelf() + GetOffsetRootContent(mObjectFrame);
  nsIntRect intBounds = bounds.ToNearestPixels(mObjectFrame->PresContext()->AppUnitsPerDevPixel());

  return gfxRect(intBounds);
}

void nsPluginInstanceOwner::SendSize(int width, int height)
{
  if (!mInstance)
    return;

  PRInt32 model = mInstance->GetANPDrawingModel();

  if (model != kOpenGL_ANPDrawingModel)
    return;

  ANPEvent event;
  event.inSize = sizeof(ANPEvent);
  event.eventType = kDraw_ANPEventType;
  event.data.draw.model = kOpenGL_ANPDrawingModel;
  event.data.draw.data.surfaceSize.width = width;
  event.data.draw.data.surfaceSize.height = height;

  mInstance->HandleEvent(&event, nsnull);
}

bool nsPluginInstanceOwner::AddPluginView(const gfxRect& aRect )
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

  if (AndroidBridge::Bridge())
    AndroidBridge::Bridge()->RemovePluginView((jobject)mJavaView, mFullScreen);

  AndroidBridge::GetJNIEnv()->DeleteGlobalRef((jobject)mJavaView);
  mJavaView = nsnull;

  if (mFullScreen)
    sFullScreenInstance = nsnull;
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
  
  PRInt32 model = mInstance->GetANPDrawingModel();

  if (model == kSurface_ANPDrawingModel) {
    
    
    AddPluginView(GetPluginRect());
  }

  mInstance->NotifyFullScreen(mFullScreen);

  
  
  Invalidate();
}

void nsPluginInstanceOwner::ExitFullScreen(jobject view) {
  JNIEnv* env = AndroidBridge::GetJNIEnv();

  if (env && sFullScreenInstance && sFullScreenInstance->mInstance &&
      env->IsSameObject(view, (jobject)sFullScreenInstance->mInstance->GetJavaSurface())) {
    sFullScreenInstance->ExitFullScreen();
  } 
}

#endif

nsresult nsPluginInstanceOwner::DispatchFocusToPlugin(nsIDOMEvent* aFocusEvent)
{
#ifdef MOZ_WIDGET_ANDROID
  {
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
    mInstance->HandleEvent(&event, nsnull);
  }
#endif

#ifndef XP_MACOSX
  if (!mPluginWindow || (mPluginWindow->type == NPWindowTypeWindow)) {
    
    return aFocusEvent->PreventDefault(); 
  }
#endif

  nsEvent* theEvent = aFocusEvent->GetInternalNSEvent();
  if (theEvent) {
    
    nsGUIEvent focusEvent(NS_IS_TRUSTED_EVENT(theEvent), theEvent->message,
                          nsnull);
    nsEventStatus rv = ProcessEvent(focusEvent);
    if (nsEventStatus_eConsumeNoDefault == rv) {
      aFocusEvent->PreventDefault();
      aFocusEvent->StopPropagation();
    }   
  }
  
  return NS_OK;
}    

#if defined(MOZ_WIDGET_QT) && (MOZ_PLATFORM_MAEMO == 6)
nsresult nsPluginInstanceOwner::Text(nsIDOMEvent* aTextEvent)
{
  if (mInstance) {
    nsEvent *event = aTextEvent->GetInternalNSEvent();
    if (event && event->eventStructType == NS_TEXT_EVENT) {
      nsEventStatus rv = ProcessEvent(*static_cast<nsGUIEvent*>(event));
      if (nsEventStatus_eConsumeNoDefault == rv) {
        aTextEvent->PreventDefault();
        aTextEvent->StopPropagation();
      }
    }
  }

  return NS_OK;
}
#endif

nsresult nsPluginInstanceOwner::KeyPress(nsIDOMEvent* aKeyEvent)
{
#ifdef XP_MACOSX
#ifndef NP_NO_CARBON
  if (GetEventModel() == NPEventModelCarbon) {
    
    
    
    nsEvent *theEvent = aKeyEvent->GetInternalNSEvent();
    const EventRecord *ev;
    if (theEvent &&
        theEvent->message == NS_KEY_PRESS &&
        (ev = (EventRecord*)(((nsGUIEvent*)theEvent)->pluginEvent)) &&
        ev->what == keyDown)
      return aKeyEvent->PreventDefault(); 

    
    
    
    
    static bool sInKeyDispatch = false;

    if (sInKeyDispatch)
      return aKeyEvent->PreventDefault(); 

    sInKeyDispatch = true;
    nsresult rv =  DispatchKeyToPlugin(aKeyEvent);
    sInKeyDispatch = false;
    return rv;
  }
#endif

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
    nsEvent *event = aKeyEvent->GetInternalNSEvent();
    if (event && event->eventStructType == NS_KEY_EVENT) {
      nsEventStatus rv = ProcessEvent(*static_cast<nsGUIEvent*>(event));
      if (nsEventStatus_eConsumeNoDefault == rv) {
        aKeyEvent->PreventDefault();
        aKeyEvent->StopPropagation();
      }   
    }
  }

  return NS_OK;
}    

nsresult
nsPluginInstanceOwner::MouseDown(nsIDOMEvent* aMouseEvent)
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

  nsEvent* event = aMouseEvent->GetInternalNSEvent();
    if (event && event->eventStructType == NS_MOUSE_EVENT) {
      nsEventStatus rv = ProcessEvent(*static_cast<nsGUIEvent*>(event));
    if (nsEventStatus_eConsumeNoDefault == rv) {
      return aMouseEvent->PreventDefault(); 
    }
  }
  
  return NS_OK;
}

nsresult nsPluginInstanceOwner::DispatchMouseToPlugin(nsIDOMEvent* aMouseEvent)
{
#if !defined(XP_MACOSX)
  if (!mPluginWindow || (mPluginWindow->type == NPWindowTypeWindow))
    return aMouseEvent->PreventDefault(); 
  
#endif
  
  if (!mWidgetVisible)
    return NS_OK;

  nsEvent* event = aMouseEvent->GetInternalNSEvent();
  if (event && event->eventStructType == NS_MOUSE_EVENT) {
    nsEventStatus rv = ProcessEvent(*static_cast<nsGUIEvent*>(event));
    if (nsEventStatus_eConsumeNoDefault == rv) {
      aMouseEvent->PreventDefault();
      aMouseEvent->StopPropagation();
    }
  }
  return NS_OK;
}

nsresult
nsPluginInstanceOwner::HandleEvent(nsIDOMEvent* aEvent)
{
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
    return MouseDown(aEvent);
  }
  if (eventType.EqualsLiteral("mouseup")) {
    
    
    
    
    if (!mContentFocused) {
      aEvent->PreventDefault();
      return NS_OK;
    }
    return DispatchMouseToPlugin(aEvent);
  }
  if (eventType.EqualsLiteral("mousemove") ||
      eventType.EqualsLiteral("click") ||
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
    return KeyPress(aEvent);
  }
#if defined(MOZ_WIDGET_QT) && (MOZ_PLATFORM_MAEMO == 6)
  if (eventType.EqualsLiteral("text")) {
    return Text(aEvent);
  }
#endif

  nsCOMPtr<nsIDOMDragEvent> dragEvent(do_QueryInterface(aEvent));
  if (dragEvent && mInstance) {
    nsEvent* ievent = aEvent->GetInternalNSEvent();
    if ((ievent && NS_IS_TRUSTED_EVENT(ievent)) &&
         ievent->message != NS_DRAGDROP_ENTER && ievent->message != NS_DRAGDROP_OVER) {
      aEvent->PreventDefault();
    }

    
    aEvent->StopPropagation();
  }
  return NS_OK;
}

#ifdef MOZ_X11
static unsigned int XInputEventState(const nsInputEvent& anEvent)
{
  unsigned int state = 0;
  if (anEvent.IsShift()) state |= ShiftMask;
  if (anEvent.IsControl()) state |= ControlMask;
  if (anEvent.IsAlt()) state |= Mod1Mask;
  if (anEvent.IsMeta()) state |= Mod4Mask;
  return state;
}
#endif

nsEventStatus nsPluginInstanceOwner::ProcessEvent(const nsGUIEvent& anEvent)
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

  
#ifndef NP_NO_CARBON
  EventRecord synthCarbonEvent;
#endif
  NPCocoaEvent synthCocoaEvent;
  void* event = anEvent.pluginEvent;
  nsPoint pt =
  nsLayoutUtils::GetEventCoordinatesRelativeTo(&anEvent, mObjectFrame) -
  mObjectFrame->GetContentRectRelativeToSelf().TopLeft();
  nsPresContext* presContext = mObjectFrame->PresContext();
  nsIntPoint ptPx(presContext->AppUnitsToDevPixels(pt.x),
                  presContext->AppUnitsToDevPixels(pt.y));
#ifndef NP_NO_CARBON
  nsIntPoint geckoScreenCoords = mWidget->WidgetToScreenOffset();
  ::Point carbonPt = { static_cast<short>(ptPx.y + geckoScreenCoords.y),
                       static_cast<short>(ptPx.x + geckoScreenCoords.x) };
  if (eventModel == NPEventModelCarbon) {
    if (event && anEvent.eventStructType == NS_MOUSE_EVENT) {
      static_cast<EventRecord*>(event)->where = carbonPt;
    }
  }
#endif
  if (!event) {
#ifndef NP_NO_CARBON
    if (eventModel == NPEventModelCarbon) {
      InitializeEventRecord(&synthCarbonEvent, &carbonPt);
    } else
#endif
    {
      InitializeNPCocoaEvent(&synthCocoaEvent);
    }
    
    switch (anEvent.message) {
      case NS_FOCUS_CONTENT:
      case NS_BLUR_CONTENT:
#ifndef NP_NO_CARBON
        if (eventModel == NPEventModelCarbon) {
          synthCarbonEvent.what = (anEvent.message == NS_FOCUS_CONTENT) ?
          NPEventType_GetFocusEvent : NPEventType_LoseFocusEvent;
          event = &synthCarbonEvent;
        }
#endif
        break;
      case NS_MOUSE_MOVE:
      {
        
        
        nsRefPtr<nsFrameSelection> frameselection = mObjectFrame->GetFrameSelection();
        if (!frameselection->GetMouseDownState() ||
            (nsIPresShell::GetCapturingContent() == mObjectFrame->GetContent())) {
#ifndef NP_NO_CARBON
          if (eventModel == NPEventModelCarbon) {
            synthCarbonEvent.what = osEvt;
            event = &synthCarbonEvent;
          } else
#endif
          {
            synthCocoaEvent.type = NPCocoaEventMouseMoved;
            synthCocoaEvent.data.mouse.pluginX = static_cast<double>(ptPx.x);
            synthCocoaEvent.data.mouse.pluginY = static_cast<double>(ptPx.y);
            event = &synthCocoaEvent;
          }
        }
      }
        break;
      case NS_MOUSE_BUTTON_DOWN:
#ifndef NP_NO_CARBON
        if (eventModel == NPEventModelCarbon) {
          synthCarbonEvent.what = mouseDown;
          event = &synthCarbonEvent;
        } else
#endif
        {
          synthCocoaEvent.type = NPCocoaEventMouseDown;
          synthCocoaEvent.data.mouse.pluginX = static_cast<double>(ptPx.x);
          synthCocoaEvent.data.mouse.pluginY = static_cast<double>(ptPx.y);
          event = &synthCocoaEvent;
        }
        break;
      case NS_MOUSE_BUTTON_UP:
        
        
        
        
        if ((static_cast<const nsMouseEvent&>(anEvent).button == nsMouseEvent::eLeftButton) &&
            (nsIPresShell::GetCapturingContent() != mObjectFrame->GetContent())) {
          if (eventModel == NPEventModelCocoa) {
            synthCocoaEvent.type = NPCocoaEventMouseEntered;
            synthCocoaEvent.data.mouse.pluginX = static_cast<double>(ptPx.x);
            synthCocoaEvent.data.mouse.pluginY = static_cast<double>(ptPx.y);
            event = &synthCocoaEvent;
          }
        } else {
#ifndef NP_NO_CARBON
          if (eventModel == NPEventModelCarbon) {
            synthCarbonEvent.what = mouseUp;
            event = &synthCarbonEvent;
          } else
#endif
          {
            synthCocoaEvent.type = NPCocoaEventMouseUp;
            synthCocoaEvent.data.mouse.pluginX = static_cast<double>(ptPx.x);
            synthCocoaEvent.data.mouse.pluginY = static_cast<double>(ptPx.y);
            event = &synthCocoaEvent;
          }
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

#ifndef NP_NO_CARBON
  
  
  
  if (eventModel == NPEventModelCarbon && anEvent.message == NS_FOCUS_CONTENT)
    ::DeactivateTSMDocument(::TSMGetActiveDocument());
#endif

  PRInt16 response = kNPEventNotHandled;
  void* window = FixUpPluginWindow(ePluginPaintEnable);
  if (window || (eventModel == NPEventModelCocoa)) {
    mInstance->HandleEvent(event, &response);
  }

  if (eventModel == NPEventModelCocoa && response == kNPEventStartIME) {
    pluginWidget->StartComplexTextInputForCurrentEvent();
  }

  if ((response == kNPEventHandled || response == kNPEventStartIME) &&
      !(anEvent.eventStructType == NS_MOUSE_EVENT &&
        anEvent.message == NS_MOUSE_BUTTON_DOWN &&
        static_cast<const nsMouseEvent&>(anEvent).button == nsMouseEvent::eLeftButton &&
        !mContentFocused))
    rv = nsEventStatus_eConsumeNoDefault;

  pluginWidget->EndDrawPlugin();
#endif

#ifdef XP_WIN
  
  NPEvent *pPluginEvent = (NPEvent*)anEvent.pluginEvent;
  
  
  NPEvent pluginEvent;
  if (anEvent.eventStructType == NS_MOUSE_EVENT) {
    if (!pPluginEvent) {
      
      
      pluginEvent.event = 0;
      const nsMouseEvent* mouseEvent = static_cast<const nsMouseEvent*>(&anEvent);
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
      pPluginEvent->lParam = MAKELPARAM(widgetPtPx.x, widgetPtPx.y);
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
    PRInt16 response = kNPEventNotHandled;
    mInstance->HandleEvent(pPluginEvent, &response);
    if (response == kNPEventHandled)
      rv = nsEventStatus_eConsumeNoDefault;
  }
#endif

#ifdef MOZ_X11
  
  nsIWidget* widget = anEvent.widget;
  XEvent pluginEvent = XEvent();
  pluginEvent.type = 0;

  switch(anEvent.eventStructType)
    {
    case NS_MOUSE_EVENT:
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
        const nsMouseEvent& mouseEvent =
          static_cast<const nsMouseEvent&>(anEvent);
        
        nsIntPoint rootPoint(-1,-1);
        if (widget)
          rootPoint = anEvent.refPoint + widget->WidgetToScreenOffset();
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
                case nsMouseEvent::eMiddleButton:
                  event.button = 2;
                  break;
                case nsMouseEvent::eRightButton:
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

   
 
   case NS_KEY_EVENT:
      if (anEvent.pluginEvent)
        {
          XKeyEvent &event = pluginEvent.xkey;
#ifdef MOZ_WIDGET_GTK
          event.root = GDK_ROOT_WINDOW();
          event.time = anEvent.time;
          const GdkEventKey* gdkEvent =
            static_cast<const GdkEventKey*>(anEvent.pluginEvent);
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

#ifdef MOZ_WIDGET_QT
          const nsKeyEvent& keyEvent = static_cast<const nsKeyEvent&>(anEvent);

          memset( &event, 0, sizeof(event) );
          event.time = anEvent.time;

          QWidget* qWidget = static_cast<QWidget*>(widget->GetNativeData(NS_NATIVE_WINDOW));

          if (qWidget)
#if defined(Q_WS_X11)
            event.root = qWidget->x11Info().appRootWindow();
#else
            event.root = RootWindowOfScreen(DefaultScreenOfDisplay(gfxQtPlatform::GetXDisplay(qWidget)));
#endif

          
          const QKeyEvent* qtEvent = static_cast<const QKeyEvent*>(anEvent.pluginEvent);
          if (qtEvent) {

            if (qtEvent->nativeModifiers())
              event.state = qtEvent->nativeModifiers();
            else 
              event.state = XInputEventState(keyEvent);

            if (qtEvent->nativeScanCode())
              event.keycode = qtEvent->nativeScanCode();
            else 
              event.keycode = XKeysymToKeycode( (widget ? static_cast<Display*>(widget->GetNativeData(NS_NATIVE_DISPLAY)) : nsnull), qtEvent->key());
          }

          switch (anEvent.message)
            {
            case NS_KEY_DOWN:
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
          
          
          
          
#if defined(MOZ_WIDGET_QT) && (MOZ_PLATFORM_MAEMO == 6)
          bool handled;
          if (NS_SUCCEEDED(mInstance->HandleGUIEvent(anEvent, &handled)) &&
              handled) {
            rv = nsEventStatus_eConsumeNoDefault;
          }
#else
          NS_WARNING("Synthesized key event not sent to plugin");
#endif
        }
      break;

#if defined(MOZ_WIDGET_QT) && (MOZ_PLATFORM_MAEMO == 6)
   case NS_TEXT_EVENT:
        {
          bool handled;
          if (NS_SUCCEEDED(mInstance->HandleGUIEvent(anEvent, &handled)) &&
              handled) {
            rv = nsEventStatus_eConsumeNoDefault;
          }
        }
      break;
#endif
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
    static_cast<Display*>(widget->GetNativeData(NS_NATIVE_DISPLAY)) : nsnull;
  event.window = None; 
  
  event.serial = 0;
  event.send_event = False;

  PRInt16 response = kNPEventNotHandled;
  mInstance->HandleEvent(&pluginEvent, &response);
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
  switch(anEvent.eventStructType)
    {
    case NS_MOUSE_EVENT:
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
              mInstance->HandleEvent(&event, nsnull);
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
              mInstance->HandleEvent(&event, nsnull);
            }
            break;
          }
      }
      break;

    case NS_KEY_EVENT:
     {
       const nsKeyEvent& keyEvent = static_cast<const nsKeyEvent&>(anEvent);
       LOG("Firing NS_KEY_EVENT %d %d\n", keyEvent.keyCode, keyEvent.charCode);
       
       ANPEvent* pluginEvent = reinterpret_cast<ANPEvent*>(keyEvent.pluginEvent);
       if (pluginEvent) {
         MOZ_ASSERT(pluginEvent->inSize == sizeof(ANPEvent));
         MOZ_ASSERT(pluginEvent->eventType == kKey_ANPEventType);
         mInstance->HandleEvent(pluginEvent, nsnull);
       }
     }
    }
    rv = nsEventStatus_eConsumeNoDefault;
#endif
 
  return rv;
}

nsresult
nsPluginInstanceOwner::Destroy()
{
  if (mObjectFrame)
    mObjectFrame->SetInstanceOwner(nsnull);

#ifdef MAC_CARBON_PLUGINS
  
  CancelTimer();
#endif
#ifdef XP_MACOSX
  RemoveFromCARefreshTimer();
  if (mColorProfile)
    ::CGColorSpaceRelease(mColorProfile);  
#endif

  
  if (mCXMenuListener) {
    mCXMenuListener->Destroy(mContent);
    mCXMenuListener = nsnull;
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
#if defined(MOZ_WIDGET_QT) && (MOZ_PLATFORM_MAEMO == 6)
  mContent->RemoveEventListener(NS_LITERAL_STRING("text"), this, true);
#endif

#if MOZ_WIDGET_ANDROID
  RemovePluginView();

  if (mLayer)
    mLayer->SetVisible(false);

#endif

  if (mWidget) {
    if (mPluginWindow) {
      mPluginWindow->SetPluginWidget(nsnull);
    }

    nsCOMPtr<nsIPluginWidget> pluginWidget = do_QueryInterface(mWidget);
    if (pluginWidget) {
      pluginWidget->SetPluginInstanceOwner(nsnull);
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
 
  nsCOMPtr<nsIPluginWidget> pluginWidget = do_QueryInterface(mWidget);
  if (pluginWidget && NS_SUCCEEDED(pluginWidget->StartDrawPlugin())) {
#ifndef NP_NO_CARBON
    void* window = FixUpPluginWindow(ePluginPaintEnable);
    if (GetEventModel() == NPEventModelCarbon && window) {
      EventRecord updateEvent;
      InitializeEventRecord(&updateEvent, nsnull);
      updateEvent.what = updateEvt;
      updateEvent.message = UInt32(window);

      mInstance->HandleEvent(&updateEvent, nsnull);
    } else if (GetEventModel() == NPEventModelCocoa)
#endif
    {
      DoCocoaEventDrawRect(aDirtyRect, cgContext);
    }
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

  mInstance->HandleEvent(&updateEvent, nsnull);
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
  mInstance->HandleEvent(&pluginEvent, nsnull);
}
#endif

#ifdef XP_OS2
void nsPluginInstanceOwner::Paint(const nsRect& aDirtyRect, HPS aHPS)
{
  if (!mInstance || !mObjectFrame)
    return;

  NPWindow *window;
  GetWindow(window);
  nsIntRect relDirtyRect = aDirtyRect.ToOutsidePixels(mObjectFrame->PresContext()->AppUnitsPerDevPixel());

  
  
  RECTL rectl;
  rectl.xLeft   = relDirtyRect.x + window->x;
  rectl.yBottom = relDirtyRect.y + window->y;
  rectl.xRight  = rectl.xLeft + relDirtyRect.width;
  rectl.yTop    = rectl.yBottom + relDirtyRect.height;

  NPEvent pluginEvent;
  pluginEvent.event = WM_PAINT;
  pluginEvent.wParam = (uint32)aHPS;
  pluginEvent.lParam = (uint32)&rectl;
  mInstance->HandleEvent(&pluginEvent, nsnull);
}
#endif

#ifdef MOZ_WIDGET_ANDROID

void nsPluginInstanceOwner::Paint(gfxContext* aContext,
                                  const gfxRect& aFrameRect,
                                  const gfxRect& aDirtyRect)
{
  if (!mInstance || !mObjectFrame || !mPluginDocumentActiveState || mFullScreen)
    return;

  PRInt32 model = mInstance->GetANPDrawingModel();

  gfxRect pluginRect = GetPluginRect();

  if (model == kSurface_ANPDrawingModel) {
    if (!AddPluginView(pluginRect)) {
      Invalidate();
    }
    return;
  }

  if (model == kOpenGL_ANPDrawingModel) {
    if (!mLayer)
      mLayer = new AndroidMediaLayer();

    mLayer->UpdatePosition(pluginRect);

    float xResolution = mObjectFrame->PresContext()->GetRootPresContext()->PresShell()->GetXResolution();
    float yResolution = mObjectFrame->PresContext()->GetRootPresContext()->PresShell()->GetYResolution();
    pluginRect.Scale(xResolution, yResolution);

    SendSize((int)pluginRect.width, (int)pluginRect.height);
    return;
  }

  if (model != kBitmap_ANPDrawingModel)
    return;

#ifdef ANP_BITMAP_DRAWING_MODEL
  static nsRefPtr<gfxImageSurface> pluginSurface;

  if (pluginSurface == nsnull ||
      aFrameRect.width  != pluginSurface->Width() ||
      aFrameRect.height != pluginSurface->Height()) {

    pluginSurface = new gfxImageSurface(gfxIntSize(aFrameRect.width, aFrameRect.height), 
                                        gfxImageSurface::ImageFormatARGB32);
    if (!pluginSurface)
      return;
  }

  
  nsRefPtr<gfxContext> ctx = new gfxContext(pluginSurface);
  ctx->SetOperator(gfxContext::OPERATOR_CLEAR);
  ctx->Paint();
  
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
    
  mInstance->HandleEvent(&event, nsnull);

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

  
  nsIntRect pluginDirtyRect(PRInt32(dirtyRect.x),
                            PRInt32(dirtyRect.y),
                            PRInt32(dirtyRect.width),
                            PRInt32(dirtyRect.height));
  if (!pluginDirtyRect.
      IntersectRect(nsIntRect(0, 0, pluginSize.width, pluginSize.height),
                    pluginDirtyRect))
    return;

  NPWindow* window;
  GetWindow(window);

  PRUint32 rendererFlags = 0;
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
  aContext->Translate(pluginRect.TopLeft());

  Renderer renderer(window, this, pluginSize, pluginDirtyRect);

  Display* dpy = mozilla::DefaultXDisplay();
  Screen* screen = DefaultScreenOfDisplay(dpy);
  Visual* visual = DefaultVisualOfScreen(screen);

  renderer.Draw(aContext, nsIntSize(window->width, window->height),
                rendererFlags, screen, visual, nsnull);
}
nsresult
nsPluginInstanceOwner::Renderer::DrawWithXlib(gfxXlibSurface* xsurface, 
                                              nsIntPoint offset,
                                              nsIntRect *clipRects, 
                                              PRUint32 numClipRects)
{
  Screen *screen = cairo_xlib_surface_get_screen(xsurface->CairoSurface());
  Colormap colormap;
  Visual* visual;
  if (!xsurface->GetColormapAndVisual(&colormap, &visual)) {
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
    
    
    gfxIntSize surfaceSize = xsurface->GetSize();
    clipRect.IntersectRect(clipRect,
                           nsIntRect(0, 0,
                                     surfaceSize.width, surfaceSize.height));
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
    exposeEvent.drawable = xsurface->XDrawable();
    exposeEvent.x = dirtyRect.x;
    exposeEvent.y = dirtyRect.y;
    exposeEvent.width  = dirtyRect.width;
    exposeEvent.height = dirtyRect.height;
    exposeEvent.count = 0;
    
    exposeEvent.serial = 0;
    exposeEvent.send_event = False;
    exposeEvent.major_code = 0;
    exposeEvent.minor_code = 0;

    instance->HandleEvent(&pluginEvent, nsnull);
  }
  return NS_OK;
}
#endif

void nsPluginInstanceOwner::SendIdleEvent()
{
#ifdef MAC_CARBON_PLUGINS
  
  
  
  if (mInstance) {
    nsCOMPtr<nsIPluginWidget> pluginWidget = do_QueryInterface(mWidget);
    if (pluginWidget && NS_SUCCEEDED(pluginWidget->StartDrawPlugin())) {
      void* window = FixUpPluginWindow(ePluginPaintEnable);
      if (window) {
        EventRecord idleEvent;
        InitializeEventRecord(&idleEvent, nsnull);
        idleEvent.what = nullEvent;

        
        
        if (!mWidgetVisible)
          idleEvent.where.h = idleEvent.where.v = 20000;

        mInstance->HandleEvent(&idleEvent, nsnull);
      }

      pluginWidget->EndDrawPlugin();
    }
  }
#endif
}

#ifdef MAC_CARBON_PLUGINS
void nsPluginInstanceOwner::StartTimer(bool isVisible)
{
  if (GetEventModel() != NPEventModelCarbon)
    return;

  mPluginHost->AddIdleTimeTarget(this, isVisible);
}

void nsPluginInstanceOwner::CancelTimer()
{
  mPluginHost->RemoveIdleTimeTarget(this);
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
    return NS_ERROR_FAILURE;
  }

  
  mCXMenuListener = new nsPluginDOMContextMenuListener();
  if (mCXMenuListener) {    
    mCXMenuListener->Init(aContent);
  }

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
#if defined(MOZ_WIDGET_QT) && (MOZ_PLATFORM_MAEMO == 6)
  mContent->AddEventListener(NS_LITERAL_STRING("text"), this, true);
#endif

  return NS_OK; 
}

void* nsPluginInstanceOwner::GetPluginPortFromWidget()
{


  void* result = NULL;
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
  if (!windowless && !nsIWidget::UsePuppetWidgets()) {
    
    
    nsCOMPtr<nsIWidget> parentWidget;
    nsIDocument *doc = nsnull;
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
    rv = mWidget->Create(parentWidget.get(), nsnull, nsIntRect(0,0,0,0),
                         nsnull, nsnull, &initData);
    if (NS_FAILED(rv)) {
      mWidget->Destroy();
      mWidget = nsnull;
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

    
    
    
    
    mPluginWindow->window = nsnull;
#ifdef MOZ_X11
    
    NPSetWindowCallbackStruct* ws_info = 
    static_cast<NPSetWindowCallbackStruct*>(mPluginWindow->ws_info);
    ws_info->display = DefaultXDisplay();
    
    nsCAutoString description;
    GetPluginDescription(description);
    NS_NAMED_LITERAL_CSTRING(flash10Head, "Shockwave Flash 10.");
    mFlash10Quirks = StringBeginsWith(description, flash10Head);
#endif
  } else if (mWidget) {
    
    
    mPluginWindow->type = NPWindowTypeWindow;
    mPluginWindow->window = GetPluginPortFromWidget();
#ifdef MAC_CARBON_PLUGINS
    
    StartTimer(true);
#endif
    
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

void* nsPluginInstanceOwner::FixUpPluginWindow(PRInt32 inPaintState)
{
  if (!mWidget || !mPluginWindow || !mInstance || !mObjectFrame)
    return nsnull;

  NPEventModel eventModel = GetEventModel();

  nsCOMPtr<nsIPluginWidget> pluginWidget = do_QueryInterface(mWidget);
  if (!pluginWidget)
    return nsnull;

  
  
  void* pluginPort = nsnull;
  if (mInCGPaintLevel > 0) {
    pluginPort = mPluginWindow->window;
  } else {
    pluginPort = SetPluginPortAndDetectChange();
  }

#ifdef MAC_CARBON_PLUGINS
  if (eventModel == NPEventModelCarbon && !pluginPort)
    return nsnull;
#endif

  
  void* cocoaTopLevelWindow = nsnull;
  if (eventModel == NPEventModelCocoa) {
    nsIWidget* widget = mObjectFrame->GetNearestWidget();
    if (!widget)
      return nsnull;
    cocoaTopLevelWindow = widget->GetNativeData(NS_NATIVE_WINDOW);
    if (!cocoaTopLevelWindow)
      return nsnull;
  }

  nsIntPoint pluginOrigin;
  nsIntRect widgetClip;
  bool widgetVisible;
  pluginWidget->GetPluginClipRect(widgetClip, pluginOrigin, widgetVisible);
  mWidgetVisible = widgetVisible;

  

#ifndef NP_NO_QUICKDRAW
  
  NPDrawingModel drawingModel = GetDrawingModel();
  if (drawingModel == NPDrawingModelQuickDraw) {
    mPluginWindow->x = -static_cast<NP_Port*>(pluginPort)->portx;
    mPluginWindow->y = -static_cast<NP_Port*>(pluginPort)->porty;
  }
  else if (drawingModel == NPDrawingModelCoreGraphics || 
           drawingModel == NPDrawingModelCoreAnimation ||
           drawingModel == NPDrawingModelInvalidatingCoreAnimation)
#endif
  {
    
    
    
    
    
    nsIntPoint geckoScreenCoords = mWidget->WidgetToScreenOffset();

    nsRect windowRect;
#ifndef NP_NO_CARBON
    if (eventModel == NPEventModelCarbon)
      NS_NPAPI_CarbonWindowFrame(static_cast<WindowRef>(static_cast<NP_CGContext*>(pluginPort)->window), windowRect);
    else
#endif
    {
      NS_NPAPI_CocoaWindowFrame(cocoaTopLevelWindow, windowRect);
    }

    mPluginWindow->x = geckoScreenCoords.x - windowRect.x;
    mPluginWindow->y = geckoScreenCoords.y - windowRect.y;
  }

  NPRect oldClipRect = mPluginWindow->clipRect;
  
  
  mPluginWindow->clipRect.top    = widgetClip.y;
  mPluginWindow->clipRect.left   = widgetClip.x;

  if (!mWidgetVisible || inPaintState == ePluginPaintDisable) {
    mPluginWindow->clipRect.bottom = mPluginWindow->clipRect.top;
    mPluginWindow->clipRect.right  = mPluginWindow->clipRect.left;
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
    mPluginPortChanged = false;
#ifdef MAC_CARBON_PLUGINS
    
    CancelTimer();
    if (mPluginWindow->clipRect.left == mPluginWindow->clipRect.right ||
        mPluginWindow->clipRect.top == mPluginWindow->clipRect.bottom) {
      StartTimer(false);
    }
    else {
      StartTimer(true);
    }
#endif
  } else if (mPluginPortChanged) {
    if (UseAsyncRendering()) {
      mInstance->AsyncSetWindow(mPluginWindow);
    }
    else {
      mPluginWindow->CallSetWindow(mInstance);
    }
    mPluginPortChanged = false;
  }

  
  
  if (eventModel == NPEventModelCocoa && !mSentInitialTopLevelWindowEvent) {
    
    mSentInitialTopLevelWindowEvent = true;

    nsPluginEvent pluginEvent(true, NS_PLUGIN_FOCUS_EVENT, nsnull);
    NPCocoaEvent cocoaEvent;
    InitializeNPCocoaEvent(&cocoaEvent);
    cocoaEvent.type = NPCocoaEventWindowFocusChanged;
    cocoaEvent.data.focus.hasFocus = NS_NPAPI_CocoaWindowIsMain(cocoaTopLevelWindow);
    pluginEvent.pluginEvent = &cocoaEvent;
    ProcessEvent(pluginEvent);
  }

#ifndef NP_NO_QUICKDRAW
  if (drawingModel == NPDrawingModelQuickDraw)
    return ::GetWindowFromPort(static_cast<NP_Port*>(pluginPort)->port);
#endif

#ifdef MAC_CARBON_PLUGINS
  if (drawingModel == NPDrawingModelCoreGraphics && eventModel == NPEventModelCarbon)
    return static_cast<NP_CGContext*>(pluginPort)->window;
#endif

  return nsnull;
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
    if (mLayer)
      mLayer->SetVisible(mPluginDocumentActiveState);

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

void nsPluginInstanceOwner::SetFrame(nsObjectFrame *aFrame)
{
  
  if (mObjectFrame == aFrame) {
    return;
  }

  
  if (mObjectFrame) {
    
    
    nsRefPtr<ImageContainer> container = mObjectFrame->GetImageContainer();
    if (container) {
#ifdef XP_MACOSX
      AutoLockImage autoLock(container);
      Image *image = autoLock.GetImage();
      if (image && (image->GetFormat() == Image::MAC_IO_SURFACE) && mObjectFrame) {
        
        MacIOSurfaceImage *oglImage = static_cast<MacIOSurfaceImage*>(image);
        oglImage->SetUpdateCallback(nsnull, nsnull);
        oglImage->SetDestroyCallback(nsnull);
        
        
        
        NS_RELEASE_THIS();
      }
      
      
      autoLock.Unlock();
#endif
      container->SetCurrentImage(nsnull);
    }

    
#if defined(XP_MACOSX) && !defined(NP_NO_QUICKDRAW)
    
    
    
    for (nsIFrame* f = mObjectFrame; f; f = nsLayoutUtils::GetCrossDocParentFrame(f)) {
      nsIScrollableFrame* sf = do_QueryFrame(f);
      if (sf) {
        sf->RemoveScrollPositionListener(this);
      }
    }
#endif

    
    mObjectFrame->SetInstanceOwner(nsnull);
  }

  
  mObjectFrame = aFrame;

  
  if (mObjectFrame) {
    mObjectFrame->SetInstanceOwner(this);
    
    
    if (mWidgetCreationComplete) {
      mObjectFrame->PrepForDrawing(mWidget);
    }
    mObjectFrame->FixupWindow(mObjectFrame->GetContentRectRelativeToSelf().Size());
    mObjectFrame->Invalidate(mObjectFrame->GetContentRectRelativeToSelf());

    
#if defined(XP_MACOSX) && !defined(NP_NO_QUICKDRAW)
    
    if (GetEventModel() == NPEventModelCarbon) {
      for (nsIFrame* f = aFrame; f; f = nsLayoutUtils::GetCrossDocParentFrame(f)) {
        nsIScrollableFrame* sf = do_QueryFrame(f);
        if (sf) {
          sf->AddScrollPositionListener(this);
        }
      }
    }
#endif
  }
}

nsObjectFrame* nsPluginInstanceOwner::GetFrame()
{
  return mObjectFrame;
}



void nsPluginInstanceOwner::FixUpURLS(const nsString &name, nsAString &value)
{
  if (name.LowerCaseEqualsLiteral("pluginurl") ||
      name.LowerCaseEqualsLiteral("pluginspage")) {        
    nsCOMPtr<nsIURI> baseURI = mContent->GetBaseURI();
    nsAutoString newURL;
    NS_MakeAbsoluteURI(newURL, value, baseURI);
    if (!newURL.IsEmpty())
      value = newURL;
  }
}

NS_IMETHODIMP nsPluginInstanceOwner::PrivateModeChanged(bool aEnabled)
{
  return mInstance ? mInstance->PrivateModeStateChanged(aEnabled) : NS_OK;
}



nsPluginDOMContextMenuListener::nsPluginDOMContextMenuListener()
{
}

nsPluginDOMContextMenuListener::~nsPluginDOMContextMenuListener()
{
}

NS_IMPL_ISUPPORTS1(nsPluginDOMContextMenuListener,
                   nsIDOMEventListener)

NS_IMETHODIMP
nsPluginDOMContextMenuListener::HandleEvent(nsIDOMEvent* aEvent)
{
  aEvent->PreventDefault(); 
  
  return NS_OK;
}

nsresult nsPluginDOMContextMenuListener::Init(nsIContent* aContent)
{
  nsCOMPtr<nsIDOMEventTarget> receiver(do_QueryInterface(aContent));
  if (receiver) {
    receiver->AddEventListener(NS_LITERAL_STRING("contextmenu"), this, true);
    return NS_OK;
  }
  
  return NS_ERROR_NO_INTERFACE;
}

nsresult nsPluginDOMContextMenuListener::Destroy(nsIContent* aContent)
{
  
  nsCOMPtr<nsIDOMEventTarget> receiver(do_QueryInterface(aContent));
  if (receiver) {
    receiver->RemoveEventListener(NS_LITERAL_STRING("contextmenu"), this, true);
  }
  
  return NS_OK;
}
