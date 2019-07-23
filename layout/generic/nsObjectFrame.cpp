















































#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsWidgetsCID.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsIDOMKeyListener.h"
#include "nsIDOMDragEvent.h"
#ifdef MOZ_X11
#ifdef MOZ_WIDGET_QT
#include <QWidget>
#include <QX11Info>
#endif
#endif
#include "nsIPluginHost.h"
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
#include "nsIPluginInstance.h"
#include "nsIPluginTagInfo.h"
#include "plstr.h"
#include "nsILinkHandler.h"
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
#include "nsIDOMElement.h"
#include "nsContentUtils.h"
#include "nsDisplayList.h"
#include "nsAttrName.h"
#include "nsDataHashtable.h"
#include "nsDOMClassInfo.h"
#include "nsFocusManager.h"
#include "nsLayoutUtils.h"
#include "nsFrameManager.h"
#include "nsComponentManagerUtils.h"
#include "nsIObserverService.h"


#include "nsIScriptGlobalObject.h"
#include "nsIScriptContext.h"
#include "nsIXPConnect.h"
#include "nsIXPCScriptable.h"
#include "nsIClassInfo.h"
#include "nsIDOMClientRect.h"

#include "nsObjectFrame.h"
#include "nsIObjectFrame.h"
#include "nsPluginNativeWindow.h"
#include "nsIPluginDocument.h"

#include "nsThreadUtils.h"

#include "gfxContext.h"

#ifdef XP_WIN
#include "gfxWindowsNativeDrawing.h"
#include "gfxWindowsSurface.h"
#endif

#include "gfxImageSurface.h"


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
#include "nsPluginUtilsOSX.h"
#endif

#ifdef MOZ_X11
#include <X11/Xlib.h>

enum { XKeyPress = KeyPress };
#ifdef KeyPress
#undef KeyPress
#endif

#if defined(MOZ_PLATFORM_HILDON) && defined(MOZ_WIDGET_GTK2)
#define MOZ_COMPOSITED_PLUGINS 1

#include "gfxXlibSurface.h"

#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>


#endif

#ifdef MOZ_WIDGET_GTK2
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#endif
#endif

#ifdef MOZ_WIDGET_GTK2
#include "gfxGdkNativeRenderer.h"
#endif

#ifdef MOZ_WIDGET_QT
#include "gfxQtNativeRenderer.h"
#endif

#ifdef XP_WIN
#include <wtypes.h>
#include <winuser.h>
#ifdef MOZ_IPC
#define NS_OOPP_DOUBLEPASS_MSGID TEXT("MozDoublePassMsg")
#endif
#endif

#ifdef XP_OS2
#define INCL_PM
#define INCL_GPI
#include <os2.h>
#endif

#ifdef CreateEvent 
#undef CreateEvent
#endif

#ifdef PR_LOGGING 
static PRLogModuleInfo *nsObjectFrameLM = PR_NewLogModule("nsObjectFrame");
#endif 

#define NORMAL_PLUGIN_DELAY 20

#define HIDDEN_PLUGIN_DELAY 125




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


class nsPluginInstanceOwner : public nsIPluginInstanceOwner,
                              public nsIPluginTagInfo,
                              public nsITimerCallback,
                              public nsIDOMMouseListener,
                              public nsIDOMMouseMotionListener,
                              public nsIDOMKeyListener,
                              public nsIDOMFocusListener,
                              public nsIScrollPositionListener
{
public:
  nsPluginInstanceOwner();
  virtual ~nsPluginInstanceOwner();

  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIPLUGININSTANCEOWNER

  NS_IMETHOD GetURL(const char *aURL, const char *aTarget, void *aPostData, 
                    PRUint32 aPostDataLen, void *aHeadersData, 
                    PRUint32 aHeadersDataLen, PRBool isFile = PR_FALSE);

  NS_IMETHOD ShowStatus(const PRUnichar *aStatusMsg);

  NPError    ShowNativeContextMenu(NPMenu* menu, void* event);

  NPBool     ConvertPoint(double sourceX, double sourceY, NPCoordinateSpace sourceSpace,
                          double *destX, double *destY, NPCoordinateSpace destSpace);

  
  NS_DECL_NSIPLUGINTAGINFO

  
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

  nsresult Destroy();  

  void PrepareToStop(PRBool aDelayedStop);
  
#ifdef XP_WIN
  void Paint(const RECT& aDirty, HDC aDC);
#elif defined(XP_MACOSX)
  void Paint(const gfxRect& aDirtyRect, CGContextRef cgContext);  
#elif defined(MOZ_X11) || defined(MOZ_DFB)
  void Paint(gfxContext* aContext,
             const gfxRect& aFrameRect,
             const gfxRect& aDirtyRect);
#elif defined(XP_OS2)
  void Paint(const nsRect& aDirtyRect, HPS aHPS);
#endif

  
  NS_DECL_NSITIMERCALLBACK
  
  void CancelTimer();
  void StartTimer(unsigned int aDelay);

  
  NS_IMETHOD ScrollPositionWillChange(nsIScrollableView* aScrollable, nscoord aX, nscoord aY);
  virtual void ViewPositionDidChange(nsIScrollableView* aScrollable,
                                     nsTArray<nsIWidget::Configuration>* aConfigurations) {}
  NS_IMETHOD ScrollPositionDidChange(nsIScrollableView* aScrollable, nscoord aX, nscoord aY);

  

  nsresult Init(nsPresContext* aPresContext, nsObjectFrame* aFrame,
                nsIContent* aContent);

  void* GetPluginPort();
  void ReleasePluginPort(void* pluginPort);

  void SetPluginHost(nsIPluginHost* aHost);

  nsEventStatus ProcessEvent(const nsGUIEvent & anEvent);

#ifdef XP_MACOSX
  NPDrawingModel GetDrawingModel();
  NPEventModel GetEventModel();
  void* FixUpPluginWindow(PRInt32 inPaintState);
  
  
  void SetPluginPortChanged(PRBool aState) { mPluginPortChanged = aState; }
  
  
  void* GetPluginPortCopy();
  
  
  
  
  void* SetPluginPortAndDetectChange();
  
  
  
  
  
  void BeginCGPaint();
  void EndCGPaint();
#endif

  void SetOwner(nsObjectFrame *aOwner)
  {
    mObjectFrame = aOwner;
  }

  PRUint32 GetLastEventloopNestingLevel() const {
    return mLastEventloopNestingLevel; 
  }

  static PRUint32 GetEventloopNestingLevel();
      
  void ConsiderNewEventloopNestingLevel() {
    PRUint32 currentLevel = GetEventloopNestingLevel();

    if (currentLevel < mLastEventloopNestingLevel) {
      mLastEventloopNestingLevel = currentLevel;
    }
  }

  const char* GetPluginName()
  {
    if (mInstance && mPluginHost) {
      const char* name = NULL;
      if (NS_SUCCEEDED(mPluginHost->GetPluginName(mInstance, &name)) && name)
        return name;
    }
    return "";
  }

  PRBool SendNativeEvents()
  {
#ifdef XP_WIN
    return MatchPluginName("Shockwave Flash");
#else
    return PR_FALSE;
#endif
  }

  PRBool MatchPluginName(const char *aPluginName)
  {
    return strncmp(GetPluginName(), aPluginName, strlen(aPluginName)) == 0;
  }

#if defined(MOZ_PLATFORM_HILDON) && defined(MOZ_WIDGET_GTK2)
  nsresult SetAbsoluteScreenPosition(nsIDOMElement* element,
                                     nsIDOMClientRect* position,
                                     nsIDOMClientRect* clip);
#endif

private:
  void FixUpURLS(const nsString &name, nsAString &value);

  nsPluginNativeWindow       *mPluginWindow;
  nsCOMPtr<nsIPluginInstance> mInstance;
  nsObjectFrame              *mObjectFrame; 
  nsCOMPtr<nsIContent>        mContent;
  nsCString                   mDocumentBase;
  char                       *mTagText;
  nsCOMPtr<nsIWidget>         mWidget;
  nsCOMPtr<nsITimer>          mPluginTimer;
  nsCOMPtr<nsIPluginHost>     mPluginHost;

#ifdef XP_MACOSX
  NP_CGContext                mCGPluginPortCopy;
  NP_Port                     mQDPluginPortCopy;
  PRInt32                     mInCGPaintLevel;
#endif

  
  
  
  PRUint32                    mLastEventloopNestingLevel;
  PRPackedBool                mContentFocused;
  PRPackedBool                mWidgetVisible;    
  PRPackedBool                mPluginPortChanged;

  
  
  PRPackedBool                mDestroyWidget;
  PRPackedBool                mTimerCanceled;
  PRUint16          mNumCachedAttrs;
  PRUint16          mNumCachedParams;
  char              **mCachedAttrParamNames;
  char              **mCachedAttrParamValues;

#ifdef MOZ_COMPOSITED_PLUGINS
  nsIntPoint        mLastPoint;
#endif

#ifdef XP_MACOSX
  NPEventModel mEventModel;
#endif

  
  nsRefPtr<nsPluginDOMContextMenuListener> mCXMenuListener;

  nsresult DispatchKeyToPlugin(nsIDOMEvent* aKeyEvent);
  nsresult DispatchMouseToPlugin(nsIDOMEvent* aMouseEvent);
  nsresult DispatchFocusToPlugin(nsIDOMEvent* aFocusEvent);

  nsresult EnsureCachedAttrParamArrays();

#ifdef MOZ_COMPOSITED_PLUGINS
  nsEventStatus ProcessEventX11Composited(const nsGUIEvent & anEvent);
#endif

#if defined(MOZ_WIDGET_GTK2)
  class Renderer : public gfxGdkNativeRenderer {
  public:
    Renderer(NPWindow* aWindow, nsIPluginInstance* aInstance,
             const nsIntSize& aPluginSize, const nsIntRect& aDirtyRect)
      : mWindow(aWindow), mInstance(aInstance),
        mPluginSize(aPluginSize), mDirtyRect(aDirtyRect)
    {}
    virtual nsresult NativeDraw(GdkDrawable * drawable, short offsetX, 
            short offsetY, GdkRectangle * clipRects, PRUint32 numClipRects);
  private:
    NPWindow* mWindow;
    nsIPluginInstance* mInstance;
    const nsIntSize& mPluginSize;
    const nsIntRect& mDirtyRect;
  };
#elif defined(MOZ_WIDGET_QT)
  class Renderer : public gfxQtNativeRenderer {
  public:
    Renderer(NPWindow* aWindow, nsIPluginInstance* aInstance,
             const nsIntSize& aPluginSize, const nsIntRect& aDirtyRect)
      : mWindow(aWindow), mInstance(aInstance),
        mPluginSize(aPluginSize), mDirtyRect(aDirtyRect)
    {}
    virtual nsresult NativeDraw(QWidget * drawable, short offsetX, 
            short offsetY, QRect * clipRects, PRUint32 numClipRects);
  private:
    NPWindow* mWindow;
    nsIPluginInstance* mInstance;
    const nsIntSize& mPluginSize;
    const nsIntRect& mDirtyRect;
  };
#endif

#if defined(MOZ_PLATFORM_HILDON) && defined(MOZ_WIDGET_GTK2)

  
  

  
  nsIntSize mPluginSize;

  
  
  nsCOMPtr<nsIDOMElement> mBlitParentElement;

  
  gfxRect mAbsolutePosition;

  
  gfxRect mAbsolutePositionClip;

  GC mXlibSurfGC;
  Window mBlitWindow;
  XImage *mSharedXImage;
  XShmSegmentInfo mSharedSegmentInfo;

  PRBool SetupXShm();
  void ReleaseXShm();
  void NativeImageDraw(NPRect* invalidRect = nsnull);
  PRBool UpdateVisibility();

#endif
};

  
#ifdef XP_MACOSX

  enum { ePluginPaintEnable, ePluginPaintDisable };

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

