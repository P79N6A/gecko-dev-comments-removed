





#ifndef mozilla_dom_TVListeners_h
#define mozilla_dom_TVListeners_h

#include "mozilla/dom/TVSource.h"
#include "nsCycleCollectionParticipant.h"
#include "nsITVService.h"
#include "nsTArray.h"

namespace mozilla {
namespace dom {

class TVSourceListener : public nsITVSourceListener
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(TVSourceListener)
  NS_DECL_NSITVSOURCELISTENER

  void RegisterSource(TVSource* aSource);

  void UnregisterSource(TVSource* aSource);

private:
  ~TVSourceListener() {}

  already_AddRefed<TVSource> GetSource(const nsAString& aTunerId,
                                       const nsAString& aSourceType);

  nsTArray<nsRefPtr<TVSource>> mSources;
};

} 
} 

#endif 
