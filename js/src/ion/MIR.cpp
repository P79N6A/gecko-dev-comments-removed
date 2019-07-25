








































#include "IonBuilder.h"
#include "MIR.h"
#include "MIRGraph.h"

using namespace js;
using namespace js::ion;

static void
PrintOpcodeName(FILE *fp, MDefinition::Opcode op)
{
    static const char *names[] =
    {
#define NAME(x) #x,
        MIR_OPCODE_LIST(NAME)
#undef NAME
    };
    const char *name = names[op];
    size_t len = strlen(name);
    for (size_t i = 0; i < len; i++)
        fprintf(fp, "%c", tolower(name[i]));
}

void
MDefinition::printName(FILE *fp)
{
    PrintOpcodeName(fp, op());
    fprintf(fp, "%u", id());

    if (valueNumber() != 0)
        fprintf(fp, "-vn%u", valueNumber());
}

HashNumber
MDefinition::valueHash() const
{
    HashNumber out = op();
    for (size_t i = 0; i < numOperands(); i++) {
        uint32 valueNumber = getOperand(i)->valueNumber();
        out = valueNumber + (out << 6) + (out << 16) - out;
    }
    return out;
}

bool
MDefinition::congruentTo(MDefinition * const &ins) const
{
    if (numOperands() != ins->numOperands())
        return false;

    if (op() != ins->op())
        return false;

    for (size_t i = 0; i < numOperands(); i++) {
        if (getOperand(i)->valueNumber() != ins->getOperand(i)->valueNumber())
            return false;
    }

    return true;
}
static const char *MirTypeNames[] =
{
    "",
    "v",
    "n",
    "b",
    "i",
    "s",
    "d",
    "x",
    "a"
};

void
MDefinition::printOpcode(FILE *fp)
{
    PrintOpcodeName(fp, op());
    fprintf(fp, " ");
    for (size_t j = 0; j < numOperands(); j++) {
        getOperand(j)->printName(fp);
        if (j != numOperands() - 1)
            fprintf(fp, " ");
    }
}

size_t
MDefinition::useCount() const
{
    MUse *use = uses();
    size_t count = 0;
    while (use) {
        count++;
        use = use->next();
    }
    return count;
}

MUse *
MDefinition::removeUse(MUse *prev, MUse *use)
{
    if (!prev) {
        JS_ASSERT(uses_ = use);
        uses_ = use->next();
        return uses_;
    }
    JS_ASSERT(prev->next() == use);
    prev->next_ = use->next();
    return prev->next_;
}

void
MNode::replaceOperand(MUse *prev, MUse *use, MDefinition *ins)
{
    MDefinition *used = getOperand(use->index());
    if (used == ins)
        return;

    used->removeUse(prev, use);
    setOperand(use->index(), ins);
    ins->linkUse(use);
}

void
MNode::replaceOperand(size_t index, MDefinition *def)
{
    MUse *prev = NULL;
    MUse *current = getOperand(index)->uses();
    while (current) {
        if (current->index() == index && current->node() == this) {
            replaceOperand(prev, current, def);
            return;
        }
        prev = current;
        current = current->next();
    }

    JS_NOT_REACHED("could not find use");
}

void
MDefinition::replaceAllUsesWith(MDefinition *dom)
{
    while (uses_) {
        MUse *use = uses_;
        uses_ = uses_->next();
        use->node()->setOperand(use->index(), dom);
        dom->linkUse(use);
    }
}

static inline bool
IsPowerOfTwo(uint32 n)
{
    return (n > 0) && ((n & (n - 1)) == 0);
}

MIRType
MDefinition::usedAsType() const
{
    
    JS_ASSERT(!(usedTypes() & (1 << MIRType_Value)));

    if (IsPowerOfTwo(usedTypes())) {
        
        
        int t;
        JS_FLOOR_LOG2(t, usedTypes());
        return MIRType(t);
    }

    return MIRType_Value;
}

MConstant *
MConstant::New(const Value &v)
{
    return new MConstant(v);
}

MConstant::MConstant(const js::Value &vp)
  : value_(vp)
{
    setResultType(MIRTypeFromValue(vp));
}


HashNumber
MConstant::valueHash() const
{
    
    
    return (HashNumber)value_.asRawBits();
}
bool
MConstant::congruentTo(MDefinition * const &ins) const
{
    if (!ins->isConstant())
        return false;
    return ins->toConstant()->value() == value();
}

