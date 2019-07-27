





#ifndef mozilla_dom_TabContext_h
#define mozilla_dom_TabContext_h

#include "mozIApplication.h"
#include "nsCOMPtr.h"

namespace mozilla {
namespace dom {

class IPCTabContext;













class TabContext
{
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

protected:
  friend class MaybeInvalidTabContext;

  








  


  bool SetTabContext(const TabContext& aContext);

  



  bool SetTabContextForAppFrame(mozIApplication* aOwnApp,
                                mozIApplication* aAppFrameOwnerApp);

  



  bool SetTabContextForBrowserFrame(mozIApplication* aBrowserFrameOwnerApp);

  


  bool SetTabContextForNormalFrame();

private:
  


  bool mInitialized;

  



  nsCOMPtr<mozIApplication> mOwnApp;

  



  uint32_t mOwnAppId;

  




  nsCOMPtr<mozIApplication> mContainingApp;

  


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

  bool SetTabContextForAppFrame(mozIApplication* aOwnApp,
                                mozIApplication* aAppFrameOwnerApp)
  {
    return TabContext::SetTabContextForAppFrame(aOwnApp, aAppFrameOwnerApp);
  }

  bool SetTabContextForBrowserFrame(mozIApplication* aBrowserFrameOwnerApp)
  {
    return TabContext::SetTabContextForBrowserFrame(aBrowserFrameOwnerApp);
  }

  bool SetTabContextForNormalFrame()
  {
    return TabContext::SetTabContextForNormalFrame();
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
  MaybeInvalidTabContext(const MaybeInvalidTabContext&) = delete;
  MaybeInvalidTabContext& operator=(const MaybeInvalidTabContext&) = delete;

  const char* mInvalidReason;
  MutableTabContext mTabContext;
};

} 
} 

#endif
