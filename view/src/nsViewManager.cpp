








































#define PL_ARENA_CONST_ALIGN_MASK (sizeof(void*)-1)
#include "plarena.h"

#include "nsAutoPtr.h"
#include "nsViewManager.h"
#include "nsUnitConversion.h"
#include "nsIRenderingContext.h"
#include "nsIDeviceContext.h"
#include "nsGfxCIID.h"
#include "nsIScrollableView.h"
#include "nsView.h"
#include "nsISupportsArray.h"
#include "nsICompositeListener.h"
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsGUIEvent.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsRegion.h"
#include "nsInt64.h"
#include "nsScrollPortView.h"
#include "nsHashtable.h"
#include "nsCOMArray.h"
#include "nsThreadUtils.h"

#include "gfxContext.h"

static NS_DEFINE_IID(kBlenderCID, NS_BLENDER_CID);
static NS_DEFINE_IID(kRegionCID, NS_REGION_CID);
static NS_DEFINE_IID(kRenderingContextCID, NS_RENDERING_CONTEXT_CID);



















#define NSCOORD_NONE      PR_INT32_MIN

#ifdef NS_VM_PERF_METRICS
#include "nsITimeRecorder.h"
#endif



class nsInvalidateEvent : public nsViewManagerEvent {
public:
  nsInvalidateEvent(nsViewManager *vm) : nsViewManagerEvent(vm) {}

  NS_IMETHOD Run() {
    if (mViewManager)
      mViewManager->ProcessInvalidateEvent();
    return NS_OK;
  }
};



static PRBool IsViewVisible(nsView *aView)
{
  for (nsIView *view = aView; view; view = view->GetParent()) {
    
    
    
    
    if (view->GetVisibility() == nsViewVisibility_kHide)
      return PR_FALSE;
  }
  
  
  
  nsIViewObserver* vo = aView->GetViewManager()->GetViewObserver();
  return vo && vo->IsVisible();
}

void
nsViewManager::PostInvalidateEvent()
{
  NS_ASSERTION(IsRootVM(), "Caller screwed up");

  if (!mInvalidateEvent.IsPending()) {
    nsRefPtr<nsViewManagerEvent> ev = new nsInvalidateEvent(this);
    if (NS_FAILED(NS_DispatchToCurrentThread(ev))) {
      NS_WARNING("failed to dispatch nsInvalidateEvent");
    } else {
      mInvalidateEvent = ev;
    }
  }
}

#undef DEBUG_MOUSE_LOCATION

PRInt32 nsViewManager::mVMCount = 0;
nsIRenderingContext* nsViewManager::gCleanupContext = nsnull;


nsVoidArray* nsViewManager::gViewManagers = nsnull;
PRUint32 nsViewManager::gLastUserEventTime = 0;

nsViewManager::nsViewManager()
  : mMouseLocation(NSCOORD_NONE, NSCOORD_NONE)
  , mDelayedResize(NSCOORD_NONE, NSCOORD_NONE)
  , mRootViewManager(this)
{
  if (gViewManagers == nsnull) {
    NS_ASSERTION(mVMCount == 0, "View Manager count is incorrect");
    
    gViewManagers = new nsVoidArray;
  }
 
  if (gCleanupContext == nsnull) {
    
    CallCreateInstance(kRenderingContextCID, &gCleanupContext);
    NS_ASSERTION(gCleanupContext,
                 "Wasn't able to create a graphics context for cleanup");
  }

  gViewManagers->AppendElement(this);

  mVMCount++;
  
  
  mDefaultBackgroundColor = NS_RGBA(0, 0, 0, 0);
  mHasPendingUpdates = PR_FALSE;
  mRecursiveRefreshPending = PR_FALSE;
  mUpdateBatchFlags = 0;
}

nsViewManager::~nsViewManager()
{
  if (mRootView) {
    
    mRootView->Destroy();
    mRootView = nsnull;
  }

  
  
  mInvalidateEvent.Revoke();
  mSynthMouseMoveEvent.Revoke();
  
  if (!IsRootVM()) {
    
    NS_RELEASE(mRootViewManager);
  }

  mRootScrollable = nsnull;

  NS_ASSERTION((mVMCount > 0), "underflow of viewmanagers");
  --mVMCount;

#ifdef DEBUG
  PRBool removed =
#endif
    gViewManagers->RemoveElement(this);
  NS_ASSERTION(removed, "Viewmanager instance not was not in the global list of viewmanagers");

  if (0 == mVMCount) {
    
    
   
    NS_ASSERTION(gViewManagers != nsnull, "About to delete null gViewManagers");
    delete gViewManagers;
    gViewManagers = nsnull;

    
    

    
    
    
    NS_IF_RELEASE(gCleanupContext);
  }

  mObserver = nsnull;
  mContext = nsnull;

  if (nsnull != mCompositeListeners) {
    mCompositeListeners->Clear();
    NS_RELEASE(mCompositeListeners);
  }
}

NS_IMPL_ISUPPORTS1(nsViewManager, nsIViewManager)

nsresult
nsViewManager::CreateRegion(nsIRegion* *result)
{
  nsresult rv;

  if (!mRegionFactory) {
    mRegionFactory = do_GetClassObject(kRegionCID, &rv);
    if (NS_FAILED(rv)) {
      *result = nsnull;
      return rv;
    }
  }

  nsIRegion* region = nsnull;
  rv = CallCreateInstance(mRegionFactory.get(), &region);
  if (NS_SUCCEEDED(rv)) {
    rv = region->Init();
    *result = region;
  }
  return rv;
}



NS_IMETHODIMP nsViewManager::Init(nsIDeviceContext* aContext)
{
  NS_PRECONDITION(nsnull != aContext, "null ptr");

  if (nsnull == aContext) {
    return NS_ERROR_NULL_POINTER;
  }
  if (nsnull != mContext) {
    return NS_ERROR_ALREADY_INITIALIZED;
  }
  mContext = aContext;

  mRefreshEnabled = PR_TRUE;

  mMouseGrabber = nsnull;

  return NS_OK;
}

NS_IMETHODIMP_(nsIView *)
nsViewManager::CreateView(const nsRect& aBounds,
                          const nsIView* aParent,
                          nsViewVisibility aVisibilityFlag)
{
  nsView *v = new nsView(this, aVisibilityFlag);
  if (v) {
    v->SetPosition(aBounds.x, aBounds.y);
    nsRect dim(0, 0, aBounds.width, aBounds.height);
    v->SetDimensions(dim, PR_FALSE);
    v->SetParent(NS_STATIC_CAST(nsView*, NS_CONST_CAST(nsIView*, aParent)));
  }
  return v;
}

NS_IMETHODIMP_(nsIScrollableView *)
nsViewManager::CreateScrollableView(const nsRect& aBounds,
                                    const nsIView* aParent)
{
  nsScrollPortView *v = new nsScrollPortView(this);
  if (v) {
    v->SetPosition(aBounds.x, aBounds.y);
    nsRect dim(0, 0, aBounds.width, aBounds.height);
    v->SetDimensions(dim, PR_FALSE);
    v->SetParent(NS_STATIC_CAST(nsView*, NS_CONST_CAST(nsIView*, aParent)));
  }
  return v;
}

NS_IMETHODIMP nsViewManager::GetRootView(nsIView *&aView)
{
  aView = mRootView;
  return NS_OK;
}

NS_IMETHODIMP nsViewManager::SetRootView(nsIView *aView)
{
  nsView* view = NS_STATIC_CAST(nsView*, aView);

  NS_PRECONDITION(!view || view->GetViewManager() == this,
                  "Unexpected viewmanager on root view");
  
  
  
  mRootView = view;

  if (mRootView) {
    nsView* parent = mRootView->GetParent();
    if (parent) {
      
      
      parent->InsertChild(mRootView, nsnull);
    } else {
      InvalidateHierarchy();
    }

    mRootView->SetZIndex(PR_FALSE, 0, PR_FALSE);
  }
  

  return NS_OK;
}

NS_IMETHODIMP nsViewManager::GetWindowDimensions(nscoord *aWidth, nscoord *aHeight)
{
  if (nsnull != mRootView) {
    if (mDelayedResize == nsSize(NSCOORD_NONE, NSCOORD_NONE)) {
      nsRect dim;
      mRootView->GetDimensions(dim);
      *aWidth = dim.width;
      *aHeight = dim.height;
    } else {
      *aWidth = mDelayedResize.width;
      *aHeight = mDelayedResize.height;
    }
  }
  else
    {
      *aWidth = 0;
      *aHeight = 0;
    }
  return NS_OK;
}

NS_IMETHODIMP nsViewManager::SetWindowDimensions(nscoord aWidth, nscoord aHeight)
{
  if (mRootView) {
    if (IsViewVisible(mRootView)) {
      mDelayedResize.SizeTo(NSCOORD_NONE, NSCOORD_NONE);
      DoSetWindowDimensions(aWidth, aHeight);
    } else {
      mDelayedResize.SizeTo(aWidth, aHeight);
    }
  }

  return NS_OK;
}


static
PRBool DoDoubleBuffering(void)
{
  static PRBool gotDoublebufferPrefs = PR_FALSE;
  static PRBool doDoublebuffering    = PR_TRUE;  
  
  if (!gotDoublebufferPrefs) {
    nsCOMPtr<nsIPrefBranch> prefBranch = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (prefBranch) {
      PRBool val;
      if (NS_SUCCEEDED(prefBranch->GetBoolPref("viewmanager.do_doublebuffering", &val))) {
        doDoublebuffering = val;
      }
    }

#ifdef DEBUG
    if (!doDoublebuffering) {
      printf("nsViewManager: Note: Double-buffering disabled via prefs.\n");
    }
#endif 

    gotDoublebufferPrefs = PR_TRUE;
  }
  
  return doDoublebuffering;
}

