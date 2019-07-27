






#ifndef GrEffectUnitTest_DEFINED
#define GrEffectUnitTest_DEFINED

#include "SkRandom.h"
#include "SkTArray.h"
#include "SkTypes.h"

class SkMatrix;
class GrDrawTargetCaps;

namespace GrEffectUnitTest {

enum {
    kSkiaPMTextureIdx = 0,
    kAlphaTextureIdx = 1,
};




const SkMatrix& TestMatrix(SkRandom*);

}

#if SK_ALLOW_STATIC_GLOBAL_INITIALIZERS

class GrContext;
class GrEffect;
class GrTexture;

class GrEffectTestFactory : SkNoncopyable {
public:

    typedef GrEffect* (*CreateProc)(SkRandom*,
                                    GrContext*,
                                    const GrDrawTargetCaps& caps,
                                    GrTexture* dummyTextures[]);

    GrEffectTestFactory(CreateProc createProc) {
        fCreateProc = createProc;
        GetFactories()->push_back(this);
    }

    static GrEffect* CreateStage(SkRandom* random,
                                 GrContext* context,
                                 const GrDrawTargetCaps& caps,
                                 GrTexture* dummyTextures[]) {
        uint32_t idx = random->nextRangeU(0, GetFactories()->count() - 1);
        GrEffectTestFactory* factory = (*GetFactories())[idx];
        return factory->fCreateProc(random, context, caps, dummyTextures);
    }

private:
    CreateProc fCreateProc;
    static SkTArray<GrEffectTestFactory*, true>* GetFactories();
};




#define GR_DECLARE_EFFECT_TEST                                                      \
    static GrEffectTestFactory gTestFactory;                                        \
    static GrEffect* TestCreate(SkRandom*,                                          \
                                GrContext*,                                         \
                                const GrDrawTargetCaps&,                            \
                                GrTexture* dummyTextures[2])












#define GR_DEFINE_EFFECT_TEST(Effect)                                               \
    GrEffectTestFactory Effect :: gTestFactory(Effect :: TestCreate)

#else 



#define GR_DECLARE_EFFECT_TEST                                                      \
    static GrEffect* TestCreate(SkRandom*,                                          \
                                GrContext*,                                         \
                                const GrDrawTargetCaps&,                            \
                                GrTexture* dummyTextures[2])
#define GR_DEFINE_EFFECT_TEST(X)

#endif 
#endif
