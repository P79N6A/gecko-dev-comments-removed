






#ifndef GrSingleTextureEffect_DEFINED
#define GrSingleTextureEffect_DEFINED

#include "GrEffect.h"
#include "SkMatrix.h"
#include "GrCoordTransform.h"

class GrTexture;





class GrSingleTextureEffect : public GrEffect {
public:
    virtual ~GrSingleTextureEffect();

protected:
    
    GrSingleTextureEffect(GrTexture*, const SkMatrix&, GrCoordSet = kLocal_GrCoordSet);
    
    GrSingleTextureEffect(GrTexture*, const SkMatrix&, GrTextureParams::FilterMode filterMode,
                          GrCoordSet = kLocal_GrCoordSet);
    GrSingleTextureEffect(GrTexture*,
                          const SkMatrix&,
                          const GrTextureParams&,
                          GrCoordSet = kLocal_GrCoordSet);

    


    bool hasSameTextureParamsMatrixAndSourceCoords(const GrSingleTextureEffect& other) const {
        
        return fTextureAccess == other.fTextureAccess &&
               fCoordTransform.getMatrix().cheapEqualTo(other.fCoordTransform.getMatrix()) &&
               fCoordTransform.sourceCoords() == other.fCoordTransform.sourceCoords();
    }

    




    void updateConstantColorComponentsForModulation(GrColor* color, uint32_t* validFlags) const {
        if ((*validFlags & kA_GrColorComponentFlag) && 0xFF == GrColorUnpackA(*color) &&
            GrPixelConfigIsOpaque(this->texture(0)->config())) {
            *validFlags = kA_GrColorComponentFlag;
        } else {
            *validFlags = 0;
        }
    }

private:
    GrCoordTransform fCoordTransform;
    GrTextureAccess  fTextureAccess;

    typedef GrEffect INHERITED;
};

#endif