static void ConvertNativeRegionToAppRegion(nsIRegion* aIn, nsRegion* aOut,
                                           nsIDeviceContext* context)
{
  nsRegionRectSet* rects = nsnull;
  aIn->GetRects(&rects);
  if (!rects)
    return;

  PRInt32 p2a = context->AppUnitsPerDevPixel();

  PRUint32 i;
  for (i = 0; i < rects->mNumRects; i++) {
    const nsRegionRect& inR = rects->mRects[i];
    nsRect outR;
    outR.x = NSIntPixelsToAppUnits(inR.x, p2a);
    outR.y = NSIntPixelsToAppUnits(inR.y, p2a);
    outR.width = NSIntPixelsToAppUnits(inR.width, p2a);
    outR.height = NSIntPixelsToAppUnits(inR.height, p2a);
    aOut->Or(*aOut, outR);
  }

  aIn->FreeRects(rects);
}

static nsView* GetDisplayRootFor(nsView* aView)
{
  nsView *displayRoot = aView;
  for (;;) {
    nsView *displayParent = displayRoot->GetParent();
    if (!displayParent)
      return displayRoot;

    if (displayRoot->GetFloating() && !displayParent->GetFloating())
      return displayRoot;
    displayRoot = displayParent;
  }
}




void nsViewManager::Refresh(nsView *aView, nsIRenderingContext *aContext,
                            nsIRegion *aRegion, PRUint32 aUpdateFlags)
{
  NS_ASSERTION(aRegion != nsnull, "Null aRegion");

  if (! IsRefreshEnabled())
    return;

  nsRect viewRect;
  aView->GetDimensions(viewRect);

  
  nsRegion damageRegion;
  
  ConvertNativeRegionToAppRegion(aRegion, &damageRegion, mContext);
  
  damageRegion.MoveBy(viewRect.x, viewRect.y);

  
  damageRegion.And(damageRegion, viewRect);
  if (damageRegion.IsEmpty()) {
#ifdef DEBUG_roc
    nsRect damageRect = damageRegion.GetBounds();
    printf("XXX Damage rectangle (%d,%d,%d,%d) does not intersect the widget's view (%d,%d,%d,%d)!\n",
           damageRect.x, damageRect.y, damageRect.width, damageRect.height,
           viewRect.x, viewRect.y, viewRect.width, viewRect.height);
#endif
    return;
  }

#ifdef NS_VM_PERF_METRICS
  MOZ_TIMER_DEBUGLOG(("Reset nsViewManager::Refresh(region), this=%p\n", this));
  MOZ_TIMER_RESET(mWatch);

  MOZ_TIMER_DEBUGLOG(("Start: nsViewManager::Refresh(region)\n"));
  MOZ_TIMER_START(mWatch);
#endif

  NS_ASSERTION(!IsPainting(), "recursive painting not permitted");
  if (IsPainting()) {
    RootViewManager()->mRecursiveRefreshPending = PR_TRUE;
    return;
  }  
  SetPainting(PR_TRUE);

  nsCOMPtr<nsIRenderingContext> localcx;
  nsIDrawingSurface*    ds = nsnull;

  if (nsnull == aContext)
    {
      localcx = CreateRenderingContext(*aView);

      
      if (nsnull == localcx) {
        SetPainting(PR_FALSE);
        return;
      }
    } else {
      
      localcx = aContext;
    }

  
  if (nsnull != mCompositeListeners) {
    PRUint32 listenerCount;
    if (NS_SUCCEEDED(mCompositeListeners->Count(&listenerCount))) {
      nsCOMPtr<nsICompositeListener> listener;
      for (PRUint32 i = 0; i < listenerCount; i++) {
        if (NS_SUCCEEDED(mCompositeListeners->QueryElementAt(i, NS_GET_IID(nsICompositeListener), getter_AddRefs(listener)))) {
          listener->WillRefreshRegion(this, aView, aContext, aRegion, aUpdateFlags);
        }
      }
    }
  }

  
  nsRect damageRect = damageRegion.GetBounds();
  PRInt32 p2a = mContext->AppUnitsPerDevPixel();

  nsRefPtr<gfxContext> ctx =
    (gfxContext*) localcx->GetNativeGraphicData(nsIRenderingContext::NATIVE_THEBES_CONTEXT);

  ctx->Save();

  ctx->Translate(gfxPoint(NSAppUnitsToIntPixels(viewRect.x, p2a),
                          NSAppUnitsToIntPixels(viewRect.y, p2a)));

  nsRegion opaqueRegion;
  AddCoveringWidgetsToOpaqueRegion(opaqueRegion, mContext, aView);
  damageRegion.Sub(damageRegion, opaqueRegion);

  RenderViews(aView, *localcx, damageRegion, ds);

  ctx->Restore();

  SetPainting(PR_FALSE);

  
  if (nsnull != mCompositeListeners) {
    PRUint32 listenerCount;
    if (NS_SUCCEEDED(mCompositeListeners->Count(&listenerCount))) {
      nsCOMPtr<nsICompositeListener> listener;
      for (PRUint32 i = 0; i < listenerCount; i++) {
        if (NS_SUCCEEDED(mCompositeListeners->QueryElementAt(i, NS_GET_IID(nsICompositeListener), getter_AddRefs(listener)))) {
          listener->DidRefreshRegion(this, aView, aContext, aRegion, aUpdateFlags);
        }
      }
    }
  }

  if (RootViewManager()->mRecursiveRefreshPending) {
    
    
    RootViewManager()->mRecursiveRefreshPending = PR_FALSE;
    UpdateAllViews(aUpdateFlags);
  }

#ifdef NS_VM_PERF_METRICS
  MOZ_TIMER_DEBUGLOG(("Stop: nsViewManager::Refresh(region), this=%p\n", this));
  MOZ_TIMER_STOP(mWatch);
  MOZ_TIMER_LOG(("vm2 Paint time (this=%p): ", this));
  MOZ_TIMER_PRINT(mWatch);
#endif

}

void nsViewManager::DefaultRefresh(nsView* aView, nsIRenderingContext *aContext, const nsRect* aRect)
{
  NS_PRECONDITION(aView, "Must have a view to work with!");
  nsIWidget* widget = aView->GetNearestWidget(nsnull);
  if (! widget)
    return;

  nsCOMPtr<nsIRenderingContext> context = aContext;
  if (! aContext)
    context = CreateRenderingContext(*aView);

  if (! context)
    return;

  nscolor bgcolor = mDefaultBackgroundColor;

  if (NS_GET_A(mDefaultBackgroundColor) == 0) {
    NS_WARNING("nsViewManager: Asked to paint a default background, but no default background color is set!");
    return;
  }

  context->SetColor(bgcolor);
  context->FillRect(*aRect);
}

void nsViewManager::AddCoveringWidgetsToOpaqueRegion(nsRegion &aRgn, nsIDeviceContext* aContext,
                                                     nsView* aRootView) {
  NS_PRECONDITION(aRootView, "Must have root view");
  
  
  
  
  
  
  
  
  
  
  
  
  
  aRgn.SetEmpty();

  nsIWidget* widget = aRootView->GetNearestWidget(nsnull);
  if (!widget) {
    return;
  }
  
  for (nsIWidget* childWidget = widget->GetFirstChild();
       childWidget;
       childWidget = childWidget->GetNextSibling()) {
    PRBool widgetVisible;
    childWidget->IsVisible(widgetVisible);
    if (widgetVisible) {
      nsView* view = nsView::GetViewFor(childWidget);
      if (view && view->GetVisibility() == nsViewVisibility_kShow
          && !view->GetFloating()) {
        nsRect bounds = view->GetBounds();
        if (bounds.width > 0 && bounds.height > 0) {
          nsView* viewParent = view->GetParent();
            
          while (viewParent && viewParent != aRootView) {
            viewParent->ConvertToParentCoords(&bounds.x, &bounds.y);
            viewParent = viewParent->GetParent();
          }
            
          
          
          
          if (viewParent) {
            aRgn.Or(aRgn, bounds);
          }
        }
      }
    }
  }
}






void nsViewManager::RenderViews(nsView *aView, nsIRenderingContext& aRC,
                                const nsRegion& aRegion, nsIDrawingSurface* aRCSurface)
{
  if (mObserver) {
    nsView* displayRoot = GetDisplayRootFor(aView);
    nsPoint offsetToRoot = aView->GetOffsetTo(displayRoot); 
    nsRegion damageRegion(aRegion);
    damageRegion.MoveBy(offsetToRoot);
    
    aRC.PushState();
    aRC.Translate(-offsetToRoot.x, -offsetToRoot.y);
    mObserver->Paint(displayRoot, &aRC, damageRegion);
    aRC.PopState();
  }
}

static nsresult NewOffscreenContext(nsIDeviceContext* deviceContext, nsIDrawingSurface* surface,
                                    const nsRect& aRect, nsIRenderingContext* *aResult)
{
  nsresult             rv;
  nsIRenderingContext *context = nsnull;

  rv = deviceContext->CreateRenderingContext(surface, context);
  if (NS_FAILED(rv))
    return rv;

  
  nsRect clip(0, 0, aRect.width, aRect.height);
  context->SetClipRect(clip, nsClipCombine_kReplace);

  context->Translate(-aRect.x, -aRect.y);

  *aResult = context;
  return NS_OK;
}

nsIViewManager::BlendingBuffers::BlendingBuffers(nsIRenderingContext* aCleanupContext) {
  mCleanupContext = aCleanupContext;

  mOwnBlackSurface = PR_FALSE;
  mWhite = nsnull;
  mBlack = nsnull;
}

