







































#include "mozilla/dom/PBrowserChild.h"
#include "BasicLayers.h"
#if defined(MOZ_ENABLE_D3D10_LAYER)
# include "LayerManagerD3D10.h"
#endif

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
    aWidget->Invalidate(*r, false);
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

static bool
IsPopup(const nsWidgetInitData* aInitData)
{
  return aInitData && aInitData->mWindowType == eWindowType_popup;
}

static bool
MightNeedIMEFocus(const nsWidgetInitData* aInitData)
{
  
  
  return !IsPopup(aInitData);
}



const size_t PuppetWidget::kMaxDimension = 4000;

NS_IMPL_ISUPPORTS_INHERITED1(PuppetWidget, nsBaseWidget,
                             nsISupportsWeakReference)

PuppetWidget::PuppetWidget(PBrowserChild *aTabChild)
  : mTabChild(aTabChild)
  , mDPI(-1)
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
                     nsDeviceContext *aContext,
                     nsWidgetInitData *aInitData)
{
  NS_ABORT_IF_FALSE(!aNativeParent, "got a non-Puppet native parent");

  BaseCreate(nsnull, aRect, aHandleEventFunction, aContext, aInitData);

  mBounds = aRect;
  mEnabled = true;
  mVisible = true;

  mSurface = gfxPlatform::GetPlatform()
             ->CreateOffscreenSurface(gfxIntSize(1, 1),
                                      gfxASurface::ContentFromFormat(gfxASurface::ImageFormatARGB32));

  mIMEComposing = false;
  if (MightNeedIMEFocus(aInitData)) {
    PRUint32 chromeSeqno;
    mTabChild->SendNotifyIMEFocus(false, &mIMEPreference, &chromeSeqno);
    mIMELastBlurSeqno = mIMELastReceivedSeqno = chromeSeqno;
  }

  PuppetWidget* parent = static_cast<PuppetWidget*>(aParent);
  if (parent) {
    parent->SetChild(this);
    mLayerManager = parent->GetLayerManager();
  }
  else {
    Resize(mBounds.x, mBounds.y, mBounds.width, mBounds.height, false);
  }

  return NS_OK;
}

already_AddRefed<nsIWidget>
PuppetWidget::CreateChild(const nsIntRect  &aRect,
                          EVENT_CALLBACK   aHandleEventFunction,
                          nsDeviceContext *aContext,
                          nsWidgetInitData *aInitData,
                          bool             aForceUseIWidgetParent)
{
  bool isPopup = IsPopup(aInitData);
  nsCOMPtr<nsIWidget> widget = nsIWidget::CreatePuppetWidget(mTabChild);
  return ((widget &&
           NS_SUCCEEDED(widget->Create(isPopup ? nsnull: this, nsnull, aRect,
                                       aHandleEventFunction,
                                       aContext, aInitData))) ?
          widget.forget() : nsnull);
}

NS_IMETHODIMP
PuppetWidget::Destroy()
{
  Base::OnDestroy();
  Base::Destroy();
  mPaintTask.Revoke();
  mChild = nsnull;
  if (mLayerManager) {
    mLayerManager->Destroy();
  }
  mLayerManager = nsnull;
  mTabChild = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
PuppetWidget::Show(bool aState)
{
  NS_ASSERTION(mEnabled,
               "does it make sense to Show()/Hide() a disabled widget?");

  bool wasVisible = mVisible;
  mVisible = aState;

  if (!wasVisible && mVisible) {
    Resize(mBounds.width, mBounds.height, false);
  }

  return NS_OK;
}

NS_IMETHODIMP
PuppetWidget::Resize(PRInt32 aWidth,
                     PRInt32 aHeight,
                     bool    aRepaint)
{
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

  if (!oldBounds.IsEqualEdges(mBounds)) {
    DispatchResizeEvent();
  }

  return NS_OK;
}

NS_IMETHODIMP
PuppetWidget::SetFocus(bool aRaise)
{
  
  
  return NS_OK;
}

NS_IMETHODIMP
PuppetWidget::Invalidate(const nsIntRect& aRect, bool aIsSynchronous)
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

  NS_ABORT_IF_FALSE(!mChild || mChild->mWindowType == eWindowType_popup,
                    "Unexpected event dispatch!");

  aStatus = nsEventStatus_eIgnore;

  NS_ABORT_IF_FALSE(mViewCallback, "No view callback!");

  if (event->message == NS_COMPOSITION_START) {
    mIMEComposing = true;
  }
  switch (event->eventStructType) {
  case NS_COMPOSITION_EVENT:
    mIMELastReceivedSeqno = static_cast<nsCompositionEvent*>(event)->seqno;
    if (mIMELastReceivedSeqno < mIMELastBlurSeqno)
      return NS_OK;
    break;
  case NS_TEXT_EVENT:
    mIMELastReceivedSeqno = static_cast<nsTextEvent*>(event)->seqno;
    if (mIMELastReceivedSeqno < mIMELastBlurSeqno)
      return NS_OK;
    break;
  case NS_SELECTION_EVENT:
    mIMELastReceivedSeqno = static_cast<nsSelectionEvent*>(event)->seqno;
    if (mIMELastReceivedSeqno < mIMELastBlurSeqno)
      return NS_OK;
    break;
  }
  aStatus = (*mViewCallback)(event);

  if (event->message == NS_COMPOSITION_END) {
    mIMEComposing = false;
  }

  return NS_OK;
}

