






#ifndef GrGLCustomStage_DEFINED
#define GrGLCustomStage_DEFINED

#include "GrAllocator.h"
#include "GrCustomStage.h"
#include "GrGLProgram.h"
#include "GrGLShaderBuilder.h"
#include "GrGLShaderVar.h"
#include "GrGLSL.h"

struct GrGLInterface;
class GrGLTexture;










class GrGLProgramStage {

public:
    typedef GrCustomStage::StageKey StageKey;
    enum {
        
        kProgramStageKeyBits = GrProgramStageFactory::kProgramStageKeyBits,
    };

    typedef GrGLShaderBuilder::TextureSamplerArray TextureSamplerArray;

    GrGLProgramStage(const GrProgramStageFactory&);

    virtual ~GrGLProgramStage();

    
    virtual void setupVariables(GrGLShaderBuilder* builder);

    





    virtual void emitVS(GrGLShaderBuilder* builder,
                        const char* vertexCoords) = 0;

    









    virtual void emitFS(GrGLShaderBuilder* builder,
                        const char* outputColor,
                        const char* inputColor,
                        const TextureSamplerArray&) = 0;

    



    virtual void setData(const GrGLUniformManager&,
                         const GrCustomStage& stage,
                         const GrRenderTarget* renderTarget,
                         int stageNum);

    const char* name() const { return fFactory.name(); }

    static StageKey GenTextureKey(const GrCustomStage&, const GrGLCaps&);

protected:

    const GrProgramStageFactory& fFactory;
};

#endif
