






#include "GrGLSL.h"
#include "GrGLProgramStage.h"

GrGLProgramStage::GrGLProgramStage(const GrProgramStageFactory& factory)
    : fFactory(factory) {
}

GrGLProgramStage::~GrGLProgramStage() {
}



void GrGLProgramStage::setupVariables(GrGLShaderBuilder*) {

}

void GrGLProgramStage::setData(const GrGLUniformManager&,
                               const GrCustomStage&,
                               const GrRenderTarget*,
                               int stageNum) {
}

GrGLProgramStage::StageKey GrGLProgramStage::GenTextureKey(const GrCustomStage& stage,
                                                           const GrGLCaps& caps) {
    StageKey key = 0;
    for (int index = 0; index < stage.numTextures(); ++index) {
        const GrTextureAccess& access = stage.textureAccess(index);
        StageKey value = GrGLShaderBuilder::KeyForTextureAccess(access, caps) << index;
        GrAssert(0 == (value & key)); 
        key |= value;
    }
    return key;
}
