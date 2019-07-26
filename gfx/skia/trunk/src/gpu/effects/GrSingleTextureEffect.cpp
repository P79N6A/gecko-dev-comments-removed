






#include "effects/GrSingleTextureEffect.h"

GrSingleTextureEffect::GrSingleTextureEffect(GrTexture* texture,
                                             const SkMatrix& m,
                                             GrCoordSet coordSet)
    : fCoordTransform(coordSet, m, texture)
    , fTextureAccess(texture) {
    this->addCoordTransform(&fCoordTransform);
    this->addTextureAccess(&fTextureAccess);
}

GrSingleTextureEffect::GrSingleTextureEffect(GrTexture* texture,
                                             const SkMatrix& m,
                                             GrTextureParams::FilterMode filterMode,
                                             GrCoordSet coordSet)
    : fCoordTransform(coordSet, m, texture)
    , fTextureAccess(texture, filterMode) {
    this->addCoordTransform(&fCoordTransform);
    this->addTextureAccess(&fTextureAccess);
}

GrSingleTextureEffect::GrSingleTextureEffect(GrTexture* texture,
                                             const SkMatrix& m,
                                             const GrTextureParams& params,
                                             GrCoordSet coordSet)
    : fCoordTransform(coordSet, m, texture)
    , fTextureAccess(texture, params) {
    this->addCoordTransform(&fCoordTransform);
    this->addTextureAccess(&fTextureAccess);
}

GrSingleTextureEffect::~GrSingleTextureEffect() {
}
