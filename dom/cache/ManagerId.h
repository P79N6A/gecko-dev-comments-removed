





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

class ManagerId final
{
public:
  
  static nsresult Create(nsIPrincipal* aPrincipal, ManagerId** aManagerIdOut);

  
  already_AddRefed<nsIPrincipal> Principal() const;

  const nsACString& ExtendedOrigin() const { return mExtendedOrigin; }

  bool operator==(const ManagerId& aOther) const
  {
    return mExtendedOrigin == aOther.mExtendedOrigin;
  }

private:
  ManagerId(nsIPrincipal* aPrincipal, const nsACString& aOrigin,
            const nsACString& aJarPrefix);
  ~ManagerId();

  ManagerId(const ManagerId&) = delete;
  ManagerId& operator=(const ManagerId&) = delete;

  
  nsCOMPtr<nsIPrincipal> mPrincipal;

  
  const nsCString mExtendedOrigin;

public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(mozilla::dom::cache::ManagerId)
};

} 
} 
} 

#endif
