




#define PL_ARENA_CONST_ALIGN_MASK (sizeof(void*)-1)
#include "plarena.h"

#include "nsAutoPtr.h"
#include "nsViewManager.h"
#include "nsGfxCIID.h"
#include "nsView.h"
#include "nsISupportsArray.h"
#include "nsCOMPtr.h"
#include "nsGUIEvent.h"
#include "nsRegion.h"
#include "nsHashtable.h"
#include "nsCOMArray.h"
#include "nsThreadUtils.h"
#include "nsContentUtils.h"
#include "nsIPluginWidget.h"
#include "nsXULPopupManager.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsEventStateManager.h"
#include "mozilla/StartupTimeline.h"
#include "sampler.h"

















#define NSCOORD_NONE      PR_INT32_MIN

#undef DEBUG_MOUSE_LOCATION

PRInt32 nsViewManager::mVMCount = 0;


nsVoidArray* nsViewManager::gViewManagers = nsnull;
PRUint32 nsViewManager::gLastUserEventTime = 0;

nsViewManager::nsViewManager()
  : mDelayedResize(NSCOORD_NONE, NSCOORD_NONE)
  , mRootViewManager(this)
{
  if (gViewManagers == nsnull) {
    NS_ASSERTION(mVMCount == 0, "View Manager count is incorrect");
    
    gViewManagers = new nsVoidArray;
  }
 
  gViewManagers->AppendElement(this);

  ++mVMCount;

  
  
  mHasPendingUpdates = false;
  mHasPendingWidgetGeometryChanges = false;
  mRecursiveRefreshPending = false;
}

nsViewManager::~nsViewManager()
{
  if (mRootView) {
    
    mRootView->Destroy();
    mRootView = nsnull;
  }

  if (!IsRootVM()) {
    
    NS_RELEASE(mRootViewManager);
  }

  NS_ASSERTION((mVMCount > 0), "underflow of viewmanagers");
  --mVMCount;

#ifdef DEBUG
  bool removed =
#endif
    gViewManagers->RemoveElement(this);
  NS_ASSERTION(removed, "Viewmanager instance not was not in the global list of viewmanagers");

  if (0 == mVMCount) {
    
    
   
    NS_ASSERTION(gViewManagers != nsnull, "About to delete null gViewManagers");
    delete gViewManagers;
    gViewManagers = nsnull;
  }

  mPresShell = nsnull;
}

NS_IMPL_ISUPPORTS1(nsViewManager, nsIViewManager)



NS_IMETHODIMP nsViewManager::Init(nsDeviceContext* aContext)
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
    v->SetDimensions(dim, false);
  }
  return v;
}

NS_IMETHODIMP_(nsIView*)
nsViewManager::GetRootView()
{
  return mRootView;
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

    mRootView->SetZIndex(false, 0, false);
  }
  

  return NS_OK;
}

