







































#include "CodeGenerator-shared.h"

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
        return false;
    if (!generateBody())
        return false;
    if (!generateEpilogue())
        return false;
    return true;
}

bool
CodeGeneratorShared::visitParameter(LParameter *param)
{
    return true;
}

