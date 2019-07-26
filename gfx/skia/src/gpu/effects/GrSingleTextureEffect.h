






#ifndef GrSingleTextureEffect_DEFINED
#define GrSingleTextureEffect_DEFINED

#include "GrCustomStage.h"

class GrGLSingleTextureEffect;




class GrSingleTextureEffect : public GrCustomStage {

public:
    GrSingleTextureEffect(GrTexture* texture);
    virtual ~GrSingleTextureEffect();

    virtual int numTextures() const SK_OVERRIDE;
    virtual const GrTextureAccess& textureAccess(int index) const SK_OVERRIDE;

    static const char* Name() { return "Single Texture"; }

    typedef GrGLSingleTextureEffect GLProgramStage;

    virtual const GrProgramStageFactory& getFactory() const SK_OVERRIDE;

private:
    GR_DECLARE_CUSTOM_STAGE_TEST;

    GrTextureAccess fTextureAccess;

    typedef GrCustomStage INHERITED;
};

#endif
