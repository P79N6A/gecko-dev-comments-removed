






#ifndef GrColorTableEffect_DEFINED
#define GrColorTableEffect_DEFINED

#include "GrSingleTextureEffect.h"
#include "GrTexture.h"

class GrGLColorTableEffect;






class GrColorTableEffect : public GrCustomStage {
public:

    GrColorTableEffect(GrTexture* texture);
    virtual ~GrColorTableEffect();

    static const char* Name() { return "ColorTable"; }
    virtual const GrProgramStageFactory& getFactory() const SK_OVERRIDE;
    virtual bool isEqual(const GrCustomStage&) const SK_OVERRIDE;

    virtual int numTextures() const SK_OVERRIDE { return 1; }
    virtual const GrTextureAccess& textureAccess(int index) const SK_OVERRIDE;

    typedef GrGLColorTableEffect GLProgramStage;

private:
    GR_DECLARE_CUSTOM_STAGE_TEST;

    GrTextureAccess fTextureAccess;

    typedef GrCustomStage INHERITED;
};
#endif
