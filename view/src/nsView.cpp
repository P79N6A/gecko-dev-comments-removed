




#include "nsView.h"
#include "nsIWidget.h"
#include "nsWidgetsCID.h"
#include "nsViewManager.h"
#include "nsIFrame.h"
#include "nsGUIEvent.h"
#include "nsIComponentManager.h"
#include "nsGfxCIID.h"
#include "nsIInterfaceRequestor.h"
#include "mozilla/Attributes.h"
#include "mozilla/Likely.h"
#include "mozilla/Util.h"
#include "nsXULPopupManager.h"
#include "nsIWidgetListener.h"

using namespace mozilla;

nsView::nsView(nsViewManager* aViewManager, nsViewVisibility aVisibility)
{
  MOZ_COUNT_CTOR(nsView);

  mVis = aVisibility;
  
  
  
  
  mVFlags = 0;
  mViewManager = aViewManager;
  mDirtyRegion = nullptr;
  mWidgetIsTopLevel = false;
  mInAlternatePaint = false;
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
  
    nsView *rootView = mViewManager->GetRootViewImpl();
    
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

void nsView::DestroyWidget()
{
  if (mWindow)
  {
    
    
    
    
    if (mWidgetIsTopLevel) {
      mWindow->SetAttachedWidgetListener(nullptr);
    }
    else {
      mWindow->SetWidgetListener(nullptr);
      mWindow->Destroy();
    }

    NS_RELEASE(mWindow);
  }
}

nsresult nsView::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  if (nullptr == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }

  NS_ASSERTION(!aIID.Equals(NS_GET_IID(nsISupports)),
               "Someone expects views to be ISupports-derived!");
  
  *aInstancePtr = nullptr;
  
  if (aIID.Equals(NS_GET_IID(nsIView))) {
    *aInstancePtr = (void*)(nsIView*)this;
    return NS_OK;
  }

  return NS_NOINTERFACE;
}

nsIView* nsIView::GetViewFor(nsIWidget* aWidget)
{
  NS_PRECONDITION(nullptr != aWidget, "null widget ptr");

  nsIWidgetListener* listener = aWidget->GetWidgetListener();
  if (listener) {
    nsIView* view = listener->GetView();
    if (view)
      return view;
  }

  listener = aWidget->GetAttachedWidgetListener();
  return listener ? listener->GetView() : nullptr;
}

