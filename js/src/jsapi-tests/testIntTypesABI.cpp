










#include "jscpucfg.h"
#include "jspubtd.h"
#include "jstypes.h"

#include "js/Anchor.h"
#include "js/CallArgs.h"
#include "js/CallNonGenericMethod.h"
#include "js/CharacterEncoding.h"
#include "js/Class.h"
#include "js/Date.h"
#include "js/Debug.h"
#include "js/GCAPI.h"
#include "js/HashTable.h"
#include "js/HeapAPI.h"
#include "js/Id.h"

#include "js/MemoryMetrics.h"
#include "js/OldDebugAPI.h"
#include "js/ProfilingStack.h"
#include "js/PropertyKey.h"
#include "js/RequiredDefines.h"
#include "js/RootingAPI.h"
#include "js/SliceBudget.h"
#include "js/StructuredClone.h"
#include "js/TracingAPI.h"
#include "js/TypeDecls.h"
#include "js/UbiNode.h"
#include "js/Utility.h"
#include "js/Value.h"
#include "js/Vector.h"
#include "js/WeakMapPtr.h"
#include "jsapi-tests/tests.h"












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
