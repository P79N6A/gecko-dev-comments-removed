





#include "jit/LIR.h"

#include <ctype.h>

#include "jsprf.h"

#include "jit/JitSpewer.h"
#include "jit/MIR.h"
#include "jit/MIRGenerator.h"

using namespace js;
using namespace js::jit;

LIRGraph::LIRGraph(MIRGraph *mir)
  : blocks_(),
    constantPool_(mir->alloc()),
    constantPoolMap_(mir->alloc()),
    safepoints_(mir->alloc()),
    nonCallSafepoints_(mir->alloc()),
    numVirtualRegisters_(0),
    numInstructions_(1), 
    localSlotCount_(0),
    argumentSlotCount_(0),
    entrySnapshot_(nullptr),
    mir_(*mir)
{
}

bool
LIRGraph::addConstantToPool(const Value &v, uint32_t *index)
{
    JS_ASSERT(constantPoolMap_.initialized());

    ConstantPoolMap::AddPtr p = constantPoolMap_.lookupForAdd(v);
    if (p) {
        *index = p->value();
        return true;
    }
    *index = constantPool_.length();
    return constantPool_.append(v) && constantPoolMap_.add(p, v, *index);
}

bool
LIRGraph::noteNeedsSafepoint(LInstruction *ins)
{
    
    JS_ASSERT_IF(!safepoints_.empty(), safepoints_.back()->id() < ins->id());
    if (!ins->isCall() && !nonCallSafepoints_.append(ins))
        return false;
    return safepoints_.append(ins);
}

void
LIRGraph::dump(FILE *fp) const
{
    for (size_t i = 0; i < numBlocks(); i++) {
        getBlock(i)->dump(fp);
        fprintf(fp, "\n");
    }
}

void
LIRGraph::dump() const
{
    dump(stderr);
}

LBlock *
LBlock::New(TempAllocator &alloc, MBasicBlock *from)
{
    LBlock *block = new(alloc) LBlock(from);
    if (!block)
        return nullptr;

    
    size_t numLPhis = 0;
    for (MPhiIterator i(from->phisBegin()), e(from->phisEnd()); i != e; ++i) {
        MPhi *phi = *i;
        numLPhis += (phi->type() == MIRType_Value) ? BOX_PIECES : 1;
    }

    
    if (!block->phis_.init(alloc, numLPhis))
        return nullptr;

    
    
    
    size_t phiIndex = 0;
    size_t numPreds = from->numPredecessors();
    for (MPhiIterator i(from->phisBegin()), e(from->phisEnd()); i != e; ++i) {
        MPhi *phi = *i;
        MOZ_ASSERT(phi->numOperands() == numPreds);

        int numPhis = (phi->type() == MIRType_Value) ? BOX_PIECES : 1;
        for (int i = 0; i < numPhis; i++) {
            void *array = alloc.allocateArray<sizeof(LAllocation)>(numPreds);
            LAllocation *inputs = static_cast<LAllocation *>(array);
            if (!inputs)
                return nullptr;

            new (&block->phis_[phiIndex++]) LPhi(phi, inputs);
        }
    }
    return block;
}

uint32_t
LBlock::firstId() const
{
    if (phis_.length()) {
        return phis_[0].id();
    } else {
        for (LInstructionIterator i(instructions_.begin()); i != instructions_.end(); i++) {
            if (i->id())
                return i->id();
        }
    }
    return 0;
}
uint32_t
LBlock::lastId() const
{
    LInstruction *last = *instructions_.rbegin();
    JS_ASSERT(last->id());
    
    
    JS_ASSERT(last->numDefs() == 0);
    return last->id();
}

LMoveGroup *
LBlock::getEntryMoveGroup(TempAllocator &alloc)
{
    if (entryMoveGroup_)
        return entryMoveGroup_;
    entryMoveGroup_ = LMoveGroup::New(alloc);
    if (begin()->isLabel())
        insertAfter(*begin(), entryMoveGroup_);
    else
        insertBefore(*begin(), entryMoveGroup_);
    return entryMoveGroup_;
}

LMoveGroup *
LBlock::getExitMoveGroup(TempAllocator &alloc)
{
    if (exitMoveGroup_)
        return exitMoveGroup_;
    exitMoveGroup_ = LMoveGroup::New(alloc);
    insertBefore(*rbegin(), exitMoveGroup_);
    return exitMoveGroup_;
}

void
LBlock::dump(FILE *fp)
{
    fprintf(fp, "block%u:\n", mir()->id());
    for (size_t i = 0; i < numPhis(); ++i) {
        getPhi(i)->dump(fp);
        fprintf(fp, "\n");
    }
    for (LInstructionIterator iter = begin(); iter != end(); iter++) {
        iter->dump(fp);
        fprintf(fp, "\n");
    }
}

void
LBlock::dump()
{
    dump(stderr);
}

static size_t
TotalOperandCount(LRecoverInfo *recoverInfo)
{
    size_t accum = 0;
    for (LRecoverInfo::OperandIter it(recoverInfo); !it; ++it) {
        if (!it->isRecoveredOnBailout())
            accum++;
    }
    return accum;
}

