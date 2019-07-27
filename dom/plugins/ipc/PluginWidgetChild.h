



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

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

  void SetWidget(mozilla::widget::PluginWidgetProxy* aWidget) {
    mWidget = aWidget;
  }
  void ProxyShutdown();

private:
  void KillWidget();

  mozilla::widget::PluginWidgetProxy* mWidget;
};

} 
} 

#endif 

