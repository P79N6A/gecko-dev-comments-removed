




#include "nsView.h"

#include "mozilla/Attributes.h"
#include "mozilla/BasicEvents.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/IntegerPrintfMacros.h"
#include "mozilla/Likely.h"
#include "mozilla/Poison.h"
#include "nsIWidget.h"
#include "nsViewManager.h"
#include "nsIFrame.h"
#include "nsPresArena.h"
#include "nsXULPopupManager.h"
#include "nsIWidgetListener.h"
#include "nsContentUtils.h" 

using namespace mozilla;

nsView::nsView(nsViewManager* aViewManager, nsViewVisibility aVisibility)
{
  MOZ_COUNT_CTOR(nsView);

  mVis = aVisibility;
  
  
  
  
  mVFlags = 0;
  mViewManager = aViewManager;
  mDirtyRegion = nullptr;
  mWidgetIsTopLevel = false;
}

void nsView::DropMouseGrabbing()
{
  nsIPresShell* presShell = mViewManager->GetPresShell();
  if (presShell)
    presShell->ClearMouseCaptureOnView(this);
}

nsView::~nsView()
{
  MOZ_COUNT_DTOR(nsView);

  while (GetFirstChild())
  {
    nsView* child = GetFirstChild();
    if (child->GetViewManager() == mViewManager) {
      child->Destroy();
    } else {
      
      RemoveChild(child);
    }
  }

  if (mViewManager)
  {
    DropMouseGrabbing();
  
    nsView *rootView = mViewManager->GetRootView();
    
    if (rootView)
    {
      
      if (mParent)
      {
        mViewManager->RemoveChild(this);
      }

      if (rootView == this)
      {
        
        mViewManager->SetRootView(nullptr);
      }
    }
    else if (mParent)
    {
      mParent->RemoveChild(this);
    }
    
    mViewManager = nullptr;
  }
  else if (mParent)
  {
    mParent->RemoveChild(this);
  }

  
  DestroyWidget();

  delete mDirtyRegion;
}

class DestroyWidgetRunnable : public nsRunnable {
public:
  NS_DECL_NSIRUNNABLE

  explicit DestroyWidgetRunnable(nsIWidget* aWidget) : mWidget(aWidget) {}

private:
  nsCOMPtr<nsIWidget> mWidget;
};

NS_IMETHODIMP DestroyWidgetRunnable::Run()
{
  mWidget->Destroy();
  mWidget = nullptr;
  return NS_OK;
}


void nsView::DestroyWidget()
{
  if (mWindow)
  {
    
    
    
    
    if (mWidgetIsTopLevel) {
      mWindow->SetAttachedWidgetListener(nullptr);
    }
    else {
      mWindow->SetWidgetListener(nullptr);

      nsCOMPtr<nsIRunnable> widgetDestroyer =
        new DestroyWidgetRunnable(mWindow);

      NS_DispatchToMainThread(widgetDestroyer);
    }

    mWindow = nullptr;
  }
}

nsView* nsView::GetViewFor(nsIWidget* aWidget)
{
  NS_PRECONDITION(nullptr != aWidget, "null widget ptr");

  nsIWidgetListener* listener = aWidget->GetWidgetListener();
  if (listener) {
    nsView* view = listener->GetView();
    if (view)
      return view;
  }

  listener = aWidget->GetAttachedWidgetListener();
  return listener ? listener->GetView() : nullptr;
}

void nsView::Destroy()
{
  this->~nsView();
  mozWritePoison(this, sizeof(*this));
  nsView::operator delete(this);
}

void nsView::SetPosition(nscoord aX, nscoord aY)
{
  mDimBounds.x += aX - mPosX;
  mDimBounds.y += aY - mPosY;
  mPosX = aX;
  mPosY = aY;

  NS_ASSERTION(GetParent() || (aX == 0 && aY == 0),
               "Don't try to move the root widget to something non-zero");

  ResetWidgetBounds(true, false);
}

