



#ifndef nsIdleServiceX_h_
#define nsIdleServiceX_h_

#include "nsIdleService.h"

class nsIdleServiceX : public nsIdleService
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  bool PollIdleTime(uint32_t* aIdleTime) override;

  static already_AddRefed<nsIdleServiceX> GetInstance() 
  {
    nsRefPtr<nsIdleService> idleService = nsIdleService::GetInstance();
    if (!idleService) {
      idleService = new nsIdleServiceX();
    }
    
    return idleService.forget().downcast<nsIdleServiceX>();
  }
  
protected:
    nsIdleServiceX() { }
    virtual ~nsIdleServiceX() { }
    bool UsePollMode() override;
};

#endif 
