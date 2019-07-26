


#ifndef _MAC_WEB_APP_UTILS_H_
#define _MAC_WEB_APP_UTILS_H_

#include "nsIMacWebAppUtils.h"

#define NS_MACWEBAPPUTILS_CONTRACTID "@mozilla.org/widget/mac-web-app-utils;1"

class nsMacWebAppUtils : public nsIMacWebAppUtils {
public:
  nsMacWebAppUtils() {}
  virtual ~nsMacWebAppUtils() {}
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMACWEBAPPUTILS
};

#endif
