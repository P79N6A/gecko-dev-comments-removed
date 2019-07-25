








































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

static inline bool
EqualValues(bool useGVN, MDefinition *left, MDefinition *right)
{
    if (useGVN)
        return left->valueNumber() == right->valueNumber();

    return left->id() == right->id();
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

MDefinition *
MDefinition::foldsTo(bool useValueNumbers)
{
    
    return this;
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
    size_t count = 0;
    for (MUseIterator i(uses_.begin()); i != uses_.end(); i++)
        count++;
    return count;
}

MUseIterator
MDefinition::removeUse(MUseIterator use)
{
    return uses_.removeAt(use);
}

MUseIterator
MNode::replaceOperand(MUseIterator use, MDefinition *ins)
{
    MDefinition *used = getOperand(use->index());
    if (used == ins)
        return use;

    MUse *save = *use;
    MUseIterator result(used->removeUse(use));
    setOperand(save->index(), ins);
    ins->linkUse(save);
    return result;
}

void
MNode::replaceOperand(size_t index, MDefinition *def)
{
    MDefinition *d = getOperand(index);
    for (MUseIterator i(d->usesBegin()); i != d->usesEnd(); i++) {
        if (i->index() == index && i->node() == this) {
            replaceOperand(i, def);
            return;
        }
    }

    JS_NOT_REACHED("could not find use");
}

void
MDefinition::replaceAllUsesWith(MDefinition *dom)
{
    for (MUseIterator i(uses_.begin()); i != uses_.end(); ) {
        MUse *use = *i;
        i = uses_.removeAt(i);
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
    setIdempotent();
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

MTableSwitch *
MTableSwitch::New(MDefinition *ins, int32 low, int32 high)
{
    return new MTableSwitch(ins, low, high);
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

MDefinition *
MPhi::foldsTo(bool useValueNumbers)
{
    JS_ASSERT(inputs_.length() != 0);

    MDefinition *first = getOperand(0);

    for (size_t i = 1; i < inputs_.length(); i++) {
        if (!EqualValues(useValueNumbers, getOperand(i), first))
            return this;
    }

    return first;
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
    ins->addUse(this, inputs_.length());
    return inputs_.append(ins);
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
    else {
        specialization_ = MIRType_Int32;
        setIdempotent();
        setCommutative();
    }
}

void
MBinaryArithInstruction::infer(const TypeOracle::Binary &b)
{
    if (b.lhs == MIRType_Int32 && b.rhs == MIRType_Int32) {
        specialization_ = MIRType_Int32;
        setIdempotent();
        setCommutative();
        setResultType(specialization_);
    } else if (b.lhs == MIRType_Double && b.rhs == MIRType_Double) {
        specialization_ = MIRType_Double;
        setIdempotent();
        setCommutative();
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

static inline int32
ToInt32(MDefinition *def)
{
    JS_ASSERT(def->isConstant());
    return def->toConstant()->value().toInt32();
}

static inline double
ToDouble(MDefinition *def)
{
    JS_ASSERT(def->isConstant());
    return def->toConstant()->value().toDouble();
}

MDefinition *
MBitAnd::foldsTo(bool useValueNumbers)
{
    if (specialization_ != MIRType_Int32)
        return this;

    MDefinition *lhs = getOperand(0);
    MDefinition *rhs = getOperand(1);

    if (lhs->isConstant() && rhs->isConstant()) {
        js::Value v = Int32Value(ToInt32(lhs) & ToInt32(rhs));
        return MConstant::New(v);
    }

    if (lhs->isConstant() && lhs->toConstant()->value() == Int32Value(0))
        return lhs; 

    if (rhs->isConstant() && rhs->toConstant()->value() == Int32Value(0))
        return rhs; 

    if (EqualValues(useValueNumbers, lhs, rhs))
        return lhs;
    return this;
}


MBitOr *
MBitOr::New(MDefinition *left, MDefinition *right)
{
    return new MBitOr(left, right);
}

MDefinition *
MBitOr::foldsTo(bool useValueNumbers)
{
    if (specialization_ != MIRType_Int32)
        return this;

    MDefinition *lhs = getOperand(0);
    MDefinition *rhs = getOperand(1);

    if (lhs->isConstant() && rhs->isConstant()) {
        js::Value v = Int32Value(ToInt32(lhs) | ToInt32(rhs));
        return MConstant::New(v);
    }

    if (lhs->isConstant() && lhs->toConstant()->value() == Int32Value(0))
        return rhs; 

    if (rhs->isConstant() && rhs->toConstant()->value() == Int32Value(0))
        return lhs; 

    if (EqualValues(useValueNumbers, lhs, rhs))
        return lhs; 

    return this;
}

MBitXor *
MBitXor::New(MDefinition *left, MDefinition *right)
{
    return new MBitXor(left, right);
}

MDefinition *
MBitXor::foldsTo(bool useValueNumbers)
{
    if (specialization_ != MIRType_Int32)
        return this;

    MDefinition *lhs = getOperand(0);
    MDefinition *rhs = getOperand(1);

    if (lhs->isConstant() && rhs->isConstant()) {
        js::Value v = Int32Value(ToInt32(lhs) ^ ToInt32(rhs));
        return MConstant::New(v);
    }

    if (lhs->isConstant() && lhs->toConstant()->value() == Int32Value(0))
        return rhs; 

    if (rhs->isConstant() && rhs->toConstant()->value() == Int32Value(0))
        return lhs; 

    if (EqualValues(useValueNumbers, lhs, rhs))
        return MConstant::New(Int32Value(0)); 

    return this;
}


MDefinition *
MAdd::foldsTo(bool useValueNumbers)
{
    MDefinition *lhs = getOperand(0);
    MDefinition *rhs = getOperand(1);
    js::Value val;
    if (specialization_ == MIRType_Int32) {
        if (lhs->isConstant() && rhs->isConstant()) {
            val = Int32Value(ToInt32(lhs) + ToInt32(rhs));
            return MConstant::New(val);
        }
        val = Int32Value(0);
    } else if (specialization_ == MIRType_Double) {
        if (lhs->isConstant() && rhs->isConstant()) {
            val = DoubleValue (ToDouble(lhs) + ToDouble(rhs));
            return MConstant::New(val);
        }
        val = DoubleValue(0.0);
    } else {
        return this; 
    }

    if (lhs->isConstant() && lhs->toConstant()->value() == val)
        return rhs; 
    if (rhs->isConstant() && rhs->toConstant()->value() == val)
        return lhs; 

    return this;
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

