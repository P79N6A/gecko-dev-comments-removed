






#ifndef SkOTTable_OS_2_DEFINED
#define SkOTTable_OS_2_DEFINED

#include "SkOTTable_OS_2_VA.h"
#include "SkOTTable_OS_2_V0.h"
#include "SkOTTable_OS_2_V1.h"
#include "SkOTTable_OS_2_V2.h"
#include "SkOTTable_OS_2_V3.h"
#include "SkOTTable_OS_2_V4.h"

#pragma pack(push, 1)

struct SkOTTableOS2 {
    static const SK_OT_CHAR TAG0 = 'O';
    static const SK_OT_CHAR TAG1 = 'S';
    static const SK_OT_CHAR TAG2 = '/';
    static const SK_OT_CHAR TAG3 = '2';
    static const SK_OT_ULONG TAG = SkOTTableTAG<SkOTTableOS2>::value;

    union Version {
        SK_OT_USHORT version;

        
        struct VA : SkOTTableOS2_VA { } vA;
        struct V0 : SkOTTableOS2_V0 { } v0;
        struct V1 : SkOTTableOS2_V1 { } v1;
        struct V2 : SkOTTableOS2_V2 { } v2;
        
        struct V3 : SkOTTableOS2_V3 { } v3;
        
        struct V4 : SkOTTableOS2_V4 { } v4;
    } version;
};

#pragma pack(pop)


SK_COMPILE_ASSERT(sizeof(SkOTTableOS2::Version::VA) == 68, sizeof_SkOTTableOS2__VA_not_68);
SK_COMPILE_ASSERT(sizeof(SkOTTableOS2::Version::V0) == 78, sizeof_SkOTTableOS2__V0_not_78);
SK_COMPILE_ASSERT(sizeof(SkOTTableOS2::Version::V1) == 86, sizeof_SkOTTableOS2__V1_not_86);
SK_COMPILE_ASSERT(sizeof(SkOTTableOS2::Version::V2) == 96, sizeof_SkOTTableOS2__V2_not_96);
SK_COMPILE_ASSERT(sizeof(SkOTTableOS2::Version::V3) == 96, sizeof_SkOTTableOS2__V3_not_96);
SK_COMPILE_ASSERT(sizeof(SkOTTableOS2::Version::V4) == 96, sizeof_SkOTTableOS2__V4_not_96);

#endif
