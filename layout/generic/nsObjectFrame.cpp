















































#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsWidgetsCID.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsIDOMKeyListener.h"
#include "nsIPluginHost.h"
#include "nsplugin.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "prmem.h"
#include "nsGkAtoms.h"
#include "nsIAppShell.h"
#include "nsIDocument.h"
#include "nsINodeInfo.h"
#include "nsIURL.h"
#include "nsNetUtil.h"
#include "nsIPluginInstanceOwner.h"
#include "plstr.h"
#include "nsILinkHandler.h"
#ifdef OJI
#include "nsIJVMPluginTagInfo.h"
#endif
#include "nsIEventListener.h"
#include "nsIScrollableView.h"
#include "nsIScrollPositionListener.h"
#include "nsITimer.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeOwner.h"
#include "nsDocShellCID.h"
#include "nsIWebBrowserChrome.h"
#include "nsIDOMElement.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMHTMLObjectElement.h"
#include "nsIDOMHTMLEmbedElement.h"
#include "nsIDOMHTMLAppletElement.h"
#include "nsIDOMWindow.h"
#include "nsIDOMDocumentEvent.h"
#include "nsIDOMMouseListener.h"
#include "nsIDOMMouseMotionListener.h"
#include "nsIDOMFocusListener.h"
#include "nsIDOMContextMenuListener.h"
#include "nsIDOMDragListener.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMNSEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIDocumentEncoder.h"
#include "nsXPIDLString.h"
#include "nsIDOMRange.h"
#include "nsIPluginWidget.h"
#include "nsGUIEvent.h"
#include "nsIRenderingContext.h"
#include "npapi.h"
#include "nsTransform2D.h"
#include "nsIImageLoadingContent.h"
#include "nsIObjectLoadingContent.h"
#include "nsPIDOMWindow.h"
#include "nsContentUtils.h"
#include "nsDisplayList.h"
#include "nsAttrName.h"
#include "nsDataHashtable.h"
#include "nsDOMClassInfo.h"


#include "nsIScriptGlobalObject.h"
#include "nsIScriptContext.h"
#include "nsIXPConnect.h"
#include "nsIXPCScriptable.h"
#include "nsIClassInfo.h"

#include "nsObjectFrame.h"
#include "nsIObjectFrame.h"
#include "nsPluginNativeWindow.h"
#include "nsPIPluginHost.h"
#include "nsIPluginDocument.h"

#include "nsThreadUtils.h"

#include "gfxContext.h"

#ifdef XP_WIN
#include "gfxWindowsNativeDrawing.h"
#endif


#ifdef ACCESSIBILITY
#include "nsIAccessibilityService.h"
#endif

#ifdef MOZ_LOGGING
#define FORCE_PR_LOG 1 /* Allow logging in the release build */
#endif 
#include "prlog.h"

#include <errno.h>

#include "nsContentCID.h"
static NS_DEFINE_CID(kRangeCID, NS_RANGE_CID);
static NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);

#ifdef XP_MACOSX
#include "gfxQuartzNativeDrawing.h"
#endif

#ifdef MOZ_X11
#include <X11/Xlib.h>

enum { XKeyPress = KeyPress };
#ifdef KeyPress
#undef KeyPress
#endif
#include "gfxXlibNativeRenderer.h"
#ifdef MOZ_WIDGET_GTK2
#include <gdk/gdkwindow.h>
#include <gdk/gdkx.h>
#endif
#endif

#ifdef XP_WIN
#include <wtypes.h>
#include <winuser.h>
#endif

#ifdef CreateEvent 
#undef CreateEvent
#endif

#ifdef PR_LOGGING 
static PRLogModuleInfo *nsObjectFrameLM = PR_NewLogModule("nsObjectFrame");
#endif 


#define NORMAL_PLUGIN_DELAY 17


#define HIDDEN_PLUGIN_DELAY 100




class nsPluginDOMContextMenuListener : public nsIDOMContextMenuListener,
                                       public nsIEventListener
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


class nsPluginInstanceOwner : public nsIPluginInstanceOwner,
                              public nsIPluginTagInfo2,
#ifdef OJI
                              public nsIJVMPluginTagInfo,
#endif
                              public nsIEventListener,
                              public nsITimerCallback,
                              public nsIDOMMouseListener,
                              public nsIDOMMouseMotionListener,
                              public nsIDOMKeyListener,
                              public nsIDOMFocusListener,
                              public nsIScrollPositionListener,
                              public nsIDOMDragListener