LayerManager*
PuppetWidget::GetLayerManager(PLayersChild* aShadowManager,
                              LayersBackend aBackendHint,
                              LayerManagerPersistence aPersistence,
                              bool* aAllowRetaining)
{
  if (!mLayerManager) {
    
    
#if defined(MOZ_ENABLE_D3D10_LAYER)
    if (LayerManager::LAYERS_D3D10 == aBackendHint) {
      nsRefPtr<LayerManagerD3D10> m = new LayerManagerD3D10(this);
      m->AsShadowForwarder()->SetShadowManager(aShadowManager);
      if (m->Initialize()) {
        mLayerManager = m;
      }
    }
#endif
    if (!mLayerManager) {
      mLayerManager = new BasicShadowLayerManager(this);
      mLayerManager->AsShadowForwarder()->SetShadowManager(aShadowManager);
    }
  }
  if (aAllowRetaining) {
    *aAllowRetaining = true;
  }
  return mLayerManager;
}

gfxASurface*
PuppetWidget::GetThebesSurface()
{
  return mSurface;
}

nsresult
PuppetWidget::IMEEndComposition(bool aCancel)
{
  nsEventStatus status;
  nsTextEvent textEvent(true, NS_TEXT_TEXT, this);
  InitEvent(textEvent, nsnull);
  textEvent.seqno = mIMELastReceivedSeqno;
  
  
  if (!mTabChild ||
      !mTabChild->SendEndIMEComposition(aCancel, &textEvent.theText)) {
    return NS_ERROR_FAILURE;
  }

  if (!mIMEComposing)
    return NS_OK;

  DispatchEvent(&textEvent, status);

  nsCompositionEvent compEvent(true, NS_COMPOSITION_END, this);
  InitEvent(compEvent, nsnull);
  compEvent.seqno = mIMELastReceivedSeqno;
  DispatchEvent(&compEvent, status);
  return NS_OK;
}

NS_IMETHODIMP
PuppetWidget::ResetInputState()
{
  return IMEEndComposition(false);
}

NS_IMETHODIMP
PuppetWidget::CancelComposition()
{
  return IMEEndComposition(true);
}