LRecoverInfo::LRecoverInfo(TempAllocator &alloc)
  : instructions_(alloc),
    recoverOffset_(INVALID_RECOVER_OFFSET)
{ }

LRecoverInfo *
LRecoverInfo::New(MIRGenerator *gen, MResumePoint *mir)
{
    LRecoverInfo *recoverInfo = new(gen->alloc()) LRecoverInfo(gen->alloc());
    if (!recoverInfo || !recoverInfo->init(mir))
        return nullptr;

    JitSpew(JitSpew_IonSnapshots, "Generating LIR recover info %p from MIR (%p)",
            (void *)recoverInfo, (void *)mir);

    return recoverInfo;
}

bool
LRecoverInfo::appendOperands(MNode *ins)
{
    for (size_t i = 0, end = ins->numOperands(); i < end; i++) {
        MDefinition *def = ins->getOperand(i);

        
        
        
        
        if (def->isRecoveredOnBailout() && !def->isInWorklist()) {
            if (!appendDefinition(def))
                return false;
        }
    }

    return true;
}

bool
LRecoverInfo::appendDefinition(MDefinition *def)
{
    MOZ_ASSERT(def->isRecoveredOnBailout());
    def->setInWorklist();
    if (!appendOperands(def))
        return false;
    return instructions_.append(def);
}

bool
LRecoverInfo::appendResumePoint(MResumePoint *rp)
{
    if (rp->caller() && !appendResumePoint(rp->caller()))
        return false;

    if (!appendOperands(rp))
        return false;

    return instructions_.append(rp);
}

bool
LRecoverInfo::init(MResumePoint *rp)
{
    
    
    
    
    if (!appendResumePoint(rp))
        return false;

    
    for (MNode **it = begin(); it != end(); it++) {
        if (!(*it)->isDefinition())
            continue;

        (*it)->toDefinition()->setNotInWorklist();
    }

    MOZ_ASSERT(mir() == rp);
    return true;
}

LSnapshot::LSnapshot(LRecoverInfo *recoverInfo, BailoutKind kind)
  : numSlots_(TotalOperandCount(recoverInfo) * BOX_PIECES),
    slots_(nullptr),
    recoverInfo_(recoverInfo),
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
LSnapshot::New(MIRGenerator *gen, LRecoverInfo *recover, BailoutKind kind)
{
    LSnapshot *snapshot = new(gen->alloc()) LSnapshot(recover, kind);
    if (!snapshot || !snapshot->init(gen))
        return nullptr;

    JitSpew(JitSpew_IonSnapshots, "Generating LIR snapshot %p from recover (%p)",
            (void *)snapshot, (void *)recover);

    return snapshot;
}

void
LSnapshot::rewriteRecoveredInput(LUse input)
{
    
    
    for (size_t i = 0; i < numEntries(); i++) {
        if (getEntry(i)->isUse() && getEntry(i)->toUse()->virtualRegister() == input.virtualRegister())
            setEntry(i, LUse(input.virtualRegister(), LUse::RECOVERED_INPUT));
    }
}

void
LInstruction::printName(FILE *fp, Opcode op)
{
    static const char * const names[] =
    {
#define LIROP(x) #x,
        LIR_OPCODE_LIST(LIROP)
#undef LIROP
    };
    const char *name = names[op];
    size_t len = strlen(name);
    for (size_t i = 0; i < len; i++)
        fprintf(fp, "%c", tolower(name[i]));
}

void
LInstruction::printName(FILE *fp)
{
    printName(fp, op());
}

bool
LAllocation::aliases(const LAllocation &other) const
{
    if (isFloatReg() && other.isFloatReg())
        return toFloatReg()->reg().aliases(other.toFloatReg()->reg());
    return *this == other;
}

#ifdef DEBUG
static const char * const TypeChars[] =
{
    "g",            
    "i",            
    "o",            
    "s",            
    "f",            
    "d",            
#ifdef JS_NUNBOX32
    "t",            
    "p"             
#elif JS_PUNBOX64
    "x"             
#endif
};

static void
PrintDefinition(char *buf, size_t size, const LDefinition &def)
{
    char *cursor = buf;
    char *end = buf + size;

    cursor += JS_snprintf(cursor, end - cursor, "v%u", def.virtualRegister());
    cursor += JS_snprintf(cursor, end - cursor, "<%s>", TypeChars[def.type()]);

    if (def.policy() == LDefinition::FIXED)
        cursor += JS_snprintf(cursor, end - cursor, ":%s", def.output()->toString());
    else if (def.policy() == LDefinition::MUST_REUSE_INPUT)
        cursor += JS_snprintf(cursor, end - cursor, ":tied(%u)", def.getReusedInput());
}

const char *
LDefinition::toString() const
{
    
    static char buf[40];

    if (isBogusTemp())
        return "bogus";

    PrintDefinition(buf, sizeof(buf), *this);
    return buf;
}

