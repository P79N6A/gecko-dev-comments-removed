




#ifndef nsPermission_h__
#define nsPermission_h__

#include "nsIPermission.h"
#include "nsString.h"



class nsPermission : public nsIPermission
{
public:
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPERMISSION

  nsPermission(nsIPrincipal* aPrincipal,
               const nsACString &aType,
               uint32_t aCapability,
               uint32_t aExpireType,
               int64_t aExpireTime);

protected:
  virtual ~nsPermission() {};

  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsCString mType;
  uint32_t  mCapability;
  uint32_t  mExpireType;
  int64_t   mExpireTime;
};

#endif 
