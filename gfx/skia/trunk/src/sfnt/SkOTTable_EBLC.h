






#ifndef SkOTTable_EBLC_DEFINED
#define SkOTTable_EBLC_DEFINED

#include "SkEndian.h"
#include "SkOTTable_EBDT.h"
#include "SkOTTableTypes.h"
#include "SkTypedEnum.h"

#pragma pack(push, 1)

struct SkOTTableEmbeddedBitmapLocation {
    static const SK_OT_CHAR TAG0 = 'E';
    static const SK_OT_CHAR TAG1 = 'B';
    static const SK_OT_CHAR TAG2 = 'L';
    static const SK_OT_CHAR TAG3 = 'C';
    static const SK_OT_ULONG TAG = SkOTTableTAG<SkOTTableEmbeddedBitmapLocation>::value;

    SK_OT_Fixed version;
    static const SK_OT_Fixed version_initial = SkTEndian_SwapBE32(0x00020000);

    SK_OT_ULONG numSizes;

    struct SbitLineMetrics {
        SK_OT_CHAR ascender;
        SK_OT_CHAR descender;
        SK_OT_BYTE widthMax;
        SK_OT_CHAR caretSlopeNumerator;
        SK_OT_CHAR caretSlopeDenominator;
        SK_OT_CHAR caretOffset;
        SK_OT_CHAR minOriginSB;
        SK_OT_CHAR minAdvanceSB;
        SK_OT_CHAR maxBeforeBL;
        SK_OT_CHAR minAfterBL;
        SK_OT_CHAR pad1;
        SK_OT_CHAR pad2;
    };

    struct BitmapSizeTable {
        SK_OT_ULONG indexSubTableArrayOffset; 
        SK_OT_ULONG indexTablesSize; 
        SK_OT_ULONG numberOfIndexSubTables; 
        SK_OT_ULONG colorRef; 
        SbitLineMetrics hori; 
        SbitLineMetrics vert; 
        SK_OT_USHORT startGlyphIndex; 
        SK_OT_USHORT endGlyphIndex; 
        SK_OT_BYTE ppemX; 
        SK_OT_BYTE ppemY; 
        struct BitDepth {
            SK_TYPED_ENUM(Value, SK_OT_BYTE,
                ((BW, 1))
                ((Gray4, 2))
                ((Gray16, 4))
                ((Gray256, 8))
                SK_SEQ_END,
            SK_SEQ_END)
            SK_OT_BYTE value;
        } bitDepth; 
        union Flags {
            struct Field {
                
                SK_OT_BYTE_BITFIELD(
                    Horizontal, 
                    Vertical,  
                    Reserved02,
                    Reserved03,
                    Reserved04,
                    Reserved05,
                    Reserved06,
                    Reserved07)
            } field;
            struct Raw {
                static const SK_OT_CHAR Horizontal = 1u << 0;
                static const SK_OT_CHAR Vertical = 1u << 1;
                SK_OT_CHAR value;
            } raw;
        } flags;
    }; 

    struct IndexSubTableArray {
        SK_OT_USHORT firstGlyphIndex; 
        SK_OT_USHORT lastGlyphIndex; 
        SK_OT_ULONG additionalOffsetToIndexSubtable; 
    }; 

    struct IndexSubHeader {
        SK_OT_USHORT indexFormat; 
        SK_OT_USHORT imageFormat; 
        SK_OT_ULONG imageDataOffset; 
    };

    
    struct IndexSubTable1 {
        IndexSubHeader header;
        
        
    };

    
    struct IndexSubTable2 {
        IndexSubHeader header;
        SK_OT_ULONG imageSize; 
        SkOTTableEmbeddedBitmapData::BigGlyphMetrics bigMetrics; 
    };

    
    struct IndexSubTable3 {
        IndexSubHeader header;
        
        
    };

    
    struct IndexSubTable4 {
        IndexSubHeader header;
        SK_OT_ULONG numGlyphs;
        struct CodeOffsetPair {
            SK_OT_USHORT glyphCode;
            SK_OT_USHORT offset; 
        }; 
    };

    
    struct IndexSubTable5 {
        IndexSubHeader header;
        SK_OT_ULONG imageSize; 
        SkOTTableEmbeddedBitmapData::BigGlyphMetrics bigMetrics; 
        SK_OT_ULONG numGlyphs;
        
    };

    union IndexSubTable {
        IndexSubHeader header;
        IndexSubTable1 format1;
        IndexSubTable2 format2;
        IndexSubTable3 format3;
        IndexSubTable4 format4;
        IndexSubTable5 format5;
    };

};

#pragma pack(pop)

#endif
