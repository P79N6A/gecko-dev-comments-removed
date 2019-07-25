








































#include "IonBuilder.h"
#include "Ion.h"
#include "IonSpew.h"
#include "MIRGraph.h"
#include "IonLIR.h"
#include "jsscriptinlines.h"

using namespace js;
using namespace js::ion;

C1Spewer::C1Spewer(MIRGraph &graph, JSScript *script)
  : graph(graph),
    script(script),
    spewout_(NULL)
{
}

C1Spewer::~C1Spewer()
{
    fclose(spewout_);
}

void
C1Spewer::enable(const char *path)
{
    spewout_ = fopen(path, "w");
    if (!spewout_)
        return;

    fprintf(spewout_, "begin_compilation\n");
    fprintf(spewout_, "  name \"%s:%d\"\n", script->filename, script->lineno);
    fprintf(spewout_, "  method \"%s:%d\"\n", script->filename, script->lineno);
    fprintf(spewout_, "  date %d\n", (int)time(NULL));
    fprintf(spewout_, "end_compilation\n");
}

void
C1Spewer::spew(const char *pass)
{
    if (spewout_)
        spew(spewout_, pass);
}

void
C1Spewer::spew(FILE *fp, const char *pass)
{
    fprintf(fp, "begin_cfg\n");
    fprintf(fp, "  name \"%s\"\n", pass);

    for (size_t i = 0; i < graph.numBlocks(); i++)
        spew(fp, graph.getBlock(i));

    fprintf(fp, "end_cfg\n");
}

static void
DumpInstruction(FILE *fp, MInstruction *ins)
{
    fprintf(fp, "      ");
    fprintf(fp, "0 %d ", (int)ins->useCount());
    ins->printName(fp);
    fprintf(fp, " ");
    ins->printOpcode(fp);
    fprintf(fp, " <|@\n");
}

static void
DumpLIR(FILE *fp, LInstruction *ins)
{
    fprintf(fp, "      ");
    fprintf(fp, "%d ", ins->id());
    ins->print(fp);
    fprintf(fp, " <|@\n");
}

void
C1Spewer::spew(FILE *fp, MBasicBlock *block)
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

    fprintf(fp, "    xhandlers\n");
    fprintf(fp, "    flags\n");

    if (block->lir() && block->lir()->begin() != block->lir()->end()) {
        LInstruction *first = *block->lir()->begin();
        LInstruction *last = *(block->lir()->end().prev());
        fprintf(fp, "    first_lir_id %d\n", first->id());
        fprintf(fp, "    last_lir_id %d\n", last->id());
    }

    fprintf(fp, "    begin_states\n");

    fprintf(fp, "      begin_locals\n");
    fprintf(fp, "        size %d\n", (int)block->numEntrySlots());
    fprintf(fp, "        method \"None\"\n");
    for (uint32 i = 0; i < block->numEntrySlots(); i++) {
        MInstruction *ins = block->getEntrySlot(i);
        fprintf(fp, "        ");
        fprintf(fp, "%d ", i);
        ins->printName(fp);
        fprintf(fp, "\n");
    }
    fprintf(fp, "      end_locals\n");

    fprintf(fp, "    end_states\n");

    fprintf(fp, "    begin_HIR\n");
    for (size_t i = 0; i < block->numPhis(); i++)
        DumpInstruction(fp, block->getPhi(i));
    for (MInstructionIterator i = block->begin(); i != block->end(); i++)
        DumpInstruction(fp, *i);
    fprintf(fp, "    end_HIR\n");

    if (block->lir()) {
        fprintf(fp, "    begin_LIR\n");
        for (LInstructionIterator i = block->lir()->begin(); i != block->lir()->end(); i++)
            DumpLIR(fp, *i);
        fprintf(fp, "    end_LIR\n");
    }

    fprintf(fp, "  end_block\n");
}

