 





#ifndef PeerIdentity_h
#define PeerIdentity_h

#ifdef MOZILLA_INTERNAL_API
#include "nsString.h"
#else
#include "nsStringAPI.h"
#endif

template <class T> class nsCOMPtr;
class nsIIDNService;

namespace mozilla {










class PeerIdentity MOZ_FINAL
{
public:
  explicit PeerIdentity(const nsAString& aPeerIdentity)
    : mPeerIdentity(aPeerIdentity) {}
  ~PeerIdentity() {}

  bool Equals(const PeerIdentity& aOther) const;
  bool Equals(const nsAString& aOtherString) const;
  const nsString& ToString() const { return mPeerIdentity; }

private:
  static void GetUser(const nsAString& aPeerIdentity, nsAString& aUser);
  static void GetHost(const nsAString& aPeerIdentity, nsAString& aHost);

  static void GetNormalizedHost(const nsCOMPtr<nsIIDNService>& aIdnService,
                                const nsAString& aHost,
                                nsACString& aNormalizedHost);

  nsString mPeerIdentity;
};

inline bool
operator==(const PeerIdentity& aOne, const PeerIdentity& aTwo)
{
  return aOne.Equals(aTwo);
}

inline bool
operator==(const PeerIdentity& aOne, const nsAString& aString)
{
  return aOne.Equals(aString);
}

inline bool
operator!=(const PeerIdentity& aOne, const PeerIdentity& aTwo)
{
  return !aOne.Equals(aTwo);
}

inline bool
operator!=(const PeerIdentity& aOne, const nsAString& aString)
{
  return !aOne.Equals(aString);
}


} 

#endif 
