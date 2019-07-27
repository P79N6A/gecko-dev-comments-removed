



#include "mozilla/plugins/PluginWidgetChild.h"
#include "PluginWidgetProxy.h"
#include "mozilla/DebugOnly.h"
#include "nsDebug.h"

#if defined(XP_WIN)
#include "mozilla/plugins/PluginInstanceParent.h"
using mozilla::plugins::PluginInstanceParent;
#endif

namespace mozilla {
namespace plugins {

PluginWidgetChild::PluginWidgetChild() :
  mWidget(nullptr)
{
  MOZ_COUNT_CTOR(PluginWidgetChild);
}

PluginWidgetChild::~PluginWidgetChild()
{
  MOZ_COUNT_DTOR(PluginWidgetChild);
}

void
PluginWidgetChild::ActorDestroy(ActorDestroyReason aWhy)
{
  if (mWidget) {
    mWidget->ChannelDestroyed();
  }
  mWidget = nullptr;
}

bool
PluginWidgetChild::RecvUpdateWindow(const uintptr_t& aChildId)
{
#if defined(XP_WIN)
  NS_ASSERTION(aChildId, "Expected child hwnd value for remote plugin instance.");
  PluginInstanceParent* parentInstance =
    PluginInstanceParent::LookupPluginInstanceByID(aChildId);
  NS_ASSERTION(parentInstance, "Expected matching plugin instance");
  if (parentInstance) {
    
    
    parentInstance->CallUpdateWindow();
  }
  return true;
#else
  NS_NOTREACHED("PluginWidgetChild::RecvUpdateWindow calls unexpected on this platform.");
  return false;
#endif
}

} 
} 