void nsView::ResetWidgetBounds(bool aRecurse, bool aForceSync)
{
  if (mWindow) {
    if (!aForceSync) {
      
      
      mViewManager->PostPendingUpdate();
    } else {
      DoResetWidgetBounds(false, true);
    }
    return;
  }

  if (aRecurse) {
    
    for (nsView* v = GetFirstChild(); v; v = v->GetNextSibling()) {
      v->ResetWidgetBounds(true, aForceSync);
    }
  }
}

bool nsView::IsEffectivelyVisible()
{
  for (nsView* v = this; v; v = v->mParent) {
    if (v->GetVisibility() == nsViewVisibility_kHide)
      return false;
  }
  return true;
}

nsIntRect nsView::CalcWidgetBounds(nsWindowType aType)
{
  int32_t p2a = mViewManager->AppUnitsPerDevPixel();

  nsRect viewBounds(mDimBounds);

  nsView* parent = GetParent();
  nsIWidget* parentWidget = nullptr;
  if (parent) {
    nsPoint offset;
    parentWidget = parent->GetNearestWidget(&offset, p2a);
    
    viewBounds += offset;

    if (parentWidget && aType == eWindowType_popup &&
        IsEffectivelyVisible()) {
      
      LayoutDeviceIntPoint screenPoint = parentWidget->WidgetToScreenOffset();
      viewBounds += nsPoint(NSIntPixelsToAppUnits(screenPoint.x, p2a),
                            NSIntPixelsToAppUnits(screenPoint.y, p2a));
    }
  }

  
  nsIntRect newBounds = viewBounds.ToNearestPixels(p2a);

#ifdef XP_MACOSX
  
  
  
  nsIWidget* widget = parentWidget ? parentWidget : mWindow.get();
  uint32_t round;
  if (aType == eWindowType_popup && widget &&
      ((round = widget->RoundsWidgetCoordinatesTo()) > 1)) {
    nsIntSize pixelRoundedSize = newBounds.Size();
    
    newBounds.x = NSToIntRoundUp(NSAppUnitsToDoublePixels(viewBounds.x, p2a) / round) * round;
    newBounds.y = NSToIntRoundUp(NSAppUnitsToDoublePixels(viewBounds.y, p2a) / round) * round;
    newBounds.width =
      NSToIntRoundUp(NSAppUnitsToDoublePixels(viewBounds.XMost(), p2a) / round) * round - newBounds.x;
    newBounds.height =
      NSToIntRoundUp(NSAppUnitsToDoublePixels(viewBounds.YMost(), p2a) / round) * round - newBounds.y;
    
    
    if (newBounds.width > pixelRoundedSize.width) {
      newBounds.width -= round;
    }
    if (newBounds.height > pixelRoundedSize.height) {
      newBounds.height -= round;
    }
  }
#endif

  
  
  nsPoint roundedOffset(NSIntPixelsToAppUnits(newBounds.x, p2a),
                        NSIntPixelsToAppUnits(newBounds.y, p2a));

  
  
  
  
  
  mViewToWidgetOffset = nsPoint(mPosX, mPosY)
    - mDimBounds.TopLeft() + viewBounds.TopLeft() - roundedOffset;

  return newBounds;
}

