





#include "mozilla/unused.h"
#include "mozilla/dom/ContentChild.h"
#include "nsScreenManagerProxy.h"
#include "nsServiceManagerUtils.h"
#include "nsIAppShell.h"
#include "nsIScreen.h"
#include "nsIScreenManager.h"
#include "nsWidgetsCID.h"

static NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::widget;

NS_IMPL_ISUPPORTS(nsScreenManagerProxy, nsIScreenManager)

nsScreenManagerProxy::nsScreenManagerProxy()
  : mNumberOfScreens(-1)
  , mSystemDefaultScale(1.0)
  , mCacheValid(true)
  , mCacheWillInvalidate(false)
{
  bool success = false;
  unused << ContentChild::GetSingleton()->CallPScreenManagerConstructor(
                                            this,
                                            &mNumberOfScreens,
                                            &mSystemDefaultScale,
                                            &success);

  if (!success) {
    
    
    NS_WARNING("Setting up communications with the parent nsIScreenManager failed.");
  }

  InvalidateCacheOnNextTick();

  
  
  
  
  
  
  
  AddRef();
}





NS_IMETHODIMP
nsScreenManagerProxy::GetPrimaryScreen(nsIScreen** outScreen)
{
  InvalidateCacheOnNextTick();

  if (!mPrimaryScreen) {
    ScreenDetails details;
    bool success = false;
    unused << CallGetPrimaryScreen(&details, &success);
    if (!success) {
      return NS_ERROR_FAILURE;
    }

    mPrimaryScreen = new ScreenProxy(this, details);
  }
  NS_ADDREF(*outScreen = mPrimaryScreen);
  return NS_OK;
}

NS_IMETHODIMP
nsScreenManagerProxy::ScreenForId(uint32_t aId, nsIScreen** outScreen)
{
  
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsScreenManagerProxy::ScreenForRect(int32_t inLeft,
                                    int32_t inTop,
                                    int32_t inWidth,
                                    int32_t inHeight,
                                    nsIScreen** outScreen)
{
  bool success = false;
  ScreenDetails details;
  unused << CallScreenForRect(inLeft, inTop, inWidth, inHeight, &details, &success);
  if (!success) {
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<ScreenProxy> screen = new ScreenProxy(this, details);
  NS_ADDREF(*outScreen = screen);

  return NS_OK;
}

NS_IMETHODIMP
nsScreenManagerProxy::ScreenForNativeWidget(void* aWidget,
                                            nsIScreen** outScreen)
{
  
  
  
  TabChild* tabChild = static_cast<TabChild*>(aWidget);

  
  
  for (uint32_t i = 0; i < mScreenCache.Length(); ++i) {
      ScreenCacheEntry& curr = mScreenCache[i];
      if (curr.mTabChild == aWidget) {
          NS_ADDREF(*outScreen = static_cast<nsIScreen*>(curr.mScreenProxy));
          return NS_OK;
      }
  }

  
  
  bool success = false;
  ScreenDetails details;
  unused << CallScreenForBrowser(tabChild, &details, &success);
  if (!success) {
    return NS_ERROR_FAILURE;
  }

  ScreenCacheEntry newEntry;
  nsRefPtr<ScreenProxy> screen = new ScreenProxy(this, details);

  newEntry.mScreenProxy = screen;
  newEntry.mTabChild = tabChild;

  mScreenCache.AppendElement(newEntry);

  NS_ADDREF(*outScreen = screen);

  InvalidateCacheOnNextTick();
  return NS_OK;
}

NS_IMETHODIMP
nsScreenManagerProxy::GetNumberOfScreens(uint32_t* aNumberOfScreens)
{
  if (!EnsureCacheIsValid()) {
    return NS_ERROR_FAILURE;
  }

  *aNumberOfScreens = mNumberOfScreens;
  return NS_OK;
}

NS_IMETHODIMP
nsScreenManagerProxy::GetSystemDefaultScale(float *aSystemDefaultScale)
{
  if (!EnsureCacheIsValid()) {
    return NS_ERROR_FAILURE;
  }

  *aSystemDefaultScale = mSystemDefaultScale;
  return NS_OK;
}

bool
nsScreenManagerProxy::EnsureCacheIsValid()
{
  if (mCacheValid) {
    return true;
  }

  bool success = false;
  
  
  unused << CallRefresh(&mNumberOfScreens, &mSystemDefaultScale, &success);
  if (!success) {
    NS_WARNING("Refreshing nsScreenManagerProxy failed in the parent process.");
    return false;
  }

  mCacheValid = true;

  InvalidateCacheOnNextTick();
  return true;
}

void
nsScreenManagerProxy::InvalidateCacheOnNextTick()
{
  if (mCacheWillInvalidate) {
    return;
  }

  mCacheWillInvalidate = true;

  nsCOMPtr<nsIAppShell> appShell = do_GetService(kAppShellCID);
  if (appShell) {
    appShell->RunInStableState(
      NS_NewRunnableMethod(this, &nsScreenManagerProxy::InvalidateCache)
    );
  } else {
    
    
    InvalidateCache();
  }
}

void
nsScreenManagerProxy::InvalidateCache()
{
  mCacheValid = false;
  mCacheWillInvalidate = false;

  if (mPrimaryScreen) {
    mPrimaryScreen = nullptr;
  }
  for (int32_t i = mScreenCache.Length() - 1; i >= 0; --i) {
    mScreenCache.RemoveElementAt(i);
  }
}

