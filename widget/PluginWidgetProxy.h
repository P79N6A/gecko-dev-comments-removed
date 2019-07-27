



#ifndef mozilla_widget_RemotePlugin_h__
#define mozilla_widget_RemotePlugin_h__

#include "PuppetWidget.h"
#include "mozilla/dom/TabChild.h"








namespace mozilla {
namespace plugins {
class PluginWidgetChild;
}
namespace widget {

class PluginWidgetProxy final : public PuppetWidget
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
                    nsWidgetInitData* aInitData = nullptr) override;
  NS_IMETHOD Destroy() override;
  NS_IMETHOD SetFocus(bool aRaise = false) override;
  NS_IMETHOD SetParent(nsIWidget* aNewParent) override;

  virtual nsIWidget* GetParent(void) override;
  virtual void* GetNativeData(uint32_t aDataType) override;
#if defined(XP_WIN)
  void SetNativeData(uint32_t aDataType, uintptr_t aVal) override;
#endif
  virtual nsTransparencyMode GetTransparencyMode() override
  { return eTransparencyOpaque; }
  virtual void GetWindowClipRegion(nsTArray<nsIntRect>* aRects) override;

public:
  






  void ChannelDestroyed() { mActor = nullptr; }

private:
  
  mozilla::plugins::PluginWidgetChild* mActor;
  
  
  nsCOMPtr<nsIWidget> mParent;
};

}  
}  
#endif