NS_IMETHODIMP
PuppetWidget::SetIMEOpenState(bool aState)
{
  if (mTabChild &&
      mTabChild->SendSetIMEOpenState(aState))
    return NS_OK;
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
PuppetWidget::SetInputMode(const InputContext& aContext)
{
  if (mTabChild &&
      mTabChild->SendSetInputMode(aContext.mIMEEnabled,
                                  aContext.mHTMLInputType,
                                  aContext.mActionHint, aContext.mReason))
    return NS_OK;
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
PuppetWidget::GetIMEOpenState(bool *aState)
{
  if (mTabChild &&
      mTabChild->SendGetIMEOpenState(aState))
    return NS_OK;
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
PuppetWidget::GetInputMode(InputContext& aContext)
{
  if (mTabChild &&
      mTabChild->SendGetIMEEnabled(&aContext.mIMEEnabled))
    return NS_OK;
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
PuppetWidget::OnIMEFocusChange(bool aFocus)
{
  if (!mTabChild)
    return NS_ERROR_FAILURE;

  if (aFocus) {
    nsEventStatus status;
    nsQueryContentEvent queryEvent(true, NS_QUERY_TEXT_CONTENT, this);
    InitEvent(queryEvent, nsnull);
    
    queryEvent.InitForQueryTextContent(0, PR_UINT32_MAX);
    DispatchEvent(&queryEvent, status);

    if (queryEvent.mSucceeded) {
      mTabChild->SendNotifyIMETextHint(queryEvent.mReply.mString);
    }
  } else {
    
    ResetInputState();
  }

  PRUint32 chromeSeqno;
  mIMEPreference.mWantUpdates = false;
  mIMEPreference.mWantHints = false;
  if (!mTabChild->SendNotifyIMEFocus(aFocus, &mIMEPreference, &chromeSeqno))
    return NS_ERROR_FAILURE;

  if (aFocus) {
    if (!mIMEPreference.mWantUpdates && !mIMEPreference.mWantHints)
      
      return NS_SUCCESS_IME_NO_UPDATES;
    OnIMESelectionChange(); 
  } else {
    mIMELastBlurSeqno = chromeSeqno;
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
    nsQueryContentEvent queryEvent(true, NS_QUERY_TEXT_CONTENT, this);
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

  if (mIMEPreference.mWantUpdates) {
    nsEventStatus status;
    nsQueryContentEvent queryEvent(true, NS_QUERY_SELECTED_TEXT, this);
    InitEvent(queryEvent, nsnull);
    DispatchEvent(&queryEvent, status);

    if (queryEvent.mSucceeded) {
      mTabChild->SendNotifyIMESelection(mIMELastReceivedSeqno,
                                        queryEvent.GetSelectionStart(),
                                        queryEvent.GetSelectionEnd());
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
PuppetWidget::SetCursor(nsCursor aCursor)
{
  if (!mTabChild ||
      !mTabChild->SendSetCursor(aCursor)) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

nsresult
PuppetWidget::DispatchPaintEvent()
{
  NS_ABORT_IF_FALSE(!mDirtyRegion.IsEmpty(), "paint event logic messed up");

  nsIntRect dirtyRect = mDirtyRegion.GetBounds();
  nsPaintEvent event(true, NS_PAINT, this);
  event.refPoint.x = dirtyRect.x;
  event.refPoint.x = dirtyRect.y;
  event.region = mDirtyRegion;
  event.willSendDidPaint = true;

  
  mDirtyRegion.SetEmpty();
  mPaintTask.Revoke();

  nsEventStatus status;
  {
#ifdef DEBUG
    debug_DumpPaintEvent(stderr, this, &event,
                         nsCAutoString("PuppetWidget"), nsnull);
#endif

    if (LayerManager::LAYERS_D3D10 == mLayerManager->GetBackendType()) {
      DispatchEvent(&event, status);
    } else {
      nsRefPtr<gfxContext> ctx = new gfxContext(mSurface);
      AutoLayerManagerSetup setupLayerManager(this, ctx,
                                              BasicLayerManager::BUFFER_NONE);
      DispatchEvent(&event, status);  
    }
  }

  nsPaintEvent didPaintEvent(true, NS_DID_PAINT, this);
  DispatchEvent(&didPaintEvent, status);

  return NS_OK;
}

nsresult
PuppetWidget::DispatchResizeEvent()
{
  nsSizeEvent event(true, NS_SIZE, this);

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

float
PuppetWidget::GetDPI()
{
  if (mDPI < 0) {
    NS_ABORT_IF_FALSE(mTabChild, "Need TabChild to get the DPI from!");
    mTabChild->SendGetDPI(&mDPI);
  }

  return mDPI;
}

void*
PuppetWidget::GetNativeData(PRUint32 aDataType)
{
    switch (aDataType) {
    case NS_NATIVE_SHAREABLE_WINDOW: {
        NS_ABORT_IF_FALSE(mTabChild, "Need TabChild to get the nativeWindow from!");
        mozilla::WindowsHandle nativeData = nsnull;
        mTabChild->SendGetWidgetNativeData(&nativeData);
        return (void*)nativeData;
    }
    case NS_NATIVE_WINDOW:
    case NS_NATIVE_DISPLAY:
    case NS_NATIVE_PLUGIN_PORT:
    case NS_NATIVE_GRAPHIC:
    case NS_NATIVE_SHELLWIDGET:
    case NS_NATIVE_WIDGET:
        NS_WARNING("nsWindow::GetNativeData not implemented for this type");
        break;
    default:
        NS_WARNING("nsWindow::GetNativeData called with bad value");
        break;
    }
    return nsnull;
}

}  
}  
