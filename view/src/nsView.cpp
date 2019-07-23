




































#include "nsView.h"
#include "nsIWidget.h"
#include "nsViewManager.h"
#include "nsGUIEvent.h"
#include "nsIDeviceContext.h"
#include "nsIComponentManager.h"
#include "nsIScrollableView.h"
#include "nsGfxCIID.h"
#include "nsIRegion.h"
#include "nsIInterfaceRequestor.h"



static nsEventStatus HandleEvent(nsGUIEvent *aEvent);





#define VIEW_WRAPPER_IID \
{ 0x34297a07, 0xa8fd, 0xd811, { 0x87, 0xc6, 0x0, 0x2, 0x44, 0x21, 0x2b, 0xcb } }





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

  NS_ASSERTION(!aIID.Equals(NS_GET_IID(nsIView)) &&
               !aIID.Equals(NS_GET_IID(nsIScrollableView)),
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
  if (aIID.Equals(NS_GET_IID(nsIScrollableView))) {
    *aInstancePtr = mView->ToScrollableView();
    return NS_OK;
  }
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




nsEventStatus HandleEvent(nsGUIEvent *aEvent)
{ 


  nsEventStatus result = nsEventStatus_eIgnore;
  nsView       *view = nsView::GetViewFor(aEvent->widget);

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
  
    nsView *rootView = mViewManager->GetRootView();
    
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

  
  if (mWindow)
  {
    
    ViewWrapper* wrapper = GetWrapperFor(mWindow);
    NS_IF_RELEASE(wrapper);

    mWindow->SetClientData(nsnull);
    mWindow->Destroy();
    NS_RELEASE(mWindow);
  }
  delete mDirtyRegion;

  if (mDeletionObserver) {
    mDeletionObserver->Clear();
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

PRBool nsView::IsEffectivelyVisible()
{
  for (nsView* v = this; v; v = v->mParent) {
    if (v->GetVisibility() == nsViewVisibility_kHide)
      return PR_FALSE;
  }
  return PR_TRUE;
}

nsIntRect nsView::CalcWidgetBounds(nsWindowType aType)
{
  nsCOMPtr<nsIDeviceContext> dx;
  mViewManager->GetDeviceContext(*getter_AddRefs(dx));
  NS_ASSERTION(dx, "View manager can't be created without a device context");
  PRInt32 p2a = dx->AppUnitsPerDevPixel();

  nsRect viewBounds(mDimBounds);

  if (GetParent()) {
    
    nsPoint offset;
    nsIWidget* parentWidget = GetParent()->GetNearestWidget(&offset);
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
  mViewToWidgetOffset = viewBounds.TopLeft() - roundedOffset;

  return newBounds;
}

void nsView::DoResetWidgetBounds(PRBool aMoveOnly,
                                 PRBool aInvalidateChangedSize) {
  
  
  if (mViewManager->GetRootView() == this) {
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
  if (mViewManager->GetRootView() == this)
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
    if (vm->GetRootView() == aChild)
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
    if (vm->GetRootView() == child)
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

nsresult nsIView::CreateWidget(const nsIID &aWindowIID,
                               nsWidgetInitData *aWidgetInitData,
                               nsNativeWidget aNative,
                               PRBool aEnableDragDrop,
                               PRBool aResetVisibility,
                               nsContentType aContentType,
                               nsIWidget* aParentWidget)
{
  if (NS_UNLIKELY(mWindow)) {
    NS_ERROR("We already have a window for this view? BAD");
    ViewWrapper* wrapper = GetWrapperFor(mWindow);
    NS_IF_RELEASE(wrapper);
    mWindow->SetClientData(nsnull);
    mWindow->Destroy();
    NS_RELEASE(mWindow);
  }

  nsView* v = static_cast<nsView*>(this);

  nsIntRect trect = v->CalcWidgetBounds(aWidgetInitData
                                        ? aWidgetInitData->mWindowType
                                        : eWindowType_child);

  if (NS_OK == v->LoadWidget(aWindowIID))
  {
    PRBool usewidgets;
    nsCOMPtr<nsIDeviceContext> dx;
    mViewManager->GetDeviceContext(*getter_AddRefs(dx));
    dx->SupportsNativeWidgets(usewidgets);

    if (PR_TRUE == usewidgets)
    {
      PRBool initDataPassedIn = PR_TRUE;
      nsWidgetInitData initData;
      if (!aWidgetInitData) {
        
        
        initDataPassedIn = PR_FALSE;
        initData.clipChildren = PR_TRUE; 
        initData.clipSiblings = PR_TRUE; 
        aWidgetInitData = &initData;
      }
      aWidgetInitData->mContentType = aContentType;

      if (aNative && aWidgetInitData->mWindowType != eWindowType_popup)
        mWindow->Create(nsnull, aNative, trect, ::HandleEvent, dx, nsnull, nsnull, aWidgetInitData);
      else
      {
        if (!initDataPassedIn && GetParent() && 
          GetParent()->GetViewManager() != mViewManager)
          initData.mListenForResizes = PR_TRUE;
        if (aParentWidget) {
          NS_ASSERTION(aWidgetInitData->mWindowType == eWindowType_popup,
                       "popup widget type expected");
          mWindow->Create(aParentWidget, nsnull, trect,
                          ::HandleEvent, dx, nsnull, nsnull, aWidgetInitData);
        }
        else {
          nsIWidget* parentWidget = GetParent() ? GetParent()->GetNearestWidget(nsnull)
                                                : nsnull;
          if (aWidgetInitData->mWindowType == eWindowType_popup) {
            
            
            if (!parentWidget)
              return NS_ERROR_FAILURE;
            mWindow->Create(nsnull, parentWidget->GetNativeData(NS_NATIVE_WIDGET), trect,
                            ::HandleEvent, dx, nsnull, nsnull, aWidgetInitData);
          } else {
            mWindow->Create(parentWidget, nsnull, trect,
                            ::HandleEvent, dx, nsnull, nsnull, aWidgetInitData);
          }
        }
      }
      if (aEnableDragDrop) {
        mWindow->EnableDragDrop(PR_TRUE);
      }
      
      
      UpdateNativeWidgetZIndexes(v, FindNonAutoZIndex(v));
    } else {
      
      
      
      mWindow->Resize(trect.x, trect.y, trect.width, trect.height,
                      PR_FALSE);
    }
  }

  

  if (aResetVisibility) {
    v->SetVisibility(GetVisibility());
  }

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




nsresult nsView::LoadWidget(const nsCID &aClassIID)
{
  ViewWrapper* wrapper = new ViewWrapper(this);
  if (!wrapper)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(wrapper); 

  nsresult rv = CallCreateInstance(aClassIID, &mWindow);

  if (NS_SUCCEEDED(rv)) {
    
    mWindow->SetClientData(wrapper);
  } else {
    delete wrapper;
  }

  return rv;
}

EVENT_CALLBACK nsIView::AttachWidgetEventHandler(nsIWidget* aWidget)
{
#ifdef DEBUG
  void* data = nsnull;
  aWidget->GetClientData(data);
  NS_ASSERTION(!data, "Already got client data");
#endif

  nsView* v = static_cast<nsView*>(this);
  ViewWrapper* wrapper = new ViewWrapper(v);
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
    nsIDeviceContext *dx;
    mViewManager->GetDeviceContext(dx);
    nscoord p2a = dx->AppUnitsPerDevPixel();
    NS_RELEASE(dx);
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
  nsPoint offset(0, 0);
  const nsIView* v;
  for (v = this; v != aOther && v; v = v->GetParent()) {
    offset += v->GetPosition();
  }

  if (v != aOther) {
    
    
    
    while (aOther) {
      offset -= aOther->GetPosition();
      aOther = aOther->GetParent();
    }
  }

  return offset;
}

nsIntPoint nsIView::GetScreenPosition() const
{
  nsIntPoint screenPoint(0,0);  
  nsPoint toWidgetOffset(0,0);
  nsIWidget* widget = GetNearestWidget(&toWidgetOffset);
  if (widget) {
    nsCOMPtr<nsIDeviceContext> dx;
    mViewManager->GetDeviceContext(*getter_AddRefs(dx));
    PRInt32 p2a = dx->AppUnitsPerDevPixel();
    nsIntPoint ourPoint(NSAppUnitsToIntPixels(toWidgetOffset.x, p2a),
                        NSAppUnitsToIntPixels(toWidgetOffset.y, p2a));
    screenPoint = ourPoint + widget->WidgetToScreenOffset();
  }
  
  return screenPoint;
}

nsIWidget* nsIView::GetNearestWidget(nsPoint* aOffset) const
{
  nsPoint pt(0, 0);
  const nsView* v;
  for (v = static_cast<const nsView*>(this);
       v && !v->HasWidget(); v = v->GetParent()) {
    pt += v->GetPosition();
  }
  if (!v) {
    if (aOffset) {
      *aOffset = pt;
    }
    return nsnull;
  }

  
  
  
  if (aOffset) {
    nsRect vBounds = v->GetBounds();
    *aOffset = pt + v->GetPosition() -  nsPoint(vBounds.x, vBounds.y) +
               v->ViewToWidgetOffset();
  }
  return v->GetWidget();
}

PRBool nsIView::IsRoot() const
{
  NS_ASSERTION(mViewManager != nsnull," View manager is null in nsView::IsRoot()");
  return mViewManager->GetRootView() == this;
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




PRBool nsIView::NeedsInvalidateFrameOnScroll() const
{
  for (const nsIView *currView = this; currView != nsnull; currView = currView->GetParent())
    if (currView->mVFlags & NS_VIEW_FLAG_INVALIDATE_ON_SCROLL)
      return PR_TRUE;
  
  return PR_FALSE;
}
