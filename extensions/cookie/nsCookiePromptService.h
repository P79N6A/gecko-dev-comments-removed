



































#ifndef nsCookiePromptService_h__
#define nsCookiePromptService_h__

#include "nsICookiePromptService.h"

class nsCookiePromptService : public nsICookiePromptService {

public:

  nsCookiePromptService();
  virtual ~nsCookiePromptService();

  NS_DECL_NSICOOKIEPROMPTSERVICE
  NS_DECL_ISUPPORTS

private:

};


#define NS_COOKIEPROMPTSERVICE_CID \
 {0xCE002B28, 0x92B7, 0x4701, {0x86, 0x21, 0xCC, 0x92, 0x58, 0x66, 0xFB, 0x87}}

#endif
