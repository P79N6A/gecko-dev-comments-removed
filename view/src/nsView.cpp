




































#include "nsView.h"
#include "nsIWidget.h"
#include "nsWidgetsCID.h"
#include "nsViewManager.h"
#include "nsGUIEvent.h"
#include "nsIComponentManager.h"
#include "nsGfxCIID.h"
#include "nsIRegion.h"
#include "nsIInterfaceRequestor.h"



static nsEventStatus HandleEvent(nsGUIEvent *aEvent);




#define VIEW_WRAPPER_IID \
  { 0xbf4e1841, 0xe9ec, 0x47f2, \
    { 0xb4, 0x77, 0x0f, 0xf6, 0x0f, 0x5a, 0xac, 0xbd } }




class ViewWrapper : public nsIInterfaceRequestor
{
  public:
    NS_DECLARE_STATIC_IID_ACCESSOR(VIEW_WRAPPER_IID)
    NS_DECL_ISUPPORTS
    NS_DECL_NSIINTERFACEREQUESTOR

    ViewWrapper(nsView* aView) : mView(aView) {}

    nsView* GetView() { return mView; }
  private:
    nsView* mView;
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
  
  *aInstancePtr = nsnull;
  
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
  
  if (aWidget) {
    void* clientData;
    aWidget->GetClientData(clientData);
    nsISupports* data = (nsISupports*)clientData;
    
    if (data) {
      ViewWrapper* wrapper;
      CallQueryInterface(data, &wrapper);
      
      
      if (wrapper)
        wrapper->Release();
      return wrapper;
    }
  }
  return nsnull;
}


static nsEventStatus HandleEvent(nsGUIEvent *aEvent)
{
#if 0
  printf(" %d %d %d (%d,%d) \n", aEvent->widget, aEvent->widgetSupports, 
         aEvent->message, aEvent->point.x, aEvent->point.y);
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
  NS_PRECONDITION(nsnull != aWidget, "null widget ptr");
  return aWidget->GetAttachedViewPtr();
}

static nsView* GetAttachedViewFor(nsIWidget* aWidget)
{           
  NS_PRECONDITION(nsnull != aWidget, "null widget ptr");

  ViewWrapper* wrapper = GetAttachedWrapperFor(aWidget);
  if (!wrapper)
    return nsnull;
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
  mDirtyRegion = nsnull;
  mDeletionObserver = nsnull;
  mHaveInvalidationDimensions = PR_FALSE;
  mWidgetIsTopLevel = PR_FALSE;
}

void nsView::DropMouseGrabbing()
{
  nsCOMPtr<nsIViewObserver> viewObserver = mViewManager->GetViewObserver();
  if (viewObserver) {
    viewObserver->ClearMouseCapture(this);
  }
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
        
        mViewManager->SetRootView(nsnull);
      }
    }
    else if (mParent)
    {
      mParent->RemoveChild(this);
    }
    
    mViewManager = nsnull;
  }
  else if (mParent)
  {
    mParent->RemoveChild(this);
  }

  
  DestroyWidget();

  delete mDirtyRegion;

  if (mDeletionObserver) {
    mDeletionObserver->Clear();
  }
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

      mWindow->SetAttachedViewPtr(nsnull);
    }
    else {
      mWindow->SetClientData(nsnull);
      mWindow->Destroy();
    }

    NS_RELEASE(mWindow);
  }
}

nsresult nsView::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  if (nsnull == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }

  NS_ASSERTION(!aIID.Equals(NS_GET_IID(nsISupports)),
               "Someone expects views to be ISupports-derived!");
  
  *aInstancePtr = nsnull;
  
  if (aIID.Equals(NS_GET_IID(nsIView))) {
    *aInstancePtr = (void*)(nsIView*)this;
    return NS_OK;
  }

  return NS_NOINTERFACE;
}

nsIView* nsIView::GetViewFor(nsIWidget* aWidget)
{           
  NS_PRECONDITION(nsnull != aWidget, "null widget ptr");

  ViewWrapper* wrapper = GetWrapperFor(aWidget);

  if (!wrapper)
    wrapper = GetAttachedWrapperFor(aWidget);

  if (wrapper)
    return wrapper->GetView();

  return nsnull;
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

  ResetWidgetBounds(PR_TRUE, PR_TRUE, PR_FALSE);
}

void nsIView::SetInvalidationDimensions(const nsRect* aRect)
{
  return Impl()->SetInvalidationDimensions(aRect);
}

void nsView::SetPositionIgnoringChildWidgets(nscoord aX, nscoord aY)
{
  mDimBounds.x += aX - mPosX;
  mDimBounds.y += aY - mPosY;
  mPosX = aX;
  mPosY = aY;

  ResetWidgetBounds(PR_FALSE, PR_TRUE, PR_FALSE);
}

