




#ifndef GFX_PATTERN_H
#define GFX_PATTERN_H

#include "gfxTypes.h"

#include "gfxColor.h"
#include "gfxMatrix.h"
#include "nsISupportsImpl.h"
#include "nsAutoPtr.h"
#include "mozilla/Alignment.h"
#include "mozilla/gfx/2D.h"
#include "GraphicsFilter.h"

class gfxContext;
class gfxASurface;
typedef struct _cairo_pattern cairo_pattern_t;


class gfxPattern {
    NS_INLINE_DECL_REFCOUNTING(gfxPattern)

public:
    gfxPattern(cairo_pattern_t *aPattern);
    gfxPattern(const gfxRGBA& aColor);
    gfxPattern(gfxASurface *surface); 
    
    gfxPattern(gfxFloat x0, gfxFloat y0, gfxFloat x1, gfxFloat y1); 
    gfxPattern(gfxFloat cx0, gfxFloat cy0, gfxFloat radius0,
               gfxFloat cx1, gfxFloat cy1, gfxFloat radius1); 
    gfxPattern(mozilla::gfx::SourceSurface *aSurface,
               const mozilla::gfx::Matrix &aTransform); 
    virtual ~gfxPattern();

    cairo_pattern_t *CairoPattern();
    void AddColorStop(gfxFloat offset, const gfxRGBA& c);
    void SetColorStops(mozilla::RefPtr<mozilla::gfx::GradientStops> aStops);

    void SetMatrix(const gfxMatrix& matrix);
    gfxMatrix GetMatrix() const;
    gfxMatrix GetInverseMatrix() const;

    




    mozilla::gfx::Pattern *GetPattern(mozilla::gfx::DrawTarget *aTarget,
                                      mozilla::gfx::Matrix *aPatternTransform = nullptr);
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

    already_AddRefed<gfxASurface> GetSurface();

    bool IsAzure() { return !mPattern; }

    mozilla::TemporaryRef<mozilla::gfx::SourceSurface> GetAzureSurface() { return mSourceSurface; }

protected:
    cairo_pattern_t *mPattern;

    










    void AdjustTransformForPattern(mozilla::gfx::Matrix &aPatternTransform,
                                   const mozilla::gfx::Matrix &aCurrentTransform,
                                   const mozilla::gfx::Matrix *aOriginalTransform);

    union {
      mozilla::AlignedStorage2<mozilla::gfx::ColorPattern> mColorPattern;
      mozilla::AlignedStorage2<mozilla::gfx::LinearGradientPattern> mLinearGradientPattern;
      mozilla::AlignedStorage2<mozilla::gfx::RadialGradientPattern> mRadialGradientPattern;
      mozilla::AlignedStorage2<mozilla::gfx::SurfacePattern> mSurfacePattern;
    };

    mozilla::gfx::Pattern *mGfxPattern;

    mozilla::RefPtr<mozilla::gfx::SourceSurface> mSourceSurface;
    mozilla::gfx::Matrix mTransform;
    mozilla::RefPtr<mozilla::gfx::GradientStops> mStops;
    GraphicsExtend mExtend;
    mozilla::gfx::Filter mFilter;
};

#endif 
