





#ifndef nsCategoryManagerUtils_h__
#define nsCategoryManagerUtils_h__

#include "nsICategoryManager.h"

void
NS_CreateServicesFromCategory(const char* aCategory,
                              nsISupports* aOrigin,
                              const char* aObserverTopic);

#endif
