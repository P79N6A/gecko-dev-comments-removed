




#define PL_ARENA_CONST_ALIGN_MASK (sizeof(void*)-1)
#include "plarena.h"

#include "nsAutoPtr.h"
#include "nsViewManager.h"
#include "nsGfxCIID.h"
#include "nsIView.h"
#include "nsISupportsArray.h"
#include "nsCOMPtr.h"
#include "nsGUIEvent.h"
#include "nsRegion.h"
#include "nsHashtable.h"
#include "nsCOMArray.h"
#include "nsIPluginWidget.h"
#include "nsXULPopupManager.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsEventStateManager.h"
#include "mozilla/StartupTimeline.h"
#include "sampler.h"
#include "nsRefreshDriver.h"
#include "mozilla/Preferences.h"

















#define NSCOORD_NONE      INT32_MIN

#undef DEBUG_MOUSE_LOCATION

static bool
IsRefreshDriverPaintingEnabled()
{
  static bool sRefreshDriverPaintingEnabled;
  static bool sRefreshDriverPaintingPrefCached = false;

  if (!sRefreshDriverPaintingPrefCached) {
    sRefreshDriverPaintingPrefCached = true;
    mozilla::Preferences::AddBoolVarCache(&sRefreshDriverPaintingEnabled,
                                          "viewmanager.refresh-driver-painting.enabled",
                                          true);
  }

  return sRefreshDriverPaintingEnabled;
}

int32_t nsViewManager::mVMCount = 0;


nsVoidArray* nsViewManager::gViewManagers = nullptr;
uint32_t nsViewManager::gLastUserEventTime = 0;

nsViewManager::nsViewManager()
  : mDelayedResize(NSCOORD_NONE, NSCOORD_NONE)
{
  mRootViewManager = this;
  if (gViewManagers == nullptr) {
    NS_ASSERTION(mVMCount == 0, "View Manager count is incorrect");
    
    gViewManagers = new nsVoidArray;
  }
 
  gViewManagers->AppendElement(this);

  ++mVMCount;

  
  
  mHasPendingWidgetGeometryChanges = false;
  mRecursiveRefreshPending = false;
}

nsViewManager::~nsViewManager()
{
  if (mRootView) {
    
    mRootView->Destroy();
    mRootView = nullptr;
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
    
    
   
    NS_ASSERTION(gViewManagers != nullptr, "About to delete null gViewManagers");
    delete gViewManagers;
    gViewManagers = nullptr;
  }

  mPresShell = nullptr;
}

NS_IMPL_ISUPPORTS1(nsViewManager, nsIViewManager)



