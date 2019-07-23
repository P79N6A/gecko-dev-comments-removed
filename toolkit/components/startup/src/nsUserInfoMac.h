




































#ifndef __nsUserInfoMac_h
#define __nsUserInfoMac_h

#include "nsIUserInfo.h"
#include "nsReadableUtils.h"

class nsUserInfo: public nsIUserInfo
{
public:
  nsUserInfo();
  virtual ~nsUserInfo() {}

  NS_DECL_ISUPPORTS
  NS_DECL_NSIUSERINFO
  
  nsresult GetPrimaryEmailAddress(nsCString &aEmailAddress);  
};

#endif 
