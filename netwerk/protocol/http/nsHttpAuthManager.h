




#ifndef nsHttpAuthManager_h__
#define nsHttpAuthManager_h__

#include "nsIHttpAuthManager.h"

namespace mozilla {
namespace net {

class nsHttpAuthCache;

class nsHttpAuthManager : public nsIHttpAuthManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIHTTPAUTHMANAGER

  nsHttpAuthManager();
  nsresult Init();

protected:
  virtual ~nsHttpAuthManager();

  nsHttpAuthCache *mAuthCache;
  nsHttpAuthCache *mPrivateAuthCache;
};

} 
} 

#endif 
