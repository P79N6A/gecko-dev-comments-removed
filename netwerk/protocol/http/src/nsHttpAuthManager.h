




































#ifndef nsHttpAuthManager_h__
#define nsHttpAuthManager_h__

#include "nsHttpAuthCache.h"
#include "nsIHttpAuthManager.h"

class nsHttpAuthManager : public nsIHttpAuthManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIHTTPAUTHMANAGER

  nsHttpAuthManager();
  virtual ~nsHttpAuthManager();
  nsresult Init();

protected:
  nsHttpAuthCache *mAuthCache;
};

#endif 
