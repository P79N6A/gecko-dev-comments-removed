




#ifndef mozilla_dom_workers_navigator_h__
#define mozilla_dom_workers_navigator_h__

#include "Workers.h"
#include "nsString.h"
#include "nsWrapperCache.h"

BEGIN_WORKERS_NAMESPACE

class WorkerNavigator MOZ_FINAL : public nsWrapperCache
{
  nsString mAppName;
  nsString mAppVersion;
  nsString mPlatform;
  nsString mUserAgent;

  WorkerNavigator(const nsAString& aAppName,
                  const nsAString& aAppVersion,
                  const nsAString& aPlatform,
                  const nsAString& aUserAgent)
    : mAppName(aAppName)
    , mAppVersion(aAppVersion)
    , mPlatform(aPlatform)
    , mUserAgent(aUserAgent)
  {
    MOZ_COUNT_CTOR(WorkerNavigator);
    SetIsDOMBinding();
  }

public:

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(WorkerNavigator)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(WorkerNavigator)

  static already_AddRefed<WorkerNavigator>
  Create();

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  nsISupports* GetParentObject() const {
    return nullptr;
  }

  ~WorkerNavigator()
  {
    MOZ_COUNT_DTOR(WorkerNavigator);
  }

  void GetAppName(nsString& aAppName) const
  {
    aAppName = mAppName;
  }
  void GetAppVersion(nsString& aAppVersion) const
  {
    aAppVersion = mAppVersion;
  }
  void GetPlatform(nsString& aPlatform) const
  {
    aPlatform = mPlatform;
  }
  void GetUserAgent(nsString& aUserAgent) const
  {
    aUserAgent = mUserAgent;
  }
};

END_WORKERS_NAMESPACE

#endif 