void nsView::DoResetWidgetBounds(bool aMoveOnly,
                                 bool aInvalidateChangedSize) {
  
  
  if (mViewManager->GetRootView() == this) {
    return;
  }

  NS_PRECONDITION(mWindow, "Why was this called??");

  
  nsCOMPtr<nsIWidget> widget = mWindow;

  
  
  nsIntRect newBounds;
  nsRefPtr<nsDeviceContext> dx = mViewManager->GetDeviceContext();

  nsWindowType type = widget->WindowType();

  nsIntRect curBounds;
  widget->GetClientBounds(curBounds);
  bool invisiblePopup = type == eWindowType_popup &&
                        ((curBounds.IsEmpty() && mDimBounds.IsEmpty()) ||
                         mVis == nsViewVisibility_kHide);

  if (invisiblePopup) {
    
  } else {
    newBounds = CalcWidgetBounds(type);
  }

  bool curVisibility = widget->IsVisible();
  bool newVisibility = IsEffectivelyVisible();
  if (curVisibility && !newVisibility) {
    widget->Show(false);
  }

  if (invisiblePopup) {
    
    
    
    
    
    return;
  }

  bool changedPos = curBounds.TopLeft() != newBounds.TopLeft();
  bool changedSize = curBounds.Size() != newBounds.Size();

  

  
  
  
  
  double invScale;

  
  
  
  
  
  
  
  
  
  
  CSSToLayoutDeviceScale scale = widget->GetDefaultScale();
  if (NSToIntRound(60.0 / scale.scale) == dx->AppUnitsPerDevPixelAtUnitFullZoom()) {
    invScale = 1.0 / scale.scale;
  } else {
    invScale = dx->AppUnitsPerDevPixelAtUnitFullZoom() / 60.0;
  }

  if (changedPos) {
    if (changedSize && !aMoveOnly) {
      widget->ResizeClient(newBounds.x * invScale,
                           newBounds.y * invScale,
                           newBounds.width * invScale,
                           newBounds.height * invScale,
                           aInvalidateChangedSize);
    } else {
      widget->MoveClient(newBounds.x * invScale,
                         newBounds.y * invScale);
    }
  } else {
    if (changedSize && !aMoveOnly) {
      widget->ResizeClient(newBounds.width * invScale,
                           newBounds.height * invScale,
                           aInvalidateChangedSize);
    } 
  }

  if (!curVisibility && newVisibility) {
    widget->Show(true);
  }
}

void nsView::SetDimensions(const nsRect& aRect, bool aPaint, bool aResizeWidget)
{
  nsRect dims = aRect;
  dims.MoveBy(mPosX, mPosY);

  
  
  
  if (mDimBounds.TopLeft() == dims.TopLeft() &&
      mDimBounds.Size() == dims.Size()) {
    return;
  }

  mDimBounds = dims;

  if (aResizeWidget) {
    ResetWidgetBounds(false, false);
  }
}

void nsView::NotifyEffectiveVisibilityChanged(bool aEffectivelyVisible)
{
  if (!aEffectivelyVisible)
  {
    DropMouseGrabbing();
  }

  SetForcedRepaint(true);

  if (nullptr != mWindow)
  {
    ResetWidgetBounds(false, false);
  }

  for (nsView* child = mFirstChild; child; child = child->mNextSibling) {
    if (child->mVis == nsViewVisibility_kHide) {
      
      continue;
    }
    
    child->NotifyEffectiveVisibilityChanged(aEffectivelyVisible);
  }
}

void nsView::SetVisibility(nsViewVisibility aVisibility)
{
  mVis = aVisibility;
  NotifyEffectiveVisibilityChanged(IsEffectivelyVisible());
}

void nsView::SetFloating(bool aFloatingView)
{
	if (aFloatingView)
		mVFlags |= NS_VIEW_FLAG_FLOATING;
	else
		mVFlags &= ~NS_VIEW_FLAG_FLOATING;
}

void nsView::InvalidateHierarchy(nsViewManager *aViewManagerParent)
{
  if (mViewManager->GetRootView() == this)
    mViewManager->InvalidateHierarchy();

  for (nsView *child = mFirstChild; child; child = child->GetNextSibling())
    child->InvalidateHierarchy(aViewManagerParent);
}

