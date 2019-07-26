



#ifndef mozilla_finalizationwitnessservice_h__
#define mozilla_finalizationwitnessservice_h__

#include "nsIFinalizationWitnessService.h"

namespace mozilla {




class FinalizationWitnessService MOZ_FINAL : public nsIFinalizationWitnessService
{
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFINALIZATIONWITNESSSERVICE
 private:
  void operator=(const FinalizationWitnessService* other) MOZ_DELETE;
};

} 

#endif 
