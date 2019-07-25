








































#include "BytecodeAnalyzer.h"
#include "Ion.h"
#include "jsscriptinlines.h"

using namespace js;
using namespace js::ion;

void
MIRGenerator::c1spew(FILE *fp, const char *pass)
{
    fprintf(fp, "begin_cfg\n");
    fprintf(fp, "  name \"%s\"\n", pass);

    for (size_t i = 0; i < blocks_.length(); i++)
        c1spew(fp, blocks_[i]);

    fprintf(fp, "end_cfg\n");
}

void
MIRGenerator::c1spew(FILE *fp, MBasicBlock *block)
{
    fprintf(fp, "  begin_block\n");
    fprintf(fp, "    name \"B%d\"\n", block->id());
    fprintf(fp, "    from_bci -1\n");
    fprintf(fp, "    to_bci -1\n");

    fprintf(fp, "    predecessors");
    for (uint32 i = 0; i < block->numPredecessors(); i++) {
        MBasicBlock *pred = block->getPredecessor(i);
        fprintf(fp, " \"B%d\"", pred->id());
    }
    fprintf(fp, "\n");

    fprintf(fp, "    successors");
    MControlInstruction *last = block->lastIns();
    for (uint32 i = 0; i < last->numSuccessors(); i++) {
        MBasicBlock *successor = last->getSuccessor(i);
        fprintf(fp, " \"B%d\"", successor->id());
    }
    fprintf(fp, "\n");

    fprintf(fp, "  end_block\n");
}

