








































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
        for (LInstructionIterator iter = current->begin(); iter != current->end(); iter++) {
            iter->accept(this);
        }
    }
    return true;
}

bool
CodeGenerator::generate()
{
    if (!generatePrologue())
        return false;
    if (!generateBody())
        return false;
    if (!generateEpilogue())
        return false;
    if (!generateOutOfLineCode())
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

