










#ifndef __COLLATIONCOMPARE_H__
#define __COLLATIONCOMPARE_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/ucol.h"

U_NAMESPACE_BEGIN

class CollationIterator;
struct CollationSettings;

class U_I18N_API CollationCompare  {
public:
    static UCollationResult compareUpToQuaternary(CollationIterator &left, CollationIterator &right,
                                                  const CollationSettings &settings,
                                                  UErrorCode &errorCode);
};

U_NAMESPACE_END

#endif  
#endif  