{
public:
  nsPluginInstanceOwner();
  virtual ~nsPluginInstanceOwner();

  NS_DECL_ISUPPORTS

  

  NS_IMETHOD SetInstance(nsIPluginInstance *aInstance);

  NS_IMETHOD GetInstance(nsIPluginInstance *&aInstance);

  NS_IMETHOD GetWindow(nsPluginWindow *&aWindow);

  NS_IMETHOD GetMode(nsPluginMode *aMode);

  NS_IMETHOD CreateWidget(void);

  NS_IMETHOD GetURL(const char *aURL, const char *aTarget, void *aPostData, 
                    PRUint32 aPostDataLen, void *aHeadersData, 
                    PRUint32 aHeadersDataLen, PRBool isFile = PR_FALSE);

  NS_IMETHOD ShowStatus(const char *aStatusMsg);

  NS_IMETHOD ShowStatus(const PRUnichar *aStatusMsg);
  
  NS_IMETHOD GetDocument(nsIDocument* *aDocument);

  NS_IMETHOD InvalidateRect(nsPluginRect *invalidRect);

  NS_IMETHOD InvalidateRegion(nsPluginRegion invalidRegion);

  NS_IMETHOD ForceRedraw();

  NS_IMETHOD GetValue(nsPluginInstancePeerVariable variable, void *value);

  

  NS_IMETHOD GetAttributes(PRUint16& n, const char*const*& names,
                           const char*const*& values);

  NS_IMETHOD GetAttribute(const char* name, const char* *result);

  

  NS_IMETHOD GetTagType(nsPluginTagType *result);

  NS_IMETHOD GetTagText(const char* *result);

  NS_IMETHOD GetParameters(PRUint16& n, const char*const*& names, const char*const*& values);

  NS_IMETHOD GetParameter(const char* name, const char* *result);
  
  NS_IMETHOD GetDocumentBase(const char* *result);
  
  NS_IMETHOD GetDocumentEncoding(const char* *result);
  
  NS_IMETHOD GetAlignment(const char* *result);
  
  NS_IMETHOD GetWidth(PRUint32 *result);
  
  NS_IMETHOD GetHeight(PRUint32 *result);

  NS_IMETHOD GetBorderVertSpace(PRUint32 *result);
  
  NS_IMETHOD GetBorderHorizSpace(PRUint32 *result);

  NS_IMETHOD GetUniqueID(PRUint32 *result);

  NS_IMETHOD GetDOMElement(nsIDOMElement* *result);

#ifdef OJI
  

  NS_IMETHOD GetCode(const char* *result);

  NS_IMETHOD GetCodeBase(const char* *result);

  NS_IMETHOD GetArchive(const char* *result);

  NS_IMETHOD GetName(const char* *result);

  NS_IMETHOD GetMayScript(PRBool *result);

#endif 

 


  NS_IMETHOD MouseDown(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseUp(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseClick(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseDblClick(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseOver(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseOut(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);     
  

  
  NS_IMETHOD MouseMove(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD DragMove(nsIDOMEvent* aMouseEvent) { return NS_OK; }

  
  NS_IMETHOD KeyDown(nsIDOMEvent* aKeyEvent);
  NS_IMETHOD KeyUp(nsIDOMEvent* aKeyEvent);
  NS_IMETHOD KeyPress(nsIDOMEvent* aKeyEvent);
  

  
  NS_IMETHOD Focus(nsIDOMEvent * aFocusEvent);
  NS_IMETHOD Blur(nsIDOMEvent * aFocusEvent);
  
  
  NS_IMETHOD DragEnter(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD DragOver(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD DragExit(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD DragDrop(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD DragGesture(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD Drag(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD DragEnd(nsIDOMEvent* aMouseEvent);
  

  nsresult Destroy();  

  void PrepareToStop(PRBool aDelayedStop);

  
  nsEventStatus ProcessEvent(const nsGUIEvent & anEvent);
  
#ifdef XP_WIN
  void Paint(const nsRect& aDirtyRect, HDC ndc);
#elif defined(XP_MACOSX)
  void Paint(const nsRect& aDirtyRect);  
#elif defined(MOZ_X11)
  void Paint(nsIRenderingContext& aRenderingContext,
             const nsRect& aDirtyRect);
#elif defined(XP_OS2)
  void Paint(const nsRect& aDirtyRect, HPS aHPS);
#endif

  
  NS_DECL_NSITIMERCALLBACK
  
  void CancelTimer();
  void StartTimer(unsigned int aDelay);

  
  NS_IMETHOD ScrollPositionWillChange(nsIScrollableView* aScrollable, nscoord aX, nscoord aY);
  NS_IMETHOD ScrollPositionDidChange(nsIScrollableView* aScrollable, nscoord aX, nscoord aY);

  

  nsresult Init(nsPresContext* aPresContext, nsObjectFrame* aFrame,
                nsIContent* aContent);

  nsPluginPort* GetPluginPort();
  void ReleasePluginPort(nsPluginPort * pluginPort);

  void SetPluginHost(nsIPluginHost* aHost);

#ifdef XP_MACOSX
  NPDrawingModel GetDrawingModel();
  WindowRef FixUpPluginWindow(PRInt32 inPaintState);
  void GUItoMacEvent(const nsGUIEvent& anEvent, EventRecord* origEvent, EventRecord& aMacEvent);
#endif

  void SetOwner(nsObjectFrame *aOwner)
  {
    mOwner = aOwner;
  }

  PRUint32 GetLastEventloopNestingLevel() const {
    return mLastEventloopNestingLevel; 
  }

  void ConsiderNewEventloopNestingLevel() {
    nsCOMPtr<nsIAppShell> appShell = do_GetService(kAppShellCID);
    if (appShell) {
      PRUint32 currentLevel = 0;
      appShell->GetEventloopNestingLevel(&currentLevel);
#ifdef XP_MACOSX
      
      
      currentLevel++;
#else
      
      
      
      
      if (!currentLevel) {
        currentLevel++;
      }
#endif
      if (currentLevel < mLastEventloopNestingLevel) {
        mLastEventloopNestingLevel = currentLevel;
      }
    }
  }

  const char* GetPluginName()
  {
    if (mInstance && mPluginHost) {
      nsCOMPtr<nsPIPluginHost> piPluginHost = do_QueryInterface(mPluginHost);
      char* name = NULL;
      if (NS_SUCCEEDED(piPluginHost->GetPluginName(mInstance, &name)) &&
          name)
        return name;
    }
    return "";
  }

private:
  void FixUpURLS(const nsString &name, nsAString &value);

  nsPluginNativeWindow       *mPluginWindow;
  nsCOMPtr<nsIPluginInstance> mInstance;
  nsObjectFrame              *mOwner;
  nsCOMPtr<nsIContent>        mContent;
  nsCString                   mDocumentBase;
  char                       *mTagText;
  nsCOMPtr<nsIWidget>         mWidget;
  nsCOMPtr<nsITimer>          mPluginTimer;
  nsCOMPtr<nsIPluginHost>     mPluginHost;

  
  
  
  PRUint32                    mLastEventloopNestingLevel;
  PRPackedBool                mContentFocused;
  PRPackedBool                mWidgetVisible;    

  
  
  PRPackedBool                mDestroyWidget;
  PRUint16          mNumCachedAttrs;
  PRUint16          mNumCachedParams;
  char              **mCachedAttrParamNames;
  char              **mCachedAttrParamValues;

  
  nsRefPtr<nsPluginDOMContextMenuListener> mCXMenuListener;

  nsresult DispatchKeyToPlugin(nsIDOMEvent* aKeyEvent);
  nsresult DispatchMouseToPlugin(nsIDOMEvent* aMouseEvent);
  nsresult DispatchFocusToPlugin(nsIDOMEvent* aFocusEvent);

  nsresult EnsureCachedAttrParamArrays();

#ifdef MOZ_X11
  class Renderer : public gfxXlibNativeRenderer {
  public:
    Renderer(nsPluginWindow* aWindow, nsIPluginInstance* aInstance,
             const nsIntRect& aDirtyRect)
      : mWindow(aWindow), mInstance(aInstance), mDirtyRect(aDirtyRect)
    {}
    virtual nsresult NativeDraw(Display* dpy, Drawable drawable, Visual* visual,
                                short offsetX, short offsetY,
                                XRectangle* clipRects, PRUint32 numClipRects);
  private:
    nsPluginWindow* mWindow;
    nsIPluginInstance* mInstance;
    const nsIntRect& mDirtyRect;
  };
#endif

};

#if defined(XP_WIN) || (defined(DO_DIRTY_INTERSECT) && defined(XP_MACOSX)) || defined(MOZ_X11) || defined(XP_OS2)
static void ConvertAppUnitsToPixels(const nsPresContext& aPresContext, const nsRect& aTwipsRect, nsIntRect& aPixelRect);
#endif

  
#ifdef XP_MACOSX

#ifdef DO_DIRTY_INTERSECT
  
  static void ConvertRelativeToWindowAbsolute(nsIFrame* aFrame, nsPoint& aRel, nsPoint& aAbs, nsIWidget *&aContainerWidget);
#endif

  enum { ePluginPaintIgnore, ePluginPaintEnable, ePluginPaintDisable };

#endif 

nsObjectFrame::nsObjectFrame(nsStyleContext* aContext)
  : nsObjectFrameSuper(aContext)
{
  PR_LOG(nsObjectFrameLM, PR_LOG_DEBUG,
         ("Created new nsObjectFrame %p\n", this));
}

nsObjectFrame::~nsObjectFrame()
{
  PR_LOG(nsObjectFrameLM, PR_LOG_DEBUG,
         ("nsObjectFrame %p deleted\n", this));
}

NS_IMETHODIMP
nsObjectFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  NS_PRECONDITION(aInstancePtr, "null out param");

  if (aIID.Equals(NS_GET_IID(nsIObjectFrame))) {
    *aInstancePtr = static_cast<nsIObjectFrame*>(this);
    return NS_OK;
  }

  return nsObjectFrameSuper::QueryInterface(aIID, aInstancePtr);
}

NS_IMETHODIMP_(nsrefcnt) nsObjectFrame::AddRef(void)
{
  NS_WARNING("not supported for frames");
  return 1;
}

NS_IMETHODIMP_(nsrefcnt) nsObjectFrame::Release(void)
{
  NS_WARNING("not supported for frames");
  return 1;
}

#ifdef ACCESSIBILITY
NS_IMETHODIMP nsObjectFrame::GetAccessible(nsIAccessible** aAccessible)
{
  nsCOMPtr<nsIAccessibilityService> accService = do_GetService("@mozilla.org/accessibilityService;1");

  if (accService) {
    return accService->CreateHTMLObjectFrameAccessible(this, aAccessible);
  }

  return NS_ERROR_FAILURE;
}

#ifdef XP_WIN
NS_IMETHODIMP nsObjectFrame::GetPluginPort(HWND *aPort)
{
  *aPort = (HWND) mInstanceOwner->GetPluginPort();
  return NS_OK;
}
#endif
#endif


static NS_DEFINE_CID(kWidgetCID, NS_CHILD_CID);
static NS_DEFINE_CID(kCPluginManagerCID, NS_PLUGINMANAGER_CID);



NS_IMETHODIMP 
nsObjectFrame::Init(nsIContent*      aContent,
                    nsIFrame*        aParent,
                    nsIFrame*        aPrevInFlow)
{
  mPreventInstantiation = PR_FALSE;

  PR_LOG(nsObjectFrameLM, PR_LOG_DEBUG,
         ("Initializing nsObjectFrame %p for content %p\n", this, aContent));

  return nsObjectFrameSuper::Init(aContent, aParent, aPrevInFlow);
}

void
nsObjectFrame::Destroy()
{
  NS_ASSERTION(!mPreventInstantiation, "about to crash due to bug 136927");

  
  
  StopPluginInternal(PR_TRUE);
  
  nsObjectFrameSuper::Destroy();
}

NS_IMETHODIMP
nsObjectFrame::DidSetStyleContext()
{
  if (HasView()) {
    nsIView* view = GetView();
    nsIViewManager* vm = view->GetViewManager();
    if (vm) {
      nsViewVisibility visibility = 
        IsHidden() ? nsViewVisibility_kHide : nsViewVisibility_kShow;
      vm->SetViewVisibility(view, visibility);
    }
  }

  return nsObjectFrameSuper::DidSetStyleContext();
}

nsIAtom*
nsObjectFrame::GetType() const
{
  return nsGkAtoms::objectFrame; 
}

#ifdef DEBUG
NS_IMETHODIMP
nsObjectFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("ObjectFrame"), aResult);
}
#endif

nsresult
nsObjectFrame::CreateWidgetForView(nsIView* aView)
{
  
  nsWidgetInitData initData;
  initData.mUnicode = PR_FALSE;
  return aView->CreateWidget(kWidgetCID, &initData);
}

nsresult
nsObjectFrame::CreateWidget(nscoord aWidth,
                            nscoord aHeight,
                            PRBool  aViewOnly)
{
  nsIView* view = GetView();
  NS_ASSERTION(view, "Object frames must have views");  
  if (!view) {
    return NS_OK;       
  }

  nsIViewManager* viewMan = view->GetViewManager();
  
  
  viewMan->SetViewVisibility(view, nsViewVisibility_kHide);

  
  
  
  
  nsIView* parentWithView;
  nsPoint origin;
  nsRect r(0, 0, mRect.width, mRect.height);

  GetOffsetFromView(origin, &parentWithView);
  viewMan->ResizeView(view, r);
  viewMan->MoveViewTo(view, origin.x, origin.y);

  if (!aViewOnly && !view->HasWidget()) {
    nsresult rv = CreateWidgetForView(view);
    if (NS_FAILED(rv)) {
      return NS_OK;       
    }
  }

  {
    
    
    
    
    for (nsIFrame* frame = this; frame; frame = frame->GetParent()) {
      const nsStyleBackground* background = frame->GetStyleBackground();
      if (!background->IsTransparent()) {  
        nsIWidget* win = view->GetWidget();
        if (win)
          win->SetBackgroundColor(background->mBackgroundColor);
        break;
      }
    }

  }

  if (!IsHidden()) {
    viewMan->SetViewVisibility(view, nsViewVisibility_kShow);
  }

  return NS_OK;
}

#define EMBED_DEF_WIDTH 240
#define EMBED_DEF_HEIGHT 200

 nscoord
nsObjectFrame::GetMinWidth(nsIRenderingContext *aRenderingContext)
{
  nscoord result = 0;

  if (!IsHidden(PR_FALSE)) {
    nsIAtom *atom = mContent->Tag();
    if (atom == nsGkAtoms::applet || atom == nsGkAtoms::embed) {
      result = nsPresContext::CSSPixelsToAppUnits(EMBED_DEF_WIDTH);
    }
  }

  DISPLAY_MIN_WIDTH(this, result);
  return result;
}

 nscoord
nsObjectFrame::GetPrefWidth(nsIRenderingContext *aRenderingContext)
{
  return nsObjectFrame::GetMinWidth(aRenderingContext);
}

void
nsObjectFrame::GetDesiredSize(nsPresContext* aPresContext,
                              const nsHTMLReflowState& aReflowState,
                              nsHTMLReflowMetrics& aMetrics)
{
  
  aMetrics.width = 0;
  aMetrics.height = 0;

  if (IsHidden(PR_FALSE)) {
    return;
  }
  
  aMetrics.width = aReflowState.ComputedWidth();
  aMetrics.height = aReflowState.ComputedHeight();

  
  nsIAtom *atom = mContent->Tag();
  if (atom == nsGkAtoms::applet || atom == nsGkAtoms::embed) {
    if (aMetrics.width == NS_UNCONSTRAINEDSIZE) {
      aMetrics.width = PR_MIN(PR_MAX(nsPresContext::CSSPixelsToAppUnits(EMBED_DEF_WIDTH),
                                     aReflowState.mComputedMinWidth),
                              aReflowState.mComputedMaxWidth);
    }
    if (aMetrics.height == NS_UNCONSTRAINEDSIZE) {
      aMetrics.height = PR_MIN(PR_MAX(nsPresContext::CSSPixelsToAppUnits(EMBED_DEF_HEIGHT),
                                      aReflowState.mComputedMinHeight),
                               aReflowState.mComputedMaxHeight);
    }

#if defined (MOZ_WIDGET_GTK2)
    
    
    
    
    aMetrics.height = PR_MIN(aPresContext->DevPixelsToAppUnits(PR_INT16_MAX), aMetrics.height);
    aMetrics.width = PR_MIN(aPresContext->DevPixelsToAppUnits(PR_INT16_MAX), aMetrics.width);
#endif
  }

  
  
  
  if (aMetrics.width == NS_UNCONSTRAINEDSIZE) {
    aMetrics.width =
      (aReflowState.mComputedMinWidth != NS_UNCONSTRAINEDSIZE) ?
        aReflowState.mComputedMinWidth : 0;
  }

  
  
  
  
  if (aMetrics.height == NS_UNCONSTRAINEDSIZE) {
    aMetrics.height =
      (aReflowState.mComputedMinHeight != NS_UNCONSTRAINEDSIZE) ?
        aReflowState.mComputedMinHeight : 0;
  }

  
  
  
  
  
}

NS_IMETHODIMP
nsObjectFrame::Reflow(nsPresContext*           aPresContext,
                      nsHTMLReflowMetrics&     aMetrics,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsObjectFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aMetrics, aStatus);

  
  GetDesiredSize(aPresContext, aReflowState, aMetrics);
  aMetrics.mOverflowArea = nsRect(0, 0,
                                  aMetrics.width, aMetrics.height);

  
  
  
  if (!GetContent()->IsDoneAddingChildren()) {
    aStatus = NS_FRAME_COMPLETE;
    return NS_OK;
  }

  
  if (aPresContext->Medium() == nsGkAtoms::print) {
    aStatus = NS_FRAME_COMPLETE;
    return NS_OK;
  }

  FixupWindow(nsSize(aMetrics.width, aMetrics.height));

  aStatus = NS_FRAME_COMPLETE;

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aMetrics);
  return NS_OK;
}

nsresult
nsObjectFrame::InstantiatePlugin(nsIPluginHost* aPluginHost, 
                                 const char* aMimeType,
                                 nsIURI* aURI)
{
  NS_ASSERTION(mPreventInstantiation,
               "Instantiation should be prevented here!");

  
  
  
  nsCOMPtr<nsIAppShell> appShell = do_GetService(kAppShellCID);
  if (appShell) {
    appShell->SuspendNative();
  }

  NS_ASSERTION(mContent, "We should have a content node.");

  nsIDocument* doc = mContent->GetOwnerDoc();
  nsCOMPtr<nsIPluginDocument> pDoc (do_QueryInterface(doc));

  nsresult rv;
  if (pDoc) {  
    nsCOMPtr<nsIStreamListener> stream;
    rv = aPluginHost->InstantiateFullPagePlugin(aMimeType, aURI,
                 *getter_AddRefs(stream),
                                                mInstanceOwner);
    if (NS_SUCCEEDED(rv))
      pDoc->SetStreamListener(stream);
  } else {   
    rv = aPluginHost->InstantiateEmbeddedPlugin(aMimeType, aURI,
                                                mInstanceOwner);
  }

  

  if (appShell) {
    appShell->ResumeNative();
  }

  return rv;
}

void
nsObjectFrame::FixupWindow(const nsSize& aSize)
{
  nsPresContext* presContext = PresContext();

  if (!mInstanceOwner)
    return;

  nsPluginWindow  *window;
  mInstanceOwner->GetWindow(window);

  NS_ENSURE_TRUE(window, );

#ifdef XP_MACOSX
  mInstanceOwner->FixUpPluginWindow(ePluginPaintDisable);
#endif

  PRBool windowless = (window->type == nsPluginWindowType_Drawable);

  nsPoint origin = GetWindowOriginInPixels(windowless);

  window->x = origin.x;
  window->y = origin.y;

  window->width = presContext->AppUnitsToDevPixels(aSize.width);
  window->height = presContext->AppUnitsToDevPixels(aSize.height);

  
  
  
  window->clipRect.top = 0;
  window->clipRect.left = 0;
#ifdef XP_MACOSX
  window->clipRect.bottom = 0;
  window->clipRect.right = 0;
#else
  window->clipRect.bottom = presContext->AppUnitsToDevPixels(aSize.height);
  window->clipRect.right = presContext->AppUnitsToDevPixels(aSize.width);
#endif
}

void
nsObjectFrame::CallSetWindow()
{
  nsPluginWindow *win = nsnull;
 
  nsresult rv;
  nsCOMPtr<nsIPluginInstance> pi; 
  if (!mInstanceOwner ||
      NS_FAILED(rv = mInstanceOwner->GetInstance(*getter_AddRefs(pi))) ||
      !pi ||
      NS_FAILED(rv = mInstanceOwner->GetWindow(win)) || 
      !win)
    return;

  nsPluginNativeWindow *window = (nsPluginNativeWindow *)win;

#ifdef XP_MACOSX
  mInstanceOwner->FixUpPluginWindow(ePluginPaintDisable);
#endif

  if (IsHidden())
    return;

  PRBool windowless = (window->type == nsPluginWindowType_Drawable);

  nsPoint origin = GetWindowOriginInPixels(windowless);

  window->x = origin.x;
  window->y = origin.y;

  
#ifdef MOZ_X11
  if(windowless) {
    
    
    nsIWidget* widget = GetWindow();
    if (widget) {
      NPSetWindowCallbackStruct* ws_info = 
        static_cast<NPSetWindowCallbackStruct*>(window->ws_info);
      ws_info->display =
        static_cast<Display*>(widget->GetNativeData(NS_NATIVE_DISPLAY));
#ifdef MOZ_WIDGET_GTK2
      GdkWindow* gdkWindow =
        static_cast<GdkWindow*>(widget->GetNativeData(NS_NATIVE_WINDOW));
      GdkColormap* gdkColormap = gdk_drawable_get_colormap(gdkWindow);
      ws_info->colormap = gdk_x11_colormap_get_xcolormap(gdkColormap);
      GdkVisual* gdkVisual = gdk_colormap_get_visual(gdkColormap);
      ws_info->visual = gdk_x11_visual_get_xvisual(gdkVisual);
      ws_info->depth = gdkVisual->depth;
#endif
    }
  }
  else
#endif
  {
    window->window = mInstanceOwner->GetPluginPort();
  }

  
  
  window->CallSetWindow(pi);

  mInstanceOwner->ReleasePluginPort((nsPluginPort *)window->window);
}

PRBool
nsObjectFrame::IsFocusable(PRInt32 *aTabIndex, PRBool aWithMouse)
{
  if (aTabIndex)
    *aTabIndex = -1;
  return nsObjectFrameSuper::IsFocusable(aTabIndex, aWithMouse);
}

PRBool
nsObjectFrame::IsHidden(PRBool aCheckVisibilityStyle) const
{
  if (aCheckVisibilityStyle) {
    if (!GetStyleVisibility()->IsVisibleOrCollapsed())
      return PR_TRUE;    
  }

  
  if (mContent->Tag() == nsGkAtoms::embed) {
    
    
    

    
    
    
    nsAutoString hidden;
    if (mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::hidden, hidden) &&
       (hidden.IsEmpty() ||
        !hidden.LowerCaseEqualsLiteral("false") &&
        !hidden.LowerCaseEqualsLiteral("no") &&
        !hidden.LowerCaseEqualsLiteral("off"))) {
      return PR_TRUE;
    }
  }

  return PR_FALSE;
}

nsPoint nsObjectFrame::GetWindowOriginInPixels(PRBool aWindowless)
{
  nsIView * parentWithView;
  nsPoint origin(0,0);

  GetOffsetFromView(origin, &parentWithView);

  
  
  if (aWindowless && parentWithView) {
    
    
    

    nsIViewManager* parentVM = parentWithView->GetViewManager();

    
    
    
    nsIView* theView = parentWithView;
    while (theView && !theView->GetWidget()) {
      if (theView->GetViewManager() != parentVM)
        break;

      origin += theView->GetPosition();
      theView = theView->GetParent();
    }  
  }

  origin.x = PresContext()->AppUnitsToDevPixels(origin.x);
  origin.y = PresContext()->AppUnitsToDevPixels(origin.y);

  return origin;
}

NS_IMETHODIMP
nsObjectFrame::DidReflow(nsPresContext*            aPresContext,
                         const nsHTMLReflowState*  aReflowState,
                         nsDidReflowStatus         aStatus)
{
  
  
  if (aStatus == NS_FRAME_REFLOW_FINISHED &&
      (GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
    nsCOMPtr<nsIObjectLoadingContent> objContent(do_QueryInterface(mContent));
    NS_ASSERTION(objContent, "Why not an object loading content?");
    objContent->HasNewFrame(this);
  }

  nsresult rv = nsObjectFrameSuper::DidReflow(aPresContext, aReflowState, aStatus);

  
  
  if (aStatus != NS_FRAME_REFLOW_FINISHED) 
    return rv;

  if (HasView()) {
    nsIView* view = GetView();
    nsIViewManager* vm = view->GetViewManager();
    if (vm)
      vm->SetViewVisibility(view, IsHidden() ? nsViewVisibility_kHide : nsViewVisibility_kShow);
  }

  
  CallSetWindow();

  return rv;
}

static void PaintPrintPlugin(nsIFrame* aFrame, nsIRenderingContext* aCtx,
                             const nsRect& aDirtyRect, nsPoint aPt)
{
  nsIRenderingContext::AutoPushTranslation translate(aCtx, aPt.x, aPt.y);
  static_cast<nsObjectFrame*>(aFrame)->PrintPlugin(*aCtx, aDirtyRect);
}

static void PaintPlugin(nsIFrame* aFrame, nsIRenderingContext* aCtx,
                        const nsRect& aDirtyRect, nsPoint aPt)
{
  nsIRenderingContext::AutoPushTranslation translate(aCtx, aPt.x, aPt.y);
#ifdef MOZ_X11 
  nsRect relativeDirtyRect = aDirtyRect - aPt;
  static_cast<nsObjectFrame*>(aFrame)->PaintPlugin(*aCtx, relativeDirtyRect);
#else
  static_cast<nsObjectFrame*>(aFrame)->PaintPlugin(*aCtx, aDirtyRect);
#endif
}

NS_IMETHODIMP
nsObjectFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists)
{
  
  if (!IsVisibleOrCollapsedForPainting(aBuilder))
    return NS_OK;
    
  nsresult rv = DisplayBorderBackgroundOutline(aBuilder, aLists);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsPresContext::nsPresContextType type = PresContext()->Type();

  
  if (type == nsPresContext::eContext_PrintPreview)
    return NS_OK;

  DO_GLOBAL_REFLOW_COUNT_DSP("nsObjectFrame");
  
  
  if (type == nsPresContext::eContext_Print)
    return aLists.Content()->AppendNewToTop(new (aBuilder)
        nsDisplayGeneric(this, PaintPrintPlugin, "PrintPlugin"));
  
  return aLists.Content()->AppendNewToTop(new (aBuilder)
      nsDisplayGeneric(this, ::PaintPlugin, "Plugin"));
}

void
nsObjectFrame::PrintPlugin(nsIRenderingContext& aRenderingContext,
                           const nsRect& aDirtyRect)
{
  
  

  
  nsIDocument* doc = mContent->GetCurrentDoc();
  if (!doc)
    return;

  
  
  nsIPresShell *shell = doc->GetPrimaryShell();
  if (!shell)
    return;

  
  nsIFrame* frame = shell->GetPrimaryFrameFor(mContent);
  if (!frame)
    return;

  nsPresContext* presContext = PresContext();
  
  
  nsIObjectFrame* objectFrame = nsnull;
  CallQueryInterface(frame,&objectFrame);
  if (!objectFrame)
    objectFrame = GetNextObjectFrame(presContext,frame);
  if (!objectFrame)
    return;

  
  nsCOMPtr<nsIPluginInstance> pi;
  if (NS_FAILED(objectFrame->GetPluginInstance(*getter_AddRefs(pi))) || !pi)
    return;

  
  nsresult rv;
  nsPluginWindow    window;
  window.window =   nsnull;

  
  nsPluginPrint npprint;
  npprint.mode = nsPluginMode_Embedded;

  
  PRBool windowless = PR_FALSE;
  pi->GetValue(nsPluginInstanceVariable_WindowlessBool, (void *)&windowless);
  window.type  =  windowless ? nsPluginWindowType_Drawable : nsPluginWindowType_Window;

  window.clipRect.bottom = 0; window.clipRect.top = 0;
  window.clipRect.left = 0; window.clipRect.right = 0;
  

#if defined(XP_UNIX) && !defined(XP_MACOSX)

  


#if 0
    






  PR_LOG(nsObjectFrameLM, PR_LOG_DEBUG, ("nsObjectFrame::Paint() start for X11 platforms\n"));
         
  FILE *plugintmpfile = tmpfile();
  if (!plugintmpfile) {
    PR_LOG(nsObjectFrameLM, PR_LOG_DEBUG, ("error: could not open tmp. file, errno=%d\n", errno));
    return;
  }
 
    
  NPPrintCallbackStruct npPrintInfo;
  npPrintInfo.type = NP_PRINT;
  npPrintInfo.fp   = plugintmpfile;
  npprint.print.embedPrint.platformPrint = (void *)&npPrintInfo;
  
  window.x =   aDirtyRect.x;
  window.y =   aDirtyRect.y;
  window.width =   aDirtyRect.width;
  window.height =   aDirtyRect.height;
  npprint.print.embedPrint.window        = window;
  rv = pi->Print(&npprint);
  if (NS_FAILED(rv)) {
    PR_LOG(nsObjectFrameLM, PR_LOG_DEBUG, ("error: plugin returned failure %lx\n", (long)rv));
    fclose(plugintmpfile);
    return;
  }

  
  rv = aRenderingContext.RenderEPS(aDirtyRect, plugintmpfile);

  fclose(plugintmpfile);

  PR_LOG(nsObjectFrameLM, PR_LOG_DEBUG, ("plugin printing done, return code is %lx\n", (long)rv));
#endif

#elif defined(XP_OS2)
  void *hps = aRenderingContext.GetNativeGraphicData(nsIRenderingContext::NATIVE_OS2_PS);
  if (!hps)
    return;

  npprint.print.embedPrint.platformPrint = hps;
  npprint.print.embedPrint.window = window;
  
  rv = pi->Print(&npprint);
#elif defined(XP_WIN)

  














  
  window.x = 0;
  window.y = 0;
  window.width = presContext->AppUnitsToDevPixels(mRect.width);
  window.height = presContext->AppUnitsToDevPixels(mRect.height);

  gfxContext *ctx = aRenderingContext.ThebesContext();

  ctx->Save();

  
  ctx->NewPath();
  ctx->Rectangle(gfxRect(window.x, window.y,
                         window.width, window.height));
  ctx->Clip();

  



  if (windowless)
    ctx->PushGroup(gfxASurface::CONTENT_COLOR_ALPHA);
  else
    ctx->PushGroup(gfxASurface::CONTENT_COLOR);

  gfxWindowsNativeDrawing nativeDraw(ctx,
                                     gfxRect(window.x, window.y,
                                             window.width, window.height));
  do {
    HDC dc = nativeDraw.BeginNativeDrawing();
    if (!dc)
      return;

    npprint.print.embedPrint.platformPrint = dc;
    npprint.print.embedPrint.window = window;
    
    rv = pi->Print(&npprint);

    nativeDraw.EndNativeDrawing();
  } while (nativeDraw.ShouldRenderAgain());
  nativeDraw.PaintToContext();

  ctx->PopGroupToSource();
  ctx->Paint();

  ctx->Restore();

#else

  
  nsTransform2D* rcTransform;
  aRenderingContext.GetCurrentTransform(rcTransform);
  nsPoint origin;
  rcTransform->GetTranslationCoord(&origin.x, &origin.y);

  
  
  window.x = presContext->AppUnitsToDevPixels(origin.x);
  window.y = presContext->AppUnitsToDevPixels(origin.y);
  window.width = presContext->AppUnitsToDevPixels(mRect.width);
  window.height= presContext->AppUnitsToDevPixels(mRect.height);

  
  
  
  void* dc;
  dc = aRenderingContext.GetNativeGraphicData(nsIRenderingContext::NATIVE_WINDOWS_DC);
  if (!dc)
    return; 

  npprint.print.embedPrint.platformPrint = dc;
  npprint.print.embedPrint.window = window;
  
    rv = pi->Print(&npprint);
#endif

  
  nsDidReflowStatus status = NS_FRAME_REFLOW_FINISHED; 
  frame->DidReflow(presContext,
                   nsnull, status);  
}

void
nsObjectFrame::PaintPlugin(nsIRenderingContext& aRenderingContext,
                           const nsRect& aDirtyRect)
{
  
#if defined(XP_MACOSX)
  
  if (mInstanceOwner) {
    if (mInstanceOwner->GetDrawingModel() == NPDrawingModelCoreGraphics) {
      PRInt32 p2a = PresContext()->AppUnitsPerDevPixel();
      gfxRect nativeClipRect(aDirtyRect.x, aDirtyRect.y,
                             aDirtyRect.width, aDirtyRect.height);
      nativeClipRect.ScaleInverse(gfxFloat(p2a));
      gfxContext* ctx = aRenderingContext.ThebesContext();
      gfxQuartzNativeDrawing nativeDrawing(ctx, nativeClipRect);

      CGContextRef cgContext = nativeDrawing.BeginNativeDrawing();
      if (!cgContext) {
        NS_WARNING("null CGContextRef during PaintPlugin");
        return;
      }

      
      
      
      
      
      nsPluginPort* pluginPort = mInstanceOwner->GetPluginPort();
      nsCOMPtr<nsIPluginInstance> inst;
      GetPluginInstance(*getter_AddRefs(inst));
      if (!inst) {
        NS_WARNING("null plugin instance during PaintPlugin");
        return;
      }
      nsPluginWindow* window;
      mInstanceOwner->GetWindow(window);
      if (!window) {
        NS_WARNING("null plugin window during PaintPlugin");
        return;
      }
      pluginPort->cgPort.context = cgContext;
      window->window = pluginPort;
      inst->SetWindow(window);

      mInstanceOwner->Paint(aDirtyRect);

      nativeDrawing.EndNativeDrawing();
    } else {
      mInstanceOwner->Paint(aDirtyRect);
    }
  }
#elif defined(MOZ_X11)
  if (mInstanceOwner)
    {
      nsPluginWindow * window;
      mInstanceOwner->GetWindow(window);

      if (window->type == nsPluginWindowType_Drawable)
        mInstanceOwner->Paint(aRenderingContext, aDirtyRect);
    }
#elif defined (XP_WIN) || defined(XP_OS2)
  nsCOMPtr<nsIPluginInstance> inst;
  GetPluginInstance(*getter_AddRefs(inst));
  if (inst) {
    
    nsPluginWindow * window;
    mInstanceOwner->GetWindow(window);

    if (window->type == nsPluginWindowType_Drawable) {
      
      PRBool doupdatewindow = PR_FALSE;
      
      nsPoint origin;

      







      gfxContext *ctx = aRenderingContext.ThebesContext();
      gfxMatrix ctxMatrix = ctx->CurrentMatrix();
      if (ctxMatrix.HasNonTranslation()) {
        
        
        

        
        
        

        return;
      }

      origin.x = NSToIntRound(float(ctxMatrix.GetTranslation().x));
      origin.y = NSToIntRound(float(ctxMatrix.GetTranslation().y));

      
      ctx->UpdateSurfaceClip();

      
      gfxFloat xoff, yoff;
      nsRefPtr<gfxASurface> surf = ctx->CurrentSurface(&xoff, &yoff);

      if (surf->CairoStatus() != 0) {
        NS_WARNING("Plugin is being asked to render to a surface that's in error!");
        return;
      }

#ifdef XP_WIN
      
      HDC hdc = (HDC)aRenderingContext.GetNativeGraphicData(nsIRenderingContext::NATIVE_WINDOWS_DC);

      if (reinterpret_cast<HDC>(window->window) != hdc) {
        window->window = reinterpret_cast<nsPluginPort*>(hdc);
        doupdatewindow = PR_TRUE;
      }

      SaveDC(hdc);

      POINT origViewportOrigin;
      GetViewportOrgEx(hdc, &origViewportOrigin);
      SetViewportOrgEx(hdc, origViewportOrigin.x + (int) xoff, origViewportOrigin.y + (int) yoff, NULL);
#else 
      
      HPS hps = (HPS)aRenderingContext.GetNativeGraphicData(nsIRenderingContext::NATIVE_OS2_PS);
      if (reinterpret_cast<HPS>(window->window) != hps) {
        window->window = reinterpret_cast<nsPluginPort*>(hps);
        doupdatewindow = PR_TRUE;
      }
      LONG lPSid = GpiSavePS(hps);
      RECTL rclViewport;
      if (GpiQueryDevice(hps) != NULLHANDLE) { 
        if (GpiQueryPageViewport(hps, &rclViewport)) {
          rclViewport.xLeft += (LONG)xoff;
          rclViewport.xRight += (LONG)xoff;
          rclViewport.yBottom += (LONG)yoff;
          rclViewport.yTop += (LONG)yoff;
          GpiSetPageViewport(hps, &rclViewport);
        }
      }
#endif

      if ((window->x != origin.x) || (window->y != origin.y)) {
        window->x = origin.x;
        window->y = origin.y;
        doupdatewindow = PR_TRUE;
      }

      
      if (doupdatewindow) {
#ifdef XP_WIN    
           
           
           
           
           
           
           
           
           
           

              origin = GetWindowOriginInPixels(PR_TRUE);
              nsRect winlessRect = nsRect(origin, nsSize(window->width, window->height));
              
              
              
              
              
              if (mWindowlessRect != winlessRect) {
                mWindowlessRect = winlessRect;

                WINDOWPOS winpos;
                memset(&winpos, 0, sizeof(winpos));
                winpos.x = mWindowlessRect.x;
                winpos.y = mWindowlessRect.y;
                winpos.cx = mWindowlessRect.width;
                winpos.cy = mWindowlessRect.height;

                
                nsPluginEvent pluginEvent;
                pluginEvent.event = WM_WINDOWPOSCHANGED;
                pluginEvent.wParam = 0;
                pluginEvent.lParam = (uint32)&winpos;
                PRBool eventHandled = PR_FALSE;

                inst->HandleEvent(&pluginEvent, &eventHandled);
              }
#endif

        inst->SetWindow(window);        
      }

#ifdef XP_WIN
      
      
      
      
      mInstanceOwner->Paint(aDirtyRect, hdc);

      RestoreDC(hdc, -1);
#else 
      mInstanceOwner->Paint(aDirtyRect, hps);
      if (lPSid >= 1) {
        GpiRestorePS(hps, lPSid);
      }
#endif
      surf->MarkDirty();
    }
  }
#endif
}

NS_IMETHODIMP
nsObjectFrame::HandleEvent(nsPresContext* aPresContext,
                           nsGUIEvent*     anEvent,
                           nsEventStatus*  anEventStatus)
{
  NS_ENSURE_ARG_POINTER(anEventStatus);
  nsresult rv = NS_OK;

  if (!mInstanceOwner)
    return NS_ERROR_NULL_POINTER;

  mInstanceOwner->ConsiderNewEventloopNestingLevel();

  if (anEvent->message == NS_PLUGIN_ACTIVATE) {
    nsIContent* content = GetContent();
    if (content) {
      content->SetFocus(aPresContext);
      return rv;
    }
  }

#ifdef XP_WIN
  rv = nsObjectFrameSuper::HandleEvent(aPresContext, anEvent, anEventStatus);
  return rv;
#endif

  switch (anEvent->message) {
  case NS_DESTROY:
    mInstanceOwner->CancelTimer();
    break;
  case NS_GOTFOCUS:
  case NS_LOSTFOCUS:
    *anEventStatus = mInstanceOwner->ProcessEvent(*anEvent);
    break;
    
  default:
    
    rv = nsObjectFrameSuper::HandleEvent(aPresContext, anEvent, anEventStatus);
  }

  return rv;
}

nsresult
nsObjectFrame::GetPluginInstance(nsIPluginInstance*& aPluginInstance)
{
  aPluginInstance = nsnull;

  if (!mInstanceOwner)
    return NS_OK;
  
  return mInstanceOwner->GetInstance(aPluginInstance);
}

nsresult
nsObjectFrame::PrepareInstanceOwner()
{
  nsWeakFrame weakFrame(this);

  
  StopPluginInternal(PR_FALSE);

  if (!weakFrame.IsAlive()) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  NS_ASSERTION(!mInstanceOwner, "Must not have an instance owner here");

  mInstanceOwner = new nsPluginInstanceOwner();

  PR_LOG(nsObjectFrameLM, PR_LOG_DEBUG,
         ("Created new instance owner %p for frame %p\n", mInstanceOwner.get(),
          this));

  if (!mInstanceOwner)
    return NS_ERROR_OUT_OF_MEMORY;

  
  return mInstanceOwner->Init(PresContext(), this, GetContent());
}

nsresult
nsObjectFrame::Instantiate(nsIChannel* aChannel, nsIStreamListener** aStreamListener)
{
  if (mPreventInstantiation) {
    return NS_OK;
  }
  
  
  
  nsresult rv = PrepareInstanceOwner();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPluginHost> pluginHost(do_GetService(kCPluginManagerCID, &rv));
  if (NS_FAILED(rv))
    return rv;
  mInstanceOwner->SetPluginHost(pluginHost);

  
  FixupWindow(mRect.Size());

  nsWeakFrame weakFrame(this);

  NS_ASSERTION(!mPreventInstantiation, "Say what?");
  mPreventInstantiation = PR_TRUE;
  rv = pluginHost->InstantiatePluginForChannel(aChannel, mInstanceOwner, aStreamListener);

  if (!weakFrame.IsAlive()) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  NS_ASSERTION(mPreventInstantiation,
               "Instantiation should still be prevented!");
  mPreventInstantiation = PR_FALSE;

  return rv;
}

nsresult
nsObjectFrame::Instantiate(const char* aMimeType, nsIURI* aURI)
{
  PR_LOG(nsObjectFrameLM, PR_LOG_DEBUG,
         ("nsObjectFrame::Instantiate(%s) called on frame %p\n", aMimeType,
          this));

  if (mPreventInstantiation) {
    return NS_OK;
  }

  NS_ASSERTION(aMimeType || aURI, "Need a type or a URI!");

  
  
  nsresult rv = PrepareInstanceOwner();
  NS_ENSURE_SUCCESS(rv, rv);

  nsWeakFrame weakFrame(this);

  
  FixupWindow(mRect.Size());

  
  nsCOMPtr<nsIPluginHost> pluginHost(do_GetService(kCPluginManagerCID, &rv));
  if (NS_FAILED(rv))
    return rv;
  mInstanceOwner->SetPluginHost(pluginHost);

  mPreventInstantiation = PR_TRUE;

  rv = InstantiatePlugin(pluginHost, aMimeType, aURI);

  if (!weakFrame.IsAlive()) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  if (NS_SUCCEEDED(rv)) {
    TryNotifyContentObjectWrapper();

    if (!weakFrame.IsAlive()) {
      return NS_ERROR_NOT_AVAILABLE;
    }

    CallSetWindow();
  }

  NS_ASSERTION(mPreventInstantiation,
               "Instantiation should still be prevented!");

  mPreventInstantiation = PR_FALSE;

  return rv;
}

void
nsObjectFrame::TryNotifyContentObjectWrapper()
{
  nsCOMPtr<nsIPluginInstance> inst;
  mInstanceOwner->GetInstance(*getter_AddRefs(inst));
  if (inst) {
    
    
    
    
    NotifyContentObjectWrapper();
  }
}

class nsStopPluginRunnable : public nsRunnable, public nsITimerCallback
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  nsStopPluginRunnable(nsPluginInstanceOwner *aInstanceOwner)
    : mInstanceOwner(aInstanceOwner)
  {
    NS_ASSERTION(aInstanceOwner, "need an owner");
  }

  
  NS_IMETHOD Run();

  
  NS_IMETHOD Notify(nsITimer *timer);

private:  
  nsCOMPtr<nsITimer> mTimer;
  nsRefPtr<nsPluginInstanceOwner> mInstanceOwner;
};

NS_IMPL_ISUPPORTS_INHERITED1(nsStopPluginRunnable, nsRunnable, nsITimerCallback)

static const char*
GetMIMEType(nsIPluginInstance *aPluginInstance)
{
  nsCOMPtr<nsIPluginInstancePeer> peer;
  aPluginInstance->GetPeer(getter_AddRefs(peer));
  if (peer) {
    nsMIMEType mime = NULL;
    if (NS_SUCCEEDED(peer->GetMIMEType(&mime)) && mime)
      return mime;
  }
  return "";
}

static PRBool
MatchPluginName(nsPluginInstanceOwner *aInstanceOwner, const char *aPluginName)
{
  return strncmp(aInstanceOwner->GetPluginName(),
                 aPluginName,
                 strlen(aPluginName)) == 0;
}

static PRBool
DoDelayedStop(nsPluginInstanceOwner *aInstanceOwner, PRBool aDelayedStop)
{
  
  if (aDelayedStop &&
      !::MatchPluginName(aInstanceOwner, "QuickTime") &&
      !::MatchPluginName(aInstanceOwner, "Flip4Mac")) {
    nsCOMPtr<nsIRunnable> evt = new nsStopPluginRunnable(aInstanceOwner);
    NS_DispatchToCurrentThread(evt);
    return PR_TRUE;
  }
  return PR_FALSE;
}

static void
DoStopPlugin(nsPluginInstanceOwner *aInstanceOwner, PRBool aDelayedStop)
{
  nsCOMPtr<nsIPluginInstance> inst;
  aInstanceOwner->GetInstance(*getter_AddRefs(inst));
  if (inst) {
    nsPluginWindow *win;
    aInstanceOwner->GetWindow(win);
    nsPluginNativeWindow *window = (nsPluginNativeWindow *)win;
    nsCOMPtr<nsIPluginInstance> nullinst;

    PRBool doCache = PR_TRUE;
    PRBool doCallSetWindowAfterDestroy = PR_FALSE;

    
    inst->GetValue(nsPluginInstanceVariable_DoCacheBool, (void *)&doCache);
    if (!doCache) {
      
      
      inst->GetValue(nsPluginInstanceVariable_CallSetWindowAfterDestroyBool, 
                     (void *)&doCallSetWindowAfterDestroy);
      if (doCallSetWindowAfterDestroy) {
        
        
        inst->Stop();
        inst->Destroy();

        if (window) 
          window->CallSetWindow(nullinst);
        else 
          inst->SetWindow(nsnull);
      }
      else {
        if (window) 
          window->CallSetWindow(nullinst);
        else 
          inst->SetWindow(nsnull);

        if (DoDelayedStop(aInstanceOwner, aDelayedStop))
          return;

        inst->Stop();
        inst->Destroy();
      }
    }
    else {
      if (window) 
        window->CallSetWindow(nullinst);
      else 
        inst->SetWindow(nsnull);

      if (DoDelayedStop(aInstanceOwner, aDelayedStop))
        return;

      inst->Stop();
    }

    nsCOMPtr<nsIPluginHost> pluginHost = do_GetService(kCPluginManagerCID);
    if (pluginHost)
      pluginHost->StopPluginInstance(inst);

    
    
    if (window)
      window->SetPluginWidget(nsnull);
  }

  aInstanceOwner->Destroy();
}

NS_IMETHODIMP
nsStopPluginRunnable::Notify(nsITimer *aTimer)
{
  return Run();
}

NS_IMETHODIMP
nsStopPluginRunnable::Run()
{
  
  
  nsCOMPtr<nsITimerCallback> kungFuDeathGrip = this;
  nsCOMPtr<nsIAppShell> appShell = do_GetService(kAppShellCID);
  if (appShell) {
    PRUint32 currentLevel = 0;
    appShell->GetEventloopNestingLevel(&currentLevel);
    if (currentLevel > mInstanceOwner->GetLastEventloopNestingLevel()) {
      if (!mTimer)
        mTimer = do_CreateInstance("@mozilla.org/timer;1");
      if (mTimer) {
        
        
        nsresult rv = mTimer->InitWithCallback(this, 100, nsITimer::TYPE_ONE_SHOT);
        if (NS_SUCCEEDED(rv)) {
          return rv;
        }
      }
      NS_ERROR("Failed to setup a timer to stop the plugin later (at a safe "
               "time). Stopping the plugin now, this might crash.");
    }
  }

  mTimer = nsnull;

  DoStopPlugin(mInstanceOwner, PR_FALSE);

  return NS_OK;
}

void
nsObjectFrame::StopPlugin()
{
  PRBool delayedStop = PR_FALSE;
#ifdef XP_WIN
  nsCOMPtr<nsIPluginInstance> inst;
  if (mInstanceOwner)
    mInstanceOwner->GetInstance(*getter_AddRefs(inst));
  if (inst) {
    
    const char* pluginType = ::GetMIMEType(inst);
    delayedStop = strcmp(pluginType, "audio/x-pn-realaudio-plugin") == 0;
  }
#endif
  StopPluginInternal(delayedStop);
}

void
nsObjectFrame::StopPluginInternal(PRBool aDelayedStop)
{
  if (!mInstanceOwner) {
    return;
  }

  
  
  
  
  

  nsRefPtr<nsPluginInstanceOwner> owner;
  owner.swap(mInstanceOwner);

  
  
  mWindowlessRect.Empty();

  PRBool oldVal = mPreventInstantiation;
  mPreventInstantiation = PR_TRUE;

  nsWeakFrame weakFrame(this);

#ifdef XP_WIN
  if (aDelayedStop) {
    
    
    
    

    
    nsIView *view = GetView();
    if (view) {
      view->DisownWidget();
    }
  }
#endif

  
  
  owner->PrepareToStop(aDelayedStop);

  DoStopPlugin(owner, aDelayedStop);

  
  if (weakFrame.IsAlive()) {
    NS_ASSERTION(mPreventInstantiation,
                 "Instantiation should still be prevented!");

    mPreventInstantiation = oldVal;
  }

  
  owner->SetOwner(nsnull);
}

void
nsObjectFrame::NotifyContentObjectWrapper()
{
  nsCOMPtr<nsIDocument> doc = mContent->GetDocument();
  if (!doc)
    return;

  nsIScriptGlobalObject *sgo = doc->GetScriptGlobalObject();
  if (!sgo)
    return;

  nsIScriptContext *scx = sgo->GetContext();
  if (!scx)
    return;

  JSContext *cx = (JSContext *)scx->GetNativeContext();

  nsCOMPtr<nsIXPConnectWrappedNative> wrapper;
  nsContentUtils::XPConnect()->
    GetWrappedNativeOfNativeObject(cx, sgo->GetGlobalJSObject(), mContent,
                                   NS_GET_IID(nsISupports),
                                   getter_AddRefs(wrapper));

  if (!wrapper) {
    
    
    return;
  }

  JSObject *obj = nsnull;
  nsresult rv = wrapper->GetJSObject(&obj);
  if (NS_FAILED(rv))
    return;

  nsHTMLPluginObjElementSH::SetupProtoChain(wrapper, cx, obj);
}


nsIObjectFrame *
nsObjectFrame::GetNextObjectFrame(nsPresContext* aPresContext, nsIFrame* aRoot)
{
  nsIFrame* child = aRoot->GetFirstChild(nsnull);

  while (child) {
    nsIObjectFrame* outFrame = nsnull;
    CallQueryInterface(child, &outFrame);
    if (outFrame) {
      nsCOMPtr<nsIPluginInstance> pi;
      outFrame->GetPluginInstance(*getter_AddRefs(pi));  
      if (pi)
        return outFrame;
    }

    outFrame = GetNextObjectFrame(aPresContext, child);
    if (outFrame)
      return outFrame;
    child = child->GetNextSibling();
  }

  return nsnull;
}

nsIFrame*
NS_NewObjectFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsObjectFrame(aContext);
}




nsPluginDOMContextMenuListener::nsPluginDOMContextMenuListener()
{
}

nsPluginDOMContextMenuListener::~nsPluginDOMContextMenuListener()
{
}

NS_IMPL_ISUPPORTS3(nsPluginDOMContextMenuListener,
                   nsIDOMContextMenuListener,
                   nsIDOMEventListener,
                   nsIEventListener)

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



nsPluginInstanceOwner::nsPluginInstanceOwner()
{
  
  
  nsCOMPtr<nsIPluginHost> ph = do_GetService(kCPluginManagerCID);
  nsCOMPtr<nsPIPluginHost> pph(do_QueryInterface(ph));
  if (pph)
    pph->NewPluginNativeWindow(&mPluginWindow);
  else
    mPluginWindow = nsnull;

  mOwner = nsnull;
  mTagText = nsnull;
  mContentFocused = PR_FALSE;
  mWidgetVisible = PR_TRUE;
  mNumCachedAttrs = 0;
  mNumCachedParams = 0;
  mCachedAttrParamNames = nsnull;
  mCachedAttrParamValues = nsnull;
  mDestroyWidget = PR_FALSE;

  PR_LOG(nsObjectFrameLM, PR_LOG_DEBUG,
         ("nsPluginInstanceOwner %p created\n", this));
}

nsPluginInstanceOwner::~nsPluginInstanceOwner()
{
  PRInt32 cnt;

  PR_LOG(nsObjectFrameLM, PR_LOG_DEBUG,
         ("nsPluginInstanceOwner %p deleted\n", this));

  
  if (mPluginTimer != nsnull) {
    CancelTimer();
  }

  mOwner = nsnull;

  for (cnt = 0; cnt < (mNumCachedAttrs + 1 + mNumCachedParams); cnt++) {
    if (mCachedAttrParamNames && mCachedAttrParamNames[cnt]) {
      PR_Free(mCachedAttrParamNames[cnt]);
      mCachedAttrParamNames[cnt] = nsnull;
    }

    if (mCachedAttrParamValues && mCachedAttrParamValues[cnt]) {
      PR_Free(mCachedAttrParamValues[cnt]);
      mCachedAttrParamValues[cnt] = nsnull;
    }
  }

  if (mCachedAttrParamNames) {
    PR_Free(mCachedAttrParamNames);
    mCachedAttrParamNames = nsnull;
  }

  if (mCachedAttrParamValues) {
    PR_Free(mCachedAttrParamValues);
    mCachedAttrParamValues = nsnull;
  }

  if (mTagText) {
    NS_Free(mTagText);
    mTagText = nsnull;
  }

  
  nsCOMPtr<nsIPluginHost> ph = do_GetService(kCPluginManagerCID);
  nsCOMPtr<nsPIPluginHost> pph(do_QueryInterface(ph));
  if (pph) {
    pph->DeletePluginNativeWindow(mPluginWindow);
    mPluginWindow = nsnull;
  }
}





NS_IMPL_ADDREF(nsPluginInstanceOwner)
NS_IMPL_RELEASE(nsPluginInstanceOwner)

NS_INTERFACE_MAP_BEGIN(nsPluginInstanceOwner)
  NS_INTERFACE_MAP_ENTRY(nsIPluginInstanceOwner)
  NS_INTERFACE_MAP_ENTRY(nsIPluginTagInfo)
  NS_INTERFACE_MAP_ENTRY(nsIPluginTagInfo2)
#ifdef OJI
  NS_INTERFACE_MAP_ENTRY(nsIJVMPluginTagInfo)
#endif
  NS_INTERFACE_MAP_ENTRY(nsIEventListener)
  NS_INTERFACE_MAP_ENTRY(nsITimerCallback)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMouseListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMouseMotionListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMKeyListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMFocusListener)
  NS_INTERFACE_MAP_ENTRY(nsIScrollPositionListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDragListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIDOMEventListener, nsIDOMMouseListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIPluginInstanceOwner)
NS_INTERFACE_MAP_END

NS_IMETHODIMP nsPluginInstanceOwner::SetInstance(nsIPluginInstance *aInstance)
{
  mInstance = aInstance;

  return NS_OK;
}

NS_IMETHODIMP nsPluginInstanceOwner::GetWindow(nsPluginWindow *&aWindow)
{
  NS_ASSERTION(mPluginWindow, "the plugin window object being returned is null");
  aWindow = mPluginWindow;
  return NS_OK;
}

NS_IMETHODIMP nsPluginInstanceOwner::GetMode(nsPluginMode *aMode)
{
  nsCOMPtr<nsIDocument> doc;
  nsresult rv = GetDocument(getter_AddRefs(doc));
  nsCOMPtr<nsIPluginDocument> pDoc (do_QueryInterface(doc));

  if (pDoc) {
    *aMode = nsPluginMode_Full;
  } else {
    *aMode = nsPluginMode_Embedded;
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

NS_IMETHODIMP nsPluginInstanceOwner::GetInstance(nsIPluginInstance *&aInstance)
{
  NS_IF_ADDREF(aInstance = mInstance);

  return NS_OK;
}

NS_IMETHODIMP nsPluginInstanceOwner::GetURL(const char *aURL, const char *aTarget, void *aPostData, PRUint32 aPostDataLen, void *aHeadersData, 
                                            PRUint32 aHeadersDataLen, PRBool isFile)
{
  NS_ENSURE_TRUE(mOwner,NS_ERROR_NULL_POINTER);

  if (mContent->IsEditable()) {
    return NS_OK;
  }

  
  nsCOMPtr<nsISupports> container = mOwner->PresContext()->GetContainer();
  NS_ENSURE_TRUE(container,NS_ERROR_FAILURE);
  nsCOMPtr<nsILinkHandler> lh = do_QueryInterface(container);
  NS_ENSURE_TRUE(lh, NS_ERROR_FAILURE);

  nsAutoString  unitarget;
  unitarget.AssignASCII(aTarget); 

  nsCOMPtr<nsIURI> baseURI = mContent->GetBaseURI();

  
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aURL, baseURI);

  NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

  nsCOMPtr<nsIInputStream> postDataStream;
  nsCOMPtr<nsIInputStream> headersDataStream;

  
  if (aPostData) {

    rv = NS_NewPluginPostDataStream(getter_AddRefs(postDataStream), (const char *)aPostData, aPostDataLen, isFile);

    NS_ASSERTION(NS_SUCCEEDED(rv),"failed in creating plugin post data stream");
    if (NS_FAILED(rv))
      return rv;

    if (aHeadersData) {
      rv = NS_NewPluginPostDataStream(getter_AddRefs(headersDataStream), 
                                      (const char *) aHeadersData, 
                                      aHeadersDataLen,
                                      PR_FALSE,
                                      PR_TRUE);  

      NS_ASSERTION(NS_SUCCEEDED(rv),"failed in creating plugin header data stream");
      if (NS_FAILED(rv))
        return rv;
    }
  }

  PRInt32 blockPopups =
    nsContentUtils::GetIntPref("privacy.popups.disable_from_plugins");
  nsAutoPopupStatePusher popupStatePusher((PopupControlState)blockPopups);

  rv = lh->OnLinkClick(mContent, uri, unitarget.get(), 
                       postDataStream, headersDataStream);

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

  if (!mOwner) {
    return rv;
  }
  nsCOMPtr<nsISupports> cont = mOwner->PresContext()->GetContainer();
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

NS_IMETHODIMP nsPluginInstanceOwner::InvalidateRect(nsPluginRect *invalidRect)
{
  nsresult rv = NS_ERROR_FAILURE;

  if (mOwner && invalidRect && mWidgetVisible) {
    
    nsIView* view = mOwner->GetView();

    if (view) {
      nsPresContext* presContext = mOwner->PresContext();

      nsRect rect(presContext->DevPixelsToAppUnits(invalidRect->left),
            presContext->DevPixelsToAppUnits(invalidRect->top),
            presContext->DevPixelsToAppUnits(invalidRect->right - invalidRect->left),
            presContext->DevPixelsToAppUnits(invalidRect->bottom - invalidRect->top));

      
      view->GetViewManager()->UpdateView(view, rect, NS_VMREFRESH_NO_SYNC);
    }
  }

  return rv;
}

NS_IMETHODIMP nsPluginInstanceOwner::InvalidateRegion(nsPluginRegion invalidRegion)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsPluginInstanceOwner::ForceRedraw()
{
  NS_ENSURE_TRUE(mOwner,NS_ERROR_NULL_POINTER);
  nsIView* view = mOwner->GetView();
  if (view) {
    return view->GetViewManager()->Composite();
  }

  return NS_OK;
}

NS_IMETHODIMP nsPluginInstanceOwner::GetValue(nsPluginInstancePeerVariable variable, void *value)
{
  nsresult rv = NS_ERROR_FAILURE;

  switch(variable) {
    case nsPluginInstancePeerVariable_NetscapeWindow:
    {      
      if (mOwner) {
#if defined(XP_WIN) || defined(XP_OS2)
        void** pvalue = (void**)value;
        nsIViewManager* vm = mOwner->PresContext()->GetViewManager();
        if (vm) {
#if defined(XP_WIN)
          
          
          
        
          
          

          
          

          
          
          
          
         
          
          
          
          if (mPluginWindow && mPluginWindow->type == nsPluginWindowType_Drawable) {
              
              
              
              
              
              
              
              
    
              nsIWidget* win = mOwner->GetWindow();
              if (win) {
                nsIView *view = nsIView::GetViewFor(win);
                NS_ASSERTION(view, "No view for widget");
                nsIView *rootView = nsnull;
                vm->GetRootView(rootView);
                NS_ASSERTION(rootView, "No root view");
                nsPoint offset = view->GetOffsetTo(rootView);
      
                if (offset.x || offset.y) {
                  
                  
                  *pvalue = (void*)win->GetNativeData(NS_NATIVE_WINDOW);
                  if (*pvalue)
                    return NS_OK;
                }
              }
          }
#endif
          
          nsCOMPtr<nsIWidget> widget;
          rv = vm->GetWidget(getter_AddRefs(widget));            
          if (widget) {
            *pvalue = (void*)widget->GetNativeData(NS_NATIVE_WINDOW);
          } else NS_ASSERTION(widget, "couldn't get doc's widget in getting doc's window handle");
        } else NS_ASSERTION(vm, "couldn't get view manager in getting doc's window handle");
#elif defined(MOZ_WIDGET_GTK2)
        
        nsIWidget* win = mOwner->GetWindow();
        if (!win)
          return rv;
        GdkWindow* gdkWindow =
          static_cast<GdkWindow*>(win->GetNativeData(NS_NATIVE_WINDOW));
        if (!gdkWindow)
          return rv;
        gdkWindow = gdk_window_get_toplevel(gdkWindow);
        *static_cast<Window*>(value) = GDK_WINDOW_XID(gdkWindow);
        return NS_OK;
#endif
      } else NS_ASSERTION(mOwner, "plugin owner has no owner in getting doc's window handle");
      break;
    }
  }

  return rv;
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
    if (!mOwner) {
      *result = nsnull;
      return NS_ERROR_FAILURE;
    }

    nsIDocument* doc = mContent->GetOwnerDoc();
    NS_ASSERTION(doc, "Must have an owner doc");
    rv = doc->GetBaseURI()->GetSpec(mDocumentBase);
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
    {"x-gbk",           "GBK"},
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
    {"windows-936",     "MS936"},
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
  *result = NS_PTR_TO_INT32(mOwner);
  return NS_OK;
}

#ifdef OJI
NS_IMETHODIMP nsPluginInstanceOwner::GetCode(const char* *result)
{
  nsresult rv;
  nsPluginTagType tagType;  
  NS_ENSURE_SUCCESS(rv = GetTagType(&tagType), rv);

  rv = NS_ERROR_FAILURE;
  if (nsPluginTagType_Object != tagType)
    rv = GetAttribute("CODE", result);
  if (NS_FAILED(rv))
    rv = GetParameter("CODE", result);

  return rv;
}

NS_IMETHODIMP nsPluginInstanceOwner::GetCodeBase(const char* *result)
{
  nsresult rv;
  if (NS_FAILED(rv = GetAttribute("CODEBASE", result)))
    rv = GetParameter("CODEBASE", result);
  return rv;
}

NS_IMETHODIMP nsPluginInstanceOwner::GetArchive(const char* *result)
{
  nsresult rv;
  if (NS_FAILED(rv = GetAttribute("ARCHIVE", result)))
    rv = GetParameter("ARCHIVE", result);
  return rv;
}

NS_IMETHODIMP nsPluginInstanceOwner::GetName(const char* *result)
{
  nsresult rv;
  nsPluginTagType tagType;  
  NS_ENSURE_SUCCESS(rv = GetTagType(&tagType), rv);

  rv = NS_ERROR_FAILURE;
  if (nsPluginTagType_Object != tagType)
    rv = GetAttribute("NAME", result);
  if (NS_FAILED(rv))
    rv = GetParameter("NAME", result);

  return rv;
}

NS_IMETHODIMP nsPluginInstanceOwner::GetMayScript(PRBool *result)
{
  NS_ENSURE_ARG_POINTER(result);
  nsPluginTagType tagType;  
  NS_ENSURE_SUCCESS(GetTagType(&tagType), NS_ERROR_FAILURE);

  const char* unused;
  if (nsPluginTagType_Object == tagType)
    *result = NS_SUCCEEDED(GetParameter("MAYSCRIPT", &unused)); 
  else
    *result = NS_SUCCEEDED(GetAttribute("MAYSCRIPT", &unused));

  return NS_OK;
}
#endif 







nsresult nsPluginInstanceOwner::EnsureCachedAttrParamArrays()
{
  if (mCachedAttrParamValues)
    return NS_OK;

  NS_PRECONDITION(((mNumCachedAttrs + mNumCachedParams) == 0) &&
                  !mCachedAttrParamNames,
                  "re-cache of attrs/params not implemented! use the DOM "
                  "node directy instead");
  NS_ENSURE_TRUE(mOwner, NS_ERROR_NULL_POINTER);

  
  
  mNumCachedAttrs = 0;

  PRUint32 cattrs = mContent->GetAttrCount();

  if (cattrs < 0x0000FFFF) {
    
    mNumCachedAttrs = static_cast<PRUint16>(cattrs);
  } else {
    mNumCachedAttrs = 0xFFFE;  
  }

  
  
  
  
  
  
  
  

  mNumCachedParams = 0;
  nsCOMArray<nsIDOMElement> ourParams;
 
  
  
  nsCOMPtr<nsIDOMElement> mydomElement = do_QueryInterface(mContent);
  NS_ENSURE_TRUE(mydomElement, NS_ERROR_NO_INTERFACE);

  nsCOMPtr<nsIDOMNodeList> allParams; 

  nsINodeInfo *ni = mContent->NodeInfo();

  
  
  nsCOMPtr<nsIPluginInstanceOwner> kungFuDeathGrip(this);
  
  if (ni->NamespaceEquals(kNameSpaceID_XHTML)) {
    
    

    NS_NAMED_LITERAL_STRING(xhtml_ns, "http://www.w3.org/1999/xhtml");

    mydomElement->GetElementsByTagNameNS(xhtml_ns, NS_LITERAL_STRING("param"),
                                         getter_AddRefs(allParams));
  } else {
    
    

    mydomElement->GetElementsByTagName(NS_LITERAL_STRING("param"),
                                       getter_AddRefs(allParams));
  }    

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
            if (domapplet)
              parent = domapplet;
            else
              parent = domobject;

            
            

            nsCOMPtr<nsIDOMNode> mydomNode = do_QueryInterface(mydomElement);
            if (parent == mydomNode) {
              ourParams.AppendObject(domelement);
            }
          }
        }
      }
    }
  }

  
  NS_ENSURE_TRUE(mOwner, NS_ERROR_OUT_OF_MEMORY);

  PRUint32 cparams = ourParams.Count(); 
  if (cparams < 0x0000FFFF)
    mNumCachedParams = static_cast<PRUint16>(cparams);
  else 
    mNumCachedParams = 0xFFFF;

  
  
  
  
  
  PRInt16 numRealAttrs = mNumCachedAttrs;
  nsAutoString data;
  if (mContent->Tag() == nsGkAtoms::object
    && !mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::src)
    && mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::data, data)) {
      mNumCachedAttrs++;
  }

  
  mCachedAttrParamNames  = (char **)PR_Calloc(sizeof(char *) * (mNumCachedAttrs + 1 + mNumCachedParams), 1);
  NS_ENSURE_TRUE(mCachedAttrParamNames,  NS_ERROR_OUT_OF_MEMORY);
  mCachedAttrParamValues = (char **)PR_Calloc(sizeof(char *) * (mNumCachedAttrs + 1 + mNumCachedParams), 1);
  NS_ENSURE_TRUE(mCachedAttrParamValues, NS_ERROR_OUT_OF_MEMORY);

  
  PRInt16 c = 0;

  
  
  
  
  
  
  PRInt16 start, end, increment;
  if (mContent->IsNodeOfType(nsINode::eHTML) &&
      mContent->NodeInfo()->NamespaceEquals(kNameSpaceID_None)) {
    
    start = numRealAttrs - 1;
    end = -1;
    increment = -1;
  } else {
    
    start = 0;
    end = numRealAttrs;
    increment = 1;
  }
  for (PRInt16 index = start; index != end; index += increment) {
    const nsAttrName* attrName = mContent->GetAttrNameAt(index);
    nsIAtom* atom = attrName->LocalName();
    nsAutoString value;
    mContent->GetAttr(attrName->NamespaceID(), atom, value);
    nsAutoString name;
    atom->ToString(name);

    FixUpURLS(name, value);

    mCachedAttrParamNames [c] = ToNewUTF8String(name);
    mCachedAttrParamValues[c] = ToNewUTF8String(value);
    c++;
  }

  
  if (data.Length()) {
    mCachedAttrParamNames [mNumCachedAttrs-1] = ToNewUTF8String(NS_LITERAL_STRING("SRC"));
    mCachedAttrParamValues[mNumCachedAttrs-1] = ToNewUTF8String(data);
  }

  
  mCachedAttrParamNames [mNumCachedAttrs] = ToNewUTF8String(NS_LITERAL_STRING("PARAM"));
  mCachedAttrParamValues[mNumCachedAttrs] = nsnull;

  
  c = 0;
  for (PRInt16 idx = 0; idx < mNumCachedParams; idx++) {
    nsIDOMElement* param = ourParams.ObjectAt(idx);
    if (param) {
     nsAutoString name;
     nsAutoString value;
     param->GetAttribute(NS_LITERAL_STRING("name"), name); 
     param->GetAttribute(NS_LITERAL_STRING("value"), value);

     FixUpURLS(name, value);

     








            
     name.Trim(" \n\r\t\b", PR_TRUE, PR_TRUE, PR_FALSE);
     value.Trim(" \n\r\t\b", PR_TRUE, PR_TRUE, PR_FALSE);
     mCachedAttrParamNames [mNumCachedAttrs + 1 + c] = ToNewUTF8String(name);
     mCachedAttrParamValues[mNumCachedAttrs + 1 + c] = ToNewUTF8String(value);
     c++;                                                      
    }
  }

  return NS_OK;
}




