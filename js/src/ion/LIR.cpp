








































#include "MIR.h"
#include "MIRGraph.h"
#include "LIR.h"
#include "IonLIR-inl.h"
#include "IonSpewer.h"

using namespace js;
using namespace js::ion;

LIRGraph::LIRGraph(MIRGraph &mir)
  : numVirtualRegisters_(0),
    localSlotCount_(0),
    argumentSlotCount_(0),
    entrySnapshot_(NULL),
    osrBlock_(NULL),
    mir_(mir)
{
}

bool
LIRGraph::addConstantToPool(double d, uint32 *index)
{
    *index = constantPool_.length();
    return constantPool_.append(DoubleValue(d));
}

bool
LIRGraph::addConstantToPool(MConstant *ins, uint32 *index)
{
    *index = constantPool_.length();
    return constantPool_.append(ins->value());
}

uint32
LBlock::firstId()
{
    if (phis_.length()) {
        return phis_[0]->id();
    } else {
        for (LInstructionIterator i(instructions_.begin()); i != instructions_.end(); i++) {
            if (i->id())
                return i->id();
        }
    }
    return 0;
}
uint32
LBlock::lastId()
{
    LInstruction *last = *instructions_.rbegin();
    JS_ASSERT(last->id());
    if (last->numDefs())
        return last->getDef(last->numDefs() - 1)->virtualRegister();
    return last->id();
}

LMoveGroup *
LBlock::getEntryMoveGroup()
{
    if (entryMoveGroup_)
        return entryMoveGroup_;
    entryMoveGroup_ = new LMoveGroup;
    insertBefore(*begin(), entryMoveGroup_);
    return entryMoveGroup_;
}

static size_t
TotalOperandCount(MResumePoint *mir)
{
    size_t accum = mir->numOperands();
    while ((mir = mir->caller()))
        accum += mir->numOperands();
    return accum;
}

LSnapshot::LSnapshot(MResumePoint *mir, BailoutKind kind)
  : numSlots_(TotalOperandCount(mir) * BOX_PIECES),
    slots_(NULL),
    mir_(mir),
    snapshotOffset_(INVALID_SNAPSHOT_OFFSET),
    bailoutId_(INVALID_BAILOUT_ID),
    bailoutKind_(kind)
{ }

bool
LSnapshot::init(MIRGenerator *gen)
{
    slots_ = gen->allocate<LAllocation>(numSlots_);
    return !!slots_;
}

LSnapshot *
LSnapshot::New(MIRGenerator *gen, MResumePoint *mir, BailoutKind kind)
{
    LSnapshot *snapshot = new LSnapshot(mir, kind);
    if (!snapshot->init(gen))
        return NULL;

    IonSpew(IonSpew_Snapshots, "Generating LIR snapshot %p from MIR (%p)",
            (void *)snapshot, (void *)mir);

    return snapshot;
}

bool
LPhi::init(MIRGenerator *gen)
{
    inputs_ = gen->allocate<LAllocation>(numInputs_);
    return !!inputs_;
}

LPhi::LPhi(MPhi *mir)
  : numInputs_(mir->numOperands())
{
}

LPhi *
LPhi::New(MIRGenerator *gen, MPhi *ins)
{
    LPhi *phi = new LPhi(ins);
    if (!phi->init(gen))
        return NULL;
    return phi;
}

void
LInstruction::printName(FILE *fp)
{
    static const char *names[] =
    {
#define LIROP(x) #x,
        LIR_OPCODE_LIST(LIROP)
#undef LIROP
    };
    const char *name = names[op()];
    size_t len = strlen(name);
    for (size_t i = 0; i < len; i++)
        fprintf(fp, "%c", tolower(name[i]));
}

static const char *TypeChars[] =
{
    "i",            
    "o",            
    "f",            
#ifdef JS_NUNBOX32
    "t",            
    "d"             
#elif JS_PUNBOX64
    "x"             
#endif
};

static void
PrintDefinition(FILE *fp, const LDefinition &def)
{
    fprintf(fp, "[%s", TypeChars[def.type()]);
    if (def.virtualRegister())
        fprintf(fp, ":%d", def.virtualRegister());
    if (def.policy() == LDefinition::PRESET) {
        fprintf(fp, " (");
        LAllocation::PrintAllocation(fp, def.output());
        fprintf(fp, ")");
    } else if (def.policy() == LDefinition::MUST_REUSE_INPUT) {
        fprintf(fp, " (!)");
    } else if (def.policy() == LDefinition::PASSTHROUGH) {
        fprintf(fp, " (-)");
    }
    fprintf(fp, "]");
}

