




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
#include "nsXULPopupManager.h"
#include "nsIWidgetListener.h"

static nsEventStatus HandleEvent(nsGUIEvent *aEvent);

#define VIEW_WRAPPER_IID \
  { 0xbf4e1841, 0xe9ec, 0x47f2, \
    { 0xb4, 0x77, 0x0f, 0xf6, 0x0f, 0x5a, 0xac, 0xbd } }

static bool
IsPopupWidget(nsIWidget* aWidget)
{
  nsWindowType type;
  aWidget->GetWindowType(type);
  return (type == eWindowType_popup);
}




class ViewWrapper MOZ_FINAL : public nsIInterfaceRequestor,
                              public nsIWidgetListener
{
  public:
    NS_DECLARE_STATIC_IID_ACCESSOR(VIEW_WRAPPER_IID)
    NS_DECL_ISUPPORTS
    NS_DECL_NSIINTERFACEREQUESTOR

    ViewWrapper(nsView* aView) : mView(aView) {}

    nsView* GetView() { return mView; }
  private:
    nsView* mView;

  public:

    virtual nsIPresShell* GetPresShell()
    {
      return mView->GetViewManager()->GetPresShell();
    }

    bool WindowMoved(nsIWidget* aWidget, PRInt32 x, PRInt32 y)
    {
      nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
      if (pm && IsPopupWidget(aWidget)) {
        pm->PopupMoved(mView->GetFrame(), nsIntPoint(x, y));
        return true;
      }

      return false;
    }

    bool WindowResized(nsIWidget* aWidget, PRInt32 aWidth, PRInt32 aHeight)
    {
      nsIViewManager* viewManager = mView->GetViewManager();

      
      
      if (mView == viewManager->GetRootView()) {
        nsRefPtr<nsDeviceContext> devContext;
        viewManager->GetDeviceContext(*getter_AddRefs(devContext));
        PRInt32 p2a = devContext->AppUnitsPerDevPixel();
        viewManager->SetWindowDimensions(NSIntPixelsToAppUnits(aWidth, p2a),
                                         NSIntPixelsToAppUnits(aHeight, p2a));
        return true;
      }
      else if (IsPopupWidget(aWidget)) {
        nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
        if (pm) {
          pm->PopupResized(mView->GetFrame(), nsIntSize(aWidth, aHeight));
          return true;
        }
      }

      return false;
    }

    bool RequestWindowClose(nsIWidget* aWidget)
    {
      nsIFrame* frame = mView->GetFrame();
      if (frame && IsPopupWidget(aWidget) &&
          frame->GetType() == nsGkAtoms::menuPopupFrame) {
        nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
        if (pm) {
          pm->HidePopup(frame->GetContent(), false, true, false);
          return true;
        }
      }

      return false;
    }

    void WillPaintWindow(nsIWidget* aWidget, bool aWillSendPaint)
    {
      mView->GetViewManager()->WillPaintWindow(aWidget, aWillSendPaint);
    }

    bool PaintWindow(nsIWidget* aWidget, nsIntRegion aRegion, bool aSentWillPaint, bool aWillSendDidPaint)
    {
      nsCOMPtr<nsViewManager> vm = mView->GetViewManager();
      return vm->PaintWindow(aWidget, aRegion, aSentWillPaint, aWillSendDidPaint);
    }

    void DidPaintWindow()
    {
      mView->GetViewManager()->DidPaintWindow();
    }
};

NS_DEFINE_STATIC_IID_ACCESSOR(ViewWrapper, VIEW_WRAPPER_IID)

NS_IMPL_ADDREF(ViewWrapper)
NS_IMPL_RELEASE(ViewWrapper)
#ifndef DEBUG
NS_IMPL_QUERY_INTERFACE2(ViewWrapper, ViewWrapper, nsIInterfaceRequestor)

#else
NS_IMETHODIMP ViewWrapper::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  NS_ENSURE_ARG_POINTER(aInstancePtr);

  NS_ASSERTION(!aIID.Equals(NS_GET_IID(nsIView)),
               "Someone expects a viewwrapper to be a view!");
  
  *aInstancePtr = nullptr;
  
  if (aIID.Equals(NS_GET_IID(nsISupports))) {
    *aInstancePtr = static_cast<nsISupports*>(this);
  }
  else if (aIID.Equals(NS_GET_IID(ViewWrapper))) {
    *aInstancePtr = this;
  }
  else if (aIID.Equals(NS_GET_IID(nsIInterfaceRequestor))) {
    *aInstancePtr = this;
  }


  if (*aInstancePtr) {
    AddRef();
    return NS_OK;
  }

  return NS_NOINTERFACE;
}
#endif

