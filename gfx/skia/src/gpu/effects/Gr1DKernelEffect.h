






#ifndef Gr1DKernelEffect_DEFINED
#define Gr1DKernelEffect_DEFINED

#include "GrSingleTextureEffect.h"









class Gr1DKernelEffect : public GrSingleTextureEffect {

public:
    enum Direction {
        kX_Direction,
        kY_Direction,
    };

    Gr1DKernelEffect(GrTexture* texture,
                     Direction direction,
                     int radius)
        : GrSingleTextureEffect(texture)
        , fDirection(direction)
        , fRadius(radius) {}

    virtual ~Gr1DKernelEffect() {};

    static int WidthFromRadius(int radius) { return 2 * radius + 1; }

    int radius() const { return fRadius; }
    int width() const { return WidthFromRadius(fRadius); }
    Direction direction() const { return fDirection; }

private:

    Direction       fDirection;
    int             fRadius;

    typedef GrSingleTextureEffect INHERITED;
};

#endif
