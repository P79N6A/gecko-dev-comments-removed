




































#include "nsView.h"
#include "nsIWidget.h"
#include "nsViewManager.h"
#include "nsIWidget.h"
#include "nsGUIEvent.h"
#include "nsIDeviceContext.h"
#include "nsIComponentManager.h"
#include "nsIRenderingContext.h"
#include "nsTransform2D.h"
#include "nsIScrollableView.h"
#include "nsGfxCIID.h"
#include "nsIRegion.h"
#include "nsIInterfaceRequestor.h"




static nsEventStatus PR_CALLBACK HandleEvent(nsGUIEvent *aEvent);






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
    *aInstancePtr = NS_STATIC_CAST(nsISupports*, this);
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




nsEventStatus PR_CALLBACK HandleEvent(nsGUIEvent *aEvent)
{ 


  nsEventStatus result = nsEventStatus_eIgnore;
  nsView       *view = nsView::GetViewFor(aEvent->widget);

  if (view)
  {
    view->GetViewManager()->DispatchEvent(aEvent, &result);
  }

  return result;
}

nsView::nsView(nsViewManager* aViewManager, nsViewVisibility aVisibility)
{
  MOZ_COUNT_CTOR(nsView);

  mVis = aVisibility;
  
  
  
  
  mVFlags = 0;
  mViewManager = aViewManager;
  mChildRemoved = PR_FALSE;
  mDirtyRegion = nsnull;
}