NS_IMETHODIMP nsViewManager::Init(nsDeviceContext* aContext)
{
  NS_PRECONDITION(nullptr != aContext, "null ptr");

  if (nullptr == aContext) {
    return NS_ERROR_NULL_POINTER;
  }
  if (nullptr != mContext) {
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
  nsIView *v = new nsIView(this, aVisibilityFlag);
  if (v) {
    v->SetParent(static_cast<nsIView*>(const_cast<nsIView*>(aParent)));
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
  nsIView* view = static_cast<nsIView*>(aView);

  NS_PRECONDITION(!view || view->GetViewManager() == this,
                  "Unexpected viewmanager on root view");
  
  
  
  mRootView = view;

  if (mRootView) {
    nsIView* parent = mRootView->GetParent();
    if (parent) {
      
      
      parent->InsertChild(mRootView, nullptr);
    } else {
      InvalidateHierarchy();
    }

    mRootView->SetZIndex(false, 0, false);
  }
  

  return NS_OK;
}

NS_IMETHODIMP nsViewManager::GetWindowDimensions(nscoord *aWidth, nscoord *aHeight)
{
  if (nullptr != mRootView) {
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
                                          nsIView* aFromView,
                                          nsIView* aToView)
{
  nsRegion out = aIn;
  out.MoveBy(aFromView->GetOffsetTo(aToView));
  out = out.ConvertAppUnitsRoundOut(
    aFromView->GetViewManagerInternal()->AppUnitsPerDevPixel(),
    aToView->GetViewManagerInternal()->AppUnitsPerDevPixel());
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






void nsViewManager::Refresh(nsIView *aView, const nsIntRegion& aRegion,
                            bool aWillSendDidPaint)
{
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
  
  nsIWidget *widget = aView->GetWidget();
  if (!widget) {
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
#ifdef DEBUG_INVALIDATIONS
      printf("--COMPOSITE-- %p\n", mPresShell);
#endif
      mPresShell->Paint(aView, damageRegion,
                        (IsRefreshDriverPaintingEnabled() ? 0 : nsIPresShell::PAINT_LAYERS) |
                        nsIPresShell::PAINT_COMPOSITE |
                        (aWillSendDidPaint ? nsIPresShell::PAINT_WILL_SEND_DID_PAINT : 0));
#ifdef DEBUG_INVALIDATIONS
      printf("--ENDCOMPOSITE--\n");
#endif
      mozilla::StartupTimeline::RecordOnce(mozilla::StartupTimeline::FIRST_PAINT);
    }

    SetPainting(false);
  }

  if (RootViewManager()->mRecursiveRefreshPending) {
    RootViewManager()->mRecursiveRefreshPending = false;
    InvalidateAllViews();
  }
}

void nsViewManager::ProcessPendingUpdatesForView(nsIView* aView,
                                                 bool aFlushDirtyRegion)
{
  NS_ASSERTION(IsRootVM(), "Updates will be missed");

  
  if (!aView) {
    return;
  }

  if (aView->HasWidget()) {
    aView->ResetWidgetBounds(false, true);
  }

  
  for (nsIView* childView = aView->GetFirstChild(); childView;
       childView = childView->GetNextSibling()) {
    ProcessPendingUpdatesForView(childView, aFlushDirtyRegion);
  }

  
  
  if (aFlushDirtyRegion) {
    if (IsRefreshDriverPaintingEnabled()) {
      nsIWidget *widget = aView->GetWidget();
      if (widget && widget->NeedsPaint()) {
        
        
        for (nsViewManager *vm = this; vm;
             vm = vm->mRootView->GetParent()
                    ? vm->mRootView->GetParent()->GetViewManagerInternal()
                    : nullptr) {
          if (vm->mDelayedResize != nsSize(NSCOORD_NONE, NSCOORD_NONE) &&
              vm->mRootView->IsEffectivelyVisible() &&
              mPresShell && mPresShell->IsVisible()) {
            vm->FlushDelayedResize(true);
            vm->InvalidateView(vm->mRootView);
          }
        }

        NS_ASSERTION(aView->HasWidget(), "Must have a widget!");

        SetPainting(true);
#ifdef DEBUG_INVALIDATIONS
        printf("---- PAINT START ----PresShell(%p), nsIView(%p), nsIWidget(%p)\n", mPresShell, aView, widget);
#endif
        nsAutoScriptBlocker scriptBlocker;
        NS_ASSERTION(aView->HasWidget(), "Must have a widget!");
        mPresShell->Paint(aView, nsRegion(),
                          nsIPresShell::PAINT_LAYERS |
                          nsIPresShell::PAINT_WILL_SEND_DID_PAINT);
#ifdef DEBUG_INVALIDATIONS
        printf("---- PAINT END ----\n");
#endif
        aView->SetForcedRepaint(false);
        SetPainting(false);
      }
    }
    FlushDirtyRegionToWidget(aView);
  }
}

void nsViewManager::FlushDirtyRegionToWidget(nsIView* aView)
{
  if (!aView->HasNonEmptyDirtyRegion())
    return;

  nsRegion* dirtyRegion = aView->GetDirtyRegion();
  nsIView* nearestViewWithWidget = aView;
  while (!nearestViewWithWidget->HasWidget() &&
         nearestViewWithWidget->GetParent()) {
    nearestViewWithWidget = nearestViewWithWidget->GetParent();
  }
  nsRegion r =
    ConvertRegionBetweenViews(*dirtyRegion, aView, nearestViewWithWidget);
  nsViewManager* widgetVM = nearestViewWithWidget->GetViewManagerInternal();
  widgetVM->InvalidateWidgetArea(nearestViewWithWidget, r);
  dirtyRegion->SetEmpty();
}

NS_IMETHODIMP nsViewManager::InvalidateView(nsIView *aView)
{
  
  return InvalidateView(aView, aView->GetDimensions());
}

static void
AddDirtyRegion(nsIView *aView, const nsRegion &aDamagedRegion)
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
  rootVM->mHasPendingWidgetGeometryChanges = true;
  if (rootVM->mPresShell) {
    rootVM->mPresShell->ScheduleViewManagerFlush();
  }
}





void
nsViewManager::InvalidateWidgetArea(nsIView *aWidgetView,
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

  
  if (widget && !widget->IsVisible()) {
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
      nsIView* view = nsIView::GetViewFor(childWidget);
      NS_ASSERTION(view != aWidgetView, "will recur infinitely");
      nsWindowType type;
      childWidget->GetWindowType(type);
      if (view && childWidget->IsVisible() && type != eWindowType_popup) {
        NS_ASSERTION(type == eWindowType_plugin,
                     "Only plugin or popup widgets can be children!");

        
        
        
        
#ifndef XP_MACOSX
        
        nsIntRect bounds;
        childWidget->GetBounds(bounds);

        nsTArray<nsIntRect> clipRects;
        childWidget->GetWindowClipRegion(&clipRects);
        for (uint32_t i = 0; i < clipRects.Length(); ++i) {
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
    nsIView* view = aVM->GetRootViewImpl()->GetParent();
    aVM = view ? view->GetViewManagerInternal() : nullptr;
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
  NS_PRECONDITION(nullptr != aView, "null view");

  nsIView* view = static_cast<nsIView*>(aView);

  NS_ASSERTION(view->GetViewManager() == this,
               "InvalidateViewNoSuppression called on view we don't own");

  nsRect damagedRect(aRect);
  if (damagedRect.IsEmpty()) {
    return NS_OK;
  }

  nsIView* displayRoot = static_cast<nsIView*>(GetDisplayRootFor(view));
  nsViewManager* displayRootVM = displayRoot->GetViewManagerInternal();
  
  
  
  damagedRect.MoveBy(view->GetOffsetTo(displayRoot));
  int32_t rootAPD = displayRootVM->AppUnitsPerDevPixel();
  int32_t APD = AppUnitsPerDevPixel();
  damagedRect = damagedRect.ConvertAppUnitsRoundOut(APD, rootAPD);

  
  
  AddDirtyRegion(displayRoot, nsRegion(damagedRect));

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

void nsViewManager::InvalidateViews(nsIView *aView)
{
  
  InvalidateView(aView);

  
  nsIView* childView = aView->GetFirstChild();
  while (nullptr != childView)  {
    childView->GetViewManagerInternal()->InvalidateViews(childView);
    childView = childView->GetNextSibling();
  }
}

void nsViewManager::WillPaintWindow(nsIWidget* aWidget, bool aWillSendDidPaint)
{
  if (!IsRefreshDriverPaintingEnabled() && aWidget && mContext) {
    
    
    for (nsViewManager *vm = this; vm;
         vm = vm->mRootView->GetParent()
                ? vm->mRootView->GetParent()->GetViewManagerInternal()
                : nullptr) {
      if (vm->mDelayedResize != nsSize(NSCOORD_NONE, NSCOORD_NONE) &&
          vm->mRootView->IsEffectivelyVisible() &&
          mPresShell && mPresShell->IsVisible()) {
        vm->FlushDelayedResize(true);
        vm->InvalidateView(vm->mRootView);
      }
    }

    
    nsRefPtr<nsViewManager> rootVM = RootViewManager();
    rootVM->CallWillPaintOnObservers(aWillSendDidPaint);

    
    rootVM->ProcessPendingUpdates();
  }

  if (aWidget && IsRefreshDriverPaintingEnabled()) {
    nsIView* view = nsIView::GetViewFor(aWidget);
    if (view && view->ForcedRepaint()) {
      ProcessPendingUpdates();
      
      
      view = nsIView::GetViewFor(aWidget);
      if (view) {
        view->SetForcedRepaint(false);
      }
    }
  }

  nsCOMPtr<nsIPresShell> shell = mPresShell;
  if (shell) {
    shell->WillPaintWindow(aWillSendDidPaint);
  }
}

bool nsViewManager::PaintWindow(nsIWidget* aWidget, nsIntRegion aRegion,
                                uint32_t aFlags)
 {
  if (!aWidget || !mContext)
    return false;

  NS_ASSERTION(IsPaintingAllowed(),
               "shouldn't be receiving paint events while painting is disallowed!");

  if (!(aFlags & nsIWidgetListener::SENT_WILL_PAINT) && !IsRefreshDriverPaintingEnabled()) {
    WillPaintWindow(aWidget, (aFlags & nsIWidgetListener::WILL_SEND_DID_PAINT));
  }

  
  
  nsIView* view = nsIView::GetViewFor(aWidget);
  if (view && !aRegion.IsEmpty()) {
    Refresh(view, aRegion, (aFlags & nsIWidgetListener::WILL_SEND_DID_PAINT));
  }

  return true;
}

void nsViewManager::DidPaintWindow()
{
  nsCOMPtr<nsIPresShell> shell = mPresShell;
  if (shell) {
    shell->DidPaintWindow();
  }
}

nsresult nsViewManager::DispatchEvent(nsGUIEvent *aEvent, nsIView* aView, nsEventStatus* aStatus)
{
  SAMPLE_LABEL("event", "nsViewManager::DispatchEvent");

  if ((NS_IS_MOUSE_EVENT(aEvent) &&
       
       static_cast<nsMouseEvent*>(aEvent)->reason == nsMouseEvent::eReal &&
       
       
       
       aEvent->message != NS_MOUSE_EXIT &&
       aEvent->message != NS_MOUSE_ENTER) ||
      NS_IS_KEY_EVENT(aEvent) ||
      NS_IS_IME_EVENT(aEvent) ||
      aEvent->message == NS_PLUGIN_INPUT_EVENT) {
    gLastUserEventTime = PR_IntervalToMicroseconds(PR_IntervalNow());
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
       aEvent->message == NS_PLUGIN_FOCUS ||
       aEvent->message == NS_PLUGIN_RESOLUTION_CHANGED)) {
    while (view && !view->GetFrame()) {
      view = view->GetParent();
    }

    if (view) {
      frame = view->GetFrame();
    }
  }

  if (nullptr != frame) {
    
    
    
    nsCOMPtr<nsIPresShell> shell = view->GetViewManager()->GetPresShell();
    if (shell) {
      return shell->HandleEvent(frame, aEvent, false, aStatus);
    }
  }

  *aStatus = nsEventStatus_eIgnore;

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

  
  

  nsIView* view = static_cast<nsIView*>(aView);
  for (nsIView *kid = view->GetFirstChild(); kid; kid = kid->GetNextSibling()) {
    ReparentChildWidgets(kid, aNewWidget);
  }
}



void nsViewManager::ReparentWidgets(nsIView* aView, nsIView *aParent)
{
  NS_PRECONDITION(aParent, "Must have a parent");
  NS_PRECONDITION(aView, "Must have a view");
  
  
  
  
  
  
  
  
  nsIView* view = static_cast<nsIView*>(aView);
  if (view->HasWidget() || view->GetFirstChild()) {
    nsIWidget* parentWidget = aParent->GetNearestWidget(nullptr);
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
  nsIView* parent = static_cast<nsIView*>(aParent);
  nsIView* child = static_cast<nsIView*>(aChild);
  nsIView* sibling = static_cast<nsIView*>(aSibling);
  
  NS_PRECONDITION(nullptr != parent, "null ptr");
  NS_PRECONDITION(nullptr != child, "null ptr");
  NS_ASSERTION(sibling == nullptr || sibling->GetParent() == parent,
               "tried to insert view with invalid sibling");
  NS_ASSERTION(!IsViewInserted(child), "tried to insert an already-inserted view");

  if ((nullptr != parent) && (nullptr != child))
    {
      
      

#if 1
      if (nullptr == aSibling) {
        if (aAfter) {
          
          
          parent->InsertChild(child, nullptr);
          ReparentWidgets(child, parent);
        } else {
          
          nsIView *kid = parent->GetFirstChild();
          nsIView *prev = nullptr;
          while (kid) {
            prev = kid;
            kid = kid->GetNextSibling();
          }
          
          parent->InsertChild(child, prev);
          ReparentWidgets(child, parent);
        }
      } else {
        nsIView *kid = parent->GetFirstChild();
        nsIView *prev = nullptr;
        while (kid && sibling != kid) {
          
          prev = kid;
          kid = kid->GetNextSibling();
        }
        NS_ASSERTION(kid != nullptr,
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
      
      int32_t zIndex = child->GetZIndex();
      while (nullptr != kid)
        {
          int32_t idx = kid->GetZIndex();

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
        child->GetViewManagerInternal()->InvalidateView(child);
    }
  return NS_OK;
}

NS_IMETHODIMP nsViewManager::InsertChild(nsIView *aParent, nsIView *aChild, int32_t aZIndex)
{
  
  
  SetViewZIndex(aChild, false, aZIndex, false);
  return InsertChild(aParent, aChild, nullptr, true);
}

NS_IMETHODIMP nsViewManager::RemoveChild(nsIView *aChild)
{
  nsIView* child = static_cast<nsIView*>(aChild);
  NS_ENSURE_ARG_POINTER(child);

  nsIView* parent = child->GetParent();

  if (nullptr != parent) {
    NS_ASSERTION(child->GetViewManager() == this ||
                 parent->GetViewManager() == this, "wrong view manager");
    child->GetViewManagerInternal()->InvalidateView(child);
    parent->RemoveChild(child);
  }

  return NS_OK;
}

NS_IMETHODIMP nsViewManager::MoveViewTo(nsIView *aView, nscoord aX, nscoord aY)
{
  nsIView* view = static_cast<nsIView*>(aView);
  NS_ASSERTION(view->GetViewManager() == this, "wrong view manager");
  nsPoint oldPt = view->GetPosition();
  nsRect oldBounds = view->GetBoundsInParentUnits();
  view->SetPosition(aX, aY);

  

  if ((aX != oldPt.x) || (aY != oldPt.y)) {
    if (view->GetVisibility() != nsViewVisibility_kHide) {
      nsIView* parentView = view->GetParent();
      if (parentView) {
        nsViewManager* parentVM = parentView->GetViewManagerInternal();
        parentVM->InvalidateView(parentView, oldBounds);
        parentVM->InvalidateView(parentView, view->GetBoundsInParentUnits());
      }
    }
  }
  return NS_OK;
}

void nsViewManager::InvalidateHorizontalBandDifference(nsIView *aView, const nsRect& aRect, const nsRect& aCutOut,
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

void nsViewManager::InvalidateRectDifference(nsIView *aView, const nsRect& aRect, const nsRect& aCutOut) {
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
  nsIView* view = static_cast<nsIView*>(aView);
  NS_ASSERTION(view->GetViewManager() == this, "wrong view manager");

  nsRect oldDimensions = view->GetDimensions();
  if (!oldDimensions.IsEqualEdges(aRect)) {
    
    
    if (view->GetVisibility() == nsViewVisibility_kHide) {  
      view->SetDimensions(aRect, false);
    } else {
      nsIView* parentView = view->GetParent();
      if (!parentView) {
        parentView = view;
      }
      nsRect oldBounds = view->GetBoundsInParentUnits();
      view->SetDimensions(aRect, true);
      nsViewManager* parentVM = parentView->GetViewManagerInternal();
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
  nsIView* view = static_cast<nsIView*>(aView);

  NS_ASSERTION(!(nullptr == view), "no view");

  view->SetFloating(aFloating);

  return NS_OK;
}

NS_IMETHODIMP nsViewManager::SetViewVisibility(nsIView *aView, nsViewVisibility aVisible)
{
  nsIView* view = static_cast<nsIView*>(aView);
  NS_ASSERTION(view->GetViewManager() == this, "wrong view manager");

  if (aVisible != view->GetVisibility()) {
    view->SetVisibility(aVisible);

    if (IsViewInserted(view)) {
      if (!view->HasWidget()) {
        if (nsViewVisibility_kHide == aVisible) {
          nsIView* parentView = view->GetParent();
          if (parentView) {
            parentView->GetViewManagerInternal()->
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

bool nsViewManager::IsViewInserted(nsIView *aView)
{
  if (mRootView == aView) {
    return true;
  } else if (aView->GetParent() == nullptr) {
    return false;
  } else {
    nsIView* view = aView->GetParent()->GetFirstChild();
    while (view != nullptr) {
      if (view == aView) {
        return true;
      }        
      view = view->GetNextSibling();
    }
    return false;
  }
}

NS_IMETHODIMP nsViewManager::SetViewZIndex(nsIView *aView, bool aAutoZIndex, int32_t aZIndex, bool aTopMost)
{
  nsIView* view = static_cast<nsIView*>(aView);
  nsresult  rv = NS_OK;

  NS_ASSERTION((view != nullptr), "no view");

  
  
  if (aView == mRootView) {
    return rv;
  }

  bool oldTopMost = view->IsTopMost();
  bool oldIsAuto = view->GetZIndexIsAuto();

  if (aAutoZIndex) {
    aZIndex = 0;
  }

  int32_t oldidx = view->GetZIndex();
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
    *aWidget = nullptr;
    return NS_OK;
  }
  if (mRootView->HasWidget()) {
    *aWidget = mRootView->GetWidget();
    NS_ADDREF(*aWidget);
    return NS_OK;
  }
  if (mRootView->GetParent())
    return mRootView->GetParent()->GetViewManagerInternal()->GetRootWidget(aWidget);
  *aWidget = nullptr;
  return NS_OK;
}

nsIntRect nsViewManager::ViewToWidget(nsIView *aView, const nsRect &aRect) const
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

  if (IsRefreshDriverPaintingEnabled()) {
    mPresShell->GetPresContext()->RefreshDriver()->RevokeViewManagerFlush();
      
    
    if (mPresShell) {
      CallWillPaintOnObservers(true);
    }
    ProcessPendingUpdatesForView(mRootView, true);
  } else {
    ProcessPendingUpdatesForView(mRootView, true);
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
    if (IsRefreshDriverPaintingEnabled()) {
      mHasPendingWidgetGeometryChanges = false;
    }
    ProcessPendingUpdatesForView(mRootView, false);
    if (!IsRefreshDriverPaintingEnabled()) {
      mHasPendingWidgetGeometryChanges = false;
    }
  }
}

void
nsViewManager::CallWillPaintOnObservers(bool aWillSendDidPaint)
{
  NS_PRECONDITION(IsRootVM(), "Must be root VM for this to be called!");

  int32_t index;
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

NS_IMETHODIMP
nsViewManager::GetLastUserEventTime(uint32_t& aTime)
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
    nsIView *parent = mRootView->GetParent();
    if (parent) {
      mRootViewManager = parent->GetViewManagerInternal()->RootViewManager();
      NS_ADDREF(mRootViewManager);
      NS_ASSERTION(mRootViewManager != this,
                   "Root view had a parent, but it has the same view manager");
    } else {
      mRootViewManager = this;
    }
  }
}
