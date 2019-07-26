






#ifndef GrCustomStageUnitTest_DEFINED
#define GrCustomStageUnitTest_DEFINED

#include "SkRandom.h"
#include "GrNoncopyable.h"
#include "SkTArray.h"

namespace GrCustomStageUnitTest {

enum {
    kSkiaPMTextureIdx = 0,
    kAlphaTextureIdx = 1,
};
}

#if SK_ALLOW_STATIC_GLOBAL_INITIALIZERS

class GrCustomStage;
class GrContext;
class GrTexture;

class GrCustomStageTestFactory : GrNoncopyable {
public:

    typedef GrCustomStage* (*CreateProc)(SkRandom*, GrContext*, GrTexture* dummyTextures[]);

    GrCustomStageTestFactory(CreateProc createProc) {
        fCreateProc = createProc;
        GetFactories()->push_back(this);
    }

    static GrCustomStage* CreateStage(SkRandom* random,
                                      GrContext* context,
                                      GrTexture* dummyTextures[]) {
        uint32_t idx = random->nextRangeU(0, GetFactories()->count() - 1);
        GrCustomStageTestFactory* factory = (*GetFactories())[idx];
        return factory->fCreateProc(random, context, dummyTextures);
    }

private:
    CreateProc fCreateProc;
    static SkTArray<GrCustomStageTestFactory*, true>* GetFactories();
};




#define GR_DECLARE_CUSTOM_STAGE_TEST                                           \
    static GrCustomStageTestFactory gTestFactory;                              \
    static GrCustomStage* TestCreate(SkRandom*, GrContext*, GrTexture* dummyTextures[2])








#define GR_DEFINE_CUSTOM_STAGE_TEST(CustomStage)                               \
    GrCustomStageTestFactory CustomStage :: gTestFactory(CustomStage :: TestCreate)

#else 



#define GR_DECLARE_CUSTOM_STAGE_TEST \
    static GrCustomStage* TestCreate(SkRandom*, GrContext*, GrTexture* dummyTextures[2])
#define GR_DEFINE_CUSTOM_STAGE_TEST(X)

#endif 
#endif
