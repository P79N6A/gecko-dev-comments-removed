







































#include "BasicLayers.h"

#include "gfxPlatform.h"
#include "PuppetWidget.h"

using namespace mozilla::layers;
using namespace mozilla::widget;

static void
InvalidateRegion(nsIWidget* aWidget, const nsIntRegion& aRegion)
{
  nsIntRegionRectIterator it(aRegion);
  while(const nsIntRect* r = it.Next()) {
    aWidget->Invalidate(*r, PR_FALSE);
  }
}

 already_AddRefed<nsIWidget>
nsIWidget::CreatePuppetWidget()
{
  NS_ABORT_IF_FALSE(nsIWidget::UsePuppetWidgets(),
                    "PuppetWidgets not allowed in this configuration");

  nsCOMPtr<nsIWidget> widget = new PuppetWidget();
  return widget.forget();
}

namespace mozilla {
namespace widget {


const size_t PuppetWidget::kMaxDimension = 4000;

NS_IMPL_ISUPPORTS_INHERITED1(PuppetWidget, nsBaseWidget,
                             nsISupportsWeakReference)

PuppetWidget::PuppetWidget()
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

  nsCOMPtr<nsIWidget> widget = nsIWidget::CreatePuppetWidget();
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

NS_IMETHODIMP
PuppetWidget::DispatchEvent(nsGUIEvent* event, nsEventStatus& aStatus)
{
#ifdef DEBUG
  debug_DumpEvent(stdout, event->widget, event,
                  nsCAutoString("PuppetWidget"), nsnull);
#endif

  aStatus = nsEventStatus_eIgnore;
  if (mEventCallback) {
    aStatus = (*mEventCallback)(event);
  }

  if (mChild) {
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