nsIViewManager::BlendingBuffers::~BlendingBuffers() {
  if (mWhite)
    mCleanupContext->DestroyDrawingSurface(mWhite);

  if (mBlack && mOwnBlackSurface)
    mCleanupContext->DestroyDrawingSurface(mBlack);
}












nsIViewManager::BlendingBuffers*
nsViewManager::CreateBlendingBuffers(nsIRenderingContext *aRC,
                                     PRBool aBorrowContext,
                                     nsIDrawingSurface* aBorrowSurface,
                                     PRBool aNeedAlpha,
                                     const nsRect& aRect)
{
  nsresult rv;

  
  if (!mBlender) {
    mBlender = do_CreateInstance(kBlenderCID, &rv);
    if (NS_FAILED(rv))
      return nsnull;
    rv = mBlender->Init(mContext);
    if (NS_FAILED(rv)) {
      mBlender = nsnull;
      return nsnull;
    }
  }

  BlendingBuffers* buffers = new BlendingBuffers(aRC);
  if (!buffers)
    return nsnull;

  buffers->mOffset = nsPoint(aRect.x, aRect.y);

  nsRect offscreenBounds(0, 0, aRect.width, aRect.height);
  offscreenBounds.ScaleRoundOut(1.0f / mContext->AppUnitsPerDevPixel());

  if (aBorrowContext) {
    buffers->mBlackCX = aRC;
    buffers->mBlack = aBorrowSurface;
  } else {
    rv = aRC->CreateDrawingSurface(offscreenBounds, NS_CREATEDRAWINGSURFACE_FOR_PIXEL_ACCESS, buffers->mBlack);
    if (NS_FAILED(rv)) {
      delete buffers;
      return nsnull;
    }
    buffers->mOwnBlackSurface = PR_TRUE;
    
    rv = NewOffscreenContext(mContext, buffers->mBlack, aRect, getter_AddRefs(buffers->mBlackCX));
    if (NS_FAILED(rv)) {
      delete buffers;
      return nsnull;
    }
  }

  if (aNeedAlpha) {
    rv = aRC->CreateDrawingSurface(offscreenBounds, NS_CREATEDRAWINGSURFACE_FOR_PIXEL_ACCESS, buffers->mWhite);
    if (NS_FAILED(rv)) {
      delete buffers;
      return nsnull;
    }
    
    rv = NewOffscreenContext(mContext, buffers->mWhite, aRect, getter_AddRefs(buffers->mWhiteCX));
    if (NS_FAILED(rv)) {
      delete buffers;
      return nsnull;
    }
    
    
    
    buffers->mBlackCX->SetColor(NS_RGB(0, 0, 0));
    buffers->mBlackCX->FillRect(aRect);
    buffers->mWhiteCX->SetColor(NS_RGB(255, 255, 255));
    buffers->mWhiteCX->FillRect(aRect);
  }

  return buffers;
}

void nsViewManager::ProcessPendingUpdates(nsView* aView, PRBool aDoInvalidate)
{
  NS_ASSERTION(IsRootVM(), "Updates will be missed");

  
  if (!aView) {
    return;
  }

  if (aView->HasWidget()) {
    aView->ResetWidgetBounds(PR_FALSE, PR_FALSE, PR_TRUE);
  }

  
  for (nsView* childView = aView->GetFirstChild(); childView;
       childView = childView->GetNextSibling()) {
    ProcessPendingUpdates(childView, aDoInvalidate);
  }

  if (aDoInvalidate && aView->HasNonEmptyDirtyRegion()) {
    
    
    NS_ASSERTION(mRefreshEnabled, "Cannot process pending updates with refresh disabled");
    nsRegion* dirtyRegion = aView->GetDirtyRegion();
    if (dirtyRegion) {
      UpdateWidgetArea(aView, *dirtyRegion, nsnull);
      dirtyRegion->SetEmpty();
    }
  }
}

NS_IMETHODIMP nsViewManager::Composite()
{
  if (!IsRootVM()) {
    return RootViewManager()->Composite();
  }
  
  if (UpdateCount() > 0)
    {
      ForceUpdate();
      ClearUpdateCount();
    }

  return NS_OK;
}

NS_IMETHODIMP nsViewManager::UpdateView(nsIView *aView, PRUint32 aUpdateFlags)
{
  
  nsView* view = NS_STATIC_CAST(nsView*, aView);

  nsRect bounds = view->GetBounds();
  view->ConvertFromParentCoords(&bounds.x, &bounds.y);
  return UpdateView(view, bounds, aUpdateFlags);
}




static void
AccumulateIntersectionsIntoDirtyRegion(nsView* aTargetView,
                                       nsView* aSourceView,
                                       const nsPoint& aOffset)
{
  if (aSourceView->HasNonEmptyDirtyRegion()) {
    
    
    nsPoint offset = aTargetView->GetOffsetTo(aSourceView);
    nsRegion intersection;
    intersection = *aSourceView->GetDirtyRegion();
    if (!intersection.IsEmpty()) {
      nsRegion* targetRegion = aTargetView->GetDirtyRegion();
      if (targetRegion) {
        intersection.MoveBy(-offset + aOffset);
        targetRegion->Or(*targetRegion, intersection);
        
        targetRegion->SimplifyOutward(20);
      }
    }
  }

  if (aSourceView == aTargetView) {
    
    return;
  }
  
  for (nsView* kid = aSourceView->GetFirstChild();
       kid;
       kid = kid->GetNextSibling()) {
    AccumulateIntersectionsIntoDirtyRegion(aTargetView, kid, aOffset);
  }
}

nsresult
nsViewManager::WillBitBlit(nsView* aView, nsPoint aScrollAmount)
{
  if (!IsRootVM()) {
    RootViewManager()->WillBitBlit(aView, aScrollAmount);
    return NS_OK;
  }

  NS_PRECONDITION(aView, "Must have a view");
  NS_PRECONDITION(aView->HasWidget(), "View must have a widget");

  ++mScrollCnt;
  
  
  
  AccumulateIntersectionsIntoDirtyRegion(aView, GetRootView(), -aScrollAmount);
  return NS_OK;
}


void
nsViewManager::UpdateViewAfterScroll(nsView *aView, const nsRegion& aUpdateRegion)
{
  NS_ASSERTION(RootViewManager()->mScrollCnt > 0,
               "Someone forgot to call WillBitBlit()");
  nsPoint offset = ComputeViewOffset(aView);
  if (!aUpdateRegion.IsEmpty()) {
    
    
    nsRegion update(aUpdateRegion);
    update.MoveBy(offset);
    UpdateWidgetArea(RootViewManager()->GetRootView(), update, nsnull);
    
  }

  Composite();
  --RootViewManager()->mScrollCnt;
}







void
nsViewManager::UpdateWidgetArea(nsView *aWidgetView, const nsRegion &aDamagedRegion,
                                nsView* aIgnoreWidgetView)
{
  if (!IsRefreshEnabled()) {
    
    
    nsRegion* dirtyRegion = aWidgetView->GetDirtyRegion();
    if (!dirtyRegion) return;

    dirtyRegion->Or(*dirtyRegion, aDamagedRegion);
    
    dirtyRegion->SimplifyOutward(8);
    nsViewManager* rootVM = RootViewManager();
    rootVM->mHasPendingUpdates = PR_TRUE;
    rootVM->IncrementUpdateCount();
    return;
    
    
    
  }

  
  nsRegion intersection;
  intersection.And(aWidgetView->GetDimensions(), aDamagedRegion);
  if (intersection.IsEmpty()) {
    return;
  }

  
  if (nsViewVisibility_kHide == aWidgetView->GetVisibility()) {
#ifdef DEBUG
    
    nsIWidget* widget = aWidgetView->GetNearestWidget(nsnull);
    if (widget) {
      PRBool visible;
      widget->IsVisible(visible);
      NS_ASSERTION(!visible, "View is hidden but widget is visible!");
    }
#endif
    return;
  }

  if (aWidgetView == aIgnoreWidgetView) {
    
    return;
  }

  nsIWidget* widget = aWidgetView->GetNearestWidget(nsnull);
  if (!widget) {
    
    
    
    return;
  }

  
  
  
  nsRegion children;
  for (nsIWidget* childWidget = widget->GetFirstChild();
       childWidget;
       childWidget = childWidget->GetNextSibling()) {
    nsView* view = nsView::GetViewFor(childWidget);
    NS_ASSERTION(view != aWidgetView, "will recur infinitely");
    if (view && view->GetVisibility() == nsViewVisibility_kShow) {
      
      
      if (view->GetViewManager()->RootViewManager() == RootViewManager()) {
        
        nsRegion damage = intersection;
        nsPoint offset = view->GetOffsetTo(aWidgetView);
        damage.MoveBy(-offset);
        UpdateWidgetArea(view, damage, aIgnoreWidgetView);
        children.Or(children, view->GetDimensions() + offset);
        children.SimplifyInward(20);
      }
    }
  }

  nsRegion leftOver;
  leftOver.Sub(intersection, children);

  if (!leftOver.IsEmpty()) {
    NS_ASSERTION(IsRefreshEnabled(), "Can only get here with refresh enabled, I hope");

    const nsRect* r;
    for (nsRegionRectIterator iter(leftOver); (r = iter.Next());) {
      nsRect bounds = *r;
      ViewToWidget(aWidgetView, aWidgetView, bounds);
      widget->Invalidate(bounds, PR_FALSE);
    }
  }
}

