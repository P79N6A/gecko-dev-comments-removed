








































#include "IonBuilder.h"
#include "MIR.h"
#include "MIRGraph.h"

using namespace js;
using namespace js::ion;

static void
PrintOpcodeName(FILE *fp, MInstruction::Opcode op)
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
MInstruction::printName(FILE *fp)
{
    PrintOpcodeName(fp, op());
    fprintf(fp, "%u", id());

    if (valueNumber() != 0)
        fprintf(fp, "-vn%u", valueNumber());
}

HashNumber
MInstruction::valueHash() const
{
    HashNumber out = op();
    for (size_t i = 0; i < numOperands(); i++) {
        uint32 valueNumber = getInput(i)->valueNumber();
        out = valueNumber + (out << 6) + (out << 16) - out;
    }
    return out;
}

bool
MInstruction::congruentTo(MInstruction * const &ins) const
{
    if (numOperands() != ins->numOperands())
        return false;

    if (op() != ins->op())
        return false;

    for (size_t i = 0; i < numOperands(); i++) {
        if (getInput(i)->valueNumber() != ins->getInput(i)->valueNumber())
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
MInstruction::printOpcode(FILE *fp)
{
    PrintOpcodeName(fp, op());
    fprintf(fp, " ");
    for (size_t j = 0; j < numOperands(); j++) {
        getInput(j)->printName(fp);
        if (j != numOperands() - 1)
            fprintf(fp, " ");
    }
}

size_t
MInstruction::useCount() const
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
MInstruction::removeUse(MUse *prev, MUse *use)
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
MInstruction::replaceOperand(MUse *prev, MUse *use, MInstruction *ins)
{
    MInstruction *used = getInput(use->index());
    if (used == ins)
        return;

    used->removeUse(prev, use);
    setOperand(use->index(), ins);
    ins->addUse(use);
}

void
MInstruction::replaceOperand(MUseIterator &use, MInstruction *ins)
{
    size_t index = use->index();
    use.next();
    replaceOperand(index, ins);
}

void
MInstruction::replaceOperand(size_t index, MInstruction *ins)
{
    MInstruction *old = getInput(index);
    for (MUseIterator uses(old); uses.more(); uses.next()) {
        if (uses->index() == index && uses->ins() == this) {
            replaceOperand(uses.prev(), *uses, ins);
            return;
        }
    }

    JS_NOT_REACHED("could not find use");
}

void
MInstruction::setOperand(size_t index, MInstruction *ins)
{
    getOperand(index)->setInstruction(ins);
}

static inline bool
IsPowerOfTwo(uint32 n)
{
    return (n > 0) && ((n & (n - 1)) == 0);
}

MIRType
MInstruction::usedAsType() const
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
MConstant::congruentTo(MInstruction * const &ins) const
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
MParameter::congruentTo(MInstruction * const &ins) const
{
    if (!ins->isParameter())
        return false;

    return ins->toParameter()->index() == index_;
}

MCopy *
MCopy::New(MInstruction *ins)
{
    
    if (ins->isCopy())
        ins = ins->toCopy()->getInput(0);

    return new MCopy(ins);
}

MTest *
MTest::New(MInstruction *ins, MBasicBlock *ifTrue, MBasicBlock *ifFalse)
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
MPhi::addInput(MInstruction *ins)
{
    for (size_t i = 0; i < inputs_.length(); i++) {
        if (getInput(i) == ins)
            return true;
    }

    ins->addUse(this, inputs_.length());

    MOperand *operand = MOperand::New(ins);
    if (!inputs_.append(operand))
        return false;
    return true;
}

MReturn *
MReturn::New(MInstruction *ins)
{
    return new MReturn(ins);
}

void
MBinaryInstruction::infer(const TypeOracle::Binary &b)
{
    if (b.lhs == MIRType_Int32 && b.rhs == MIRType_Int32) {
        specialization_ = MIRType_Int32;
        setResultType(specialization_);
    } else if (b.lhs == MIRType_Double && b.rhs == MIRType_Double) {
        specialization_ = MIRType_Double;
        setResultType(specialization_);
    } else if (b.lhs < MIRType_String && b.rhs < MIRType_String) {
        specialization_ = MIRType_Any;
        setResultType(b.rval);
    } else {
        specialization_ = MIRType_Value;
    }
}

static inline bool
HasComplexNumberConversion(MIRType type)
{
    return type >= MIRType_String && type < MIRType_Value;
}

bool
MBinaryInstruction::adjustForInputs()
{
    if (specialization() == MIRType_Value)
        return false;

    MIRType lhsActual = getInput(0)->type();
    MIRType lhsUsedAs = getInput(0)->usedAsType();
    MIRType rhsActual = getInput(1)->type();
    MIRType rhsUsedAs = getInput(1)->usedAsType();

    if (HasComplexNumberConversion(lhsActual) ||
        HasComplexNumberConversion(lhsUsedAs) ||
        HasComplexNumberConversion(rhsActual) ||
        HasComplexNumberConversion(rhsUsedAs)) {
        specialization_ = MIRType_Value;
        return true;
    }
    return false;
}

MBitAnd *
MBitAnd::New(MInstruction *left, MInstruction *right)
{
    return new MBitAnd(left, right);
}

MBitOr *
MBitOr::New(MInstruction *left, MInstruction *right)
{
    return new MBitOr(left, right);
}

MBitXOr *
MBitXOr::New(MInstruction *left, MInstruction *right)
{
    return new MBitXOr(left, right);
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
  : stackDepth_(block->stackDepth()),
    pc_(pc)
{
}

bool
MSnapshot::init(MBasicBlock *block)
{
    operands_ = block->gen()->allocate<MOperand *>(stackDepth());
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

void
MSnapshot::printOpcode(FILE *fp)
{
    PrintOpcodeName(fp, op());
}

HashNumber
MSnapshot::valueHash() const
{
    return id();
}

bool
MSnapshot::congruentTo(MInstruction *const &ins) const
{
    return ins->id() == id();
}
