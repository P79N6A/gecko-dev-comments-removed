



































#ifndef nsIdleServiceX_h_
#define nsIdleServiceX_h_

#include "nsIdleService.h"

class nsIdleServiceX : public nsIdleService
{
public:
  NS_DECL_ISUPPORTS

  nsIdleServiceX() {}
  virtual ~nsIdleServiceX() {}

  bool PollIdleTime(PRUint32* aIdleTime);

protected:
    bool UsePollMode();
};

#endif 
