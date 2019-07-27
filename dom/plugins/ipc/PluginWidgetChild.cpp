



#include "mozilla/plugins/PluginWidgetChild.h"
#include "PluginWidgetProxy.h"

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

} 
} 



