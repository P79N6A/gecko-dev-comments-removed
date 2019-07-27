




#ifndef nsScreenManagerProxy_h
#define nsScreenManagerProxy_h

#include "nsIScreenManager.h"
#include "mozilla/dom/PScreenManagerChild.h"
#include "mozilla/dom/TabChild.h"
#include "ScreenProxy.h"













class nsScreenManagerProxy MOZ_FINAL : public nsIScreenManager,
                                       public mozilla::dom::PScreenManagerChild
{
public:
  nsScreenManagerProxy();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISCREENMANAGER

private:
  ~nsScreenManagerProxy() {};

  bool EnsureCacheIsValid();
  void InvalidateCacheOnNextTick();
  void InvalidateCache();

  uint32_t mNumberOfScreens;
  float mSystemDefaultScale;
  bool mCacheValid;
  bool mCacheWillInvalidate;

  nsRefPtr<mozilla::widget::ScreenProxy> mPrimaryScreen;

  
  
  
  
  
  
  
  
  
  struct ScreenCacheEntry
  {
    nsRefPtr<mozilla::widget::ScreenProxy> mScreenProxy;
    nsRefPtr<mozilla::dom::TabChild> mTabChild;
  };

  nsTArray<ScreenCacheEntry> mScreenCache;
};

#endif 