static void
PrintUse(FILE *fp, const LUse *use)
{
    fprintf(fp, "v%d:", use->virtualRegister());
    if (use->policy() == LUse::ANY) {
        fprintf(fp, "*");
    } else if (use->policy() == LUse::REGISTER) {
        fprintf(fp, "r");
    } else if (use->policy() == LUse::COPY) {
        fprintf(fp, "c");
    } else {
        
        
        
        fprintf(fp, "%s", Registers::GetName(Registers::Code(use->registerCode())));
    }
}

void
LAllocation::PrintAllocation(FILE *fp, const LAllocation *a)
{
    switch (a->kind()) {
      case LAllocation::CONSTANT_VALUE:
      case LAllocation::CONSTANT_INDEX:
        fprintf(fp, "c");
        break;
      case LAllocation::GPR:
        fprintf(fp, "=%s", a->toGeneralReg()->reg().name());
        break;
      case LAllocation::FPU:
        fprintf(fp, "=%s", a->toFloatReg()->reg().name());
        break;
      case LAllocation::STACK_SLOT:
        fprintf(fp, "stack:i%d", a->toStackSlot()->slot());
        break;
      case LAllocation::DOUBLE_SLOT:
        fprintf(fp, "stack:d%d", a->toStackSlot()->slot());
        break;
      case LAllocation::ARGUMENT:
        fprintf(fp, "arg:%d", a->toArgument()->index());
        break;
      case LAllocation::USE:
        PrintUse(fp, a->toUse());
        break;
      default:
        JS_NOT_REACHED("what?");
        break;
    }
}

void
LInstruction::printOperands(FILE *fp)
{
    for (size_t i = 0; i < numOperands(); i++) {
        fprintf(fp, " (");
        LAllocation::PrintAllocation(fp, getOperand(i));
        fprintf(fp, ")");
        if (i != numOperands() - 1)
            fprintf(fp, ",");
    }
}

void
LInstruction::assignSnapshot(LSnapshot *snapshot)
{
    JS_ASSERT(!snapshot_);
    snapshot_ = snapshot;

#ifdef DEBUG
    if (IonSpewEnabled(IonSpew_Snapshots)) {
        IonSpewHeader(IonSpew_Snapshots);
        fprintf(IonSpewFile, "Assigning snapshot %p to instruction %p (",
                (void *)snapshot, (void *)this);
        printName(IonSpewFile);
        fprintf(IonSpewFile, ")\n");
    }
#endif
}

void
LInstruction::assignPostSnapshot(LSnapshot *snapshot)
{
    JS_ASSERT(!postSnapshot_);
    postSnapshot_ = snapshot;

#ifdef DEBUG
    if (IonSpewEnabled(IonSpew_Snapshots)) {
        IonSpewHeader(IonSpew_Snapshots);
        fprintf(IonSpewFile, "Assigning (no-fallback) snapshot %p to instruction %p (",
                (void *)snapshot, (void *)this);
        printName(IonSpewFile);
        fprintf(IonSpewFile, ")\n");
    }
#endif
}

RegisterSet
LInstruction::liveRegisters()
{
    return RegisterSet(GeneralRegisterSet(Registers::AllocatableMask),
                       FloatRegisterSet(FloatRegisters::AllocatableMask));
}

void
LInstruction::print(FILE *fp)
{
    printName(fp);

    fprintf(fp, " (");
    for (size_t i = 0; i < numDefs(); i++) {
        PrintDefinition(fp, *getDef(i));
        if (i != numDefs() - 1)
            fprintf(fp, ", ");
    }
    fprintf(fp, ")");

    printInfo(fp);

    if (numTemps()) {
        fprintf(fp, " t=(");
        for (size_t i = 0; i < numTemps(); i++) {
            PrintDefinition(fp, *getTemp(i));
            if (i != numTemps() - 1)
                fprintf(fp, ", ");
        }
        fprintf(fp, ")");
    }
}

void
LMoveGroup::printOperands(FILE *fp)
{
    for (size_t i = 0; i < numMoves(); i++) {
        const LMove &move = getMove(i);
        fprintf(fp, "[");
        LAllocation::PrintAllocation(fp, move.from());
        fprintf(fp, " -> ");
        LAllocation::PrintAllocation(fp, move.to());
        fprintf(fp, "]");
        if (i != numMoves() - 1)
            fprintf(fp, ", ");
    }
}

Label *
LTestVAndBranch::ifTrue()
{
    return ifTrue_->lir()->label();
}

Label *
LTestVAndBranch::ifFalse()
{
    return ifFalse_->lir()->label();
}

