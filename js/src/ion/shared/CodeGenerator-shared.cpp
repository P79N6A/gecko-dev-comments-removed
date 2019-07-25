







































#include "CodeGenerator-shared.h"
#include "ion/MIRGenerator.h"

using namespace js;
using namespace js::ion;

CodeGeneratorShared::CodeGeneratorShared(MIRGenerator *gen, LIRGraph &graph)
  : gen(gen),
    graph(graph)
{
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
        return NULL;
    if (!generateBody())
        return NULL;

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

