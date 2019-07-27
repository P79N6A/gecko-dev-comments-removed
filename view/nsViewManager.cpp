




#define PL_ARENA_CONST_ALIGN_MASK (sizeof(void*)-1)
#include "plarena.h"

#include "nsAutoPtr.h"
#include "nsViewManager.h"
#include "nsGfxCIID.h"
#include "nsView.h"
#include "nsCOMPtr.h"
#include "mozilla/MouseEvents.h"
#include "nsRegion.h"
#include "nsCOMArray.h"
#include "nsIPluginWidget.h"
#include "nsXULPopupManager.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "mozilla/StartupTimeline.h"
#include "GeckoProfiler.h"
#include "nsRefreshDriver.h"
#include "mozilla/Preferences.h"
#include "nsContentUtils.h" 
#include "nsLayoutUtils.h"
#include "Layers.h"
#include "gfxPlatform.h"
#include "gfxPrefs.h"
#include "nsIDocument.h"

















using namespace mozilla;
using namespace mozilla::layers;

#define NSCOORD_NONE      INT32_MIN

#undef DEBUG_MOUSE_LOCATION


nsTArray<nsViewManager*>* nsViewManager::gViewManagers = nullptr;
uint32_t nsViewManager::gLastUserEventTime = 0;

nsViewManager::nsViewManager()
  : mDelayedResize(NSCOORD_NONE, NSCOORD_NONE)
{
  mRootViewManager = this;
  if (gViewManagers == nullptr) {
    
    gViewManagers = new nsTArray<nsViewManager*>;
  }
 
  gViewManagers->AppendElement(this);

  
  
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

  NS_ASSERTION(gViewManagers != nullptr, "About to use null gViewManagers");

#ifdef DEBUG
  bool removed =
#endif
    gViewManagers->RemoveElement(this);
  NS_ASSERTION(removed, "Viewmanager instance was not in the global list of viewmanagers");

  if (gViewManagers->IsEmpty()) {
    
    
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
                          nsView* aParent,
                          nsViewVisibility aVisibilityFlag)
{
  nsView *v = new nsView(this, aVisibilityFlag);
  v->SetParent(aParent);
  v->SetPosition(aBounds.x, aBounds.y);
  nsRect dim(0, 0, aBounds.width, aBounds.height);
  v->SetDimensions(dim, false);
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

    mRootView->SetZIndex(false, 0);
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
    } else if (mPresShell && !mPresShell->GetIsViewportOverridden()) {
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
  out = out.ScaleToOtherAppUnitsRoundOut(
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
    if (widget && widget->WindowType() == eWindowType_popup) {
      NS_ASSERTION(displayRoot->GetFloating() && displayParent->GetFloating(),
        "this should only happen with floating views that have floating parents");
      return displayRoot;
    }

    displayRoot = displayParent;
  }
}






void nsViewManager::Refresh(nsView *aView, const nsIntRegion& aRegion)
{
  NS_ASSERTION(aView->GetViewManager() == this, "wrong view manager");

  if (mPresShell && mPresShell->IsNeverPainting()) {
    return;
  }

  
  nsRegion damageRegion = aRegion.ToAppUnits(AppUnitsPerDevPixel());
  
  damageRegion.MoveBy(-aView->ViewToWidgetOffset());

  if (damageRegion.IsEmpty()) {
#ifdef DEBUG_roc
    nsRect viewRect = aView->GetDimensions();
    nsRect damageRect = damageRegion.GetBounds();
    printf_stderr("XXX Damage rectangle (%d,%d,%d,%d) does not intersect the widget's view (%d,%d,%d,%d)!\n",
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
#ifdef MOZ_DUMP_PAINTING
      if (nsLayoutUtils::InvalidationDebuggingIsEnabled()) {
        printf_stderr("--COMPOSITE-- %p\n", mPresShell);
      }
#endif
      uint32_t paintFlags = nsIPresShell::PAINT_COMPOSITE;
      LayerManager *manager = widget->GetLayerManager();
      if (!manager->NeedsWidgetInvalidation()) {
        manager->FlushRendering();
      } else {
        mPresShell->Paint(aView, damageRegion,
                          paintFlags);
      }
#ifdef MOZ_DUMP_PAINTING
      if (nsLayoutUtils::InvalidationDebuggingIsEnabled()) {
        printf_stderr("--ENDCOMPOSITE--\n");
      }
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

void
nsViewManager::ProcessPendingUpdatesForView(nsView* aView,
                                            bool aFlushDirtyRegion)
{
  NS_ASSERTION(IsRootVM(), "Updates will be missed");
  if (!aView) {
    return;
  }

  nsCOMPtr<nsIPresShell> rootShell(mPresShell);
  nsTArray<nsCOMPtr<nsIWidget> > widgets;
  aView->GetViewManager()->ProcessPendingUpdatesRecurse(aView, widgets);
  for (uint32_t i = 0; i < widgets.Length(); ++i) {
    nsView* view = nsView::GetViewFor(widgets[i]);
    if (view) {
      view->ResetWidgetBounds(false, true);
    }
  }
  if (rootShell->GetViewManager() != this) {
    return; 
  }
  if (aFlushDirtyRegion) {
    profiler_tracing("Paint", "DisplayList", TRACING_INTERVAL_START);
    nsAutoScriptBlocker scriptBlocker;
    SetPainting(true);
    for (uint32_t i = 0; i < widgets.Length(); ++i) {
      nsIWidget* widget = widgets[i];
      nsView* view = nsView::GetViewFor(widget);
      if (view) {
        view->GetViewManager()->ProcessPendingUpdatesPaint(widget);
      }
    }
    SetPainting(false);
    profiler_tracing("Paint", "DisplayList", TRACING_INTERVAL_END);
  }
}

void
nsViewManager::ProcessPendingUpdatesRecurse(nsView* aView,
                                            nsTArray<nsCOMPtr<nsIWidget> >& aWidgets)
{
  if (mPresShell && mPresShell->IsNeverPainting()) {
    return;
  }

  for (nsView* childView = aView->GetFirstChild(); childView;
       childView = childView->GetNextSibling()) {
    childView->GetViewManager()->
      ProcessPendingUpdatesRecurse(childView, aWidgets);
  }

  nsIWidget* widget = aView->GetWidget();
  if (widget) {
    aWidgets.AppendElement(widget);
  } else {
    FlushDirtyRegionToWidget(aView);
  }
}

void
nsViewManager::ProcessPendingUpdatesPaint(nsIWidget* aWidget)
{
  if (aWidget->NeedsPaint()) {
    
    
    for (nsViewManager *vm = this; vm;
         vm = vm->mRootView->GetParent()
           ? vm->mRootView->GetParent()->GetViewManager()
           : nullptr) {
      if (vm->mDelayedResize != nsSize(NSCOORD_NONE, NSCOORD_NONE) &&
          vm->mRootView->IsEffectivelyVisible() &&
          vm->mPresShell && vm->mPresShell->IsVisible()) {
        vm->FlushDelayedResize(true);
      }
    }
    nsView* view = nsView::GetViewFor(aWidget);
    if (!view) {
      NS_ERROR("FlushDelayedResize destroyed the nsView?");
      return;
    }

    if (mPresShell) {
#ifdef MOZ_DUMP_PAINTING
      if (nsLayoutUtils::InvalidationDebuggingIsEnabled()) {
        printf_stderr("---- PAINT START ----PresShell(%p), nsView(%p), nsIWidget(%p)\n",
                      mPresShell, view, aWidget);
      }
#endif

      mPresShell->Paint(view, nsRegion(), nsIPresShell::PAINT_LAYERS);
      view->SetForcedRepaint(false);

#ifdef MOZ_DUMP_PAINTING
      if (nsLayoutUtils::InvalidationDebuggingIsEnabled()) {
        printf_stderr("---- PAINT END ----\n");
      }
#endif
    }
  }
  FlushDirtyRegionToWidget(nsView::GetViewFor(aWidget));
}

void nsViewManager::FlushDirtyRegionToWidget(nsView* aView)
{
  NS_ASSERTION(aView->GetViewManager() == this,
               "FlushDirtyRegionToWidget called on view we don't own");

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

  
  
  if (gfxPrefs::DrawFrameCounter()) {
    nsRect counterBounds = gfxPlatform::FrameCounterBounds().ToAppUnits(AppUnitsPerDevPixel());
    r = r.Or(r, counterBounds);
  }

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
      nsWindowType type = childWidget->WindowType();
      if (view && childWidget->IsVisible() && type != eWindowType_popup) {
        NS_ASSERTION(childWidget->IsPlugin(),
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
  damagedRect = damagedRect.ScaleToOtherAppUnitsRoundOut(APD, rootAPD);

  
  
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
    LayerManager *manager = aWidget->GetLayerManager();
    if (view &&
        (view->ForcedRepaint() || !manager->NeedsWidgetInvalidation())) {
      ProcessPendingUpdates(eNoSyncUpdate);
      
      
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

bool nsViewManager::PaintWindow(nsIWidget* aWidget, nsIntRegion aRegion)
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
nsViewManager::DispatchEvent(WidgetGUIEvent *aEvent,
                             nsView* aView,
                             nsEventStatus* aStatus)
{
  PROFILER_LABEL("nsViewManager", "DispatchEvent",
    js::ProfileEntry::Category::EVENTS);

  WidgetMouseEvent* mouseEvent = aEvent->AsMouseEvent();
  if ((mouseEvent &&
       
       mouseEvent->reason == WidgetMouseEvent::eReal &&
       
       
       
       mouseEvent->message != NS_MOUSE_EXIT &&
       mouseEvent->message != NS_MOUSE_ENTER) ||
      aEvent->HasKeyEventMessage() ||
      aEvent->HasIMEEventMessage() ||
      aEvent->message == NS_PLUGIN_INPUT_EVENT) {
    gLastUserEventTime = PR_IntervalToMicroseconds(PR_IntervalNow());
  }

  
  nsView* view = aView;
  bool dispatchUsingCoordinates = aEvent->IsUsingCoordinates();
  if (dispatchUsingCoordinates) {
    
    
    view = GetDisplayRootFor(view);
  }

  
  nsIFrame* frame = view->GetFrame();
  if (!frame &&
      (dispatchUsingCoordinates || aEvent->HasKeyEventMessage() ||
       aEvent->IsIMERelatedEvent() ||
       aEvent->IsNonRetargetedNativeEventDelivererForPlugin() ||
       aEvent->HasPluginActivationEventMessage())) {
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
    }
}

void
nsViewManager::InsertChild(nsView *aParent, nsView *aChild, int32_t aZIndex)
{
  
  
  SetViewZIndex(aChild, false, aZIndex);
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
    parent->RemoveChild(aChild);
  }
}

void
nsViewManager::MoveViewTo(nsView *aView, nscoord aX, nscoord aY)
{
  NS_ASSERTION(aView->GetViewManager() == this, "wrong view manager");
  aView->SetPosition(aX, aY);
}

void
nsViewManager::ResizeView(nsView *aView, const nsRect &aRect, bool aRepaintExposedAreaOnly)
{
  NS_ASSERTION(aView->GetViewManager() == this, "wrong view manager");

  nsRect oldDimensions = aView->GetDimensions();
  if (!oldDimensions.IsEqualEdges(aRect)) {
    aView->SetDimensions(aRect, true);
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
nsViewManager::SetViewZIndex(nsView *aView, bool aAutoZIndex, int32_t aZIndex)
{
  NS_ASSERTION((aView != nullptr), "no view");

  
  
  if (aView == mRootView) {
    return;
  }

  if (aAutoZIndex) {
    aZIndex = 0;
  }

  aView->SetZIndex(aAutoZIndex, aZIndex);
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
nsViewManager::ProcessPendingUpdates(UpdatingMode aMode)
{
  if (!IsRootVM()) {
    RootViewManager()->ProcessPendingUpdates(aMode);
    return;
  }

  
  if (mPresShell) {
    mPresShell->GetPresContext()->RefreshDriver()->RevokeViewManagerFlush();

    CallWillPaintOnObservers();

    ProcessPendingUpdatesForView(mRootView, true);
  }

  if (aMode == eTrySyncUpdate) {
    nsCOMPtr<nsIWidget> w;
    GetRootWidget(getter_AddRefs(w));
    if (w) {
      w->Update();
    }
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
    mHasPendingWidgetGeometryChanges = false;
    ProcessPendingUpdatesForView(mRootView, false);
  }
}

void
nsViewManager::CallWillPaintOnObservers()
{
  NS_PRECONDITION(IsRootVM(), "Must be root VM for this to be called!");

  if (NS_WARN_IF(!gViewManagers)) {
    return;
  }

  uint32_t index;
  for (index = 0; index < gViewManagers->Length(); index++) {
    nsViewManager* vm = gViewManagers->ElementAt(index);
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
