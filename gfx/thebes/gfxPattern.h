




































#ifndef GFX_PATTERN_H
#define GFX_PATTERN_H

#include "gfxTypes.h"

#include "gfxColor.h"
#include "gfxMatrix.h"
#include "nsISupportsImpl.h"
#include "nsAutoPtr.h"

class gfxContext;
class gfxASurface;
typedef struct _cairo_pattern cairo_pattern_t;


class THEBES_API gfxPattern {
    NS_INLINE_DECL_REFCOUNTING(gfxPattern)

public:
    gfxPattern(cairo_pattern_t *aPattern);
    gfxPattern(const gfxRGBA& aColor);
    gfxPattern(gfxASurface *surface); 
    
    gfxPattern(gfxFloat x0, gfxFloat y0, gfxFloat x1, gfxFloat y1); 
    gfxPattern(gfxFloat cx0, gfxFloat cy0, gfxFloat radius0,
               gfxFloat cx1, gfxFloat cy1, gfxFloat radius1); 
    virtual ~gfxPattern();

    cairo_pattern_t *CairoPattern();
    void AddColorStop(gfxFloat offset, const gfxRGBA& c);

    void SetMatrix(const gfxMatrix& matrix);
    gfxMatrix GetMatrix() const;

    enum GraphicsExtend {
        EXTEND_NONE,
        EXTEND_REPEAT,
        EXTEND_REFLECT,
        EXTEND_PAD,

        
        
        
        
        
        
        
        
        EXTEND_PAD_EDGE = 1000
    };

    
    void SetExtend(GraphicsExtend extend);
    GraphicsExtend Extend() const;

    enum GraphicsPatternType {
        PATTERN_SOLID,
        PATTERN_SURFACE,
        PATTERN_LINEAR,
        PATTERN_RADIAL
    };

    GraphicsPatternType GetType() const;

    int CairoStatus();

    enum GraphicsFilter {
        FILTER_FAST,
        FILTER_GOOD,
        FILTER_BEST,
        FILTER_NEAREST,
        FILTER_BILINEAR,
        FILTER_GAUSSIAN
    };

    void SetFilter(GraphicsFilter filter);
    GraphicsFilter Filter() const;

    
    bool GetSolidColor(gfxRGBA& aColor);

    already_AddRefed<gfxASurface> GetSurface();

protected:
    cairo_pattern_t *mPattern;
};

#endif 