void nsView::InsertChild(nsView *aChild, nsView *aSibling)
{
  NS_PRECONDITION(nullptr != aChild, "null ptr");

  if (nullptr != aChild)
  {
    if (nullptr != aSibling)
    {
#ifdef DEBUG
      NS_ASSERTION(aSibling->GetParent() == this, "tried to insert view with invalid sibling");
#endif
      
      aChild->SetNextSibling(aSibling->GetNextSibling());
      aSibling->SetNextSibling(aChild);
    }
    else
    {
      aChild->SetNextSibling(mFirstChild);
      mFirstChild = aChild;
    }
    aChild->SetParent(this);

    
    

    nsViewManager *vm = aChild->GetViewManager();
    if (vm->GetRootView() == aChild)
    {
      aChild->InvalidateHierarchy(nullptr); 
    }
  }
}

void nsView::RemoveChild(nsView *child)
{
  NS_PRECONDITION(nullptr != child, "null ptr");

  if (nullptr != child)
  {
    nsView* prevKid = nullptr;
    nsView* kid = mFirstChild;
    DebugOnly<bool> found = false;
    while (nullptr != kid) {
      if (kid == child) {
        if (nullptr != prevKid) {
          prevKid->SetNextSibling(kid->GetNextSibling());
        } else {
          mFirstChild = kid->GetNextSibling();
        }
        child->SetParent(nullptr);
        found = true;
        break;
      }
      prevKid = kid;
	    kid = kid->GetNextSibling();
    }
    NS_ASSERTION(found, "tried to remove non child");

    
    

    nsViewManager *vm = child->GetViewManager();
    if (vm->GetRootView() == child)
    {
      child->InvalidateHierarchy(GetViewManager());
    }
  }
}






static void UpdateNativeWidgetZIndexes(nsView* aView, int32_t aZIndex)
{
  if (aView->HasWidget()) {
    nsIWidget* widget = aView->GetWidget();
    if (widget->GetZIndex() != aZIndex) {
      widget->SetZIndex(aZIndex);
    }
  } else {
    for (nsView* v = aView->GetFirstChild(); v; v = v->GetNextSibling()) {
      if (v->GetZIndexIsAuto()) {
        UpdateNativeWidgetZIndexes(v, aZIndex);
      }
    }
  }
}

static int32_t FindNonAutoZIndex(nsView* aView)
{
  while (aView) {
    if (!aView->GetZIndexIsAuto()) {
      return aView->GetZIndex();
    }
    aView = aView->GetParent();
  }
  return 0;
}

struct DefaultWidgetInitData : public nsWidgetInitData {
  DefaultWidgetInitData() : nsWidgetInitData()
  {
    mWindowType = eWindowType_child;
    clipChildren = true;
    clipSiblings = true;
  }
};

nsresult nsView::CreateWidget(nsWidgetInitData *aWidgetInitData,
                               bool aEnableDragDrop,
                               bool aResetVisibility)
{
  AssertNoWindow();
  MOZ_ASSERT(!aWidgetInitData ||
             aWidgetInitData->mWindowType != eWindowType_popup,
             "Use CreateWidgetForPopup");

  DefaultWidgetInitData defaultInitData;
  bool initDataPassedIn = !!aWidgetInitData;
  aWidgetInitData = aWidgetInitData ? aWidgetInitData : &defaultInitData;
  defaultInitData.mListenForResizes =
    (!initDataPassedIn && GetParent() &&
     GetParent()->GetViewManager() != mViewManager);

  nsIntRect trect = CalcWidgetBounds(aWidgetInitData->mWindowType);

  nsIWidget* parentWidget =
    GetParent() ? GetParent()->GetNearestWidget(nullptr) : nullptr;
  if (!parentWidget) {
    NS_ERROR("nsView::CreateWidget without suitable parent widget??");
    return NS_ERROR_FAILURE;
  }

  
  
  mWindow = parentWidget->CreateChild(trect, aWidgetInitData, true);
  if (!mWindow) {
    return NS_ERROR_FAILURE;
  }
 
  InitializeWindow(aEnableDragDrop, aResetVisibility);

  return NS_OK;
}

