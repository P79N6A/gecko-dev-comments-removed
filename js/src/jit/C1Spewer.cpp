





#ifdef DEBUG

#include "jit/C1Spewer.h"

#include "mozilla/SizePrintfMacros.h"

#include <time.h>

#include "jit/BacktrackingAllocator.h"
#include "jit/LIR.h"
#include "jit/MIRGraph.h"

#include "vm/Printer.h"

using namespace js;
using namespace js::jit;

void
C1Spewer::beginFunction(MIRGraph* graph, JSScript* script)
{
    this->graph  = graph;

    out_.printf("begin_compilation\n");
    if (script) {
        out_.printf("  name \"%s:%" PRIuSIZE "\"\n", script->filename(), script->lineno());
        out_.printf("  method \"%s:%" PRIuSIZE "\"\n", script->filename(), script->lineno());
    } else {
        out_.printf("  name \"asm.js compilation\"\n");
        out_.printf("  method \"asm.js compilation\"\n");
    }
    out_.printf("  date %d\n", (int)time(nullptr));
    out_.printf("end_compilation\n");
}

void
C1Spewer::spewPass(const char* pass)
{
    out_.printf("begin_cfg\n");
    out_.printf("  name \"%s\"\n", pass);

    for (MBasicBlockIterator block(graph->begin()); block != graph->end(); block++)
        spewPass(out_, *block);

    out_.printf("end_cfg\n");
}

void
C1Spewer::spewIntervals(const char* pass, BacktrackingAllocator* regalloc)
{
    out_.printf("begin_intervals\n");
    out_.printf(" name \"%s\"\n", pass);

    size_t nextId = 0x4000;
    for (MBasicBlockIterator block(graph->begin()); block != graph->end(); block++)
        spewIntervals(out_, *block, regalloc, nextId);

    out_.printf("end_intervals\n");
}

void
C1Spewer::endFunction()
{
}

static void
DumpDefinition(GenericPrinter& out, MDefinition* def)
{
    out.printf("      ");
    out.printf("%u %u ", def->id(), unsigned(def->useCount()));
    def->printName(out);
    out.printf(" ");
    def->printOpcode(out);
    out.printf(" <|@\n");
}

static void
DumpLIR(GenericPrinter& out, LNode* ins)
{
    out.printf("      ");
    out.printf("%d ", ins->id());
    ins->dump(out);
    out.printf(" <|@\n");
}

void
C1Spewer::spewIntervals(GenericPrinter& out, BacktrackingAllocator* regalloc, LNode* ins, size_t& nextId)
{
    for (size_t k = 0; k < ins->numDefs(); k++) {
        uint32_t id = ins->getDef(k)->virtualRegister();
        VirtualRegister* vreg = &regalloc->vregs[id];

        for (size_t i = 0; i < vreg->numIntervals(); i++) {
            LiveInterval* live = vreg->getInterval(i);
            if (live->numRanges()) {
                out.printf("%d object \"", (i == 0) ? id : int32_t(nextId++));
                out.printf("%s", live->getAllocation()->toString());
                out.printf("\" %d -1", id);
                for (size_t j = 0; j < live->numRanges(); j++) {
                    out.printf(" [%u, %u[", live->getRange(j)->from.bits(),
                               live->getRange(j)->to.bits());
                }
                for (UsePositionIterator usePos(live->usesBegin()); usePos != live->usesEnd(); usePos++)
                    out.printf(" %u M", usePos->pos.bits());
                out.printf(" \"\"\n");
            }
        }
    }
}

void
C1Spewer::spewIntervals(GenericPrinter& out, MBasicBlock* block, BacktrackingAllocator* regalloc, size_t& nextId)
{
    LBlock* lir = block->lir();
    if (!lir)
        return;

    for (size_t i = 0; i < lir->numPhis(); i++)
        spewIntervals(out, regalloc, lir->getPhi(i), nextId);

    for (LInstructionIterator ins = lir->begin(); ins != lir->end(); ins++)
        spewIntervals(out, regalloc, *ins, nextId);
}

void
C1Spewer::spewPass(GenericPrinter& out, MBasicBlock* block)
{
    out.printf("  begin_block\n");
    out.printf("    name \"B%d\"\n", block->id());
    out.printf("    from_bci -1\n");
    out.printf("    to_bci -1\n");

    out.printf("    predecessors");
    for (uint32_t i = 0; i < block->numPredecessors(); i++) {
        MBasicBlock* pred = block->getPredecessor(i);
        out.printf(" \"B%d\"", pred->id());
    }
    out.printf("\n");

    out.printf("    successors");
    for (uint32_t i = 0; i < block->numSuccessors(); i++) {
        MBasicBlock* successor = block->getSuccessor(i);
        out.printf(" \"B%d\"", successor->id());
    }
    out.printf("\n");

    out.printf("    xhandlers\n");
    out.printf("    flags\n");

    if (block->lir() && block->lir()->begin() != block->lir()->end()) {
        out.printf("    first_lir_id %d\n", block->lir()->firstId());
        out.printf("    last_lir_id %d\n", block->lir()->lastId());
    }

    out.printf("    begin_states\n");

    if (block->entryResumePoint()) {
        out.printf("      begin_locals\n");
        out.printf("        size %d\n", (int)block->numEntrySlots());
        out.printf("        method \"None\"\n");
        for (uint32_t i = 0; i < block->numEntrySlots(); i++) {
            MDefinition* ins = block->getEntrySlot(i);
            out.printf("        ");
            out.printf("%d ", i);
            if (ins->isUnused())
                out.printf("unused");
            else
                ins->printName(out);
            out.printf("\n");
        }
        out.printf("      end_locals\n");
    }
    out.printf("    end_states\n");

    out.printf("    begin_HIR\n");
    for (MPhiIterator phi(block->phisBegin()); phi != block->phisEnd(); phi++)
        DumpDefinition(out, *phi);
    for (MInstructionIterator i(block->begin()); i != block->end(); i++)
        DumpDefinition(out, *i);
    out.printf("    end_HIR\n");

    if (block->lir()) {
        out.printf("    begin_LIR\n");
        for (size_t i = 0; i < block->lir()->numPhis(); i++)
            DumpLIR(out, block->lir()->getPhi(i));
        for (LInstructionIterator i(block->lir()->begin()); i != block->lir()->end(); i++)
            DumpLIR(out, *i);
        out.printf("    end_LIR\n");
    }

    out.printf("  end_block\n");
}

#endif 

