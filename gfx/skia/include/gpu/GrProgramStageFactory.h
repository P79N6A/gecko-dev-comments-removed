






#ifndef GrProgramStageFactory_DEFINED
#define GrProgramStageFactory_DEFINED

#include "GrTypes.h"
#include "SkTemplates.h"
#include "GrNoncopyable.h"






class GrCustomStage;
class GrGLProgramStage;
class GrGLCaps;

class GrProgramStageFactory : public GrNoncopyable {
public:
    typedef uint32_t StageKey;
    enum {
        kProgramStageKeyBits = 10,
        kTexturingStageKeyBits = 6
    };

    virtual StageKey glStageKey(const GrCustomStage& stage,
                                const GrGLCaps& caps ) const = 0;
    virtual GrGLProgramStage* createGLInstance(
        const GrCustomStage& stage) const = 0;

    bool operator ==(const GrProgramStageFactory& b) const {
        return fStageClassID == b.fStageClassID;
    }
    bool operator !=(const GrProgramStageFactory& b) const {
        return !(*this == b);
    }

    virtual const char* name() const = 0;

protected:
    enum {
        kIllegalStageClassID = 0,
    };

    GrProgramStageFactory() {
        fStageClassID = kIllegalStageClassID;
    }

    static StageKey GenID() {
        
        
        
        int32_t id = sk_atomic_inc(&fCurrStageClassID) + 1;
        GrAssert(id < (1 << (8 * sizeof(StageKey) - kProgramStageKeyBits)));
        return id;
    }

    StageKey fStageClassID;

private:
    static int32_t fCurrStageClassID;
};

template <typename StageClass>
class GrTProgramStageFactory : public GrProgramStageFactory {

public:
    typedef typename StageClass::GLProgramStage GLProgramStage;

    


    virtual const char* name() const SK_OVERRIDE { return StageClass::Name(); }

    




    virtual StageKey glStageKey(const GrCustomStage& stage,
                                const GrGLCaps& caps) const SK_OVERRIDE {
        GrAssert(kIllegalStageClassID != fStageClassID);
        StageKey stageID = GLProgramStage::GenKey(stage, caps);
        StageKey textureKey = GLProgramStage::GenTextureKey(stage, caps);
#if GR_DEBUG
        static const StageKey kIllegalIDMask =
            (uint16_t) (~((1U << kProgramStageKeyBits) - 1));
        GrAssert(!(kIllegalIDMask & stageID));

        static const StageKey kIllegalTextureKeyMask =
            (uint16_t) (~((1U << kTexturingStageKeyBits) - 1));
        GrAssert(!(kIllegalTextureKeyMask & textureKey));
#endif
        return fStageClassID | (textureKey << kProgramStageKeyBits) | stageID;
    }

    


    virtual GLProgramStage* createGLInstance(
                        const GrCustomStage& stage) const SK_OVERRIDE {
        return SkNEW_ARGS(GLProgramStage, (*this, stage));
    }

    

    static const GrProgramStageFactory& getInstance() {
        static SkAlignedSTStorage<1, GrTProgramStageFactory> gInstanceMem;
        static const GrTProgramStageFactory* gInstance;
        if (!gInstance) {
            gInstance = SkNEW_PLACEMENT(gInstanceMem.get(),
                                        GrTProgramStageFactory);
        }
        return *gInstance;
    }

protected:
    GrTProgramStageFactory() {
        fStageClassID = GenID() << (kProgramStageKeyBits + kTexturingStageKeyBits) ;
    }
};

#endif
