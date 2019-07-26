








































#include "IonBuilder.h"
#include "LICM.h" 
#include "MIR.h"
#include "MIRGraph.h"
#include "EdgeCaseAnalysis.h"
#include "jsnum.h"
#include "jstypedarrayinlines.h" 

using namespace js;
using namespace js::ion;

void
MDefinition::PrintOpcodeName(FILE *fp, MDefinition::Opcode op)
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

static MConstant *
EvaluateConstantOperands(MBinaryInstruction *ins)
{
    MDefinition *left = ins->getOperand(0);
    MDefinition *right = ins->getOperand(1);

    if (!left->isConstant() || !right->isConstant())
        return NULL;

    Value lhs = left->toConstant()->value();
    Value rhs = right->toConstant()->value();
    Value ret = UndefinedValue();

    switch (ins->op()) {
      case MDefinition::Op_BitAnd:
        ret = Int32Value(lhs.toInt32() & rhs.toInt32());
        break;
      case MDefinition::Op_BitOr:
        ret = Int32Value(lhs.toInt32() | rhs.toInt32());
        break;
      case MDefinition::Op_BitXor:
        ret = Int32Value(lhs.toInt32() ^ rhs.toInt32());
        break;
      case MDefinition::Op_Lsh:
        ret = Int32Value(lhs.toInt32() << (rhs.toInt32() & 0x1F));
        break;
      case MDefinition::Op_Rsh:
        ret = Int32Value(lhs.toInt32() >> (rhs.toInt32() & 0x1F));
        break;
      case MDefinition::Op_Ursh: {
        uint32 unsignedLhs = (uint32_t)lhs.toInt32();
        ret.setNumber(uint32(unsignedLhs >> (rhs.toInt32() & 0x1F)));
        break;
      }
      case MDefinition::Op_Add:
        ret.setNumber(lhs.toNumber() + rhs.toNumber());
        break;
      case MDefinition::Op_Sub:
        ret.setNumber(lhs.toNumber() - rhs.toNumber());
        break;
      case MDefinition::Op_Mul:
        ret.setNumber(lhs.toNumber() * rhs.toNumber());
        break;
      case MDefinition::Op_Div:
        ret.setNumber(NumberDiv(lhs.toNumber(), rhs.toNumber()));
        break;
      case MDefinition::Op_Mod:
        ret.setNumber(NumberMod(lhs.toNumber(), rhs.toNumber()));
        break;
      default:
        JS_NOT_REACHED("NYI");
        return NULL;
    }

    if (ins->type() != MIRTypeFromValue(ret))
        return NULL;

    return MConstant::New(ret);
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
MDefinition::congruentIfOperandsEqual(MDefinition * const &ins) const
{
    if (numOperands() != ins->numOperands())
        return false;

    if (op() != ins->op())
        return false;

    if (type() != ins->type())
        return false;

    if (isEffectful() || ins->isEffectful())
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

void
MDefinition::analyzeEdgeCasesForward()
{
    return;
}

void
MDefinition::analyzeEdgeCasesBackward()
{
    return;
}
void
MDefinition::analyzeTruncateBackward()
{
    return;
}

MDefinition *
MTest::foldsTo(bool useValueNumbers)
{
    MDefinition *op = getOperand(0);

    if (op->isNot())
        return MTest::New(op->toNot()->operand(), ifFalse(), ifTrue());

    return this;
}

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
    if (ins) {
        setOperand(save->index(), ins);
        ins->linkUse(save);
    }
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

MConstant *
MConstant::New(const Value &v)
{
    return new MConstant(v);
}

MConstant::MConstant(const js::Value &vp)
  : value_(vp)
{
    setResultType(MIRTypeFromValue(vp));
    setMovable();

    if (type() == MIRType_Int32) {
        range()->setLower(value().toInt32());
        range()->setUpper(value().toInt32());
    }
}

HashNumber
MConstant::valueHash() const
{
    
    
    return (HashNumber)JSVAL_TO_IMPL(value_).asBits;
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
        fprintf(fp, "0x%x", value().toInt32());
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
      case MIRType_Magic:
        fprintf(fp, "magic");
        break;
      default:
        JS_NOT_REACHED("unexpected type");
        break;
    }
}

void
MConstantElements::printOpcode(FILE *fp)
{
    PrintOpcodeName(fp, op());
    fprintf(fp, " %p", value());
}

MParameter *
MParameter::New(int32 index, types::TypeSet *types)
{
    return new MParameter(index, types);
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

MCall *
MCall::New(JSFunction *target, size_t maxArgc, size_t numActualArgs, bool construct)
{
    JS_ASSERT(maxArgc >= numActualArgs);
    MCall *ins = new MCall(target, numActualArgs, construct);
    if (!ins->init(maxArgc + NumNonArgumentOperands))
        return NULL;
    return ins;
}

MApplyArgs *
MApplyArgs::New(JSFunction *target, MDefinition *fun, MDefinition *argc, MDefinition *self)
{
    return new MApplyArgs(target, fun, argc, self);
}

MDefinition*
MStringLength::foldsTo(bool useValueNumbers)
{
    if ((type() == MIRType_Int32) && (string()->isConstant())) {
        Value value = string()->toConstant()->value();
        size_t length = JS_GetStringLength(value.toString());

        return MConstant::New(Int32Value(length));
    }

    return this;
}

MTest *
MTest::New(MDefinition *ins, MBasicBlock *ifTrue, MBasicBlock *ifFalse)
{
    return new MTest(ins, ifTrue, ifFalse);
}

MInlineFunctionGuard *
MInlineFunctionGuard::New(MDefinition *input, JSFunction *fun,
                          MBasicBlock *functionBlock,
                          MBasicBlock *fallbackBlock)
{
    return new MInlineFunctionGuard(input, fun, functionBlock, fallbackBlock);
}

MCompare *
MCompare::New(MDefinition *left, MDefinition *right, JSOp op)
{
    return new MCompare(left, right, op);
}

MTableSwitch *
MTableSwitch::New(MDefinition *ins, int32 low, int32 high)
{
    return new MTableSwitch(ins, low, high);
}

MGoto *
MGoto::New(MBasicBlock *target)
{
    JS_ASSERT(target);
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
        
        
        if (!EqualValues(false, getOperand(i), first))
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

    return congruentIfOperandsEqual(ins);
}

bool
MPhi::addInput(MDefinition *ins)
{
    ins->addUse(this, inputs_.length());
    return inputs_.append(ins);
}

uint32
MPrepareCall::argc() const
{
    JS_ASSERT(useCount() == 1);
    MCall *call = usesBegin()->node()->toDefinition()->toCall();
    return call->numStackArgs();
}

void
MPassArg::printOpcode(FILE *fp)
{
    PrintOpcodeName(fp, op());
    fprintf(fp, " %d ", argnum_);
    for (size_t j = 0; j < numOperands(); j++) {
        getOperand(j)->printName(fp);
        if (j != numOperands() - 1)
            fprintf(fp, " ");
    }
}

void
MCall::addArg(size_t argnum, MPassArg *arg)
{
    
    
    arg->setArgnum(argnum);
    MNode::initOperand(argnum + NumNonArgumentOperands, arg->toDefinition());
}

void
MBitNot::infer(const TypeOracle::Unary &u)
{
    JS_ASSERT(u.ival != MIRType_Any);

    if (u.ival == MIRType_Object)
        specialization_ = MIRType_None;
    else
        specialization_ = MIRType_Int32;
}

static inline bool
IsConstant(MDefinition *def, double v)
{
    return def->isConstant() && def->toConstant()->value().toNumber() == v;
}

MDefinition *
MBinaryBitwiseInstruction::foldsTo(bool useValueNumbers)
{
    if (specialization_ != MIRType_Int32)
        return this;

    MDefinition *lhs = getOperand(0);
    MDefinition *rhs = getOperand(1);

    if (MDefinition *folded = EvaluateConstantOperands(this))
        return folded;

    if (IsConstant(lhs, 0))
        return foldIfZero(0);

    if (IsConstant(rhs, 0))
        return foldIfZero(1);

    if (EqualValues(useValueNumbers, lhs, rhs))
        return foldIfEqual();

    return this;
}

void
MBinaryBitwiseInstruction::infer(const TypeOracle::Binary &b)
{
    JS_ASSERT(b.lhs != MIRType_Any);
    JS_ASSERT(b.rhs != MIRType_Any);

    if (b.lhs == MIRType_Object || b.rhs == MIRType_Object) {
        specialization_ = MIRType_None;
    } else {
        specialization_ = MIRType_Int32;
        setCommutative();
    }
}

void
MShiftInstruction::infer(const TypeOracle::Binary &b)
{
    if (b.lhs == MIRType_Object || b.rhs == MIRType_Object)
        specialization_ = MIRType_None;
    else
        specialization_ = MIRType_Int32;
}

void
MUrsh::infer(const TypeOracle::Binary &b)
{
    this->MShiftInstruction::infer(b);
    if (specialization_ != MIRType_Int32)
        setResultType(MIRType_Value);
}

static inline bool
NeedNegativeZeroCheck(MDefinition *def)
{
    
    for (MUseIterator use = def->usesBegin(); use != def->usesEnd(); use++) {
        if (use->node()->isResumePoint())
            return true;

        MDefinition *use_def = use->node()->toDefinition();
        switch (use_def->op()) {
          case MDefinition::Op_Add: {
            
            
            
            
            
            MDefinition *operand = use_def->getOperand(0);
            if (operand == def) {
                operand = use_def->getOperand(1);

                
                
                if (operand == def)
                    return true;
            }

            
            
            if (operand->isMul()) {
                MMul *mul = operand->toMul();
                if (!mul->canBeNegativeZero())
                    return true;
            } else if (operand->isDiv()) {
                MDiv *div = operand->toDiv();
                if (!div->canBeNegativeZero())
                    return true;
            } else if (operand->isToInt32()) {
                MToInt32 *int32 = operand->toToInt32();
                if (!int32->canBeNegativeZero())
                    return true;
            } else if (operand->isPhi()) {
                return true;
            }
            break;
          }
          case MDefinition::Op_StoreElement:
          case MDefinition::Op_StoreElementHole:
          case MDefinition::Op_LoadElement:
          case MDefinition::Op_LoadElementHole:
          case MDefinition::Op_LoadTypedArrayElement:
          case MDefinition::Op_LoadTypedArrayElementHole:
          case MDefinition::Op_CharCodeAt:
          case MDefinition::Op_Mod:
          case MDefinition::Op_Sub:
            
            if (use_def->getOperand(0) == def)
                return true;
            if (use_def->numOperands() > 2) {
                for (size_t i = 2; i < use_def->numOperands(); i++) {
                    if (use_def->getOperand(i) == def)
                        return true;
                }
            }
            break;
          case MDefinition::Op_BoundsCheck:
            
            if (use_def->getOperand(1) == def)
                return true;
            break;
          case MDefinition::Op_ToString:
          case MDefinition::Op_FromCharCode:
          case MDefinition::Op_TableSwitch:
          case MDefinition::Op_Compare:
          case MDefinition::Op_BitAnd:
          case MDefinition::Op_BitOr:
          case MDefinition::Op_BitXor:
          case MDefinition::Op_Abs:
            
            break;
          default:
            return true;
        }
    }
    return false;
}

MDefinition *
MBinaryArithInstruction::foldsTo(bool useValueNumbers)
{
    if (specialization_ == MIRType_None)
        return this;

    MDefinition *lhs = getOperand(0);
    MDefinition *rhs = getOperand(1);
    if (MDefinition *folded = EvaluateConstantOperands(this))
        return folded;

    
    if (isAdd() && specialization_ != MIRType_Int32)
        return this;

    if (IsConstant(rhs, getIdentity()))
        return lhs;

    
    if (isSub())
        return this;

    if (IsConstant(lhs, getIdentity()))
        return rhs; 

    return this;
}

MDefinition *
MDiv::foldsTo(bool useValueNumbers)
{
    if (specialization_ == MIRType_None)
        return this;

    if (MDefinition *folded = EvaluateConstantOperands(this))
        return folded;

    
    
    if (IsConstant(lhs(), 0) || IsConstant(rhs(), 1))
        return lhs();

    return this;
}

void
MDiv::analyzeEdgeCasesForward()
{
    
    if (specialization_ != MIRType_Int32)
        return;

    
    if (rhs()->isConstant() && !rhs()->toConstant()->value().isInt32(0))
        canBeDivideByZero_ =  false;

    
    
    if (lhs()->isConstant() && !lhs()->toConstant()->value().isInt32(INT32_MIN))
        canBeNegativeOverflow_ = false;

    
    if (rhs()->isConstant() && !rhs()->toConstant()->value().isInt32(-1))
        canBeNegativeOverflow_ = false;

    
    if (lhs()->isConstant() && !lhs()->toConstant()->value().isInt32(0))
        canBeNegativeZero_ = false;

    
    if (rhs()->isConstant()) {
        const js::Value &val = rhs()->toConstant()->value();
        if (val.isInt32() && val.toInt32() >= 0)
            canBeNegativeZero_ = false;
    }
}

void
MDiv::analyzeEdgeCasesBackward()
{
    if (canBeNegativeZero_)
        canBeNegativeZero_ = NeedNegativeZeroCheck(this);
}

void
MDiv::analyzeTruncateBackward()
{
    if (!isTruncated())
        setTruncated(js::ion::EdgeCaseAnalysis::AllUsesTruncate(this));
}

bool
MDiv::updateForReplacement(MDefinition *ins_)
{
    JS_ASSERT(ins_->isDiv());
    MDiv *ins = ins_->toDiv();
    
    
    if (isTruncated())
        setTruncated(ins->isTruncated());
    return true;
}

static inline MDefinition *
TryFold(MDefinition *original, MDefinition *replacement)
{
    if (original->type() == replacement->type())
        return replacement;
    return original;
}

MDefinition *
MMod::foldsTo(bool useValueNumbers)
{
    if (specialization_ == MIRType_None)
        return this;

    if (MDefinition *folded = EvaluateConstantOperands(this))
        return folded;

    JSRuntime *rt = GetIonContext()->cx->runtime;
    double NaN = rt->NaNValue.toDouble();
    double Inf = rt->positiveInfinityValue.toDouble();

    
    bool lhsConstant = lhs()->isConstant() && lhs()->toConstant()->value().isNumber();
    bool rhsConstant = rhs()->isConstant() && rhs()->toConstant()->value().isNumber();

    double lhsd = lhsConstant ? lhs()->toConstant()->value().toNumber() : 0;
    double rhsd = rhsConstant ? rhs()->toConstant()->value().toNumber() : 0;

    
    if (lhsConstant && lhsd == NaN)
        return lhs();

    
    if (rhsConstant && rhsd == NaN)
        return rhs();

    
    if (rhsConstant && (rhsd == 0))
        return TryFold(this, MConstant::New(rt->NaNValue));

    
    
    if (lhsConstant && (lhsd == 0))
        return TryFold(this, lhs());

    
    if (lhsConstant && (lhsd == Inf || lhsd == -Inf))
        return TryFold(this, MConstant::New(rt->NaNValue));

    
    
    if (rhsConstant && (rhsd == Inf || rhsd == -Inf))
        return TryFold(this, lhs());

    return this;
}

void
MAdd::analyzeTruncateBackward()
{
    if (!isTruncated())
        setTruncated(js::ion::EdgeCaseAnalysis::AllUsesTruncate(this));
}

bool
MAdd::updateForReplacement(MDefinition *ins_)
{
    JS_ASSERT(ins_->isAdd());
    MAdd *ins = ins_->toAdd();
    if (isTruncated())
        setTruncated(ins->isTruncated());
    return true;
}

void
MSub::analyzeTruncateBackward()
{
    if (!isTruncated())
        setTruncated(js::ion::EdgeCaseAnalysis::AllUsesTruncate(this));
}

bool
MSub::updateForReplacement(MDefinition *ins_)
{
    JS_ASSERT(ins_->isSub());
    MSub *ins = ins_->toSub();
    if (isTruncated())
        setTruncated(ins->isTruncated());
    return true;
}

MDefinition *
MMul::foldsTo(bool useValueNumbers)
{
    MDefinition *out = MBinaryArithInstruction::foldsTo(useValueNumbers);
    if (out != this)
        return out;

    if (specialization() != MIRType_Int32)
        return this;

    if (lhs()->congruentTo(rhs()))
        canBeNegativeZero_ = false;

    return this;
}

void
MMul::analyzeEdgeCasesForward()
{
    
    
    if (specialization() != MIRType_Int32)
        return;

    
    if (lhs()->isConstant()) {
        const js::Value &val = lhs()->toConstant()->value();
        if (val.isInt32() && val.toInt32() > 0)
            canBeNegativeZero_ = false;
    }

    
    if (rhs()->isConstant()) {
        const js::Value &val = rhs()->toConstant()->value();
        if (val.isInt32() && val.toInt32() > 0)
            canBeNegativeZero_ = false;
    }

}

void
MMul::analyzeEdgeCasesBackward()
{
    if (canBeNegativeZero_)
        canBeNegativeZero_ = NeedNegativeZeroCheck(this);
}

bool
MMul::updateForReplacement(MDefinition *ins)
{
    return true;
}

void
MBinaryArithInstruction::infer(JSContext *cx, const TypeOracle::BinaryTypes &b)
{
    
    
    
    MIRType lhs = MIRTypeFromValueType(b.lhsTypes->getKnownTypeTag(cx));
    MIRType rhs = MIRType_Int32;

    
    bool lhsCoerces = b.lhsTypes->knownNonStringPrimitive(cx);
    bool rhsCoerces = true;

    
    if (b.rhsTypes) {
        rhs = MIRTypeFromValueType(b.rhsTypes->getKnownTypeTag(cx));
        rhsCoerces = b.rhsTypes->knownNonStringPrimitive(cx);
    }

    MIRType rval = MIRTypeFromValueType(b.outTypes->getKnownTypeTag(cx));

    
    if (rval != MIRType_Int32 && rval != MIRType_Double) {
        specialization_ = MIRType_None;
        return;
    }

    
    if (!lhsCoerces || !rhsCoerces) {
        specialization_ = MIRType_None;
        return;
    }

    JS_ASSERT(lhs < MIRType_String || lhs == MIRType_Value);
    JS_ASSERT(rhs < MIRType_String || rhs == MIRType_Value);

    
    if (lhs == MIRType_Value || rhs == MIRType_Value) {
        if (rval != MIRType_Double) {
            specialization_ = MIRType_None;
            return;
        }
    }

    
    if (isMod() && rval == MIRType_Double) {
        specialization_ = MIRType_None;
        return;
    }

    
    
    if (rval == MIRType_Int32 && (lhs == MIRType_Undefined || rhs == MIRType_Undefined)) {
        specialization_ = MIRType_None;
        return;
    }

    specialization_ = rval;

    setCommutative();
    setResultType(rval);
}

void
MCompare::infer(JSContext *cx, const TypeOracle::BinaryTypes &b)
{
    
    if (!b.lhsTypes || !b.rhsTypes)
        return;

    MIRType lhs = MIRTypeFromValueType(b.lhsTypes->getKnownTypeTag(cx));
    MIRType rhs = MIRTypeFromValueType(b.rhsTypes->getKnownTypeTag(cx));

    if ((lhs == MIRType_Int32 && rhs == MIRType_Int32) ||
        (lhs == MIRType_Boolean && rhs == MIRType_Boolean))
    {
        specialization_ = MIRType_Int32;
        return;
    }
    if (IsNumberType(lhs) && IsNumberType(rhs)) {
        specialization_ = MIRType_Double;
        return;
    }
    if (jsop() == JSOP_STRICTEQ || jsop() == JSOP_EQ ||
        jsop() == JSOP_STRICTNE || jsop() == JSOP_NE)
    {
        if (lhs == MIRType_Object && rhs == MIRType_Object) {
            if (b.lhsTypes->hasObjectFlags(cx, types::OBJECT_FLAG_SPECIAL_EQUALITY) ||
                b.rhsTypes->hasObjectFlags(cx, types::OBJECT_FLAG_SPECIAL_EQUALITY))
            {
                return;
            }
            specialization_ = MIRType_Object;
            return;
        }

        if (lhs == MIRType_String && rhs == MIRType_String) {
            
            specialization_ = MIRType_String;
            return;
        }

        if (IsNullOrUndefined(lhs)) {
            
            
            
            
            specialization_ = lhs;
            swapOperands();
            return;
        }

        if (IsNullOrUndefined(rhs)) {
            specialization_ = rhs;
            return;
        }
    }

    if (jsop() == JSOP_STRICTEQ || jsop() == JSOP_STRICTNE) {
        
        JS_ASSERT(!(lhs == MIRType_Boolean && rhs == MIRType_Boolean));

        if (lhs == MIRType_Boolean) {
            
            
            swapOperands();
            specialization_ = MIRType_Boolean;
            return;
        }
        if (rhs == MIRType_Boolean) {
            specialization_ = MIRType_Boolean;
            return;
        }
    }
}

MBitNot *
MBitNot::New(MDefinition *input)
{
    return new MBitNot(input);
}

MDefinition *
MBitNot::foldsTo(bool useValueNumbers)
{
    if (specialization_ != MIRType_Int32)
        return this;

    MDefinition *input = getOperand(0);

    if (input->isConstant()) {
        js::Value v = Int32Value(~(input->toConstant()->value().toInt32()));
        return MConstant::New(v);
    }

    if (input->isBitNot() && input->toBitNot()->specialization_ == MIRType_Int32) {
        JS_ASSERT(input->getOperand(0)->type() == MIRType_Int32);
        return input->getOperand(0); 
    }

    return this;
}

MDefinition *
MTypeOf::foldsTo(bool useValueNumbers)
{
    
    
    JS_ASSERT(input()->type() == MIRType_Value);

    JSType type;

    switch (inputType()) {
      case MIRType_Double:
      case MIRType_Int32:
        type = JSTYPE_NUMBER;
        break;
      case MIRType_String:
        type = JSTYPE_STRING;
        break;
      case MIRType_Null:
        type = JSTYPE_OBJECT;
        break;
      case MIRType_Undefined:
        type = JSTYPE_VOID;
        break;
      case MIRType_Boolean:
        type = JSTYPE_BOOLEAN;
        break;
      default:
        return this;
    }

    JSContext *cx = GetIonContext()->cx;
    return MConstant::New(StringValue(cx->runtime->atomState.typeAtoms[type]));
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

MLsh *
MLsh::New(MDefinition *left, MDefinition *right)
{
    return new MLsh(left, right);
}

MRsh *
MRsh::New(MDefinition *left, MDefinition *right)
{
    return new MRsh(left, right);
}

MUrsh *
MUrsh::New(MDefinition *left, MDefinition *right)
{
    return new MUrsh(left, right);
}

MResumePoint *
MResumePoint::New(MBasicBlock *block, jsbytecode *pc, MResumePoint *parent, Mode mode)
{
    MResumePoint *resume = new MResumePoint(block, pc, parent, mode);
    if (!resume->init(block))
        return NULL;
    resume->inherit(block);
    return resume;
}

MResumePoint::MResumePoint(MBasicBlock *block, jsbytecode *pc, MResumePoint *caller,
                           Mode mode)
  : MNode(block),
    stackDepth_(block->stackDepth()),
    pc_(pc),
    caller_(caller),
    mode_(mode)
{
}

bool
MResumePoint::init(MBasicBlock *block)
{
    operands_ = block->graph().allocate<MDefinition *>(stackDepth());
    if (!operands_)
        return false;
    return true;
}

void
MResumePoint::inherit(MBasicBlock *block)
{
    for (size_t i = 0; i < stackDepth(); i++) {
        MDefinition *def = block->getSlot(i);
        
        
        if (def->isPassArg())
            def = def->toPassArg()->getArgument();
        initOperand(i, def);
    }
}

MDefinition *
MToInt32::foldsTo(bool useValueNumbers)
{
    MDefinition *input = getOperand(0);
    if (input->type() == MIRType_Int32)
        return input;
    return this;
}

void
MToInt32::analyzeEdgeCasesBackward()
{
    canBeNegativeZero_ = NeedNegativeZeroCheck(this);
}

MDefinition *
MTruncateToInt32::foldsTo(bool useValueNumbers)
{
    MDefinition *input = getOperand(0);
    if (input->type() == MIRType_Int32)
        return input;

    if (input->type() == MIRType_Double && input->isConstant()) {
        const Value &v = input->toConstant()->value();
        uint32 ret = ToInt32(v.toDouble());
        return MConstant::New(Int32Value(ret));
    }

    return this;
}

MDefinition *
MToDouble::foldsTo(bool useValueNumbers)
{
    if (input()->isConstant()) {
        const Value &v = input()->toConstant()->value();
        if (v.isPrimitive()) {
            double out;
            DebugOnly<bool> ok = ToNumber(GetIonContext()->cx, v, &out);
            JS_ASSERT(ok);

            return MConstant::New(DoubleValue(out));
        }
    }

    return this;
}

MDefinition *
MToString::foldsTo(bool useValueNumbers)
{
    return this;
}

MDefinition *
MClampToUint8::foldsTo(bool useValueNumbers)
{
    if (input()->isConstant()) {
        const Value &v = input()->toConstant()->value();
        if (v.isDouble()) {
            int32_t clamped = ClampDoubleToUint8(v.toDouble());
            return MConstant::New(Int32Value(clamped));
        }
        if (v.isInt32()) {
            int32_t clamped = ClampIntForUint8Array(v.toInt32());
            return MConstant::New(Int32Value(clamped));
        }
    }
    return this;
}

bool
MCompare::tryFold(bool *result)
{
    JSOp op = jsop();

    if (IsNullOrUndefined(specialization())) {
        JS_ASSERT(op == JSOP_EQ || op == JSOP_STRICTEQ ||
                  op == JSOP_NE || op == JSOP_STRICTNE);

        
        switch (lhs()->type()) {
          case MIRType_Value:
            return false;
          case MIRType_Undefined:
          case MIRType_Null:
            if (lhs()->type() == specialization()) {
                
                *result = (op == JSOP_EQ || op == JSOP_STRICTEQ);
            } else {
                
                
                *result = (op == JSOP_EQ || op == JSOP_STRICTNE);
            }
            return true;
          case MIRType_Int32:
          case MIRType_Double:
          case MIRType_String:
          case MIRType_Object:
          case MIRType_Boolean:
            *result = (op == JSOP_NE || op == JSOP_STRICTNE);
            return true;
          default:
            JS_NOT_REACHED("Unexpected type");
            return false;
        }
    }

    if (specialization_ == MIRType_Boolean) {
        JS_ASSERT(op == JSOP_STRICTEQ || op == JSOP_STRICTNE);
        JS_ASSERT(rhs()->type() == MIRType_Boolean);

        switch (lhs()->type()) {
          case MIRType_Value:
            return false;
          case MIRType_Int32:
          case MIRType_Double:
          case MIRType_String:
          case MIRType_Object:
          case MIRType_Null:
          case MIRType_Undefined:
            *result = (op == JSOP_STRICTNE);
            return true;
          case MIRType_Boolean:
            
            JS_NOT_REACHED("Wrong specialization");
            return false;
          default:
            JS_NOT_REACHED("Unexpected type");
            return false;
        }
    }

    return false;
}

bool
MCompare::evaluateConstantOperands(bool *result)
{
    if (type() != MIRType_Boolean)
        return false;

    MDefinition *left = getOperand(0);
    MDefinition *right = getOperand(1);

    if (!left->isConstant() || !right->isConstant())
        return false;

    Value lhs = left->toConstant()->value();
    Value rhs = right->toConstant()->value();

    if (!lhs.isNumber() || !rhs.isNumber())
        return false;

    switch (jsop_) {
      case JSOP_LT:
        *result = (lhs.toNumber() < rhs.toNumber());
        break;
      case JSOP_LE:
        *result = (lhs.toNumber() <= rhs.toNumber());
        break;
      case JSOP_GT:
        *result = (lhs.toNumber() > rhs.toNumber());
        break;
      case JSOP_GE:
        *result = (lhs.toNumber() >= rhs.toNumber());
        break;
      case JSOP_EQ:
        *result = (lhs.toNumber() == rhs.toNumber());
        break;
      case JSOP_NE:
        *result = (lhs.toNumber() != rhs.toNumber());
        break;
      default:
        return false;
    }

    return true;
}

MDefinition *
MCompare::foldsTo(bool useValueNumbers)
{
    bool result;

    if (tryFold(&result))
        return MConstant::New(BooleanValue(result));
    else if (evaluateConstantOperands(&result))
        return MConstant::New(BooleanValue(result));

    return this;
}

MDefinition *
MNot::foldsTo(bool useValueNumbers)
{
    
    if (operand()->isConstant()) {
       const Value &v = operand()->toConstant()->value();
        
        return MConstant::New(BooleanValue(!js_ValueToBoolean(v)));
    }

    
    if (operand()->type() == MIRType_Object)
        return MConstant::New(BooleanValue(false));

    
    if (operand()->type() == MIRType_Undefined || operand()->type() == MIRType_Null)
        return MConstant::New(BooleanValue(true));

    return this;
}

void
MBeta::printOpcode(FILE *fp)
{
    PrintOpcodeName(fp, op());
    fprintf(fp, " ");
    getOperand(0)->printName(fp);
    fprintf(fp, " ");
    comparison_.printRange(fp);
}
