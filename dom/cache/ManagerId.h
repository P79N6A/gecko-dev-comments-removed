





#ifndef mozilla_dom_cache_ManagerId_h
#define mozilla_dom_cache_ManagerId_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/cache/Types.h"
#include "nsCOMPtr.h"
#include "nsError.h"
#include "nsISupportsImpl.h"
#include "nsString.h"

class nsIPrincipal;

namespace mozilla {
namespace dom {
namespace cache {

class ManagerId MOZ_FINAL
{
public:
  
  class MOZ_STACK_CLASS Comparator MOZ_FINAL
  {
  public:
    bool Equals(ManagerId *aA, ManagerId* aB) const { return *aA == *aB; }
    bool LessThan(ManagerId *aA, ManagerId* aB) const { return *aA < *aB; }
  };

  
  static nsresult Create(nsIPrincipal* aPrincipal, ManagerId** aManagerIdOut);

  
  already_AddRefed<nsIPrincipal> Principal() const;

  const nsACString& Origin() const { return mOrigin; }

  bool operator==(const ManagerId& aOther) const
  {
    return mOrigin == aOther.mOrigin &&
           mAppId == aOther.mAppId &&
           mInBrowserElement == aOther.mInBrowserElement;
  }

  bool operator<(const ManagerId& aOther) const
  {
    return mOrigin < aOther.mOrigin ||
           (mOrigin == aOther.mOrigin && mAppId < aOther.mAppId) ||
           (mOrigin == aOther.mOrigin && mAppId == aOther.mAppId &&
            mInBrowserElement < aOther.mInBrowserElement);
  }

private:
  ManagerId(nsIPrincipal* aPrincipal, const nsACString& aOrigin,
            uint32_t aAppId, bool aInBrowserElement);
  ~ManagerId();

  ManagerId(const ManagerId&) = delete;
  ManagerId& operator=(const ManagerId&) = delete;

  
  nsCOMPtr<nsIPrincipal> mPrincipal;

  
  const nsCString mOrigin;
  const uint32_t mAppId;
  const bool mInBrowserElement;

public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(mozilla::dom::cache::ManagerId)
};

} 
} 
} 

#endif