NS_IMETHODIMP nsViewManager::GetWindowDimensions(nscoord *aWidth, nscoord *aHeight)
{
  if (nsnull != mRootView) {
    if (mDelayedResize == nsSize(NSCOORD_NONE, NSCOORD_NONE)) {
      nsRect dim = mRootView->GetDimensions();
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
  nsRect oldDim = mRootView->GetDimensions();
  nsRect newDim(0, 0, aWidth, aHeight);
  
  if (!oldDim.IsEqualEdges(newDim)) {
    
    mRootView->SetDimensions(newDim, true, false);
    if (mPresShell)
      mPresShell->ResizeReflow(aWidth, aHeight);
  }
}

NS_IMETHODIMP nsViewManager::SetWindowDimensions(nscoord aWidth, nscoord aHeight)
{
  if (mRootView) {
    if (mRootView->IsEffectivelyVisible() && mPresShell && mPresShell->IsVisible()) {
      if (mDelayedResize != nsSize(NSCOORD_NONE, NSCOORD_NONE) &&
          mDelayedResize != nsSize(aWidth, aHeight)) {
        
        
        
        
        
        mDelayedResize = nsSize(aWidth, aHeight);
        FlushDelayedResize(false);
      }
      mDelayedResize.SizeTo(NSCOORD_NONE, NSCOORD_NONE);
      DoSetWindowDimensions(aWidth, aHeight);
    } else {
      mDelayedResize.SizeTo(aWidth, aHeight);
      if (mPresShell && mPresShell->GetDocument()) {
        mPresShell->GetDocument()->SetNeedStyleFlush();
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP nsViewManager::FlushDelayedResize(bool aDoReflow)
{
  if (mDelayedResize != nsSize(NSCOORD_NONE, NSCOORD_NONE)) {
    if (aDoReflow) {
      DoSetWindowDimensions(mDelayedResize.width, mDelayedResize.height);
      mDelayedResize.SizeTo(NSCOORD_NONE, NSCOORD_NONE);
    } else if (mPresShell) {
      nsPresContext* presContext = mPresShell->GetPresContext();
      if (presContext) {
        presContext->SetVisibleArea(nsRect(nsPoint(0, 0), mDelayedResize));
      }
    }
  }
  return NS_OK;
}



static nsRegion ConvertRegionBetweenViews(const nsRegion& aIn,
                                          nsView* aFromView,
                                          nsView* aToView)
{
  nsRegion out = aIn;
  out.MoveBy(aFromView->GetOffsetTo(aToView));
  out = out.ConvertAppUnitsRoundOut(
    aFromView->GetViewManager()->AppUnitsPerDevPixel(),
    aToView->GetViewManager()->AppUnitsPerDevPixel());
  return out;
}

nsIView* nsIViewManager::GetDisplayRootFor(nsIView* aView)
{
  nsIView *displayRoot = aView;
  for (;;) {
    nsIView *displayParent = displayRoot->GetParent();
    if (!displayParent)
      return displayRoot;

    if (displayRoot->GetFloating() && !displayParent->GetFloating())
      return displayRoot;

    
    
    
    
    nsIWidget* widget = displayRoot->GetWidget();
    if (widget) {
      nsWindowType type;
      widget->GetWindowType(type);
      if (type == eWindowType_popup) {
        NS_ASSERTION(displayRoot->GetFloating() && displayParent->GetFloating(),
          "this should only happen with floating views that have floating parents");
        return displayRoot;
      }
    }

    displayRoot = displayParent;
  }
}






void nsViewManager::Refresh(nsView *aView, nsIWidget *aWidget,
                            const nsIntRegion& aRegion,
                            bool aWillSendDidPaint)
{
  NS_ASSERTION(aView == nsView::GetViewFor(aWidget), "view widget mismatch");
  NS_ASSERTION(aView->GetViewManager() == this, "wrong view manager");

  
  nsRegion damageRegion = aRegion.ToAppUnits(AppUnitsPerDevPixel());
  
  damageRegion.MoveBy(-aView->ViewToWidgetOffset());

  if (damageRegion.IsEmpty()) {
#ifdef DEBUG_roc
    nsRect viewRect = aView->GetDimensions();
    nsRect damageRect = damageRegion.GetBounds();
    printf("XXX Damage rectangle (%d,%d,%d,%d) does not intersect the widget's view (%d,%d,%d,%d)!\n",
           damageRect.x, damageRect.y, damageRect.width, damageRect.height,
           viewRect.x, viewRect.y, viewRect.width, viewRect.height);
#endif
    return;
  }

  NS_ASSERTION(!IsPainting(), "recursive painting not permitted");
  if (IsPainting()) {
    RootViewManager()->mRecursiveRefreshPending = true;
    return;
  }  

  {
    nsAutoScriptBlocker scriptBlocker;
    SetPainting(true);

    NS_ASSERTION(GetDisplayRootFor(aView) == aView,
                 "Widgets that we paint must all be display roots");

    if (mPresShell) {
      mPresShell->Paint(aView, aWidget, damageRegion, aRegion,
                        aWillSendDidPaint);
      mozilla::StartupTimeline::RecordOnce(mozilla::StartupTimeline::FIRST_PAINT);
    }

    SetPainting(false);
  }

  if (RootViewManager()->mRecursiveRefreshPending) {
    RootViewManager()->mRecursiveRefreshPending = false;
    InvalidateAllViews();
  }
}

void nsViewManager::ProcessPendingUpdatesForView(nsView* aView,
                                                 bool aFlushDirtyRegion)
{
  NS_ASSERTION(IsRootVM(), "Updates will be missed");

  
  if (!aView) {
    return;
  }

  if (aView->HasWidget()) {
    aView->ResetWidgetBounds(false, true);
  }

  
  for (nsView* childView = aView->GetFirstChild(); childView;
       childView = childView->GetNextSibling()) {
    ProcessPendingUpdatesForView(childView, aFlushDirtyRegion);
  }

  
  
  if (aFlushDirtyRegion) {
    FlushDirtyRegionToWidget(aView);
  }
}

void nsViewManager::FlushDirtyRegionToWidget(nsView* aView)
{
  if (!aView->HasNonEmptyDirtyRegion())
    return;

  nsRegion* dirtyRegion = aView->GetDirtyRegion();
  nsView* nearestViewWithWidget = aView;
  while (!nearestViewWithWidget->HasWidget() &&
         nearestViewWithWidget->GetParent()) {
    nearestViewWithWidget = nearestViewWithWidget->GetParent();
  }
  nsRegion r =
    ConvertRegionBetweenViews(*dirtyRegion, aView, nearestViewWithWidget);
  nsViewManager* widgetVM = nearestViewWithWidget->GetViewManager();
  widgetVM->InvalidateWidgetArea(nearestViewWithWidget, r);
  dirtyRegion->SetEmpty();
}

NS_IMETHODIMP nsViewManager::InvalidateView(nsIView *aView)
{
  
  return InvalidateView(aView, aView->GetDimensions());
}

static void
AddDirtyRegion(nsView *aView, const nsRegion &aDamagedRegion)
{
  nsRegion* dirtyRegion = aView->GetDirtyRegion();
  if (!dirtyRegion)
    return;

  dirtyRegion->Or(*dirtyRegion, aDamagedRegion);
  dirtyRegion->SimplifyOutward(8);
}

void
nsViewManager::PostPendingUpdate()
{
  nsViewManager* rootVM = RootViewManager();
  rootVM->mHasPendingUpdates = true;
  rootVM->mHasPendingWidgetGeometryChanges = true;
  if (rootVM->mPresShell) {
    rootVM->mPresShell->ScheduleViewManagerFlush();
  }
}





void
nsViewManager::InvalidateWidgetArea(nsView *aWidgetView,
                                    const nsRegion &aDamagedRegion)
{
  NS_ASSERTION(aWidgetView->GetViewManager() == this,
               "InvalidateWidgetArea called on view we don't own");
  nsIWidget* widget = aWidgetView->GetWidget();

#if 0
  nsRect dbgBounds = aDamagedRegion.GetBounds();
  printf("InvalidateWidgetArea view:%X (%d) widget:%X region: %d, %d, %d, %d\n",
    aWidgetView, aWidgetView->IsAttachedToTopLevel(),
    widget, dbgBounds.x, dbgBounds.y, dbgBounds.width, dbgBounds.height);
#endif

  
  if (widget) {
    bool visible;
    widget->IsVisible(visible);
    if (!visible)
      return;
  }

  if (!widget) {
    
    
    
    return;
  }

  
  
  
  nsRegion children;
  if (widget->GetTransparencyMode() != eTransparencyTransparent) {
    for (nsIWidget* childWidget = widget->GetFirstChild();
         childWidget;
         childWidget = childWidget->GetNextSibling()) {
      nsView* view = nsView::GetViewFor(childWidget);
      NS_ASSERTION(view != aWidgetView, "will recur infinitely");
      bool visible;
      childWidget->IsVisible(visible);
      nsWindowType type;
      childWidget->GetWindowType(type);
      if (view && visible && type != eWindowType_popup) {
        NS_ASSERTION(type == eWindowType_plugin,
                     "Only plugin or popup widgets can be children!");

        
        
        
        
#ifndef XP_MACOSX
        
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
#endif
      }
    }
  }

  nsRegion leftOver;
  leftOver.Sub(aDamagedRegion, children);

  if (!leftOver.IsEmpty()) {
    const nsRect* r;
    for (nsRegionRectIterator iter(leftOver); (r = iter.Next());) {
      nsIntRect bounds = ViewToWidget(aWidgetView, *r);
      widget->Invalidate(bounds);
    }
  }
}

static bool
ShouldIgnoreInvalidation(nsViewManager* aVM)
{
  while (aVM) {
    nsIPresShell* shell = aVM->GetPresShell();
    if (!shell || shell->ShouldIgnoreInvalidation()) {
      return true;
    }
    nsView* view = aVM->GetRootViewImpl()->GetParent();
    aVM = view ? view->GetViewManager() : nsnull;
  }
  return false;
}

nsresult nsViewManager::InvalidateView(nsIView *aView, const nsRect &aRect)
{
  
  
  if (ShouldIgnoreInvalidation(this)) {
    return NS_OK;
  }

  return InvalidateViewNoSuppression(aView, aRect);
}

NS_IMETHODIMP nsViewManager::InvalidateViewNoSuppression(nsIView *aView,
                                                         const nsRect &aRect)
{
  NS_PRECONDITION(nsnull != aView, "null view");

  nsView* view = static_cast<nsView*>(aView);

  NS_ASSERTION(view->GetViewManager() == this,
               "InvalidateViewNoSuppression called on view we don't own");

  nsRect damagedRect(aRect);
  if (damagedRect.IsEmpty()) {
    return NS_OK;
  }

  nsView* displayRoot = static_cast<nsView*>(GetDisplayRootFor(view));
  nsViewManager* displayRootVM = displayRoot->GetViewManager();
  
  
  
  damagedRect.MoveBy(view->GetOffsetTo(displayRoot));
  PRInt32 rootAPD = displayRootVM->AppUnitsPerDevPixel();
  PRInt32 APD = AppUnitsPerDevPixel();
  damagedRect = damagedRect.ConvertAppUnitsRoundOut(APD, rootAPD);

  
  
  AddDirtyRegion(displayRoot, nsRegion(damagedRect));

  
  PostPendingUpdate();

  return NS_OK;
}

NS_IMETHODIMP nsViewManager::InvalidateAllViews()
{
  if (RootViewManager() != this) {
    return RootViewManager()->InvalidateAllViews();
  }
  
  InvalidateViews(mRootView);
  return NS_OK;
}

void nsViewManager::InvalidateViews(nsView *aView)
{
  
  InvalidateView(aView);

  
  nsView* childView = aView->GetFirstChild();
  while (nsnull != childView)  {
    childView->GetViewManager()->InvalidateViews(childView);
    childView = childView->GetNextSibling();
  }
}

static bool
IsViewForPopup(nsIView* aView)
{
  nsIWidget* widget = aView->GetWidget();
  if (widget) {
    nsWindowType type;
    widget->GetWindowType(type);
    return (type == eWindowType_popup);
  }

  return false;
}

NS_IMETHODIMP nsViewManager::DispatchEvent(nsGUIEvent *aEvent,
                                           nsIView* aView, nsEventStatus *aStatus)
{
  NS_ASSERTION(!aView || static_cast<nsView*>(aView)->GetViewManager() == this,
               "wrong view manager");

  SAMPLE_LABEL("event", "nsViewManager::DispatchEvent");

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
            else if (IsViewForPopup(aView))
              {
                nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
                if (pm)
                  {
                    pm->PopupResized(aView->GetFrame(), nsIntSize(width, height));
                    *aStatus = nsEventStatus_eConsumeNoDefault;
                  }
              }
          }
        }

        break;

    case NS_MOVE:
      {
        
        
        
        if (aView && IsViewForPopup(aView))
          {
            nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
            if (pm)
              {
                pm->PopupMoved(aView->GetFrame(), aEvent->refPoint);
                *aStatus = nsEventStatus_eConsumeNoDefault;
              }
          }
        break;
      }

    case NS_DONESIZEMOVE:
      {
        if (mPresShell) {
          nsPresContext* presContext = mPresShell->GetPresContext();
          if (presContext) {
            nsEventStateManager::ClearGlobalActiveContent(nsnull);
          }

        }

        nsIPresShell::ClearMouseCapture(nsnull);
      }
      break;
  
    case NS_XUL_CLOSE:
      {
        
        
        nsIWidget* widget = aView->GetWidget();
        if (widget) {
          nsWindowType type;
          widget->GetWindowType(type);
          if (type == eWindowType_popup) {
            nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
            if (pm) {
              pm->HidePopup(aView->GetFrame());
              *aStatus = nsEventStatus_eConsumeNoDefault;
            }
          }
        }
      }
      break;

    case NS_WILL_PAINT:
      {
        if (!aView || !mContext)
          break;

        *aStatus = nsEventStatus_eConsumeNoDefault;

        nsPaintEvent *event = static_cast<nsPaintEvent*>(aEvent);

        NS_ASSERTION(static_cast<nsView*>(aView) ==
                       nsView::GetViewFor(event->widget),
                     "view/widget mismatch");

        
        
        for (nsViewManager *vm = this; vm;
             vm = vm->mRootView->GetParent()
                    ? vm->mRootView->GetParent()->GetViewManager()
                    : nsnull) {
          if (vm->mDelayedResize != nsSize(NSCOORD_NONE, NSCOORD_NONE) &&
              vm->mRootView->IsEffectivelyVisible() &&
              mPresShell && mPresShell->IsVisible()) {
            vm->FlushDelayedResize(true);
            vm->InvalidateView(vm->mRootView);
          }
        }

        
        
        nsRefPtr<nsViewManager> rootVM = RootViewManager();
        if (mPresShell) {
          rootVM->CallWillPaintOnObservers(event->willSendDidPaint);
        }
        
        rootVM->ProcessPendingUpdates();
      }
      break;

    case NS_PAINT:
      {
        if (!aView || !mContext)
          break;

        *aStatus = nsEventStatus_eConsumeNoDefault;
        nsPaintEvent *event = static_cast<nsPaintEvent*>(aEvent);
        nsView* view = static_cast<nsView*>(aView);
        NS_ASSERTION(view == nsView::GetViewFor(event->widget),
                     "view/widget mismatch");
        NS_ASSERTION(IsPaintingAllowed(),
                     "shouldn't be receiving paint events while painting is "
                     "disallowed!");

        if (!event->didSendWillPaint) {
          
          nsPaintEvent willPaintEvent(true, NS_WILL_PAINT, event->widget);
          willPaintEvent.willSendDidPaint = event->willSendDidPaint;
          DispatchEvent(&willPaintEvent, view, aStatus);

          
          
          view = nsView::GetViewFor(event->widget);
        }

        if (!view || event->region.IsEmpty())
          break;

        
        Refresh(view, event->widget, event->region, event->willSendDidPaint);

        break;
      }

    case NS_DID_PAINT: {
      nsRefPtr<nsViewManager> rootVM = RootViewManager();
      rootVM->CallDidPaintOnObserver();
      break;
    }

    case NS_CREATE:
    case NS_DESTROY:
    case NS_SETZLEVEL:
      


      *aStatus = nsEventStatus_eConsumeNoDefault;
      break;

    case NS_DISPLAYCHANGED:

      
      
      
      *aStatus = nsEventStatus_eConsumeDoDefault;
      break;

    case NS_SYSCOLORCHANGED:
      {
        if (mPresShell) {
          
          
          
          nsCOMPtr<nsIPresShell> presShell = mPresShell;
          presShell->HandleEvent(aView->GetFrame(), aEvent, false, aStatus);
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
            aEvent->message == NS_PLUGIN_INPUT_EVENT) {
          gLastUserEventTime = PR_IntervalToMicroseconds(PR_IntervalNow());
        }

        if (aEvent->message == NS_DEACTIVATE) {
          
          
          nsIPresShell::ClearMouseCapture(nsnull);
        }

        
        nsIView* view = aView;
        bool dispatchUsingCoordinates = NS_IsEventUsingCoordinates(aEvent);
        if (dispatchUsingCoordinates) {
          
          
          view = GetDisplayRootFor(view);
        }
  
        
        nsIFrame* frame = view->GetFrame();
        if (!frame &&
            (dispatchUsingCoordinates || NS_IS_KEY_EVENT(aEvent) ||
             NS_IS_IME_RELATED_EVENT(aEvent) ||
             NS_IS_NON_RETARGETED_PLUGIN_EVENT(aEvent) ||
             aEvent->message == NS_PLUGIN_ACTIVATE ||
             aEvent->message == NS_PLUGIN_FOCUS)) {
          while (view && !view->GetFrame()) {
            view = view->GetParent();
          }

          if (view) {
            frame = view->GetFrame();
          }
        }

        if (nsnull != frame) {
          
          
          
          nsCOMPtr<nsIPresShell> shell = view->GetViewManager()->GetPresShell();
          if (shell) {
            shell->HandleEvent(frame, aEvent, false, aStatus);
          }
        }
    
        break;
      }
    }

  return NS_OK;
}



void nsViewManager::ReparentChildWidgets(nsIView* aView, nsIWidget *aNewWidget)
{
  NS_PRECONDITION(aNewWidget, "");

  if (aView->HasWidget()) {
    
    
    
    
    nsIWidget* widget = aView->GetWidget();
    nsIWidget* parentWidget = widget->GetParent();
    if (parentWidget) {
      
      if (parentWidget != aNewWidget) {
#ifdef DEBUG
        nsresult rv =
#endif
          widget->SetParent(aNewWidget);
        NS_ASSERTION(NS_SUCCEEDED(rv), "SetParent failed!");
      }
    } else {
      
      widget->ReparentNativeWidget(aNewWidget);
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
                                         bool aAfter)
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
        child->SetFloating(true);

      

      if (nsViewVisibility_kHide != child->GetVisibility())
        child->GetViewManager()->InvalidateView(child);
    }
  return NS_OK;
}

NS_IMETHODIMP nsViewManager::InsertChild(nsIView *aParent, nsIView *aChild, PRInt32 aZIndex)
{
  
  
  SetViewZIndex(aChild, false, aZIndex, false);
  return InsertChild(aParent, aChild, nsnull, true);
}

NS_IMETHODIMP nsViewManager::RemoveChild(nsIView *aChild)
{
  nsView* child = static_cast<nsView*>(aChild);
  NS_ENSURE_ARG_POINTER(child);

  nsView* parent = child->GetParent();

  if (nsnull != parent) {
    NS_ASSERTION(child->GetViewManager() == this ||
                 parent->GetViewManager() == this, "wrong view manager");
    child->GetViewManager()->InvalidateView(child);
    parent->RemoveChild(child);
  }

  return NS_OK;
}

NS_IMETHODIMP nsViewManager::MoveViewTo(nsIView *aView, nscoord aX, nscoord aY)
{
  nsView* view = static_cast<nsView*>(aView);
  NS_ASSERTION(view->GetViewManager() == this, "wrong view manager");
  nsPoint oldPt = view->GetPosition();
  nsRect oldBounds = view->GetBoundsInParentUnits();
  view->SetPosition(aX, aY);

  

  if ((aX != oldPt.x) || (aY != oldPt.y)) {
    if (view->GetVisibility() != nsViewVisibility_kHide) {
      nsView* parentView = view->GetParent();
      if (parentView) {
        nsViewManager* parentVM = parentView->GetViewManager();
        parentVM->InvalidateView(parentView, oldBounds);
        parentVM->InvalidateView(parentView, view->GetBoundsInParentUnits());
      }
    }
  }
  return NS_OK;
}

void nsViewManager::InvalidateHorizontalBandDifference(nsView *aView, const nsRect& aRect, const nsRect& aCutOut,
  nscoord aY1, nscoord aY2, bool aInCutOut) {
  nscoord height = aY2 - aY1;
  if (aRect.x < aCutOut.x) {
    nsRect r(aRect.x, aY1, aCutOut.x - aRect.x, height);
    InvalidateView(aView, r);
  }
  if (!aInCutOut && aCutOut.x < aCutOut.XMost()) {
    nsRect r(aCutOut.x, aY1, aCutOut.width, height);
    InvalidateView(aView, r);
  }
  if (aCutOut.XMost() < aRect.XMost()) {
    nsRect r(aCutOut.XMost(), aY1, aRect.XMost() - aCutOut.XMost(), height);
    InvalidateView(aView, r);
  }
}

void nsViewManager::InvalidateRectDifference(nsView *aView, const nsRect& aRect, const nsRect& aCutOut) {
  NS_ASSERTION(aView->GetViewManager() == this,
               "InvalidateRectDifference called on view we don't own");
  if (aRect.y < aCutOut.y) {
    InvalidateHorizontalBandDifference(aView, aRect, aCutOut, aRect.y, aCutOut.y, false);
  }
  if (aCutOut.y < aCutOut.YMost()) {
    InvalidateHorizontalBandDifference(aView, aRect, aCutOut, aCutOut.y, aCutOut.YMost(), true);
  }
  if (aCutOut.YMost() < aRect.YMost()) {
    InvalidateHorizontalBandDifference(aView, aRect, aCutOut, aCutOut.YMost(), aRect.YMost(), false);
  }
}

NS_IMETHODIMP nsViewManager::ResizeView(nsIView *aView, const nsRect &aRect, bool aRepaintExposedAreaOnly)
{
  nsView* view = static_cast<nsView*>(aView);
  NS_ASSERTION(view->GetViewManager() == this, "wrong view manager");

  nsRect oldDimensions = view->GetDimensions();
  if (!oldDimensions.IsEqualEdges(aRect)) {
    
    
    if (view->GetVisibility() == nsViewVisibility_kHide) {  
      view->SetDimensions(aRect, false);
    } else {
      nsView* parentView = view->GetParent();
      if (!parentView) {
        parentView = view;
      }
      nsRect oldBounds = view->GetBoundsInParentUnits();
      view->SetDimensions(aRect, true);
      nsViewManager* parentVM = parentView->GetViewManager();
      if (!aRepaintExposedAreaOnly) {
        
        InvalidateView(view, aRect);
        parentVM->InvalidateView(parentView, oldBounds);
      } else {
        InvalidateRectDifference(view, aRect, oldDimensions);
        nsRect newBounds = view->GetBoundsInParentUnits();
        parentVM->InvalidateRectDifference(parentView, oldBounds, newBounds);
      } 
    }
  }

  
  
  
  
  

  return NS_OK;
}

NS_IMETHODIMP nsViewManager::SetViewFloating(nsIView *aView, bool aFloating)
{
  nsView* view = static_cast<nsView*>(aView);

  NS_ASSERTION(!(nsnull == view), "no view");

  view->SetFloating(aFloating);

  return NS_OK;
}

NS_IMETHODIMP nsViewManager::SetViewVisibility(nsIView *aView, nsViewVisibility aVisible)
{
  nsView* view = static_cast<nsView*>(aView);
  NS_ASSERTION(view->GetViewManager() == this, "wrong view manager");

  if (aVisible != view->GetVisibility()) {
    view->SetVisibility(aVisible);

    if (IsViewInserted(view)) {
      if (!view->HasWidget()) {
        if (nsViewVisibility_kHide == aVisible) {
          nsView* parentView = view->GetParent();
          if (parentView) {
            parentView->GetViewManager()->
              InvalidateView(parentView, view->GetBoundsInParentUnits());
          }
        }
        else {
          InvalidateView(view);
        }
      }
    }
  }
  return NS_OK;
}

bool nsViewManager::IsViewInserted(nsView *aView)
{
  if (mRootView == aView) {
    return true;
  } else if (aView->GetParent() == nsnull) {
    return false;
  } else {
    nsView* view = aView->GetParent()->GetFirstChild();
    while (view != nsnull) {
      if (view == aView) {
        return true;
      }        
      view = view->GetNextSibling();
    }
    return false;
  }
}

NS_IMETHODIMP nsViewManager::SetViewZIndex(nsIView *aView, bool aAutoZIndex, PRInt32 aZIndex, bool aTopMost)
{
  nsView* view = static_cast<nsView*>(aView);
  nsresult  rv = NS_OK;

  NS_ASSERTION((view != nsnull), "no view");

  
  
  if (aView == mRootView) {
    return rv;
  }

  bool oldTopMost = view->IsTopMost();
  bool oldIsAuto = view->GetZIndexIsAuto();

  if (aAutoZIndex) {
    aZIndex = 0;
  }

  PRInt32 oldidx = view->GetZIndex();
  view->SetZIndex(aAutoZIndex, aZIndex, aTopMost);

  if (oldidx != aZIndex || oldTopMost != aTopMost ||
      oldIsAuto != aAutoZIndex) {
    InvalidateView(view);
  }

  return rv;
}

NS_IMETHODIMP nsViewManager::GetDeviceContext(nsDeviceContext *&aContext)
{
  aContext = mContext;
  NS_IF_ADDREF(aContext);
  return NS_OK;
}

nsIViewManager*
nsViewManager::IncrementDisableRefreshCount()
{
  if (!IsRootVM()) {
    return RootViewManager()->IncrementDisableRefreshCount();
  }

  ++mRefreshDisableCount;

  return this;
}

void
nsViewManager::DecrementDisableRefreshCount()
{
  NS_ASSERTION(IsRootVM(), "Should only be called on root");
  --mRefreshDisableCount;
  NS_ASSERTION(mRefreshDisableCount >= 0, "Invalid refresh disable count!");
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

nsIntRect nsViewManager::ViewToWidget(nsView *aView, const nsRect &aRect) const
{
  NS_ASSERTION(aView->GetViewManager() == this, "wrong view manager");

  
  nsRect rect = aRect + aView->ViewToWidgetOffset();

  
  return rect.ToOutsidePixels(AppUnitsPerDevPixel());
}

NS_IMETHODIMP
nsViewManager::IsPainting(bool& aIsPainting)
{
  aIsPainting = IsPainting();
  return NS_OK;
}

void
nsViewManager::ProcessPendingUpdates()
{
  if (!IsRootVM()) {
    RootViewManager()->ProcessPendingUpdates();
    return;
  }

  if (mHasPendingUpdates) {
    ProcessPendingUpdatesForView(mRootView, true);
    mHasPendingUpdates = false;
  }
}

void
nsViewManager::UpdateWidgetGeometry()
{
  if (!IsRootVM()) {
    RootViewManager()->UpdateWidgetGeometry();
    return;
  }

  if (mHasPendingWidgetGeometryChanges) {
    ProcessPendingUpdatesForView(mRootView, false);
    mHasPendingWidgetGeometryChanges = false;
  }
}

void
nsViewManager::CallWillPaintOnObservers(bool aWillSendDidPaint)
{
  NS_PRECONDITION(IsRootVM(), "Must be root VM for this to be called!");

  PRInt32 index;
  for (index = 0; index < mVMCount; index++) {
    nsViewManager* vm = (nsViewManager*)gViewManagers->ElementAt(index);
    if (vm->RootViewManager() == this) {
      
      if (vm->mRootView && vm->mRootView->IsEffectivelyVisible()) {
        nsCOMPtr<nsIPresShell> shell = vm->GetPresShell();
        if (shell) {
          shell->WillPaint(aWillSendDidPaint);
        }
      }
    }
  }
}

void
nsViewManager::CallDidPaintOnObserver()
{
  NS_PRECONDITION(IsRootVM(), "Must be root VM for this to be called!");

  if (mRootView && mRootView->IsEffectivelyVisible()) {
    nsCOMPtr<nsIPresShell> shell = GetPresShell();
    if (shell) {
      shell->DidPaint();
    }
  }
}

NS_IMETHODIMP
nsViewManager::GetLastUserEventTime(PRUint32& aTime)
{
  aTime = gLastUserEventTime;
  return NS_OK;
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