static void
PrintUse(char *buf, size_t size, const LUse *use)
{
    switch (use->policy()) {
      case LUse::REGISTER:
        JS_snprintf(buf, size, "v%d:r", use->virtualRegister());
        break;
      case LUse::FIXED:
        
        
        
        JS_snprintf(buf, size, "v%d:%s", use->virtualRegister(),
                    Registers::GetName(Registers::Code(use->registerCode())));
        break;
      case LUse::ANY:
        JS_snprintf(buf, size, "v%d:r?", use->virtualRegister());
        break;
      case LUse::KEEPALIVE:
        JS_snprintf(buf, size, "v%d:*", use->virtualRegister());
        break;
      case LUse::RECOVERED_INPUT:
        JS_snprintf(buf, size, "v%d:**", use->virtualRegister());
        break;
      default:
        MOZ_CRASH("invalid use policy");
    }
}

const char *
LAllocation::toString() const
{
    
    static char buf[40];

    if (isBogus())
        return "bogus";

    switch (kind()) {
      case LAllocation::CONSTANT_VALUE:
      case LAllocation::CONSTANT_INDEX:
        return "c";
      case LAllocation::GPR:
        JS_snprintf(buf, sizeof(buf), "%s", toGeneralReg()->reg().name());
        return buf;
      case LAllocation::FPU:
        JS_snprintf(buf, sizeof(buf), "%s", toFloatReg()->reg().name());
        return buf;
      case LAllocation::STACK_SLOT:
        JS_snprintf(buf, sizeof(buf), "stack:%d", toStackSlot()->slot());
        return buf;
      case LAllocation::ARGUMENT_SLOT:
        JS_snprintf(buf, sizeof(buf), "arg:%d", toArgument()->index());
        return buf;
      case LAllocation::USE:
        PrintUse(buf, sizeof(buf), toUse());
        return buf;
      default:
        MOZ_CRASH("what?");
    }
}
#endif 

void
LAllocation::dump() const
{
    fprintf(stderr, "%s\n", toString());
}

void
LDefinition::dump() const
{
    fprintf(stderr, "%s\n", toString());
}

void
LInstruction::printOperands(FILE *fp)
{
    for (size_t i = 0, e = numOperands(); i < e; i++) {
        fprintf(fp, " (%s)", getOperand(i)->toString());
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
    if (JitSpewEnabled(JitSpew_IonSnapshots)) {
        JitSpewHeader(JitSpew_IonSnapshots);
        fprintf(JitSpewFile, "Assigning snapshot %p to instruction %p (",
                (void *)snapshot, (void *)this);
        printName(JitSpewFile);
        fprintf(JitSpewFile, ")\n");
    }
#endif
}

void
LInstruction::dump(FILE *fp)
{
    if (numDefs() != 0) {
        fprintf(fp, "{");
        for (size_t i = 0; i < numDefs(); i++) {
            fprintf(fp, "%s", getDef(i)->toString());
            if (i != numDefs() - 1)
                fprintf(fp, ", ");
        }
        fprintf(fp, "} <- ");
    }

    printName(fp);
    printInfo(fp);

    if (numTemps()) {
        fprintf(fp, " t=(");
        for (size_t i = 0; i < numTemps(); i++) {
            fprintf(fp, "%s", getTemp(i)->toString());
            if (i != numTemps() - 1)
                fprintf(fp, ", ");
        }
        fprintf(fp, ")");
    }

    if (numSuccessors()) {
        fprintf(fp, " s=(");
        for (size_t i = 0; i < numSuccessors(); i++) {
            fprintf(fp, "block%u", getSuccessor(i)->id());
            if (i != numSuccessors() - 1)
                fprintf(fp, ", ");
        }
        fprintf(fp, ")");
    }
}

void
LInstruction::dump()
{
    dump(stderr);
    fprintf(stderr, "\n");
}

void
LInstruction::initSafepoint(TempAllocator &alloc)
{
    JS_ASSERT(!safepoint_);
    safepoint_ = new(alloc) LSafepoint(alloc);
    JS_ASSERT(safepoint_);
}

bool
LMoveGroup::add(LAllocation *from, LAllocation *to, LDefinition::Type type)
{
#ifdef DEBUG
    JS_ASSERT(*from != *to);
    for (size_t i = 0; i < moves_.length(); i++)
        JS_ASSERT(*to != *moves_[i].to());
#endif
    return moves_.append(LMove(from, to, type));
}

bool
LMoveGroup::addAfter(LAllocation *from, LAllocation *to, LDefinition::Type type)
{
    
    
    

    for (size_t i = 0; i < moves_.length(); i++) {
        if (*moves_[i].to() == *from) {
            from = moves_[i].from();
            break;
        }
    }

    if (*from == *to)
        return true;

    for (size_t i = 0; i < moves_.length(); i++) {
        if (*to == *moves_[i].to()) {
            moves_[i] = LMove(from, to, type);
            return true;
        }
    }

    return add(from, to, type);
}

void
LMoveGroup::printOperands(FILE *fp)
{
    for (size_t i = 0; i < numMoves(); i++) {
        const LMove &move = getMove(i);
        
        fprintf(fp, " [%s", move.from()->toString());
        fprintf(fp, " -> %s]", move.to()->toString());
        if (i != numMoves() - 1)
            fprintf(fp, ",");
    }
}
