




































#ifndef nsPermission_h__
#define nsPermission_h__

#include "nsIPermission.h"
#include "nsString.h"



class nsPermission : public nsIPermission
{
public:
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPERMISSION

  nsPermission(const nsACString &aHost,
               const nsACString &aType, 
               PRUint32 aCapability,
               PRUint32 aExpireType,
               PRInt64 aExpireTime);

  virtual ~nsPermission();
  
protected:
  nsCString mHost;
  nsCString mType;
  PRUint32  mCapability;
  PRUint32  mExpireType;
  PRInt64   mExpireTime;
};

#endif 
