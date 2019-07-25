



#ifndef nsIdleServiceX_h_
#define nsIdleServiceX_h_

#include "nsIdleService.h"

class nsIdleServiceX : public nsIdleService
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  bool PollIdleTime(uint32_t* aIdleTime);

  static already_AddRefed<nsIdleServiceX> GetInstance() 
  {
    nsIdleServiceX* idleService = 
      static_cast<nsIdleServiceX*>(nsIdleService::GetInstance().get());
    if (!idleService) {
      idleService = new nsIdleServiceX();
      NS_ADDREF(idleService);
    }
    
    return idleService;
  }
  
protected:
    nsIdleServiceX() { }
    virtual ~nsIdleServiceX() { }
    bool UsePollMode();
};

#endif 
