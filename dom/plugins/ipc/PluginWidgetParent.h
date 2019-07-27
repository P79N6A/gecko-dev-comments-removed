



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
  





  static void SendAsyncUpdate(nsIWidget* aWidget);

  PluginWidgetParent();
  virtual ~PluginWidgetParent();

  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;
  virtual bool RecvCreate(nsresult* aResult) MOZ_OVERRIDE;
  virtual bool RecvDestroy() MOZ_OVERRIDE;
  virtual bool RecvSetFocus(const bool& aRaise) MOZ_OVERRIDE;
  virtual bool RecvGetNativePluginPort(uintptr_t* value) MOZ_OVERRIDE;

  
  bool ActorDestroyed() { return !mWidget; }

  
  void ParentDestroy();

  
  void SetParent(nsIWidget* aParent);

private:
  
  mozilla::dom::TabParent* GetTabParent();

public:
  
  enum ShutdownType {
    TAB_CLOSURE = 1,
    CONTENT     = 2
  };

private:
  void Shutdown(ShutdownType aType);

  
  nsCOMPtr<nsIWidget> mWidget;
#if defined(MOZ_WIDGET_GTK)
  nsAutoPtr<nsPluginNativeWindowGtk> mWrapper;
#endif
};

} 
} 

#endif