NS_QUERYFRAME_HEAD(nsObjectFrame)
  NS_QUERYFRAME_ENTRY(nsIObjectFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsObjectFrameSuper)

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

NS_IMETHODIMP 
nsObjectFrame::Init(nsIContent*      aContent,
                    nsIFrame*        aParent,
                    nsIFrame*        aPrevInFlow)
{
  NS_PRECONDITION(aContent, "How did that happen?");
  mPreventInstantiation =
    (aContent->GetCurrentDoc()->GetDisplayDocument() != nsnull);

  PR_LOG(nsObjectFrameLM, PR_LOG_DEBUG,
         ("Initializing nsObjectFrame %p for content %p\n", this, aContent));

  nsresult rv = nsObjectFrameSuper::Init(aContent, aParent, aPrevInFlow);

  if (NS_SUCCEEDED(rv)) {
    NotifyPluginEventObservers(NS_LITERAL_STRING("init").get());
  }
#ifdef XP_WIN
  mDoublePassEvent = 0;
#endif
  return rv;
}

void
nsObjectFrame::Destroy()
{
  NS_ASSERTION(!mPreventInstantiation ||
               (mContent && mContent->GetCurrentDoc()->GetDisplayDocument()),
               "about to crash due to bug 136927");

  NotifyPluginEventObservers(NS_LITERAL_STRING("destroy").get());

  PresContext()->RootPresContext()->UnregisterPluginForGeometryUpdates(this);

  
  
  StopPluginInternal(PR_TRUE);

  
  
  if (mWidget) {
    mInnerView->DetachWidgetEventHandler(mWidget);
    mWidget->Destroy();
  }

  nsObjectFrameSuper::Destroy();
}

 void
nsObjectFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
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

  nsObjectFrameSuper::DidSetStyleContext(aOldStyleContext);
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

  PRBool usewidgets;
  nsCOMPtr<nsIDeviceContext> dx;
  viewMan->GetDeviceContext(*getter_AddRefs(dx));
  dx->SupportsNativeWidgets(usewidgets);

  
  
  
  
  nsIView* parentWithView;
  nsPoint origin;
  nsRect r(0, 0, mRect.width, mRect.height);

  GetOffsetFromView(origin, &parentWithView);
  viewMan->ResizeView(view, r);
  viewMan->MoveViewTo(view, origin.x, origin.y);

  if (!aViewOnly && !mWidget && usewidgets) {
    mInnerView = viewMan->CreateView(GetContentRect() - GetPosition(), view);
    if (!mInnerView) {
      NS_ERROR("Could not create inner view");
      return NS_ERROR_OUT_OF_MEMORY;
    }
    viewMan->InsertChild(view, mInnerView, nsnull, PR_TRUE);

    nsresult rv;
    mWidget = do_CreateInstance(kWidgetCID, &rv);
    if (NS_FAILED(rv))
      return rv;

    nsRootPresContext* rpc = PresContext()->RootPresContext();
    
    nsIWidget* parentWidget =
      rpc->PresShell()->FrameManager()->GetRootFrame()->GetWindow();

    nsWidgetInitData initData;
    initData.mWindowType = eWindowType_plugin;
    initData.mUnicode = PR_FALSE;
    initData.clipChildren = PR_TRUE;
    initData.clipSiblings = PR_TRUE;
    
    
    
    
    EVENT_CALLBACK eventHandler = mInnerView->AttachWidgetEventHandler(mWidget);
    mWidget->Create(parentWidget, nsnull, nsIntRect(0,0,0,0),
                    eventHandler, dx, nsnull, nsnull, &initData);

    mWidget->EnableDragDrop(PR_TRUE);

    rpc->RegisterPluginForGeometryUpdates(this);
    rpc->UpdatePluginGeometry(this);

    
    
    
    
    
    if (parentWidget == GetWindow()) {
      mWidget->Show(PR_TRUE);
    }
  }

  if (mWidget) {
    
    
    
    
    for (nsIFrame* frame = this; frame; frame = frame->GetParent()) {
      const nsStyleBackground* background = frame->GetStyleBackground();
      if (!background->IsTransparent()) {  
        mWidget->SetBackgroundColor(background->mBackgroundColor);
        break;
      }
    }

#ifdef XP_MACOSX
    
    
    nsCOMPtr<nsIPluginWidget> pluginWidget = do_QueryInterface(mWidget);
    if (!pluginWidget)
      return NS_ERROR_FAILURE;
    pluginWidget->SetPluginEventModel(mInstanceOwner->GetEventModel());
#endif
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
      aMetrics.width = NS_MIN(NS_MAX(nsPresContext::CSSPixelsToAppUnits(EMBED_DEF_WIDTH),
                                     aReflowState.mComputedMinWidth),
                              aReflowState.mComputedMaxWidth);
    }
    if (aMetrics.height == NS_UNCONSTRAINEDSIZE) {
      aMetrics.height = NS_MIN(NS_MAX(nsPresContext::CSSPixelsToAppUnits(EMBED_DEF_HEIGHT),
                                      aReflowState.mComputedMinHeight),
                               aReflowState.mComputedMaxHeight);
    }

#if defined (MOZ_WIDGET_GTK2)
    
    
    
    
    aMetrics.height = NS_MIN(aPresContext->DevPixelsToAppUnits(PR_INT16_MAX), aMetrics.height);
    aMetrics.width = NS_MIN(aPresContext->DevPixelsToAppUnits(PR_INT16_MAX), aMetrics.width);
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
  aMetrics.mOverflowArea.SetRect(0, 0, aMetrics.width, aMetrics.height);
  FinishAndStoreOverflow(&aMetrics);

  
  
  
  if (!GetContent()->IsDoneAddingChildren()) {
    aStatus = NS_FRAME_COMPLETE;
    return NS_OK;
  }

  
  if (aPresContext->Medium() == nsGkAtoms::print) {
    aStatus = NS_FRAME_COMPLETE;
    return NS_OK;
  }

  nsRect r(0, 0, aMetrics.width, aMetrics.height);
  r.Deflate(aReflowState.mComputedBorderPadding);

  if (mInnerView) {
    nsIViewManager* vm = mInnerView->GetViewManager();
    vm->MoveViewTo(mInnerView, r.x, r.y);
    vm->ResizeView(mInnerView, nsRect(nsPoint(0, 0), r.Size()), PR_TRUE);
  }

  FixupWindow(r.Size());

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
  PRBool fullPageMode = PR_FALSE;
  if (pDoc) {
    pDoc->GetWillHandleInstantiation(&fullPageMode);
  }

  nsresult rv;
  if (fullPageMode) {  
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

  NPWindow *window;
  mInstanceOwner->GetWindow(window);

  NS_ENSURE_TRUE(window, );

#ifdef XP_MACOSX
  mInstanceOwner->FixUpPluginWindow(ePluginPaintDisable);
#endif

  PRBool windowless = (window->type == NPWindowTypeDrawable);

  nsIntPoint origin = GetWindowOriginInPixels(windowless);

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
  NotifyPluginEventObservers(NS_LITERAL_STRING("reflow").get());
}

void
nsObjectFrame::CallSetWindow()
{
  NPWindow *win = nsnull;
 
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

  PRBool windowless = (window->type == NPWindowTypeDrawable);

  nsIntPoint origin = GetWindowOriginInPixels(windowless);

  window->x = origin.x;
  window->y = origin.y;

  
  window->window = mInstanceOwner->GetPluginPort();

  
  
  window->CallSetWindow(pi);

  mInstanceOwner->ReleasePluginPort(window->window);
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
        (!hidden.LowerCaseEqualsLiteral("false") &&
         !hidden.LowerCaseEqualsLiteral("no") &&
         !hidden.LowerCaseEqualsLiteral("off")))) {
      return PR_TRUE;
    }
  }

  return PR_FALSE;
}

nsIntPoint nsObjectFrame::GetWindowOriginInPixels(PRBool aWindowless)
{
  nsIView * parentWithView;
  nsPoint origin(0,0);

  GetOffsetFromView(origin, &parentWithView);

  
  
  if (aWindowless && parentWithView) {
    nsPoint offsetToWidget;
    parentWithView->GetNearestWidget(&offsetToWidget);
    origin += offsetToWidget;
  }
  
  
  origin += GetUsedBorderAndPadding().TopLeft();

  return nsIntPoint(PresContext()->AppUnitsToDevPixels(origin.x),
                    PresContext()->AppUnitsToDevPixels(origin.y));
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

 void
nsObjectFrame::PaintPrintPlugin(nsIFrame* aFrame, nsIRenderingContext* aCtx,
                                const nsRect& aDirtyRect, nsPoint aPt)
{
  nsPoint pt = aPt + aFrame->GetUsedBorderAndPadding().TopLeft();
  nsIRenderingContext::AutoPushTranslation translate(aCtx, pt.x, pt.y);
  
  static_cast<nsObjectFrame*>(aFrame)->PrintPlugin(*aCtx, aDirtyRect);
}

nsRect
nsDisplayPlugin::GetBounds(nsDisplayListBuilder* aBuilder)
{
  return mFrame->GetContentRect() +
    aBuilder->ToReferenceFrame(mFrame->GetParent());
}

void
nsDisplayPlugin::Paint(nsDisplayListBuilder* aBuilder,
                       nsIRenderingContext* aCtx)
{
  nsObjectFrame* f = static_cast<nsObjectFrame*>(mFrame);
  f->PaintPlugin(*aCtx, mVisibleRect, GetBounds(aBuilder));
}

PRBool
nsDisplayPlugin::ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                   nsRegion* aVisibleRegion,
                                   nsRegion* aVisibleRegionBeforeMove)
{
  NS_ASSERTION((aVisibleRegionBeforeMove != nsnull) == aBuilder->HasMovingFrames(),
               "Should have aVisibleRegionBeforeMove when there are moving frames");

  mVisibleRegion.And(*aVisibleRegion, GetBounds(aBuilder));  
  return nsDisplayItem::ComputeVisibility(aBuilder, aVisibleRegion,
                                          aVisibleRegionBeforeMove);
}

PRBool
nsDisplayPlugin::IsOpaque(nsDisplayListBuilder* aBuilder)
{
  nsObjectFrame* f = static_cast<nsObjectFrame*>(mFrame);
  return f->IsOpaque();
}

void
nsDisplayPlugin::GetWidgetConfiguration(nsDisplayListBuilder* aBuilder,
                                        nsTArray<nsIWidget::Configuration>* aConfigurations)
{
  nsObjectFrame* f = static_cast<nsObjectFrame*>(mFrame);
  nsPoint pluginOrigin = mFrame->GetUsedBorderAndPadding().TopLeft() +
    aBuilder->ToReferenceFrame(mFrame);
  f->ComputeWidgetGeometry(mVisibleRegion, pluginOrigin, aConfigurations);
}

void
nsObjectFrame::ComputeWidgetGeometry(const nsRegion& aRegion,
                                     const nsPoint& aPluginOrigin,
                                     nsTArray<nsIWidget::Configuration>* aConfigurations)
{
  if (!mWidget)
    return;

  nsIWidget::Configuration* configuration =
    aConfigurations->AppendElement();
  if (!configuration)
    return;
  configuration->mChild = mWidget;

  nsPresContext* presContext = PresContext();
  PRInt32 appUnitsPerDevPixel = presContext->AppUnitsPerDevPixel();
  nsIFrame* rootFrame =
    presContext->RootPresContext()->PresShell()->FrameManager()->GetRootFrame();
  nsRect bounds = GetContentRect() + GetParent()->GetOffsetTo(rootFrame);
  configuration->mBounds = bounds.ToNearestPixels(appUnitsPerDevPixel);

  nsRegionRectIterator iter(aRegion);
  nsIntPoint pluginOrigin = aPluginOrigin.ToNearestPixels(appUnitsPerDevPixel);
  for (const nsRect* r = iter.Next(); r; r = iter.Next()) {
    
    
    nsIntRect pixRect =
      r->ToNearestPixels(appUnitsPerDevPixel) - pluginOrigin;
    if (!pixRect.IsEmpty()) {
      configuration->mClipRegion.AppendElement(pixRect);
    }
  }
}

