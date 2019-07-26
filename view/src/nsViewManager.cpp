




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
#include "nsContentUtils.h"

















#define NSCOORD_NONE      INT32_MIN

#undef DEBUG_MOUSE_LOCATION

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



nsresult
nsViewManager::Init(nsDeviceContext* aContext)
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

nsView*
nsViewManager::CreateView(const nsRect& aBounds,
                          const nsView* aParent,
                          nsViewVisibility aVisibilityFlag)
{
  nsView *v = new nsView(this, aVisibilityFlag);
  if (v) {
    v->SetParent(const_cast<nsView*>(aParent));
    v->SetPosition(aBounds.x, aBounds.y);
    nsRect dim(0, 0, aBounds.width, aBounds.height);
    v->SetDimensions(dim, false);
  }
  return v;
}

void
nsViewManager::SetRootView(nsView *aView)
{
  NS_PRECONDITION(!aView || aView->GetViewManager() == this,
                  "Unexpected viewmanager on root view");
  
  
  
  mRootView = aView;

  if (mRootView) {
    nsView* parent = mRootView->GetParent();
    if (parent) {
      
      
      parent->InsertChild(mRootView, nullptr);
    } else {
      InvalidateHierarchy();
    }

    mRootView->SetZIndex(false, 0, false);
  }
  
}

void
nsViewManager::GetWindowDimensions(nscoord *aWidth, nscoord *aHeight)
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

void
nsViewManager::SetWindowDimensions(nscoord aWidth, nscoord aHeight)
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
}

void
nsViewManager::FlushDelayedResize(bool aDoReflow)
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

nsView* nsViewManager::GetDisplayRootFor(nsView* aView)
{
  nsView *displayRoot = aView;
  for (;;) {
    nsView *displayParent = displayRoot->GetParent();
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






void nsViewManager::Refresh(nsView *aView, const nsIntRegion& aRegion)
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
                        nsIPresShell::PAINT_COMPOSITE);
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
    nsIWidget *widget = aView->GetWidget();
    if (widget && widget->NeedsPaint()) {
      
      
      for (nsViewManager *vm = this; vm;
           vm = vm->mRootView->GetParent()
                  ? vm->mRootView->GetParent()->GetViewManager()
                  : nullptr) {
        if (vm->mDelayedResize != nsSize(NSCOORD_NONE, NSCOORD_NONE) &&
            vm->mRootView->IsEffectivelyVisible() &&
            mPresShell && mPresShell->IsVisible()) {
          vm->FlushDelayedResize(true);
          vm->InvalidateView(vm->mRootView);
        }
      }

      NS_ASSERTION(aView->HasWidget(), "Must have a widget!");

#ifdef DEBUG_INVALIDATIONS
      printf("---- PAINT START ----PresShell(%p), nsView(%p), nsIWidget(%p)\n", mPresShell, aView, widget);
#endif
      nsAutoScriptBlocker scriptBlocker;
      NS_ASSERTION(aView->HasWidget(), "Must have a widget!");
      aView->GetWidget()->WillPaint();
      SetPainting(true);
      mPresShell->Paint(aView, nsRegion(),
                        nsIPresShell::PAINT_LAYERS);
#ifdef DEBUG_INVALIDATIONS
      printf("---- PAINT END ----\n");
#endif
      aView->SetForcedRepaint(false);
      SetPainting(false);
    }
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

void
nsViewManager::InvalidateView(nsView *aView)
{
  
  InvalidateView(aView, aView->GetDimensions());
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
      nsView* view = nsView::GetViewFor(childWidget);
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
    nsView* view = aVM->GetRootView()->GetParent();
    aVM = view ? view->GetViewManager() : nullptr;
  }
  return false;
}

void
nsViewManager::InvalidateView(nsView *aView, const nsRect &aRect)
{
  
  
  if (ShouldIgnoreInvalidation(this)) {
    return;
  }

  InvalidateViewNoSuppression(aView, aRect);
}