void nsView::ResetWidgetBounds(PRBool aRecurse, PRBool aMoveOnly,
                               PRBool aInvalidateChangedSize) {
  if (mWindow) {
    
    
    
    if (!mViewManager->IsRefreshEnabled()) {
      mViewManager->PostPendingUpdate();
      return;
    }

    DoResetWidgetBounds(aMoveOnly, aInvalidateChangedSize);
  } else if (aRecurse) {
    
    for (nsView* v = GetFirstChild(); v; v = v->GetNextSibling()) {
      v->ResetWidgetBounds(PR_TRUE, aMoveOnly, aInvalidateChangedSize);
    }
  }
}

PRBool nsIView::IsEffectivelyVisible()
{
  for (nsIView* v = this; v; v = v->mParent) {
    if (v->GetVisibility() == nsViewVisibility_kHide)
      return PR_FALSE;
  }
  return PR_TRUE;
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

void nsView::DoResetWidgetBounds(PRBool aMoveOnly,
                                 PRBool aInvalidateChangedSize) {
  
  
  if (mViewManager->GetRootViewImpl() == this) {
    return;
  }
  
  nsIntRect curBounds;
  mWindow->GetBounds(curBounds);

  nsWindowType type;
  mWindow->GetWindowType(type);

  if (curBounds.IsEmpty() && mDimBounds.IsEmpty() && type == eWindowType_popup) {
    
    
    
    
    
    return;
  }

  NS_PRECONDITION(mWindow, "Why was this called??");

  nsIntRect newBounds = CalcWidgetBounds(type);

  PRBool changedPos = curBounds.TopLeft() != newBounds.TopLeft();
  PRBool changedSize = curBounds.Size() != newBounds.Size();

  
  if (changedPos) {
    if (changedSize && !aMoveOnly) {
      mWindow->Resize(newBounds.x, newBounds.y, newBounds.width, newBounds.height,
                      aInvalidateChangedSize);
    } else {
      mWindow->Move(newBounds.x, newBounds.y);
    }
  } else {
    if (changedSize && !aMoveOnly) {
      mWindow->Resize(newBounds.width, newBounds.height, aInvalidateChangedSize);
    } 
  }
}

void nsView::SetDimensions(const nsRect& aRect, PRBool aPaint, PRBool aResizeWidget)
{
  nsRect dims = aRect;
  dims.MoveBy(mPosX, mPosY);

  
  
  
  if (mDimBounds.TopLeft() == dims.TopLeft() &&
      mDimBounds.Size() == dims.Size()) {
    return;
  }

  mDimBounds = dims;

  if (aResizeWidget) {
    ResetWidgetBounds(PR_FALSE, PR_FALSE, aPaint);
  }
}

void nsView::SetInvalidationDimensions(const nsRect* aRect)
{
  if ((mHaveInvalidationDimensions = !!aRect)) {
    mInvalidationDimensions = *aRect;
  }
}

void nsView::NotifyEffectiveVisibilityChanged(PRBool aEffectivelyVisible)
{
  if (!aEffectivelyVisible)
  {
    DropMouseGrabbing();
  }

  if (nsnull != mWindow)
  {
    if (aEffectivelyVisible)
    {
      DoResetWidgetBounds(PR_FALSE, PR_TRUE);
      mWindow->Show(PR_TRUE);
    }
    else
      mWindow->Show(PR_FALSE);
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

NS_IMETHODIMP nsView::SetFloating(PRBool aFloatingView)
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
  NS_PRECONDITION(nsnull != aChild, "null ptr");

  if (nsnull != aChild)
  {
    if (nsnull != aSibling)
    {
#ifdef NS_DEBUG
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
      aChild->InvalidateHierarchy(nsnull); 
    }
  }
}

void nsView::RemoveChild(nsView *child)
{
  NS_PRECONDITION(nsnull != child, "null ptr");

  if (nsnull != child)
  {
    nsView* prevKid = nsnull;
    nsView* kid = mFirstChild;
    PRBool found = PR_FALSE;
    while (nsnull != kid) {
      if (kid == child) {
        if (nsnull != prevKid) {
          prevKid->SetNextSibling(kid->GetNextSibling());
        } else {
          mFirstChild = kid->GetNextSibling();
        }
        child->SetParent(nsnull);
        found = PR_TRUE;
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
                               PRBool aEnableDragDrop,
                               PRBool aResetVisibility)
{
  return Impl()->CreateWidget(aWidgetInitData,
                              aEnableDragDrop, aResetVisibility);
}

nsresult nsIView::CreateWidgetForParent(nsIWidget* aParentWidget,
                                        nsWidgetInitData *aWidgetInitData,
                                        PRBool aEnableDragDrop,
                                        PRBool aResetVisibility)
{
  return Impl()->CreateWidgetForParent(aParentWidget, aWidgetInitData,
                                       aEnableDragDrop, aResetVisibility);
}

nsresult nsIView::CreateWidgetForPopup(nsWidgetInitData *aWidgetInitData,
                                       nsIWidget* aParentWidget,
                                       PRBool aEnableDragDrop,
                                       PRBool aResetVisibility)
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
    clipChildren = PR_TRUE;
    clipSiblings = PR_TRUE;
  }
};

nsresult nsView::CreateWidget(nsWidgetInitData *aWidgetInitData,
                              PRBool aEnableDragDrop,
                              PRBool aResetVisibility)
{
  AssertNoWindow();
  NS_ABORT_IF_FALSE(!aWidgetInitData ||
                    aWidgetInitData->mWindowType != eWindowType_popup,
                    "Use CreateWidgetForPopup");

  DefaultWidgetInitData defaultInitData;
  PRBool initDataPassedIn = !!aWidgetInitData;
  aWidgetInitData = aWidgetInitData ? aWidgetInitData : &defaultInitData;
  defaultInitData.mListenForResizes =
    (!initDataPassedIn && GetParent() &&
     GetParent()->GetViewManager() != mViewManager);

  nsIntRect trect = CalcWidgetBounds(aWidgetInitData->mWindowType);

  nsCOMPtr<nsIDeviceContext> dx;
  mViewManager->GetDeviceContext(*getter_AddRefs(dx));

  nsIWidget* parentWidget =
    GetParent() ? GetParent()->GetNearestWidget(nsnull) : nsnull;
  if (!parentWidget) {
    NS_ERROR("nsView::CreateWidget without suitable parent widget??");
    return NS_ERROR_FAILURE;
  }

  
  
  mWindow = parentWidget->CreateChild(trect, ::HandleEvent,
                                      dx, nsnull, nsnull, aWidgetInitData,
                                      PR_TRUE).get();
  if (!mWindow) {
    return NS_ERROR_FAILURE;
  }
 
  InitializeWindow(aEnableDragDrop, aResetVisibility);

  return NS_OK;
}

nsresult nsView::CreateWidgetForParent(nsIWidget* aParentWidget,
                                       nsWidgetInitData *aWidgetInitData,
                                       PRBool aEnableDragDrop,
                                       PRBool aResetVisibility)
{
  AssertNoWindow();
  NS_ABORT_IF_FALSE(!aWidgetInitData ||
                    aWidgetInitData->mWindowType != eWindowType_popup,
                    "Use CreateWidgetForPopup");
  NS_ABORT_IF_FALSE(aParentWidget, "Parent widget required");

  DefaultWidgetInitData defaultInitData;
  aWidgetInitData = aWidgetInitData ? aWidgetInitData : &defaultInitData;

  nsIntRect trect = CalcWidgetBounds(aWidgetInitData->mWindowType);

  nsCOMPtr<nsIDeviceContext> dx;
  mViewManager->GetDeviceContext(*getter_AddRefs(dx));

  mWindow =
    aParentWidget->CreateChild(trect, ::HandleEvent,
                               dx, nsnull, nsnull, aWidgetInitData).get();
  if (!mWindow) {
    return NS_ERROR_FAILURE;
  }

  InitializeWindow(aEnableDragDrop, aResetVisibility);

  return NS_OK;
}

nsresult nsView::CreateWidgetForPopup(nsWidgetInitData *aWidgetInitData,
                                      nsIWidget* aParentWidget,
                                      PRBool aEnableDragDrop,
                                      PRBool aResetVisibility)
{
  AssertNoWindow();
  NS_ABORT_IF_FALSE(aWidgetInitData, "Widget init data required");
  NS_ABORT_IF_FALSE(aWidgetInitData->mWindowType == eWindowType_popup,
                    "Use one of the other CreateWidget methods");

  nsIntRect trect = CalcWidgetBounds(aWidgetInitData->mWindowType);

  nsCOMPtr<nsIDeviceContext> dx;
  mViewManager->GetDeviceContext(*getter_AddRefs(dx));

  
  
  
  
  if (aParentWidget) {
    
    
    mWindow = aParentWidget->CreateChild(trect, ::HandleEvent,
                                         dx, nsnull, nsnull, aWidgetInitData,
                                         PR_TRUE).get();
  }
  else {
    nsIWidget* nearestParent = GetParent() ? GetParent()->GetNearestWidget(nsnull)
                                           : nsnull;
    if (!nearestParent) {
      
      
      return NS_ERROR_FAILURE;
    }

    mWindow =
      nearestParent->CreateChild(trect, ::HandleEvent,
                                 dx, nsnull, nsnull, aWidgetInitData).get();
  }
  if (!mWindow) {
    return NS_ERROR_FAILURE;
  }

  InitializeWindow(aEnableDragDrop, aResetVisibility);

  return NS_OK;
}

void
nsView::InitializeWindow(PRBool aEnableDragDrop, PRBool aResetVisibility)
{
  NS_ABORT_IF_FALSE(mWindow, "Must have a window to initialize");

  ViewWrapper* wrapper = new ViewWrapper(this);
  NS_ADDREF(wrapper); 
  mWindow->SetClientData(wrapper);

  if (aEnableDragDrop) {
    mWindow->EnableDragDrop(PR_TRUE);
  }
      
  
  UpdateNativeWidgetZIndexes(this, FindNonAutoZIndex(this));

  

  if (aResetVisibility) {
    SetVisibility(GetVisibility());
  }
}


nsresult nsIView::AttachToTopLevelWidget(nsIWidget* aWidget)
{
  NS_PRECONDITION(nsnull != aWidget, "null widget ptr");
  
  
  nsIView *oldView = GetAttachedViewFor(aWidget);
  if (oldView) {
    oldView->DetachFromTopLevelWidget();
  }

  nsCOMPtr<nsIDeviceContext> dx;
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
  mWindow->EnableDragDrop(PR_TRUE);
  mWidgetIsTopLevel = PR_TRUE;

  
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

  mWindow->SetAttachedViewPtr(nsnull);
  NS_RELEASE(mWindow);

  mWidgetIsTopLevel = PR_FALSE;
  
  return NS_OK;
}

void nsView::SetZIndex(PRBool aAuto, PRInt32 aZIndex, PRBool aTopMost)
{
  PRBool oldIsAuto = GetZIndexIsAuto();
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
    mWindow->SetClientData(nsnull);
    mWindow->Destroy();
    NS_RELEASE(mWindow);
  }
}




EVENT_CALLBACK nsIView::AttachWidgetEventHandler(nsIWidget* aWidget)
{
#ifdef DEBUG
  void* data = nsnull;
  aWidget->GetClientData(data);
  NS_ASSERTION(!data, "Already got client data");
#endif

  ViewWrapper* wrapper = new ViewWrapper(Impl());
  if (!wrapper)
    return nsnull;
  NS_ADDREF(wrapper); 
  aWidget->SetClientData(wrapper);
  return ::HandleEvent;
}

void nsIView::DetachWidgetEventHandler(nsIWidget* aWidget)
{
  ViewWrapper* wrapper = GetWrapperFor(aWidget);
  NS_ASSERTION(!wrapper || wrapper->GetView() == this, "Wrong view");
  NS_IF_RELEASE(wrapper);
  aWidget->SetClientData(nsnull);
}

#ifdef DEBUG
void nsIView::List(FILE* out, PRInt32 aIndent) const
{
  PRInt32 i;
  for (i = aIndent; --i >= 0; ) fputs("  ", out);
  fprintf(out, "%p ", (void*)this);
  if (nsnull != mWindow) {
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
  fprintf(out, " z=%d vis=%d clientData=%p <\n",
          mZIndex, mVis, mClientData);
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
  const nsView* root = nsnull;
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
    return nsnull;
  }

  
  
  if (aOffset) {
    docPt += v->ViewToWidgetOffset();
    pt += docPt.ConvertAppUnits(currAPD, aAPD);
    *aOffset = pt;
  }
  return v->GetWidget();
}

PRBool nsIView::IsRoot() const
{
  NS_ASSERTION(mViewManager != nsnull," View manager is null in nsView::IsRoot()");
  return mViewManager->GetRootViewImpl() == this;
}

PRBool nsIView::ExternalIsRoot() const
{
  return nsIView::IsRoot();
}

void
nsIView::SetDeletionObserver(nsWeakView* aDeletionObserver)
{
  if (mDeletionObserver && aDeletionObserver) {
    aDeletionObserver->SetPrevious(mDeletionObserver);
  }
  mDeletionObserver = aDeletionObserver;
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
nsView::ConvertFromParentCoords(nsPoint aPt) const
{
  nsView* parent = GetParent();
  if (parent) {
    aPt = aPt.ConvertAppUnits(parent->GetViewManager()->AppUnitsPerDevPixel(),
                              GetViewManager()->AppUnitsPerDevPixel());
  }
  aPt -= GetPosition();
  return aPt;
}
