










#ifndef __COLLATIONROOT_H__
#define __COLLATIONROOT_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

U_NAMESPACE_BEGIN

struct CollationCacheEntry;
struct CollationData;
struct CollationSettings;
struct CollationTailoring;




class U_I18N_API CollationRoot {  
public:
    static const CollationCacheEntry *getRootCacheEntry(UErrorCode &errorCode);
    static const CollationTailoring *getRoot(UErrorCode &errorCode);
    static const CollationData *getData(UErrorCode &errorCode);
    static const CollationSettings *getSettings(UErrorCode &errorCode);

private:
    static void load(UErrorCode &errorCode);
};

U_NAMESPACE_END

#endif  
#endif  
