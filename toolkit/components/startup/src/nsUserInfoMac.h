




































#ifndef __nsUserInfoMac_h
#define __nsUserInfoMac_h

#include "nsIUserInfo.h"


#include <InternetConfig.h>

class nsUserInfo: public nsIUserInfo

{
public:
              nsUserInfo(void);
  virtual     ~nsUserInfo();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIUSERINFO

  
protected:

  nsresult    EnsureInitted();

  static OSType      GetAppCreatorCode();
  static PRUnichar*  PStringToNewUCS2(ConstStr255Param str);
  
protected:

  ICInstance    mInstance;
  PRBool        mInitted;
  
};

#endif 
