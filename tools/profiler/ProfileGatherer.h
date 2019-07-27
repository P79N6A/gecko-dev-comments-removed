



#ifndef MOZ_PROFILE_GATHERER_H
#define MOZ_PROFILE_GATHERER_H

#include "mozilla/dom/Promise.h"

class TableTicker;

namespace mozilla {

class ProfileGatherer final : public nsISupports
{
public:
  NS_DECL_ISUPPORTS

  ProfileGatherer(TableTicker* aTicker,
                  double aSinceTime,
                  mozilla::dom::Promise* aPromise);
  void WillGatherOOPProfile();
  void GatheredOOPProfile();
  void Start();

private:
  ~ProfileGatherer() {};
  void Finish();

  nsRefPtr<mozilla::dom::Promise> mPromise;
  TableTicker* mTicker;
  double mSinceTime;
  uint32_t mPendingProfiles;
};

} 

#endif
