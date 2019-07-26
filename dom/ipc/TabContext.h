





#ifndef mozilla_dom_TabContext_h
#define mozilla_dom_TabContext_h

#include "mozilla/Assertions.h"
#include "mozilla/dom/PContent.h"
#include "mozilla/dom/PBrowser.h"
#include "mozilla/layout/RenderFrameUtils.h"
#include "nsIScriptSecurityManager.h"
#include "mozIApplication.h"

namespace mozilla {
namespace dom {













class TabContext
{
protected:
  typedef mozilla::layout::ScrollingBehavior ScrollingBehavior;

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

  


  ScrollingBehavior GetScrollingBehavior() const { return mScrollingBehavior; }

protected:
  








  


  bool SetTabContext(const TabContext& aContext);

  



  bool SetTabContextForAppFrame(mozIApplication* aOwnApp,
                                mozIApplication* aAppFrameOwnerApp,
                                ScrollingBehavior aRequestedBehavior);

  



  bool SetTabContextForBrowserFrame(mozIApplication* aBrowserFrameOwnerApp,
                                    ScrollingBehavior aRequestedBehavior);

private:
  


  already_AddRefed<mozIApplication> GetAppForId(uint32_t aAppId) const;

  


  bool mInitialized;

  



  uint32_t mOwnAppId;
  








  nsCOMPtr<mozIApplication> mOwnApp;

  





  uint32_t mContainingAppId;
  
  nsCOMPtr<mozIApplication> mContainingApp;

  


  ScrollingBehavior mScrollingBehavior;

  




  bool mIsBrowser;
};






class MutableTabContext : public TabContext
{
public:
  bool SetTabContext(const TabContext& aContext)
  {
    return TabContext::SetTabContext(aContext);
  }

  bool SetTabContextForAppFrame(mozIApplication* aOwnApp, mozIApplication* aAppFrameOwnerApp,
                                ScrollingBehavior aRequestedBehavior)
  {
    return TabContext::SetTabContextForAppFrame(aOwnApp, aAppFrameOwnerApp,
                                                aRequestedBehavior);
  }

  bool SetTabContextForBrowserFrame(mozIApplication* aBrowserFrameOwnerApp,
                                    ScrollingBehavior aRequestedBehavior)
  {
    return TabContext::SetTabContextForBrowserFrame(aBrowserFrameOwnerApp,
                                                    aRequestedBehavior);
  }
};

} 
} 

#endif
