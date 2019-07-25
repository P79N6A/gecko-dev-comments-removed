







































#include "CodeGenerator-shared.h"
#include "ion/MIRGenerator.h"
#include "ion/IonFrames.h"
#include "ion/IonLinker.h"

using namespace js;
using namespace js::ion;

CodeGeneratorShared::CodeGeneratorShared(MIRGenerator *gen, LIRGraph &graph)
  : gen(gen),
    graph(graph),
    frameDepth_(graph.localSlotCount() * sizeof(STACK_SLOT_SIZE))
{
    frameClass_ = FrameSizeClass::FromDepth(frameDepth_);
    frameStaticSize_ = frameClass_.frameSize();
}

bool
CodeGeneratorShared::generateBody()
{
    for (size_t i = 0; i < graph.numBlocks(); i++) {
        current = graph.getBlock(i);
        for (LInstructionIterator iter = current->begin(); iter != current->end(); iter++) {
            iter->accept(this);
        }
    }
    return true;
}

bool
CodeGeneratorShared::generate()
{
    if (!generatePrologue())
        return false;
    if (!generateBody())
        return false;
    if (!generateEpilogue())
        return false;

    Linker linker(masm);
    IonCode *code = linker.newCode(GetIonContext()->cx);
    if (!code)
        return false;

    if (!gen->script->ion) {
        gen->script->ion= IonScript::New(GetIonContext()->cx);
        if (!gen->script->ion)
            return false;
    }

    gen->script->ion->setMethod(code);
    return true;
}

bool
CodeGeneratorShared::visitParameter(LParameter *param)
{
    return true;
}

