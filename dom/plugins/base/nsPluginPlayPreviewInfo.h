




#ifndef nsPluginPlayPreviewInfo_h_
#define nsPluginPlayPreviewInfo_h_

#include "nsString.h"
#include "nsIPluginHost.h"

class nsPluginPlayPreviewInfo : public nsIPluginPlayPreviewInfo
{
  virtual ~nsPluginPlayPreviewInfo();

public:
   NS_DECL_ISUPPORTS
   NS_DECL_NSIPLUGINPLAYPREVIEWINFO

  nsPluginPlayPreviewInfo(const char* aMimeType,
                          bool aIgnoreCTP,
                          const char* aRedirectURL,
                          const char* aWhitelist);
  explicit nsPluginPlayPreviewInfo(const nsPluginPlayPreviewInfo* aSource);

  nsCString mMimeType;
  bool      mIgnoreCTP;
  nsCString mRedirectURL;
  nsCString mWhitelist;
};


#endif 