void nsIView::Destroy()
{
  delete this;
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

bool nsIView::IsEffectivelyVisible()
{
  for (nsIView* v = this; v; v = v->mParent) {
    if (v->GetVisibility() == nsViewVisibility_kHide)
      return false;
  }
  return true;
}

nsIntRect nsIView::CalcWidgetBounds(nsWindowType aType)
{
  int32_t p2a = mViewManager->AppUnitsPerDevPixel();

  nsRect viewBounds(mDimBounds);

  nsView* parent = GetParent()->Impl();
  if (parent) {
    nsPoint offset;
    nsIWidget* parentWidget = parent->GetNearestWidget(&offset, p2a);
    
    viewBounds += offset;

    if (parentWidget && aType == eWindowType_popup &&
        IsEffectivelyVisible()) {
      
      nsIntPoint screenPoint = parentWidget->WidgetToScreenOffset();
      viewBounds += nsPoint(NSIntPixelsToAppUnits(screenPoint.x, p2a),
                            NSIntPixelsToAppUnits(screenPoint.y, p2a));
    }
  }

  
  nsIntRect newBounds = viewBounds.ToNearestPixels(p2a);

  
  
  nsPoint roundedOffset(NSIntPixelsToAppUnits(newBounds.x, p2a),
                        NSIntPixelsToAppUnits(newBounds.y, p2a));

  
  
  
  
  
  mViewToWidgetOffset = nsPoint(mPosX, mPosY)
    - mDimBounds.TopLeft() + viewBounds.TopLeft() - roundedOffset;

  return newBounds;
}

void nsView::DoResetWidgetBounds(bool aMoveOnly,
                                 bool aInvalidateChangedSize) {
  
  
  if (mViewManager->GetRootViewImpl() == this) {
    return;
  }
  
  nsIntRect curBounds;
  mWindow->GetClientBounds(curBounds);

  nsWindowType type;
  mWindow->GetWindowType(type);

  if (type == eWindowType_popup &&
      ((curBounds.IsEmpty() && mDimBounds.IsEmpty()) ||
       mVis == nsViewVisibility_kHide)) {
    
    
    
    
    
    return;
  }

  NS_PRECONDITION(mWindow, "Why was this called??");

  nsIntRect newBounds = CalcWidgetBounds(type);

  bool changedPos = curBounds.TopLeft() != newBounds.TopLeft();
  bool changedSize = curBounds.Size() != newBounds.Size();

  

  
  
  
  
  nsRefPtr<nsDeviceContext> dx;
  mViewManager->GetDeviceContext(*getter_AddRefs(dx));
  double invScale = dx->UnscaledAppUnitsPerDevPixel() / 60.0;

  if (changedPos) {
    if (changedSize && !aMoveOnly) {
      mWindow->ResizeClient(newBounds.x * invScale,
                            newBounds.y * invScale,
                            newBounds.width * invScale,
                            newBounds.height * invScale,
                            aInvalidateChangedSize);
    } else {
      mWindow->MoveClient(newBounds.x * invScale,
                          newBounds.y * invScale);
    }
  } else {
    if (changedSize && !aMoveOnly) {
      mWindow->ResizeClient(newBounds.width * invScale,
                            newBounds.height * invScale,
                            aInvalidateChangedSize);
    } 
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

  if (nullptr != mWindow)
  {
    if (aEffectivelyVisible)
    {
      DoResetWidgetBounds(false, true);
      mWindow->Show(true);
    }
    else
      mWindow->Show(false);
  }

  for (nsView* child = mFirstChild; child; child = child->mNextSibling) {
    if (child->mVis == nsViewVisibility_kHide) {
      
      continue;
    }
    
    child->NotifyEffectiveVisibilityChanged(aEffectivelyVisible);
  }
}

NS_IMETHODIMP nsView::SetVisibility(nsViewVisibility aVisibility)
{
  mVis = aVisibility;
  NotifyEffectiveVisibilityChanged(IsEffectivelyVisible());
  return NS_OK;
}

NS_IMETHODIMP nsView::SetFloating(bool aFloatingView)
{
	if (aFloatingView)
		mVFlags |= NS_VIEW_FLAG_FLOATING;
	else
		mVFlags &= ~NS_VIEW_FLAG_FLOATING;

#if 0
	
	for (nsView* child = mFirstChild; chlid; child = child->GetNextSibling()) {
		child->SetFloating(aFloatingView);
	}
#endif

	return NS_OK;
}

void nsView::InvalidateHierarchy(nsViewManager *aViewManagerParent)
{
  if (mViewManager->GetRootViewImpl() == this)
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
    if (vm->GetRootViewImpl() == aChild)
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
    if (vm->GetRootViewImpl() == child)
    {
      child->InvalidateHierarchy(GetViewManager());
    }
  }
}






static void UpdateNativeWidgetZIndexes(nsView* aView, int32_t aZIndex)
{
  if (aView->HasWidget()) {
    nsIWidget* widget = aView->GetWidget();
    int32_t curZ;
    widget->GetZIndex(&curZ);
    if (curZ != aZIndex) {
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

nsresult nsIView::CreateWidget(nsWidgetInitData *aWidgetInitData,
                               bool aEnableDragDrop,
                               bool aResetVisibility)
{
  return Impl()->CreateWidget(aWidgetInitData,
                              aEnableDragDrop, aResetVisibility);
}

nsresult nsIView::CreateWidgetForParent(nsIWidget* aParentWidget,
                                        nsWidgetInitData *aWidgetInitData,
                                        bool aEnableDragDrop,
                                        bool aResetVisibility)
{
  return Impl()->CreateWidgetForParent(aParentWidget, aWidgetInitData,
                                       aEnableDragDrop, aResetVisibility);
}

nsresult nsIView::CreateWidgetForPopup(nsWidgetInitData *aWidgetInitData,
                                       nsIWidget* aParentWidget,
                                       bool aEnableDragDrop,
                                       bool aResetVisibility)
{
  return Impl()->CreateWidgetForPopup(aWidgetInitData, aParentWidget,
                                      aEnableDragDrop, aResetVisibility);
}

void nsIView::DestroyWidget()
{
  Impl()->DestroyWidget();
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
  NS_ABORT_IF_FALSE(!aWidgetInitData ||
                    aWidgetInitData->mWindowType != eWindowType_popup,
                    "Use CreateWidgetForPopup");

  DefaultWidgetInitData defaultInitData;
  bool initDataPassedIn = !!aWidgetInitData;
  aWidgetInitData = aWidgetInitData ? aWidgetInitData : &defaultInitData;
  defaultInitData.mListenForResizes =
    (!initDataPassedIn && GetParent() &&
     GetParent()->GetViewManager() != mViewManager);

  nsIntRect trect = CalcWidgetBounds(aWidgetInitData->mWindowType);

  nsRefPtr<nsDeviceContext> dx;
  mViewManager->GetDeviceContext(*getter_AddRefs(dx));

  nsIWidget* parentWidget =
    GetParent() ? GetParent()->GetNearestWidget(nullptr) : nullptr;
  if (!parentWidget) {
    NS_ERROR("nsView::CreateWidget without suitable parent widget??");
    return NS_ERROR_FAILURE;
  }

  
  
  mWindow = parentWidget->CreateChild(trect, dx, aWidgetInitData,
                                      true).get();
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
  NS_ABORT_IF_FALSE(!aWidgetInitData ||
                    aWidgetInitData->mWindowType != eWindowType_popup,
                    "Use CreateWidgetForPopup");
  NS_ABORT_IF_FALSE(aParentWidget, "Parent widget required");

  DefaultWidgetInitData defaultInitData;
  aWidgetInitData = aWidgetInitData ? aWidgetInitData : &defaultInitData;

  nsIntRect trect = CalcWidgetBounds(aWidgetInitData->mWindowType);

  nsRefPtr<nsDeviceContext> dx;
  mViewManager->GetDeviceContext(*getter_AddRefs(dx));

  mWindow =
    aParentWidget->CreateChild(trect, dx, aWidgetInitData).get();
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
  NS_ABORT_IF_FALSE(aWidgetInitData, "Widget init data required");
  NS_ABORT_IF_FALSE(aWidgetInitData->mWindowType == eWindowType_popup,
                    "Use one of the other CreateWidget methods");

  nsIntRect trect = CalcWidgetBounds(aWidgetInitData->mWindowType);

  nsRefPtr<nsDeviceContext> dx;
  mViewManager->GetDeviceContext(*getter_AddRefs(dx));

  
  
  
  
  if (aParentWidget) {
    
    
    mWindow = aParentWidget->CreateChild(trect, dx, aWidgetInitData,
                                         true).get();
  }
  else {
    nsIWidget* nearestParent = GetParent() ? GetParent()->GetNearestWidget(nullptr)
                                           : nullptr;
    if (!nearestParent) {
      
      
      return NS_ERROR_FAILURE;
    }

    mWindow =
      nearestParent->CreateChild(trect, dx, aWidgetInitData).get();
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
  NS_ABORT_IF_FALSE(mWindow, "Must have a window to initialize");

  mWindow->SetWidgetListener(this);

  if (aEnableDragDrop) {
    mWindow->EnableDragDrop(true);
  }
      
  
  UpdateNativeWidgetZIndexes(this, FindNonAutoZIndex(this));

  

  if (aResetVisibility) {
    SetVisibility(GetVisibility());
  }
}


nsresult nsIView::AttachToTopLevelWidget(nsIWidget* aWidget)
{
  NS_PRECONDITION(nullptr != aWidget, "null widget ptr");
  
  
  nsIWidgetListener* listener = aWidget->GetAttachedWidgetListener();
  if (listener) {
    nsIView *oldView = listener->GetView();
    if (oldView) {
      oldView->DetachFromTopLevelWidget();
    }
  }

  nsRefPtr<nsDeviceContext> dx;
  mViewManager->GetDeviceContext(*getter_AddRefs(dx));

  
  
  nsresult rv = aWidget->AttachViewToTopLevel(!nsIWidget::UsePuppetWidgets(), dx);
  if (NS_FAILED(rv))
    return rv;

  mWindow = aWidget;
  NS_ADDREF(mWindow);

  mWindow->SetAttachedWidgetListener(Impl());
  mWindow->EnableDragDrop(true);
  mWidgetIsTopLevel = true;

  
  nsWindowType type;
  mWindow->GetWindowType(type);
  CalcWidgetBounds(type);

  return NS_OK;
}


nsresult nsIView::DetachFromTopLevelWidget()
{
  NS_PRECONDITION(mWidgetIsTopLevel, "Not attached currently!");
  NS_PRECONDITION(mWindow, "null mWindow for DetachFromTopLevelWidget!");

  mWindow->SetAttachedWidgetListener(nullptr);
  NS_RELEASE(mWindow);

  mWidgetIsTopLevel = false;
  
  return NS_OK;
}

void nsView::SetZIndex(bool aAuto, int32_t aZIndex, bool aTopMost)
{
  bool oldIsAuto = GetZIndexIsAuto();
  mVFlags = (mVFlags & ~NS_VIEW_FLAG_AUTO_ZINDEX) | (aAuto ? NS_VIEW_FLAG_AUTO_ZINDEX : 0);
  mZIndex = aZIndex;
  SetTopMost(aTopMost);
  
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
    NS_RELEASE(mWindow);
  }
}




void nsIView::AttachWidgetEventHandler(nsIWidget* aWidget)
{
#ifdef DEBUG
  NS_ASSERTION(!aWidget->GetWidgetListener(), "Already have a widget listener");
#endif

  aWidget->SetWidgetListener(Impl());
}

void nsIView::DetachWidgetEventHandler(nsIWidget* aWidget)
{
  NS_ASSERTION(!aWidget->GetWidgetListener() ||
               aWidget->GetWidgetListener()->GetView() == this, "Wrong view");
  aWidget->SetWidgetListener(nullptr);
}

#ifdef DEBUG
void nsIView::List(FILE* out, int32_t aIndent) const
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
    nsrefcnt widgetRefCnt = mWindow->AddRef() - 1;
    mWindow->Release();
    int32_t Z;
    mWindow->GetZIndex(&Z);
    fprintf(out, "(widget=%p[%d] z=%d pos={%d,%d,%d,%d}) ",
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

nsPoint nsIView::GetOffsetTo(const nsIView* aOther) const
{
  return Impl()->GetOffsetTo(static_cast<const nsView*>(aOther),
                             Impl()->GetViewManager()->AppUnitsPerDevPixel());
}

nsPoint nsView::GetOffsetTo(const nsView* aOther) const
{
  return GetOffsetTo(aOther, GetViewManager()->AppUnitsPerDevPixel());
}

nsPoint nsView::GetOffsetTo(const nsView* aOther, const int32_t aAPD) const
{
  NS_ABORT_IF_FALSE(GetParent() || !aOther || aOther->GetParent() ||
                    this == aOther, "caller of (outer) GetOffsetTo must not "
                    "pass unrelated views");
  
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
        offset += docOffset.ConvertAppUnits(currAPD, aAPD);
        docOffset.x = docOffset.y = 0;
        currAPD = newAPD;
      }
      currVM = newVM;
    }
    docOffset += v->GetPosition();
  }
  offset += docOffset.ConvertAppUnits(currAPD, aAPD);

  if (v != aOther) {
    
    
    
    nsPoint negOffset = aOther->GetOffsetTo(root, aAPD);
    offset -= negOffset;
  }

  return offset;
}

nsPoint nsIView::GetOffsetToWidget(nsIWidget* aWidget) const
{
  nsPoint pt;
  
  nsIView* widgetIView = GetViewFor(aWidget);
  if (!widgetIView) {
    return pt;
  }
  nsView* widgetView = widgetIView->Impl();

  
  
  
  
  
  
  
  pt = -widgetView->GetOffsetTo(static_cast<const nsView*>(this));
  
  pt += widgetView->ViewToWidgetOffset();

  
  int32_t widgetAPD = widgetView->GetViewManager()->AppUnitsPerDevPixel();
  int32_t ourAPD = static_cast<const nsView*>(this)->
                    GetViewManager()->AppUnitsPerDevPixel();
  pt = pt.ConvertAppUnits(widgetAPD, ourAPD);
  return pt;
}

nsIWidget* nsIView::GetNearestWidget(nsPoint* aOffset) const
{
  return Impl()->GetNearestWidget(aOffset,
                                  Impl()->GetViewManager()->AppUnitsPerDevPixel());
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
        pt += docPt.ConvertAppUnits(currAPD, aAPD);
        docPt.x = docPt.y = 0;
        currAPD = newAPD;
      }
      currVM = newVM;
    }
    docPt += v->GetPosition();
  }
  if (!v) {
    if (aOffset) {
      pt += docPt.ConvertAppUnits(currAPD, aAPD);
      *aOffset = pt;
    }
    return nullptr;
  }

  
  
  if (aOffset) {
    docPt += v->ViewToWidgetOffset();
    pt += docPt.ConvertAppUnits(currAPD, aAPD);
    *aOffset = pt;
  }
  return v->GetWidget();
}

