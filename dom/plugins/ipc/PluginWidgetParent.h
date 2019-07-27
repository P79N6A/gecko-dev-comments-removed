



#ifndef mozilla_plugins_PluginWidgetParent_h
#define mozilla_plugins_PluginWidgetParent_h

#include "mozilla/plugins/PPluginWidgetParent.h"
#include "nsIWidget.h"
#include "nsCOMPtr.h"

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
  virtual bool RecvShow(const bool& aState) MOZ_OVERRIDE;
  virtual bool RecvSetFocus(const bool& aRaise) MOZ_OVERRIDE;
  virtual bool RecvInvalidate(const nsIntRect& aRect) MOZ_OVERRIDE;
  virtual bool RecvGetNativePluginPort(uintptr_t* value) MOZ_OVERRIDE;
  virtual bool RecvResize(const nsIntRect& aRect) MOZ_OVERRIDE;
  virtual bool RecvMove(const double& aX, const double& aY) MOZ_OVERRIDE;
  virtual bool RecvSetWindowClipRegion(const nsTArray<nsIntRect>& Regions,
                                        const bool& aIntersectWithExisting) MOZ_OVERRIDE;

private:
  
  mozilla::dom::TabParent* GetTabParent();
  
  nsCOMPtr<nsIWidget> mWidget;
};

} 
} 

#endif 

