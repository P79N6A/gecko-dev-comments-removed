













































#ifdef MOZ_WIDGET_QT
#include <QWidget>
#include <QKeyEvent>
#ifdef MOZ_X11
#include <QX11Info>
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
#include "nsIDOMContextMenuListener.h"
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
#include "nsIPrivateDOMEvent.h"
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

#include "nsContentCID.h"
static NS_DEFINE_CID(kRangeCID, NS_RANGE_CID);

#include "nsWidgetsCID.h"
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
#include "gfxXlibNativeRenderer.h"
#endif

using namespace mozilla;




class nsPluginDOMContextMenuListener : public nsIDOMContextMenuListener
{
public:
  nsPluginDOMContextMenuListener();
  virtual ~nsPluginDOMContextMenuListener();

  NS_DECL_ISUPPORTS

  NS_IMETHOD ContextMenu(nsIDOMEvent* aContextMenuEvent);
  
  nsresult Init(nsIContent* aContent);
  nsresult Destroy(nsIContent* aContent);
  
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent)
  {
    return NS_OK;
  }
  nsEventStatus ProcessEvent(const nsGUIEvent& anEvent)
  {
    return nsEventStatus_eConsumeNoDefault;
  }
};

class AsyncPaintWaitEvent : public nsRunnable
{
public:
  AsyncPaintWaitEvent(nsIContent* aContent, PRBool aFinished) :
    mContent(aContent), mFinished(aFinished)
  {
  }

  NS_IMETHOD Run()
  {
    nsContentUtils::DispatchTrustedEvent(mContent->GetOwnerDoc(), mContent,
        mFinished ? NS_LITERAL_STRING("MozPaintWaitFinished") : NS_LITERAL_STRING("MozPaintWait"),
        PR_TRUE, PR_TRUE);
    return NS_OK;
  }

private:
  nsCOMPtr<nsIContent> mContent;
  PRPackedBool         mFinished;
};

void
nsPluginInstanceOwner::NotifyPaintWaiter(nsDisplayListBuilder* aBuilder)
{
  
  if (!mWaitingForPaint && !IsUpToDate() && aBuilder->ShouldSyncDecodeImages()) {
    nsCOMPtr<nsIRunnable> event = new AsyncPaintWaitEvent(mContent, PR_FALSE);
    
    
    mWaitingForPaint = nsContentUtils::AddScriptRunner(event);
  }
}

#ifdef XP_MACOSX
static void DrawPlugin(ImageContainer* aContainer, void* aPluginInstanceOwner)
{
  nsObjectFrame* frame = static_cast<nsPluginInstanceOwner*>(aPluginInstanceOwner)->GetOwner();
  if (frame) {
    frame->UpdateImageLayer(aContainer, gfxRect(0,0,0,0));
  }
}

static void OnDestroyImage(void* aPluginInstanceOwner)
{
  nsPluginInstanceOwner* owner = static_cast<nsPluginInstanceOwner*>(aPluginInstanceOwner);
  NS_IF_RELEASE(owner);
}
#endif 

PRBool
nsPluginInstanceOwner::SetCurrentImage(ImageContainer* aContainer)
{
  if (mInstance) {
    nsRefPtr<Image> image;
    
    
    mInstance->GetImage(aContainer, getter_AddRefs(image));
    if (image) {
#ifdef XP_MACOSX
      if (image->GetFormat() == Image::MAC_IO_SURFACE && mObjectFrame) {
        MacIOSurfaceImage *oglImage = static_cast<MacIOSurfaceImage*>(image.get());
        NS_ADDREF_THIS();
        oglImage->SetUpdateCallback(&DrawPlugin, this);
        oglImage->SetDestroyCallback(&OnDestroyImage);
      }
#endif
      aContainer->SetCurrentImage(image);
      return PR_TRUE;
    }
  }
  aContainer->SetCurrentImage(nsnull);
  return PR_FALSE;
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
  nsPluginHost *pluginHost = static_cast<nsPluginHost*>(pluginHostCOM.get());  
  if (pluginHost)
    pluginHost->NewPluginNativeWindow(&mPluginWindow);
  else
    mPluginWindow = nsnull;

  mObjectFrame = nsnull;
  mTagText = nsnull;
#ifdef XP_MACOSX
  memset(&mCGPluginPortCopy, 0, sizeof(NP_CGContext));
#ifndef NP_NO_QUICKDRAW
  memset(&mQDPluginPortCopy, 0, sizeof(NP_Port));
#endif
  mInCGPaintLevel = 0;
  mSentInitialTopLevelWindowEvent = PR_FALSE;
  mIOSurface = nsnull;
  mColorProfile = nsnull;
  mPluginPortChanged = PR_FALSE;
#endif
  mContentFocused = PR_FALSE;
  mWidgetVisible = PR_TRUE;
  mPluginWindowVisible = PR_FALSE;
  mNumCachedAttrs = 0;
  mNumCachedParams = 0;
  mCachedAttrParamNames = nsnull;
  mCachedAttrParamValues = nsnull;
  mDestroyWidget = PR_FALSE;

#ifdef XP_MACOSX
#ifndef NP_NO_QUICKDRAW
  mEventModel = NPEventModelCarbon;
#else
  mEventModel = NPEventModelCocoa;
#endif
#endif

  mWaitingForPaint = PR_FALSE;
}