void
nsViewManager::InvalidateViewNoSuppression(nsView *aView,
                                           const nsRect &aRect)
{
  NS_PRECONDITION(nullptr != aView, "null view");

  NS_ASSERTION(aView->GetViewManager() == this,
               "InvalidateViewNoSuppression called on view we don't own");

  nsRect damagedRect(aRect);
  if (damagedRect.IsEmpty()) {
    return;
  }

  nsView* displayRoot = GetDisplayRootFor(aView);
  nsViewManager* displayRootVM = displayRoot->GetViewManager();
  
  
  
  damagedRect.MoveBy(aView->GetOffsetTo(displayRoot));
  int32_t rootAPD = displayRootVM->AppUnitsPerDevPixel();
  int32_t APD = AppUnitsPerDevPixel();
  damagedRect = damagedRect.ConvertAppUnitsRoundOut(APD, rootAPD);

  
  
  AddDirtyRegion(displayRoot, nsRegion(damagedRect));
}

void
nsViewManager::InvalidateAllViews()
{
  if (RootViewManager() != this) {
    return RootViewManager()->InvalidateAllViews();
  }
  
  InvalidateViews(mRootView);
}

void nsViewManager::InvalidateViews(nsView *aView)
{
  
  InvalidateView(aView);

  
  nsView* childView = aView->GetFirstChild();
  while (nullptr != childView)  {
    childView->GetViewManager()->InvalidateViews(childView);
    childView = childView->GetNextSibling();
  }
}

void nsViewManager::WillPaintWindow(nsIWidget* aWidget)
{
  if (aWidget) {
    nsView* view = nsView::GetViewFor(aWidget);
    if (view && view->ForcedRepaint()) {
      ProcessPendingUpdates();
      
      
      view = nsView::GetViewFor(aWidget);
      if (view) {
        view->SetForcedRepaint(false);
      }
    }
  }

  nsCOMPtr<nsIPresShell> shell = mPresShell;
  if (shell) {
    shell->WillPaintWindow();
  }
}

bool nsViewManager::PaintWindow(nsIWidget* aWidget, nsIntRegion aRegion,
                                uint32_t aFlags)
{
  if (!aWidget || !mContext)
    return false;

  NS_ASSERTION(IsPaintingAllowed(),
               "shouldn't be receiving paint events while painting is disallowed!");

  
  
  nsView* view = nsView::GetViewFor(aWidget);
  if (view && !aRegion.IsEmpty()) {
    Refresh(view, aRegion);
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

void
nsViewManager::DispatchEvent(nsGUIEvent *aEvent, nsView* aView, nsEventStatus* aStatus)
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

  
  nsView* view = aView;
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
      shell->HandleEvent(frame, aEvent, false, aStatus);
	  return;
    }
  }

  *aStatus = nsEventStatus_eIgnore;
}



void nsViewManager::ReparentChildWidgets(nsView* aView, nsIWidget *aNewWidget)
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

  
  

  for (nsView *kid = aView->GetFirstChild(); kid; kid = kid->GetNextSibling()) {
    ReparentChildWidgets(kid, aNewWidget);
  }
}



void nsViewManager::ReparentWidgets(nsView* aView, nsView *aParent)
{
  NS_PRECONDITION(aParent, "Must have a parent");
  NS_PRECONDITION(aView, "Must have a view");
  
  
  
  
  
  
  
  
  if (aView->HasWidget() || aView->GetFirstChild()) {
    nsIWidget* parentWidget = aParent->GetNearestWidget(nullptr);
    if (parentWidget) {
      ReparentChildWidgets(aView, parentWidget);
      return;
    }
    NS_WARNING("Can not find a widget for the parent view");
  }
}

