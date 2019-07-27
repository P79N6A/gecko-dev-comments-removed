




#ifndef GFX_PATTERN_H
#define GFX_PATTERN_H

#include "gfxTypes.h"

#include "gfxMatrix.h"
#include "mozilla/Alignment.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/gfx/PatternHelpers.h"
#include "GraphicsFilter.h"
#include "nsISupportsImpl.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"

struct gfxRGBA;
typedef struct _cairo_pattern cairo_pattern_t;


class gfxPattern final{
    NS_INLINE_DECL_REFCOUNTING(gfxPattern)

public:
    explicit gfxPattern(const gfxRGBA& aColor);
    
    gfxPattern(gfxFloat x0, gfxFloat y0, gfxFloat x1, gfxFloat y1); 
    gfxPattern(gfxFloat cx0, gfxFloat cy0, gfxFloat radius0,
               gfxFloat cx1, gfxFloat cy1, gfxFloat radius1); 
    gfxPattern(mozilla::gfx::SourceSurface *aSurface,
               const mozilla::gfx::Matrix &aPatternToUserSpace);

    void AddColorStop(gfxFloat offset, const gfxRGBA& c);
    void SetColorStops(mozilla::gfx::GradientStops* aStops);

    
    
    
    void CacheColorStops(const mozilla::gfx::DrawTarget *aDT);

    void SetMatrix(const gfxMatrix& matrix);
    gfxMatrix GetMatrix() const;
    gfxMatrix GetInverseMatrix() const;

    




    mozilla::gfx::Pattern *GetPattern(const mozilla::gfx::DrawTarget *aTarget,
                                      mozilla::gfx::Matrix *aOriginalUserToDevice = nullptr);
    bool IsOpaque();

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

    void SetFilter(GraphicsFilter filter);
    GraphicsFilter Filter() const;

    
    bool GetSolidColor(gfxRGBA& aColor);

private:
    
    ~gfxPattern() {}

    mozilla::gfx::GeneralPattern mGfxPattern;
    mozilla::RefPtr<mozilla::gfx::SourceSurface> mSourceSurface;
    mozilla::gfx::Matrix mPatternToUserSpace;
    mozilla::RefPtr<mozilla::gfx::GradientStops> mStops;
    nsTArray<mozilla::gfx::GradientStop> mStopsList;
    GraphicsExtend mExtend;
};

#endif 