nsPluginInstanceOwner::~nsPluginInstanceOwner()
{
  PRInt32 cnt;

  if (mWaitingForPaint) {
    
    
    nsCOMPtr<nsIRunnable> event = new AsyncPaintWaitEvent(mContent, PR_TRUE);
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

  
  nsCOMPtr<nsIPluginHost> pluginHostCOM = do_GetService(MOZ_PLUGIN_HOST_CONTRACTID);
  nsPluginHost *pluginHost = static_cast<nsPluginHost*>(pluginHostCOM.get());
  if (pluginHost) {
    pluginHost->DeletePluginNativeWindow(mPluginWindow);
    mPluginWindow = nsnull;
  }

  if (mInstance) {
    mInstance->InvalidateOwner();
  }
}

NS_IMPL_ADDREF(nsPluginInstanceOwner)
NS_IMPL_RELEASE(nsPluginInstanceOwner)

NS_INTERFACE_MAP_BEGIN(nsPluginInstanceOwner)
  NS_INTERFACE_MAP_ENTRY(nsIPluginInstanceOwner)
  NS_INTERFACE_MAP_ENTRY(nsIPluginTagInfo)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMouseListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMouseMotionListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMKeyListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMFocusListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIDOMEventListener, nsIDOMMouseListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIPluginInstanceOwner)
NS_INTERFACE_MAP_END

nsresult
nsPluginInstanceOwner::SetInstance(nsNPAPIPluginInstance *aInstance)
{
  NS_ASSERTION(!mInstance || !aInstance, "mInstance should only be set or unset!");

  
  
  
  if (mInstance && !aInstance)
    mInstance->InvalidateOwner();

  mInstance = aInstance;

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

  NS_IF_ADDREF(mInstance);
  *aInstance = mInstance;
  return NS_OK;
}

NS_IMETHODIMP nsPluginInstanceOwner::GetURL(const char *aURL,
                                            const char *aTarget,
                                            nsIInputStream *aPostStream,
                                            void *aHeadersData,
                                            PRUint32 aHeadersDataLen)
{
  NS_ENSURE_TRUE(mObjectFrame, NS_ERROR_NULL_POINTER);

  if (mContent->IsEditable()) {
    return NS_OK;
  }

  
  nsCOMPtr<nsISupports> container = mObjectFrame->PresContext()->GetContainer();
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
                       aPostStream, headersDataStream);

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

  
  
  NS_IF_ADDREF(*aDocument = mContent->GetOwnerDoc());
  return NS_OK;
}

