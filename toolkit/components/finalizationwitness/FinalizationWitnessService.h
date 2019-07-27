



#ifndef mozilla_finalizationwitnessservice_h__
#define mozilla_finalizationwitnessservice_h__

#include "nsIFinalizationWitnessService.h"

namespace mozilla {




class FinalizationWitnessService final : public nsIFinalizationWitnessService
{
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFINALIZATIONWITNESSSERVICE
 private:
  ~FinalizationWitnessService() {}
  void operator=(const FinalizationWitnessService* other) = delete;
};

} 

#endif 
