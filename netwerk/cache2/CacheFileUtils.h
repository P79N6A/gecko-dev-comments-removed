



#ifndef CacheFileUtils__h__
#define CacheFileUtils__h__

#include "nsError.h"

class nsILoadContextInfo;
class nsACString;

namespace mozilla {
namespace net {
namespace CacheFileUtils {

nsresult ParseKey(const nsACString &aKey,
                  nsILoadContextInfo **aInfo,
                  nsACString *aURL);

void CreateKeyPrefix(nsILoadContextInfo* aInfo, nsACString &_retval);

} 
} 
} 

#endif