NS_IMETHODIMP nsViewManager::UpdateView(nsIView *aView, const nsRect &aRect, PRUint32 aUpdateFlags)
{
  NS_PRECONDITION(nsnull != aView, "null view");

  nsView* view = NS_STATIC_CAST(nsView*, aView);

  nsRect damagedRect(aRect);

   
   
   
   
   
  nsRectVisibility rectVisibility;
  GetRectVisibility(view, damagedRect, 0, &rectVisibility);
  if (rectVisibility != nsRectVisibility_kVisible) {
    return NS_OK;
  }

  
  
  
  
  if (view->GetFloating()) {
    nsView* widgetParent = view;

    while (!widgetParent->HasWidget()) {
      widgetParent->ConvertToParentCoords(&damagedRect.x, &damagedRect.y);
      widgetParent = widgetParent->GetParent();
    }

    UpdateWidgetArea(widgetParent, nsRegion(damagedRect), nsnull);
  } else {
    
    
    
    
    damagedRect.MoveBy(ComputeViewOffset(view));

    UpdateWidgetArea(RootViewManager()->GetRootView(), nsRegion(damagedRect), nsnull);
  }

  RootViewManager()->IncrementUpdateCount();

  if (!IsRefreshEnabled()) {
    return NS_OK;
  }

  
  if (aUpdateFlags & NS_VMREFRESH_IMMEDIATE) {
    Composite();
  } 

  return NS_OK;
}

NS_IMETHODIMP nsViewManager::UpdateAllViews(PRUint32 aUpdateFlags)
{
  if (RootViewManager() != this) {
    return RootViewManager()->UpdateAllViews(aUpdateFlags);
  }
  
  UpdateViews(mRootView, aUpdateFlags);
  return NS_OK;
}

void nsViewManager::UpdateViews(nsView *aView, PRUint32 aUpdateFlags)
{
  
  UpdateView(aView, aUpdateFlags);

  
  nsView* childView = aView->GetFirstChild();
  while (nsnull != childView)  {
    UpdateViews(childView, aUpdateFlags);
    childView = childView->GetNextSibling();
  }
}

NS_IMETHODIMP nsViewManager::DispatchEvent(nsGUIEvent *aEvent, nsEventStatus *aStatus)
{
  *aStatus = nsEventStatus_eIgnore;

  switch(aEvent->message)
    {
    case NS_SIZE:
      {
        nsView* view = nsView::GetViewFor(aEvent->widget);

        if (nsnull != view)
          {
            nscoord width = ((nsSizeEvent*)aEvent)->windowSize->width;
            nscoord height = ((nsSizeEvent*)aEvent)->windowSize->height;
            width = ((nsSizeEvent*)aEvent)->mWinWidth;
            height = ((nsSizeEvent*)aEvent)->mWinHeight;

            
            

            if (view == mRootView)
              {
                PRInt32 p2a = mContext->AppUnitsPerDevPixel();
                SetWindowDimensions(NSIntPixelsToAppUnits(width, p2a),
                                    NSIntPixelsToAppUnits(height, p2a));
                *aStatus = nsEventStatus_eConsumeNoDefault;
              }
          }

        break;
      }

    case NS_PAINT:
      {
        nsPaintEvent *event = NS_STATIC_CAST(nsPaintEvent*, aEvent);
        nsView *view = nsView::GetViewFor(aEvent->widget);

        if (!view || !mContext)
          break;

        *aStatus = nsEventStatus_eConsumeNoDefault;

        
        
        nsCOMPtr<nsIRegion> region = event->region;
        if (!region) {
          if (NS_FAILED(CreateRegion(getter_AddRefs(region))))
            break;

          const nsRect& damrect = *event->rect;
          region->SetTo(damrect.x, damrect.y, damrect.width, damrect.height);
        }
        
        if (region->IsEmpty())
          break;

        
        if (IsRefreshEnabled()) {
          
          
          PRBool didResize = PR_FALSE;
          for (nsViewManager *vm = this; vm;
               vm = vm->mRootView->GetParent()
                      ? vm->mRootView->GetParent()->GetViewManager()
                      : nsnull) {
            if (vm->mDelayedResize != nsSize(NSCOORD_NONE, NSCOORD_NONE) &&
                IsViewVisible(vm->mRootView)) {
              vm->DoSetWindowDimensions(vm->mDelayedResize.width,
                                        vm->mDelayedResize.height);
              vm->mDelayedResize.SizeTo(NSCOORD_NONE, NSCOORD_NONE);

              
              vm->UpdateView(vm->mRootView, NS_VMREFRESH_NO_SYNC);
              didResize = PR_TRUE;

              
              
              
              *aStatus = nsEventStatus_eIgnore;
            }
          }

          if (!didResize) {
            

            
            
            
            
            nsViewManager* rootVM = RootViewManager();

            nsIWidget *widget = mRootView->GetWidget();
            PRBool translucentWindow = PR_FALSE;
            if (widget)
                widget->GetWindowTranslucency(translucentWindow);

            if (rootVM->mScrollCnt == 0 && !translucentWindow) {
              nsIViewObserver* observer = GetViewObserver();
              if (observer) {
                
                
                
                
                
                
                
                
                
                
                
                BeginUpdateViewBatch();
                observer->WillPaint();
                EndUpdateViewBatch(NS_VMREFRESH_NO_SYNC);

                
                
                view = nsView::GetViewFor(aEvent->widget);
              }
            }
            
            
            if (rootVM->mHasPendingUpdates) {
              rootVM->ProcessPendingUpdates(mRootView, PR_FALSE);
            }
            
            if (view) {
              Refresh(view, event->renderingContext, region,
                      NS_VMREFRESH_DOUBLE_BUFFER);
            }
          }
        } else {
          
          
          nsRect damRect;
          region->GetBoundingBox(&damRect.x, &damRect.y, &damRect.width, &damRect.height);
          PRInt32 p2a = mContext->AppUnitsPerDevPixel();
          damRect.ScaleRoundOut(float(p2a));
          DefaultRefresh(view, event->renderingContext, &damRect);
        
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          UpdateView(view, damRect, NS_VMREFRESH_NO_SYNC);
        }

        break;
      }

    case NS_CREATE:
    case NS_DESTROY:
    case NS_SETZLEVEL:
    case NS_MOVE:
      


      *aStatus = nsEventStatus_eConsumeNoDefault;
      break;


    case NS_DISPLAYCHANGED:

      
      
      
      *aStatus = nsEventStatus_eConsumeDoDefault;
      break;

    case NS_SYSCOLORCHANGED:
      {
        
        
        
        nsView *view = nsView::GetViewFor(aEvent->widget);
        nsCOMPtr<nsIViewObserver> obs = GetViewObserver();
        if (obs) {
          obs->HandleEvent(view, aEvent, aStatus);
        }
      }
      break; 

    default:
      {
        if ((NS_IS_MOUSE_EVENT(aEvent) &&
             
             NS_STATIC_CAST(nsMouseEvent*,aEvent)->reason ==
               nsMouseEvent::eReal &&
             
             
             
             aEvent->message != NS_MOUSE_EXIT &&
             aEvent->message != NS_MOUSE_ENTER) ||
            NS_IS_KEY_EVENT(aEvent) ||
            NS_IS_IME_EVENT(aEvent)) {
          gLastUserEventTime = PR_IntervalToMicroseconds(PR_IntervalNow());
        }

        if (aEvent->message == NS_DEACTIVATE) {
          PRBool result;
          GrabMouseEvents(nsnull, result);
        }

        
        nsView* baseView = nsView::GetViewFor(aEvent->widget);
        nsView* view = baseView;
        PRBool capturedEvent = PR_FALSE;
        
        if (!NS_IS_KEY_EVENT(aEvent) && !NS_IS_IME_EVENT(aEvent) &&
            !NS_IS_CONTEXT_MENU_KEY(aEvent) && !NS_IS_FOCUS_EVENT(aEvent) &&
             aEvent->eventStructType != NS_ACCESSIBLE_EVENT) {
          
          
          view = GetDisplayRootFor(baseView);
        }

        
        
        if (NS_IS_MOUSE_EVENT(aEvent) || NS_IS_DRAG_EVENT(aEvent)) {
          nsView* mouseGrabber = GetMouseEventGrabber();
          if (mouseGrabber) {
            view = mouseGrabber;
            capturedEvent = PR_TRUE;
          }
        }

        if (nsnull != view) {
          PRInt32 p2a = mContext->AppUnitsPerDevPixel();

          if ((aEvent->message == NS_MOUSE_MOVE &&
               NS_STATIC_CAST(nsMouseEvent*,aEvent)->reason ==
                 nsMouseEvent::eReal) ||
              aEvent->message == NS_MOUSE_ENTER) {
            
            
            nsPoint rootOffset = baseView->GetDimensions().TopLeft();
            rootOffset += baseView->GetOffsetTo(RootViewManager()->mRootView);
            RootViewManager()->mMouseLocation = aEvent->refPoint +
                nsPoint(NSAppUnitsToIntPixels(rootOffset.x, p2a),
                        NSAppUnitsToIntPixels(rootOffset.y, p2a));
#ifdef DEBUG_MOUSE_LOCATION
            if (aEvent->message == NS_MOUSE_ENTER)
              printf("[vm=%p]got mouse enter for %p\n",
                     this, aEvent->widget);
            printf("[vm=%p]setting mouse location to (%d,%d)\n",
                   this, mMouseLocation.x, mMouseLocation.y);
#endif
            if (aEvent->message == NS_MOUSE_ENTER)
              SynthesizeMouseMove(PR_FALSE);
          } else if (aEvent->message == NS_MOUSE_EXIT) {
            
            
            
            
            
            
            
            RootViewManager()->mMouseLocation = nsPoint(NSCOORD_NONE, NSCOORD_NONE);
#ifdef DEBUG_MOUSE_LOCATION
            printf("[vm=%p]got mouse exit for %p\n",
                   this, aEvent->widget);
            printf("[vm=%p]clearing mouse location\n",
                   this);
#endif
          }

          
          nsPoint offset(0, 0);

          if (view != baseView) {
            
            nsView *parent;
            for (parent = baseView; parent; parent = parent->GetParent())
              parent->ConvertToParentCoords(&offset.x, &offset.y);

            
            for (parent = view; parent; parent = parent->GetParent())
              parent->ConvertFromParentCoords(&offset.x, &offset.y);
          }

          
          nsRect baseViewDimensions;
          if (baseView != nsnull) {
            baseView->GetDimensions(baseViewDimensions);
          }

          nsPoint pt;
          pt.x = baseViewDimensions.x + 
            NSFloatPixelsToAppUnits(float(aEvent->refPoint.x) + 0.5f, p2a);
          pt.y = baseViewDimensions.y + 
            NSFloatPixelsToAppUnits(float(aEvent->refPoint.y) + 0.5f, p2a);
          pt += offset;

          *aStatus = HandleEvent(view, pt, aEvent, capturedEvent);

          
          
          
          if (aEvent->message==NS_TEXT_TEXT) {
            ((nsTextEvent*)aEvent)->theReply.mCursorPosition.x=NSAppUnitsToIntPixels(((nsTextEvent*)aEvent)->theReply.mCursorPosition.x, p2a);
            ((nsTextEvent*)aEvent)->theReply.mCursorPosition.y=NSAppUnitsToIntPixels(((nsTextEvent*)aEvent)->theReply.mCursorPosition.y, p2a);
            ((nsTextEvent*)aEvent)->theReply.mCursorPosition.width=NSAppUnitsToIntPixels(((nsTextEvent*)aEvent)->theReply.mCursorPosition.width, p2a);
            ((nsTextEvent*)aEvent)->theReply.mCursorPosition.height=NSAppUnitsToIntPixels(((nsTextEvent*)aEvent)->theReply.mCursorPosition.height, p2a);
          }
          if((aEvent->message==NS_COMPOSITION_START) ||
             (aEvent->message==NS_COMPOSITION_QUERY)) {
            ((nsCompositionEvent*)aEvent)->theReply.mCursorPosition.x=NSAppUnitsToIntPixels(((nsCompositionEvent*)aEvent)->theReply.mCursorPosition.x, p2a);
            ((nsCompositionEvent*)aEvent)->theReply.mCursorPosition.y=NSAppUnitsToIntPixels(((nsCompositionEvent*)aEvent)->theReply.mCursorPosition.y, p2a);
            ((nsCompositionEvent*)aEvent)->theReply.mCursorPosition.width=NSAppUnitsToIntPixels(((nsCompositionEvent*)aEvent)->theReply.mCursorPosition.width, p2a);
            ((nsCompositionEvent*)aEvent)->theReply.mCursorPosition.height=NSAppUnitsToIntPixels(((nsCompositionEvent*)aEvent)->theReply.mCursorPosition.height, p2a);
          }
          if(aEvent->message==NS_QUERYCARETRECT) {
            ((nsQueryCaretRectEvent*)aEvent)->theReply.mCaretRect.x=NSAppUnitsToIntPixels(((nsQueryCaretRectEvent*)aEvent)->theReply.mCaretRect.x, p2a);
            ((nsQueryCaretRectEvent*)aEvent)->theReply.mCaretRect.y=NSAppUnitsToIntPixels(((nsQueryCaretRectEvent*)aEvent)->theReply.mCaretRect.y, p2a);
            ((nsQueryCaretRectEvent*)aEvent)->theReply.mCaretRect.width=NSAppUnitsToIntPixels(((nsQueryCaretRectEvent*)aEvent)->theReply.mCaretRect.width, p2a);
            ((nsQueryCaretRectEvent*)aEvent)->theReply.mCaretRect.height=NSAppUnitsToIntPixels(((nsQueryCaretRectEvent*)aEvent)->theReply.mCaretRect.height, p2a);
          }
        }
    
        break;
      }
    }

  return NS_OK;
}

