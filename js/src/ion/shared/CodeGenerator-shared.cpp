







































#include "CodeGenerator-shared.h"
#include "ion/MIRGenerator.h"
#include "ion/IonFrames.h"

using namespace js;
using namespace js::ion;

CodeGeneratorShared::CodeGeneratorShared(MIRGenerator *gen, LIRGraph &graph)
  : gen(gen),
    graph(graph),
    frameDepth_(graph.localSlotCount() * sizeof(STACK_SLOT_SIZE))
{
    frameClass_ = FrameSizeClass::FromDepth(frameDepth_);
}

bool
CodeGeneratorShared::generateOutOfLineCode()
{
    for (size_t i = 0; i < outOfLineCode_.length(); i++) {
        if (!outOfLineCode_[i]->generate(this))
            return false;
    }

    return true;
}

bool
CodeGeneratorShared::visitParameter(LParameter *param)
{
    return true;
}

bool
CodeGeneratorShared::addOutOfLineCode(OutOfLineCode *code)
{
    return outOfLineCode_.append(code);
}

