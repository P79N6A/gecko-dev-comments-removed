



#include "PluginWidgetParent.h"
#include "mozilla/dom/TabParent.h"
#include "nsComponentManagerUtils.h"
#include "nsWidgetsCID.h"
#include "nsDebug.h"

#if defined(MOZ_WIDGET_GTK)
#include "nsPluginNativeWindowGtk.h"
#else
#include "nsPluginNativeWindow.h"
#endif

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
#if defined(MOZ_WIDGET_GTK)
  : mWrapper(nullptr)
#endif
{
  PWLOG("PluginWidgetParent::PluginWidgetParent()\n");
  MOZ_COUNT_CTOR(PluginWidgetParent);
}

PluginWidgetParent::~PluginWidgetParent()
{
  PWLOG("PluginWidgetParent::~PluginWidgetParent()\n");
  MOZ_COUNT_DTOR(PluginWidgetParent);
  
  
  
  if (mWidget) {
    mWidget->UnregisterPluginWindowForRemoteUpdates();
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

#if defined(MOZ_WIDGET_GTK)
  
  
  PLUG_NewPluginNativeWindow((nsPluginNativeWindow**)&mWrapper);
  if (!mWrapper) {
    return false;
  }
#endif

  
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

  
  
  
  
  mWidget->RegisterPluginWindowForRemoteUpdates();

#if defined(MOZ_WIDGET_GTK)
  
  mWrapper->window = mWidget->GetNativeData(NS_NATIVE_PLUGIN_PORT);
  mWrapper->CreateXEmbedWindow(false);
  mWrapper->SetAllocation();
  PWLOG("Plugin XID=%p\n", (void*)mWrapper->window);
#endif

  return true;
}

bool
PluginWidgetParent::RecvDestroy()
{
  ENSURE_CHANNEL;
  PWLOG("PluginWidgetParent::RecvDestroy()\n");
  mWidget->UnregisterPluginWindowForRemoteUpdates();
  mWidget->Destroy();
  mWidget = nullptr;
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
PluginWidgetParent::RecvGetNativePluginPort(uintptr_t* value)
{
  ENSURE_CHANNEL;
  PWLOG("PluginWidgetParent::RecvGetNativeData()\n");
#if defined(MOZ_WIDGET_GTK)
  *value = (uintptr_t)mWrapper->window;
#else
  *value = (uintptr_t)mWidget->GetNativeData(NS_NATIVE_PLUGIN_PORT);
#endif
  PWLOG("PluginWidgetParent::RecvGetNativeData() %p\n", (void*)*value);
  return true;
}

} 
} 
