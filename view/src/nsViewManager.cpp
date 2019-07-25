








































#define PL_ARENA_CONST_ALIGN_MASK (sizeof(void*)-1)
#include "plarena.h"

#include "nsAutoPtr.h"
#include "nsViewManager.h"
#include "nsGfxCIID.h"
#include "nsView.h"
#include "nsISupportsArray.h"
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsGUIEvent.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsRegion.h"
#include "nsInt64.h"
#include "nsHashtable.h"
#include "nsCOMArray.h"
#include "nsThreadUtils.h"
#include "nsContentUtils.h"
#include "nsIPluginWidget.h"

static NS_DEFINE_IID(kRegionCID, NS_REGION_CID);



















#define NSCOORD_NONE      PR_INT32_MIN



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
  if (!aView->IsEffectivelyVisible())
    return PR_FALSE;

  
  
  
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
 
  gViewManagers->AppendElement(this);

  ++mVMCount;

  
  
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
  }

  mObserver = nsnull;
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

  return NS_OK;
}

NS_IMETHODIMP_(nsIView *)
nsViewManager::CreateView(const nsRect& aBounds,
                          const nsIView* aParent,
                          nsViewVisibility aVisibilityFlag)
{
  nsView *v = new nsView(this, aVisibilityFlag);
  if (v) {
    v->SetParent(static_cast<nsView*>(const_cast<nsIView*>(aParent)));
    v->SetPosition(aBounds.x, aBounds.y);
    nsRect dim(0, 0, aBounds.width, aBounds.height);
    v->SetDimensions(dim, PR_FALSE);
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
  nsView* view = static_cast<nsView*>(aView);

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

void nsViewManager::DoSetWindowDimensions(nscoord aWidth, nscoord aHeight)
{
  nsRect oldDim;
  nsRect newDim(0, 0, aWidth, aHeight);
  mRootView->GetDimensions(oldDim);
  
  if (!oldDim.IsExactEqual(newDim)) {
    
    mRootView->SetDimensions(newDim, PR_TRUE, PR_FALSE);
    if (mObserver)
      mObserver->ResizeReflow(mRootView, aWidth, aHeight);
  }
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

NS_IMETHODIMP nsViewManager::FlushDelayedResize()
{
  if (mDelayedResize != nsSize(NSCOORD_NONE, NSCOORD_NONE)) {
    DoSetWindowDimensions(mDelayedResize.width, mDelayedResize.height);
    mDelayedResize.SizeTo(NSCOORD_NONE, NSCOORD_NONE);
  }
  return NS_OK;
}

static nsRegion ConvertDeviceRegionToAppRegion(const nsIntRegion& aIn,
                                               nsIDeviceContext* aContext)
{
  PRInt32 p2a = aContext->AppUnitsPerDevPixel();

  nsRegion out;
  nsIntRegionRectIterator iter(aIn);
  for (;;) {
    const nsIntRect* r = iter.Next();
    if (!r)
      break;
    out.Or(out, r->ToAppUnits(p2a));
  }
  return out;
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






void nsViewManager::Refresh(nsView *aView, nsIWidget *aWidget,
                            const nsIntRegion& aRegion,
                            PRUint32 aUpdateFlags)
{
  if (! IsRefreshEnabled())
    return;

  nsRect viewRect;
  aView->GetDimensions(viewRect);
  nsPoint vtowoffset = aView->ViewToWidgetOffset();

  
  nsRegion damageRegion = ConvertDeviceRegionToAppRegion(aRegion, mContext);
  
  damageRegion.MoveBy(viewRect.TopLeft() - vtowoffset);

  if (damageRegion.IsEmpty()) {
#ifdef DEBUG_roc
    nsRect damageRect = damageRegion.GetBounds();
    printf("XXX Damage rectangle (%d,%d,%d,%d) does not intersect the widget's view (%d,%d,%d,%d)!\n",
           damageRect.x, damageRect.y, damageRect.width, damageRect.height,
           viewRect.x, viewRect.y, viewRect.width, viewRect.height);
#endif
    return;
  }

  NS_ASSERTION(!IsPainting(), "recursive painting not permitted");
  if (IsPainting()) {
    RootViewManager()->mRecursiveRefreshPending = PR_TRUE;
    return;
  }  

  {
    nsAutoScriptBlocker scriptBlocker;
    SetPainting(PR_TRUE);

    RenderViews(aView, aWidget, damageRegion);

    SetPainting(PR_FALSE);
  }

  if (RootViewManager()->mRecursiveRefreshPending) {
    
    
    RootViewManager()->mRecursiveRefreshPending = PR_FALSE;
    UpdateAllViews(aUpdateFlags);
  }
}


void nsViewManager::RenderViews(nsView *aView, nsIWidget *aWidget,
                                const nsRegion& aRegion)
{
  nsView* displayRoot = GetDisplayRootFor(aView);
  
  
  nsViewManager* displayRootVM = displayRoot->GetViewManager();
  if (displayRootVM && displayRootVM != this) {
    displayRootVM->RenderViews(aView, aWidget, aRegion);
    return;
  }

  if (mObserver) {
    mObserver->Paint(displayRoot, aView, aWidget, aRegion, PR_FALSE);
  }
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
    
    
    NS_ASSERTION(IsRefreshEnabled(), "Cannot process pending updates with refresh disabled");
    nsRegion* dirtyRegion = aView->GetDirtyRegion();
    if (dirtyRegion) {
      nsView* nearestViewWithWidget = aView;
      while (!nearestViewWithWidget->HasWidget() &&
             nearestViewWithWidget->GetParent()) {
        nearestViewWithWidget =
          static_cast<nsView*>(nearestViewWithWidget->GetParent());
      }
      nsRegion r = *dirtyRegion;
      r.MoveBy(aView->GetOffsetTo(nearestViewWithWidget));
      nsViewManager* widgetVM = nearestViewWithWidget->GetViewManager();
      widgetVM->
        UpdateWidgetArea(nearestViewWithWidget,
                         nearestViewWithWidget->GetWidget(), r, nsnull);
      dirtyRegion->SetEmpty();
    }
  }
}

NS_IMETHODIMP nsViewManager::Composite()
{
  if (!IsRootVM()) {
    return RootViewManager()->Composite();
  }
#ifndef MOZ_GFX_OPTIMIZE_MOBILE  
  if (UpdateCount() > 0)
#endif
    {
      ForceUpdate();
      ClearUpdateCount();
    }

  return NS_OK;
}

NS_IMETHODIMP nsViewManager::UpdateView(nsIView *aView, PRUint32 aUpdateFlags)
{
  
  nsView* view = static_cast<nsView*>(aView);

  nsRect bounds = view->GetBounds();
  view->ConvertFromParentCoords(&bounds.x, &bounds.y);
  return UpdateView(view, bounds, aUpdateFlags);
}

nsresult
nsViewManager::WillBitBlit(nsIView* aView, const nsRect& aRect,
                           nsPoint aCopyDelta)
{
  if (!IsRootVM()) {
    RootViewManager()->WillBitBlit(aView, aRect, aCopyDelta);
    return NS_OK;
  }

  
  NS_PRECONDITION(aView &&
                  (aView == mRootView || aView->GetFloating()),
                  "Must have a display root view");

  ++mScrollCnt;
  
  nsView* v = static_cast<nsView*>(aView);
  if (v->HasNonEmptyDirtyRegion()) {
    nsRegion* dirty = v->GetDirtyRegion();
    nsRegion intersection = *dirty;
    intersection.MoveBy(-aCopyDelta);
    intersection.And(intersection, aRect);
    if (!intersection.IsEmpty()) {
      dirty->Or(*dirty, intersection);
      
      dirty->SimplifyOutward(20);
    }
  }
  return NS_OK;
}


void
nsViewManager::UpdateViewAfterScroll(nsIView *aView,
                                     const nsRegion& aUpdateRegion)
{
  NS_ASSERTION(RootViewManager()->mScrollCnt > 0,
               "Someone forgot to call WillBitBlit()");
  
  

  nsView* view = static_cast<nsView*>(aView);
  nsView* displayRoot = GetDisplayRootFor(view);
  nsPoint offset = view->GetOffsetTo(displayRoot);
  nsRegion update(aUpdateRegion);
  update.MoveBy(offset);

  nsViewManager* displayRootVM = displayRoot->GetViewManager();
  displayRootVM->UpdateWidgetArea(displayRoot, displayRoot->GetWidget(),
                                  update, nsnull);
  

  Composite();
  --RootViewManager()->mScrollCnt;
}

static PRBool
IsWidgetDrawnByPlugin(nsIWidget* aWidget, nsIView* aView)
{
  if (aView->GetWidget() == aWidget)
    return PR_FALSE;
  nsCOMPtr<nsIPluginWidget> pw = do_QueryInterface(aWidget);
  if (pw) {
    
    
    return PR_FALSE;
  }
  return PR_TRUE;
}










void
nsViewManager::UpdateWidgetArea(nsView *aWidgetView, nsIWidget* aWidget,
                                const nsRegion &aDamagedRegion,
                                nsView* aIgnoreWidgetView)
{
#if 0
  nsRect dbgBounds = aDamagedRegion.GetBounds();
  printf("UpdateWidgetArea view:%X (%d) widget:%X region: %d, %d, %d, %d\n",
    aWidgetView, aWidgetView->IsAttachedToTopLevel(),
    aWidget, dbgBounds.x, dbgBounds.y, dbgBounds.width, dbgBounds.height);
#endif

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

  
  if (aWidget) {
    PRBool visible;
    aWidget->IsVisible(visible);
    if (!visible)
      return;
  }

  if (aWidgetView == aIgnoreWidgetView) {
    
    return;
  }

  if (!aWidget) {
    
    
    
    return;
  }

  
  
  
  nsRegion children;
  if (aWidget->GetTransparencyMode() != eTransparencyTransparent) {
    for (nsIWidget* childWidget = aWidget->GetFirstChild();
         childWidget;
         childWidget = childWidget->GetNextSibling()) {
      nsView* view = nsView::GetViewFor(childWidget);
      NS_ASSERTION(view != aWidgetView, "will recur infinitely");
      PRBool visible;
      childWidget->IsVisible(visible);
      if (view && visible && !IsWidgetDrawnByPlugin(childWidget, view)) {
        
        
        nsViewManager* viewManager = view->GetViewManager();
        if (viewManager->RootViewManager() == RootViewManager()) {
          
          nsRegion damage = intersection;

          nsPoint offset = view->GetOffsetTo(aWidgetView);
          damage.MoveBy(-offset);

          
          viewManager->
            UpdateWidgetArea(view, childWidget, damage, aIgnoreWidgetView);

          
          nsIntRect bounds;
          childWidget->GetBounds(bounds);

          nsTArray<nsIntRect> clipRects;
          childWidget->GetWindowClipRegion(&clipRects);
          for (PRUint32 i = 0; i < clipRects.Length(); ++i) {
            nsRect rr = (clipRects[i] + bounds.TopLeft()).
              ToAppUnits(AppUnitsPerDevPixel());
            children.Or(children, rr - aWidgetView->ViewToWidgetOffset()); 
            children.SimplifyInward(20);
          }
        }
      }
    }
  }

  nsRegion leftOver;
  leftOver.Sub(intersection, children);

  if (!leftOver.IsEmpty()) {
    NS_ASSERTION(IsRefreshEnabled(), "Can only get here with refresh enabled, I hope");

    const nsRect* r;
    for (nsRegionRectIterator iter(leftOver); (r = iter.Next());) {
      nsIntRect bounds = ViewToWidget(aWidgetView, *r);
      aWidget->Invalidate(bounds, PR_FALSE);
    }
  }
}

NS_IMETHODIMP nsViewManager::UpdateView(nsIView *aView, const nsRect &aRect, PRUint32 aUpdateFlags)
{
  NS_PRECONDITION(nsnull != aView, "null view");

  nsView* view = static_cast<nsView*>(aView);

  nsRect damagedRect(aRect);
  if (damagedRect.IsEmpty()) {
    return NS_OK;
  }

  nsView* displayRoot = GetDisplayRootFor(view);
  nsViewManager* displayRootVM = displayRoot->GetViewManager();
  
  
  
  damagedRect.MoveBy(view->GetOffsetTo(displayRoot));
  displayRootVM->UpdateWidgetArea(displayRoot, displayRoot->GetWidget(),
                                  nsRegion(damagedRect), nsnull);

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

NS_IMETHODIMP nsViewManager::DispatchEvent(nsGUIEvent *aEvent,
                                           nsIView* aView, nsEventStatus *aStatus)
{
  *aStatus = nsEventStatus_eIgnore;

  switch(aEvent->message)
    {
    case NS_SIZE:
      {
        if (aView)
          {
            
            nscoord width = ((nsSizeEvent*)aEvent)->windowSize->width;
            nscoord height = ((nsSizeEvent*)aEvent)->windowSize->height;

            
            

            if (aView == mRootView)
              {
                PRInt32 p2a = AppUnitsPerDevPixel();
                SetWindowDimensions(NSIntPixelsToAppUnits(width, p2a),
                                    NSIntPixelsToAppUnits(height, p2a));
                *aStatus = nsEventStatus_eConsumeNoDefault;
              }
          }

        break;
      }

    case NS_WILL_PAINT:
    case NS_PAINT:
      {
        nsPaintEvent *event = static_cast<nsPaintEvent*>(aEvent);

        if (!aView || !mContext)
          break;

        *aStatus = nsEventStatus_eConsumeNoDefault;

        if (aEvent->message == NS_PAINT && event->region.IsEmpty())
          break;

        
        

        
        if (IsRefreshEnabled()) {
          nsRefPtr<nsViewManager> rootVM = RootViewManager();

          
          
          PRBool didResize = PR_FALSE;
          if (rootVM->mScrollCnt == 0) {
            for (nsViewManager *vm = this; vm;
                 vm = vm->mRootView->GetParent()
                        ? vm->mRootView->GetParent()->GetViewManager()
                        : nsnull) {
              if (vm->mDelayedResize != nsSize(NSCOORD_NONE, NSCOORD_NONE) &&
                  IsViewVisible(vm->mRootView)) {
                vm->FlushDelayedResize();

                
                vm->UpdateView(vm->mRootView, NS_VMREFRESH_NO_SYNC);
                didResize = PR_TRUE;

                
                
                
                *aStatus = nsEventStatus_eIgnore;
              }
            }
          }

          if (!didResize) {
            

            
            

            nsCOMPtr<nsIWidget> widget;
            rootVM->GetRootWidget(getter_AddRefs(widget));
            PRBool transparentWindow = PR_FALSE;
            if (widget)
                transparentWindow = widget->GetTransparencyMode() == eTransparencyTransparent;

            nsView* view = static_cast<nsView*>(aView);
            if (rootVM->mScrollCnt == 0 && !transparentWindow) {
              nsIViewObserver* observer = GetViewObserver();
              if (observer) {
                
                
                
                
                
                
                
                
                
                
                
                UpdateViewBatch batch(this);
                rootVM->CallWillPaintOnObservers();
                batch.EndUpdateViewBatch(NS_VMREFRESH_NO_SYNC);

                
                
                view = nsView::GetViewFor(aEvent->widget);
              }
            }
            
            
            if (rootVM->mHasPendingUpdates) {
              rootVM->ProcessPendingUpdates(mRootView, PR_FALSE);
            }
            
            if (view && aEvent->message == NS_PAINT) {
              Refresh(view, event->widget,
                      event->region, NS_VMREFRESH_DOUBLE_BUFFER);
            }
          }
        } else if (aEvent->message == NS_PAINT) {
          
          
          
          nsRegion rgn = ConvertDeviceRegionToAppRegion(event->region, mContext);
          mObserver->Paint(aView, aView, event->widget, rgn, PR_TRUE);

          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          

          UpdateView(aView, rgn.GetBounds(), NS_VMREFRESH_NO_SYNC);
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
        
        
        
        nsCOMPtr<nsIViewObserver> obs = GetViewObserver();
        if (obs) {
          obs->HandleEvent(aView, aEvent, aStatus);
        }
      }
      break; 

    default:
      {
        if ((NS_IS_MOUSE_EVENT(aEvent) &&
             
             static_cast<nsMouseEvent*>(aEvent)->reason ==
               nsMouseEvent::eReal &&
             
             
             
             aEvent->message != NS_MOUSE_EXIT &&
             aEvent->message != NS_MOUSE_ENTER) ||
            NS_IS_KEY_EVENT(aEvent) ||
            NS_IS_IME_EVENT(aEvent) ||
            NS_IS_PLUGIN_EVENT(aEvent) ||
            NS_IS_NON_RETARGETED_PLUGIN_EVENT(aEvent)) {
          gLastUserEventTime = PR_IntervalToMicroseconds(PR_IntervalNow());
        }

        if (aEvent->message == NS_DEACTIVATE) {
          
          
          nsIViewObserver* viewObserver = GetViewObserver();
          if (viewObserver) {
            viewObserver->ClearMouseCapture(nsnull);
          }
        }

        
        nsView* baseView = static_cast<nsView*>(aView);
        nsView* view = baseView;

        if (NS_IsEventUsingCoordinates(aEvent)) {
          
          
          view = GetDisplayRootFor(baseView);
        }

        if (nsnull != view) {
          PRInt32 p2a = AppUnitsPerDevPixel();

          if ((aEvent->message == NS_MOUSE_MOVE &&
               static_cast<nsMouseEvent*>(aEvent)->reason ==
                 nsMouseEvent::eReal) ||
              aEvent->message == NS_MOUSE_ENTER ||
              aEvent->message == NS_MOUSE_BUTTON_DOWN ||
              aEvent->message == NS_MOUSE_BUTTON_UP) {
            
            
            nsPoint rootOffset = baseView->GetDimensions().TopLeft();
            rootOffset += baseView->GetOffsetTo(RootViewManager()->mRootView);
            RootViewManager()->mMouseLocation = aEvent->refPoint +
                nsIntPoint(NSAppUnitsToIntPixels(rootOffset.x, p2a),
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
            
            
            
            
            
            
            
            RootViewManager()->mMouseLocation = nsIntPoint(NSCOORD_NONE, NSCOORD_NONE);
#ifdef DEBUG_MOUSE_LOCATION
            printf("[vm=%p]got mouse exit for %p\n",
                   this, aEvent->widget);
            printf("[vm=%p]clearing mouse location\n",
                   this);
#endif
          }

          *aStatus = HandleEvent(view, aEvent);
        }
    
        break;
      }
    }

  return NS_OK;
}

nsEventStatus nsViewManager::HandleEvent(nsView* aView, nsGUIEvent* aEvent)
{
#if 0
  printf(" %d %d %d %d (%d,%d) \n", this, event->widget, event->widgetSupports, 
         event->message, event->point.x, event->point.y);
#endif
  
  
  
  nsCOMPtr<nsIViewObserver> obs = aView->GetViewManager()->GetViewObserver();
  nsEventStatus status = nsEventStatus_eIgnore;
  if (obs) {
     obs->HandleEvent(aView, aEvent, &status);
  }

  return status;
}



void nsViewManager::ReparentChildWidgets(nsIView* aView, nsIWidget *aNewWidget)
{
  if (aView->HasWidget()) {
    
    
    
    
    nsIWidget* widget = aView->GetWidget();
    nsIWidget* parentWidget = widget->GetParent();
    
    if (parentWidget && parentWidget != aNewWidget) {
#ifdef DEBUG
      nsresult rv =
#endif
        widget->SetParent(aNewWidget);
      NS_ASSERTION(NS_SUCCEEDED(rv), "SetParent failed!");
    }
    return;
  }

  
  

  nsView* view = static_cast<nsView*>(aView);
  for (nsView *kid = view->GetFirstChild(); kid; kid = kid->GetNextSibling()) {
    ReparentChildWidgets(kid, aNewWidget);
  }
}



void nsViewManager::ReparentWidgets(nsIView* aView, nsIView *aParent)
{
  NS_PRECONDITION(aParent, "Must have a parent");
  NS_PRECONDITION(aView, "Must have a view");
  
  
  
  
  
  
  
  
  nsView* view = static_cast<nsView*>(aView);
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
  nsView* parent = static_cast<nsView*>(aParent);
  nsView* child = static_cast<nsView*>(aChild);
  nsView* sibling = static_cast<nsView*>(aSibling);
  
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

NS_IMETHODIMP nsViewManager::InsertChild(nsIView *aParent, nsIView *aChild, PRInt32 aZIndex)
{
  
  
  SetViewZIndex(aChild, PR_FALSE, aZIndex, PR_FALSE);
  return InsertChild(aParent, aChild, nsnull, PR_TRUE);
}

NS_IMETHODIMP nsViewManager::RemoveChild(nsIView *aChild)
{
  nsView* child = static_cast<nsView*>(aChild);
  NS_ENSURE_ARG_POINTER(child);

  nsView* parent = child->GetParent();

  if (nsnull != parent)
    {
      UpdateView(child, NS_VMREFRESH_NO_SYNC);
      parent->RemoveChild(child);
    }

  return NS_OK;
}

NS_IMETHODIMP nsViewManager::MoveViewTo(nsIView *aView, nscoord aX, nscoord aY)
{
  nsView* view = static_cast<nsView*>(aView);
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
  nsView* view = static_cast<nsView*>(aView);
  nsRect oldDimensions;

  view->GetDimensions(oldDimensions);
  if (!oldDimensions.IsExactEqual(aRect)) {
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

NS_IMETHODIMP nsViewManager::SetViewFloating(nsIView *aView, PRBool aFloating)
{
  nsView* view = static_cast<nsView*>(aView);

  NS_ASSERTION(!(nsnull == view), "no view");

  view->SetFloating(aFloating);

  return NS_OK;
}

NS_IMETHODIMP nsViewManager::SetViewVisibility(nsIView *aView, nsViewVisibility aVisible)
{
  nsView* view = static_cast<nsView*>(aView);

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
  }
  return NS_OK;
}

void nsViewManager::UpdateWidgetsForView(nsView* aView)
{
  NS_PRECONDITION(aView, "Must have view!");

  
  if (!IsRefreshEnabled())
    return;  

  nsWeakView parentWeakView = aView;
  if (aView->HasWidget()) {
    aView->GetWidget()->Update();  
    if (!parentWeakView.IsAlive()) {
      return;
    }
  }

  nsView* childView = aView->GetFirstChild();
  while (childView) {
    nsWeakView childWeakView = childView;
    UpdateWidgetsForView(childView);
    if (NS_LIKELY(childWeakView.IsAlive())) {
      childView = childView->GetNextSibling();
    }
    else {
      
      
      childView = parentWeakView.IsAlive() ? aView->GetFirstChild() : nsnull;
    }
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
  nsView* view = static_cast<nsView*>(aView);
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

void nsViewManager::TriggerRefresh(PRUint32 aUpdateFlags)
{
  if (!IsRootVM()) {
    RootViewManager()->TriggerRefresh(aUpdateFlags);
    return;
  }
  
  if (mUpdateBatchCnt > 0)
    return;

  
  
  
  
  
  if (aUpdateFlags & NS_VMREFRESH_IMMEDIATE) {
    FlushPendingInvalidates();
    Composite();
  } else if (!mHasPendingUpdates) {
    
  } else if (aUpdateFlags & NS_VMREFRESH_DEFERRED) {
    PostInvalidateEvent();
  } else { 
    FlushPendingInvalidates();
  }
}

nsIViewManager* nsViewManager::BeginUpdateViewBatch(void)
{
  if (!IsRootVM()) {
    return RootViewManager()->BeginUpdateViewBatch();
  }
  
  if (mUpdateBatchCnt == 0) {
    mUpdateBatchFlags = 0;
  }

  ++mUpdateBatchCnt;

  return this;
}

NS_IMETHODIMP nsViewManager::EndUpdateViewBatch(PRUint32 aUpdateFlags)
{
  NS_ASSERTION(IsRootVM(), "Should only be called on root");
  
  --mUpdateBatchCnt;

  NS_ASSERTION(mUpdateBatchCnt >= 0, "Invalid batch count!");

  if (mUpdateBatchCnt < 0)
    {
      mUpdateBatchCnt = 0;
      return NS_ERROR_FAILURE;
    }

  mUpdateBatchFlags |= aUpdateFlags;
  if (mUpdateBatchCnt == 0) {
    TriggerRefresh(mUpdateBatchFlags);
  }

  return NS_OK;
}

NS_IMETHODIMP nsViewManager::GetRootWidget(nsIWidget **aWidget)
{
  if (!mRootView) {
    *aWidget = nsnull;
    return NS_OK;
  }
  if (mRootView->HasWidget()) {
    *aWidget = mRootView->GetWidget();
    NS_ADDREF(*aWidget);
    return NS_OK;
  }
  if (mRootView->GetParent())
    return mRootView->GetParent()->GetViewManager()->GetRootWidget(aWidget);
  *aWidget = nsnull;
  return NS_OK;
}

NS_IMETHODIMP nsViewManager::ForceUpdate()
{
  if (!IsRootVM()) {
    return RootViewManager()->ForceUpdate();
  }

  
  if (mRootView) {
    UpdateWidgetsForView(mRootView);
  }
  
  return NS_OK;
}

nsIntRect nsViewManager::ViewToWidget(nsView *aView, const nsRect &aRect) const
{
  NS_ASSERTION(aView->GetViewManager() == this, "wrong view manager");

  
  nsRect bounds = aView->GetDimensions();
  nsRect rect;
  rect.IntersectRect(aRect, bounds);

  
  rect += aView->ViewToWidgetOffset();

  
  return rect.ToOutsidePixels(AppUnitsPerDevPixel());
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
    
    
    
    
    
    ++mUpdateBatchCnt;
    CallWillPaintOnObservers();
    --mUpdateBatchCnt;
  }
  
  if (mHasPendingUpdates) {
    ProcessPendingUpdates(mRootView, PR_TRUE);
    mHasPendingUpdates = PR_FALSE;
  }
}

void
nsViewManager::CallWillPaintOnObservers()
{
  NS_PRECONDITION(IsRootVM(), "Must be root VM for this to be called!\n");
  NS_PRECONDITION(mUpdateBatchCnt > 0, "Must be in an update batch!");

#ifdef DEBUG
  PRInt32 savedUpdateBatchCnt = mUpdateBatchCnt;
#endif
  PRInt32 index;
  for (index = 0; index < mVMCount; index++) {
    nsViewManager* vm = (nsViewManager*)gViewManagers->ElementAt(index);
    if (vm->RootViewManager() == this) {
      
      nsCOMPtr<nsIViewObserver> obs = vm->GetViewObserver();
      if (obs) {
        obs->WillPaint();
        NS_ASSERTION(mUpdateBatchCnt == savedUpdateBatchCnt,
                     "Observer did not end view batch?");
      }
    }
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

  if (mMouseLocation == nsIntPoint(NSCOORD_NONE, NSCOORD_NONE))
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
  if (aView->GetVisibility() == nsViewVisibility_kHide)
    
    return nsnull;

  for (nsView* v = aView->GetFirstChild(); v; v = v->GetNextSibling()) {
    nsView* r = FindFloatingViewContaining(v, aPt - v->GetOffsetTo(aView));
    if (r)
      return r;
  }

  if (aView->GetFloating() && aView->HasWidget() &&
      aView->GetDimensions().Contains(aPt))
    return aView;
    
  return nsnull;
}










static nsView* FindViewContaining(nsView* aView, nsPoint aPt)
{
  if (!aView->GetDimensions().Contains(aPt) ||
      aView->GetVisibility() == nsViewVisibility_kHide) {
    return nsnull;
  }

  for (nsView* v = aView->GetFirstChild(); v; v = v->GetNextSibling()) {
    nsView* r = FindViewContaining(v, aPt - v->GetOffsetTo(aView));
    if (r)
      return r;
  }

  return aView;
}

void
nsViewManager::ProcessSynthMouseMoveEvent(PRBool aFromScroll)
{
  
  
  if (aFromScroll)
    mSynthMouseMoveEvent.Forget();

  NS_ASSERTION(IsRootVM(), "Only the root view manager should be here");

  if (mMouseLocation == nsIntPoint(NSCOORD_NONE, NSCOORD_NONE) || !mRootView) {
    mSynthMouseMoveEvent.Forget();
    return;
  }

  
  
  nsCOMPtr<nsIViewManager> kungFuDeathGrip(this);
  
#ifdef DEBUG_MOUSE_LOCATION
  printf("[vm=%p]synthesizing mouse move to (%d,%d)\n",
         this, mMouseLocation.x, mMouseLocation.y);
#endif
                                                       
  nsPoint pt;
  PRInt32 p2a = AppUnitsPerDevPixel();
  pt.x = NSIntPixelsToAppUnits(mMouseLocation.x, p2a);
  pt.y = NSIntPixelsToAppUnits(mMouseLocation.y, p2a);
  
  
  nsView* view = FindFloatingViewContaining(mRootView, pt);
  nsIntPoint offset(0, 0);
  nsViewManager *pointVM;
  if (!view) {
    view = mRootView;
    nsView *pointView = FindViewContaining(mRootView, pt);
    
    pointVM = (pointView ? pointView : view)->GetViewManager();
  } else {
    nsPoint viewoffset = view->GetOffsetTo(mRootView);
    offset.x = NSAppUnitsToIntPixels(viewoffset.x, p2a);
    offset.y = NSAppUnitsToIntPixels(viewoffset.y, p2a);
    pointVM = view->GetViewManager();
  }
  nsMouseEvent event(PR_TRUE, NS_MOUSE_MOVE, view->GetWidget(),
                     nsMouseEvent::eSynthesized);
  event.refPoint = mMouseLocation - offset;
  event.time = PR_IntervalNow();
  

  nsCOMPtr<nsIViewObserver> observer = pointVM->GetViewObserver();
  if (observer) {
    observer->DispatchSynthMouseMove(&event, !aFromScroll);
  }

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