nsresult nsView::CreateWidgetForParent(nsIWidget* aParentWidget,
                                        nsWidgetInitData *aWidgetInitData,
                                        bool aEnableDragDrop,
                                        bool aResetVisibility)
{
  AssertNoWindow();
  MOZ_ASSERT(!aWidgetInitData ||
             aWidgetInitData->mWindowType != eWindowType_popup,
             "Use CreateWidgetForPopup");
  MOZ_ASSERT(aParentWidget, "Parent widget required");

  DefaultWidgetInitData defaultInitData;
  aWidgetInitData = aWidgetInitData ? aWidgetInitData : &defaultInitData;

  nsIntRect trect = CalcWidgetBounds(aWidgetInitData->mWindowType);

  mWindow = aParentWidget->CreateChild(trect, aWidgetInitData);
  if (!mWindow) {
    return NS_ERROR_FAILURE;
  }

  InitializeWindow(aEnableDragDrop, aResetVisibility);

  return NS_OK;
}

nsresult nsView::CreateWidgetForPopup(nsWidgetInitData *aWidgetInitData,
                                       nsIWidget* aParentWidget,
                                       bool aEnableDragDrop,
                                       bool aResetVisibility)
{
  AssertNoWindow();
  MOZ_ASSERT(aWidgetInitData, "Widget init data required");
  MOZ_ASSERT(aWidgetInitData->mWindowType == eWindowType_popup,
             "Use one of the other CreateWidget methods");

  nsIntRect trect = CalcWidgetBounds(aWidgetInitData->mWindowType);

  
  
  
  
  if (aParentWidget) {
    
    
    mWindow = aParentWidget->CreateChild(trect, aWidgetInitData, true);
  }
  else {
    nsIWidget* nearestParent = GetParent() ? GetParent()->GetNearestWidget(nullptr)
                                           : nullptr;
    if (!nearestParent) {
      
      
      return NS_ERROR_FAILURE;
    }

    mWindow = nearestParent->CreateChild(trect, aWidgetInitData);
  }
  if (!mWindow) {
    return NS_ERROR_FAILURE;
  }

  InitializeWindow(aEnableDragDrop, aResetVisibility);

  return NS_OK;
}

void
nsView::InitializeWindow(bool aEnableDragDrop, bool aResetVisibility)
{
  MOZ_ASSERT(mWindow, "Must have a window to initialize");

  mWindow->SetWidgetListener(this);

  if (aEnableDragDrop) {
    mWindow->EnableDragDrop(true);
  }
      
  
  UpdateNativeWidgetZIndexes(this, FindNonAutoZIndex(this));

  

  if (aResetVisibility) {
    SetVisibility(GetVisibility());
  }
}


nsresult nsView::AttachToTopLevelWidget(nsIWidget* aWidget)
{
  NS_PRECONDITION(nullptr != aWidget, "null widget ptr");
  
  
  nsIWidgetListener* listener = aWidget->GetAttachedWidgetListener();
  if (listener) {
    nsView *oldView = listener->GetView();
    if (oldView) {
      oldView->DetachFromTopLevelWidget();
    }
  }

  
  
  nsresult rv = aWidget->AttachViewToTopLevel(!nsIWidget::UsePuppetWidgets());
  if (NS_FAILED(rv))
    return rv;

  mWindow = aWidget;

  mWindow->SetAttachedWidgetListener(this);
  mWindow->EnableDragDrop(true);
  mWidgetIsTopLevel = true;

  
  CalcWidgetBounds(mWindow->WindowType());

  return NS_OK;
}


nsresult nsView::DetachFromTopLevelWidget()
{
  NS_PRECONDITION(mWidgetIsTopLevel, "Not attached currently!");
  NS_PRECONDITION(mWindow, "null mWindow for DetachFromTopLevelWidget!");

  mWindow->SetAttachedWidgetListener(nullptr);
  mWindow = nullptr;

  mWidgetIsTopLevel = false;
  
  return NS_OK;
}