void
nsViewManager::InsertChild(nsView *aParent, nsView *aChild, nsView *aSibling,
                           bool aAfter)
{
  NS_PRECONDITION(nullptr != aParent, "null ptr");
  NS_PRECONDITION(nullptr != aChild, "null ptr");
  NS_ASSERTION(aSibling == nullptr || aSibling->GetParent() == aParent,
               "tried to insert view with invalid sibling");
  NS_ASSERTION(!IsViewInserted(aChild), "tried to insert an already-inserted view");

  if ((nullptr != aParent) && (nullptr != aChild))
    {
      
      

      if (nullptr == aSibling) {
        if (aAfter) {
          
          
          aParent->InsertChild(aChild, nullptr);
          ReparentWidgets(aChild, aParent);
        } else {
          
          nsView *kid = aParent->GetFirstChild();
          nsView *prev = nullptr;
          while (kid) {
            prev = kid;
            kid = kid->GetNextSibling();
          }
          
          aParent->InsertChild(aChild, prev);
          ReparentWidgets(aChild, aParent);
        }
      } else {
        nsView *kid = aParent->GetFirstChild();
        nsView *prev = nullptr;
        while (kid && aSibling != kid) {
          
          prev = kid;
          kid = kid->GetNextSibling();
        }
        NS_ASSERTION(kid != nullptr,
                     "couldn't find sibling in child list");
        if (aAfter) {
          
          aParent->InsertChild(aChild, prev);
          ReparentWidgets(aChild, aParent);
        } else {
          
          aParent->InsertChild(aChild, kid);
          ReparentWidgets(aChild, aParent);
        }
      }

      
      if (aParent->GetFloating())
        aChild->SetFloating(true);

      

      if (nsViewVisibility_kHide != aChild->GetVisibility())
        aChild->GetViewManager()->InvalidateView(aChild);
    }
}

void
nsViewManager::InsertChild(nsView *aParent, nsView *aChild, int32_t aZIndex)
{
  
  
  SetViewZIndex(aChild, false, aZIndex, false);
  InsertChild(aParent, aChild, nullptr, true);
}

void
nsViewManager::RemoveChild(nsView *aChild)
{
  NS_ASSERTION(aChild, "aChild must not be null");

  nsView* parent = aChild->GetParent();

  if (nullptr != parent) {
    NS_ASSERTION(aChild->GetViewManager() == this ||
                 parent->GetViewManager() == this, "wrong view manager");
    aChild->GetViewManager()->InvalidateView(aChild);
    parent->RemoveChild(aChild);
  }
}

