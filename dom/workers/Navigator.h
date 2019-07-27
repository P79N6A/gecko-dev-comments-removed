




#ifndef mozilla_dom_workers_navigator_h__
#define mozilla_dom_workers_navigator_h__

#include "Workers.h"
#include "RuntimeService.h"
#include "nsString.h"
#include "nsWrapperCache.h"



#include "mozilla/dom/Navigator.h"

namespace mozilla {
namespace dom {
class Promise;
}
}

BEGIN_WORKERS_NAMESPACE

class WorkerNavigator MOZ_FINAL : public nsWrapperCache
{
  typedef struct RuntimeService::NavigatorProperties NavigatorProperties;

  NavigatorProperties mProperties;
  bool mOnline;

  WorkerNavigator(const NavigatorProperties& aProperties,
                  bool aOnline)
    : mProperties(aProperties)
    , mOnline(aOnline)
  {
    MOZ_COUNT_CTOR(WorkerNavigator);
  }

  ~WorkerNavigator()
  {
    MOZ_COUNT_DTOR(WorkerNavigator);
  }

public:

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(WorkerNavigator)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(WorkerNavigator)

  static already_AddRefed<WorkerNavigator>
  Create(bool aOnLine);

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) MOZ_OVERRIDE;

  nsISupports* GetParentObject() const {
    return nullptr;
  }

  void GetAppCodeName(nsString& aAppCodeName) const
  {
    aAppCodeName.AssignLiteral("Mozilla");
  }
  void GetAppName(nsString& aAppName) const;

  void GetAppVersion(nsString& aAppVersion) const;

  void GetPlatform(nsString& aPlatform) const;

  void GetProduct(nsString& aProduct) const
  {
    aProduct.AssignLiteral("Gecko");
  }

  bool TaintEnabled() const
  {
    return false;
  }

  void GetLanguage(nsString& aLanguage) const
  {
    if (mProperties.mLanguages.Length() >= 1) {
      aLanguage.Assign(mProperties.mLanguages[0]);
    } else {
      aLanguage.Truncate();
    }
  }

  void GetLanguages(nsTArray<nsString>& aLanguages) const
  {
    aLanguages = mProperties.mLanguages;
  }

  void GetUserAgent(nsString& aUserAgent) const;

  bool OnLine() const
  {
    return mOnline;
  }

  
  void SetOnLine(bool aOnline)
  {
    mOnline = aOnline;
  }

  void SetLanguages(const nsTArray<nsString>& aLanguages);

  already_AddRefed<Promise> GetDataStores(JSContext* aCx,
                                          const nsAString& aName,
                                          const nsAString& aOwner,
                                          ErrorResult& aRv);
};

END_WORKERS_NAMESPACE

#endif 
