





#ifndef mozilla_dom_TVListeners_h
#define mozilla_dom_TVListeners_h

#include "nsClassHashtable.h"
#include "nsITVService.h"
#include "nsRefPtrHashtable.h"

namespace mozilla {
namespace dom {

class TVSource;

class TVSourceListener : public nsITVSourceListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITVSOURCELISTENER

  void RegisterSource(TVSource* aSource);

  void UnregisterSource(TVSource* aSource);

private:
  ~TVSourceListener() {}

  already_AddRefed<TVSource> GetSource(const nsAString& aTunerId,
                                       const nsAString& aSourceType);

  
  
  nsClassHashtable<nsStringHashKey, nsRefPtrHashtable<nsStringHashKey, TVSource>> mSources;
};

} 
} 

#endif 
