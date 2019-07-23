




































#include "nsICipherInfo.h"
#include "nsString.h"
#include "sslt.h"

class nsCipherInfoService : public nsICipherInfoService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICIPHERINFOSERVICE

  nsCipherInfoService();
  virtual ~nsCipherInfoService();
};

class nsCipherInfo : public nsICipherInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICIPHERINFO

  nsCipherInfo(PRUint16 aCipherId);
  virtual ~nsCipherInfo();

private:
  PRBool mHaveInfo;
  SSLCipherSuiteInfo mInfo;
};