#ifdef XP_MACOSX

static void InitializeEventRecord(EventRecord* event)
{
    memset(event, 0, sizeof(EventRecord));
    ::GetGlobalMouse(&event->where);
    event->when = ::TickCount();
    event->modifiers = ::GetCurrentEventKeyModifiers();
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

  mInstance->GetValue(nsPluginInstanceVariable_DrawingModel,
                      (void *)&drawingModel);

  return drawingModel;
}

void nsPluginInstanceOwner::GUItoMacEvent(const nsGUIEvent& anEvent, EventRecord* origEvent, EventRecord& aMacEvent)
{
  nsPresContext* presContext = mOwner ? mOwner->PresContext() : nsnull;
  InitializeEventRecord(&aMacEvent);
  switch (anEvent.message) {
    case NS_FOCUS_EVENT_START:   
        aMacEvent.what = nsPluginEventType_GetFocusEvent;
        if (presContext) {
            nsIContent* content = mContent;
            if (content)
                content->SetFocus(presContext);
        }
        break;

    case NS_BLUR_CONTENT:
        aMacEvent.what = nsPluginEventType_LoseFocusEvent;
        if (presContext) {
            nsIContent* content = mContent;
            if (content)
                content->RemoveFocus(presContext);
        }
        break;

    case NS_MOUSE_MOVE:
    case NS_MOUSE_ENTER:
        if (origEvent)
          aMacEvent = *origEvent;
        aMacEvent.what = nsPluginEventType_AdjustCursorEvent;
        break;
  }
}

