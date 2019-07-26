






#ifndef GrSingleTextureEffect_DEFINED
#define GrSingleTextureEffect_DEFINED

#include "GrEffect.h"
#include "SkMatrix.h"

class GrTexture;





class GrSingleTextureEffect : public GrEffect {
public:
    virtual ~GrSingleTextureEffect();

    const SkMatrix& getMatrix() const { return fMatrix; }

    
    CoordsType coordsType() const { return fCoordsType; }

protected:
    
    GrSingleTextureEffect(GrTexture*, const SkMatrix&, CoordsType = kLocal_CoordsType);
    
    GrSingleTextureEffect(GrTexture*, const SkMatrix&, bool bilerp, CoordsType = kLocal_CoordsType);
    GrSingleTextureEffect(GrTexture*,
                          const SkMatrix&,
                          const GrTextureParams&,
                          CoordsType = kLocal_CoordsType);

    


    bool hasSameTextureParamsMatrixAndCoordsType(const GrSingleTextureEffect& other) const {
        const GrTextureAccess& otherAccess = other.fTextureAccess;
        
        return fTextureAccess.getTexture() == otherAccess.getTexture() &&
               fTextureAccess.getParams() == otherAccess.getParams() &&
               this->getMatrix().cheapEqualTo(other.getMatrix()) &&
               fCoordsType == other.fCoordsType;
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
    GrTextureAccess fTextureAccess;
    SkMatrix        fMatrix;
    CoordsType      fCoordsType;

    typedef GrEffect INHERITED;
};

#endif