void nsView::SetZIndex(bool aAuto, int32_t aZIndex)
{
  bool oldIsAuto = GetZIndexIsAuto();
  mVFlags = (mVFlags & ~NS_VIEW_FLAG_AUTO_ZINDEX) | (aAuto ? NS_VIEW_FLAG_AUTO_ZINDEX : 0);
  mZIndex = aZIndex;
  
  if (HasWidget() || !oldIsAuto || !aAuto) {
    UpdateNativeWidgetZIndexes(this, FindNonAutoZIndex(this));
  }
}

void nsView::AssertNoWindow()
{
  
  if (MOZ_UNLIKELY(mWindow)) {
    NS_ERROR("We already have a window for this view? BAD");
    mWindow->SetWidgetListener(nullptr);
    mWindow->Destroy();
    mWindow = nullptr;
  }
}




void nsView::AttachWidgetEventHandler(nsIWidget* aWidget)
{
#ifdef DEBUG
  NS_ASSERTION(!aWidget->GetWidgetListener(), "Already have a widget listener");
#endif

  aWidget->SetWidgetListener(this);
}

void nsView::DetachWidgetEventHandler(nsIWidget* aWidget)
{
  NS_ASSERTION(!aWidget->GetWidgetListener() ||
               aWidget->GetWidgetListener()->GetView() == this, "Wrong view");
  aWidget->SetWidgetListener(nullptr);
}

#ifdef DEBUG
void nsView::List(FILE* out, int32_t aIndent) const
{
  int32_t i;
  for (i = aIndent; --i >= 0; ) fputs("  ", out);
  fprintf(out, "%p ", (void*)this);
  if (nullptr != mWindow) {
    nscoord p2a = mViewManager->AppUnitsPerDevPixel();
    nsIntRect rect;
    mWindow->GetClientBounds(rect);
    nsRect windowBounds = rect.ToAppUnits(p2a);
    mWindow->GetBounds(rect);
    nsRect nonclientBounds = rect.ToAppUnits(p2a);
    nsrefcnt widgetRefCnt = mWindow.get()->AddRef() - 1;
    mWindow.get()->Release();
    int32_t Z = mWindow->GetZIndex();
    fprintf(out, "(widget=%p[%" PRIuPTR "] z=%d pos={%d,%d,%d,%d}) ",
            (void*)mWindow, widgetRefCnt, Z,
            nonclientBounds.x, nonclientBounds.y,
            windowBounds.width, windowBounds.height);
  }
  nsRect brect = GetBounds();
  fprintf(out, "{%d,%d,%d,%d}",
          brect.x, brect.y, brect.width, brect.height);
  fprintf(out, " z=%d vis=%d frame=%p <\n",
          mZIndex, mVis, static_cast<void*>(mFrame));
  for (nsView* kid = mFirstChild; kid; kid = kid->GetNextSibling()) {
    NS_ASSERTION(kid->GetParent() == this, "incorrect parent");
    kid->List(out, aIndent + 1);
  }
  for (i = aIndent; --i >= 0; ) fputs("  ", out);
  fputs(">\n", out);
}
#endif 

nsPoint nsView::GetOffsetTo(const nsView* aOther) const
{
  return GetOffsetTo(aOther, GetViewManager()->AppUnitsPerDevPixel());
}

nsPoint nsView::GetOffsetTo(const nsView* aOther, const int32_t aAPD) const
{
  MOZ_ASSERT(GetParent() || !aOther || aOther->GetParent() || this == aOther,
             "caller of (outer) GetOffsetTo must not pass unrelated views");
  
  nsPoint offset(0, 0);
  
  nsPoint docOffset(0, 0);
  const nsView* v = this;
  nsViewManager* currVM = v->GetViewManager();
  int32_t currAPD = currVM->AppUnitsPerDevPixel();
  const nsView* root = nullptr;
  for ( ; v != aOther && v; root = v, v = v->GetParent()) {
    nsViewManager* newVM = v->GetViewManager();
    if (newVM != currVM) {
      int32_t newAPD = newVM->AppUnitsPerDevPixel();
      if (newAPD != currAPD) {
        offset += docOffset.ScaleToOtherAppUnits(currAPD, aAPD);
        docOffset.x = docOffset.y = 0;
        currAPD = newAPD;
      }
      currVM = newVM;
    }
    docOffset += v->GetPosition();
  }
  offset += docOffset.ScaleToOtherAppUnits(currAPD, aAPD);

  if (v != aOther) {
    
    
    
    nsPoint negOffset = aOther->GetOffsetTo(root, aAPD);
    offset -= negOffset;
  }

  return offset;
}

