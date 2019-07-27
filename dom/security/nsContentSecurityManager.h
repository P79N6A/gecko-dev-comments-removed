





#ifndef nsContentSecurityManager_h___
#define nsContentSecurityManager_h___

#include "nsIChannel.h"
class nsIStreamListener;

class nsContentSecurityManager
{
private:
  nsContentSecurityManager() {}
  virtual ~nsContentSecurityManager() {}

public:
  static nsresult doContentSecurityCheck(nsIChannel* aChannel,
                                         nsCOMPtr<nsIStreamListener>& aInAndOutListener);
};

#endif 
