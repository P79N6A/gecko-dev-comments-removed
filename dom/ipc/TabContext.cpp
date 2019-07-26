





#include "mozilla/dom/TabContext.h"
#include "mozilla/dom/TabParent.h"
#include "mozilla/dom/TabChild.h"
#include "nsIAppsService.h"

using namespace mozilla::dom::ipc;
using namespace mozilla::layout;

namespace mozilla {
namespace dom {

TabContext::TabContext()
  : mInitialized(false)
  , mOwnAppId(nsIScriptSecurityManager::NO_APP_ID)
  , mContainingAppId(nsIScriptSecurityManager::NO_APP_ID)
  , mScrollingBehavior(DEFAULT_SCROLLING)
  , mIsBrowser(false)
{
}

TabContext::TabContext(const IPCTabContext& aParams)
  : mInitialized(true)
{
  const IPCTabAppBrowserContext& appBrowser = aParams.appBrowserContext();
  switch(appBrowser.type()) {
    case IPCTabAppBrowserContext::TPopupIPCTabContext: {
      const PopupIPCTabContext &ipcContext = appBrowser.get_PopupIPCTabContext();

      TabContext *context;
      if (ipcContext.openerParent()) {
        context = static_cast<TabParent*>(ipcContext.openerParent());
        if (context->IsBrowserElement() && !ipcContext.isBrowserElement()) {
          
          
          
          
          MOZ_CRASH();
        }
      }
      else if (ipcContext.openerChild()) {
        context = static_cast<TabChild*>(ipcContext.openerChild());
      }
      else {
        
        
        MOZ_CRASH();
      }

      
      
      
      
      
      
      
      if (ipcContext.isBrowserElement()) {
        mIsBrowser = true;
        mOwnAppId = nsIScriptSecurityManager::NO_APP_ID;
        mContainingAppId = context->OwnOrContainingAppId();
      }
      else {
        mIsBrowser = false;
        mOwnAppId = context->mOwnAppId;
        mContainingAppId = context->mContainingAppId;
      }
      break;
    }
    case IPCTabAppBrowserContext::TAppFrameIPCTabContext: {
      const AppFrameIPCTabContext &ipcContext =
        appBrowser.get_AppFrameIPCTabContext();

      mIsBrowser = false;
      mOwnAppId = ipcContext.ownAppId();
      mContainingAppId = ipcContext.appFrameOwnerAppId();
      break;
    }
    case IPCTabAppBrowserContext::TBrowserFrameIPCTabContext: {
      const BrowserFrameIPCTabContext &ipcContext =
        appBrowser.get_BrowserFrameIPCTabContext();

      mIsBrowser = true;
      mOwnAppId = nsIScriptSecurityManager::NO_APP_ID;
      mContainingAppId = ipcContext.browserFrameOwnerAppId();
      break;
    }
    case IPCTabAppBrowserContext::TVanillaFrameIPCTabContext: {
      mIsBrowser = false;
      mOwnAppId = nsIScriptSecurityManager::NO_APP_ID;
      mContainingAppId = nsIScriptSecurityManager::NO_APP_ID;
      break;
    }
    default: {
      MOZ_CRASH();
    }
  }

  mScrollingBehavior = aParams.scrollingBehavior();
}

bool
TabContext::IsBrowserElement() const
{
  return mIsBrowser;
}

bool
TabContext::IsBrowserOrApp() const
{
  return HasOwnApp() || IsBrowserElement();
}

uint32_t
TabContext::OwnAppId() const
{
  return mOwnAppId;
}

already_AddRefed<mozIApplication>
TabContext::GetOwnApp() const
{
  if (mOwnApp) {
    return nsCOMPtr<mozIApplication>(mOwnApp).forget();
  }
  return GetAppForId(OwnAppId());
}

bool
TabContext::HasOwnApp() const
{
  return mOwnAppId != nsIScriptSecurityManager::NO_APP_ID;
}

uint32_t
TabContext::BrowserOwnerAppId() const
{
  if (mIsBrowser) {
    return mContainingAppId;
  }
  return nsIScriptSecurityManager::NO_APP_ID;
}

already_AddRefed<mozIApplication>
TabContext::GetBrowserOwnerApp() const
{
  return GetAppForId(BrowserOwnerAppId());
}

bool
TabContext::HasBrowserOwnerApp() const
{
  return BrowserOwnerAppId() != nsIScriptSecurityManager::NO_APP_ID;
}

uint32_t
TabContext::AppOwnerAppId() const
{
  if (mOwnAppId != nsIScriptSecurityManager::NO_APP_ID) {
    return mContainingAppId;
  }
  return nsIScriptSecurityManager::NO_APP_ID;
}

already_AddRefed<mozIApplication>
TabContext::GetAppOwnerApp() const
{
  if (mContainingApp) {
    return nsCOMPtr<mozIApplication>(mContainingApp).forget();
  }
  return GetAppForId(AppOwnerAppId());
}

bool
TabContext::HasAppOwnerApp() const
{
  return AppOwnerAppId() != nsIScriptSecurityManager::NO_APP_ID;
}

uint32_t
TabContext::OwnOrContainingAppId() const
{
  if (mIsBrowser) {
    MOZ_ASSERT(mOwnAppId == nsIScriptSecurityManager::NO_APP_ID);
    return mContainingAppId;
  }

  if (mOwnAppId) {
    return mOwnAppId;
  }

  return mContainingAppId;
}

already_AddRefed<mozIApplication>
TabContext::GetOwnOrContainingApp() const
{
  if (mOwnApp) {
    return nsCOMPtr<mozIApplication>(mOwnApp).forget();
  }
  if (mContainingApp) {
    return nsCOMPtr<mozIApplication>(mContainingApp).forget();
  }
  return GetAppForId(OwnOrContainingAppId());
}

bool
TabContext::HasOwnOrContainingApp() const
{
  return OwnOrContainingAppId() != nsIScriptSecurityManager::NO_APP_ID;
}

bool
TabContext::SetTabContext(const TabContext& aContext)
{
  NS_ENSURE_FALSE(mInitialized, false);

  
  
  if (aContext.mOwnAppId != nsIScriptSecurityManager::NO_APP_ID) {
    nsCOMPtr<mozIApplication> app = GetAppForId(aContext.mOwnAppId);
    NS_ENSURE_TRUE(app, false);
  }

  if (aContext.mContainingAppId != nsIScriptSecurityManager::NO_APP_ID) {
    nsCOMPtr<mozIApplication> app = GetAppForId(aContext.mContainingAppId);
    NS_ENSURE_TRUE(app, false);
  }

  mInitialized = true;
  mIsBrowser = aContext.mIsBrowser;
  mOwnAppId = aContext.mOwnAppId;
  mContainingAppId = aContext.mContainingAppId;
  mScrollingBehavior = aContext.mScrollingBehavior;
  return true;
}

bool
TabContext::SetTabContextForAppFrame(mozIApplication* aOwnApp, mozIApplication* aAppFrameOwnerApp,
                                     ScrollingBehavior aRequestedBehavior)
{
  NS_ENSURE_FALSE(mInitialized, false);

  
  
  uint32_t ownAppId = nsIScriptSecurityManager::NO_APP_ID;
  if (aOwnApp) {
    nsresult rv = aOwnApp->GetLocalId(&ownAppId);
    NS_ENSURE_SUCCESS(rv, false);
  }

  uint32_t containingAppId = nsIScriptSecurityManager::NO_APP_ID;
  if (aAppFrameOwnerApp) {
    nsresult rv = aOwnApp->GetLocalId(&containingAppId);
    NS_ENSURE_SUCCESS(rv, false);
  }

  mInitialized = true;
  mIsBrowser = false;
  mOwnAppId = ownAppId;
  mOwnApp = aOwnApp;
  mContainingAppId = containingAppId;
  mContainingApp = aAppFrameOwnerApp;
  mScrollingBehavior = aRequestedBehavior;
  return true;
}

bool
TabContext::SetTabContextForBrowserFrame(mozIApplication* aBrowserFrameOwnerApp,
                                         ScrollingBehavior aRequestedBehavior)
{
  NS_ENSURE_FALSE(mInitialized, false);

  uint32_t containingAppId = nsIScriptSecurityManager::NO_APP_ID;
  if (aBrowserFrameOwnerApp) {
    nsresult rv = aBrowserFrameOwnerApp->GetLocalId(&containingAppId);
    NS_ENSURE_SUCCESS(rv, false);
  }

  mInitialized = true;
  mIsBrowser = true;
  mOwnAppId = nsIScriptSecurityManager::NO_APP_ID;
  mContainingAppId = containingAppId;
  mScrollingBehavior = aRequestedBehavior;
  return true;
}

IPCTabContext
TabContext::AsIPCTabContext() const
{
  if (mIsBrowser) {
    return IPCTabContext(BrowserFrameIPCTabContext(mContainingAppId),
                         mScrollingBehavior);
  }

  return IPCTabContext(AppFrameIPCTabContext(mOwnAppId, mContainingAppId),
                       mScrollingBehavior);
}

already_AddRefed<mozIApplication>
TabContext::GetAppForId(uint32_t aAppId) const
{
  if (aAppId == nsIScriptSecurityManager::NO_APP_ID) {
    return nullptr;
  }

  nsCOMPtr<nsIAppsService> appsService = do_GetService(APPS_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(appsService, nullptr);

  nsCOMPtr<mozIDOMApplication> domApp;
  appsService->GetAppByLocalId(aAppId, getter_AddRefs(domApp));

  nsCOMPtr<mozIApplication> app = do_QueryInterface(domApp);
  return app.forget();
}

} 
} 
