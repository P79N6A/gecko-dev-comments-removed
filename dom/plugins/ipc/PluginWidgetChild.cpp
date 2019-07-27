



#include "mozilla/plugins/PluginWidgetChild.h"

#include "mozilla/dom/TabChild.h"
#include "mozilla/plugins/PluginWidgetParent.h"
#include "PluginWidgetProxy.h"

#include "mozilla/unused.h"
#include "mozilla/DebugOnly.h"
#include "nsDebug.h"

#define PWLOG(...)


namespace mozilla {
namespace plugins {

PluginWidgetChild::PluginWidgetChild() :
  mWidget(nullptr)
{
  PWLOG("PluginWidgetChild::PluginWidgetChild()\n");
  MOZ_COUNT_CTOR(PluginWidgetChild);
}

PluginWidgetChild::~PluginWidgetChild()
{
  PWLOG("PluginWidgetChild::~PluginWidgetChild()\n");
  MOZ_COUNT_DTOR(PluginWidgetChild);
}



void
PluginWidgetChild::ProxyShutdown()
{
  PWLOG("PluginWidgetChild::ProxyShutdown()\n");
  if (mWidget) {
    mWidget = nullptr;
    auto tab = static_cast<mozilla::dom::TabChild*>(Manager());
    if (!tab->IsDestroyed()) {
      unused << Send__delete__(this);
    }
  }
}

void
PluginWidgetChild::KillWidget()
{
  PWLOG("PluginWidgetChild::KillWidget()\n");
  if (mWidget) {
    mWidget->ChannelDestroyed();
  }
  mWidget = nullptr;
}

void
PluginWidgetChild::ActorDestroy(ActorDestroyReason aWhy)
{
  PWLOG("PluginWidgetChild::ActorDestroy(%d)\n", aWhy);
  KillWidget();
}

} 
} 