nsresult
nsObjectFrame::SetAbsoluteScreenPosition(nsIDOMElement* element,
                                         nsIDOMClientRect* position,
                                         nsIDOMClientRect* clip)
{
#if defined(MOZ_PLATFORM_HILDON) && defined(MOZ_WIDGET_GTK2)
  if (!mInstanceOwner)
    return NS_ERROR_NOT_AVAILABLE;
  return mInstanceOwner->SetAbsoluteScreenPosition(element, position, clip);
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif
}

void
nsObjectFrame::NotifyPluginEventObservers(const PRUnichar *eventType)
{
  nsCOMPtr<nsIDOMElement> e = do_QueryInterface(mContent);
  if (!e)
    return;
  nsCOMPtr<nsIObserverService> obsSvc = do_GetService("@mozilla.org/observer-service;1");
  obsSvc->NotifyObservers(e, "plugin-changed-event", eventType);
}

void
nsObjectFrame::DidSetWidgetGeometry()
{
#if defined(XP_MACOSX)
  if (mInstanceOwner) {
    mInstanceOwner->FixUpPluginWindow(ePluginPaintEnable);
  }
#endif
}

PRBool
nsObjectFrame::IsOpaque() const
{
#if defined(XP_MACOSX)
  return PR_FALSE;
#else
  if (mInstanceOwner) {
    NPWindow *window;
    mInstanceOwner->GetWindow(window);
    if (window->type == NPWindowTypeDrawable) {
      
      
      if (mInstanceOwner) {
        nsresult rv;
        PRBool transparent = PR_FALSE;
        nsCOMPtr<nsIPluginInstance> pi;
        if (NS_SUCCEEDED(rv = mInstanceOwner->GetInstance(*getter_AddRefs(pi)))) {
          pi->IsTransparent(&transparent);
          return !transparent;
        }
      }
      return PR_FALSE;
    }
  }
  return PR_TRUE;
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

#ifndef XP_MACOSX
  if (mWidget && aBuilder->IsInTransform()) {
    
    return NS_OK;
  }
#endif

  
  if (type == nsPresContext::eContext_Print)
    return aLists.Content()->AppendNewToTop(new (aBuilder)
        nsDisplayGeneric(this, PaintPrintPlugin, "PrintPlugin"));

  return aLists.Content()->AppendNewToTop(new (aBuilder)
      nsDisplayPlugin(this));
}

void
nsObjectFrame::PrintPlugin(nsIRenderingContext& aRenderingContext,
                           const nsRect& aDirtyRect)
{
  nsCOMPtr<nsIObjectLoadingContent> obj(do_QueryInterface(mContent));
  if (!obj)
    return;

  nsIFrame* frame = nsnull;
  obj->GetPrintFrame(&frame);
  if (!frame)
    return;

  nsPresContext* presContext = PresContext();
  
  
  nsIObjectFrame* objectFrame = do_QueryFrame(frame);
  if (!objectFrame)
    objectFrame = GetNextObjectFrame(presContext,frame);
  if (!objectFrame)
    return;

  
  nsCOMPtr<nsIPluginInstance> pi;
  if (NS_FAILED(objectFrame->GetPluginInstance(*getter_AddRefs(pi))) || !pi)
    return;

  
  NPWindow window;
  window.window = nsnull;

  
  NPPrint npprint;
  npprint.mode = NP_EMBED;

  
  PRBool windowless = PR_FALSE;
  pi->IsWindowless(&windowless);
  window.type = windowless ? NPWindowTypeDrawable : NPWindowTypeWindow;

  window.clipRect.bottom = 0; window.clipRect.top = 0;
  window.clipRect.left = 0; window.clipRect.right = 0;
  

#if defined(XP_MACOSX) && !defined(NP_NO_CARBON)
  nsSize contentSize = GetContentRect().Size();
  window.x = 0;
  window.y = 0;
  window.width = presContext->AppUnitsToDevPixels(contentSize.width);
  window.height = presContext->AppUnitsToDevPixels(contentSize.height);

  gfxContext *ctx = aRenderingContext.ThebesContext();
  if (!ctx)
    return;
  gfxContextAutoSaveRestore save(ctx);

  ctx->NewPath();

  gfxRect rect(window.x, window.y, window.width, window.height);

  ctx->Rectangle(rect);
  ctx->Clip();

  gfxQuartzNativeDrawing nativeDraw(ctx, rect);
  CGContextRef cgContext = nativeDraw.BeginNativeDrawing();
  if (!cgContext) {
    nativeDraw.EndNativeDrawing();
    return;
  }

  window.clipRect.right = window.width;
  window.clipRect.bottom = window.height;
  window.type = NPWindowTypeDrawable;

  Rect gwBounds;
  ::SetRect(&gwBounds, 0, 0, window.width, window.height);

  nsTArray<char> buffer(window.width * window.height * 4);
  CGColorSpaceRef cspace = ::CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
  if (!cspace) {
    nativeDraw.EndNativeDrawing();
    return;
  }
  CGContextRef cgBuffer =
    ::CGBitmapContextCreate(buffer.Elements(), 
                            window.width, window.height, 8, window.width * 4,
                            cspace, kCGBitmapByteOrder32Big | kCGImageAlphaPremultipliedFirst);
  ::CGColorSpaceRelease(cspace);
  if (!cgBuffer) {
    nativeDraw.EndNativeDrawing();
    return;
  }
  GWorldPtr gWorld;
  if (::NewGWorldFromPtr(&gWorld, k32ARGBPixelFormat, &gwBounds, NULL, NULL, 0,
                         buffer.Elements(), window.width * 4) != noErr) {
    ::CGContextRelease(cgBuffer);
    nativeDraw.EndNativeDrawing();
    return;
  }

  window.clipRect.right = window.width;
  window.clipRect.bottom = window.height;
  window.type = NPWindowTypeDrawable;
  
  
  
  
  
  
  
  
  window.window = &gWorld;
  npprint.print.embedPrint.platformPrint = gWorld;
  npprint.print.embedPrint.window = window;
  nsresult rv = pi->Print(&npprint);

  ::CGContextSaveGState(cgContext);
  ::CGContextTranslateCTM(cgContext, 0.0f, float(window.height));
  ::CGContextScaleCTM(cgContext, 1.0f, -1.0f);
  CGImageRef image = ::CGBitmapContextCreateImage(cgBuffer);
  if (!image) {
    ::CGContextRestoreGState(cgContext);
    ::CGContextRelease(cgBuffer);
    ::DisposeGWorld(gWorld);
    nativeDraw.EndNativeDrawing();
    return;
  }
  ::CGContextDrawImage(cgContext,
                       ::CGRectMake(0, 0, window.width, window.height),
                       image);
  ::CGImageRelease(image);
  ::CGContextRestoreGState(cgContext);
  ::CGContextRelease(cgBuffer);

  ::DisposeGWorld(gWorld);

  nativeDraw.EndNativeDrawing();
#elif defined(XP_UNIX)

  



#elif defined(XP_OS2)
  void *hps = aRenderingContext.GetNativeGraphicData(nsIRenderingContext::NATIVE_OS2_PS);
  if (!hps)
    return;

  npprint.print.embedPrint.platformPrint = hps;
  npprint.print.embedPrint.window = window;
  
  pi->Print(&npprint);
#elif defined(XP_WIN)

  














  
  nsSize contentSize = GetContentRect().Size();
  window.x = 0;
  window.y = 0;
  window.width = presContext->AppUnitsToDevPixels(contentSize.width);
  window.height = presContext->AppUnitsToDevPixels(contentSize.height);

  gfxContext *ctx = aRenderingContext.ThebesContext();

  ctx->Save();

  
  ctx->NewPath();
  gfxRect r(window.x, window.y, window.width, window.height);
  ctx->Rectangle(r);
  ctx->Clip();

  gfxWindowsNativeDrawing nativeDraw(ctx, r);
  do {
    HDC dc = nativeDraw.BeginNativeDrawing();
    if (!dc)
      return;

    
    npprint.print.embedPrint.platformPrint = dc;
    npprint.print.embedPrint.window = window;
    
    pi->Print(&npprint);

    nativeDraw.EndNativeDrawing();
  } while (nativeDraw.ShouldRenderAgain());
  nativeDraw.PaintToContext();

  ctx->Restore();
#endif

  
  
  nsDidReflowStatus status = NS_FRAME_REFLOW_FINISHED; 
  frame->DidReflow(presContext,
                   nsnull, status);  
}

