








































#include <stdarg.h>

#include "JSONSpewer.h"
#include "IonLIR.h"

using namespace js;
using namespace js::ion;

JSONSpewer::~JSONSpewer()
{
    if (fp_)
        fclose(fp_);
}

void
JSONSpewer::property(const char *name)
{
    if (!fp_)
        return;

    if (!first_)
        fprintf(fp_, ",");
    fprintf(fp_, "\"%s\":", name);
    first_ = false;
}

void
JSONSpewer::beginObject()
{
    if (!fp_)
        return;

    if (!first_)
        fprintf(fp_, ",");
    fprintf(fp_, "{");
    first_ = true;
}

void
JSONSpewer::beginObjectProperty(const char *name)
{
    if (!fp_)
        return;

    property(name);
    fprintf(fp_, "{");
    first_ = true;
}

void
JSONSpewer::beginListProperty(const char *name)
{
    if (!fp_)
        return;

    property(name);
    fprintf(fp_, "[");
    first_ = true;
}

void
JSONSpewer::stringProperty(const char *name, const char *format, ...)
{
    if (!fp_)
        return;

    va_list ap;
    va_start(ap, format);

    property(name);
    fprintf(fp_, "\"");
    vfprintf(fp_, format, ap);
    fprintf(fp_, "\"");

    va_end(ap);
}

void
JSONSpewer::stringValue(const char *format, ...)
{
    if (!fp_)
        return;

    va_list ap;
    va_start(ap, format);

    if (!first_)
        fprintf(fp_, ",");
    fprintf(fp_, "\"");
    vfprintf(fp_, format, ap);
    fprintf(fp_, "\"");

    va_end(ap);
    first_ = false;
}

void
JSONSpewer::integerProperty(const char *name, int value)
{
    if (!fp_)
        return;

    property(name);
    fprintf(fp_, "%d", value);
}

void
JSONSpewer::integerValue(int value)
{
    if (!fp_)
        return;

    if (!first_)
        fprintf(fp_, ",");
    fprintf(fp_, "%d", value);
    first_ = false;
}

void
JSONSpewer::endObject()
{
    if (!fp_)
        return;

    fprintf(fp_, "}");
    first_ = false;
}

void
JSONSpewer::endList()
{
    if (!fp_)
        return;

    fprintf(fp_, "]");
    first_ = false;
}

bool
JSONSpewer::init(const char *path)
{
    fp_ = fopen(path, "w");
    if (!fp_)
        return false;

    beginObject();
    beginListProperty("functions");
    return true;
}

void
JSONSpewer::beginFunction(JSScript *script)
{
    beginObject();
    stringProperty("name", "%s:%d", script->filename, script->lineno);
    beginListProperty("passes");
}

void
JSONSpewer::beginPass(const char *pass)
{
    beginObject();
    stringProperty("name", pass);
}

void
JSONSpewer::spewMDef(MDefinition *def)
{
    beginObject();

    integerProperty("id", def->id());

    property("opcode");
    fprintf(fp_, "\"");
    def->printOpcode(fp_);
    fprintf(fp_, "\"");

    beginListProperty("inputs");
    for (size_t i = 0; i < def->numOperands(); i++)
        integerValue(def->getOperand(i)->id());
    endList();

    beginListProperty("uses");
    for (MUseDefIterator use(def); use; use++)
        integerValue(use.def()->id());
    endList();

    endObject();
}

void
JSONSpewer::spewMIR(MIRGraph *mir)
{
    if (!fp_)
        return;

    beginObjectProperty("mir");
    beginListProperty("blocks");

    for (size_t bno = 0; bno < mir->numBlocks(); bno++) {
        MBasicBlock *block = mir->getBlock(bno);
        beginObject();

        integerProperty("number", bno);

        beginListProperty("attributes");
        if (block->isLoopBackedge())
            stringValue("backedge");
        if (block->isLoopHeader())
            stringValue("loopheader");
        endList();

        beginListProperty("predecessors");
        for (size_t i = 0; i < block->numPredecessors(); i++)
            integerValue(block->getPredecessor(i)->id());
        endList();

        beginListProperty("successors");
        for (size_t i = 0; i < block->numSuccessors(); i++)
            integerValue(block->getSuccessor(i)->id());
        endList();

        beginListProperty("instructions");
        for (size_t i = 0; i < block->numPhis(); i++)
            spewMDef(block->getPhi(i));
        for (MInstructionIterator i = block->begin(); i != block->end(); i++)
            spewMDef(*i);
        endList();

        endObject();
    }

    endList();
    endObject();
}

void
JSONSpewer::spewLIR(MIRGraph *mir)
{
    if (!fp_)
        return;

    beginObjectProperty("lir");
    beginListProperty("blocks");

    for (size_t bno = 0; bno < mir->numBlocks(); bno++) {
        LBlock *block = mir->getBlock(bno)->lir();
        if (!block)
            continue;

        beginObject();
        integerProperty("number", bno);

        beginListProperty("instructions");
        for (LInstructionIterator ins(block->begin());
             ins != block->end();
             ins++)
        {
            beginObject();

            integerProperty("id", ins->id());

            property("opcode");
            fprintf(fp_, "\"");
            ins->print(fp_);
            fprintf(fp_, "\"");

            beginListProperty("defs");
            for (size_t i = 0; i < ins->numDefs(); i++)
                integerValue(ins->getDef(i)->virtualRegister());
            endList();

            endObject();
        }
        endList();

        endObject();
    }

    endList();
    endObject();
}

void
JSONSpewer::spewIntervals(LinearScanAllocator *regalloc)
{
    if (!fp_)
        return;

    beginObjectProperty("intervals");
    beginListProperty("blocks");

    for (size_t bno = 0; bno < regalloc->graph.numBlocks(); bno++) {
        beginObject();
        integerProperty("number", bno);
        beginListProperty("vregs");

        LBlock *lir = regalloc->graph.getBlock(bno);
        for (LInstructionIterator ins = lir->begin(); ins != lir->end(); ins++) {
            for (size_t k = 0; k < ins->numDefs(); k++) {
                VirtualRegister *vreg = &regalloc->vregs[ins->getDef(k)->virtualRegister()];

                beginObject();
                integerProperty("vreg", vreg->reg());
                beginListProperty("intervals");

                for (size_t i = 0; i < vreg->numIntervals(); i++) {
                    LiveInterval *live = vreg->getInterval(i);

                    if (live->numRanges()) {
                        beginObject();
                        property("allocation");
                        fprintf(fp_, "\"");
                        LAllocation::PrintAllocation(fp_, live->getAllocation());
                        fprintf(fp_, "\"");
                        beginListProperty("ranges");

                        for (size_t j = 0; j < live->numRanges(); j++) {
                            beginObject();
                            integerProperty("start", live->getRange(j)->from.pos());
                            integerProperty("end", live->getRange(j)->to.pos());
                            endObject();
                        }

                        endList();
                        endObject();
                    }
                }

                endList();
                endObject();
            }
        }

        endList();
        endObject();
    }

    endList();
    endObject();
}

void
JSONSpewer::endPass()
{
    endObject();
}

void
JSONSpewer::endFunction()
{
    endList();
    endObject();
}

void
JSONSpewer::finish()
{
    if (!fp_)
        return;

    endList();
    endObject();
    fprintf(fp_, "\n");
}

