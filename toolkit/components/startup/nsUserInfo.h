




































#ifndef __nsUserInfo_h
#define __nsUserInfo_h

#include "nsIUserInfo.h"

class nsUserInfo: public nsIUserInfo

{
public:
  nsUserInfo(void);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIUSERINFO

  virtual ~nsUserInfo();
};

#endif 
