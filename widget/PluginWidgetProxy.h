



#ifndef mozilla_widget_RemotePlugin_h__
#define mozilla_widget_RemotePlugin_h__

#include "PuppetWidget.h"
#include "mozilla/dom/TabChild.h"








namespace mozilla {
namespace plugins {
class PluginWidgetChild;
}
namespace widget {

class PluginWidgetProxy MOZ_FINAL : public PuppetWidget
{
public:
  explicit PluginWidgetProxy(dom::TabChild* aTabChild,
                             mozilla::plugins::PluginWidgetChild* aChannel);

protected:
  virtual ~PluginWidgetProxy();

public:
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD Create(nsIWidget* aParent, nsNativeWidget aNativeParent,
                    const nsIntRect& aRect,
                    nsWidgetInitData* aInitData = nullptr) MOZ_OVERRIDE;
  NS_IMETHOD Destroy() MOZ_OVERRIDE;
  NS_IMETHOD SetFocus(bool aRaise = false) MOZ_OVERRIDE;
  NS_IMETHOD SetParent(nsIWidget* aNewParent) MOZ_OVERRIDE;

  virtual nsIWidget* GetParent(void) MOZ_OVERRIDE;
  virtual void* GetNativeData(uint32_t aDataType) MOZ_OVERRIDE;
  virtual nsTransparencyMode GetTransparencyMode() MOZ_OVERRIDE
  { return eTransparencyOpaque; }
  virtual void GetWindowClipRegion(nsTArray<nsIntRect>* aRects) MOZ_OVERRIDE;

public:
  






  void ChannelDestroyed() { mActor = nullptr; }

private:
  
  mozilla::plugins::PluginWidgetChild* mActor;
  
  
  nsCOMPtr<nsIWidget> mParent;
};

}  
}  
#endif
