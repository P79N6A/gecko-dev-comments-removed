






#ifndef GrFontScaler_DEFINED
#define GrFontScaler_DEFINED

#include "GrGlyph.h"
#include "GrKey.h"

class SkPath;








class GrFontScaler : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(GrFontScaler)

    virtual const GrKey* getKey() = 0;
    virtual GrMaskFormat getMaskFormat() = 0;
    virtual bool getPackedGlyphBounds(GrGlyph::PackedID, SkIRect* bounds) = 0;
    virtual bool getPackedGlyphImage(GrGlyph::PackedID, int width, int height,
                                     int rowBytes, void* image) = 0;
    virtual bool getGlyphPath(uint16_t glyphID, SkPath*) = 0;

private:
    typedef SkRefCnt INHERITED;
};

#endif
