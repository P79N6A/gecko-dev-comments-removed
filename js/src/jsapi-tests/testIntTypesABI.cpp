#include "tests.h"







#include "js-config.h"
#include "jsapi.h"
#include "jsclass.h"
#include "jscompat.h"
#include "jscpucfg.h"
#include "jspubtd.h"
#include "jstypes.h"
#include "jsval.h"
#include "jsxdrapi.h"

#include "js/HashTable.h"
#include "js/MemoryMetrics.h"
#include "js/TemplateLib.h"
#include "js/Utility.h"
#include "js/Vector.h"












struct ConflictingType {
    uint64_t u64;
};

typedef ConflictingType uint8;
typedef ConflictingType uint16;
typedef ConflictingType uint32;
typedef ConflictingType uint64;

typedef ConflictingType int8;
typedef ConflictingType int16;
typedef ConflictingType int32;
typedef ConflictingType int64;

typedef ConflictingType JSUint8;
typedef ConflictingType JSUint16;
typedef ConflictingType JSUint32;
typedef ConflictingType JSUint64;

typedef ConflictingType JSInt8;
typedef ConflictingType JSInt16;
typedef ConflictingType JSInt32;
typedef ConflictingType JSInt64;

typedef ConflictingType jsword;
typedef ConflictingType jsuword;
typedef ConflictingType JSWord;
typedef ConflictingType JSUword;

BEGIN_TEST(testIntTypesABI)
{
    
    return true;
}
END_TEST(testIntTypesABI)