bool nsIView::IsRoot() const
{
  NS_ASSERTION(mViewManager != nullptr," View manager is null in nsView::IsRoot()");
  return mViewManager->GetRootViewImpl() == this;
}

bool nsIView::ExternalIsRoot() const
{
  return nsIView::IsRoot();
}

nsView*
nsIView::Impl()
{
  return static_cast<nsView*>(this);
}

const nsView*
nsIView::Impl() const
{
  return static_cast<const nsView*>(this);
}

nsRect
nsView::GetBoundsInParentUnits() const
{
  nsView* parent = GetParent();
  nsViewManager* VM = GetViewManager();
  if (this != VM->GetRootViewImpl() || !parent) {
    return mDimBounds;
  }
  int32_t ourAPD = VM->AppUnitsPerDevPixel();
  int32_t parentAPD = parent->GetViewManager()->AppUnitsPerDevPixel();
  return mDimBounds.ConvertAppUnitsRoundOut(ourAPD, parentAPD);
}

nsPoint
nsIView::ConvertFromParentCoords(nsPoint aPt) const
{
  const nsView* view = static_cast<const nsView*>(this);
  const nsView* parent = view->GetParent();
  if (parent) {
    aPt = aPt.ConvertAppUnits(parent->GetViewManager()->AppUnitsPerDevPixel(),
                              view->GetViewManager()->AppUnitsPerDevPixel());
  }
  aPt -= GetPosition();
  return aPt;
}

