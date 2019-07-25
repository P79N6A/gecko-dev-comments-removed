









#ifndef GrGlyph_DEFINED
#define GrGlyph_DEFINED

#include "GrPath.h"
#include "GrRect.h"

class GrAtlas;







struct GrGlyph {
    typedef uint32_t PackedID;

    GrAtlas*    fAtlas;
    GrPath*     fPath;
    PackedID    fPackedID;
    GrIRect16   fBounds;
    GrIPoint16  fAtlasLocation;

    void init(GrGlyph::PackedID packed, const GrIRect& bounds) {
        fAtlas = NULL;
        fPath = NULL;
        fPackedID = packed;
        fBounds.set(bounds);
        fAtlasLocation.set(0, 0);
    }
    
    void free() {
        if (fPath) {
            delete fPath;
            fPath = NULL;
        }
    }
    
    int width() const { return fBounds.width(); }
    int height() const { return fBounds.height(); }
    bool isEmpty() const { return fBounds.isEmpty(); }
    uint16_t glyphID() const { return UnpackID(fPackedID); }

    
    
    static inline unsigned ExtractSubPixelBitsFromFixed(GrFixed pos) {
        
        return (pos >> 14) & 3;
    }
    
    static inline PackedID Pack(uint16_t glyphID, GrFixed x, GrFixed y) {
        x = ExtractSubPixelBitsFromFixed(x);
        y = ExtractSubPixelBitsFromFixed(y);
        return (x << 18) | (y << 16) | glyphID;
    }
    
    static inline GrFixed UnpackFixedX(PackedID packed) {
        return ((packed >> 18) & 3) << 14;
    }
    
    static inline GrFixed UnpackFixedY(PackedID packed) {
        return ((packed >> 16) & 3) << 14;
    }
    
    static inline uint16_t UnpackID(PackedID packed) {
        return (uint16_t)packed;
    }
};


#endif