NS_IMETHODIMP ViewWrapper::GetInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (aIID.Equals(NS_GET_IID(nsIView))) {
    *aInstancePtr = mView;
    return NS_OK;
  }
  return QueryInterface(aIID, aInstancePtr);
}





static ViewWrapper* GetWrapperFor(nsIWidget* aWidget)
{
  return aWidget ? static_cast<ViewWrapper *>(aWidget->GetWidgetListener()) : nullptr;
}


static nsEventStatus HandleEvent(nsGUIEvent *aEvent)
{
#if 0
  printf(" %d %d %d (%d,%d) \n", aEvent->widget, aEvent->message);
#endif
  nsEventStatus result = nsEventStatus_eIgnore;
  nsView *view = nsView::GetViewFor(aEvent->widget);

  if (view)
  {
    nsCOMPtr<nsIViewManager> vm = view->GetViewManager();
    vm->DispatchEvent(aEvent, view, &result);
  }

  return result;
}


static ViewWrapper* GetAttachedWrapperFor(nsIWidget* aWidget)
{
  NS_PRECONDITION(nullptr != aWidget, "null widget ptr");
  return aWidget->GetAttachedViewPtr();
}

static nsView* GetAttachedViewFor(nsIWidget* aWidget)
{           
  NS_PRECONDITION(nullptr != aWidget, "null widget ptr");

  ViewWrapper* wrapper = GetAttachedWrapperFor(aWidget);
  if (!wrapper)
    return nullptr;
  return wrapper->GetView();
}