#endif

nsresult nsPluginInstanceOwner::ScrollPositionWillChange(nsIScrollableView* aScrollable, nscoord aX, nscoord aY)
{
#ifdef XP_MACOSX
    CancelTimer();

    if (mInstance) {
        nsCOMPtr<nsIPluginWidget> pluginWidget = do_QueryInterface(mWidget);
        if (pluginWidget && NS_SUCCEEDED(pluginWidget->StartDrawPlugin())) {
            EventRecord scrollEvent;
            InitializeEventRecord(&scrollEvent);
            scrollEvent.what = nsPluginEventType_ScrollingBeginsEvent;
    
            WindowRef window = FixUpPluginWindow(ePluginPaintDisable);
            if (window) {
              nsPluginEvent pluginEvent = { &scrollEvent, nsPluginPlatformWindowRef(window) };
              PRBool eventHandled = PR_FALSE;
              mInstance->HandleEvent(&pluginEvent, &eventHandled);
            }
            pluginWidget->EndDrawPlugin();
        }
    }
#endif
    return NS_OK;
}

nsresult nsPluginInstanceOwner::ScrollPositionDidChange(nsIScrollableView* aScrollable, nscoord aX, nscoord aY)
{
#ifdef XP_MACOSX
    if (mInstance) {
      nsCOMPtr<nsIPluginWidget> pluginWidget = do_QueryInterface(mWidget);
      if (pluginWidget && NS_SUCCEEDED(pluginWidget->StartDrawPlugin())) {
        EventRecord scrollEvent;
        InitializeEventRecord(&scrollEvent);
        scrollEvent.what = nsPluginEventType_ScrollingEndsEvent;
  
        WindowRef window = FixUpPluginWindow(ePluginPaintEnable);
        if (window) {
          nsPluginEvent pluginEvent = { &scrollEvent, nsPluginPlatformWindowRef(window) };
          PRBool eventHandled = PR_FALSE;
          mInstance->HandleEvent(&pluginEvent, &eventHandled);
        }
        pluginWidget->EndDrawPlugin();
      }

      
      
      if (mWidget)
        mWidget->Invalidate(PR_FALSE);
    }
#endif

    StartTimer(NORMAL_PLUGIN_DELAY);
    return NS_OK;
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
  if (!mPluginWindow || nsPluginWindowType_Window == mPluginWindow->type) {
    
    return aFocusEvent->PreventDefault(); 
  }
#endif

  nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(aFocusEvent));
  if (privateEvent) {
    nsEvent * theEvent;
    privateEvent->GetInternalNSEvent(&theEvent);
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


nsresult nsPluginInstanceOwner::DragEnter(nsIDOMEvent* aMouseEvent)
{
  if (mInstance) {
    
    aMouseEvent->PreventDefault();
    aMouseEvent->StopPropagation();
  }

  return NS_OK;
}

nsresult nsPluginInstanceOwner::DragOver(nsIDOMEvent* aMouseEvent)
{
  if (mInstance) {
    
    aMouseEvent->PreventDefault();
    aMouseEvent->StopPropagation();
  }

  return NS_OK;
}

nsresult nsPluginInstanceOwner::DragExit(nsIDOMEvent* aMouseEvent)
{
  if (mInstance) {
    
    aMouseEvent->PreventDefault();
    aMouseEvent->StopPropagation();
  }

  return NS_OK;
}

nsresult nsPluginInstanceOwner::DragDrop(nsIDOMEvent* aMouseEvent)
{
  if (mInstance) {
    
    aMouseEvent->PreventDefault();
    aMouseEvent->StopPropagation();
  }

  return NS_OK;
}

nsresult nsPluginInstanceOwner::DragGesture(nsIDOMEvent* aMouseEvent)
{
  if (mInstance) {
    
    aMouseEvent->PreventDefault();
    aMouseEvent->StopPropagation();
  }

  return NS_OK;
}

nsresult nsPluginInstanceOwner::Drag(nsIDOMEvent* aMouseEvent)
{
  if (mInstance) {
    
    aMouseEvent->PreventDefault();
    aMouseEvent->StopPropagation();
  }

  return NS_OK;
}

nsresult nsPluginInstanceOwner::DragEnd(nsIDOMEvent* aMouseEvent)
{
  if (mInstance) {
    
    aMouseEvent->PreventDefault();
    aMouseEvent->StopPropagation();
  }

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

  
  
  
  nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(aKeyEvent));
  if (privateEvent) {
    nsEvent *theEvent;
    privateEvent->GetInternalNSEvent(&theEvent);
    const nsGUIEvent *guiEvent = (nsGUIEvent*)theEvent;
    const EventRecord *ev = (EventRecord*)(guiEvent->nativeMsg); 
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
#else
  if (mInstance) {
    
    
    aKeyEvent->PreventDefault();
    aKeyEvent->StopPropagation();
  }
  return NS_OK;
#endif
}

nsresult nsPluginInstanceOwner::DispatchKeyToPlugin(nsIDOMEvent* aKeyEvent)
{
#ifndef XP_MACOSX
  if (!mPluginWindow || nsPluginWindowType_Window == mPluginWindow->type)
    return aKeyEvent->PreventDefault(); 
  
#endif

  if (mInstance) {
    nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(aKeyEvent));
    if (privateEvent) {
      nsKeyEvent* keyEvent = nsnull;
      privateEvent->GetInternalNSEvent((nsEvent**)&keyEvent);
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
  if (!mPluginWindow || nsPluginWindowType_Window == mPluginWindow->type)
    return aMouseEvent->PreventDefault(); 
  
#endif

  
  if (!mWidgetVisible)
    return NS_OK;

  nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(aMouseEvent));
  if (privateEvent) {
    nsMouseEvent* mouseEvent = nsnull;
    privateEvent->GetInternalNSEvent((nsEvent**)&mouseEvent);
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
  if (!mPluginWindow || nsPluginWindowType_Window == mPluginWindow->type)
    return aMouseEvent->PreventDefault(); 
  
#endif

  
  
  if (mOwner && mPluginWindow &&
      mPluginWindow->type == nsPluginWindowType_Drawable) {
    mContent->SetFocus(mOwner->PresContext());
  }

  nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(aMouseEvent));
  if (privateEvent) {
    nsMouseEvent* mouseEvent = nsnull;
    privateEvent->GetInternalNSEvent((nsEvent**)&mouseEvent);
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
  if (!mPluginWindow || nsPluginWindowType_Window == mPluginWindow->type)
    return aMouseEvent->PreventDefault(); 
  
#endif

  
  if (!mWidgetVisible)
    return NS_OK;

  nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(aMouseEvent));
  if (privateEvent) {
    nsMouseEvent* mouseEvent = nsnull;
    privateEvent->GetInternalNSEvent((nsEvent**)&mouseEvent);
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
  return NS_OK;
}

#ifdef MOZ_X11
static unsigned int XInputEventState(const nsInputEvent& anEvent)
{
  unsigned int state = 0;
  if(anEvent.isShift) state |= ShiftMask;
  if(anEvent.isControl) state |= ControlMask;
  if(anEvent.isAlt) state |= Mod1Mask;
  if(anEvent.isMeta) state |= Mod4Mask;
  return state;
}
#endif

nsEventStatus nsPluginInstanceOwner::ProcessEvent(const nsGUIEvent& anEvent)
{
  
  nsEventStatus rv = nsEventStatus_eIgnore;
  if (!mInstance || !mOwner)   
    return rv;

#ifdef XP_MACOSX
  
  if (mWidget) {
    nsCOMPtr<nsIPluginWidget> pluginWidget = do_QueryInterface(mWidget);
    if (pluginWidget && NS_SUCCEEDED(pluginWidget->StartDrawPlugin())) {
      EventRecord macEvent;
      EventRecord* event = (EventRecord*)anEvent.nativeMsg;
      if ((event == NULL) || (event->what == nullEvent)  || 
          (anEvent.message == NS_FOCUS_CONTENT)          || 
          (anEvent.message == NS_BLUR_CONTENT)           || 
          (anEvent.message == NS_MOUSE_MOVE)             ||
          (anEvent.message == NS_MOUSE_ENTER)) {
        GUItoMacEvent(anEvent, event, macEvent);
        event = &macEvent;
      }

      if (anEvent.message == NS_FOCUS_CONTENT) {
        
        
        
        ::DeactivateTSMDocument(::TSMGetActiveDocument());
      }

      PRBool eventHandled = PR_FALSE;
      WindowRef window = FixUpPluginWindow(ePluginPaintIgnore);
      if (window) {
        nsPluginEvent pluginEvent = { event, nsPluginPlatformWindowRef(window) };
        mInstance->HandleEvent(&pluginEvent, &eventHandled);
      }

      if (eventHandled && !(anEvent.eventStructType == NS_MOUSE_EVENT &&
                            anEvent.message == NS_MOUSE_BUTTON_DOWN &&
                            static_cast<const nsMouseEvent&>(anEvent).button ==
                              nsMouseEvent::eLeftButton &&
                            !mContentFocused))
        rv = nsEventStatus_eConsumeNoDefault;

      pluginWidget->EndDrawPlugin();
    }
  }
#endif

#ifdef XP_WIN
  
  nsPluginEvent * pPluginEvent = (nsPluginEvent *)anEvent.nativeMsg;
  
  
  nsPluginEvent pluginEvent;
  if (!pPluginEvent) {
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

  if (pPluginEvent) {
    PRBool eventHandled = PR_FALSE;
    mInstance->HandleEvent(pPluginEvent, &eventHandled);
    if (eventHandled)
      rv = nsEventStatus_eConsumeNoDefault;
  }
#endif

#ifdef MOZ_X11
  
  nsIWidget* widget = anEvent.widget;
  nsPluginEvent pluginEvent;
  pluginEvent.event.type = 0;

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

        
        const nsPresContext* presContext = mOwner->PresContext();
        nsPoint appPoint =
          nsLayoutUtils::GetEventCoordinatesRelativeTo(&anEvent, mOwner); 
        nsIntPoint pluginPoint(presContext->AppUnitsToDevPixels(appPoint.x),
                               presContext->AppUnitsToDevPixels(appPoint.y));
        const nsMouseEvent& mouseEvent =
          static_cast<const nsMouseEvent&>(anEvent);
        
        nsRect windowRect(anEvent.refPoint, nsSize(1, 1));
        nsRect rootPoint(-1,-1,1,1);
        if (widget)
          widget->WidgetToScreen(windowRect, rootPoint);
#ifdef MOZ_WIDGET_GTK2
        Window root = GDK_ROOT_WINDOW();
#else
        Window root = None; 
#endif

        switch (anEvent.message)
          {
          case NS_MOUSE_ENTER_SYNTH:
          case NS_MOUSE_EXIT_SYNTH:
            {
              XCrossingEvent& event = pluginEvent.event.xcrossing;
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
              XMotionEvent& event = pluginEvent.event.xmotion;
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
              XButtonEvent& event = pluginEvent.event.xbutton;
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
      if (anEvent.nativeMsg)
        {
          XKeyEvent &event = pluginEvent.event.xkey;
#ifdef MOZ_WIDGET_GTK2
          event.root = GDK_ROOT_WINDOW();
          event.time = anEvent.time;
          const GdkEventKey* gdkEvent =
            static_cast<const GdkEventKey*>(anEvent.nativeMsg);
          event.keycode = gdkEvent->hardware_keycode;
          event.state = gdkEvent->state;
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
            XFocusChangeEvent &event = pluginEvent.event.xfocus;
            event.type =
              anEvent.message == NS_FOCUS_CONTENT ? FocusIn : FocusOut;
            
            event.mode = -1;
            event.detail = NotifyDetailNone;
          }
          break;
        }
    }

  if (!pluginEvent.event.type) {
    PR_LOG(nsObjectFrameLM, PR_LOG_DEBUG,
           ("Unhandled event message %d with struct type %d\n",
            anEvent.message, anEvent.eventStructType));
    return rv;
  }

  
  XAnyEvent& event = pluginEvent.event.xany;
  event.display = widget ?
    static_cast<Display*>(widget->GetNativeData(NS_NATIVE_DISPLAY)) : nsnull;
  event.window = None; 
  
  event.serial = 0;
  event.send_event = False;

  PRBool eventHandled = PR_FALSE;
  mInstance->HandleEvent(&pluginEvent, &eventHandled);
  if (eventHandled)
      rv = nsEventStatus_eConsumeNoDefault;
#endif

  return rv;
}

nsresult
nsPluginInstanceOwner::Destroy()
{
  
  CancelTimer();

  
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

    
    target->RemoveEventListener(NS_LITERAL_STRING("dragdrop"), listener, PR_TRUE);
    target->RemoveEventListener(NS_LITERAL_STRING("dragover"), listener, PR_TRUE);
    target->RemoveEventListener(NS_LITERAL_STRING("dragexit"), listener, PR_TRUE);
    target->RemoveEventListener(NS_LITERAL_STRING("dragenter"), listener, PR_TRUE);
    target->RemoveEventListener(NS_LITERAL_STRING("draggesture"), listener, PR_TRUE);
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
#ifdef XP_WIN
  if (aDelayedStop && mWidget) {
    
    
    

    
    
    mWidget->Show(PR_FALSE);
    mWidget->Enable(PR_FALSE);

    
    
    
    mWidget->SetParent(nsnull);

    mDestroyWidget = PR_TRUE;
  }
#endif

  
  nsIFrame* parentWithView = mOwner->GetAncestorWithView();
  nsIView* curView = parentWithView ? parentWithView->GetView() : nsnull;
  while (curView) {
    nsIScrollableView* scrollingView = curView->ToScrollableView();
    if (scrollingView)
      scrollingView->RemoveScrollPositionListener((nsIScrollPositionListener *)this);
    
    curView = curView->GetParent();
  }
}



#ifdef XP_MACOSX
void nsPluginInstanceOwner::Paint(const nsRect& aDirtyRect)
{
  if (!mInstance || !mOwner)
    return;
 
#ifdef DO_DIRTY_INTERSECT   
  nsPoint rel(aDirtyRect.x, aDirtyRect.y);
  nsPoint abs(0,0);
  nsCOMPtr<nsIWidget> containerWidget;

  
  ConvertRelativeToWindowAbsolute(mOwner, rel, abs, *getter_AddRefs(containerWidget));

  nsRect absDirtyRect = nsRect(abs.x, abs.y, aDirtyRect.width, aDirtyRect.height);

  
  nsIntRect absDirtyRectInPixels;
  ConvertAppUnitsToPixels(*mOwner->GetPresContext(), absDirtyRect,
                          absDirtyRectInPixels);
#endif

  nsCOMPtr<nsIPluginWidget> pluginWidget = do_QueryInterface(mWidget);
  if (pluginWidget && NS_SUCCEEDED(pluginWidget->StartDrawPlugin())) {
    WindowRef window = FixUpPluginWindow(ePluginPaintEnable);
    if (window) {
      EventRecord updateEvent;
      InitializeEventRecord(&updateEvent);
      updateEvent.what = updateEvt;
      updateEvent.message = UInt32(window);
    
      nsPluginEvent pluginEvent = { &updateEvent, nsPluginPlatformWindowRef(window) };
      PRBool eventHandled = PR_FALSE;
      mInstance->HandleEvent(&pluginEvent, &eventHandled);
    }
    pluginWidget->EndDrawPlugin();
  }
}
#endif

#ifdef XP_WIN
void nsPluginInstanceOwner::Paint(const nsRect& aDirtyRect, HDC ndc)
{
  if (!mInstance || !mOwner)
    return;

  nsPluginWindow * window;
  GetWindow(window);
  nsRect relDirtyRect = nsRect(aDirtyRect.x, aDirtyRect.y, aDirtyRect.width, aDirtyRect.height);
  nsIntRect relDirtyRectInPixels;
  ConvertAppUnitsToPixels(*mOwner->PresContext(), relDirtyRect,
                          relDirtyRectInPixels);

  
  
  RECT drc;
  drc.left   = relDirtyRectInPixels.x + window->x;
  drc.top    = relDirtyRectInPixels.y + window->y;
  drc.right  = drc.left + relDirtyRectInPixels.width;
  drc.bottom = drc.top + relDirtyRectInPixels.height;

  nsPluginEvent pluginEvent;
  pluginEvent.event = WM_PAINT;
  pluginEvent.wParam = (uint32)ndc;
  pluginEvent.lParam = (uint32)&drc;
  PRBool eventHandled = PR_FALSE;
  mInstance->HandleEvent(&pluginEvent, &eventHandled);
}
#endif

#ifdef XP_OS2
void nsPluginInstanceOwner::Paint(const nsRect& aDirtyRect, HPS aHPS)
{
  if (!mInstance || !mOwner)
    return;

  nsPluginWindow * window;
  GetWindow(window);
  nsRect relDirtyRect = nsRect(aDirtyRect.x, aDirtyRect.y, aDirtyRect.width, aDirtyRect.height);
  nsIntRect relDirtyRectInPixels;
  ConvertAppUnitsToPixels(*mOwner->PresContext(), relDirtyRect,
                          relDirtyRectInPixels);

  
  
  RECTL rectl;
  rectl.xLeft   = relDirtyRectInPixels.x + window->x;
  rectl.yBottom = relDirtyRectInPixels.y + window->y;
  rectl.xRight  = rectl.xLeft + relDirtyRectInPixels.width;
  rectl.yTop    = rectl.yBottom + relDirtyRectInPixels.height;

  nsPluginEvent pluginEvent;
  pluginEvent.event = WM_PAINT;
  pluginEvent.wParam = (uint32)aHPS;
  pluginEvent.lParam = (uint32)&rectl;
  PRBool eventHandled = PR_FALSE;
  mInstance->HandleEvent(&pluginEvent, &eventHandled);
}
#endif

#ifdef MOZ_X11
void nsPluginInstanceOwner::Paint(nsIRenderingContext& aRenderingContext,
                                  const nsRect& aDirtyRect)
{
  if (!mInstance || !mOwner)
    return;
 
  nsPluginWindow* window;
  GetWindow(window);

  nsIntRect dirtyRectInPixels;
  ConvertAppUnitsToPixels(*mOwner->PresContext(), aDirtyRect,
                          dirtyRectInPixels);
  
  
  nsIntRect pluginDirtyRect;
  if (!pluginDirtyRect.IntersectRect(nsIntRect(0, 0, window->width, window->height), dirtyRectInPixels))
    return;

  Renderer renderer(window, mInstance, pluginDirtyRect);
  PRUint32 rendererFlags =
    Renderer::DRAW_SUPPORTS_OFFSET |
    Renderer::DRAW_SUPPORTS_CLIP_RECT |
    Renderer::DRAW_SUPPORTS_NONDEFAULT_VISUAL |
    Renderer::DRAW_SUPPORTS_ALTERNATE_DISPLAY;

  PRBool transparent = PR_TRUE;
  mInstance->GetValue(nsPluginInstanceVariable_TransparentBool,
                      (void *)&transparent);
  if (!transparent)
    rendererFlags |= Renderer::DRAW_IS_OPAQUE;

  gfxContext* ctx =
    static_cast<gfxContext*>
               (aRenderingContext.GetNativeGraphicData(nsIRenderingContext::NATIVE_THEBES_CONTEXT));

  
  
  
  
  NPSetWindowCallbackStruct* ws_info = 
    static_cast<NPSetWindowCallbackStruct*>(window->ws_info);
  renderer.Draw(ws_info->display, ctx, window->width, window->height,
                rendererFlags, nsnull);
}

nsresult
nsPluginInstanceOwner::Renderer::NativeDraw(Display* dpy, Drawable drawable,
                                            Visual* visual,
                                            short offsetX, short offsetY,
                                            XRectangle* clipRects,
                                            PRUint32 numClipRects)
{
  
  PRBool doupdatewindow = PR_FALSE;

  if (mWindow->x != offsetX || mWindow->y != offsetY) {
    mWindow->x = offsetX;
    mWindow->y = offsetY;
    doupdatewindow = PR_TRUE;
  }

  NS_ASSERTION(numClipRects <= 1, "We don't support multiple clip rectangles!");
  nsPluginRect newClipRect;
  if (numClipRects) {
    newClipRect.left = clipRects[0].x;
    newClipRect.top = clipRects[0].y;
    newClipRect.right  = clipRects[0].x + clipRects[0].width;
    newClipRect.bottom = clipRects[0].y + clipRects[0].height;
  }
  else {
    
    NS_ASSERTION(offsetX >= 0 && offsetY >= 0,
                 "Clip rectangle offsets are negative!");
    newClipRect.left = offsetX;
    newClipRect.top  = offsetY;
    newClipRect.right  = offsetX + mWindow->width;
    newClipRect.bottom = offsetY + mWindow->height;
  }

  if (mWindow->clipRect.left    != newClipRect.left   ||
      mWindow->clipRect.top     != newClipRect.top    ||
      mWindow->clipRect.right   != newClipRect.right  ||
      mWindow->clipRect.bottom  != newClipRect.bottom) {
    mWindow->clipRect = newClipRect;
    doupdatewindow = PR_TRUE;
  }

  NPSetWindowCallbackStruct* ws_info = 
    static_cast<NPSetWindowCallbackStruct*>(mWindow->ws_info);
  if ( ws_info->visual != visual) {
    
    
    
    
    NS_ASSERTION(ws_info->visual == visual,
                 "Visual changed: colormap may not match");
    ws_info->visual = visual;
    doupdatewindow = PR_TRUE;
  }

  if (doupdatewindow)
      mInstance->SetWindow(mWindow);

  nsPluginEvent pluginEvent;
  XGraphicsExposeEvent& exposeEvent = pluginEvent.event.xgraphicsexpose;
  
  exposeEvent.type = GraphicsExpose;
  exposeEvent.display = dpy;
  exposeEvent.drawable = drawable;
  exposeEvent.x = mDirtyRect.x + offsetX;
  exposeEvent.y = mDirtyRect.y + offsetY;
  exposeEvent.width  = mDirtyRect.width;
  exposeEvent.height = mDirtyRect.height;
  exposeEvent.count = 0;
  
  exposeEvent.serial = 0;
  exposeEvent.send_event = False;
  exposeEvent.major_code = 0;
  exposeEvent.minor_code = 0;

  PRBool eventHandled = PR_FALSE;
  mInstance->HandleEvent(&pluginEvent, &eventHandled);

  return NS_OK;
}
#endif



NS_IMETHODIMP nsPluginInstanceOwner::Notify(nsITimer* )
{
#ifdef XP_MACOSX
    
    
    
    if (mInstance) {
        nsCOMPtr<nsIPluginWidget> pluginWidget = do_QueryInterface(mWidget);
        if (pluginWidget && NS_SUCCEEDED(pluginWidget->StartDrawPlugin())) {
            WindowRef window = FixUpPluginWindow(ePluginPaintIgnore);
            if (window) {
                EventRecord idleEvent;
                InitializeEventRecord(&idleEvent);
                idleEvent.what = nullEvent;
                    
                
                
                if (!mWidgetVisible)
                    idleEvent.where.h = idleEvent.where.v = 20000;
    
                nsPluginEvent pluginEvent = { &idleEvent, nsPluginPlatformWindowRef(window) };
    
                PRBool eventHandled = PR_FALSE;
                mInstance->HandleEvent(&pluginEvent, &eventHandled);
            }
            
            pluginWidget->EndDrawPlugin();
       }
    }
#endif
    return NS_OK;
}

void nsPluginInstanceOwner::StartTimer(unsigned int aDelay)
{
#ifdef XP_MACOSX
    nsresult rv;

    
    if (!mPluginTimer) {
      mPluginTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
      if (NS_SUCCEEDED(rv))
        mPluginTimer->InitWithCallback(this, aDelay, nsITimer::TYPE_REPEATING_SLACK);
    }
#endif
}

void nsPluginInstanceOwner::CancelTimer()
{
    if (mPluginTimer) {
        mPluginTimer->Cancel();
        mPluginTimer = nsnull;
    }
}

nsresult nsPluginInstanceOwner::Init(nsPresContext* aPresContext,
                                     nsObjectFrame* aFrame,
                                     nsIContent*    aContent)
{
  mLastEventloopNestingLevel = 0;
  nsCOMPtr<nsIAppShell> appShell = do_GetService(kAppShellCID);
  if (appShell) {
    appShell->GetEventloopNestingLevel(&mLastEventloopNestingLevel);
  }

  PR_LOG(nsObjectFrameLM, PR_LOG_DEBUG,
         ("nsPluginInstanceOwner::Init() called on %p for frame %p\n", this,
          aFrame));

  mOwner = aFrame;
  mContent = aContent;

  nsWeakFrame weakFrame(aFrame);

  
  
  
  
  aPresContext->EnsureVisible(PR_TRUE);

  if (!weakFrame.IsAlive()) {
    PR_LOG(nsObjectFrameLM, PR_LOG_DEBUG,
           ("nsPluginInstanceOwner::Init's EnsureVisible() call destroyed "
            "instance owner %p\n", this));

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

    
    target->AddEventListener(NS_LITERAL_STRING("dragdrop"), listener, PR_TRUE);
    target->AddEventListener(NS_LITERAL_STRING("dragover"), listener, PR_TRUE);
    target->AddEventListener(NS_LITERAL_STRING("dragexit"), listener, PR_TRUE);
    target->AddEventListener(NS_LITERAL_STRING("dragenter"), listener, PR_TRUE);
    target->AddEventListener(NS_LITERAL_STRING("draggesture"), listener, PR_TRUE);
  }
  
  
  
  
  nsIFrame* parentWithView = mOwner->GetAncestorWithView();
  nsIView* curView = parentWithView ? parentWithView->GetView() : nsnull;
  while (curView) {
    nsIScrollableView* scrollingView = curView->ToScrollableView();
    if (scrollingView)
      scrollingView->AddScrollPositionListener((nsIScrollPositionListener *)this);
    
    curView = curView->GetParent();
  }

  return NS_OK; 
}

nsPluginPort* nsPluginInstanceOwner::GetPluginPort()
{


  nsPluginPort* result = NULL;
  if (mWidget) {
#ifdef XP_WIN
    if (mPluginWindow && mPluginWindow->type == nsPluginWindowType_Drawable)
      result = (nsPluginPort*) mWidget->GetNativeData(NS_NATIVE_GRAPHIC);
    else
#endif
#ifdef XP_MACOSX
    if (GetDrawingModel() == NPDrawingModelCoreGraphics)
      result = (nsPluginPort*) mWidget->GetNativeData(NS_NATIVE_PLUGIN_PORT_CG);
    else
#endif
      result = (nsPluginPort*) mWidget->GetNativeData(NS_NATIVE_PLUGIN_PORT);
  }
  return result;
}

void nsPluginInstanceOwner::ReleasePluginPort(nsPluginPort * pluginPort)
{
#ifdef XP_WIN
  if (mWidget && mPluginWindow &&
      mPluginWindow->type == nsPluginWindowType_Drawable) {
    mWidget->FreeNativeData((HDC)pluginPort, NS_NATIVE_GRAPHIC);
  }
#endif
}

NS_IMETHODIMP nsPluginInstanceOwner::CreateWidget(void)
{
  NS_ENSURE_TRUE(mPluginWindow, NS_ERROR_NULL_POINTER);

  nsIView   *view;
  nsresult  rv = NS_ERROR_FAILURE;

  if (mOwner) {
    

    view = mOwner->GetView();

    if (!view || !mWidget) {
      PRBool windowless = PR_FALSE;

      mInstance->GetValue(nsPluginInstanceVariable_WindowlessBool,
                          (void *)&windowless);

      
      nsPresContext* context = mOwner->PresContext();
      rv = mOwner->CreateWidget(context->DevPixelsToAppUnits(mPluginWindow->width),
                                context->DevPixelsToAppUnits(mPluginWindow->height),
                                windowless);
      if (NS_OK == rv) {
        view = mOwner->GetView();

        if (view) {
          mWidget = view->GetWidget();
        }

        if (PR_TRUE == windowless) {
          mPluginWindow->type = nsPluginWindowType_Drawable;

          
          
          
          
          mPluginWindow->window = nsnull;
        } else if (mWidget) {
          mWidget->Resize(mPluginWindow->width, mPluginWindow->height,
                          PR_FALSE);

          
          
          mPluginWindow->type = nsPluginWindowType_Window;
          mPluginWindow->window = GetPluginPort();

          
          StartTimer(NORMAL_PLUGIN_DELAY);

          
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
  mPluginHost = aHost;
}

#if defined(XP_WIN) || (defined(DO_DIRTY_INTERSECT) && defined(XP_MACOSX)) || defined(MOZ_X11) || defined(XP_OS2)

static void ConvertAppUnitsToPixels(const nsPresContext& aPresContext, const nsRect& aTwipsRect, nsIntRect& aPixelRect)
{
  aPixelRect.x = aPresContext.AppUnitsToDevPixels(aTwipsRect.x);
  aPixelRect.y = aPresContext.AppUnitsToDevPixels(aTwipsRect.y);
  aPixelRect.width = aPresContext.AppUnitsToDevPixels(aTwipsRect.width);
  aPixelRect.height = aPresContext.AppUnitsToDevPixels(aTwipsRect.height);
}
#endif

  
#ifdef XP_MACOSX

#ifdef DO_DIRTY_INTERSECT


static void ConvertRelativeToWindowAbsolute(nsIFrame*   aFrame,
                                            nsPoint&    aRel, 
                                            nsPoint&    aAbs,
                                            nsIWidget*& aContainerWidget)
{
  
  nsIView *view = aFrame->GetView();
  if (!view) {
    aAbs.x = 0;
    aAbs.y = 0;
    
    aFrame->GetOffsetFromView(aAbs, &view);
  } else {
    
    aAbs = aFrame->GetPosition();
  }

  NS_ASSERTION(view, "the object frame does not have a view");
  if (view) {
    
    nsPoint viewOffset;
    aContainerWidget = view->GetNearestWidget(&viewOffset);
    NS_IF_ADDREF(aContainerWidget);
    aAbs += viewOffset;
  }

  
  aAbs += aRel;
}
#endif 

WindowRef nsPluginInstanceOwner::FixUpPluginWindow(PRInt32 inPaintState)
{
  if (!mWidget || !mPluginWindow || !mInstance || !mOwner)
    return nsnull;

  nsPluginPort* pluginPort = GetPluginPort(); 

  if (!pluginPort)
    return nsnull;

  NPDrawingModel drawingModel = GetDrawingModel();

  
  PRBool isVisible =
    mOwner->GetView()->GetVisibility() == nsViewVisibility_kShow;

  nsCOMPtr<nsIPluginWidget> pluginWidget = do_QueryInterface(mWidget);
  
  nsPoint pluginOrigin;
  nsRect widgetClip;
  PRBool widgetVisible;
  pluginWidget->GetPluginClipRect(widgetClip, pluginOrigin,  widgetVisible);
  
  

  isVisible &= widgetVisible;
  if (!isVisible)
    widgetClip.Empty();

#ifndef NP_NO_QUICKDRAW
  
  if (drawingModel == NPDrawingModelQuickDraw) {
    mPluginWindow->x = -pluginPort->qdPort.portx;
    mPluginWindow->y = -pluginPort->qdPort.porty;
  }
  else if (drawingModel == NPDrawingModelCoreGraphics)
#endif
  {
    
    
    
    
    
    nsRect geckoBounds;
    mWidget->GetBounds(geckoBounds);
    
    
    geckoBounds.x = 0;
    geckoBounds.y = 0;
    nsRect geckoScreenCoords;
    mWidget->WidgetToScreen(geckoBounds, geckoScreenCoords);

    Rect windowRect;
    WindowRef window = (WindowRef)pluginPort->cgPort.window;
    ::GetWindowBounds(window, kWindowStructureRgn, &windowRect);

    mPluginWindow->x = geckoScreenCoords.x - windowRect.left;
    mPluginWindow->y = geckoScreenCoords.y - windowRect.top;
  }

  nsPluginRect oldClipRect = mPluginWindow->clipRect;
  
  
  mPluginWindow->clipRect.top    = widgetClip.y;
  mPluginWindow->clipRect.left   = widgetClip.x;

  mWidgetVisible = isVisible;

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
    
    CancelTimer();
    if (mPluginWindow->clipRect.left == mPluginWindow->clipRect.right ||
        mPluginWindow->clipRect.top == mPluginWindow->clipRect.bottom) {
      StartTimer(HIDDEN_PLUGIN_DELAY);
    }
    else {
      StartTimer(NORMAL_PLUGIN_DELAY);
    }
  }

#ifndef NP_NO_QUICKDRAW
  if (drawingModel == NPDrawingModelQuickDraw)
    return ::GetWindowFromPort(pluginPort->qdPort.port);
#endif

  if (drawingModel == NPDrawingModelCoreGraphics)
    return pluginPort->cgPort.window;

  return nsnull;
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


