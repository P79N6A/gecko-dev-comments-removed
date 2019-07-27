



#include "PluginWidgetParent.h"
#include "mozilla/dom/TabParent.h"
#include "nsComponentManagerUtils.h"
#include "nsWidgetsCID.h"
#include "nsDebug.h"

using namespace mozilla::widget;

#define PWLOG(...)


namespace mozilla {
namespace plugins {

static NS_DEFINE_CID(kWidgetCID, NS_CHILD_CID);



#define ENSURE_CHANNEL {                                      \
  if (!mWidget) {                                             \
    NS_WARNING("called on an invalid remote widget.");        \
    return true;                                              \
  }                                                           \
}

PluginWidgetParent::PluginWidgetParent()
{
  PWLOG("PluginWidgetParent::PluginWidgetParent()\n");
  MOZ_COUNT_CTOR(PluginWidgetParent);
}

PluginWidgetParent::~PluginWidgetParent()
{
  PWLOG("PluginWidgetParent::~PluginWidgetParent()\n");
  MOZ_COUNT_DTOR(PluginWidgetParent);
  
  
  
  if (mWidget) {
    mWidget->Destroy();
    mWidget = nullptr;
  }
}

mozilla::dom::TabParent*
PluginWidgetParent::GetTabParent()
{
  return static_cast<mozilla::dom::TabParent*>(Manager());
}

void
PluginWidgetParent::ActorDestroy(ActorDestroyReason aWhy)
{
  PWLOG("PluginWidgetParent::ActorDestroy()\n");
}







bool
PluginWidgetParent::RecvCreate()
{
  PWLOG("PluginWidgetParent::RecvCreate()\n");

  nsresult rv;

  mWidget = do_CreateInstance(kWidgetCID, &rv);

  
  nsCOMPtr<nsIWidget> parentWidget = GetTabParent()->GetWidget();

  nsWidgetInitData initData;
  initData.mWindowType = eWindowType_plugin_ipc_chrome;
  initData.mUnicode = false;
  initData.clipChildren = true;
  initData.clipSiblings = true;
  rv = mWidget->Create(parentWidget.get(), nullptr, nsIntRect(0,0,0,0),
                       nullptr, &initData);
  if (NS_FAILED(rv)) {
    mWidget->Destroy();
    mWidget = nullptr;
    return false;
  }

  mWidget->EnableDragDrop(true);
  mWidget->Show(false);
  mWidget->Enable(false);

  
  
  
  RecvMove(0, 0);

  return true;
}

bool
PluginWidgetParent::RecvDestroy()
{
  ENSURE_CHANNEL;
  PWLOG("PluginWidgetParent::RecvDestroy()\n");
  mWidget->Destroy();
  mWidget = nullptr;
  return true;
}

bool
PluginWidgetParent::RecvShow(const bool& aState)
{
  ENSURE_CHANNEL;
  PWLOG("PluginWidgetParent::RecvShow(%d)\n", aState);
  mWidget->Show(aState);
  return true;
}

bool
PluginWidgetParent::RecvSetFocus(const bool& aRaise)
{
  ENSURE_CHANNEL;
  PWLOG("PluginWidgetParent::RecvSetFocus(%d)\n", aRaise);
  mWidget->SetFocus(aRaise);
  return true;
}

bool
PluginWidgetParent::RecvInvalidate(const nsIntRect& aRect)
{
  ENSURE_CHANNEL;
  PWLOG("PluginWidgetParent::RecvInvalidate(%d, %d, %d, %d)\n", aRect.x, aRect.y, aRect.width, aRect.height);
  mWidget->Invalidate(aRect);
  return true;
}

bool
PluginWidgetParent::RecvGetNativePluginPort(uintptr_t* value)
{
  ENSURE_CHANNEL;
  PWLOG("PluginWidgetParent::RecvGetNativeData()\n");
  *value = (uintptr_t)mWidget->GetNativeData(NS_NATIVE_PLUGIN_PORT);
  return true;
}

bool
PluginWidgetParent::RecvResize(const nsIntRect& aRect)
{
  ENSURE_CHANNEL;
  PWLOG("PluginWidgetParent::RecvResize(%d, %d, %d, %d)\n", aRect.x, aRect.y, aRect.width, aRect.height);
  mWidget->Resize(aRect.width, aRect.height, true);
  return true;
}

bool
PluginWidgetParent::RecvMove(const double& aX, const double& aY)
{
  ENSURE_CHANNEL;
  PWLOG("PluginWidgetParent::RecvMove(%f, %f)\n", aX, aY);


  
  nsCOMPtr<nsIWidget> widget = GetTabParent()->GetWidget();
  if (!widget) {
    
    
    return true;
  }

  
  nsIntPoint offset = GetTabParent()->GetChildProcessOffset();
  offset.x = abs(offset.x);
  offset.y = abs(offset.y);
  offset += nsIntPoint(ceil(aX), ceil(aY));
  mWidget->Move(offset.x, offset.y);

  return true;
}

bool
PluginWidgetParent::RecvSetWindowClipRegion(const nsTArray<nsIntRect>& Regions,
                                            const bool& aIntersectWithExisting)
{
  ENSURE_CHANNEL;
  PWLOG("PluginWidgetParent::RecvSetWindowClipRegion()\n");
  mWidget->SetWindowClipRegion(Regions, aIntersectWithExisting);
  return true;
}

} 
} 
