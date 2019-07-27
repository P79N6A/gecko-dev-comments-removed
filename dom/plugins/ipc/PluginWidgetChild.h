



#ifndef mozilla_plugins_PluginWidgetChild_h
#define mozilla_plugins_PluginWidgetChild_h

#include "mozilla/plugins/PPluginWidgetChild.h"

namespace mozilla {
namespace widget {
class PluginWidgetProxy;
}
namespace plugins {

class PluginWidgetChild : public PPluginWidgetChild
{
public:
  PluginWidgetChild();
  virtual ~PluginWidgetChild();

  virtual bool RecvUpdateWindow(const uintptr_t& aChildId) MOZ_OVERRIDE;
  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;
  virtual bool RecvParentShutdown() MOZ_OVERRIDE;

  mozilla::widget::PluginWidgetProxy* mWidget;
};

} 
} 

#endif 

