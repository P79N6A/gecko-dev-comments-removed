





#ifndef mozilla_dom_TabContext_h
#define mozilla_dom_TabContext_h

#include "mozilla/Assertions.h"
#include "mozilla/dom/PContent.h"
#include "mozilla/dom/PBrowser.h"
#include "nsIScriptSecurityManager.h"
#include "mozIApplication.h"

namespace mozilla {
namespace dom {













class TabContext
{
public:
  




  TabContext();

  









  TabContext(const IPCTabContext& aContext);

  



  IPCTabContext AsIPCTabContext() const;

  









  bool IsBrowserElement() const;

  



  bool IsBrowserOrApp() const;

  






  uint32_t OwnAppId() const;
  already_AddRefed<mozIApplication> GetOwnApp() const;
  bool HasOwnApp() const;

  







  uint32_t BrowserOwnerAppId() const;
  already_AddRefed<mozIApplication> GetBrowserOwnerApp() const;
  bool HasBrowserOwnerApp() const;

  







  uint32_t AppOwnerAppId() const;
  already_AddRefed<mozIApplication> GetAppOwnerApp() const;
  bool HasAppOwnerApp() const;

  




  uint32_t OwnOrContainingAppId() const;
  already_AddRefed<mozIApplication> GetOwnOrContainingApp() const;
  bool HasOwnOrContainingApp() const;

protected:
  








  


  bool SetTabContext(const TabContext& aContext);

  



  bool SetTabContextForAppFrame(mozIApplication* aOwnApp,
                                mozIApplication* aAppFrameOwnerApp);

  



  bool SetTabContextForBrowserFrame(mozIApplication* aBrowserFrameOwnerApp);

private:
  


  already_AddRefed<mozIApplication> GetAppForId(uint32_t aAppId) const;

  


  bool mInitialized;

  



  uint32_t mOwnAppId;

  





  uint32_t mContainingAppId;

  




  bool mIsBrowser;
};






class MutableTabContext : public TabContext
{
public:
  bool SetTabContext(const TabContext& aContext)
  {
    return TabContext::SetTabContext(aContext);
  }

  bool SetTabContextForAppFrame(mozIApplication* aOwnApp, mozIApplication* aAppFrameOwnerApp)
  {
    return TabContext::SetTabContextForAppFrame(aOwnApp, aAppFrameOwnerApp);
  }

  bool SetTabContextForBrowserFrame(mozIApplication* aBrowserFrameOwnerApp)
  {
    return TabContext::SetTabContextForBrowserFrame(aBrowserFrameOwnerApp);
  }
};

} 
} 

#endif