static bool
IsPopupWidget(nsIWidget* aWidget)
{
  nsWindowType type;
  aWidget->GetWindowType(type);
  return (type == eWindowType_popup);
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
    nsRefPtr<nsDeviceContext> devContext;
    mViewManager->GetDeviceContext(*getter_AddRefs(devContext));
    int32_t p2a = devContext->AppUnitsPerDevPixel();
    mViewManager->SetWindowDimensions(NSIntPixelsToAppUnits(aWidth, p2a),
                                      NSIntPixelsToAppUnits(aHeight, p2a));
    return true;
  }
  else if (IsPopupWidget(aWidget)) {
    nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
    if (pm) {
      pm->PopupResized(mFrame, nsIntSize(aWidth, aHeight));
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
      pm->HidePopup(mFrame->GetContent(), false, true, false);
      return true;
    }
  }

  return false;
}

void
nsView::WillPaintWindow(nsIWidget* aWidget, bool aWillSendDidPaint)
{
  nsCOMPtr<nsViewManager> vm = mViewManager;
  vm->WillPaintWindow(aWidget, aWillSendDidPaint);
}

bool
nsView::PaintWindow(nsIWidget* aWidget, nsIntRegion aRegion, uint32_t aFlags)
{
  NS_ASSERTION(this == nsView::GetViewFor(aWidget), "wrong view for widget?");

  mInAlternatePaint = aFlags & PAINT_IS_ALTERNATE;
  nsCOMPtr<nsViewManager> vm = mViewManager;
  bool result = vm->PaintWindow(aWidget, aRegion, aFlags);
  
  
  nsView* view = nsView::GetViewFor(aWidget);
  if (view) {
    view->mInAlternatePaint = false;
  }
  return result;
}

void
nsView::DidPaintWindow()
{
  nsCOMPtr<nsViewManager> vm = mViewManager;
  vm->DidPaintWindow();
}

nsEventStatus
nsView::HandleEvent(nsGUIEvent* aEvent, bool aUseAttachedEvents)
{
  NS_PRECONDITION(nullptr != aEvent->widget, "null widget ptr");

  nsEventStatus result = nsEventStatus_eIgnore;
  nsIView* view;
  if (aUseAttachedEvents) {
    nsIWidgetListener* listener = aEvent->widget->GetAttachedWidgetListener();
    view = listener ? listener->GetView() : nullptr;
  }
  else {
    view = GetViewFor(aEvent->widget);
  }

  if (view) {
    nsCOMPtr<nsIViewManager> vm = view->GetViewManager();
    vm->DispatchEvent(aEvent, view, &result);
  }

  return result;
}
