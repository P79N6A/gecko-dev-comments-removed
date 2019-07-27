



#include "PluginWidgetParent.h"
#include "mozilla/dom/TabParent.h"
#include "mozilla/dom/ContentParent.h"
#include "nsComponentManagerUtils.h"
#include "nsWidgetsCID.h"

#include "mozilla/unused.h"
#include "mozilla/DebugOnly.h"
#include "nsDebug.h"

#if defined(MOZ_WIDGET_GTK)
#include "nsPluginNativeWindowGtk.h"
#endif

using namespace mozilla;
using namespace mozilla::widget;

#define PWLOG(...)


#if defined(XP_WIN)
namespace mozilla {
namespace dom {

const wchar_t* kPluginWidgetContentParentProperty =
  L"kPluginWidgetParentProperty";
} }
#endif

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
  
  
  
  KillWidget();
}

mozilla::dom::TabParent*
PluginWidgetParent::GetTabParent()
{
  return static_cast<mozilla::dom::TabParent*>(Manager());
}

void
PluginWidgetParent::SetParent(nsIWidget* aParent)
{
  if (mWidget && aParent) {
    mWidget->SetParent(aParent);
  }
}






bool
PluginWidgetParent::RecvCreate(nsresult* aResult)
{
  PWLOG("PluginWidgetParent::RecvCreate()\n");

  mWidget = do_CreateInstance(kWidgetCID, aResult);
  NS_ASSERTION(NS_SUCCEEDED(*aResult), "widget create failure");

#if defined(MOZ_WIDGET_GTK)
  
  
  PLUG_NewPluginNativeWindow((nsPluginNativeWindow**)&mWrapper);
  if (!mWrapper) {
    KillWidget();
    return false;
  }
  
  
  mWidget->SetNativeData(NS_NATIVE_PLUGIN_OBJECT_PTR, (uintptr_t)mWrapper.get());
#endif

  
  nsCOMPtr<nsIWidget> parentWidget = GetTabParent()->GetWidget();
  
  if (!parentWidget) {
    *aResult = NS_ERROR_NOT_AVAILABLE;
    KillWidget();
    return true;
  }

  nsWidgetInitData initData;
  initData.mWindowType = eWindowType_plugin_ipc_chrome;
  initData.mUnicode = false;
  initData.clipChildren = true;
  initData.clipSiblings = true;
  *aResult = mWidget->Create(parentWidget.get(), nullptr, nsIntRect(0,0,0,0),
                             &initData);
  if (NS_FAILED(*aResult)) {
    KillWidget();
    
    return false;
  }

  DebugOnly<nsresult> drv;
  drv = mWidget->EnableDragDrop(true);
  NS_ASSERTION(NS_SUCCEEDED(drv), "widget call failure");

#if defined(MOZ_WIDGET_GTK)
  
  mWrapper->window = mWidget->GetNativeData(NS_NATIVE_PLUGIN_PORT);
  drv = mWrapper->CreateXEmbedWindow(false);
  NS_ASSERTION(NS_SUCCEEDED(drv), "widget call failure");
  mWrapper->SetAllocation();
  PWLOG("Plugin XID=%p\n", (void*)mWrapper->window);
#elif defined(XP_WIN)
  DebugOnly<DWORD> winres =
    ::SetPropW((HWND)mWidget->GetNativeData(NS_NATIVE_WINDOW),
               mozilla::dom::kPluginWidgetContentParentProperty,
               GetTabParent()->Manager()->AsContentParent());
  NS_ASSERTION(winres, "SetPropW call failure");
#endif

  
  
  
  
  mWidget->RegisterPluginWindowForRemoteUpdates();

  return true;
}

void
PluginWidgetParent::KillWidget()
{
  PWLOG("PluginWidgetParent::KillWidget() widget=%p\n", (void*)mWidget.get());
  if (mWidget) {
    mWidget->UnregisterPluginWindowForRemoteUpdates();
    DebugOnly<nsresult> rv = mWidget->Destroy();
    NS_ASSERTION(NS_SUCCEEDED(rv), "widget destroy failure");
#if defined(MOZ_WIDGET_GTK)
    mWidget->SetNativeData(NS_NATIVE_PLUGIN_OBJECT_PTR, (uintptr_t)0);
    mWrapper = nullptr;
#elif defined(XP_WIN)
    ::RemovePropW((HWND)mWidget->GetNativeData(NS_NATIVE_WINDOW),
                  mozilla::dom::kPluginWidgetContentParentProperty);
#endif
    mWidget = nullptr;
  }
}

void
PluginWidgetParent::ActorDestroy(ActorDestroyReason aWhy)
{
  PWLOG("PluginWidgetParent::ActorDestroy(%d)\n", aWhy);
  KillWidget();
}




void
PluginWidgetParent::ParentDestroy()
{
  PWLOG("PluginWidgetParent::ParentDestroy()\n");
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
#if defined(MOZ_WIDGET_GTK)
  *value = (uintptr_t)mWrapper->window;
  NS_ASSERTION(*value, "no xid??");
#else
  *value = (uintptr_t)mWidget->GetNativeData(NS_NATIVE_PLUGIN_PORT);
  NS_ASSERTION(*value, "no native port??");
#endif
  PWLOG("PluginWidgetParent::RecvGetNativeData() %p\n", (void*)*value);
  return true;
}

} 
} 