static nsEventStatus AttachedHandleEvent(nsGUIEvent *aEvent)
{ 
  nsEventStatus result = nsEventStatus_eIgnore;
  nsView *view = GetAttachedViewFor(aEvent->widget);

  if (view)
  {
    nsCOMPtr<nsIViewManager> vm = view->GetViewManager();
    vm->DispatchEvent(aEvent, view, &result);
  }

  return result;
}

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
    
    ViewWrapper* wrapper = GetWrapperFor(mWindow);
    NS_IF_RELEASE(wrapper);

    
    
    
    
    if (mWidgetIsTopLevel) {
      ViewWrapper* wrapper = GetAttachedWrapperFor(mWindow);
      NS_IF_RELEASE(wrapper);

      mWindow->SetAttachedViewPtr(nullptr);
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

  ViewWrapper* wrapper = GetWrapperFor(aWidget);

  if (!wrapper) {
    wrapper = GetAttachedWrapperFor(aWidget);
  }

  if (wrapper) {
    return wrapper->GetView();
  }

  return nullptr;
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
  PRInt32 p2a = mViewManager->AppUnitsPerDevPixel();

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

  if (curBounds.IsEmpty() && mDimBounds.IsEmpty() && type == eWindowType_popup) {
    
    
    
    
    
    return;
  }

  NS_PRECONDITION(mWindow, "Why was this called??");

  nsIntRect newBounds = CalcWidgetBounds(type);

  bool changedPos = curBounds.TopLeft() != newBounds.TopLeft();
  bool changedSize = curBounds.Size() != newBounds.Size();

  
  if (changedPos) {
    if (changedSize && !aMoveOnly) {
      mWindow->ResizeClient(newBounds.x, newBounds.y,
                            newBounds.width, newBounds.height,
                            aInvalidateChangedSize);
    } else {
      mWindow->MoveClient(newBounds.x, newBounds.y);
    }
  } else {
    if (changedSize && !aMoveOnly) {
      mWindow->ResizeClient(newBounds.width, newBounds.height,
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
    bool found = false;
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






static void UpdateNativeWidgetZIndexes(nsView* aView, PRInt32 aZIndex)
{
  if (aView->HasWidget()) {
    nsIWidget* widget = aView->GetWidget();
    PRInt32 curZ;
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

static PRInt32 FindNonAutoZIndex(nsView* aView)
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

  
  
  mWindow = parentWidget->CreateChild(trect, ::HandleEvent,
                                      dx, aWidgetInitData,
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
    aParentWidget->CreateChild(trect, ::HandleEvent,
                               dx, aWidgetInitData).get();
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
    
    
    mWindow = aParentWidget->CreateChild(trect, ::HandleEvent,
                                         dx, aWidgetInitData,
                                         true).get();
  }
  else {
    nsIWidget* nearestParent = GetParent() ? GetParent()->GetNearestWidget(nullptr)
                                           : nullptr;
    if (!nearestParent) {
      
      
      return NS_ERROR_FAILURE;
    }

    mWindow =
      nearestParent->CreateChild(trect, ::HandleEvent,
                                 dx, aWidgetInitData).get();
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

  ViewWrapper* wrapper = new ViewWrapper(this);
  NS_ADDREF(wrapper); 
  mWindow->SetWidgetListener(wrapper);

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
  
  
  nsIView *oldView = GetAttachedViewFor(aWidget);
  if (oldView) {
    oldView->DetachFromTopLevelWidget();
  }

  nsRefPtr<nsDeviceContext> dx;
  mViewManager->GetDeviceContext(*getter_AddRefs(dx));

  
  
  nsresult rv = aWidget->AttachViewToTopLevel(
    nsIWidget::UsePuppetWidgets() ? ::HandleEvent : ::AttachedHandleEvent, dx);
  if (NS_FAILED(rv))
    return rv;

  mWindow = aWidget;
  NS_ADDREF(mWindow);

  ViewWrapper* wrapper = new ViewWrapper(Impl());
  NS_ADDREF(wrapper);
  mWindow->SetAttachedViewPtr(wrapper);
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

  
  ViewWrapper* wrapper = GetAttachedWrapperFor(mWindow);
  NS_IF_RELEASE(wrapper);

  mWindow->SetAttachedViewPtr(nullptr);
  NS_RELEASE(mWindow);

  mWidgetIsTopLevel = false;
  
  return NS_OK;
}

void nsView::SetZIndex(bool aAuto, PRInt32 aZIndex, bool aTopMost)
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
  
  if (NS_UNLIKELY(mWindow)) {
    NS_ERROR("We already have a window for this view? BAD");
    ViewWrapper* wrapper = GetWrapperFor(mWindow);
    NS_IF_RELEASE(wrapper);
    mWindow->SetWidgetListener(nullptr);
    mWindow->Destroy();
    NS_RELEASE(mWindow);
  }
}




EVENT_CALLBACK nsIView::AttachWidgetEventHandler(nsIWidget* aWidget)
{
#ifdef DEBUG
  NS_ASSERTION(!aWidget->GetWidgetListener(), "Already have a widget listener");
#endif

  ViewWrapper* wrapper = new ViewWrapper(Impl());
  if (!wrapper)
    return nullptr;
  NS_ADDREF(wrapper); 
  aWidget->SetWidgetListener(wrapper);
  return ::HandleEvent;
}

void nsIView::DetachWidgetEventHandler(nsIWidget* aWidget)
{
  ViewWrapper* wrapper = GetWrapperFor(aWidget);
  NS_ASSERTION(!wrapper || wrapper->GetView() == this, "Wrong view");
  NS_IF_RELEASE(wrapper);
  aWidget->SetWidgetListener(nullptr);
}

#ifdef DEBUG
void nsIView::List(FILE* out, PRInt32 aIndent) const
{
  PRInt32 i;
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
    PRInt32 Z;
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

nsPoint nsView::GetOffsetTo(const nsView* aOther, const PRInt32 aAPD) const
{
  NS_ABORT_IF_FALSE(GetParent() || !aOther || aOther->GetParent() ||
                    this == aOther, "caller of (outer) GetOffsetTo must not "
                    "pass unrelated views");
  
  nsPoint offset(0, 0);
  
  nsPoint docOffset(0, 0);
  const nsView* v = this;
  nsViewManager* currVM = v->GetViewManager();
  PRInt32 currAPD = currVM->AppUnitsPerDevPixel();
  const nsView* root = nullptr;
  for ( ; v != aOther && v; root = v, v = v->GetParent()) {
    nsViewManager* newVM = v->GetViewManager();
    if (newVM != currVM) {
      PRInt32 newAPD = newVM->AppUnitsPerDevPixel();
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

  
  PRInt32 widgetAPD = widgetView->GetViewManager()->AppUnitsPerDevPixel();
  PRInt32 ourAPD = static_cast<const nsView*>(this)->
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

nsIWidget* nsView::GetNearestWidget(nsPoint* aOffset, const PRInt32 aAPD) const
{
  
  

  
  nsPoint pt(0, 0);
  
  nsPoint docPt(0,0);
  const nsView* v = this;
  nsViewManager* currVM = v->GetViewManager();
  PRInt32 currAPD = currVM->AppUnitsPerDevPixel();
  for ( ; v && !v->HasWidget(); v = v->GetParent()) {
    nsViewManager* newVM = v->GetViewManager();
    if (newVM != currVM) {
      PRInt32 newAPD = newVM->AppUnitsPerDevPixel();
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
  PRInt32 ourAPD = VM->AppUnitsPerDevPixel();
  PRInt32 parentAPD = parent->GetViewManager()->AppUnitsPerDevPixel();
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
