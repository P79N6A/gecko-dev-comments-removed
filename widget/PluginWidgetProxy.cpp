



#include "PluginWidgetProxy.h"
#include "mozilla/dom/TabChild.h"
#include "mozilla/plugins/PluginWidgetChild.h"
#include "nsDebug.h"

#define PWLOG(...)



already_AddRefed<nsIWidget>
nsIWidget::CreatePluginProxyWidget(TabChild* aTabChild,
                                   mozilla::plugins::PluginWidgetChild* aActor)
{
  nsCOMPtr<nsIWidget> widget =
    new mozilla::widget::PluginWidgetProxy(aTabChild, aActor);
  return widget.forget();
}

namespace mozilla {
namespace widget {

NS_IMPL_ISUPPORTS_INHERITED(PluginWidgetProxy, PuppetWidget, nsIWidget)

#define ENSURE_CHANNEL do {                                   \
  if (!mActor) {                                              \
    NS_WARNING("called on an invalid channel.");              \
    return NS_ERROR_FAILURE;                                  \
  }                                                           \
} while (0)

PluginWidgetProxy::PluginWidgetProxy(dom::TabChild* aTabChild,
                                     mozilla::plugins::PluginWidgetChild* aActor) :
  PuppetWidget(aTabChild),
  mActor(aActor)
{
  
  mActor->SetWidget(this);
}

PluginWidgetProxy::~PluginWidgetProxy()
{
  PWLOG("PluginWidgetProxy::~PluginWidgetProxy()\n");
}

NS_IMETHODIMP
PluginWidgetProxy::Create(nsIWidget*        aParent,
                          nsNativeWidget    aNativeParent,
                          const nsIntRect&  aRect,
                          nsWidgetInitData* aInitData)
{
  ENSURE_CHANNEL;
  PWLOG("PluginWidgetProxy::Create()\n");

  nsresult rv = NS_ERROR_UNEXPECTED;
  mActor->SendCreate(&rv);
  if (NS_FAILED(rv)) {
    NS_WARNING("failed to create chrome widget, plugins won't paint.");
    return rv;
  }

  BaseCreate(aParent, aRect, aInitData);

  mBounds = aRect;
  mEnabled = true;
  mVisible = true;

  return NS_OK;
}

NS_IMETHODIMP
PluginWidgetProxy::SetParent(nsIWidget* aNewParent)
{
  mParent = aNewParent;

  nsCOMPtr<nsIWidget> kungFuDeathGrip(this);
  nsIWidget* parent = GetParent();
  if (parent) {
    parent->RemoveChild(this);
  }
  if (aNewParent) {
    aNewParent->AddChild(this);
  }
  return NS_OK;
}

nsIWidget*
PluginWidgetProxy::GetParent(void)
{
  return mParent.get();
}

NS_IMETHODIMP
PluginWidgetProxy::Destroy()
{
  PWLOG("PluginWidgetProxy::Destroy()\n");

  if (mActor) {
    
    
    mActor->ProxyShutdown();
    mActor = nullptr;
  }

  return PuppetWidget::Destroy();
}

void
PluginWidgetProxy::GetWindowClipRegion(nsTArray<nsIntRect>* aRects)
{
  if (mClipRects && mClipRectCount) {
    aRects->AppendElements(mClipRects.get(), mClipRectCount);
  }
}

void*
PluginWidgetProxy::GetNativeData(uint32_t aDataType)
{
  if (!mActor) {
    return nullptr;
  }
  auto tab = static_cast<mozilla::dom::TabChild*>(mActor->Manager());
  if (tab && tab->IsDestroyed()) {
    return nullptr;
  }
  switch (aDataType) {
    case NS_NATIVE_PLUGIN_PORT:
    case NS_NATIVE_WINDOW:
    case NS_NATIVE_SHAREABLE_WINDOW:
      break;
    default:
      NS_WARNING("PluginWidgetProxy::GetNativeData received request for unsupported data type.");
      return nullptr;
  }
  uintptr_t value = 0;
  mActor->SendGetNativePluginPort(&value);
  PWLOG("PluginWidgetProxy::GetNativeData %p\n", (void*)value);
  return (void*)value;
}

NS_IMETHODIMP
PluginWidgetProxy::SetFocus(bool aRaise)
{
  ENSURE_CHANNEL;
  PWLOG("PluginWidgetProxy::SetFocus(%d)\n", aRaise);
  mActor->SendSetFocus(aRaise);
  return NS_OK;
}

}  
}  
