





#ifndef __ARABICSHAPING_H
#define __ARABICSHAPING_H






#include "LETypes.h"
#include "OpenTypeTables.h"

U_NAMESPACE_BEGIN

class LEGlyphStorage;

class ArabicShaping  {
public:
    
    enum JoiningTypes
    {
        JT_NON_JOINING   = 0,
        JT_JOIN_CAUSING  = 1,
        JT_DUAL_JOINING  = 2,
        JT_LEFT_JOINING  = 3,
        JT_RIGHT_JOINING = 4,
        JT_TRANSPARENT   = 5,
        JT_COUNT         = 6
    };

    
    enum ShapingBitMasks
    {
        MASK_SHAPE_RIGHT    = 1, 
        MASK_SHAPE_LEFT     = 2, 
        MASK_TRANSPARENT    = 4, 
        MASK_NOSHAPE        = 8  
    };

    
    enum ShapeTypeValues
    {
        ST_NONE         = 0,
        ST_RIGHT        = MASK_SHAPE_RIGHT,
        ST_LEFT         = MASK_SHAPE_LEFT,
        ST_DUAL         = MASK_SHAPE_RIGHT | MASK_SHAPE_LEFT,
        ST_TRANSPARENT  = MASK_TRANSPARENT,
        ST_NOSHAPE_DUAL = MASK_NOSHAPE | ST_DUAL,
        ST_NOSHAPE_NONE = MASK_NOSHAPE
    };

    typedef le_int32 ShapeType;

    static void shape(const LEUnicode *chars, le_int32 offset, le_int32 charCount, le_int32 charMax,
                      le_bool rightToLeft, LEGlyphStorage &glyphStorage);

    static const FeatureMap *getFeatureMap(le_int32 &count);

private:
    
    ArabicShaping();

    static ShapeType getShapeType(LEUnicode c);

    static const le_uint8 shapingTypeTable[];
    static const size_t   shapingTypeTableLen;

    static const ShapeType shapeTypes[];

    static void adjustTags(le_int32 outIndex, le_int32 shapeOffset, LEGlyphStorage &glyphStorage);
};

U_NAMESPACE_END
#endif
