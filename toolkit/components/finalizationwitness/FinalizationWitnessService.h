



#ifndef mozilla_finalizationwitnessservice_h__
#define mozilla_finalizationwitnessservice_h__

#include "nsIFinalizationWitnessService.h"
#include "nsIObserver.h"

namespace mozilla {




class FinalizationWitnessService final : public nsIFinalizationWitnessService,
                                         public nsIObserver
{
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFINALIZATIONWITNESSSERVICE
  NS_DECL_NSIOBSERVER

  nsresult Init();
 private:
  ~FinalizationWitnessService() {}
  void operator=(const FinalizationWitnessService* other) = delete;
};

} 

#endif 
