



































#ifndef nsAlertsService_h_
#define nsAlertsService_h_

#include "nsIAlertsService.h"
#include "nsIObserver.h"
#include "nsToolkitCompsCID.h"

struct GrowlDelegateWrapper;

class nsAlertsService : public nsIAlertsService,
                        public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIALERTSSERVICE
  NS_DECL_NSIOBSERVER

  nsAlertsService();
  nsresult Init();
private:
  GrowlDelegateWrapper* mDelegate;
  virtual ~nsAlertsService();
};

class nsModuleComponentInfo;
NS_METHOD nsAlertsServiceRegister(nsIComponentManager* aCompMgr,
                                  nsIFile *aPath,
                                  const char* registryLocation,
                                  const char* componentType,
                                  const nsModuleComponentInfo* info);
NS_METHOD nsAlertsServiceUnregister(nsIComponentManager* aCompMgr,
                                    nsIFile* aPath,
                                    const char* registryLocation,
                                    const nsModuleComponentInfo* info);

#endif 