void nsView::DropMouseGrabbing() {
  
  if (mViewManager->GetMouseEventGrabber() == this) {
    
    PRBool boolResult; 
    
    mViewManager->GrabMouseEvents(GetParent(), boolResult);
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

  if (mZParent)
  {
    mZParent->RemoveReparentedView();
    mZParent->Destroy();
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
  delete mClipRect;
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

void nsView::DoResetWidgetBounds(PRBool aMoveOnly,
                                 PRBool aInvalidateChangedSize) {
  
  
  if (mViewManager->GetRootView() == this) {
    return;
  }
  
  nsRect curBounds;
  mWindow->GetBounds(curBounds);
  nsWindowType type;
  mWindow->GetWindowType(type);

  if (curBounds.IsEmpty() && mDimBounds.IsEmpty() && type == eWindowType_popup) {
    
    
    
    
    
    return;
  }

  NS_PRECONDITION(mWindow, "Why was this called??");
  nsIDeviceContext  *dx;
  
  mViewManager->GetDeviceContext(dx);
  PRInt32 p2a = dx->AppUnitsPerDevPixel();
  NS_RELEASE(dx);

  nsPoint offset(0, 0);
  if (GetParent()) {
    nsIWidget* parentWidget = GetParent()->GetNearestWidget(&offset);
    
    if (type == eWindowType_popup) {
      
      nsRect screenRect(0,0,1,1);
      parentWidget->WidgetToScreen(screenRect, screenRect);
      offset += nsPoint(NSIntPixelsToAppUnits(screenRect.x, p2a),
                        NSIntPixelsToAppUnits(screenRect.y, p2a));
    }
  }

  nsRect newBounds(NSAppUnitsToIntPixels((mDimBounds.x + offset.x), p2a),
                   NSAppUnitsToIntPixels((mDimBounds.y + offset.y), p2a),
                   NSAppUnitsToIntPixels(mDimBounds.width, p2a),
                   NSAppUnitsToIntPixels(mDimBounds.height, p2a));
    
  PRBool changedPos = PR_TRUE;
  PRBool changedSize = PR_TRUE;
  if (!(mVFlags & NS_VIEW_FLAG_HAS_POSITIONED_WIDGET)) {
    mVFlags |= NS_VIEW_FLAG_HAS_POSITIONED_WIDGET;
  } else {
    changedPos = curBounds.TopLeft() != newBounds.TopLeft();
    changedSize = curBounds.Size() != newBounds.Size();
  }

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

NS_IMETHODIMP nsView::SetVisibility(nsViewVisibility aVisibility)
{

  mVis = aVisibility;

  if (aVisibility == nsViewVisibility_kHide)
  {
    DropMouseGrabbing();
  }

  if (nsnull != mWindow)
  {
#ifndef HIDE_ALL_WIDGETS
    if (mVis == nsViewVisibility_kShow)
    {
      DoResetWidgetBounds(PR_FALSE, PR_TRUE);
      mWindow->Show(PR_TRUE);
    }
    else
#endif
      mWindow->Show(PR_FALSE);
  }

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
  if (aViewManagerParent) {
    
    
    if (aViewManagerParent->GetMouseEventGrabber() == this) {
      PRBool res;
      aViewManagerParent->GrabMouseEvents(nsnull, res);
    }
  }

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
      mChildRemoved = PR_TRUE;
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
                               nsContentType aContentType)
{
  nsIDeviceContext  *dx;
  nsRect            trect = mDimBounds;

  if (NS_UNLIKELY(mWindow)) {
    NS_ERROR("We already have a window for this view? BAD");
    ViewWrapper* wrapper = GetWrapperFor(mWindow);
    NS_IF_RELEASE(wrapper);
    mWindow->SetClientData(nsnull);
    NS_RELEASE(mWindow);
  }

  mViewManager->GetDeviceContext(dx);
  float scale = 1.0f / dx->AppUnitsPerDevPixel();

  trect *= scale;

  nsView* v = NS_STATIC_CAST(nsView*, this);
  if (NS_OK == v->LoadWidget(aWindowIID))
  {
    PRBool usewidgets;

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

      if (aNative)
        mWindow->Create(aNative, trect, ::HandleEvent, dx, nsnull, nsnull, aWidgetInitData);
      else
      {
        if (!initDataPassedIn && GetParent() && 
          GetParent()->GetViewManager() != mViewManager)
          initData.mListenForResizes = PR_TRUE;

        nsPoint offset(0, 0);
        nsIWidget* parentWidget = GetParent() ? GetParent()->GetNearestWidget(&offset)
          : nsnull;
        trect += offset;
        if (aWidgetInitData->mWindowType == eWindowType_popup) {
          
          
          if (!parentWidget)
            return NS_ERROR_FAILURE;
          mWindow->Create(parentWidget->GetNativeData(NS_NATIVE_WIDGET), trect,
                          ::HandleEvent, dx, nsnull, nsnull, aWidgetInitData);
        } else {
          mWindow->Create(parentWidget, trect,
                          ::HandleEvent, dx, nsnull, nsnull, aWidgetInitData);
        }
      }
      if (aEnableDragDrop) {
        mWindow->EnableDragDrop(PR_TRUE);
      }
      
      
      UpdateNativeWidgetZIndexes(v, FindNonAutoZIndex(v));
    }
  }

  
  
  if (aResetVisibility) {
    v->SetVisibility(GetVisibility());
  }

  NS_RELEASE(dx);

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

NS_IMETHODIMP nsView::SetWidget(nsIWidget *aWidget)
{
  ViewWrapper* wrapper = new ViewWrapper(this);
  if (!wrapper)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(wrapper); 

  
  ViewWrapper* oldWrapper = GetWrapperFor(aWidget);
  NS_IF_RELEASE(oldWrapper);
  NS_IF_RELEASE(mWindow);

  mWindow = aWidget;

  if (nsnull != mWindow)
  {
    NS_ADDREF(mWindow);
    mWindow->SetClientData(wrapper);
  }

  mVFlags &= ~NS_VIEW_FLAG_HAS_POSITIONED_WIDGET;

  UpdateNativeWidgetZIndexes(this, FindNonAutoZIndex(this));

  return NS_OK;
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

  mVFlags &= ~NS_VIEW_FLAG_HAS_POSITIONED_WIDGET;
  return rv;
}

#ifdef DEBUG
void nsIView::List(FILE* out, PRInt32 aIndent) const
{
  PRInt32 i;
  for (i = aIndent; --i >= 0; ) fputs("  ", out);
  fprintf(out, "%p ", (void*)this);
  if (nsnull != mWindow) {
    nsRect windowBounds;
    nsRect nonclientBounds;
    float p2t;
    nsIDeviceContext *dx;
    mViewManager->GetDeviceContext(dx);
    p2t = dx->AppUnitsPerDevPixel();
    NS_RELEASE(dx);
    mWindow->GetClientBounds(windowBounds);
    windowBounds *= p2t;
    mWindow->GetBounds(nonclientBounds);
    nonclientBounds *= p2t;
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
  const nsView* v = NS_STATIC_CAST(const nsView*, this);
  if (v->IsZPlaceholderView()) {
    fprintf(out, " z-placeholder(%p)",
            (void*)NS_STATIC_CAST(const nsZPlaceholderView*, this)->GetReparentedView());
  }
  if (v->GetZParent()) {
    fprintf(out, " zparent=%p", (void*)v->GetZParent());
  }
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
  nsIntRect screenRect(0,0,0,0);  
  nsPoint toWidgetOffset(0,0);
  nsIWidget* widget = GetNearestWidget(&toWidgetOffset);
  if (widget) {
    nsCOMPtr<nsIDeviceContext> dx;
    mViewManager->GetDeviceContext(*getter_AddRefs(dx));
    PRInt32 p2a = dx->AppUnitsPerDevPixel();
    nsIntRect ourRect(NSAppUnitsToIntPixels(toWidgetOffset.x, p2a),
                      NSAppUnitsToIntPixels(toWidgetOffset.y, p2a),
                      0,
                      0);
    widget->WidgetToScreen(ourRect, screenRect);
  }
  
  return nsIntPoint(screenRect.x, screenRect.y);
}

nsIWidget* nsIView::GetNearestWidget(nsPoint* aOffset) const
{
  nsPoint pt(0, 0);
  const nsView* v;
  for (v = NS_STATIC_CAST(const nsView*, this);
       v && !v->HasWidget(); v = v->GetParent()) {
    pt += v->GetPosition();
  }
  if (!v) {
    if (aOffset) {
      *aOffset = pt;
    }
    return NS_STATIC_CAST(const nsView*, this)->GetViewManager()->GetWidget();
  }

  
  
  
  if (aOffset) {
    nsRect vBounds = v->GetBounds();
    *aOffset = pt + v->GetPosition() -  nsPoint(vBounds.x, vBounds.y);
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
