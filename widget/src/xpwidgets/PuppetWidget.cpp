







































#include "mozilla/dom/PBrowserChild.h"
#include "BasicLayers.h"

#include "gfxPlatform.h"
#include "PuppetWidget.h"

using namespace mozilla::layers;
using namespace mozilla::widget;
using namespace mozilla::dom;

static void
InvalidateRegion(nsIWidget* aWidget, const nsIntRegion& aRegion)
{
  nsIntRegionRectIterator it(aRegion);
  while(const nsIntRect* r = it.Next()) {
    aWidget->Invalidate(*r, PR_FALSE);
  }
}

 already_AddRefed<nsIWidget>
nsIWidget::CreatePuppetWidget(PBrowserChild *aTabChild)
{
  NS_ABORT_IF_FALSE(nsIWidget::UsePuppetWidgets(),
                    "PuppetWidgets not allowed in this configuration");

  nsCOMPtr<nsIWidget> widget = new PuppetWidget(aTabChild);
  return widget.forget();
}

namespace mozilla {
namespace widget {


const size_t PuppetWidget::kMaxDimension = 4000;

NS_IMPL_ISUPPORTS_INHERITED1(PuppetWidget, nsBaseWidget,
                             nsISupportsWeakReference)

PuppetWidget::PuppetWidget(PBrowserChild *aTabChild)
  : mTabChild(aTabChild)
{
  MOZ_COUNT_CTOR(PuppetWidget);
}

PuppetWidget::~PuppetWidget()
{
  MOZ_COUNT_DTOR(PuppetWidget);
}

NS_IMETHODIMP
PuppetWidget::Create(nsIWidget        *aParent,
                     nsNativeWidget   aNativeParent,
                     const nsIntRect  &aRect,
                     EVENT_CALLBACK   aHandleEventFunction,
                     nsIDeviceContext *aContext,
                     nsIAppShell      *aAppShell,
                     nsIToolkit       *aToolkit,
                     nsWidgetInitData *aInitData)
{
  NS_ABORT_IF_FALSE(!aNativeParent, "got a non-Puppet native parent");

  BaseCreate(nsnull, aRect, aHandleEventFunction, aContext,
             aAppShell, aToolkit, aInitData);

  mBounds = aRect;
  mEnabled = PR_TRUE;
  mVisible = PR_TRUE;

  mSurface = gfxPlatform::GetPlatform()
             ->CreateOffscreenSurface(gfxIntSize(1, 1),
                                      gfxASurface::ContentFromFormat(gfxASurface::ImageFormatARGB32));

  mIMEComposing = PR_FALSE;
  mIMESuppressNotifySel = PR_FALSE;

  PuppetWidget* parent = static_cast<PuppetWidget*>(aParent);
  if (parent) {
    parent->SetChild(this);
    mLayerManager = parent->GetLayerManager();
  }
  else {
    Resize(mBounds.x, mBounds.y, mBounds.width, mBounds.height, PR_FALSE);
  }

  return NS_OK;
}

already_AddRefed<nsIWidget>
PuppetWidget::CreateChild(const nsIntRect  &aRect,
                          EVENT_CALLBACK   aHandleEventFunction,
                          nsIDeviceContext *aContext,
                          nsIAppShell      *aAppShell,
                          nsIToolkit       *aToolkit,
                          nsWidgetInitData *aInitData,
                          PRBool           aForceUseIWidgetParent)
{
  bool isPopup = aInitData && aInitData->mWindowType == eWindowType_popup;

  nsCOMPtr<nsIWidget> widget = nsIWidget::CreatePuppetWidget(mTabChild);
  return ((widget &&
           NS_SUCCEEDED(widget->Create(isPopup ? nsnull: this, nsnull, aRect,
                                       aHandleEventFunction,
                                       aContext, aAppShell, aToolkit,
                                       aInitData))) ?
          widget.forget() : nsnull);
}

NS_IMETHODIMP
PuppetWidget::Destroy()
{
  Base::Destroy();
  mPaintTask.Revoke();
  mChild = nsnull;
  mLayerManager = nsnull;
  mTabChild = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
PuppetWidget::Show(PRBool aState)
{
  NS_ASSERTION(mEnabled,
               "does it make sense to Show()/Hide() a disabled widget?");

  PRBool wasVisible = mVisible;
  mVisible = aState;

  if (!wasVisible && mVisible) {
    Resize(mBounds.width, mBounds.height, PR_FALSE);
  }

  return NS_OK;
}

NS_IMETHODIMP
PuppetWidget::Resize(PRInt32 aWidth,
                     PRInt32 aHeight,
                     PRBool  aRepaint)
{
  NS_ASSERTION(mEnabled && mVisible,
               "does it make sense to Resize() a disabled or hidden widget?");

  nsIntRect oldBounds = mBounds;
  mBounds.SizeTo(nsIntSize(aWidth, aHeight));

  if (mChild) {
    return mChild->Resize(aWidth, aHeight, aRepaint);
  }

  
  
  if (oldBounds.Size() < mBounds.Size() && aRepaint) {
    nsIntRegion dirty(mBounds);
    dirty.Sub(dirty,  oldBounds);
    InvalidateRegion(this, dirty);
  }

  if (oldBounds != mBounds) {
    DispatchResizeEvent();
  }

  return NS_OK;
}

NS_IMETHODIMP
PuppetWidget::SetFocus(PRBool aRaise)
{
  
  
  return NS_OK;
}

NS_IMETHODIMP
PuppetWidget::Invalidate(const nsIntRect& aRect, PRBool aIsSynchronous)
{
#ifdef DEBUG
  debug_DumpInvalidate(stderr, this, &aRect, aIsSynchronous,
                       nsCAutoString("PuppetWidget"), nsnull);
#endif

  if (mChild) {
    return mChild->Invalidate(aRect, aIsSynchronous);
  }

  mDirtyRegion.Or(mDirtyRegion, aRect);

  if (mDirtyRegion.IsEmpty()) {
    return NS_OK;
  } else if (aIsSynchronous) {
    DispatchPaintEvent();
    return NS_OK;
  } else if (!mPaintTask.IsPending()) {
    mPaintTask = new PaintTask(this);
    return NS_DispatchToCurrentThread(mPaintTask.get());
  }

  return NS_OK;
}

NS_IMETHODIMP
PuppetWidget::Update()
{
  if (mChild) {
    return mChild->Update();
  }

  if (mDirtyRegion.IsEmpty()) {
    return NS_OK;
  }
  return DispatchPaintEvent();
}

void
PuppetWidget::InitEvent(nsGUIEvent& event, nsIntPoint* aPoint)
{
  if (nsnull == aPoint) {
    event.refPoint.x = 0;
    event.refPoint.y = 0;
  }
  else {
    
    event.refPoint.x = aPoint->x;
    event.refPoint.y = aPoint->y;
  }
  event.time = PR_Now() / 1000;
}

NS_IMETHODIMP
PuppetWidget::DispatchEvent(nsGUIEvent* event, nsEventStatus& aStatus)
{
#ifdef DEBUG
  debug_DumpEvent(stdout, event->widget, event,
                  nsCAutoString("PuppetWidget"), nsnull);
#endif

  aStatus = nsEventStatus_eIgnore;
  if (mEventCallback) {
    if (event->message == NS_COMPOSITION_START) {
      mIMEComposing = PR_TRUE;
    } else if (event->message == NS_SELECTION_SET) {
      mIMESuppressNotifySel = PR_TRUE;
    }
    aStatus = (*mEventCallback)(event);

    if (event->message == NS_COMPOSITION_END) {
      mIMEComposing = PR_FALSE;
    } else if (event->message == NS_SELECTION_SET) {
      mIMESuppressNotifySel = PR_FALSE;
    }
  } else if (mChild) {
    event->widget = mChild;
    mChild->DispatchEvent(event, aStatus);
  }

  return NS_OK;
}

LayerManager*
PuppetWidget::GetLayerManager()
{
  if (!mLayerManager) {
    mLayerManager = new BasicShadowLayerManager(this);
  }
  return mLayerManager;
}

gfxASurface*
PuppetWidget::GetThebesSurface()
{
  return mSurface;
}

nsresult
PuppetWidget::IMEEndComposition(PRBool aCancel)
{
  if (!mIMEComposing)
    return NS_OK;

  nsEventStatus status;
  nsTextEvent textEvent(PR_TRUE, NS_TEXT_TEXT, this);
  InitEvent(textEvent, nsnull);
  if (!mTabChild ||
      !mTabChild->SendEndIMEComposition(aCancel, &textEvent.theText)) {
    return NS_ERROR_FAILURE;
  }
  DispatchEvent(&textEvent, status);

  nsCompositionEvent compEvent(PR_TRUE, NS_COMPOSITION_END, this);
  InitEvent(compEvent, nsnull);
  DispatchEvent(&compEvent, status);
  return NS_OK;
}

NS_IMETHODIMP
PuppetWidget::ResetInputState()
{
  return IMEEndComposition(PR_FALSE);
}

NS_IMETHODIMP
PuppetWidget::CancelComposition()
{
  return IMEEndComposition(PR_TRUE);
}

NS_IMETHODIMP
PuppetWidget::SetIMEOpenState(PRBool aState)
{
  if (mTabChild &&
      mTabChild->SendSetIMEOpenState(aState))
    return NS_OK;
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
PuppetWidget::SetIMEEnabled(PRUint32 aState)
{
  if (mTabChild &&
      mTabChild->SendSetIMEEnabled(aState))
    return NS_OK;
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
PuppetWidget::GetIMEOpenState(PRBool *aState)
{
  if (mTabChild &&
      mTabChild->SendGetIMEOpenState(aState))
    return NS_OK;
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
PuppetWidget::GetIMEEnabled(PRUint32 *aState)
{
  if (mTabChild &&
      mTabChild->SendGetIMEEnabled(aState))
    return NS_OK;
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
PuppetWidget::OnIMEFocusChange(PRBool aFocus)
{
  if (!mTabChild)
    return NS_ERROR_FAILURE;

  if (aFocus) {
    nsEventStatus status;
    nsQueryContentEvent queryEvent(PR_TRUE, NS_QUERY_TEXT_CONTENT, this);
    InitEvent(queryEvent, nsnull);
    
    queryEvent.InitForQueryTextContent(0, PR_UINT32_MAX);
    DispatchEvent(&queryEvent, status);

    if (queryEvent.mSucceeded) {
      mTabChild->SendNotifyIMETextHint(queryEvent.mReply.mString);
    }
  } else {
    
    ResetInputState();
  }

  mIMEPreference.mWantUpdates = PR_FALSE;
  mIMEPreference.mWantHints = PR_FALSE;
  if (!mTabChild->SendNotifyIMEFocus(aFocus, &mIMEPreference))
    return NS_ERROR_FAILURE;

  if (aFocus) {
    if (!mIMEPreference.mWantUpdates && !mIMEPreference.mWantHints)
      
      return NS_SUCCESS_IME_NO_UPDATES;
    OnIMESelectionChange(); 
  }
  return NS_OK;
}

NS_IMETHODIMP
PuppetWidget::OnIMETextChange(PRUint32 aStart, PRUint32 aEnd, PRUint32 aNewEnd)
{
  if (!mTabChild)
    return NS_ERROR_FAILURE;

  if (mIMEPreference.mWantHints) {
    nsEventStatus status;
    nsQueryContentEvent queryEvent(PR_TRUE, NS_QUERY_TEXT_CONTENT, this);
    InitEvent(queryEvent, nsnull);
    queryEvent.InitForQueryTextContent(0, PR_UINT32_MAX);
    DispatchEvent(&queryEvent, status);

    if (queryEvent.mSucceeded) {
      mTabChild->SendNotifyIMETextHint(queryEvent.mReply.mString);
    }
  }
  if (mIMEPreference.mWantUpdates) {
    mTabChild->SendNotifyIMETextChange(aStart, aEnd, aNewEnd);
  }
  return NS_OK;
}

NS_IMETHODIMP
PuppetWidget::OnIMESelectionChange(void)
{
  if (!mTabChild)
    return NS_ERROR_FAILURE;

  
  
  
  
  if (mIMEComposing || mIMESuppressNotifySel)
    return NS_OK;

  if (mIMEPreference.mWantUpdates) {
    nsEventStatus status;
    nsQueryContentEvent queryEvent(PR_TRUE, NS_QUERY_SELECTED_TEXT, this);
    InitEvent(queryEvent, nsnull);
    DispatchEvent(&queryEvent, status);

    if (queryEvent.mSucceeded) {
      mTabChild->SendNotifyIMESelection(queryEvent.GetSelectionStart(),
                                        queryEvent.GetSelectionEnd());
    }
  }
  return NS_OK;
}

nsresult
PuppetWidget::DispatchPaintEvent()
{
  NS_ABORT_IF_FALSE(!mDirtyRegion.IsEmpty(), "paint event logic messed up");

  nsIntRect dirtyRect = mDirtyRegion.GetBounds();
  nsPaintEvent event(PR_TRUE, NS_PAINT, this);
  event.refPoint.x = dirtyRect.x;
  event.refPoint.x = dirtyRect.y;
  event.region = mDirtyRegion;
  event.willSendDidPaint = PR_TRUE;

  
  mDirtyRegion.SetEmpty();
  mPaintTask.Revoke();

  nsEventStatus status;
  {
#ifdef DEBUG
    debug_DumpPaintEvent(stderr, this, &event,
                         nsCAutoString("PuppetWidget"), nsnull);
#endif

    nsRefPtr<gfxContext> ctx = new gfxContext(mSurface);
    AutoLayerManagerSetup setupLayerManager(this, ctx,
                                            BasicLayerManager::BUFFER_NONE);
    DispatchEvent(&event, status);  
  }

  nsPaintEvent didPaintEvent(PR_TRUE, NS_DID_PAINT, this);
  DispatchEvent(&didPaintEvent, status);

  return NS_OK;
}

nsresult
PuppetWidget::DispatchResizeEvent()
{
  nsSizeEvent event(PR_TRUE, NS_SIZE, this);

  nsIntRect rect = mBounds;     
  event.windowSize = &rect;
  event.refPoint.x = rect.x;
  event.refPoint.y = rect.y;
  event.mWinWidth = rect.width;
  event.mWinHeight = rect.height;

  nsEventStatus status;
  return DispatchEvent(&event, status);
}

void
PuppetWidget::SetChild(PuppetWidget* aChild)
{
  NS_ABORT_IF_FALSE(this != aChild, "can't parent a widget to itself");
  NS_ABORT_IF_FALSE(!aChild->mChild,
                    "fake widget 'hierarchy' only expected to have one level");

  mChild = aChild;
}

NS_IMETHODIMP
PuppetWidget::PaintTask::Run()
{
  if (mWidget) {
    mWidget->DispatchPaintEvent();
  }
  return NS_OK;
}

}  
}  
