






#ifndef SkOTTable_EBSC_DEFINED
#define SkOTTable_EBSC_DEFINED

#include "SkEndian.h"
#include "SkOTTable_EBLC.h"
#include "SkOTTableTypes.h"

#pragma pack(push, 1)

struct SkOTTableEmbeddedBitmapScaling {
    static const SK_OT_CHAR TAG0 = 'E';
    static const SK_OT_CHAR TAG1 = 'S';
    static const SK_OT_CHAR TAG2 = 'B';
    static const SK_OT_CHAR TAG3 = 'C';
    static const SK_OT_ULONG TAG = SkOTTableTAG<SkOTTableEmbeddedBitmapScaling>::value;

    SK_OT_Fixed version;
    static const SK_OT_Fixed version_initial = SkTEndian_SwapBE32(0x00020000);

    SK_OT_ULONG numSizes;

    struct BitmapScaleTable {
        SkOTTableEmbeddedBitmapLocation::SbitLineMetrics hori;
        SkOTTableEmbeddedBitmapLocation::SbitLineMetrics vert;
        SK_OT_BYTE ppemX; 
        SK_OT_BYTE ppemY; 
        SK_OT_BYTE substitutePpemX; 
        SK_OT_BYTE substitutePpemY; 
    }; 
};

#pragma pack(pop)

#endif
