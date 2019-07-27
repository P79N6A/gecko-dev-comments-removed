



#ifndef mozilla_plugins_PluginWidgetParent_h
#define mozilla_plugins_PluginWidgetParent_h

#include "mozilla/plugins/PPluginWidgetParent.h"
#include "nsIWidget.h"
#include "nsCOMPtr.h"

#if defined(MOZ_WIDGET_GTK)
class nsPluginNativeWindowGtk;
#endif

namespace mozilla {

namespace dom {
class TabParent;
}

namespace plugins {

class PluginWidgetParent : public PPluginWidgetParent
{
public:
  PluginWidgetParent();
  virtual ~PluginWidgetParent();

  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;
  virtual bool RecvCreate() MOZ_OVERRIDE;
  virtual bool RecvDestroy() MOZ_OVERRIDE;
  virtual bool RecvSetFocus(const bool& aRaise) MOZ_OVERRIDE;
  virtual bool RecvGetNativePluginPort(uintptr_t* value) MOZ_OVERRIDE;

private:
  
  mozilla::dom::TabParent* GetTabParent();
  
  nsCOMPtr<nsIWidget> mWidget;
#if defined(MOZ_WIDGET_GTK)
  UniquePtr<nsPluginNativeWindowGtk> mWrapper;
#endif
};

} 
} 

#endif 