nsPoint nsView::GetOffsetToWidget(nsIWidget* aWidget) const
{
  nsPoint pt;
  
  nsView* widgetView = GetViewFor(aWidget);
  if (!widgetView) {
    return pt;
  }

  
  
  
  
  
  
  
  pt = -widgetView->GetOffsetTo(this);
  
  pt += widgetView->ViewToWidgetOffset();

  
  int32_t widgetAPD = widgetView->GetViewManager()->AppUnitsPerDevPixel();
  int32_t ourAPD = GetViewManager()->AppUnitsPerDevPixel();
  pt = pt.ScaleToOtherAppUnits(widgetAPD, ourAPD);
  return pt;
}

nsIWidget* nsView::GetNearestWidget(nsPoint* aOffset) const
{
  return GetNearestWidget(aOffset, GetViewManager()->AppUnitsPerDevPixel());
}

nsIWidget* nsView::GetNearestWidget(nsPoint* aOffset, const int32_t aAPD) const
{
  
  

  
  nsPoint pt(0, 0);
  
  nsPoint docPt(0,0);
  const nsView* v = this;
  nsViewManager* currVM = v->GetViewManager();
  int32_t currAPD = currVM->AppUnitsPerDevPixel();
  for ( ; v && !v->HasWidget(); v = v->GetParent()) {
    nsViewManager* newVM = v->GetViewManager();
    if (newVM != currVM) {
      int32_t newAPD = newVM->AppUnitsPerDevPixel();
      if (newAPD != currAPD) {
        pt += docPt.ScaleToOtherAppUnits(currAPD, aAPD);
        docPt.x = docPt.y = 0;
        currAPD = newAPD;
      }
      currVM = newVM;
    }
    docPt += v->GetPosition();
  }
  if (!v) {
    if (aOffset) {
      pt += docPt.ScaleToOtherAppUnits(currAPD, aAPD);
      *aOffset = pt;
    }
    return nullptr;
  }

  
  
  if (aOffset) {
    docPt += v->ViewToWidgetOffset();
    pt += docPt.ScaleToOtherAppUnits(currAPD, aAPD);
    *aOffset = pt;
  }
  return v->GetWidget();
}

bool nsView::IsRoot() const
{
  NS_ASSERTION(mViewManager != nullptr," View manager is null in nsView::IsRoot()");
  return mViewManager->GetRootView() == this;
}

nsRect
nsView::GetBoundsInParentUnits() const
{
  nsView* parent = GetParent();
  nsViewManager* VM = GetViewManager();
  if (this != VM->GetRootView() || !parent) {
    return mDimBounds;
  }
  int32_t ourAPD = VM->AppUnitsPerDevPixel();
  int32_t parentAPD = parent->GetViewManager()->AppUnitsPerDevPixel();
  return mDimBounds.ScaleToOtherAppUnitsRoundOut(ourAPD, parentAPD);
}

nsPoint
nsView::ConvertFromParentCoords(nsPoint aPt) const
{
  const nsView* parent = GetParent();
  if (parent) {
    aPt = aPt.ScaleToOtherAppUnits(
      parent->GetViewManager()->AppUnitsPerDevPixel(),
      GetViewManager()->AppUnitsPerDevPixel());
  }
  aPt -= GetPosition();
  return aPt;
}