void
nsViewManager::MoveViewTo(nsView *aView, nscoord aX, nscoord aY)
{
  NS_ASSERTION(aView->GetViewManager() == this, "wrong view manager");
  nsPoint oldPt = aView->GetPosition();
  nsRect oldBounds = aView->GetBoundsInParentUnits();
  aView->SetPosition(aX, aY);

  

  if ((aX != oldPt.x) || (aY != oldPt.y)) {
    if (aView->GetVisibility() != nsViewVisibility_kHide) {
      nsView* parentView = aView->GetParent();
      if (parentView) {
        nsViewManager* parentVM = parentView->GetViewManager();
        parentVM->InvalidateView(parentView, oldBounds);
        parentVM->InvalidateView(parentView, aView->GetBoundsInParentUnits());
      }
    }
  }
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

void
nsViewManager::ResizeView(nsView *aView, const nsRect &aRect, bool aRepaintExposedAreaOnly)
{
  NS_ASSERTION(aView->GetViewManager() == this, "wrong view manager");

  nsRect oldDimensions = aView->GetDimensions();
  if (!oldDimensions.IsEqualEdges(aRect)) {
    
    
    if (aView->GetVisibility() == nsViewVisibility_kHide) {
      aView->SetDimensions(aRect, false);
    } else {
      nsView* parentView = aView->GetParent();
      if (!parentView) {
        parentView = aView;
      }
      nsRect oldBounds = aView->GetBoundsInParentUnits();
      aView->SetDimensions(aRect, true);
      nsViewManager* parentVM = parentView->GetViewManager();
      if (!aRepaintExposedAreaOnly) {
        
        InvalidateView(aView, aRect);
        parentVM->InvalidateView(parentView, oldBounds);
      } else {
        InvalidateRectDifference(aView, aRect, oldDimensions);
        nsRect newBounds = aView->GetBoundsInParentUnits();
        parentVM->InvalidateRectDifference(parentView, oldBounds, newBounds);
      } 
    }
  }

  
  
  
  
  
}

void
nsViewManager::SetViewFloating(nsView *aView, bool aFloating)
{
  NS_ASSERTION(!(nullptr == aView), "no view");

  aView->SetFloating(aFloating);
}

void
nsViewManager::SetViewVisibility(nsView *aView, nsViewVisibility aVisible)
{
  NS_ASSERTION(aView->GetViewManager() == this, "wrong view manager");

  if (aVisible != aView->GetVisibility()) {
    aView->SetVisibility(aVisible);

    if (IsViewInserted(aView)) {
      if (!aView->HasWidget()) {
        if (nsViewVisibility_kHide == aVisible) {
          nsView* parentView = aView->GetParent();
          if (parentView) {
            parentView->GetViewManager()->
              InvalidateView(parentView, aView->GetBoundsInParentUnits());
          }
        }
        else {
          InvalidateView(aView);
        }
      }
    }
  }
}

bool nsViewManager::IsViewInserted(nsView *aView)
{
  if (mRootView == aView) {
    return true;
  } else if (aView->GetParent() == nullptr) {
    return false;
  } else {
    nsView* view = aView->GetParent()->GetFirstChild();
    while (view != nullptr) {
      if (view == aView) {
        return true;
      }
      view = view->GetNextSibling();
    }
    return false;
  }
}

void
nsViewManager::SetViewZIndex(nsView *aView, bool aAutoZIndex, int32_t aZIndex, bool aTopMost)
{
  NS_ASSERTION((aView != nullptr), "no view");

  
  
  if (aView == mRootView) {
    return;
  }

  bool oldTopMost = aView->IsTopMost();
  bool oldIsAuto = aView->GetZIndexIsAuto();

  if (aAutoZIndex) {
    aZIndex = 0;
  }

  int32_t oldidx = aView->GetZIndex();
  aView->SetZIndex(aAutoZIndex, aZIndex, aTopMost);

  if (oldidx != aZIndex || oldTopMost != aTopMost ||
      oldIsAuto != aAutoZIndex) {
    InvalidateView(aView);
  }
}

void
nsViewManager::GetDeviceContext(nsDeviceContext *&aContext)
{
  aContext = mContext;
  NS_IF_ADDREF(aContext);
}

nsViewManager*
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

void
nsViewManager::GetRootWidget(nsIWidget **aWidget)
{
  if (!mRootView) {
    *aWidget = nullptr;
    return;
  }
  if (mRootView->HasWidget()) {
    *aWidget = mRootView->GetWidget();
    NS_ADDREF(*aWidget);
    return;
  }
  if (mRootView->GetParent()) {
    mRootView->GetParent()->GetViewManager()->GetRootWidget(aWidget);
    return;
  }
  *aWidget = nullptr;
}

nsIntRect nsViewManager::ViewToWidget(nsView *aView, const nsRect &aRect) const
{
  NS_ASSERTION(aView->GetViewManager() == this, "wrong view manager");

  
  nsRect rect = aRect + aView->ViewToWidgetOffset();

  
  return rect.ToOutsidePixels(AppUnitsPerDevPixel());
}

void
nsViewManager::IsPainting(bool& aIsPainting)
{
  aIsPainting = IsPainting();
}

void
nsViewManager::ProcessPendingUpdates()
{
  if (!IsRootVM()) {
    RootViewManager()->ProcessPendingUpdates();
    return;
  }

  mPresShell->GetPresContext()->RefreshDriver()->RevokeViewManagerFlush();

  
  if (mPresShell) {
    CallWillPaintOnObservers();
  }
  ProcessPendingUpdatesForView(mRootView, true);
}

void
nsViewManager::UpdateWidgetGeometry()
{
  if (!IsRootVM()) {
    RootViewManager()->UpdateWidgetGeometry();
    return;
  }

  if (mHasPendingWidgetGeometryChanges) {
    mHasPendingWidgetGeometryChanges = false;
    ProcessPendingUpdatesForView(mRootView, false);
  }
}

void
nsViewManager::CallWillPaintOnObservers()
{
  NS_PRECONDITION(IsRootVM(), "Must be root VM for this to be called!");

  int32_t index;
  for (index = 0; index < mVMCount; index++) {
    nsViewManager* vm = (nsViewManager*)gViewManagers->ElementAt(index);
    if (vm->RootViewManager() == this) {
      
      if (vm->mRootView && vm->mRootView->IsEffectivelyVisible()) {
        nsCOMPtr<nsIPresShell> shell = vm->GetPresShell();
        if (shell) {
          shell->WillPaint();
        }
      }
    }
  }
}

void
nsViewManager::GetLastUserEventTime(uint32_t& aTime)
{
  aTime = gLastUserEventTime;
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