void
MConstant::printOpcode(FILE *fp)
{
    PrintOpcodeName(fp, op());
    fprintf(fp, " ");
    switch (type()) {
      case MIRType_Undefined:
        fprintf(fp, "undefined");
        break;
      case MIRType_Null:
        fprintf(fp, "null");
        break;
      case MIRType_Boolean:
        fprintf(fp, value().toBoolean() ? "true" : "false");
        break;
      case MIRType_Int32:
        fprintf(fp, "%x", value().toInt32());
        break;
      case MIRType_Double:
        fprintf(fp, "%f", value().toDouble());
        break;
      case MIRType_Object:
        fprintf(fp, "object %p (%s)", (void *)&value().toObject(),
                value().toObject().getClass()->name);
        break;
      case MIRType_String:
        fprintf(fp, "string %p", (void *)value().toString());
        break;
      default:
        JS_NOT_REACHED("unexpected type");
        break;
    }
}

MParameter *
MParameter::New(int32 index)
{
    return new MParameter(index);
}

void
MParameter::printOpcode(FILE *fp)
{
    PrintOpcodeName(fp, op());
    fprintf(fp, " %d", index());
}

HashNumber
MParameter::valueHash() const
{
    return index_; 
}

bool
MParameter::congruentTo(MDefinition * const &ins) const
{
    if (!ins->isParameter())
        return false;

    return ins->toParameter()->index() == index_;
}

MCopy *
MCopy::New(MDefinition *ins)
{
    
    if (ins->isCopy())
        ins = ins->toCopy()->getOperand(0);

    return new MCopy(ins);
}

HashNumber
MCopy::valueHash() const
{
    return getOperand(0)->valueHash();
}

bool
MCopy::congruentTo(MDefinition * const &ins) const
{
    if (!ins->isCopy())
        return false;

    return ins->toCopy()->getOperand(0) == getOperand(0);
}

MTest *
MTest::New(MDefinition *ins, MBasicBlock *ifTrue, MBasicBlock *ifFalse)
{
    return new MTest(ins, ifTrue, ifFalse);
}

MGoto *
MGoto::New(MBasicBlock *target)
{
    return new MGoto(target);
}

MPhi *
MPhi::New(uint32 slot)
{
    return new MPhi(slot);
}

bool
MPhi::congruentTo(MDefinition *const &ins) const
{
    if (!ins->isPhi())
        return false;
    
    
    
    if (ins->block()->id() != block()->id())
        return false;

    return MDefinition::congruentTo(ins);
}



bool
MPhi::addInput(MDefinition *ins)
{
    for (size_t i = 0; i < inputs_.length(); i++) {
        if (getOperand(i) == ins)
            return true;
    }

    ins->addUse(this, inputs_.length());

    if (!inputs_.append(ins))
        return false;
    return true;
}

MReturn *
MReturn::New(MDefinition *ins)
{
    return new MReturn(ins);
}

void
MBinaryBitwiseInstruction::infer(const TypeOracle::Binary &b)
{
    if (b.lhs == MIRType_Object || b.rhs == MIRType_Object)
        specialization_ = MIRType_None;
    else
        specialization_ = MIRType_Int32;
}

void
MBinaryArithInstruction::infer(const TypeOracle::Binary &b)
{
    if (b.lhs == MIRType_Int32 && b.rhs == MIRType_Int32) {
        specialization_ = MIRType_Int32;
        setResultType(specialization_);
    } else if (b.lhs == MIRType_Double && b.rhs == MIRType_Double) {
        specialization_ = MIRType_Double;
        setResultType(specialization_);
    } else if (b.lhs < MIRType_String && b.rhs < MIRType_String) {
        specialization_ = MIRType_Any;
        if (CoercesToDouble(b.lhs) || CoercesToDouble(b.rhs))
            setResultType(MIRType_Double);
        else
            setResultType(MIRType_Int32);
    } else {
        specialization_ = MIRType_None;
    }
}

MBitAnd *
MBitAnd::New(MDefinition *left, MDefinition *right)
{
    return new MBitAnd(left, right);
}

MBitOr *
MBitOr::New(MDefinition *left, MDefinition *right)
{
    return new MBitOr(left, right);
}

MBitXor *
MBitXor::New(MDefinition *left, MDefinition *right)
{
    return new MBitXor(left, right);
}

MSnapshot *
MSnapshot::New(MBasicBlock *block, jsbytecode *pc)
{
    MSnapshot *snapshot = new MSnapshot(block, pc);
    if (!snapshot->init(block))
        return NULL;
    snapshot->inherit(block);
    return snapshot;
}

MSnapshot::MSnapshot(MBasicBlock *block, jsbytecode *pc)
  : MNode(block),
    stackDepth_(block->stackDepth()),
    pc_(pc)
{
}

bool
MSnapshot::init(MBasicBlock *block)
{
    operands_ = block->gen()->allocate<MDefinition *>(stackDepth());
    if (!operands_)
        return false;
    return true;
}

void
MSnapshot::inherit(MBasicBlock *block)
{
    for (size_t i = 0; i < stackDepth(); i++)
        initOperand(i, block->getSlot(i));
}

