




#ifndef nsCategoryManagerUtils_h__
#define nsCategoryManagerUtils_h__

#include "nsICategoryManager.h"

void
NS_CreateServicesFromCategory(const char *category,
                              nsISupports *origin,
                              const char *observerTopic);

#endif
