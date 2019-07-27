





#ifndef mozilla_dom_TabContext_h
#define mozilla_dom_TabContext_h

#include "mozilla/layout/RenderFrameUtils.h"
#include "mozIApplication.h"
#include "nsCOMPtr.h"

namespace mozilla {
namespace dom {

class IPCTabContext;













class TabContext
{
protected:
  typedef mozilla::layout::ScrollingBehavior ScrollingBehavior;

public:
  TabContext();

  

  



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
  friend class MaybeInvalidTabContext;

  








  


  bool SetTabContext(const TabContext& aContext);

  



  bool SetTabContextForAppFrame(mozIApplication* aOwnApp,
                                mozIApplication* aAppFrameOwnerApp,
                                ScrollingBehavior aRequestedBehavior);

  



  bool SetTabContextForBrowserFrame(mozIApplication* aBrowserFrameOwnerApp,
                                    ScrollingBehavior aRequestedBehavior);

  


  bool SetTabContextForNormalFrame(ScrollingBehavior aRequestedBehavior);

private:
  


  bool mInitialized;

  



  nsCOMPtr<mozIApplication> mOwnApp;

  



  uint32_t mOwnAppId;

  




  nsCOMPtr<mozIApplication> mContainingApp;

  


  uint32_t mContainingAppId;

  


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

  bool SetTabContextForNormalFrame(ScrollingBehavior aRequestedBehavior)
  {
    return TabContext::SetTabContextForNormalFrame(aRequestedBehavior);
  }
};
























class MaybeInvalidTabContext
{
public:
  



  explicit MaybeInvalidTabContext(const IPCTabContext& aContext);

  


  bool IsValid();

  




  const char* GetInvalidReason();

  




  const TabContext& GetTabContext();

private:
  MaybeInvalidTabContext(const MaybeInvalidTabContext&) MOZ_DELETE;
  MaybeInvalidTabContext& operator=(const MaybeInvalidTabContext&) MOZ_DELETE;

  const char* mInvalidReason;
  MutableTabContext mTabContext;
};

} 
} 

#endif
