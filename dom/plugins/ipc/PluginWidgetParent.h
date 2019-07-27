



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

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;
  virtual bool RecvCreate(nsresult* aResult) override;
  virtual bool RecvSetFocus(const bool& aRaise) override;
  virtual bool RecvGetNativePluginPort(uintptr_t* value) override;

  
  bool ActorDestroyed() { return !mWidget; }

  
  void ParentDestroy();

  
  void SetParent(nsIWidget* aParent);

private:
  
  mozilla::dom::TabParent* GetTabParent();

private:
  void KillWidget();

  
  nsCOMPtr<nsIWidget> mWidget;
#if defined(MOZ_WIDGET_GTK)
  nsAutoPtr<nsPluginNativeWindowGtk> mWrapper;
#endif
};

} 
} 

#endif 

