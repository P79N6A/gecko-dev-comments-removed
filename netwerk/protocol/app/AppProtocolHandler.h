





#ifndef AppProtocolHandler_
#define AppProtocolHandler_

#include "nsIProtocolHandler.h"
#include "nsClassHashtable.h"
#include "mozilla/dom/AppInfoBinding.h"

class AppProtocolHandler : public nsIProtocolHandler
{
public:
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIPROTOCOLHANDLER

  
  AppProtocolHandler();
  virtual ~AppProtocolHandler();

  
  static nsresult Create(nsISupports* aOuter,
                         const nsIID& aIID,
                         void* *aResult);

private:
  nsClassHashtable<nsCStringHashKey, mozilla::dom::AppInfo> mAppInfoCache;
};

#endif 