static bool
IsPopupWidget(nsIWidget* aWidget)
{
  return (aWidget->WindowType() == eWindowType_popup);
}

nsIPresShell*
nsView::GetPresShell()
{
  return GetViewManager()->GetPresShell();
}

bool
nsView::WindowMoved(nsIWidget* aWidget, int32_t x, int32_t y)
{
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm && IsPopupWidget(aWidget)) {
    pm->PopupMoved(mFrame, nsIntPoint(x, y));
    return true;
  }

  return false;
}

bool
nsView::WindowResized(nsIWidget* aWidget, int32_t aWidth, int32_t aHeight)
{
  
  
  SetForcedRepaint(true);
  if (this == mViewManager->GetRootView()) {
    nsRefPtr<nsDeviceContext> devContext = mViewManager->GetDeviceContext();
    
    
    devContext->CheckDPIChange();
    int32_t p2a = devContext->AppUnitsPerDevPixel();
    mViewManager->SetWindowDimensions(NSIntPixelsToAppUnits(aWidth, p2a),
                                      NSIntPixelsToAppUnits(aHeight, p2a));

    nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
    if (pm) {
      nsIPresShell* presShell = mViewManager->GetPresShell();
      if (presShell && presShell->GetDocument()) {
        pm->AdjustPopupsOnWindowChange(presShell);
      }
    }

    return true;
  }
  else if (IsPopupWidget(aWidget)) {
    nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
    if (pm) {
      pm->PopupResized(mFrame, LayoutDeviceIntSize(aWidth, aHeight));
      return true;
    }
  }

  return false;
}

bool
nsView::RequestWindowClose(nsIWidget* aWidget)
{
  if (mFrame && IsPopupWidget(aWidget) &&
      mFrame->GetType() == nsGkAtoms::menuPopupFrame) {
    nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
    if (pm) {
      pm->HidePopup(mFrame->GetContent(), false, true, false, false);
      return true;
    }
  }

  return false;
}

void
nsView::WillPaintWindow(nsIWidget* aWidget)
{
  nsRefPtr<nsViewManager> vm = mViewManager;
  vm->WillPaintWindow(aWidget);
}

bool
nsView::PaintWindow(nsIWidget* aWidget, nsIntRegion aRegion)
{
  NS_ASSERTION(this == nsView::GetViewFor(aWidget), "wrong view for widget?");

  nsRefPtr<nsViewManager> vm = mViewManager;
  bool result = vm->PaintWindow(aWidget, aRegion);
  return result;
}

void
nsView::DidPaintWindow()
{
  nsRefPtr<nsViewManager> vm = mViewManager;
  vm->DidPaintWindow();
}

void
nsView::DidCompositeWindow()
{
  nsIPresShell* presShell = mViewManager->GetPresShell();
  if (presShell) {
    nsAutoScriptBlocker scriptBlocker;
    presShell->GetPresContext()->GetDisplayRootPresContext()->GetRootPresContext()->NotifyDidPaintForSubtree(nsIPresShell::PAINT_COMPOSITE);
  }
}

void
nsView::RequestRepaint()
{
  nsIPresShell* presShell = mViewManager->GetPresShell();
  if (presShell) {
    presShell->ScheduleViewManagerFlush();
  }
}

nsEventStatus
nsView::HandleEvent(WidgetGUIEvent* aEvent,
                    bool aUseAttachedEvents)
{
  NS_PRECONDITION(nullptr != aEvent->widget, "null widget ptr");

  nsEventStatus result = nsEventStatus_eIgnore;
  nsView* view;
  if (aUseAttachedEvents) {
    nsIWidgetListener* listener = aEvent->widget->GetAttachedWidgetListener();
    view = listener ? listener->GetView() : nullptr;
  }
  else {
    view = GetViewFor(aEvent->widget);
  }

  if (view) {
    nsRefPtr<nsViewManager> vm = view->GetViewManager();
    vm->DispatchEvent(aEvent, view, &result);
  }

  return result;
}
