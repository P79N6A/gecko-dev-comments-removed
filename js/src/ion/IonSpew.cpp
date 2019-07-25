








































#include <stdarg.h>
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
    for (uint32 i = 0; i < block->numSuccessors(); i++) {
        MBasicBlock *successor = block->getSuccessor(i);
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
        for (size_t i = 0; i < block->lir()->numPhis(); i++)
            DumpLIR(fp, block->lir()->getPhi(i));
        for (LInstructionIterator i = block->lir()->begin(); i != block->lir()->end(); i++)
            DumpLIR(fp, *i);
        fprintf(fp, "    end_LIR\n");
    }

    fprintf(fp, "  end_block\n");
}

static bool LoggingChecked = false;
static uint32 LoggingBits = 0;

static const char *ChannelNames[] =
{
#define IONSPEW_CHANNEL(name) #name,
    IONSPEW_CHANNEL_LIST(IONSPEW_CHANNEL)
#undef IONSPEW_CHANNEL
};

void
ion::CheckLogging()
{
    if (LoggingChecked)
        return;
    LoggingChecked = true;
    const char *env = getenv("IONFLAGS");
    if (!env)
        return;
    if (strstr(env, "help")) {
        fflush(NULL);
        printf(
            "\n"
            "usage: IONFLAGS=option,option,option,... where options can be:\n"
            "\n"
            "  aborts   Compilation abort messages\n"
            "  mir      MIR information\n"

            "  all      Everything\n"
            "\n"
        );
        exit(0);
        
    }
    if (strstr(env, "aborts"))
        LoggingBits |= (1 << uint32(IonSpew_Abort));
    if (strstr(env, "mir"))
        LoggingBits |= (1 << uint32(IonSpew_MIR));
    if (strstr(env, "all"))
        LoggingBits = uint32(-1);
}

void
ion::IonSpewVA(IonSpewChannel channel, const char *fmt, va_list ap)
{
    JS_ASSERT(LoggingChecked);

    if (!(LoggingBits & (1 << uint32(channel))))
        return;

    fprintf(stderr, "[%s] ", ChannelNames[channel]);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
}

void
ion::IonSpew(IonSpewChannel channel, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    IonSpewVA(channel, fmt, ap);
    va_end(ap);
}

