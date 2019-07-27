






#ifndef SkOTTable_EBDT_DEFINED
#define SkOTTable_EBDT_DEFINED

#include "SkEndian.h"
#include "SkOTTableTypes.h"
#include "SkOTTable_head.h"
#include "SkOTTable_loca.h"
#include "SkTypedEnum.h"

#pragma pack(push, 1)

struct SkOTTableEmbeddedBitmapData {
    static const SK_OT_CHAR TAG0 = 'E';
    static const SK_OT_CHAR TAG1 = 'B';
    static const SK_OT_CHAR TAG2 = 'D';
    static const SK_OT_CHAR TAG3 = 'T';
    static const SK_OT_ULONG TAG = SkOTTableTAG<SkOTTableEmbeddedBitmapData>::value;

    SK_OT_Fixed version;
    static const SK_OT_Fixed version_initial = SkTEndian_SwapBE32(0x00020000);

    struct BigGlyphMetrics {
        SK_OT_BYTE height;
        SK_OT_BYTE width;
        SK_OT_CHAR horiBearingX;
        SK_OT_CHAR horiBearingY;
        SK_OT_BYTE horiAdvance;
        SK_OT_CHAR vertBearingX;
        SK_OT_CHAR vertBearingY;
        SK_OT_BYTE vertAdvance;
    };

    struct SmallGlyphMetrics {
        SK_OT_BYTE height;
        SK_OT_BYTE width;
        SK_OT_CHAR bearingX;
        SK_OT_CHAR bearingY;
        SK_OT_BYTE advance;
    };

    
    struct Format1 {
        SmallGlyphMetrics smallGlyphMetrics;
        
    };

    
    struct Format2 {
        SmallGlyphMetrics smallGlyphMetrics;
        
    };

    

    
    
    struct Format4 {
        SK_OT_ULONG whiteTreeOffset;
        SK_OT_ULONG blackTreeOffset;
        SK_OT_ULONG glyphDataOffset;
    };

    
    struct Format5 {
        
    };

    
    struct Format6 {
        BigGlyphMetrics bigGlyphMetrics;
        
    };

    
    struct Format7 {
        BigGlyphMetrics bigGlyphMetrics;
        
    };

    struct EBDTComponent {
        SK_OT_USHORT glyphCode; 
        SK_OT_CHAR xOffset; 
        SK_OT_CHAR yOffset; 
    };

    struct Format8 {
        SmallGlyphMetrics smallMetrics; 
        SK_OT_BYTE pad; 
        SK_OT_USHORT numComponents; 
        
    };

    struct Format9 {
        BigGlyphMetrics bigMetrics; 
        SK_OT_USHORT numComponents; 
        
    };
};

#pragma pack(pop)

#endif