NS_IMETHODIMP nsPluginInstanceOwner::InvalidateRect(NPRect *invalidRect)
{
  
  
  if (mWaitingForPaint && (!mObjectFrame || IsUpToDate())) {
    
    
    nsCOMPtr<nsIRunnable> event = new AsyncPaintWaitEvent(mContent, PR_TRUE);
    NS_DispatchToMainThread(event);
    mWaitingForPaint = false;
  }

  if (!mObjectFrame || !invalidRect || !mWidgetVisible)
    return NS_ERROR_FAILURE;

  
  
  
  nsRefPtr<ImageContainer> container = mObjectFrame->GetImageContainer();
  gfxIntSize oldSize(0, 0);
  if (container) {
    oldSize = container->GetCurrentSize();
    SetCurrentImage(container);
  }

#ifndef XP_MACOSX
  
  
  if (mWidget) {
    mWidget->Invalidate(nsIntRect(invalidRect->left, invalidRect->top,
                                  invalidRect->right - invalidRect->left,
                                  invalidRect->bottom - invalidRect->top),
                        PR_FALSE);
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

NS_IMETHODIMP nsPluginInstanceOwner::ForceRedraw()
{
  NS_ENSURE_TRUE(mObjectFrame, NS_ERROR_NULL_POINTER);
  nsIView* view = mObjectFrame->GetView();
  if (view) {
    return view->GetViewManager()->Composite();
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
#elif defined(MOZ_WIDGET_GTK2)
  
  nsIWidget* win = mObjectFrame->GetNearestWidget();
  if (!win)
    return NS_ERROR_FAILURE;
  GdkWindow* gdkWindow = static_cast<GdkWindow*>(win->GetNativeData(NS_NATIVE_WINDOW));
  if (!gdkWindow)
    return NS_ERROR_FAILURE;
  gdkWindow = gdk_window_get_toplevel(gdkWindow);
#ifdef MOZ_X11
  *static_cast<Window*>(value) = GDK_WINDOW_XID(gdkWindow);
#endif
  return NS_OK;
#elif defined(MOZ_WIDGET_QT)
  
  nsIWidget* win = mObjectFrame->GetNearestWidget();
  if (!win)
    return NS_ERROR_FAILURE;
  QWidget* widget = static_cast<QWidget*>(win->GetNativeData(NS_NATIVE_WINDOW));
  if (!widget)
    return NS_ERROR_FAILURE;
#ifdef MOZ_X11
  *static_cast<Window*>(value) = widget->handle();
  return NS_OK;
#endif
  return NS_ERROR_FAILURE;
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

NS_IMETHODIMP nsPluginInstanceOwner::SetWindow()
{
  NS_ENSURE_TRUE(mObjectFrame, NS_ERROR_NULL_POINTER);
  return mObjectFrame->CallSetWindow(PR_FALSE);
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
    return PR_FALSE;

  return NS_NPAPI_ConvertPointCocoa(mWidget->GetNativeData(NS_NATIVE_WIDGET),
                                    sourceX, sourceY, sourceSpace, destX, destY, destSpace);
#else
  
  return PR_FALSE;
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

    nsCOMPtr<nsIDOMRange> range(do_CreateInstance(kRangeCID,&rv));
    if (NS_FAILED(rv))
      return rv;

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

    nsIDocument* doc = mContent->GetOwnerDoc();
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
      if (!gCharsetMap || !gCharsetMap->Init(NUM_CHARSETS))
        return NS_ERROR_OUT_OF_MEMORY;

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

NS_IMETHODIMP nsPluginInstanceOwner::GetUniqueID(PRUint32 *result)
{
  NS_ENSURE_ARG_POINTER(result);
  *result = NS_PTR_TO_INT32(mObjectFrame);
  return NS_OK;
}






nsresult nsPluginInstanceOwner::EnsureCachedAttrParamArrays()
{
  if (mCachedAttrParamValues)
    return NS_OK;

  NS_PRECONDITION(((mNumCachedAttrs + mNumCachedParams) == 0) &&
                    !mCachedAttrParamNames,
                  "re-cache of attrs/params not implemented! use the DOM "
                    "node directy instead");
  NS_ENSURE_TRUE(mObjectFrame, NS_ERROR_NULL_POINTER);

  
  
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

  
  NS_ENSURE_TRUE(mObjectFrame, NS_ERROR_OUT_OF_MEMORY);

  
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

  
  if (!wmodeType.IsEmpty()) {
    mCachedAttrParamNames [nextAttrParamIndex] = ToNewUTF8String(NS_LITERAL_STRING("wmode"));
    mCachedAttrParamValues[nextAttrParamIndex] = ToNewUTF8String(NS_ConvertUTF8toUTF16(wmodeType));
    nextAttrParamIndex++;
  }

  
  for (PRInt32 index = start; index != end; index += increment) {
    const nsAttrName* attrName = mContent->GetAttrNameAt(index);
    nsIAtom* atom = attrName->LocalName();
    nsAutoString value;
    mContent->GetAttr(attrName->NamespaceID(), atom, value);
    nsAutoString name;
    atom->ToString(name);

    FixUpURLS(name, value);

    mCachedAttrParamNames [nextAttrParamIndex] = ToNewUTF8String(name);
    mCachedAttrParamValues[nextAttrParamIndex] = ToNewUTF8String(value);
    nextAttrParamIndex++;
  }

  
  if (!data.IsEmpty()) {
    mCachedAttrParamNames [nextAttrParamIndex] = ToNewUTF8String(NS_LITERAL_STRING("SRC"));
    mCachedAttrParamValues[nextAttrParamIndex] = ToNewUTF8String(data);
    nextAttrParamIndex++;
  }

  
  mCachedAttrParamNames [nextAttrParamIndex] = ToNewUTF8String(NS_LITERAL_STRING("PARAM"));
  mCachedAttrParamValues[nextAttrParamIndex] = nsnull;
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

    









    name.Trim(" \n\r\t\b", PR_TRUE, PR_TRUE, PR_FALSE);
    value.Trim(" \n\r\t\b", PR_TRUE, PR_TRUE, PR_FALSE);
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

PRBool nsPluginInstanceOwner::IsRemoteDrawingCoreAnimation()
{
  if (!mInstance)
    return PR_FALSE;

  PRBool coreAnimation;
  if (!NS_SUCCEEDED(mInstance->IsRemoteDrawingCoreAnimation(&coreAnimation)))
    return PR_FALSE;

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

void nsPluginInstanceOwner::AddToCARefreshTimer(nsPluginInstanceOwner *aPluginInstance) {
  if (!sCARefreshListeners) {
    sCARefreshListeners = new nsTArray<nsPluginInstanceOwner*>();
    if (!sCARefreshListeners) {
      return;
    }
  }

  NS_ASSERTION(!sCARefreshListeners->Contains(aPluginInstance), 
      "pluginInstanceOwner already registered as a listener");
  sCARefreshListeners->AppendElement(aPluginInstance);

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

void nsPluginInstanceOwner::RemoveFromCARefreshTimer(nsPluginInstanceOwner *aPluginInstance) {
  if (!sCARefreshListeners || sCARefreshListeners->Contains(aPluginInstance) == false) {
    return;
  }

  sCARefreshListeners->RemoveElement(aPluginInstance);

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

void nsPluginInstanceOwner::SetupCARefresh()
{
  if (!mInstance) {
    return;
  }

  const char* mime = nsnull;
  if (NS_SUCCEEDED(mInstance->GetMIMEType(&mime)) && mime) {
    
    if (strcmp(mime, "application/x-shockwave-flash") != 0) {
    AddToCARefreshTimer(this);
  }
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
    delete mIOSurface;

    
    mIOSurface = nsIOSurface::CreateIOSurface(aWidth, aHeight);
    if (mIOSurface) {
      nsIOSurface *attachSurface = nsIOSurface::LookupSurface(
                                      mIOSurface->GetIOSurfaceID());
      if (attachSurface) {
        mCARenderer.AttachIOSurface(attachSurface);
      } else {
        NS_ERROR("IOSurface attachment failed");
        delete attachSurface;
        delete mIOSurface;
        mIOSurface = NULL;
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

    mCARenderer.SetupRenderer(caLayer, aWidth, aHeight);

    
    
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
      mPluginPortChanged = PR_TRUE;
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
        mPluginPortChanged = PR_TRUE;
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


nsresult nsPluginInstanceOwner::Focus(nsIDOMEvent * aFocusEvent)
{
  mContentFocused = PR_TRUE;
  return DispatchFocusToPlugin(aFocusEvent);
}

nsresult nsPluginInstanceOwner::Blur(nsIDOMEvent * aFocusEvent)
{
  mContentFocused = PR_FALSE;
  return DispatchFocusToPlugin(aFocusEvent);
}

nsresult nsPluginInstanceOwner::DispatchFocusToPlugin(nsIDOMEvent* aFocusEvent)
{
#ifndef XP_MACOSX
  if (!mPluginWindow || (mPluginWindow->type == NPWindowTypeWindow)) {
    
    return aFocusEvent->PreventDefault(); 
  }
#endif

  nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(aFocusEvent));
  if (privateEvent) {
    nsEvent * theEvent = privateEvent->GetInternalNSEvent();
    if (theEvent) {
      
      nsGUIEvent focusEvent(NS_IS_TRUSTED_EVENT(theEvent), theEvent->message,
                            nsnull);
      nsEventStatus rv = ProcessEvent(focusEvent);
      if (nsEventStatus_eConsumeNoDefault == rv) {
        aFocusEvent->PreventDefault();
        aFocusEvent->StopPropagation();
      }
    }
    else NS_ASSERTION(PR_FALSE, "nsPluginInstanceOwner::DispatchFocusToPlugin failed, focusEvent null");   
  }
  else NS_ASSERTION(PR_FALSE, "nsPluginInstanceOwner::DispatchFocusToPlugin failed, privateEvent null");   
  
  return NS_OK;
}    



nsresult nsPluginInstanceOwner::KeyDown(nsIDOMEvent* aKeyEvent)
{
  return DispatchKeyToPlugin(aKeyEvent);
}

nsresult nsPluginInstanceOwner::KeyUp(nsIDOMEvent* aKeyEvent)
{
  return DispatchKeyToPlugin(aKeyEvent);
}

nsresult nsPluginInstanceOwner::KeyPress(nsIDOMEvent* aKeyEvent)
{
#ifdef XP_MACOSX
#ifndef NP_NO_CARBON
  if (GetEventModel() == NPEventModelCarbon) {
    
    
    
    nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(aKeyEvent));
    if (privateEvent) {
      nsEvent *theEvent = privateEvent->GetInternalNSEvent();
      const nsGUIEvent *guiEvent = (nsGUIEvent*)theEvent;
      const EventRecord *ev = (EventRecord*)(guiEvent->pluginEvent); 
      if (guiEvent &&
          guiEvent->message == NS_KEY_PRESS &&
          ev &&
          ev->what == keyDown)
        return aKeyEvent->PreventDefault(); 
    }

    
    
    
    
    static PRBool sInKeyDispatch = PR_FALSE;

    if (sInKeyDispatch)
      return aKeyEvent->PreventDefault(); 

    sInKeyDispatch = PR_TRUE;
    nsresult rv =  DispatchKeyToPlugin(aKeyEvent);
    sInKeyDispatch = PR_FALSE;
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
    nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(aKeyEvent));
    if (privateEvent) {
      nsKeyEvent *keyEvent = (nsKeyEvent *) privateEvent->GetInternalNSEvent();
      if (keyEvent) {
        nsEventStatus rv = ProcessEvent(*keyEvent);
        if (nsEventStatus_eConsumeNoDefault == rv) {
          aKeyEvent->PreventDefault();
          aKeyEvent->StopPropagation();
        }
      }
      else NS_ASSERTION(PR_FALSE, "nsPluginInstanceOwner::DispatchKeyToPlugin failed, keyEvent null");   
    }
    else NS_ASSERTION(PR_FALSE, "nsPluginInstanceOwner::DispatchKeyToPlugin failed, privateEvent null");   
  }

  return NS_OK;
}    



nsresult
nsPluginInstanceOwner::MouseMove(nsIDOMEvent* aMouseEvent)
{
#if !defined(XP_MACOSX)
  if (!mPluginWindow || (mPluginWindow->type == NPWindowTypeWindow))
    return aMouseEvent->PreventDefault(); 
  
#endif

  
  if (!mWidgetVisible)
    return NS_OK;

  nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(aMouseEvent));
  if (privateEvent) {
    nsMouseEvent* mouseEvent = (nsMouseEvent *) privateEvent->GetInternalNSEvent();
    if (mouseEvent) {
      nsEventStatus rv = ProcessEvent(*mouseEvent);
      if (nsEventStatus_eConsumeNoDefault == rv) {
        return aMouseEvent->PreventDefault(); 
      }
    }
    else NS_ASSERTION(PR_FALSE, "nsPluginInstanceOwner::MouseMove failed, mouseEvent null");   
  }
  else NS_ASSERTION(PR_FALSE, "nsPluginInstanceOwner::MouseMove failed, privateEvent null");   
  
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

  nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(aMouseEvent));
  if (privateEvent) {
    nsMouseEvent* mouseEvent = (nsMouseEvent *) privateEvent->GetInternalNSEvent();
    if (mouseEvent) {
      nsEventStatus rv = ProcessEvent(*mouseEvent);
      if (nsEventStatus_eConsumeNoDefault == rv) {
        return aMouseEvent->PreventDefault(); 
      }
    }
    else NS_ASSERTION(PR_FALSE, "nsPluginInstanceOwner::MouseDown failed, mouseEvent null");   
  }
  else NS_ASSERTION(PR_FALSE, "nsPluginInstanceOwner::MouseDown failed, privateEvent null");   
  
  return NS_OK;
}

nsresult
nsPluginInstanceOwner::MouseUp(nsIDOMEvent* aMouseEvent)
{
  
  
  
  
  if (!mContentFocused) {
    aMouseEvent->PreventDefault();
    return NS_OK;
  }
  return DispatchMouseToPlugin(aMouseEvent);
}

nsresult
nsPluginInstanceOwner::MouseClick(nsIDOMEvent* aMouseEvent)
{
  return DispatchMouseToPlugin(aMouseEvent);
}

nsresult
nsPluginInstanceOwner::MouseDblClick(nsIDOMEvent* aMouseEvent)
{
  return DispatchMouseToPlugin(aMouseEvent);
}

nsresult
nsPluginInstanceOwner::MouseOver(nsIDOMEvent* aMouseEvent)
{
  return DispatchMouseToPlugin(aMouseEvent);
}

nsresult
nsPluginInstanceOwner::MouseOut(nsIDOMEvent* aMouseEvent)
{
  return DispatchMouseToPlugin(aMouseEvent);
}

nsresult nsPluginInstanceOwner::DispatchMouseToPlugin(nsIDOMEvent* aMouseEvent)
{
#if !defined(XP_MACOSX)
  if (!mPluginWindow || (mPluginWindow->type == NPWindowTypeWindow))
    return aMouseEvent->PreventDefault(); 
  
#endif
  
  if (!mWidgetVisible)
    return NS_OK;

  nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(aMouseEvent));
  if (privateEvent) {
    nsMouseEvent* mouseEvent = (nsMouseEvent *) privateEvent->GetInternalNSEvent();
    if (mouseEvent) {
      nsEventStatus rv = ProcessEvent(*mouseEvent);
      if (nsEventStatus_eConsumeNoDefault == rv) {
        aMouseEvent->PreventDefault();
        aMouseEvent->StopPropagation();
      }
    }
    else NS_ASSERTION(PR_FALSE, "nsPluginInstanceOwner::DispatchMouseToPlugin failed, mouseEvent null");   
  }
  else NS_ASSERTION(PR_FALSE, "nsPluginInstanceOwner::DispatchMouseToPlugin failed, privateEvent null");   
  
  return NS_OK;
}

nsresult
nsPluginInstanceOwner::HandleEvent(nsIDOMEvent* aEvent)
{
  if (mInstance) {
    nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(aEvent));
    nsCOMPtr<nsIDOMDragEvent> dragEvent(do_QueryInterface(aEvent));
    if (privateEvent && dragEvent) {
      nsEvent* ievent = privateEvent->GetInternalNSEvent();
      if (ievent && NS_IS_TRUSTED_EVENT(ievent) &&
          (ievent->message == NS_DRAGDROP_ENTER || ievent->message == NS_DRAGDROP_OVER)) {
        
        nsCOMPtr<nsIDOMDataTransfer> dataTransfer;
        dragEvent->GetDataTransfer(getter_AddRefs(dataTransfer));
        if (dataTransfer)
          dataTransfer->SetEffectAllowed(NS_LITERAL_STRING("none"));
      }

      
      aEvent->PreventDefault();
      aEvent->StopPropagation();
    }
  }
  return NS_OK;
}

#ifdef MOZ_X11
static unsigned int XInputEventState(const nsInputEvent& anEvent)
{
  unsigned int state = 0;
  if (anEvent.isShift) state |= ShiftMask;
  if (anEvent.isControl) state |= ControlMask;
  if (anEvent.isAlt) state |= Mod1Mask;
  if (anEvent.isMeta) state |= Mod4Mask;
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
  ::Point carbonPt = { ptPx.y + geckoScreenCoords.y, ptPx.x + geckoScreenCoords.x };
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
      nsIntPoint widgetPtPx = ptPx + mObjectFrame->GetWindowOriginInPixels(PR_TRUE);
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
#ifdef MOZ_WIDGET_GTK2
        Window root = GDK_ROOT_WINDOW();
#elif defined(MOZ_WIDGET_QT)
        Window root = QX11Info::appRootWindow();
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
#ifdef MOZ_WIDGET_GTK2
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
            event.root = qWidget->x11Info().appRootWindow();

          
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
    static_cast<Display*>(widget->GetNativeData(NS_NATIVE_DISPLAY)) : nsnull;
  event.window = None; 
  
  event.serial = 0;
  event.send_event = False;

  PRInt16 response = kNPEventNotHandled;
  mInstance->HandleEvent(&pluginEvent, &response);
  if (response == kNPEventHandled)
    rv = nsEventStatus_eConsumeNoDefault;
#endif

  return rv;
}

nsresult
nsPluginInstanceOwner::Destroy()
{
#ifdef MAC_CARBON_PLUGINS
  
  CancelTimer();
#endif
#ifdef XP_MACOSX
  RemoveFromCARefreshTimer(this);
  delete mIOSurface;
  if (mColorProfile)
    ::CGColorSpaceRelease(mColorProfile);  
#endif

  
  if (mCXMenuListener) {
    mCXMenuListener->Destroy(mContent);
    mCXMenuListener = nsnull;
  }

  nsCOMPtr<nsIDOMEventTarget> target(do_QueryInterface(mContent));
  if (target) {

    nsCOMPtr<nsIDOMEventListener> listener;
    QueryInterface(NS_GET_IID(nsIDOMEventListener), getter_AddRefs(listener));

    
    mContent->RemoveEventListenerByIID(listener, NS_GET_IID(nsIDOMFocusListener));

    
    mContent->RemoveEventListenerByIID(listener, NS_GET_IID(nsIDOMMouseListener));

    
    mContent->RemoveEventListenerByIID(listener, NS_GET_IID(nsIDOMMouseMotionListener));

    
    target->RemoveEventListener(NS_LITERAL_STRING("keypress"), listener, PR_TRUE);
    target->RemoveEventListener(NS_LITERAL_STRING("keydown"), listener, PR_TRUE);
    target->RemoveEventListener(NS_LITERAL_STRING("keyup"), listener, PR_TRUE);

    
    target->RemoveEventListener(NS_LITERAL_STRING("drop"), listener, PR_TRUE);
    target->RemoveEventListener(NS_LITERAL_STRING("dragdrop"), listener, PR_TRUE);
    target->RemoveEventListener(NS_LITERAL_STRING("drag"), listener, PR_TRUE);
    target->RemoveEventListener(NS_LITERAL_STRING("dragenter"), listener, PR_TRUE);
    target->RemoveEventListener(NS_LITERAL_STRING("dragover"), listener, PR_TRUE);
    target->RemoveEventListener(NS_LITERAL_STRING("dragexit"), listener, PR_TRUE);
    target->RemoveEventListener(NS_LITERAL_STRING("dragleave"), listener, PR_TRUE);
    target->RemoveEventListener(NS_LITERAL_STRING("dragstart"), listener, PR_TRUE);
    target->RemoveEventListener(NS_LITERAL_STRING("draggesture"), listener, PR_TRUE);
    target->RemoveEventListener(NS_LITERAL_STRING("dragend"), listener, PR_TRUE);
  }

  if (mWidget) {
    nsCOMPtr<nsIPluginWidget> pluginWidget = do_QueryInterface(mWidget);
    if (pluginWidget)
      pluginWidget->SetPluginInstanceOwner(nsnull);

    if (mDestroyWidget)
      mWidget->Destroy();
  }

  return NS_OK;
}




void
nsPluginInstanceOwner::PrepareToStop(PRBool aDelayedStop)
{
  
  nsRefPtr<ImageContainer> container = mObjectFrame->GetImageContainer();
  if (container) {
#ifdef XP_MACOSX
    nsRefPtr<Image> image = container->GetCurrentImage();
    if (image && (image->GetFormat() == Image::MAC_IO_SURFACE) && mObjectFrame) {
      
      MacIOSurfaceImage *oglImage = static_cast<MacIOSurfaceImage*>(image.get());
      oglImage->SetUpdateCallback(nsnull, nsnull);
      oglImage->SetDestroyCallback(nsnull);
      
      
      
      NS_RELEASE_THIS();
    }
#endif
    container->SetCurrentImage(nsnull);
  }

#if defined(XP_WIN) || defined(MOZ_X11)
  if (aDelayedStop && mWidget) {
    
    
    

    
    
    mWidget->Show(PR_FALSE);
    mWidget->Enable(PR_FALSE);

    
    
    
    mWidget->SetParent(nsnull);

    mDestroyWidget = PR_TRUE;
  }
#endif

  
  for (nsIFrame* f = mObjectFrame; f; f = nsLayoutUtils::GetCrossDocParentFrame(f)) {
    nsIScrollableFrame* sf = do_QueryFrame(f);
    if (sf) {
      sf->RemoveScrollPositionListener(this);
    }
  }
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

  PRBool transparent;
  mInstance->IsTransparent(&transparent);
  if (!transparent)
    rendererFlags |= Renderer::DRAW_IS_OPAQUE;

  
  gfxContextAutoSaveRestore autoSR(aContext);
  aContext->Translate(pluginRect.TopLeft());

  Renderer renderer(window, this, pluginSize, pluginDirtyRect);
#ifdef MOZ_WIDGET_GTK2
  
  GdkVisual* gdkVisual = gdk_rgb_get_visual();
  Visual* visual = gdk_x11_visual_get_xvisual(gdkVisual);
  Screen* screen =
    gdk_x11_screen_get_xscreen(gdk_visual_get_screen(gdkVisual));
#endif
#ifdef MOZ_WIDGET_QT
  Display* dpy = QX11Info().display();
  Screen* screen = ScreenOfDisplay(dpy, QX11Info().screen());
  Visual* visual = static_cast<Visual*>(QX11Info().visual());
#endif
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

  
  PRBool doupdatewindow = PR_FALSE;

  if (mWindow->x != offset.x || mWindow->y != offset.y) {
    mWindow->x = offset.x;
    mWindow->y = offset.y;
    doupdatewindow = PR_TRUE;
  }

  if (nsIntSize(mWindow->width, mWindow->height) != mPluginSize) {
    mWindow->width = mPluginSize.width;
    mWindow->height = mPluginSize.height;
    doupdatewindow = PR_TRUE;
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
    doupdatewindow = PR_TRUE;
  }

  NPSetWindowCallbackStruct* ws_info = 
    static_cast<NPSetWindowCallbackStruct*>(mWindow->ws_info);
#ifdef MOZ_X11
  if (ws_info->visual != visual || ws_info->colormap != colormap) {
    ws_info->visual = visual;
    ws_info->colormap = colormap;
    ws_info->depth = gfxXlibSurface::DepthOfVisual(screen, visual);
    doupdatewindow = PR_TRUE;
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
void nsPluginInstanceOwner::StartTimer(PRBool isVisible)
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

nsresult nsPluginInstanceOwner::Init(nsPresContext* aPresContext,
                                     nsObjectFrame* aFrame,
                                     nsIContent*    aContent)
{
  mLastEventloopNestingLevel = GetEventloopNestingLevel();

  mObjectFrame = aFrame;
  mContent = aContent;

  nsWeakFrame weakFrame(aFrame);

  
  
  
  
  aPresContext->EnsureVisible();

  if (!weakFrame.IsAlive()) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  mCXMenuListener = new nsPluginDOMContextMenuListener();
  if (mCXMenuListener) {    
    mCXMenuListener->Init(aContent);
  }

  nsCOMPtr<nsIDOMEventTarget> target(do_QueryInterface(mContent));
  if (target) {

    nsCOMPtr<nsIDOMEventListener> listener;
    QueryInterface(NS_GET_IID(nsIDOMEventListener), getter_AddRefs(listener));

    
    mContent->AddEventListenerByIID(listener, NS_GET_IID(nsIDOMFocusListener));

    
    mContent->AddEventListenerByIID(listener, NS_GET_IID(nsIDOMMouseListener));

    
    mContent->AddEventListenerByIID(listener, NS_GET_IID(nsIDOMMouseMotionListener));

    
    target->AddEventListener(NS_LITERAL_STRING("keypress"), listener, PR_TRUE);
    target->AddEventListener(NS_LITERAL_STRING("keydown"), listener, PR_TRUE);
    target->AddEventListener(NS_LITERAL_STRING("keyup"), listener, PR_TRUE);

    
    target->AddEventListener(NS_LITERAL_STRING("drop"), listener, PR_TRUE);
    target->AddEventListener(NS_LITERAL_STRING("dragdrop"), listener, PR_TRUE);
    target->AddEventListener(NS_LITERAL_STRING("drag"), listener, PR_TRUE);
    target->AddEventListener(NS_LITERAL_STRING("dragenter"), listener, PR_TRUE);
    target->AddEventListener(NS_LITERAL_STRING("dragover"), listener, PR_TRUE);
    target->AddEventListener(NS_LITERAL_STRING("dragleave"), listener, PR_TRUE);
    target->AddEventListener(NS_LITERAL_STRING("dragexit"), listener, PR_TRUE);
    target->AddEventListener(NS_LITERAL_STRING("dragstart"), listener, PR_TRUE);
    target->AddEventListener(NS_LITERAL_STRING("draggesture"), listener, PR_TRUE);
    target->AddEventListener(NS_LITERAL_STRING("dragend"), listener, PR_TRUE);
  }
  
  
  
  
  for (nsIFrame* f = mObjectFrame; f; f = nsLayoutUtils::GetCrossDocParentFrame(f)) {
    nsIScrollableFrame* sf = do_QueryFrame(f);
    if (sf) {
      sf->AddScrollPositionListener(this);
    }
  }

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

  nsresult  rv = NS_ERROR_FAILURE;

  if (mObjectFrame) {
    if (!mWidget) {
      PRBool windowless = PR_FALSE;
      mInstance->IsWindowless(&windowless);

      
      nsPresContext* context = mObjectFrame->PresContext();
      rv = mObjectFrame->CreateWidget(context->DevPixelsToAppUnits(mPluginWindow->width),
                                      context->DevPixelsToAppUnits(mPluginWindow->height),
                                      windowless);
      if (NS_OK == rv) {
        mWidget = mObjectFrame->GetWidget();

        if (PR_TRUE == windowless) {
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

          
          mObjectFrame->FixupWindow(mObjectFrame->GetContentRectRelativeToSelf().Size());
        } else if (mWidget) {
          nsIWidget* parent = mWidget->GetParent();
          NS_ASSERTION(parent, "Plugin windows must not be toplevel");
          
          
          
          
          
          
          nsAutoTArray<nsIWidget::Configuration,1> configuration;
          mObjectFrame->GetEmptyClipConfiguration(&configuration);
          if (configuration.Length() > 0) {
            configuration[0].mBounds.width = mPluginWindow->width;
            configuration[0].mBounds.height = mPluginWindow->height;
          }
          parent->ConfigureChildren(configuration);

          
          
          mPluginWindow->type = NPWindowTypeWindow;
          mPluginWindow->window = GetPluginPortFromWidget();

#ifdef MAC_CARBON_PLUGINS
          
          StartTimer(PR_TRUE);
#endif

          
          mPluginWindow->SetPluginWidget(mWidget);

          
          nsCOMPtr<nsIPluginWidget> pluginWidget = do_QueryInterface(mWidget);
          if (pluginWidget)
            pluginWidget->SetPluginInstanceOwner(this);
        }
      }
    }
  }

  return rv;
}

void nsPluginInstanceOwner::SetPluginHost(nsIPluginHost* aHost)
{
  mPluginHost = static_cast<nsPluginHost*>(aHost);
}


#ifdef XP_MACOSX

void* nsPluginInstanceOwner::FixUpPluginWindow(PRInt32 inPaintState)
{
  if (!mWidget || !mPluginWindow || !mInstance || !mObjectFrame)
    return nsnull;

  NPDrawingModel drawingModel = GetDrawingModel();
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
  PRBool widgetVisible;
  pluginWidget->GetPluginClipRect(widgetClip, pluginOrigin, widgetVisible);
  mWidgetVisible = widgetVisible;

  

#ifndef NP_NO_QUICKDRAW
  
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
    mInstance->SetWindow(mPluginWindow);
    mPluginPortChanged = PR_FALSE;
#ifdef MAC_CARBON_PLUGINS
    
    CancelTimer();
    if (mPluginWindow->clipRect.left == mPluginWindow->clipRect.right ||
        mPluginWindow->clipRect.top == mPluginWindow->clipRect.bottom) {
      StartTimer(PR_FALSE);
    }
    else {
      StartTimer(PR_TRUE);
    }
#endif
  } else if (mPluginPortChanged) {
    mInstance->SetWindow(mPluginWindow);
    mPluginPortChanged = PR_FALSE;
  }

  
  
  if (eventModel == NPEventModelCocoa && !mSentInitialTopLevelWindowEvent) {
    
    mSentInitialTopLevelWindowEvent = PR_TRUE;

    nsPluginEvent pluginEvent(PR_TRUE, NS_PLUGIN_FOCUS_EVENT, nsnull);
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
  mWidgetVisible = PR_FALSE;
  mInstance->SetWindow(mPluginWindow);
}

#else 

void nsPluginInstanceOwner::UpdateWindowPositionAndClipRect(PRBool aSetWindow)
{
  if (!mPluginWindow)
    return;

  
  
  
  if (aSetWindow && !mWidget && mPluginWindowVisible && !UseAsyncRendering())
    return;

  const NPWindow oldWindow = *mPluginWindow;

  PRBool windowless = (mPluginWindow->type == NPWindowTypeDrawable);
  nsIntPoint origin = mObjectFrame->GetWindowOriginInPixels(windowless);

  mPluginWindow->x = origin.x;
  mPluginWindow->y = origin.y;

  mPluginWindow->clipRect.left = 0;
  mPluginWindow->clipRect.top = 0;

  if (mPluginWindowVisible) {
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
nsPluginInstanceOwner::CallSetWindow()
{
  if (!mInstance)
    return;

  if (UseAsyncRendering()) {
    mInstance->AsyncSetWindow(mPluginWindow);
  } else {
    mInstance->SetWindow(mPluginWindow);
  }
}

void
nsPluginInstanceOwner::UpdateWindowVisibility(PRBool aVisible)
{
  mPluginWindowVisible = aVisible;
  UpdateWindowPositionAndClipRect(PR_TRUE);
}

#endif 



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



nsPluginDOMContextMenuListener::nsPluginDOMContextMenuListener()
{
}

nsPluginDOMContextMenuListener::~nsPluginDOMContextMenuListener()
{
}

NS_IMPL_ISUPPORTS2(nsPluginDOMContextMenuListener,
                   nsIDOMContextMenuListener,
                   nsIDOMEventListener)

NS_IMETHODIMP
nsPluginDOMContextMenuListener::ContextMenu(nsIDOMEvent* aContextMenuEvent)
{
  aContextMenuEvent->PreventDefault(); 
  
  return NS_OK;
}

nsresult nsPluginDOMContextMenuListener::Init(nsIContent* aContent)
{
  nsCOMPtr<nsIDOMEventTarget> receiver(do_QueryInterface(aContent));
  if (receiver) {
    receiver->AddEventListener(NS_LITERAL_STRING("contextmenu"), this, PR_TRUE);
    return NS_OK;
  }
  
  return NS_ERROR_NO_INTERFACE;
}

nsresult nsPluginDOMContextMenuListener::Destroy(nsIContent* aContent)
{
  
  nsCOMPtr<nsIDOMEventTarget> receiver(do_QueryInterface(aContent));
  if (receiver) {
    receiver->RemoveEventListener(NS_LITERAL_STRING("contextmenu"), this, PR_TRUE);
  }
  
  return NS_OK;
}