void
nsObjectFrame::PaintPlugin(nsIRenderingContext& aRenderingContext,
                           const nsRect& aDirtyRect, const nsRect& aPluginRect)
{
  
#if defined(XP_MACOSX)
  
  if (mInstanceOwner) {
    if (mInstanceOwner->GetDrawingModel() == NPDrawingModelCoreGraphics) {
      PRInt32 appUnitsPerDevPixel = PresContext()->AppUnitsPerDevPixel();
      
      
      nsIntRect contentPixels = aPluginRect.ToNearestPixels(appUnitsPerDevPixel);
      nsIntRect dirtyPixels = aDirtyRect.ToOutsidePixels(appUnitsPerDevPixel);
      nsIntRect clipPixels;
      clipPixels.IntersectRect(contentPixels, dirtyPixels);
      gfxRect nativeClipRect(clipPixels.x, clipPixels.y,
                             clipPixels.width, clipPixels.height);
      gfxContext* ctx = aRenderingContext.ThebesContext();

      gfxContextAutoSaveRestore save(ctx);
      ctx->NewPath();
      ctx->Rectangle(nativeClipRect);
      ctx->Clip();
      gfxPoint offset(contentPixels.x, contentPixels.y);
      ctx->Translate(offset);

      gfxQuartzNativeDrawing nativeDrawing(ctx, nativeClipRect - offset);

      CGContextRef cgContext = nativeDrawing.BeginNativeDrawing();
      if (!cgContext) {
        NS_WARNING("null CGContextRef during PaintPlugin");
        return;
      }

      nsCOMPtr<nsIPluginInstance> inst;
      GetPluginInstance(*getter_AddRefs(inst));
      if (!inst) {
        NS_WARNING("null plugin instance during PaintPlugin");
        nativeDrawing.EndNativeDrawing();
        return;
      }
      NPWindow* window;
      mInstanceOwner->GetWindow(window);
      if (!window) {
        NS_WARNING("null plugin window during PaintPlugin");
        nativeDrawing.EndNativeDrawing();
        return;
      }
      NP_CGContext* cgPluginPortCopy =
                static_cast<NP_CGContext*>(mInstanceOwner->GetPluginPortCopy());
      if (!cgPluginPortCopy) {
        NS_WARNING("null plugin port copy during PaintPlugin");
        nativeDrawing.EndNativeDrawing();
        return;
      }
      if (!mInstanceOwner->SetPluginPortAndDetectChange()) {
        NS_WARNING("null plugin port during PaintPlugin");
        nativeDrawing.EndNativeDrawing();
        return;
      }
      
#ifndef NP_NO_CARBON
      
      
      
      
      
      
      
      
      NP_CGContext* windowContext = static_cast<NP_CGContext*>(window->window);
      if (mInstanceOwner->GetEventModel() == NPEventModelCarbon &&
          windowContext->context != cgContext) {
        windowContext->context = cgContext;
        cgPluginPortCopy->context = cgContext;
        mInstanceOwner->SetPluginPortChanged(PR_TRUE);
      }
#endif

      mInstanceOwner->BeginCGPaint();
      mInstanceOwner->Paint(nativeClipRect - offset, cgContext);
      mInstanceOwner->EndCGPaint();

      nativeDrawing.EndNativeDrawing();
    } else {
      
      nsIRenderingContext::AutoPushTranslation
        translate(&aRenderingContext, aPluginRect.x, aPluginRect.y);

      
      gfxRect tmpRect(0, 0, 0, 0);
      mInstanceOwner->Paint(tmpRect, NULL);
    }
  }
#elif defined(MOZ_X11) || defined(MOZ_DFB)
  if (mInstanceOwner) {
    NPWindow *window;
    mInstanceOwner->GetWindow(window);
#ifdef MOZ_COMPOSITED_PLUGINS
    {
#else
    if (window->type == NPWindowTypeDrawable) {
#endif
      gfxRect frameGfxRect =
        PresContext()->AppUnitsToGfxUnits(aPluginRect);
      gfxRect dirtyGfxRect =
        PresContext()->AppUnitsToGfxUnits(aDirtyRect);
      gfxContext* ctx = aRenderingContext.ThebesContext();

      mInstanceOwner->Paint(ctx, frameGfxRect, dirtyGfxRect);
    }
  }
#elif defined(XP_WIN)
  nsCOMPtr<nsIPluginInstance> inst;
  GetPluginInstance(*getter_AddRefs(inst));
  if (inst) {
    gfxRect frameGfxRect =
      PresContext()->AppUnitsToGfxUnits(aPluginRect);
    gfxRect dirtyGfxRect =
      PresContext()->AppUnitsToGfxUnits(aDirtyRect);
    gfxContext *ctx = aRenderingContext.ThebesContext();
    gfxMatrix currentMatrix = ctx->CurrentMatrix();

    if (ctx->UserToDevicePixelSnapped(frameGfxRect, PR_FALSE)) {
      dirtyGfxRect = ctx->UserToDevice(dirtyGfxRect);
      ctx->IdentityMatrix();
    }
    dirtyGfxRect.RoundOut();

    
    NPWindow *window;
    mInstanceOwner->GetWindow(window);

    if (window->type == NPWindowTypeDrawable) {
      
      PRBool doupdatewindow = PR_FALSE;
      
      nsPoint origin;
      
      gfxWindowsNativeDrawing nativeDraw(ctx, frameGfxRect);
      PRBool doublePass = PR_FALSE;
      do {
        HDC hdc = nativeDraw.BeginNativeDrawing();
        if (!hdc)
          return;

        RECT dest;
        nativeDraw.TransformToNativeRect(frameGfxRect, dest);
        RECT dirty;
        nativeDraw.TransformToNativeRect(dirtyGfxRect, dirty);

        
        
        if (reinterpret_cast<HDC>(window->window) != hdc ||
            window->x != dest.left || window->y != dest.top) {
          window->window = hdc;
          window->x = dest.left;
          window->y = dest.top;

          
          
          
          
          
          
          
          
          
          
          

          nsIntPoint origin = GetWindowOriginInPixels(PR_TRUE);
          nsIntRect winlessRect = nsIntRect(origin, nsIntSize(window->width, window->height));
          
          
          
          
          
          if (mWindowlessRect != winlessRect) {
            mWindowlessRect = winlessRect;

            WINDOWPOS winpos;
            memset(&winpos, 0, sizeof(winpos));
            winpos.x = mWindowlessRect.x;
            winpos.y = mWindowlessRect.y;
            winpos.cx = mWindowlessRect.width;
            winpos.cy = mWindowlessRect.height;

            
            NPEvent pluginEvent;
            pluginEvent.event = WM_WINDOWPOSCHANGED;
            pluginEvent.wParam = 0;
            pluginEvent.lParam = (uint32)&winpos;
            PRBool eventHandled = PR_FALSE;

            inst->HandleEvent(&pluginEvent, &eventHandled);
          }

          inst->SetWindow(window);        
        }

        mInstanceOwner->Paint(dirty, hdc);
        nativeDraw.EndNativeDrawing();
        doublePass = nativeDraw.ShouldRenderAgain();
#ifdef MOZ_IPC
        if (doublePass) {
          
          
          
          if (!mDoublePassEvent)
            mDoublePassEvent = ::RegisterWindowMessage(NS_OOPP_DOUBLEPASS_MSGID);
          if (mDoublePassEvent) {
            NPEvent pluginEvent;
            pluginEvent.event = mDoublePassEvent;
            pluginEvent.wParam = 0;
            pluginEvent.lParam = 0;
            PRBool eventHandled = PR_FALSE;

            inst->HandleEvent(&pluginEvent, &eventHandled);
          }          
        }
#endif
      } while (doublePass);

      nativeDraw.PaintToContext();
    } else if (!(ctx->GetFlags() & gfxContext::FLAG_DESTINED_FOR_SCREEN)) {
      
      
      typedef BOOL (WINAPI * PrintWindowPtr)
          (HWND hwnd, HDC hdcBlt, UINT nFlags);
      PrintWindowPtr printProc = nsnull;
      HMODULE module = ::GetModuleHandleW(L"user32.dll");
      if (module) {
        printProc = reinterpret_cast<PrintWindowPtr>
          (::GetProcAddress(module, "PrintWindow"));
      }
      
      if (printProc && !mInstanceOwner->MatchPluginName("Java(TM) Platform")) {
        HWND hwnd = reinterpret_cast<HWND>(window->window);
        RECT rc;
        GetWindowRect(hwnd, &rc);
        nsRefPtr<gfxWindowsSurface> surface =
          new gfxWindowsSurface(gfxIntSize(rc.right - rc.left, rc.bottom - rc.top));

        if (surface && printProc) {
          printProc(hwnd, surface->GetDC(), 0);
        
          ctx->Translate(frameGfxRect.pos);
          ctx->SetSource(surface);
          gfxRect r = frameGfxRect.Intersect(dirtyGfxRect) - frameGfxRect.pos;
          ctx->NewPath();
          ctx->Rectangle(r);
          ctx->Fill();
        }
      }
    }

    ctx->SetMatrix(currentMatrix);
  }
#elif defined(XP_OS2)
  nsCOMPtr<nsIPluginInstance> inst;
  GetPluginInstance(*getter_AddRefs(inst));
  if (inst) {
    
    NPWindow *window;
    mInstanceOwner->GetWindow(window);

    if (window->type == NPWindowTypeDrawable) {
      
      nsIRenderingContext::AutoPushTranslation
        translate(&aRenderingContext, aPluginRect.x, aPluginRect.y);

      
      PRBool doupdatewindow = PR_FALSE;
      
      nsIntPoint origin;

      







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

      
      HPS hps = (HPS)aRenderingContext.GetNativeGraphicData(nsIRenderingContext::NATIVE_OS2_PS);
      if (reinterpret_cast<HPS>(window->window) != hps) {
        window->window = reinterpret_cast<void*>(hps);
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

      if ((window->x != origin.x) || (window->y != origin.y)) {
        window->x = origin.x;
        window->y = origin.y;
        doupdatewindow = PR_TRUE;
      }

      
      if (doupdatewindow) {
        inst->SetWindow(window);        
      }

      mInstanceOwner->Paint(aDirtyRect, hps);
      if (lPSid >= 1) {
        GpiRestorePS(hps, lPSid);
      }
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
  NS_ENSURE_ARG_POINTER(anEvent);
  NS_ENSURE_ARG_POINTER(anEventStatus);
  nsresult rv = NS_OK;

  if (!mInstanceOwner)
    return NS_ERROR_NULL_POINTER;

  mInstanceOwner->ConsiderNewEventloopNestingLevel();

  if (anEvent->message == NS_PLUGIN_ACTIVATE) {
    nsIFocusManager* fm = nsFocusManager::GetFocusManager();
    nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(GetContent());
    if (fm && elem)
      return fm->SetFocus(elem, 0);
  }

  if (mInstanceOwner->SendNativeEvents() && NS_IS_PLUGIN_EVENT(anEvent)) {
    *anEventStatus = mInstanceOwner->ProcessEvent(*anEvent);
    return rv;
  }

#ifdef XP_WIN
  rv = nsObjectFrameSuper::HandleEvent(aPresContext, anEvent, anEventStatus);
  return rv;
#endif

#ifdef XP_MACOSX
  
  if ((anEvent->message == NS_MOUSE_ENTER || anEvent->message == NS_MOUSE_SCROLL) &&
      mInstanceOwner->GetEventModel() == NPEventModelCocoa) {
    *anEventStatus = mInstanceOwner->ProcessEvent(*anEvent);
    return rv;
  }
#endif

  if (anEvent->message == NS_DESTROY) {
    mInstanceOwner->CancelTimer();
    return rv;
  }

  return nsObjectFrameSuper::HandleEvent(aPresContext, anEvent, anEventStatus);
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

  nsCOMPtr<nsIPluginHost> pluginHost(do_GetService(MOZ_PLUGIN_HOST_CONTRACTID, &rv));
  if (NS_FAILED(rv))
    return rv;
  mInstanceOwner->SetPluginHost(pluginHost);

  
  FixupWindow(GetContentRect().Size());

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

  
  FixupWindow(GetContentRect().Size());

  
  nsCOMPtr<nsIPluginHost> pluginHost(do_GetService(MOZ_PLUGIN_HOST_CONTRACTID, &rv));
  if (NS_FAILED(rv))
    return rv;
  mInstanceOwner->SetPluginHost(pluginHost);

  NS_ASSERTION(!mPreventInstantiation, "Say what?");
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

#ifdef XP_WIN
static const char*
GetMIMEType(nsIPluginInstance *aPluginInstance)
{
  if (aPluginInstance) {
    const char* mime = nsnull;
    if (NS_SUCCEEDED(aPluginInstance->GetMIMEType(&mime)) && mime)
      return mime;
  }
  return "";
}
#endif

static PRBool
DoDelayedStop(nsPluginInstanceOwner *aInstanceOwner, PRBool aDelayedStop)
{
#if defined(MOZ_PLATFORM_HILDON) && defined(MOZ_WIDGET_GTK2)
  
  if (aDelayedStop && aInstanceOwner->MatchPluginName("Shockwave Flash"))
    return PR_FALSE;
#endif

  
  
  if (aDelayedStop
#if !(defined XP_WIN || defined MOZ_X11)
      && !aInstanceOwner->MatchPluginName("QuickTime")
      && !aInstanceOwner->MatchPluginName("Flip4Mac")
      && !aInstanceOwner->MatchPluginName("XStandard plugin")
      && !aInstanceOwner->MatchPluginName("CMISS Zinc Plugin")
#endif
      ) {
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
    NPWindow *win;
    aInstanceOwner->GetWindow(win);
    nsPluginNativeWindow *window = (nsPluginNativeWindow *)win;
    nsCOMPtr<nsIPluginInstance> nullinst;

    if (window) 
      window->CallSetWindow(nullinst);
    else 
      inst->SetWindow(nsnull);
    
    if (DoDelayedStop(aInstanceOwner, aDelayedStop))
      return;
    
    inst->Stop();

    nsCOMPtr<nsIPluginHost> pluginHost = do_GetService(MOZ_PLUGIN_HOST_CONTRACTID);
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

#if defined(XP_WIN) || defined(MOZ_X11)
  if (aDelayedStop && mWidget) {
    
    
    
    mInnerView->DetachWidgetEventHandler(mWidget);
    mWidget = nsnull;
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
    nsIObjectFrame* outFrame = do_QueryFrame(child);
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

NS_IMPL_FRAMEARENA_HELPERS(nsObjectFrame)




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



nsPluginInstanceOwner::nsPluginInstanceOwner()
{
  
  
  nsCOMPtr<nsIPluginHost> ph = do_GetService(MOZ_PLUGIN_HOST_CONTRACTID);
  if (ph)
    ph->NewPluginNativeWindow(&mPluginWindow);
  else
    mPluginWindow = nsnull;

  mObjectFrame = nsnull;
  mTagText = nsnull;
#ifdef XP_MACOSX
  memset(&mCGPluginPortCopy, 0, sizeof(NP_CGContext));
  memset(&mQDPluginPortCopy, 0, sizeof(NP_Port));
  mInCGPaintLevel = 0;
#endif
  mContentFocused = PR_FALSE;
  mWidgetVisible = PR_TRUE;
  mPluginPortChanged = PR_FALSE;
  mNumCachedAttrs = 0;
  mNumCachedParams = 0;
  mCachedAttrParamNames = nsnull;
  mCachedAttrParamValues = nsnull;
  mDestroyWidget = PR_FALSE;
  mTimerCanceled = PR_TRUE;

#ifdef MOZ_COMPOSITED_PLUGINS
  mLastPoint = nsIntPoint(0,0);
#endif

#if defined(MOZ_PLATFORM_HILDON) && defined(MOZ_WIDGET_GTK2)
  mPluginSize = nsIntSize(0,0);
  mXlibSurfGC = None;
  mSharedXImage = nsnull;
  mSharedSegmentInfo.shmaddr = nsnull;
#endif

#ifdef XP_MACOSX
#ifndef NP_NO_QUICKDRAW
  mEventModel = NPEventModelCarbon;
#else
  mEventModel = NPEventModelCocoa;
#endif
#endif
  PR_LOG(nsObjectFrameLM, PR_LOG_DEBUG,
         ("nsPluginInstanceOwner %p created\n", this));
}

nsPluginInstanceOwner::~nsPluginInstanceOwner()
{
  PRInt32 cnt;

  PR_LOG(nsObjectFrameLM, PR_LOG_DEBUG,
         ("nsPluginInstanceOwner %p deleted\n", this));

  
  CancelTimer();

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

  
  nsCOMPtr<nsIPluginHost> ph = do_GetService(MOZ_PLUGIN_HOST_CONTRACTID);
  if (ph) {
    ph->DeletePluginNativeWindow(mPluginWindow);
    mPluginWindow = nsnull;
  }

  if (mInstance) {
    mInstance->InvalidateOwner();
  }

#if defined(MOZ_PLATFORM_HILDON) && defined(MOZ_WIDGET_GTK2)
  ReleaseXShm();
#endif
}





NS_IMPL_ADDREF(nsPluginInstanceOwner)
NS_IMPL_RELEASE(nsPluginInstanceOwner)

NS_INTERFACE_MAP_BEGIN(nsPluginInstanceOwner)
  NS_INTERFACE_MAP_ENTRY(nsIPluginInstanceOwner)
  NS_INTERFACE_MAP_ENTRY(nsIPluginTagInfo)
  NS_INTERFACE_MAP_ENTRY(nsITimerCallback)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMouseListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMouseMotionListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMKeyListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMFocusListener)
  NS_INTERFACE_MAP_ENTRY(nsIScrollPositionListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIDOMEventListener, nsIDOMMouseListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIPluginInstanceOwner)
NS_INTERFACE_MAP_END

NS_IMETHODIMP
nsPluginInstanceOwner::SetInstance(nsIPluginInstance *aInstance)
{
  NS_ASSERTION(!mInstance || !aInstance, "mInstance should only be set once!");

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

NS_IMETHODIMP nsPluginInstanceOwner::GetInstance(nsIPluginInstance *&aInstance)
{
  NS_IF_ADDREF(aInstance = mInstance);

  return NS_OK;
}

NS_IMETHODIMP nsPluginInstanceOwner::GetURL(const char *aURL, const char *aTarget, void *aPostData, PRUint32 aPostDataLen, void *aHeadersData, 
                                            PRUint32 aHeadersDataLen, PRBool isFile)
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
  if (!mObjectFrame || !invalidRect || !mWidgetVisible)
    return NS_ERROR_FAILURE;

#if defined(MOZ_PLATFORM_HILDON) && defined(MOZ_WIDGET_GTK2)
  PRBool simpleImageRender = PR_FALSE;
  mInstance->GetValueFromPlugin(NPPVpluginWindowlessLocalBool,
                                &simpleImageRender);
  if (simpleImageRender) {  
    NativeImageDraw(invalidRect);
    return NS_OK;
  }
#endif

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
  mObjectFrame->Invalidate(rect + mObjectFrame->GetUsedBorderAndPadding().TopLeft());
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
    
    
    
    
    
    
    
    
    
    nsIWidget* win = mObjectFrame->GetWindow();
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
  
  nsIWidget* win = mObjectFrame->GetWindow();
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

  
  
  nsCOMPtr<nsIPluginInstanceOwner> kungFuDeathGrip(this);
 
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

  
  NS_ENSURE_TRUE(mObjectFrame, NS_ERROR_OUT_OF_MEMORY);

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

  
  
  nsAdoptingCString wmodeType = nsContentUtils::GetCharPref("plugins.force.wmode");
  if (!wmodeType.IsEmpty())
    mNumCachedAttrs++;
  
  mCachedAttrParamNames  = (char **)PR_Calloc(sizeof(char *) * (mNumCachedAttrs + 1 + mNumCachedParams), 1);
  NS_ENSURE_TRUE(mCachedAttrParamNames,  NS_ERROR_OUT_OF_MEMORY);
  mCachedAttrParamValues = (char **)PR_Calloc(sizeof(char *) * (mNumCachedAttrs + 1 + mNumCachedParams), 1);
  NS_ENSURE_TRUE(mCachedAttrParamValues, NS_ERROR_OUT_OF_MEMORY);

  
  PRInt16 c = 0;

  
  
  
  
  
  
  PRInt16 start, end, increment;
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
  if (!wmodeType.IsEmpty()) {
    mCachedAttrParamNames [c] = ToNewUTF8String(NS_LITERAL_STRING("wmode"));
    mCachedAttrParamValues[c] = ToNewUTF8String(NS_ConvertUTF8toUTF16(wmodeType));
    c++;
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

#ifndef NP_NO_CARBON
static void InitializeEventRecord(EventRecord* event, Point* aMousePosition)
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

NPEventModel nsPluginInstanceOwner::GetEventModel()
{
  return mEventModel;
}

void* nsPluginInstanceOwner::GetPluginPortCopy()
{
#ifndef NP_NO_QUICKDRAW
  if (GetDrawingModel() == NPDrawingModelQuickDraw)
    return &mQDPluginPortCopy;
#endif
  if (GetDrawingModel() == NPDrawingModelCoreGraphics)
    return &mCGPluginPortCopy;
  return nsnull;
}
  












void* nsPluginInstanceOwner::SetPluginPortAndDetectChange()
{
  if (!mPluginWindow)
    return nsnull;
  void* pluginPort = GetPluginPort();
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
  } else if (drawingModel == NPDrawingModelCoreGraphics)
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

nsresult nsPluginInstanceOwner::ScrollPositionWillChange(nsIScrollableView* aScrollable, nscoord aX, nscoord aY)
{
#if defined(XP_MACOSX) && !defined(NP_NO_CARBON)
  if (GetEventModel() != NPEventModelCarbon)
    return NS_OK;

  CancelTimer();

  if (mInstance) {
    nsCOMPtr<nsIPluginWidget> pluginWidget = do_QueryInterface(mWidget);
    if (pluginWidget && NS_SUCCEEDED(pluginWidget->StartDrawPlugin())) {
      EventRecord scrollEvent;
      InitializeEventRecord(&scrollEvent, nsnull);
      scrollEvent.what = NPEventType_ScrollingBeginsEvent;

      void* window = FixUpPluginWindow(ePluginPaintDisable);
      if (window) {
        PRBool eventHandled = PR_FALSE;
        mInstance->HandleEvent(&scrollEvent, &eventHandled);
      }
      pluginWidget->EndDrawPlugin();
    }
  }
#endif
  return NS_OK;
}

nsresult nsPluginInstanceOwner::ScrollPositionDidChange(nsIScrollableView* aScrollable, nscoord aX, nscoord aY)
{
#if defined(XP_MACOSX) && !defined(NP_NO_CARBON)
  if (GetEventModel() != NPEventModelCarbon)
    return NS_OK;

  if (mInstance) {
    nsCOMPtr<nsIPluginWidget> pluginWidget = do_QueryInterface(mWidget);
    if (pluginWidget && NS_SUCCEEDED(pluginWidget->StartDrawPlugin())) {
      EventRecord scrollEvent;
      InitializeEventRecord(&scrollEvent, nsnull);
      scrollEvent.what = NPEventType_ScrollingEndsEvent;

      void* window = FixUpPluginWindow(ePluginPaintEnable);
      if (window) {
        PRBool eventHandled = PR_FALSE;
        mInstance->HandleEvent(&scrollEvent, &eventHandled);
      }
      pluginWidget->EndDrawPlugin();
    }
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
#if defined(XP_MACOSX) && !defined(NP_NO_CARBON)
  
  if (GetEventModel() != NPEventModelCarbon)
    return aKeyEvent->PreventDefault();

  
  
  
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
#if !defined(XP_MACOSX) && !defined(MOZ_COMPOSITED_PLUGINS)
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
#if !defined(XP_MACOSX) && !defined(MOZ_COMPOSITED_PLUGINS)
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
#if !defined(XP_MACOSX) && !defined(MOZ_COMPOSITED_PLUGINS)
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

#ifdef MOZ_COMPOSITED_PLUGINS
static void find_dest_id(XID top, XID *root, XID *dest, int target_x, int target_y)
{
  XID target_id = top;
  XID parent;
  XID *children;
  unsigned int nchildren;
  while (1) {
loop:
    
    if (!XQueryTree(GDK_DISPLAY(), target_id, root, &parent, &children, &nchildren) ||
        !nchildren)
      break;
    for (unsigned int i=0; i<nchildren; i++) {
      Window root;
      int x, y;
      unsigned int width, height;
      unsigned int border_width, depth;
      XGetGeometry(GDK_DISPLAY(), children[i], &root, &x, &y,
          &width, &height, &border_width,
          &depth);
      
      
      
      
      if (target_x >= x && target_y >= y &&
          target_x <= x + int(width) &&
          target_y <= y + int(height)) {
        target_id = children[i];
        
        XFree(children);
        goto loop;
      }
    }
    XFree(children);
    
    break;
  }
  *dest = target_id;
}
#endif

#ifdef MOZ_COMPOSITED_PLUGINS
nsEventStatus nsPluginInstanceOwner::ProcessEventX11Composited(const nsGUIEvent& anEvent)
{
  
  nsEventStatus rv = nsEventStatus_eIgnore;
  if (!mInstance || !mObjectFrame)   
    return rv;

  
  nsIWidget* widget = anEvent.widget;
  XEvent pluginEvent;
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
          mObjectFrame->GetUsedBorderAndPadding().TopLeft();
        nsIntPoint pluginPoint(presContext->AppUnitsToDevPixels(appPoint.x),
                               presContext->AppUnitsToDevPixels(appPoint.y));
        mLastPoint = pluginPoint;
        const nsMouseEvent& mouseEvent =
          static_cast<const nsMouseEvent&>(anEvent);
        
        nsIntPoint rootPoint(-1,-1);
        if (widget)
          rootPoint = anEvent.refPoint + widget->WidgetToScreenOffset();
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
              XEvent be;
              be.xmotion = pluginEvent.xmotion;
              
              XID w = (XID)mPluginWindow->window;
              be.xmotion.window = w;
              XSendEvent (be.xmotion.display, w,
                  FALSE, ButtonMotionMask, &be);

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
              XEvent be;
              be.xbutton =  event;
              XID target;
              XID root;
              int wx, wy;
              unsigned int width, height, border_width, depth;

              
              XID w = (XID)mPluginWindow->window;
              XGetGeometry(GDK_DISPLAY(), w, &root, &wx, &wy, &width, &height, &border_width, &depth);
              find_dest_id(w, &root, &target, pluginPoint.x + wx, pluginPoint.y + wy);
              be.xbutton.window = target;
              XSendEvent (GDK_DISPLAY(), target,
                  FALSE, event.type == ButtonPress ? ButtonPressMask : ButtonReleaseMask, &be);

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
          XEvent be;
          be.xkey =  event;
          XID target;
          XID root;
          int wx, wy;
          unsigned int width, height, border_width, depth;

          
          XID w = (XID)mPluginWindow->window;
          XGetGeometry(GDK_DISPLAY(), w, &root, &wx, &wy, &width, &height, &border_width, &depth);
          find_dest_id(w, &root, &target, mLastPoint.x + wx, mLastPoint.y + wy);
          be.xkey.window = target;
          XSendEvent (GDK_DISPLAY(), target,
              FALSE, event.type == XKeyPress ? KeyPressMask : KeyReleaseMask, &be);


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
    PR_LOG(nsObjectFrameLM, PR_LOG_DEBUG,
           ("Unhandled event message %d with struct type %d\n",
            anEvent.message, anEvent.eventStructType));
    return rv;
  }

  
  XAnyEvent& event = pluginEvent.xany;
  event.display = widget ?
    static_cast<Display*>(widget->GetNativeData(NS_NATIVE_DISPLAY)) : nsnull;
  event.window = None; 
  
  event.serial = 0;
  event.send_event = False;

#if 0
  
  PRBool eventHandled = PR_FALSE;
  mInstance->HandleEvent(&pluginEvent, &eventHandled);
  if (eventHandled)
    rv = nsEventStatus_eConsumeNoDefault;
#endif

  return rv;
}
#endif

nsEventStatus nsPluginInstanceOwner::ProcessEvent(const nsGUIEvent& anEvent)
{
  

#ifdef MOZ_COMPOSITED_PLUGINS
  if (mPluginWindow && (mPluginWindow->type != NPWindowTypeDrawable))
    return ProcessEventX11Composited(anEvent);
#endif

  nsEventStatus rv = nsEventStatus_eIgnore;

  if (!mInstance || !mObjectFrame)   
    return nsEventStatus_eIgnore;

#ifdef XP_MACOSX
  if (mWidget) {
    
    if (anEvent.message == NS_MOUSE_ENTER_SYNTH)
      return nsEventStatus_eIgnore;

    nsCOMPtr<nsIPluginWidget> pluginWidget = do_QueryInterface(mWidget);
    if (pluginWidget && NS_SUCCEEDED(pluginWidget->StartDrawPlugin())) {
      NPEventModel eventModel = GetEventModel();

      
#ifndef NP_NO_CARBON
      EventRecord synthCarbonEvent;
#endif
      NPCocoaEvent synthCocoaEvent;

      void* event = anEvent.pluginEvent;

      if (!event) {
        nsPoint pt = nsLayoutUtils::GetEventCoordinatesRelativeTo(&anEvent, mObjectFrame)
          - mObjectFrame->GetUsedBorderAndPadding().TopLeft();
        nsPresContext* presContext = mObjectFrame->PresContext();
        nsIntPoint ptPx(presContext->AppUnitsToDevPixels(pt.x),
                        presContext->AppUnitsToDevPixels(pt.y));

#ifndef NP_NO_CARBON
        if (eventModel == NPEventModelCarbon) {
          Point carbonPt = { ptPx.y + mPluginWindow->y, ptPx.x + mPluginWindow->x };

          event = &synthCarbonEvent;
          InitializeEventRecord(&synthCarbonEvent, &carbonPt);
        } else
#endif
        {
          event = &synthCocoaEvent;
          InitializeNPCocoaEvent(&synthCocoaEvent);
        }

        switch (anEvent.message) {
        case NS_FOCUS_CONTENT:
        case NS_BLUR_CONTENT:
#ifndef NP_NO_CARBON
          if (eventModel == NPEventModelCarbon) {
            synthCarbonEvent.what = (anEvent.message == NS_FOCUS_CONTENT) ?
            NPEventType_GetFocusEvent : NPEventType_LoseFocusEvent;
          } else
#endif
          {
            synthCocoaEvent.type = NPCocoaEventFocusChanged;
            synthCocoaEvent.data.focus.hasFocus = (anEvent.message == NS_FOCUS_CONTENT);
          }
          break;
        case NS_MOUSE_MOVE:
#ifndef NP_NO_CARBON
          if (eventModel == NPEventModelCarbon) {
            synthCarbonEvent.what = osEvt;
          } else
#endif
          {
            synthCocoaEvent.type = NPCocoaEventMouseMoved;
            synthCocoaEvent.data.mouse.pluginX = static_cast<double>(ptPx.x);
            synthCocoaEvent.data.mouse.pluginY = static_cast<double>(ptPx.y);
          }
          break;
        case NS_MOUSE_BUTTON_DOWN:
#ifndef NP_NO_CARBON
          if (eventModel == NPEventModelCarbon) {
            synthCarbonEvent.what = mouseDown;
          } else
#endif
          {
            synthCocoaEvent.type = NPCocoaEventMouseDown;
            synthCocoaEvent.data.mouse.pluginX = static_cast<double>(ptPx.x);
            synthCocoaEvent.data.mouse.pluginY = static_cast<double>(ptPx.y);
          }
          break;
        case NS_MOUSE_BUTTON_UP:
#ifndef NP_NO_CARBON
          if (eventModel == NPEventModelCarbon) {
            synthCarbonEvent.what = mouseUp;
          } else
#endif
          {
            synthCocoaEvent.type = NPCocoaEventMouseUp;
            synthCocoaEvent.data.mouse.pluginX = static_cast<double>(ptPx.x);
            synthCocoaEvent.data.mouse.pluginY = static_cast<double>(ptPx.y);
          }
          break;
        default:
          pluginWidget->EndDrawPlugin();
          return nsEventStatus_eIgnore;
        }
      }

#ifndef NP_NO_CARBON
      
      
      
      if (eventModel == NPEventModelCarbon && anEvent.message == NS_FOCUS_CONTENT)
        ::DeactivateTSMDocument(::TSMGetActiveDocument());
#endif

      PRBool eventHandled = PR_FALSE;
      void* window = FixUpPluginWindow(ePluginPaintEnable);
      if (window || (eventModel == NPEventModelCocoa)) {
        mInstance->HandleEvent(event, &eventHandled);
      }

      if (eventHandled &&
          !(anEvent.eventStructType == NS_MOUSE_EVENT &&
            anEvent.message == NS_MOUSE_BUTTON_DOWN &&
            static_cast<const nsMouseEvent&>(anEvent).button == nsMouseEvent::eLeftButton &&
            !mContentFocused))
        rv = nsEventStatus_eConsumeNoDefault;

      pluginWidget->EndDrawPlugin();
    }
  }
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
        pluginEvent.event = downMsgs[mouseEvent->button];
        break;
      }
      case NS_MOUSE_BUTTON_UP: {
        static const int upMsgs[] =
          { WM_LBUTTONUP, WM_MBUTTONUP, WM_RBUTTONUP };
        pluginEvent.event = upMsgs[mouseEvent->button];
        break;
      }
      case NS_MOUSE_DOUBLECLICK: {
        static const int dblClickMsgs[] =
          { WM_LBUTTONDBLCLK, WM_MBUTTONDBLCLK, WM_RBUTTONDBLCLK };
        pluginEvent.event = dblClickMsgs[mouseEvent->button];
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
        mObjectFrame->GetUsedBorderAndPadding().TopLeft();
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

  if (pPluginEvent) {
    PRBool eventHandled = PR_FALSE;
    mInstance->HandleEvent(pPluginEvent, &eventHandled);
    if (eventHandled)
      rv = nsEventStatus_eConsumeNoDefault;
  }
#endif

#ifdef MOZ_X11
  
  nsIWidget* widget = anEvent.widget;
  XEvent pluginEvent;
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
          mObjectFrame->GetUsedBorderAndPadding().TopLeft();
        nsIntPoint pluginPoint(presContext->AppUnitsToDevPixels(appPoint.x),
                               presContext->AppUnitsToDevPixels(appPoint.y));
        const nsMouseEvent& mouseEvent =
          static_cast<const nsMouseEvent&>(anEvent);
        
        nsIntPoint rootPoint(-1,-1);
        if (widget)
          rootPoint = anEvent.refPoint + widget->WidgetToScreenOffset();
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
    PR_LOG(nsObjectFrameLM, PR_LOG_DEBUG,
           ("Unhandled event message %d with struct type %d\n",
            anEvent.message, anEvent.eventStructType));
    return rv;
  }

  
  XAnyEvent& event = pluginEvent.xany;
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
#if defined(XP_WIN) || defined(MOZ_X11)
  if (aDelayedStop && mWidget) {
    
    
    

    
    
    mWidget->Show(PR_FALSE);
    mWidget->Enable(PR_FALSE);

    
    
    
    mWidget->SetParent(nsnull);

    mDestroyWidget = PR_TRUE;
  }
#endif

  
  nsIFrame* parentWithView = mObjectFrame->GetAncestorWithView();
  nsIView* curView = parentWithView ? parentWithView->GetView() : nsnull;
  while (curView) {
    nsIScrollableView* scrollingView = curView->ToScrollableView();
    if (scrollingView)
      scrollingView->RemoveScrollPositionListener((nsIScrollPositionListener *)this);
    
    curView = curView->GetParent();
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

      PRBool eventHandled = PR_FALSE;
      mInstance->HandleEvent(&updateEvent, &eventHandled);
    } else if (GetEventModel() == NPEventModelCocoa)
#endif
    {
      
      NPCocoaEvent updateEvent;
      InitializeNPCocoaEvent(&updateEvent);
      updateEvent.type = NPCocoaEventDrawRect;
      updateEvent.data.draw.context = cgContext;
      updateEvent.data.draw.x = aDirtyRect.X();
      updateEvent.data.draw.y = aDirtyRect.Y();
      updateEvent.data.draw.width = aDirtyRect.Width();
      updateEvent.data.draw.height = aDirtyRect.Height();

      PRBool eventHandled = PR_FALSE;
      mInstance->HandleEvent(&updateEvent, &eventHandled);
    }
    pluginWidget->EndDrawPlugin();
  }
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
  PRBool eventHandled = PR_FALSE;
  mInstance->HandleEvent(&pluginEvent, &eventHandled);
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
  PRBool eventHandled = PR_FALSE;
  mInstance->HandleEvent(&pluginEvent, &eventHandled);
}
#endif

#if defined(MOZ_X11) || defined(MOZ_DFB)
void nsPluginInstanceOwner::Paint(gfxContext* aContext,
                                  const gfxRect& aFrameRect,
                                  const gfxRect& aDirtyRect)
{
  if (!mInstance || !mObjectFrame)
    return;

#if defined(MOZ_PLATFORM_HILDON) && defined(MOZ_WIDGET_GTK2)
  
  
  
  PRBool simpleImageRender = PR_FALSE;
  mInstance->GetValueFromPlugin(NPPVpluginWindowlessLocalBool,
                                &simpleImageRender);
  if (simpleImageRender) {
    gfxMatrix matrix = aContext->CurrentMatrix();
    if (!matrix.HasNonAxisAlignedTransform())
      NativeImageDraw();
    return;
  } 
#endif

  
  gfxRect pluginRect = aFrameRect;
  if (aContext->UserToDevicePixelSnapped(pluginRect)) {
    pluginRect = aContext->DeviceToUser(pluginRect);
  }

  
  
  gfxRect dirtyRect = aDirtyRect + -pluginRect.pos;
  dirtyRect.RoundOut();

  
  
  
  
  
  
  
  
  
  
  nsIntSize pluginSize(NS_lround(pluginRect.size.width),
                       NS_lround(pluginRect.size.height));

  
  nsIntRect pluginDirtyRect(PRInt32(dirtyRect.pos.x),
                            PRInt32(dirtyRect.pos.y),
                            PRInt32(dirtyRect.size.width),
                            PRInt32(dirtyRect.size.height));
  if (!pluginDirtyRect.
      IntersectRect(nsIntRect(0, 0, pluginSize.width, pluginSize.height),
                    pluginDirtyRect))
    return;

  NPWindow* window;
  GetWindow(window);

  PRUint32 rendererFlags =
    Renderer::DRAW_SUPPORTS_OFFSET |
    Renderer::DRAW_SUPPORTS_CLIP_RECT |
    Renderer::DRAW_SUPPORTS_NONDEFAULT_VISUAL |
    Renderer::DRAW_SUPPORTS_ALTERNATE_SCREEN;

  PRBool transparent;
  mInstance->IsTransparent(&transparent);
  if (!transparent)
    rendererFlags |= Renderer::DRAW_IS_OPAQUE;

  
  gfxContextAutoSaveRestore autoSR(aContext);
  aContext->Translate(pluginRect.pos);

  Renderer renderer(window, mInstance, pluginSize, pluginDirtyRect);
  renderer.Draw(aContext, window->width, window->height,
                rendererFlags, nsnull);
}

#ifdef MOZ_X11
static int
DepthOfVisual(const Screen* screen, const Visual* visual)
{
  for (int d = 0; d < screen->ndepths; d++) {
    Depth *d_info = &screen->depths[d];
    for (int v = 0; v < d_info->nvisuals; v++) {
      if (&d_info->visuals[v] == visual)
        return d_info->depth;
    }
  }

  NS_ERROR("Visual not on Screen.");
  return 0;
}
#endif

#if defined(MOZ_PLATFORM_HILDON) && defined(MOZ_WIDGET_GTK2)

static GdkWindow* GetClosestWindow(nsIDOMElement *element)
{
  nsCOMPtr<nsIDOMDocument> domDocument;
  element->GetOwnerDocument(getter_AddRefs(domDocument));

  nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDocument);  
  if (!doc)
    return nsnull;

  nsIPresShell *presShell = doc->GetPrimaryShell();
  if (!presShell)
    return nsnull;

  nsCOMPtr<nsIContent> content = do_QueryInterface(element);
  nsIFrame* frame = presShell->GetPrimaryFrameFor(content);
  if (!frame)
    return nsnull;

  nsIWidget* win = frame->GetWindow();
  if (!win)
    return nsnull;

  GdkWindow* w = static_cast<GdkWindow*>(win->GetNativeData(NS_NATIVE_WINDOW));
  return w;
}

void
nsPluginInstanceOwner::ReleaseXShm()
{
  if (mXlibSurfGC) {
    XFreeGC(gdk_x11_get_default_xdisplay(), mXlibSurfGC);
    mXlibSurfGC = None;
  }
 
 if (mSharedSegmentInfo.shmaddr) {
    XShmDetach(gdk_x11_get_default_xdisplay(), &mSharedSegmentInfo);
    shmdt(mSharedSegmentInfo.shmaddr);
    mSharedSegmentInfo.shmaddr = nsnull;
  }

  if (mSharedXImage) {
    XDestroyImage(mSharedXImage);
    mSharedXImage = nsnull;
  }
}

PRBool
nsPluginInstanceOwner::SetupXShm()
{
  mBlitWindow = GDK_WINDOW_XWINDOW(GetClosestWindow(mBlitParentElement));
  if (!mBlitWindow)
    return PR_FALSE;

  ReleaseXShm();

  mXlibSurfGC = XCreateGC(gdk_x11_get_default_xdisplay(),
                          mBlitWindow,
                          0,
                          0);
  if (!mXlibSurfGC)
    return PR_FALSE;

  
  
  XVisualInfo vinfo;
  int foundVisual = XMatchVisualInfo(gdk_x11_get_default_xdisplay(),
                                     gdk_x11_get_default_screen(),
                                     16,
                                     TrueColor,
                                     &vinfo);
  if (!foundVisual) 
    return PR_FALSE;

  memset(&mSharedSegmentInfo, 0, sizeof(XShmSegmentInfo));
  mSharedXImage = XShmCreateImage(gdk_x11_get_default_xdisplay(),
                                  vinfo.visual,
                                  16,
                                  ZPixmap,
                                  0,
                                  &mSharedSegmentInfo,
                                  mPluginSize.width,
                                  mPluginSize.height);
  if (!mSharedXImage)
    return PR_FALSE;

  NS_ASSERTION(mSharedXImage->height, "do not call shmget with zero");
  mSharedSegmentInfo.shmid = shmget(IPC_PRIVATE,
                                    mSharedXImage->bytes_per_line * mSharedXImage->height,
                                    IPC_CREAT | 0777);
  if (mSharedSegmentInfo.shmid == -1) {
    XDestroyImage(mSharedXImage);
    mSharedXImage = nsnull;
    return PR_FALSE;
  }

  mSharedXImage->data = static_cast<char*>(shmat(mSharedSegmentInfo.shmid, 0, 0));
  if (mSharedXImage->data == (char*) -1) {
    shmctl(mSharedSegmentInfo.shmid, IPC_RMID, 0);
    XDestroyImage(mSharedXImage);
    mSharedXImage = nsnull;
    return PR_FALSE;
  }
    
  mSharedSegmentInfo.shmaddr = mSharedXImage->data;
  mSharedSegmentInfo.readOnly = False;

  Status s = XShmAttach(gdk_x11_get_default_xdisplay(), &mSharedSegmentInfo);
  XSync(gdk_x11_get_default_xdisplay(), False);
  shmctl(mSharedSegmentInfo.shmid, IPC_RMID, 0);
  if (!s) {
    
    
    shmdt(mSharedSegmentInfo.shmaddr);
    mSharedSegmentInfo.shmaddr = nsnull;
    ReleaseXShm();
    return PR_FALSE;
  }

  return PR_TRUE;
}




















void
nsPluginInstanceOwner::NativeImageDraw(NPRect* invalidRect)
{
  
  if (!mBlitParentElement)
    return;

  
  if (NSToIntCeil(mAbsolutePositionClip.Width()) == 0 ||
      NSToIntCeil(mAbsolutePositionClip.Height()) == 0)
    return;
  
  
  
  PRInt32 absPosWidth  = NSToIntCeil(mAbsolutePosition.Width()) / 2 * 2;
  PRInt32 absPosHeight = NSToIntCeil(mAbsolutePosition.Height()) / 2 * 2;

  
  if (absPosHeight == 0 || absPosWidth == 0)
    return;

  if (!mSharedXImage ||
      mPluginSize.width != absPosWidth ||
      mPluginSize.height != absPosHeight) {
    
    mPluginSize = nsIntSize(absPosWidth, absPosHeight);

    if (NS_FAILED(SetupXShm()))
      return;
  }  
  
  NPWindow* window;
  GetWindow(window);
  NS_ASSERTION(window, "Window can not be null");

  
  
  
  
  NPRect newClipRect;
  newClipRect.left = 0;
  newClipRect.top = 0;
  newClipRect.right = window->width;
  newClipRect.bottom = window->height;
  
  window->clipRect = newClipRect; 
  window->x = 0;
  window->y = 0;
    
  NPSetWindowCallbackStruct* ws_info =
    static_cast<NPSetWindowCallbackStruct*>(window->ws_info);
  ws_info->visual = 0;
  ws_info->colormap = 0;
  ws_info->depth = 16;
  mInstance->SetWindow(window);

  NPEvent pluginEvent;
  NPImageExpose imageExpose;
  XGraphicsExposeEvent& exposeEvent = pluginEvent.xgraphicsexpose;

  
  exposeEvent.type = GraphicsExpose;
  exposeEvent.display = 0;

  
  exposeEvent.drawable = (Drawable)&imageExpose;
  exposeEvent.count = 0;
  exposeEvent.serial = 0;
  exposeEvent.send_event = False;
  exposeEvent.major_code = 0;
  exposeEvent.minor_code = 0;

  exposeEvent.x = 0;
  exposeEvent.y = 0;
  exposeEvent.width  = window->width;
  exposeEvent.height = window->height;

  imageExpose.x = 0;
  imageExpose.y = 0;
  imageExpose.width  = window->width;
  imageExpose.height = window->height;

  imageExpose.depth = 16;

  imageExpose.translateX = 0;
  imageExpose.translateY = 0;

  if (window->width == 0)
    return;
  
  float scale = mAbsolutePosition.Width() / (float) window->width;
  
  imageExpose.scaleX = scale;
  imageExpose.scaleY = scale;

  imageExpose.stride          = mPluginSize.width * 2;
  imageExpose.data            = mSharedXImage->data;
  imageExpose.dataSize.width  = mPluginSize.width;
  imageExpose.dataSize.height = mPluginSize.height; 

  if (invalidRect)
    memset(mSharedXImage->data, 0, mPluginSize.width * mPluginSize.height * 2);

  PRBool eventHandled = PR_FALSE;
  mInstance->HandleEvent(&pluginEvent, &eventHandled);

  if (!eventHandled)
    return;

  
  XRectangle rect;
  rect.x = NSToIntFloor(mAbsolutePositionClip.X());
  rect.y = NSToIntFloor(mAbsolutePositionClip.Y());
  rect.width = NSToIntCeil(mAbsolutePositionClip.Width());
  rect.height = NSToIntCeil(mAbsolutePositionClip.Height());
  
  PRInt32 absPosX = NSToIntFloor(mAbsolutePosition.X());
  PRInt32 absPosY = NSToIntFloor(mAbsolutePosition.Y());
  
  XSetClipRectangles(gdk_x11_get_default_xdisplay(),
                     mXlibSurfGC,
                     absPosX,
                     absPosY, 
                     &rect, 1,
                     Unsorted);

  XShmPutImage(gdk_x11_get_default_xdisplay(),
               mBlitWindow,
               mXlibSurfGC,
               mSharedXImage,
               0,
               0,
               absPosX,
               absPosY,
               mPluginSize.width,
               mPluginSize.height,
               PR_FALSE);
  
  XSetClipRectangles(gdk_x11_get_default_xdisplay(), mXlibSurfGC, 0, 0, nsnull, 0, Unsorted);  

  XFlush(gdk_x11_get_default_xdisplay());
  return;
}
#endif

#if defined(MOZ_WIDGET_GTK2)
nsresult
nsPluginInstanceOwner::Renderer::NativeDraw(GdkDrawable * drawable, 
                                            short offsetX, short offsetY,
                                            GdkRectangle * clipRects, 
                                            PRUint32 numClipRects)

{
#ifdef MOZ_X11
  Visual * visual = GDK_VISUAL_XVISUAL(gdk_drawable_get_visual(drawable));
  Colormap colormap = GDK_COLORMAP_XCOLORMAP(gdk_drawable_get_colormap(drawable));
  Screen * screen = GDK_SCREEN_XSCREEN (gdk_drawable_get_screen(drawable));
#endif
#elif defined(MOZ_WIDGET_QT)
nsresult
nsPluginInstanceOwner::Renderer::NativeDraw(QWidget * drawable,
                                            short offsetX, short offsetY,
                                            QRect * clipRects,
                                            PRUint32 numClipRects)
{
#ifdef MOZ_X11
  QX11Info xinfo = drawable->x11Info();
  Visual * visual = (Visual*) xinfo.visual();
  Colormap colormap = xinfo.colormap();
  Screen * screen = (Screen*) xinfo.screen();
#endif
#endif

  PRBool doupdatewindow = PR_FALSE;

  if (mWindow->x != offsetX || mWindow->y != offsetY) {
    mWindow->x = offsetX;
    mWindow->y = offsetY;
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
#if defined(MOZ_WIDGET_GTK2)
    clipRect.x = clipRects[0].x;
    clipRect.y = clipRects[0].y;
    clipRect.width  = clipRects[0].width;
    clipRect.height = clipRects[0].height;
#elif defined(MOZ_WIDGET_QT)
    clipRect.x = clipRects[0].x();
    clipRect.y = clipRects[0].y();
    clipRect.width  = clipRects[0].width();
    clipRect.height = clipRects[0].height();
#endif
  }
  else {
    
    
    NS_ASSERTION(offsetX >= 0 && offsetY >= 0,
                 "Clip rectangle offsets are negative!");
    clipRect.x = offsetX;
    clipRect.y = offsetY;
    clipRect.width  = mWindow->width;
    clipRect.height = mWindow->height;
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
    ws_info->depth = DepthOfVisual(screen, visual);
    doupdatewindow = PR_TRUE;
  }
#endif

#ifdef MOZ_COMPOSITED_PLUGINS
  if (mWindow->type == NPWindowTypeDrawable)
#endif
  {
    if (doupdatewindow)
      mInstance->SetWindow(mWindow);
  }

#ifdef MOZ_X11
  
  nsIntRect dirtyRect = mDirtyRect + nsIntPoint(offsetX, offsetY);
  
  
  if (!dirtyRect.IntersectRect(dirtyRect, clipRect))
    return NS_OK;

#ifdef MOZ_COMPOSITED_PLUGINS
  if (mWindow->type == NPWindowTypeDrawable) {
#endif
    XEvent pluginEvent;
    XGraphicsExposeEvent& exposeEvent = pluginEvent.xgraphicsexpose;
    
    exposeEvent.type = GraphicsExpose;
    exposeEvent.display = DisplayOfScreen(screen);
    exposeEvent.drawable =
#if defined(MOZ_WIDGET_GTK2)
      GDK_DRAWABLE_XID(drawable);
#elif defined(MOZ_WIDGET_QT)
      drawable->x11PictureHandle();
#endif
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
#ifdef MOZ_COMPOSITED_PLUGINS
  }
  else {
    
    GtkWidget *plug = (GtkWidget*)(((nsPluginNativeWindow*)mWindow)->mPlugWindow);
    

    

    XGCValues gcv;
    gcv.subwindow_mode = IncludeInferiors;
    gcv.graphics_exposures = False;
    GC gc = XCreateGC(GDK_DISPLAY(), gdk_x11_drawable_get_xid(drawable), GCGraphicsExposures | GCSubwindowMode, &gcv);
    

    XCopyArea(GDK_DISPLAY(), gdk_x11_drawable_get_xid(plug->window),
              gdk_x11_drawable_get_xid(drawable),
              gc,
              mDirtyRect.x,
              mDirtyRect.y,
              mDirtyRect.width,
              mDirtyRect.height,
              mDirtyRect.x,
              mDirtyRect.y);
    XFreeGC(GDK_DISPLAY(), gc);
  }
#endif
#endif
  return NS_OK;
}
#endif



NS_IMETHODIMP nsPluginInstanceOwner::Notify(nsITimer* timer)
{
#if defined(XP_MACOSX) && !defined(NP_NO_CARBON)
  if (GetEventModel() != NPEventModelCarbon)
    return NS_OK;

  
  
  
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

        PRBool eventHandled = PR_FALSE;
        mInstance->HandleEvent(&idleEvent, &eventHandled);
      }

      pluginWidget->EndDrawPlugin();
    }
  }
#endif
  return NS_OK;
}

void nsPluginInstanceOwner::StartTimer(unsigned int aDelay)
{
#if defined(XP_MACOSX) && !defined(NP_NO_CARBON)
  if (GetEventModel() != NPEventModelCarbon)
    return;

  if (!mTimerCanceled)
    return;

  
  if (!mPluginTimer) {
    mPluginTimer = do_CreateInstance("@mozilla.org/timer;1");
  }
  if (mPluginTimer) {
    mTimerCanceled = PR_FALSE;
    mPluginTimer->InitWithCallback(this, aDelay, nsITimer::TYPE_REPEATING_SLACK);
  }
#endif
}

void nsPluginInstanceOwner::CancelTimer()
{
  if (mPluginTimer) {
    mPluginTimer->Cancel();
  }
  mTimerCanceled = PR_TRUE;
}

nsresult nsPluginInstanceOwner::Init(nsPresContext* aPresContext,
                                     nsObjectFrame* aFrame,
                                     nsIContent*    aContent)
{
  mLastEventloopNestingLevel = GetEventloopNestingLevel();

  PR_LOG(nsObjectFrameLM, PR_LOG_DEBUG,
         ("nsPluginInstanceOwner::Init() called on %p for frame %p\n", this,
          aFrame));

  mObjectFrame = aFrame;
  mContent = aContent;

  nsWeakFrame weakFrame(aFrame);

  
  
  
  
  aPresContext->EnsureVisible();

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
  
  
  
  
  nsIFrame* parentWithView = mObjectFrame->GetAncestorWithView();
  nsIView* curView = parentWithView ? parentWithView->GetView() : nsnull;
  while (curView) {
    nsIScrollableView* scrollingView = curView->ToScrollableView();
    if (scrollingView)
      scrollingView->AddScrollPositionListener((nsIScrollPositionListener *)this);
    
    curView = curView->GetParent();
  }

  return NS_OK; 
}

void* nsPluginInstanceOwner::GetPluginPort()
{


  void* result = NULL;
  if (mWidget) {
#ifdef XP_WIN
    if (mPluginWindow && (mPluginWindow->type == NPWindowTypeDrawable))
      result = mWidget->GetNativeData(NS_NATIVE_GRAPHIC);
    else
#endif
#ifdef XP_MACOSX
    if (GetDrawingModel() == NPDrawingModelCoreGraphics)
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

  nsIView   *view;
  nsresult  rv = NS_ERROR_FAILURE;

  if (mObjectFrame) {
    

    view = mObjectFrame->GetView();

    if (!view || !mWidget) {
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
          
          nsIWidget* win = mObjectFrame->GetWindow();
          NPSetWindowCallbackStruct* ws_info = 
            static_cast<NPSetWindowCallbackStruct*>(mPluginWindow->ws_info);
          if (win) {
            ws_info->display =
              static_cast<Display*>(win->GetNativeData(NS_NATIVE_DISPLAY));
          }
#ifdef MOZ_WIDGET_GTK2
          else {
            ws_info->display = GDK_DISPLAY();
          }
#endif
#endif
        } else if (mWidget) {
          mWidget->Resize(mPluginWindow->width, mPluginWindow->height,
                          PR_FALSE);

          
          
          mPluginWindow->type = NPWindowTypeWindow;
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

#if defined(MOZ_PLATFORM_HILDON) && defined(MOZ_WIDGET_GTK2)
PRBool nsPluginInstanceOwner::UpdateVisibility()
{
  if (!mInstance)
    return PR_TRUE;

  PRBool handled;
  NPEvent pluginEvent;
  XVisibilityEvent& visibilityEvent = pluginEvent.xvisibility;
  visibilityEvent.type = VisibilityNotify;
  visibilityEvent.display = 0;
  visibilityEvent.state = VisibilityUnobscured;
  mInstance->HandleEvent(&pluginEvent, &handled);

  mWidgetVisible = PR_TRUE;
  return PR_TRUE;
}
#endif

  
#ifdef XP_MACOSX

void* nsPluginInstanceOwner::FixUpPluginWindow(PRInt32 inPaintState)
{
  if (!mWidget || !mPluginWindow || !mInstance || !mObjectFrame)
    return nsnull;

  NPDrawingModel drawingModel = GetDrawingModel();

  nsCOMPtr<nsIPluginWidget> pluginWidget = do_QueryInterface(mWidget);
  if (!pluginWidget)
    return nsnull;

  
  
  void* pluginPort = nsnull;
  if (mInCGPaintLevel > 0) {
    pluginPort = mPluginWindow->window;
  } else {
    pluginPort = SetPluginPortAndDetectChange();
  }

  if (!pluginPort)
    return nsnull;

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
  else if (drawingModel == NPDrawingModelCoreGraphics)
#endif
  {
    
    
    
    
    
    nsIntPoint geckoScreenCoords = mWidget->WidgetToScreenOffset();

    nsRect windowRect;
#ifndef NP_NO_CARBON
    if (GetEventModel() == NPEventModelCarbon)
      NS_NPAPI_CarbonWindowFrame(static_cast<WindowRef>(static_cast<NP_CGContext*>(pluginPort)->window), windowRect);
    else
#endif
    {
      nsIWidget* widget = mObjectFrame->GetWindow();
      if (!widget)
        return nsnull;
      void* nativeData = widget->GetNativeData(NS_NATIVE_WINDOW);
      if (!nativeData)
        return nsnull;
      NS_NPAPI_CocoaWindowFrame(nativeData, windowRect);
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
    
    CancelTimer();
    if (mPluginWindow->clipRect.left == mPluginWindow->clipRect.right ||
        mPluginWindow->clipRect.top == mPluginWindow->clipRect.bottom) {
      StartTimer(HIDDEN_PLUGIN_DELAY);
    }
    else {
      StartTimer(NORMAL_PLUGIN_DELAY);
    }
  } else if (mPluginPortChanged) {
    mInstance->SetWindow(mPluginWindow);
    mPluginPortChanged = PR_FALSE;
  }

#ifndef NP_NO_QUICKDRAW
  if (drawingModel == NPDrawingModelQuickDraw)
    return ::GetWindowFromPort(static_cast<NP_Port*>(pluginPort)->port);
#endif

  if (drawingModel == NPDrawingModelCoreGraphics)
    return static_cast<NP_CGContext*>(pluginPort)->window;

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

#if defined(MOZ_PLATFORM_HILDON) && defined(MOZ_WIDGET_GTK2)
nsresult
nsPluginInstanceOwner::SetAbsoluteScreenPosition(nsIDOMElement* element,
                                                 nsIDOMClientRect* position,
                                                 nsIDOMClientRect* clip)
{
  if ((mBlitParentElement && (mBlitParentElement != element)) ||
      !position || !clip)
    return NS_ERROR_FAILURE;
  
  float left, top, width, height;
  position->GetLeft(&left);
  position->GetTop(&top);
  position->GetWidth(&width);
  position->GetHeight(&height);

  mAbsolutePosition = gfxRect(left, top, width, height);
  
  clip->GetLeft(&left);
  clip->GetTop(&top);
  clip->GetWidth(&width);
  clip->GetHeight(&height);

  mAbsolutePositionClip = gfxRect(left,top, width, height);

  mBlitParentElement = element;
    
  UpdateVisibility();

  if (!mInstance)
    return NS_OK;

  PRBool simpleImageRender = PR_FALSE;
  mInstance->GetValueFromPlugin(NPPVpluginWindowlessLocalBool,
                                &simpleImageRender);
  if (simpleImageRender)
    NativeImageDraw();
  return NS_OK;
}
#endif

