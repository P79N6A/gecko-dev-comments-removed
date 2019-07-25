








#include "SkEmbossMaskFilter.h"
#include "SkBlurMaskFilter.h"
#include "SkBlurMask.h"
#include "SkEmbossMask.h"
#include "SkBuffer.h"

static inline int pin2byte(int n) {
    if (n < 0) {
        n = 0;
    } else if (n > 0xFF) {
        n = 0xFF;
    }
    return n;
}

SkMaskFilter* SkBlurMaskFilter::CreateEmboss(const SkScalar direction[3],
                                             SkScalar ambient, SkScalar specular,
                                             SkScalar blurRadius) {
    if (direction == NULL) {
        return NULL;
    }

    
    int am = pin2byte(SkScalarToFixed(ambient) >> 8);

    
    int sp = pin2byte(SkScalarToFixed(specular) >> 12);

    SkEmbossMaskFilter::Light   light;
    
    memcpy(light.fDirection, direction, sizeof(light.fDirection));
    light.fAmbient = SkToU8(am);
    light.fSpecular = SkToU8(sp);
    
    return SkNEW_ARGS(SkEmbossMaskFilter, (light, blurRadius));
}



static void normalize(SkScalar v[3]) {
    SkScalar mag = SkScalarSquare(v[0]) + SkScalarSquare(v[1]) + SkScalarSquare(v[2]);
    mag = SkScalarSqrt(mag);

    for (int i = 0; i < 3; i++) {
        v[i] = SkScalarDiv(v[i], mag);
    }
}

SkEmbossMaskFilter::SkEmbossMaskFilter(const Light& light, SkScalar blurRadius)
        : fLight(light), fBlurRadius(blurRadius) {
    normalize(fLight.fDirection);
}

SkMask::Format SkEmbossMaskFilter::getFormat() {
    return SkMask::k3D_Format;
}

bool SkEmbossMaskFilter::filterMask(SkMask* dst, const SkMask& src,
                                    const SkMatrix& matrix, SkIPoint* margin) {
    SkScalar radius = matrix.mapRadius(fBlurRadius);

    if (!SkBlurMask::Blur(dst, src, radius, SkBlurMask::kInner_Style,
                          SkBlurMask::kLow_Quality)) {
        return false;
    }

    dst->fFormat = SkMask::k3D_Format;
    if (margin) {
        margin->set(SkScalarCeil(radius), SkScalarCeil(radius));
    }

    if (src.fImage == NULL) {
        return true;
    }

    

    {
        uint8_t* alphaPlane = dst->fImage;
        size_t   planeSize = dst->computeImageSize();
        if (0 == planeSize) {
            return false;   
        }
        dst->fImage = SkMask::AllocImage(planeSize * 3);
        memcpy(dst->fImage, alphaPlane, planeSize);
        SkMask::FreeImage(alphaPlane);
    }

    
    Light   light = fLight;
    matrix.mapVectors((SkVector*)(void*)light.fDirection,
                      (SkVector*)(void*)fLight.fDirection, 1);

    
    
    SkVector* vec = (SkVector*)(void*)light.fDirection;
    vec->setLength(light.fDirection[0],
                   light.fDirection[1],
                   SkPoint::Length(fLight.fDirection[0], fLight.fDirection[1]));

    SkEmbossMask::Emboss(dst, light);

    
    memcpy(dst->fImage, src.fImage, src.computeImageSize());

    return true;
}

SkFlattenable* SkEmbossMaskFilter::CreateProc(SkFlattenableReadBuffer& buffer) {
    return SkNEW_ARGS(SkEmbossMaskFilter, (buffer));
}

SkFlattenable::Factory SkEmbossMaskFilter::getFactory() {
    return CreateProc;
}

SkEmbossMaskFilter::SkEmbossMaskFilter(SkFlattenableReadBuffer& buffer)
        : SkMaskFilter(buffer) {
    buffer.read(&fLight, sizeof(fLight));
    SkASSERT(fLight.fPad == 0); 
    fBlurRadius = buffer.readScalar();
}

void SkEmbossMaskFilter::flatten(SkFlattenableWriteBuffer& buffer) {
    this->INHERITED::flatten(buffer);

    fLight.fPad = 0;    
    buffer.writeMul4(&fLight, sizeof(fLight));
    buffer.writeScalar(fBlurRadius);
}

