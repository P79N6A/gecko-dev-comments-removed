



#ifndef mozilla_plugins_PluginWidgetParent_h
#define mozilla_plugins_PluginWidgetParent_h

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

  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  mozilla::widget::PluginWidgetProxy* mWidget;
};

} 
} 

#endif 

