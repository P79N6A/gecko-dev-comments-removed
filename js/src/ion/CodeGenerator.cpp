








































#include "CodeGenerator.h"
#include "IonLinker.h"
#include "MIRGenerator.h"

using namespace js;
using namespace js::ion;

CodeGenerator::CodeGenerator(MIRGenerator *gen, LIRGraph &graph)
  : CodeGeneratorSpecific(gen, graph)
{
}

bool
CodeGenerator::generateBody()
{
    for (size_t i = 0; i < graph.numBlocks(); i++) {
        current = graph.getBlock(i);
        masm.bind(current->label());
        for (LInstructionIterator iter = current->begin(); iter != current->end(); iter++) {
            if (!iter->accept(this))
                return false;
        }
        if (masm.oom())
            return false;
    }
    return true;
}

bool
CodeGenerator::generate()
{
    JSContext *cx = gen->cx;

    if (frameClass_ != FrameSizeClass::None()) {
        deoptTable_ = cx->compartment->ionCompartment()->getBailoutTable(cx, frameClass_);
        if (!deoptTable_)
            return false;
    }

    if (!generatePrologue())
        return false;
    if (!generateBody())
        return false;
    if (!generateEpilogue())
        return false;
    if (!generateOutOfLineCode())
        return false;

    if (masm.oom())
        return false;

    Linker linker(masm);
    IonCode *code = linker.newCode(cx);
    if (!code)
        return false;

    JS_ASSERT(!gen->script->ion);

    gen->script->ion = IonScript::New(cx, snapshots_.length(), bailouts_.length());
    if (!gen->script->ion)
        return false;

    gen->script->ion->setMethod(code);
    gen->script->ion->setDeoptTable(deoptTable_);
    if (snapshots_.length())
        gen->script->ion->copySnapshots(&snapshots_);
    if (bailouts_.length())
        gen->script->ion->copyBailoutTable(&bailouts_[0]);
    return true;
}