nsEventStatus nsViewManager::HandleEvent(nsView* aView, nsPoint aPoint,
                                         nsGUIEvent* aEvent, PRBool aCaptured) {



  
  
  
  nsCOMPtr<nsIViewObserver> obs = aView->GetViewManager()->GetViewObserver();
  nsEventStatus status = nsEventStatus_eIgnore;
  if (obs) {
     obs->HandleEvent(aView, aEvent, &status);
  }

  return status;
}

NS_IMETHODIMP nsViewManager::GrabMouseEvents(nsIView *aView, PRBool &aResult)
{
  if (!IsRootVM()) {
    return RootViewManager()->GrabMouseEvents(aView, aResult);
  }

  
  
  if (aView && NS_STATIC_CAST(nsView*, aView)->GetVisibility()
               == nsViewVisibility_kHide) {
    aView = nsnull;
  }

#ifdef DEBUG_mjudge
  if (aView)
    {
      printf("capturing mouse events for view %x\n",aView);
    }
  printf("removing mouse capture from view %x\n",mMouseGrabber);
#endif

  mMouseGrabber = NS_STATIC_CAST(nsView*, aView);
  aResult = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP nsViewManager::GetMouseEventGrabber(nsIView *&aView)
{
  aView = GetMouseEventGrabber();
  return NS_OK;
}



void nsViewManager::ReparentChildWidgets(nsIView* aView, nsIWidget *aNewWidget)
{
  if (aView->HasWidget()) {
    
    
    
    
    nsIWidget* widget = aView->GetWidget();
    nsIWidget* parentWidget = widget->GetParent();
    if (parentWidget != aNewWidget) {
#ifdef DEBUG
      nsresult rv =
#endif
        widget->SetParent(aNewWidget);
      NS_ASSERTION(NS_SUCCEEDED(rv), "SetParent failed!");
    }
    return;
  }

  
  

  nsView* view = NS_STATIC_CAST(nsView*, aView);
  for (nsView *kid = view->GetFirstChild(); kid; kid = kid->GetNextSibling()) {
    ReparentChildWidgets(kid, aNewWidget);
  }
}



void nsViewManager::ReparentWidgets(nsIView* aView, nsIView *aParent)
{
  NS_PRECONDITION(aParent, "Must have a parent");
  NS_PRECONDITION(aView, "Must have a view");
  
  
  
  
  
  
  
  
  nsView* view = NS_STATIC_CAST(nsView*, aView);
  if (view->HasWidget() || view->GetFirstChild()) {
    nsIWidget* parentWidget = aParent->GetNearestWidget(nsnull);
    if (parentWidget) {
      ReparentChildWidgets(aView, parentWidget);
      return;
    }
    NS_WARNING("Can not find a widget for the parent view");
  }
}

NS_IMETHODIMP nsViewManager::InsertChild(nsIView *aParent, nsIView *aChild, nsIView *aSibling,
                                         PRBool aAfter)
{
  nsView* parent = NS_STATIC_CAST(nsView*, aParent);
  nsView* child = NS_STATIC_CAST(nsView*, aChild);
  nsView* sibling = NS_STATIC_CAST(nsView*, aSibling);
  
  NS_PRECONDITION(nsnull != parent, "null ptr");
  NS_PRECONDITION(nsnull != child, "null ptr");
  NS_ASSERTION(sibling == nsnull || sibling->GetParent() == parent,
               "tried to insert view with invalid sibling");
  NS_ASSERTION(!IsViewInserted(child), "tried to insert an already-inserted view");

  if ((nsnull != parent) && (nsnull != child))
    {
      
      

#if 1
      if (nsnull == aSibling) {
        if (aAfter) {
          
          
          parent->InsertChild(child, nsnull);
          ReparentWidgets(child, parent);
        } else {
          
          nsView *kid = parent->GetFirstChild();
          nsView *prev = nsnull;
          while (kid) {
            prev = kid;
            kid = kid->GetNextSibling();
          }
          
          parent->InsertChild(child, prev);
          ReparentWidgets(child, parent);
        }
      } else {
        nsView *kid = parent->GetFirstChild();
        nsView *prev = nsnull;
        while (kid && sibling != kid) {
          
          prev = kid;
          kid = kid->GetNextSibling();
        }
        NS_ASSERTION(kid != nsnull,
                     "couldn't find sibling in child list");
        if (aAfter) {
          
          parent->InsertChild(child, prev);
          ReparentWidgets(child, parent);
        } else {
          
          parent->InsertChild(child, kid);
          ReparentWidgets(child, parent);
        }
      }
#else 
      
      PRInt32 zIndex = child->GetZIndex();
      while (nsnull != kid)
        {
          PRInt32 idx = kid->GetZIndex();

          if (CompareZIndex(zIndex, child->IsTopMost(), child->GetZIndexIsAuto(),
                            idx, kid->IsTopMost(), kid->GetZIndexIsAuto()) >= 0)
            break;

          prev = kid;
          kid = kid->GetNextSibling();
        }

      parent->InsertChild(child, prev);
      ReparentWidgets(child, parent);
#endif

      
      if (parent->GetFloating())
        child->SetFloating(PR_TRUE);

      

      if (nsViewVisibility_kHide != child->GetVisibility())
        UpdateView(child, NS_VMREFRESH_NO_SYNC);
    }
  return NS_OK;
}

NS_IMETHODIMP nsViewManager::InsertZPlaceholder(nsIView *aParent, nsIView *aChild,
                                                nsIView *aSibling, PRBool aAfter)
{
  nsView* parent = NS_STATIC_CAST(nsView*, aParent);
  nsView* child = NS_STATIC_CAST(nsView*, aChild);

  NS_PRECONDITION(nsnull != parent, "null ptr");
  NS_PRECONDITION(nsnull != child, "null ptr");

  nsZPlaceholderView* placeholder = new nsZPlaceholderView(this);
  
  placeholder->SetParent(parent);
  placeholder->SetReparentedView(child);
  placeholder->SetZIndex(child->GetZIndexIsAuto(), child->GetZIndex(), child->IsTopMost());
  child->SetZParent(placeholder);
  
  return InsertChild(parent, placeholder, aSibling, aAfter);
}

NS_IMETHODIMP nsViewManager::InsertChild(nsIView *aParent, nsIView *aChild, PRInt32 aZIndex)
{
  
  
  SetViewZIndex(aChild, PR_FALSE, aZIndex, PR_FALSE);
  return InsertChild(aParent, aChild, nsnull, PR_TRUE);
}

NS_IMETHODIMP nsViewManager::RemoveChild(nsIView *aChild)
{
  nsView* child = NS_STATIC_CAST(nsView*, aChild);
  NS_ENSURE_ARG_POINTER(child);

  nsView* parent = child->GetParent();

  if (nsnull != parent)
    {
      UpdateView(child, NS_VMREFRESH_NO_SYNC);
      parent->RemoveChild(child);
    }

  return NS_OK;
}

NS_IMETHODIMP nsViewManager::MoveViewBy(nsIView *aView, nscoord aX, nscoord aY)
{
  nsView* view = NS_STATIC_CAST(nsView*, aView);

  nsPoint pt = view->GetPosition();
  MoveViewTo(view, aX + pt.x, aY + pt.y);
  return NS_OK;
}

NS_IMETHODIMP nsViewManager::MoveViewTo(nsIView *aView, nscoord aX, nscoord aY)
{
  nsView* view = NS_STATIC_CAST(nsView*, aView);
  nsPoint oldPt = view->GetPosition();
  nsRect oldArea = view->GetBounds();
  view->SetPosition(aX, aY);

  

  if ((aX != oldPt.x) || (aY != oldPt.y)) {
    if (view->GetVisibility() != nsViewVisibility_kHide) {
      nsView* parentView = view->GetParent();
      UpdateView(parentView, oldArea, NS_VMREFRESH_NO_SYNC);
      UpdateView(parentView, view->GetBounds(), NS_VMREFRESH_NO_SYNC);
    }
  }
  return NS_OK;
}

void nsViewManager::InvalidateHorizontalBandDifference(nsView *aView, const nsRect& aRect, const nsRect& aCutOut,
  PRUint32 aUpdateFlags, nscoord aY1, nscoord aY2, PRBool aInCutOut) {
  nscoord height = aY2 - aY1;
  if (aRect.x < aCutOut.x) {
    nsRect r(aRect.x, aY1, aCutOut.x - aRect.x, height);
    UpdateView(aView, r, aUpdateFlags);
  }
  if (!aInCutOut && aCutOut.x < aCutOut.XMost()) {
    nsRect r(aCutOut.x, aY1, aCutOut.width, height);
    UpdateView(aView, r, aUpdateFlags);
  }
  if (aCutOut.XMost() < aRect.XMost()) {
    nsRect r(aCutOut.XMost(), aY1, aRect.XMost() - aCutOut.XMost(), height);
    UpdateView(aView, r, aUpdateFlags);
  }
}

void nsViewManager::InvalidateRectDifference(nsView *aView, const nsRect& aRect, const nsRect& aCutOut,
  PRUint32 aUpdateFlags) {
  if (aRect.y < aCutOut.y) {
    InvalidateHorizontalBandDifference(aView, aRect, aCutOut, aUpdateFlags, aRect.y, aCutOut.y, PR_FALSE);
  }
  if (aCutOut.y < aCutOut.YMost()) {
    InvalidateHorizontalBandDifference(aView, aRect, aCutOut, aUpdateFlags, aCutOut.y, aCutOut.YMost(), PR_TRUE);
  }
  if (aCutOut.YMost() < aRect.YMost()) {
    InvalidateHorizontalBandDifference(aView, aRect, aCutOut, aUpdateFlags, aCutOut.YMost(), aRect.YMost(), PR_FALSE);
  }
}

NS_IMETHODIMP nsViewManager::ResizeView(nsIView *aView, const nsRect &aRect, PRBool aRepaintExposedAreaOnly)
{
  nsView* view = NS_STATIC_CAST(nsView*, aView);
  nsRect oldDimensions;

  view->GetDimensions(oldDimensions);
  if (oldDimensions != aRect) {
    nsView* parentView = view->GetParent();
    if (parentView == nsnull)
      parentView = view;

    
    
    if (view->GetVisibility() == nsViewVisibility_kHide) {  
      view->SetDimensions(aRect, PR_FALSE);
    } else {
      if (!aRepaintExposedAreaOnly) {
        
        view->SetDimensions(aRect, PR_TRUE);

        UpdateView(view, aRect, NS_VMREFRESH_NO_SYNC);
        view->ConvertToParentCoords(&oldDimensions.x, &oldDimensions.y);
        UpdateView(parentView, oldDimensions, NS_VMREFRESH_NO_SYNC);
      } else {
        view->SetDimensions(aRect, PR_TRUE);

        InvalidateRectDifference(view, aRect, oldDimensions, NS_VMREFRESH_NO_SYNC);
        nsRect r = aRect;
        view->ConvertToParentCoords(&r.x, &r.y);
        view->ConvertToParentCoords(&oldDimensions.x, &oldDimensions.y);
        InvalidateRectDifference(parentView, oldDimensions, r, NS_VMREFRESH_NO_SYNC);
      } 
    }
  }

  
  
  
  
  

  return NS_OK;
}

PRBool nsViewManager::CanScrollWithBitBlt(nsView* aView, nsPoint aDelta,
                                          nsRegion* aUpdateRegion)
{
  NS_ASSERTION(!IsPainting(),
               "View manager shouldn't be scrolling during a paint");
  if (IsPainting() || !mObserver) {
    return PR_FALSE; 
  }

  nsView* displayRoot = GetDisplayRootFor(aView);
  nsPoint displayOffset = aView->GetParent()->GetOffsetTo(displayRoot);
  nsRect parentBounds = aView->GetParent()->GetDimensions() + displayOffset;
  
  
  nsRect toScroll;
  toScroll.IntersectRect(parentBounds + aDelta, parentBounds);
  nsresult rv =
    mObserver->ComputeRepaintRegionForCopy(displayRoot, aView, -aDelta, toScroll,
                                           aUpdateRegion);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  aUpdateRegion->MoveBy(-displayOffset);

  return PR_TRUE;
}                                            

NS_IMETHODIMP nsViewManager::SetViewCheckChildEvents(nsIView *aView, PRBool aEnable)
{
  nsView* view = NS_STATIC_CAST(nsView*, aView);

  NS_ASSERTION(!(nsnull == view), "no view");

  if (aEnable) {
    view->SetViewFlags(view->GetViewFlags() & ~NS_VIEW_FLAG_DONT_CHECK_CHILDREN);
  } else {
    view->SetViewFlags(view->GetViewFlags() | NS_VIEW_FLAG_DONT_CHECK_CHILDREN);
  }

  return NS_OK;
}

NS_IMETHODIMP nsViewManager::SetViewFloating(nsIView *aView, PRBool aFloating)
{
  nsView* view = NS_STATIC_CAST(nsView*, aView);

  NS_ASSERTION(!(nsnull == view), "no view");

  view->SetFloating(aFloating);

  return NS_OK;
}

NS_IMETHODIMP nsViewManager::SetViewVisibility(nsIView *aView, nsViewVisibility aVisible)
{
  nsView* view = NS_STATIC_CAST(nsView*, aView);

  if (aVisible != view->GetVisibility()) {
    view->SetVisibility(aVisible);

    if (IsViewInserted(view)) {
      if (!view->HasWidget()) {
        if (nsViewVisibility_kHide == aVisible) {
          nsView* parentView = view->GetParent();
          if (parentView) {
            UpdateView(parentView, view->GetBounds(), NS_VMREFRESH_NO_SYNC);
          }
        }
        else {
          UpdateView(view, NS_VMREFRESH_NO_SYNC);
        }
      }
    }

    
    
    
    for (nsView* childView = view->GetFirstChild(); childView;
         childView = childView->GetNextSibling()) {
      if (!childView->GetClientData()) {
        childView->SetVisibility(aVisible);
      }
    }
  }
  return NS_OK;
}

void nsViewManager::UpdateWidgetsForView(nsView* aView)
{
  NS_PRECONDITION(aView, "Must have view!");

  if (aView->HasWidget()) {
    aView->GetWidget()->Update();
  }

  for (nsView* childView = aView->GetFirstChild();
       childView;
       childView = childView->GetNextSibling()) {
    UpdateWidgetsForView(childView);
  }
}

PRBool nsViewManager::IsViewInserted(nsView *aView)
{
  if (mRootView == aView) {
    return PR_TRUE;
  } else if (aView->GetParent() == nsnull) {
    return PR_FALSE;
  } else {
    nsView* view = aView->GetParent()->GetFirstChild();
    while (view != nsnull) {
      if (view == aView) {
        return PR_TRUE;
      }        
      view = view->GetNextSibling();
    }
    return PR_FALSE;
  }
}

NS_IMETHODIMP nsViewManager::SetViewZIndex(nsIView *aView, PRBool aAutoZIndex, PRInt32 aZIndex, PRBool aTopMost)
{
  nsView* view = NS_STATIC_CAST(nsView*, aView);
  nsresult  rv = NS_OK;

  NS_ASSERTION((view != nsnull), "no view");

  
  
  if (aView == mRootView) {
    return rv;
  }

  PRBool oldTopMost = view->IsTopMost();
  PRBool oldIsAuto = view->GetZIndexIsAuto();

  if (aAutoZIndex) {
    aZIndex = 0;
  }

  PRInt32 oldidx = view->GetZIndex();
  view->SetZIndex(aAutoZIndex, aZIndex, aTopMost);

  if (oldidx != aZIndex || oldTopMost != aTopMost ||
      oldIsAuto != aAutoZIndex) {
    UpdateView(view, NS_VMREFRESH_NO_SYNC);
  }

  nsZPlaceholderView* zParentView = view->GetZParent();
  if (nsnull != zParentView) {
    SetViewZIndex(zParentView, aAutoZIndex, aZIndex, aTopMost);
  }

  return rv;
}

NS_IMETHODIMP nsViewManager::SetViewObserver(nsIViewObserver *aObserver)
{
  mObserver = aObserver;
  return NS_OK;
}

NS_IMETHODIMP nsViewManager::GetViewObserver(nsIViewObserver *&aObserver)
{
  if (nsnull != mObserver) {
    aObserver = mObserver;
    NS_ADDREF(mObserver);
    return NS_OK;
  } else
    return NS_ERROR_NO_INTERFACE;
}

NS_IMETHODIMP nsViewManager::GetDeviceContext(nsIDeviceContext *&aContext)
{
  NS_IF_ADDREF(mContext);
  aContext = mContext;
  return NS_OK;
}

void nsViewManager::GetMaxWidgetBounds(nsRect& aMaxWidgetBounds) const
{
  
  
  aMaxWidgetBounds.width = 0;
  aMaxWidgetBounds.height = 0;
  PRInt32 index = 0;
  for (index = 0; index < mVMCount; index++) {

    nsViewManager* vm = (nsViewManager*)gViewManagers->ElementAt(index);
    nsCOMPtr<nsIWidget> rootWidget;

    if(NS_SUCCEEDED(vm->GetWidget(getter_AddRefs(rootWidget))) && rootWidget)
      {
        nsRect widgetBounds;
        rootWidget->GetBounds(widgetBounds);
        aMaxWidgetBounds.width = PR_MAX(aMaxWidgetBounds.width, widgetBounds.width);
        aMaxWidgetBounds.height = PR_MAX(aMaxWidgetBounds.height, widgetBounds.height);
      }
  }

  
}

PRInt32 nsViewManager::GetViewManagerCount()
{
  return mVMCount;
}

const nsVoidArray* nsViewManager::GetViewManagerArray() 
{
  return gViewManagers;
}

already_AddRefed<nsIRenderingContext>
nsViewManager::CreateRenderingContext(nsView &aView)
{
  nsView              *par = &aView;
  nsIWidget*          win;
  nsIRenderingContext *cx = nsnull;
  nscoord             ax = 0, ay = 0;

  do
    {
      win = par->GetWidget();
      if (win)
        break;

      
      
      
      
      

      if (par != &aView)
        {
          par->ConvertToParentCoords(&ax, &ay);
        }

      par = par->GetParent();
    }
  while (nsnull != par);

  if (nsnull != win)
    {
      mContext->CreateRenderingContext(par, cx);

      if (nsnull != cx)
        cx->Translate(ax, ay);
    }

  return cx;
}

NS_IMETHODIMP nsViewManager::DisableRefresh(void)
{
  if (!IsRootVM()) {
    return RootViewManager()->DisableRefresh();
  }
  
  if (mUpdateBatchCnt > 0)
    return NS_OK;

  mRefreshEnabled = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsViewManager::EnableRefresh(PRUint32 aUpdateFlags)
{
  if (!IsRootVM()) {
    return RootViewManager()->EnableRefresh(aUpdateFlags);
  }
  
  if (mUpdateBatchCnt > 0)
    return NS_OK;

  mRefreshEnabled = PR_TRUE;

  if (!mHasPendingUpdates) {
    
    return NS_OK;
  }

  
  
  if (aUpdateFlags & NS_VMREFRESH_IMMEDIATE) {
    FlushPendingInvalidates();
    Composite();
  } else if (aUpdateFlags & NS_VMREFRESH_DEFERRED) {
    PostInvalidateEvent();
  } else { 
    FlushPendingInvalidates();
  }

  return NS_OK;
}

NS_IMETHODIMP nsViewManager::BeginUpdateViewBatch(void)
{
  if (!IsRootVM()) {
    return RootViewManager()->BeginUpdateViewBatch();
  }
  
  nsresult result = NS_OK;
  
  if (mUpdateBatchCnt == 0) {
    mUpdateBatchFlags = 0;
    result = DisableRefresh();
  }

  if (NS_SUCCEEDED(result))
    ++mUpdateBatchCnt;

  return result;
}

NS_IMETHODIMP nsViewManager::EndUpdateViewBatch(PRUint32 aUpdateFlags)
{
  if (!IsRootVM()) {
    return RootViewManager()->EndUpdateViewBatch(aUpdateFlags);
  }
  
  nsresult result = NS_OK;

  --mUpdateBatchCnt;

  NS_ASSERTION(mUpdateBatchCnt >= 0, "Invalid batch count!");

  if (mUpdateBatchCnt < 0)
    {
      mUpdateBatchCnt = 0;
      return NS_ERROR_FAILURE;
    }

  mUpdateBatchFlags |= aUpdateFlags;
  if (mUpdateBatchCnt == 0) {
    result = EnableRefresh(mUpdateBatchFlags);
  }

  return result;
}

NS_IMETHODIMP nsViewManager::SetRootScrollableView(nsIScrollableView *aScrollable)
{
  mRootScrollable = aScrollable;
  return NS_OK;
}

NS_IMETHODIMP nsViewManager::GetRootScrollableView(nsIScrollableView **aScrollable)
{
  *aScrollable = mRootScrollable;
  return NS_OK;
}

NS_IMETHODIMP nsViewManager::RenderOffscreen(nsIView* aView, nsRect aRect,
                                             PRBool aUntrusted,
                                             PRBool aIgnoreViewportScrolling,
                                             nscolor aBackgroundColor,
                                             nsIRenderingContext** aRenderedContext)
{
  
  
  *aRenderedContext = nsnull;
  if (!mObserver || !IsRefreshEnabled())
    return NS_ERROR_FAILURE;

  nsIScrollableView* scrollableView = nsnull;
  GetRootScrollableView(&scrollableView);
  nsIView* view;
  if (scrollableView) {
    scrollableView->GetScrolledView(view);
  } else {
    GetRootView(view);
  }

  NS_ASSERTION(view == aView,
               "Only support offscreen rendering of the root (scrolled) view");
  if (view != aView)
    return NS_ERROR_FAILURE;

  return mObserver->RenderOffscreen(aRect, aUntrusted, aIgnoreViewportScrolling,
                                    aBackgroundColor, aRenderedContext);
}

NS_IMETHODIMP nsViewManager::AddCompositeListener(nsICompositeListener* aListener)
{
  if (nsnull == mCompositeListeners) {
    nsresult rv = NS_NewISupportsArray(&mCompositeListeners);
    if (NS_FAILED(rv))
      return rv;
  }
  return mCompositeListeners->AppendElement(aListener);
}

NS_IMETHODIMP nsViewManager::RemoveCompositeListener(nsICompositeListener* aListener)
{
  if (nsnull != mCompositeListeners) {
    return mCompositeListeners->RemoveElement(aListener);
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsViewManager::GetWidget(nsIWidget **aWidget)
{
  *aWidget = GetWidget();
  NS_IF_ADDREF(*aWidget);
  return NS_OK;
}

NS_IMETHODIMP nsViewManager::ForceUpdate()
{
  if (!IsRootVM()) {
    return RootViewManager()->ForceUpdate();
  }

  
  UpdateWidgetsForView(mRootView);
  return NS_OK;
}

nsPoint nsViewManager::ComputeViewOffset(const nsView *aView)
{
  NS_PRECONDITION(aView, "Null view in ComputeViewOffset?");
  
  nsPoint origin(0, 0);
#ifdef DEBUG
  const nsView* rootView;
  const nsView* origView = aView;
#endif

  while (aView) {
#ifdef DEBUG
    rootView = aView;
#endif
    origin += aView->GetPosition();
    aView = aView->GetParent();
  }
  NS_ASSERTION(rootView ==
               origView->GetViewManager()->RootViewManager()->GetRootView(),
               "Unexpected root view");
  return origin;
}

PRBool nsViewManager::DoesViewHaveNativeWidget(nsView* aView)
{
  if (aView->HasWidget())
    return (nsnull != aView->GetWidget()->GetNativeData(NS_NATIVE_WIDGET));
  return PR_FALSE;
}


nsView* nsViewManager::GetWidgetView(nsView *aView)
{
  while (aView) {
    if (aView->HasWidget())
      return aView;
    aView = aView->GetParent();
  }
  return nsnull;
}

void nsViewManager::ViewToWidget(nsView *aView, nsView* aWidgetView, nsRect &aRect) const
{
  while (aView != aWidgetView) {
    aView->ConvertToParentCoords(&aRect.x, &aRect.y);
    aView = aView->GetParent();
  }
  
  
  nsRect bounds;
  aWidgetView->GetDimensions(bounds);
  aRect.IntersectRect(aRect, bounds);
  
  aRect.x -= bounds.x;
  aRect.y -= bounds.y;
  
  
  aRect.ScaleRoundOut(1.0f / mContext->AppUnitsPerDevPixel());
}

nsresult nsViewManager::GetVisibleRect(nsRect& aVisibleRect)
{
  nsresult rv = NS_OK;

  
  nsIScrollableView* scrollingView;
  GetRootScrollableView(&scrollingView);

  if (scrollingView) {   
    
    
    nsScrollPortView* clipView = NS_STATIC_CAST(nsScrollPortView*, scrollingView);
    clipView->GetDimensions(aVisibleRect);

    scrollingView->GetScrollPosition(aVisibleRect.x, aVisibleRect.y);
  } else {
    rv = NS_ERROR_FAILURE;
  }

  return rv;
}

nsresult nsViewManager::GetAbsoluteRect(nsView *aView, const nsRect &aRect, 
                                        nsRect& aAbsRect)
{
  nsIScrollableView* scrollingView = nsnull;
  GetRootScrollableView(&scrollingView);
  if (nsnull == scrollingView) { 
    return NS_ERROR_FAILURE;
  }

  nsIView* scrolledIView = nsnull;
  scrollingView->GetScrolledView(scrolledIView);
  
  nsView* scrolledView = NS_STATIC_CAST(nsView*, scrolledIView);

  
  
  aAbsRect = aRect;
  nsView *parentView = aView;
  while ((parentView != nsnull) && (parentView != scrolledView)) {
    parentView->ConvertToParentCoords(&aAbsRect.x, &aAbsRect.y);
    parentView = parentView->GetParent();
  }

  if (parentView != scrolledView) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}


NS_IMETHODIMP nsViewManager::GetRectVisibility(nsIView *aView, 
                                               const nsRect &aRect,
                                               PRUint16 aMinTwips, 
                                               nsRectVisibility *aRectVisibility)
{
  nsView* view = NS_STATIC_CAST(nsView*, aView);

  
  

  *aRectVisibility = nsRectVisibility_kZeroAreaRect;
  if (aRect.width == 0 || aRect.height == 0) {
    return NS_OK;
  }

  
  if (view->GetVisibility() == nsViewVisibility_kHide) {
    return NS_OK; 
  }

  
  
  if (view->GetFloating()) {
    *aRectVisibility = nsRectVisibility_kVisible;
    return NS_OK;
  }

  
  nsRect visibleRect;
  if (GetVisibleRect(visibleRect) == NS_ERROR_FAILURE) {
    *aRectVisibility = nsRectVisibility_kVisible;
    return NS_OK;
  }

  
  
  nsRect absRect;
  if ((GetAbsoluteRect(view, aRect, absRect)) == NS_ERROR_FAILURE) {
    *aRectVisibility = nsRectVisibility_kVisible;
    return NS_OK;
  }
 
  








  if (absRect.y < visibleRect.y  && 
      absRect.y + absRect.height < visibleRect.y + aMinTwips)
    *aRectVisibility = nsRectVisibility_kAboveViewport;
  else if (absRect.y + absRect.height > visibleRect.y + visibleRect.height &&
           absRect.y > visibleRect.y + visibleRect.height - aMinTwips)
    *aRectVisibility = nsRectVisibility_kBelowViewport;
  else if (absRect.x < visibleRect.x && 
           absRect.x + absRect.width < visibleRect.x + aMinTwips)
    *aRectVisibility = nsRectVisibility_kLeftOfViewport;
  else if (absRect.x + absRect.width > visibleRect.x  + visibleRect.width &&
           absRect.x > visibleRect.x + visibleRect.width - aMinTwips)
    *aRectVisibility = nsRectVisibility_kRightOfViewport;
  else
    *aRectVisibility = nsRectVisibility_kVisible;

  return NS_OK;
}

NS_IMETHODIMP
nsViewManager::IsPainting(PRBool& aIsPainting)
{
  aIsPainting = IsPainting();
  return NS_OK;
}

void
nsViewManager::FlushPendingInvalidates()
{
  NS_ASSERTION(IsRootVM(), "Must be root VM for this to be called!\n");
  NS_ASSERTION(mUpdateBatchCnt == 0, "Must not be in an update batch!");
  
  
  

  
  
  
  NS_ASSERTION(gViewManagers, "Better have a viewmanagers array!");

  
  if (mScrollCnt == 0) {
    
    
    
    
    
    PRBool refreshEnabled = mRefreshEnabled;
    mRefreshEnabled = PR_FALSE;
    ++mUpdateBatchCnt;
    
    PRInt32 index;
    for (index = 0; index < mVMCount; index++) {
      nsViewManager* vm = (nsViewManager*)gViewManagers->ElementAt(index);
      if (vm->RootViewManager() == this) {
        
        nsIViewObserver* observer = vm->GetViewObserver();
        if (observer) {
          observer->WillPaint();
          NS_ASSERTION(mUpdateBatchCnt == 1,
                       "Observer did not end view batch?");
        }
      }
    }
    
    --mUpdateBatchCnt;
    
    
    if (!mRefreshEnabled) {
      mRefreshEnabled = refreshEnabled;
    }
  }
  
  if (mHasPendingUpdates) {
    ProcessPendingUpdates(mRootView, PR_TRUE);
    mHasPendingUpdates = PR_FALSE;
  }
}

void
nsViewManager::ProcessInvalidateEvent()
{
  NS_ASSERTION(IsRootVM(),
               "Incorrectly targeted invalidate event");
  
  
  PRBool processEvent = (mUpdateBatchCnt == 0);
  if (processEvent) {
    FlushPendingInvalidates();
  }
  mInvalidateEvent.Forget();
  if (!processEvent) {
    
    PostInvalidateEvent();
  }
}

NS_IMETHODIMP
nsViewManager::SetDefaultBackgroundColor(nscolor aColor)
{
  mDefaultBackgroundColor = aColor;
  return NS_OK;
}

NS_IMETHODIMP
nsViewManager::GetDefaultBackgroundColor(nscolor* aColor)
{
  *aColor = mDefaultBackgroundColor;
  return NS_OK;
}


NS_IMETHODIMP
nsViewManager::GetLastUserEventTime(PRUint32& aTime)
{
  aTime = gLastUserEventTime;
  return NS_OK;
}

class nsSynthMouseMoveEvent : public nsViewManagerEvent {
public:
  nsSynthMouseMoveEvent(nsViewManager *aViewManager,
                        PRBool aFromScroll)
    : nsViewManagerEvent(aViewManager),
      mFromScroll(aFromScroll) {
  }

  NS_IMETHOD Run() {
    if (mViewManager)
      mViewManager->ProcessSynthMouseMoveEvent(mFromScroll);
    return NS_OK;
  }

private:
  PRBool mFromScroll;
};

NS_IMETHODIMP
nsViewManager::SynthesizeMouseMove(PRBool aFromScroll)
{
  if (!IsRootVM())
    return RootViewManager()->SynthesizeMouseMove(aFromScroll);

  if (mMouseLocation == nsPoint(NSCOORD_NONE, NSCOORD_NONE))
    return NS_OK;

  if (!mSynthMouseMoveEvent.IsPending()) {
    nsRefPtr<nsViewManagerEvent> ev =
        new nsSynthMouseMoveEvent(this, aFromScroll);

    if (NS_FAILED(NS_DispatchToCurrentThread(ev))) {
      NS_WARNING("failed to dispatch nsSynthMouseMoveEvent");
      return NS_ERROR_UNEXPECTED;
    }

    mSynthMouseMoveEvent = ev;
  }

  return NS_OK;
}













static nsView* FindFloatingViewContaining(nsView* aView, nsPoint aPt)
{
  for (nsView* v = aView->GetFirstChild(); v; v = v->GetNextSibling()) {
    nsView* r = FindFloatingViewContaining(v, aPt - v->GetOffsetTo(aView));
    if (r)
      return r;
  }

  if (aView->GetFloating() && aView->HasWidget() &&
      aView->GetDimensions().Contains(aPt) && IsViewVisible(aView))
    return aView;
    
  return nsnull;
}

void
nsViewManager::ProcessSynthMouseMoveEvent(PRBool aFromScroll)
{
  
  
  if (aFromScroll)
    mSynthMouseMoveEvent.Forget();

  NS_ASSERTION(IsRootVM(), "Only the root view manager should be here");

  if (mMouseLocation == nsPoint(NSCOORD_NONE, NSCOORD_NONE) || !mRootView) {
    mSynthMouseMoveEvent.Forget();
    return;
  }

  
  
  nsCOMPtr<nsIViewManager> kungFuDeathGrip(this);
  
#ifdef DEBUG_MOUSE_LOCATION
  printf("[vm=%p]synthesizing mouse move to (%d,%d)\n",
         this, mMouseLocation.x, mMouseLocation.y);
#endif
                                                       
  nsPoint pt = mMouseLocation;
  PRInt32 p2a = mContext->AppUnitsPerDevPixel();
  pt.x = NSIntPixelsToAppUnits(mMouseLocation.x, p2a);
  pt.y = NSIntPixelsToAppUnits(mMouseLocation.y, p2a);
  
  
  nsView* view = FindFloatingViewContaining(mRootView, pt);
  nsPoint offset(0, 0);
  if (!view) {
    view = mRootView;
  } else {
    offset = view->GetOffsetTo(mRootView);
    offset.x = NSAppUnitsToIntPixels(offset.x, p2a);
    offset.y = NSAppUnitsToIntPixels(offset.y, p2a);
  }
  nsMouseEvent event(PR_TRUE, NS_MOUSE_MOVE, view->GetWidget(),
                     nsMouseEvent::eSynthesized);
  event.refPoint = mMouseLocation - offset;
  event.time = PR_IntervalNow();
  

  nsEventStatus status;
  view->GetViewManager()->DispatchEvent(&event, &status);

  if (!aFromScroll)
    mSynthMouseMoveEvent.Forget();
}

void
nsViewManager::InvalidateHierarchy()
{
  if (mRootView) {
    if (!IsRootVM()) {
      NS_RELEASE(mRootViewManager);
    }
    nsView *parent = mRootView->GetParent();
    if (parent) {
      mRootViewManager = parent->GetViewManager()->RootViewManager();
      NS_ADDREF(mRootViewManager);
      NS_ASSERTION(mRootViewManager != this,
                   "Root view had a parent, but it has the same view manager");
    } else {
      mRootViewManager = this;
    }
  }
}
