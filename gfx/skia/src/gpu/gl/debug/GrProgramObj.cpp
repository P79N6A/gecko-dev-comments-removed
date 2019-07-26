







#include "GrProgramObj.h"
#include "GrShaderObj.h"

void GrProgramObj::AttachShader(GrShaderObj *shader) {
    shader->ref();
    fShaders.push_back(shader);
}

void GrProgramObj::deleteAction() {

    
    
    for (int i = 0; i < fShaders.count(); ++i) {
        fShaders[i]->unref();
    }
    fShaders.reset();

    this->INHERITED::deleteAction();
}
