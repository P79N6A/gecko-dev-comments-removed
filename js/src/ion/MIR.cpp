





#include "BaselineInspector.h"
#include "IonBuilder.h"
#include "LICM.h" 
#include "MIR.h"
#include "MIRGraph.h"
#include "EdgeCaseAnalysis.h"
#include "RangeAnalysis.h"
#include "IonSpewer.h"
#include "jsnum.h"
#include "jsstr.h"
#include "jsatominlines.h"
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



bool
MDefinition::earlyAbortCheck()
{
    if (isPhi())
        return false;
    for (size_t i = 0; i < numOperands(); i++) {
        if (getOperand(i)->block()->earlyAbort()) {
            block()->setEarlyAbort();
            IonSpew(IonSpew_Range, "Ignoring value from block %d because instruction %d is in a block that aborts", block()->id(), getOperand(i)->id());
            return true;
        }
    }
    return false;
}

static inline bool
EqualValues(bool useGVN, MDefinition *left, MDefinition *right)
{
    if (useGVN)
        return left->valueNumber() == right->valueNumber();

    return left->id() == right->id();
}

static MConstant *
EvaluateConstantOperands(MBinaryInstruction *ins, bool *ptypeChange = NULL)
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
        uint32_t unsignedLhs = (uint32_t)lhs.toInt32();
        ret.setNumber(uint32_t(unsignedLhs >> (rhs.toInt32() & 0x1F)));
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

    if (ins->type() != MIRTypeFromValue(ret)) {
        if (ptypeChange)
            *ptypeChange = true;
        return NULL;
    }

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
        uint32_t valueNumber = getOperand(i)->valueNumber();
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
}

void
MDefinition::analyzeEdgeCasesBackward()
{
}

static bool
MaybeEmulatesUndefined(JSContext *cx, MDefinition *op)
{
    if (!op->mightBeType(MIRType_Object))
        return false;

    types::StackTypeSet *types = op->resultTypeSet();
    if (!types)
        return true;

    if (!types->maybeObject())
        return false;
    return types->hasObjectFlags(cx, types::OBJECT_FLAG_EMULATES_UNDEFINED);
}

void
MTest::infer(JSContext *cx)
{
    JS_ASSERT(operandMightEmulateUndefined());

    if (!MaybeEmulatesUndefined(cx, getOperand(0)))
        markOperandCantEmulateUndefined();
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
MNode::replaceOperand(MUseIterator use, MDefinition *def)
{
    JS_ASSERT(def != NULL);
    uint32_t index = use->index();
    MDefinition *prev = use->producer();

    JS_ASSERT(use->index() < numOperands());
    JS_ASSERT(use->producer() == getOperand(index));
    JS_ASSERT(use->consumer() == this);

    if (prev == def)
        return use;

    MUseIterator result(prev->removeUse(use));
    setOperand(index, def);
    return result;
}

void
MNode::replaceOperand(size_t index, MDefinition *def)
{
    JS_ASSERT(def != NULL);
    MUse *use = getUseFor(index);
    MDefinition *prev = use->producer();

    JS_ASSERT(use->index() == index);
    JS_ASSERT(use->index() < numOperands());
    JS_ASSERT(use->producer() == getOperand(index));
    JS_ASSERT(use->consumer() == this);

    if (prev == def)
        return;

    prev->removeUse(use);
    setOperand(index, def);
}

void
MNode::discardOperand(size_t index)
{
    MUse *use = getUseFor(index);

    JS_ASSERT(use->index() == index);
    JS_ASSERT(use->producer() == getOperand(index));
    JS_ASSERT(use->consumer() == this);

    use->producer()->removeUse(use);

#ifdef DEBUG
    
    use->set(NULL, NULL, index);
#endif
}

void
MDefinition::replaceAllUsesWith(MDefinition *dom)
{
    JS_ASSERT(dom != NULL);
    if (dom == this)
        return;

    for (MUseIterator i(usesBegin()); i != usesEnd(); ) {
        JS_ASSERT(i->producer() == this);
        i = i->consumer()->replaceOperand(i, dom);
    }
}

static inline bool
IsPowerOfTwo(uint32_t n)
{
    return (n > 0) && ((n & (n - 1)) == 0);
}

MConstant *
MConstant::New(const Value &v)
{
    return new MConstant(v);
}

types::StackTypeSet *
ion::MakeSingletonTypeSet(JSObject *obj)
{
    LifoAlloc *alloc = GetIonContext()->temp->lifoAlloc();
    types::StackTypeSet *types = alloc->new_<types::StackTypeSet>();
    if (!types)
        return NULL;
    types::Type objectType = types::Type::ObjectType(obj);
    types->addObject(objectType.objectKey(), alloc);
    return types;
}

MConstant::MConstant(const js::Value &vp)
  : value_(vp)
{
    setResultType(MIRTypeFromValue(vp));
    if (vp.isObject()) {
        
        
        setResultTypeSet(MakeSingletonTypeSet(&vp.toObject()));
    }

    setMovable();
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
        if (value().toObject().isFunction()) {
            JSFunction *fun = value().toObject().toFunction();
            if (fun->displayAtom()) {
                fputs("function ", fp);
                FileEscapedString(fp, fun->displayAtom(), 0);
            } else {
                fputs("unnamed function", fp);
            }
            if (fun->hasScript()) {
                JSScript *script = fun->nonLazyScript();
                fprintf(fp, " (%s:%u)",
                        script->filename() ? script->filename() : "", script->lineno);
            }
            fprintf(fp, " at %p", (void *) fun);
            break;
        }
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
MParameter::New(int32_t index, types::StackTypeSet *types)
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

MCompare *
MCompare::New(MDefinition *left, MDefinition *right, JSOp op)
{
    return new MCompare(left, right, op);
}

MCompare *
MCompare::NewAsmJS(MDefinition *left, MDefinition *right, JSOp op, CompareType compareType)
{
    JS_ASSERT(compareType == Compare_Int32 || compareType == Compare_UInt32 ||
              compareType == Compare_Double);
    MCompare *comp = new MCompare(left, right, op);
    comp->compareType_ = compareType;
    comp->operandMightEmulateUndefined_ = false;
    comp->setResultType(MIRType_Int32);
    return comp;
}

MTableSwitch *
MTableSwitch::New(MDefinition *ins, int32_t low, int32_t high)
{
    return new MTableSwitch(ins, low, high);
}

MGoto *
MGoto::New(MBasicBlock *target)
{
    JS_ASSERT(target);
    return new MGoto(target);
}

void
MUnbox::printOpcode(FILE *fp)
{
    PrintOpcodeName(fp, op());
    fprintf(fp, " ");
    getOperand(0)->printName(fp);
    fprintf(fp, " ");

    switch (type()) {
      case MIRType_Int32: fprintf(fp, "to Int32"); break;
      case MIRType_Double: fprintf(fp, "to Double"); break;
      case MIRType_Boolean: fprintf(fp, "to Boolean"); break;
      case MIRType_String: fprintf(fp, "to String"); break;
      case MIRType_Object: fprintf(fp, "to Object"); break;
      default: break;
    }

    switch (mode()) {
      case Fallible: fprintf(fp, " (fallible)"); break;
      case Infallible: fprintf(fp, " (infallible)"); break;
      case TypeBarrier: fprintf(fp, " (typebarrier)"); break;
      case TypeGuard: fprintf(fp, " (typeguard)"); break;
      default: break;
    }
}

MPhi *
MPhi::New(uint32_t slot)
{
    return new MPhi(slot);
}

void
MPhi::removeOperand(size_t index)
{
    MUse *use = getUseFor(index);

    JS_ASSERT(index < inputs_.length());
    JS_ASSERT(inputs_.length() > 1);

    JS_ASSERT(use->index() == index);
    JS_ASSERT(use->producer() == getOperand(index));
    JS_ASSERT(use->consumer() == this);

    
    use->producer()->removeUse(use);

    
    
    
    size_t length = inputs_.length();
    for (size_t i = index; i < length - 1; i++) {
        MUse *next = MPhi::getUseFor(i + 1);
        next->producer()->removeUse(next);
        MPhi::setOperand(i, next->producer());
    }

    
    inputs_.shrinkBy(1);
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
MPhi::reserveLength(size_t length)
{
    
    
    
    JS_ASSERT(numOperands() == 0);
#if DEBUG
    capacity_ = length;
#endif
    return inputs_.reserve(length);
}

static inline types::StackTypeSet *
MakeMIRTypeSet(MIRType type)
{
    JS_ASSERT(type != MIRType_Value);
    types::Type ntype = type == MIRType_Object
                        ? types::Type::AnyObjectType()
                        : types::Type::PrimitiveType(ValueTypeFromMIRType(type));
    return GetIonContext()->temp->lifoAlloc()->new_<types::StackTypeSet>(ntype);
}

void
ion::MergeTypes(MIRType *ptype, types::StackTypeSet **ptypeSet,
                MIRType newType, types::StackTypeSet *newTypeSet)
{
    if (newTypeSet && newTypeSet->empty())
        return;
    if (newType != *ptype) {
        if (IsNumberType(newType) && IsNumberType(*ptype)) {
            *ptype = MIRType_Double;
        } else if (*ptype != MIRType_Value) {
            if (!*ptypeSet)
                *ptypeSet = MakeMIRTypeSet(*ptype);
            *ptype = MIRType_Value;
        }
    }
    if (*ptypeSet) {
        LifoAlloc *alloc = GetIonContext()->temp->lifoAlloc();
        if (!newTypeSet && newType != MIRType_Value)
            newTypeSet = MakeMIRTypeSet(newType);
        if (newTypeSet) {
            if (!newTypeSet->isSubset(*ptypeSet))
                *ptypeSet = types::TypeSet::unionSets(*ptypeSet, newTypeSet, alloc);
        } else {
            *ptypeSet = NULL;
        }
    }
}

void
MPhi::specializeType()
{
#ifdef DEBUG
    JS_ASSERT(!specialized_);
    specialized_ = true;
#endif

    JS_ASSERT(!inputs_.empty());

    size_t start;
    if (hasBackedgeType_) {
        
        
        start = 0;
    } else {
        setResultType(getOperand(0)->type());
        setResultTypeSet(getOperand(0)->resultTypeSet());
        start = 1;
    }

    MIRType resultType = this->type();
    types::StackTypeSet *resultTypeSet = this->resultTypeSet();

    for (size_t i = start; i < inputs_.length(); i++) {
        MDefinition *def = getOperand(i);
        MergeTypes(&resultType, &resultTypeSet, def->type(), def->resultTypeSet());
    }

    setResultType(resultType);
    setResultTypeSet(resultTypeSet);
}

void
MPhi::addBackedgeType(MIRType type, types::StackTypeSet *typeSet)
{
    JS_ASSERT(!specialized_);

    if (hasBackedgeType_) {
        MIRType resultType = this->type();
        types::StackTypeSet *resultTypeSet = this->resultTypeSet();

        MergeTypes(&resultType, &resultTypeSet, type, typeSet);

        setResultType(resultType);
        setResultTypeSet(resultTypeSet);
    } else {
        setResultType(type);
        setResultTypeSet(typeSet);
        hasBackedgeType_ = true;
    }
}

bool
MPhi::typeIncludes(MDefinition *def)
{
    if (def->type() == MIRType_Int32 && this->type() == MIRType_Double)
        return true;

    if (types::StackTypeSet *types = def->resultTypeSet()) {
        if (this->resultTypeSet())
            return types->isSubset(this->resultTypeSet());
        if (this->type() == MIRType_Value || types->empty())
            return true;
        return this->type() == MIRTypeFromValueType(types->getKnownTypeTag());
    }

    if (def->type() == MIRType_Value) {
        
        return this->type() == MIRType_Value
            && (!this->resultTypeSet() || this->resultTypeSet()->unknown());
    }

    return this->mightBeType(def->type());
}

void
MPhi::addInput(MDefinition *ins)
{
    
    
    JS_ASSERT(inputs_.length() < capacity_);

    uint32_t index = inputs_.length();
    inputs_.append(MUse());
    MPhi::setOperand(index, ins);
}

bool
MPhi::addInputSlow(MDefinition *ins, bool *ptypeChange)
{
    
    
    
    
    
    
    uint32_t index = inputs_.length();
    bool performingRealloc = !inputs_.canAppendWithoutRealloc(1);

    
    if (performingRealloc) {
        for (uint32_t i = 0; i < index; i++) {
            MUse *use = &inputs_[i];
            use->producer()->removeUse(use);
        }
    }

    
    if (!inputs_.append(MUse()))
        return false;

    MPhi::setOperand(index, ins);

    if (ptypeChange) {
        MIRType resultType = this->type();
        types::StackTypeSet *resultTypeSet = this->resultTypeSet();

        MergeTypes(&resultType, &resultTypeSet, ins->type(), ins->resultTypeSet());

        if (resultType != this->type() || resultTypeSet != this->resultTypeSet()) {
            *ptypeChange = true;
            setResultType(resultType);
            setResultTypeSet(resultTypeSet);
        }
    }

    
    if (performingRealloc) {
        for (uint32_t i = 0; i < index; i++) {
            MUse *use = &inputs_[i];
            use->producer()->addUse(use);
        }
    }

    return true;
}

uint32_t
MPrepareCall::argc() const
{
    JS_ASSERT(useCount() == 1);
    MCall *call = usesBegin()->consumer()->toDefinition()->toCall();
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
    setOperand(argnum + NumNonArgumentOperands, arg->toDefinition());
}

void
MBitNot::infer()
{
    if (getOperand(0)->mightBeType(MIRType_Object))
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

    if (MDefinition *folded = EvaluateConstantOperands(this))
        return folded;

    return this;
}

MDefinition *
MBinaryBitwiseInstruction::foldUnnecessaryBitop()
{
    if (specialization_ != MIRType_Int32)
        return this;

    
    

    MDefinition *lhs = getOperand(0);
    MDefinition *rhs = getOperand(1);

    if (IsConstant(lhs, 0))
        return foldIfZero(0);

    if (IsConstant(rhs, 0))
        return foldIfZero(1);

    if (IsConstant(lhs, -1))
        return foldIfNegOne(0);

    if (IsConstant(rhs, -1))
        return foldIfNegOne(1);

    if (EqualValues(false, lhs, rhs))
        return foldIfEqual();

    return this;
}

void
MBinaryBitwiseInstruction::infer()
{
    if (getOperand(0)->mightBeType(MIRType_Object) || getOperand(1)->mightBeType(MIRType_Object)) {
        specialization_ = MIRType_None;
    } else {
        specialization_ = MIRType_Int32;
        setCommutative();
    }
}

void
MBinaryBitwiseInstruction::specializeForAsmJS()
{
    specialization_ = MIRType_Int32;
    JS_ASSERT(type() == MIRType_Int32);
    setCommutative();
}

void
MShiftInstruction::infer()
{
    if (getOperand(0)->mightBeType(MIRType_Object) || getOperand(1)->mightBeType(MIRType_Object))
        specialization_ = MIRType_None;
    else
        specialization_ = MIRType_Int32;
}

void
MUrsh::infer()
{
    if (getOperand(0)->mightBeType(MIRType_Object) || getOperand(1)->mightBeType(MIRType_Object)) {
        specialization_ = MIRType_None;
        setResultType(MIRType_Value);
        return;
    }

    if (type() == MIRType_Int32) {
        specialization_ = MIRType_Int32;
        return;
    }

    specialization_ = MIRType_Double;
    setResultType(MIRType_Double);
}

static inline bool
NeedNegativeZeroCheck(MDefinition *def)
{
    
    for (MUseIterator use = def->usesBegin(); use != def->usesEnd(); use++) {
        if (use->consumer()->isResumePoint())
            continue;

        MDefinition *use_def = use->consumer()->toDefinition();
        switch (use_def->op()) {
          case MDefinition::Op_Add: {
            
            if (use_def->toAdd()->isTruncated())
                break;

            

            
            
            
            MDefinition *first = use_def->getOperand(0);
            MDefinition *second = use_def->getOperand(1);
            if (first->id() > second->id()) {
                MDefinition *temp = first;
                first = second;
                second = temp;
            }

            if (def == first) {
                
                
                
                
                
                
                
                
                
                
                
                switch (second->op()) {
                  case MDefinition::Op_Constant:
                  case MDefinition::Op_BitAnd:
                  case MDefinition::Op_BitOr:
                  case MDefinition::Op_BitXor:
                  case MDefinition::Op_BitNot:
                  case MDefinition::Op_Lsh:
                  case MDefinition::Op_Rsh:
                    break;
                  default:
                    return true;
                }
            }

            
            
            
            break;
          }
          case MDefinition::Op_Sub:
            
            if (use_def->toSub()->isTruncated())
                break;
            
          case MDefinition::Op_StoreElement:
          case MDefinition::Op_StoreElementHole:
          case MDefinition::Op_LoadElement:
          case MDefinition::Op_LoadElementHole:
          case MDefinition::Op_LoadTypedArrayElement:
          case MDefinition::Op_LoadTypedArrayElementHole:
          case MDefinition::Op_CharCodeAt:
          case MDefinition::Op_Mod:
            
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
          case MDefinition::Op_TruncateToInt32:
            
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

bool
MAbs::fallible() const
{
    return !implicitTruncate_ && (!range() || !range()->isInt32());
}

MDefinition *
MDiv::foldsTo(bool useValueNumbers)
{
    if (specialization_ == MIRType_None)
        return this;

    if (MDefinition *folded = EvaluateConstantOperands(this))
        return folded;

    return this;
}

void
MDiv::analyzeEdgeCasesForward()
{
    
    if (specialization_ != MIRType_Int32)
        return;

    
    if (rhs()->isConstant() && !rhs()->toConstant()->value().isInt32(0))
        canBeDivideByZero_ = false;

    
    
    if (lhs()->isConstant() && !lhs()->toConstant()->value().isInt32(INT32_MIN))
        canBeNegativeOverflow_ = false;

    
    if (rhs()->isConstant() && !rhs()->toConstant()->value().isInt32(-1))
        canBeNegativeOverflow_ = false;

    
    if (lhs()->isConstant() && !lhs()->toConstant()->value().isInt32(0))
        setCanBeNegativeZero(false);

    
    if (rhs()->isConstant()) {
        const js::Value &val = rhs()->toConstant()->value();
        if (val.isInt32() && val.toInt32() >= 0)
            setCanBeNegativeZero(false);
    }
}

void
MDiv::analyzeEdgeCasesBackward()
{
    if (canBeNegativeZero() && !NeedNegativeZeroCheck(this))
        setCanBeNegativeZero(false);
}

bool
MDiv::fallible()
{
    return !isTruncated();
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

    return this;
}

bool
MMod::fallible()
{
    return !isTruncated();
}

bool
MAdd::fallible()
{
    
    
    if (isTruncated())
        return false;
    if (range() && range()->isInt32())
        return false;
    return true;
}

bool
MSub::fallible()
{
    
    if (isTruncated())
        return false;
    if (range() && range()->isInt32())
        return false;
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

    if (EqualValues(useValueNumbers, lhs(), rhs()))
        setCanBeNegativeZero(false);

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
            setCanBeNegativeZero(false);
    }

    
    if (rhs()->isConstant()) {
        const js::Value &val = rhs()->toConstant()->value();
        if (val.isInt32() && val.toInt32() > 0)
            setCanBeNegativeZero(false);
    }
}

void
MMul::analyzeEdgeCasesBackward()
{
    if (canBeNegativeZero() && !NeedNegativeZeroCheck(this))
        setCanBeNegativeZero(false);
}

bool
MMul::updateForReplacement(MDefinition *ins_)
{
    MMul *ins = ins_->toMul();
    bool negativeZero = canBeNegativeZero() || ins->canBeNegativeZero();
    setCanBeNegativeZero(negativeZero);
    return true;
}

bool
MMul::canOverflow()
{
    if (isTruncated())
        return false;
    return !range() || !range()->isInt32();
}

static inline bool
KnownNonStringPrimitive(MDefinition *op)
{
    return !op->mightBeType(MIRType_Object)
        && !op->mightBeType(MIRType_String)
        && !op->mightBeType(MIRType_Magic);
}

void
MBinaryArithInstruction::infer(BaselineInspector *inspector,
                               jsbytecode *pc,
                               bool overflowed)
{
    JS_ASSERT(this->type() == MIRType_Value);

    specialization_ = MIRType_None;

    
    MIRType lhs = getOperand(0)->type();
    MIRType rhs = getOperand(1)->type();

    
    
    if (!KnownNonStringPrimitive(getOperand(0)) || !KnownNonStringPrimitive(getOperand(1)))
        return inferFallback(inspector, pc);

    
    
    if (lhs == MIRType_Int32 && rhs == MIRType_Int32)
        setResultType(MIRType_Int32);
    else if (lhs == MIRType_Double || rhs == MIRType_Double)
        setResultType(MIRType_Double);
    else
        return inferFallback(inspector, pc);

    
    if (overflowed)
        setResultType(MIRType_Double);

    
    
    if (isMul() || isDiv()) {
        bool typeChange = false;
        EvaluateConstantOperands(this, &typeChange);
        if (typeChange)
            setResultType(MIRType_Double);
    }

    JS_ASSERT(lhs < MIRType_String || lhs == MIRType_Value);
    JS_ASSERT(rhs < MIRType_String || rhs == MIRType_Value);

    MIRType rval = this->type();

    
    if (lhs == MIRType_Value || rhs == MIRType_Value) {
        if (rval != MIRType_Double) {
            specialization_ = MIRType_None;
            return;
        }
    }

    
    
    if (rval == MIRType_Int32 && (lhs == MIRType_Undefined || rhs == MIRType_Undefined)) {
        specialization_ = MIRType_None;
        return;
    }

    specialization_ = rval;

    if (isAdd() || isMul())
        setCommutative();
    setResultType(rval);
}

void
MBinaryArithInstruction::inferFallback(BaselineInspector *inspector,
                                       jsbytecode *pc)
{
    
    specialization_ = inspector->expectedBinaryArithSpecialization(pc);
    if (specialization_ != MIRType_None) {
        setResultType(specialization_);
        return;
    }

    
    
    
    
    
    if (block()->info().executionMode() == ParallelExecution) {
        specialization_ = MIRType_Double;
        setResultType(MIRType_Double);
        return;
    }
}

static bool
SafelyCoercesToDouble(MDefinition *op)
{
    
    
    return KnownNonStringPrimitive(op) && !op->mightBeType(MIRType_Null);
}

static bool
ObjectOrSimplePrimitive(MDefinition *op)
{
    
    return !op->mightBeType(MIRType_String)
        && !op->mightBeType(MIRType_Double)
        && !op->mightBeType(MIRType_Magic);
}

static bool
CanDoValueBitwiseCmp(JSContext *cx, MDefinition *lhs, MDefinition *rhs, bool looseEq)
{
    
    
    if (!ObjectOrSimplePrimitive(lhs) || !ObjectOrSimplePrimitive(rhs))
        return false;

    
    if (MaybeEmulatesUndefined(cx, lhs) || MaybeEmulatesUndefined(cx, rhs))
        return false;

    
    
    if (looseEq) {

        
        
        if ((lhs->mightBeType(MIRType_Undefined) && rhs->mightBeType(MIRType_Null)) ||
            (lhs->mightBeType(MIRType_Null) && rhs->mightBeType(MIRType_Undefined)))
        {
            return false;
        }

        
        
        if ((lhs->mightBeType(MIRType_Int32) && rhs->mightBeType(MIRType_Boolean)) ||
            (lhs->mightBeType(MIRType_Boolean) && rhs->mightBeType(MIRType_Int32)))
        {
            return false;
        }

        
        
        bool simpleLHS = lhs->mightBeType(MIRType_Boolean) || lhs->mightBeType(MIRType_Int32);
        bool simpleRHS = rhs->mightBeType(MIRType_Boolean) || rhs->mightBeType(MIRType_Int32);
        if ((lhs->mightBeType(MIRType_Object) && simpleRHS) ||
            (rhs->mightBeType(MIRType_Object) && simpleLHS))
        {
            return false;
        }
    }

    return true;
}

MIRType
MCompare::inputType()
{
    switch(compareType_) {
      case Compare_Undefined:
        return MIRType_Undefined;
      case Compare_Null:
        return MIRType_Null;
      case Compare_Boolean:
        return MIRType_Boolean;
      case Compare_UInt32:
      case Compare_Int32:
        return MIRType_Int32;
      case Compare_Double:
      case Compare_DoubleMaybeCoerceLHS:
      case Compare_DoubleMaybeCoerceRHS:
        return MIRType_Double;
      case Compare_String:
      case Compare_StrictString:
        return MIRType_String;
      case Compare_Object:
        return MIRType_Object;
      case Compare_Unknown:
      case Compare_Value:
        return MIRType_Value;
      default:
        JS_NOT_REACHED("No known conversion");
        return MIRType_None;
    }
}

static inline bool
MustBeUInt32(MDefinition *def, MDefinition **pwrapped)
{
    if (def->isUrsh()) {
        *pwrapped = def->toUrsh()->getOperand(0);
        MDefinition *rhs = def->toUrsh()->getOperand(1);
        return rhs->isConstant()
            && rhs->toConstant()->value().isInt32()
            && rhs->toConstant()->value().toInt32() == 0;
    }

    if (def->isConstant()) {
        *pwrapped = def;
        return def->toConstant()->value().isInt32()
            && def->toConstant()->value().toInt32() >= 0;
    }

    return false;
}

void
MCompare::infer(JSContext *cx, BaselineInspector *inspector, jsbytecode *pc)
{
    JS_ASSERT(operandMightEmulateUndefined());

    if (!MaybeEmulatesUndefined(cx, getOperand(0)) && !MaybeEmulatesUndefined(cx, getOperand(1)))
        markNoOperandEmulatesUndefined();

    MIRType lhs = getOperand(0)->type();
    MIRType rhs = getOperand(1)->type();

    bool looseEq = jsop() == JSOP_EQ || jsop() == JSOP_NE;
    bool strictEq = jsop() == JSOP_STRICTEQ || jsop() == JSOP_STRICTNE;
    bool relationalEq = !(looseEq || strictEq);

    
    
    
    MDefinition *newlhs, *newrhs;
    if (MustBeUInt32(getOperand(0), &newlhs) && MustBeUInt32(getOperand(1), &newrhs)) {
        if (newlhs != getOperand(0))
            replaceOperand(0, newlhs);
        if (newrhs != getOperand(1))
            replaceOperand(1, newrhs);
        compareType_ = Compare_UInt32;
        return;
    }

    
    if ((lhs == MIRType_Int32 && rhs == MIRType_Int32) ||
        (lhs == MIRType_Boolean && rhs == MIRType_Boolean))
    {
        compareType_ = Compare_Int32;
        return;
    }

    
    if (!strictEq &&
        (lhs == MIRType_Int32 || lhs == MIRType_Boolean) &&
        (rhs == MIRType_Int32 || rhs == MIRType_Boolean))
    {
        compareType_ = Compare_Int32;
        return;
    }

    
    if (IsNumberType(lhs) && IsNumberType(rhs)) {
        compareType_ = Compare_Double;
        return;
    }

    
    if (!strictEq && lhs == MIRType_Double && SafelyCoercesToDouble(getOperand(1))) {
        compareType_ = Compare_DoubleMaybeCoerceRHS;
        return;
    }
    if (!strictEq && rhs == MIRType_Double && SafelyCoercesToDouble(getOperand(0))) {
        compareType_ = Compare_DoubleMaybeCoerceLHS;
        return;
    }

    
    if (!relationalEq && lhs == MIRType_Object && rhs == MIRType_Object) {
        compareType_ = Compare_Object;
        return;
    }

    
    if (!relationalEq && lhs == MIRType_String && rhs == MIRType_String) {
        compareType_ = Compare_String;
        return;
    }

    if (strictEq && lhs == MIRType_String) {
        
        compareType_ = Compare_StrictString;
        swapOperands();
        return;
    }

    if (strictEq && rhs == MIRType_String) {
        compareType_ = Compare_StrictString;
        return;
    }

    
    if (!relationalEq && IsNullOrUndefined(lhs)) {
        
        
        
        
        compareType_ = (lhs == MIRType_Null) ? Compare_Null : Compare_Undefined;
        swapOperands();
        return;
    }

    
    if (!relationalEq && IsNullOrUndefined(rhs)) {
        compareType_ = (rhs == MIRType_Null) ? Compare_Null : Compare_Undefined;
        return;
    }

    
    if (strictEq && (lhs == MIRType_Boolean || rhs == MIRType_Boolean)) {
        
        JS_ASSERT(!(lhs == MIRType_Boolean && rhs == MIRType_Boolean));

        
        
        if (lhs == MIRType_Boolean)
             swapOperands();

        compareType_ = Compare_Boolean;
        return;
    }

    
    if (!relationalEq && CanDoValueBitwiseCmp(cx, getOperand(0), getOperand(1), looseEq)) {
        compareType_ = Compare_Value;
        return;
    }

    
    
    
    
    

    if (!strictEq)
        compareType_ = inspector->expectedCompareType(pc);
}

MBitNot *
MBitNot::New(MDefinition *input)
{
    return new MBitNot(input);
}

MBitNot *
MBitNot::NewAsmJS(MDefinition *input)
{
    MBitNot *ins = new MBitNot(input);
    ins->specialization_ = MIRType_Int32;
    JS_ASSERT(ins->type() == MIRType_Int32);
    return ins;
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

    JSRuntime *rt = GetIonContext()->runtime;
    return MConstant::New(StringValue(TypeName(type, rt)));
}

MBitAnd *
MBitAnd::New(MDefinition *left, MDefinition *right)
{
    return new MBitAnd(left, right);
}

MBitAnd *
MBitAnd::NewAsmJS(MDefinition *left, MDefinition *right)
{
    MBitAnd *ins = new MBitAnd(left, right);
    ins->specializeForAsmJS();
    return ins;
}

MBitOr *
MBitOr::New(MDefinition *left, MDefinition *right)
{
    return new MBitOr(left, right);
}

MBitOr *
MBitOr::NewAsmJS(MDefinition *left, MDefinition *right)
{
    MBitOr *ins = new MBitOr(left, right);
    ins->specializeForAsmJS();
    return ins;
}

MBitXor *
MBitXor::New(MDefinition *left, MDefinition *right)
{
    return new MBitXor(left, right);
}

MBitXor *
MBitXor::NewAsmJS(MDefinition *left, MDefinition *right)
{
    MBitXor *ins = new MBitXor(left, right);
    ins->specializeForAsmJS();
    return ins;
}

MLsh *
MLsh::New(MDefinition *left, MDefinition *right)
{
    return new MLsh(left, right);
}

MLsh *
MLsh::NewAsmJS(MDefinition *left, MDefinition *right)
{
    MLsh *ins = new MLsh(left, right);
    ins->specializeForAsmJS();
    return ins;
}

MRsh *
MRsh::New(MDefinition *left, MDefinition *right)
{
    return new MRsh(left, right);
}

MRsh *
MRsh::NewAsmJS(MDefinition *left, MDefinition *right)
{
    MRsh *ins = new MRsh(left, right);
    ins->specializeForAsmJS();
    return ins;
}

MUrsh *
MUrsh::New(MDefinition *left, MDefinition *right)
{
    return new MUrsh(left, right);
}

MUrsh *
MUrsh::NewAsmJS(MDefinition *left, MDefinition *right)
{
    MUrsh *ins = new MUrsh(left, right);
    ins->specializeForAsmJS();
    ins->canOverflow_ = false;
    return ins;
}

MResumePoint *
MResumePoint::New(MBasicBlock *block, jsbytecode *pc, MResumePoint *parent, Mode mode)
{
    MResumePoint *resume = new MResumePoint(block, pc, parent, mode);
    if (!resume->init())
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
    instruction_(NULL),
    mode_(mode)
{
    block->addResumePoint(this);
}

void
MResumePoint::inherit(MBasicBlock *block)
{
    for (size_t i = 0; i < stackDepth(); i++) {
        MDefinition *def = block->getSlot(i);
        
        
        if (def->isPassArg())
            def = def->toPassArg()->getArgument();
        setOperand(i, def);
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
    if (!NeedNegativeZeroCheck(this))
        setCanBeNegativeZero(false);
}

MDefinition *
MTruncateToInt32::foldsTo(bool useValueNumbers)
{
    MDefinition *input = getOperand(0);
    if (input->type() == MIRType_Int32)
        return input;

    if (input->type() == MIRType_Double && input->isConstant()) {
        const Value &v = input->toConstant()->value();
        int32_t ret = ToInt32(v.toDouble());
        return MConstant::New(Int32Value(ret));
    }

    return this;
}

MDefinition *
MToDouble::foldsTo(bool useValueNumbers)
{
    if (input()->isConstant()) {
        const Value &v = input()->toConstant()->value();
        if (v.isNumber()) {
            double out = v.toNumber();
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

    if (compareType_ == Compare_Null || compareType_ == Compare_Undefined) {
        JS_ASSERT(op == JSOP_EQ || op == JSOP_STRICTEQ ||
                  op == JSOP_NE || op == JSOP_STRICTNE);

        
        switch (lhs()->type()) {
          case MIRType_Value:
            return false;
          case MIRType_Undefined:
          case MIRType_Null:
            if (lhs()->type() == inputType()) {
                
                *result = (op == JSOP_EQ || op == JSOP_STRICTEQ);
            } else {
                
                
                *result = (op == JSOP_EQ || op == JSOP_STRICTNE);
            }
            return true;
          case MIRType_Object:
            if ((op == JSOP_EQ || op == JSOP_NE) && operandMightEmulateUndefined())
                return false;
            
          case MIRType_Int32:
          case MIRType_Double:
          case MIRType_String:
          case MIRType_Boolean:
            *result = (op == JSOP_NE || op == JSOP_STRICTNE);
            return true;
          default:
            JS_NOT_REACHED("Unexpected type");
            return false;
        }
    }

    if (compareType_ == Compare_Boolean) {
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

    if (compareType_ == Compare_StrictString) {
        JS_ASSERT(op == JSOP_STRICTEQ || op == JSOP_STRICTNE);
        JS_ASSERT(rhs()->type() == MIRType_String);

        switch (lhs()->type()) {
          case MIRType_Value:
            return false;
          case MIRType_Boolean:
          case MIRType_Int32:
          case MIRType_Double:
          case MIRType_Object:
          case MIRType_Null:
          case MIRType_Undefined:
            *result = (op == JSOP_STRICTNE);
            return true;
          case MIRType_String:
            
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
    if (type() != MIRType_Boolean && type() != MIRType_Int32)
        return false;

    MDefinition *left = getOperand(0);
    MDefinition *right = getOperand(1);

    if (!left->isConstant() || !right->isConstant())
        return false;

    Value lhs = left->toConstant()->value();
    Value rhs = right->toConstant()->value();

    
    if (lhs.isString() && rhs.isString()) {
        int32_t comp = 0; 
        if (left != right) {
            if (!CompareStrings(GetIonContext()->cx, lhs.toString(), rhs.toString(), &comp))
                return false;
        }
        
        switch (jsop_) {
          case JSOP_LT:
            *result = (comp < 0);
            break;
          case JSOP_LE:
            *result = (comp <= 0);
            break;
          case JSOP_GT:
            *result = (comp > 0);
            break;
          case JSOP_GE:
            *result = (comp >= 0);
            break;
          case JSOP_STRICTEQ: 
          case JSOP_EQ:
            *result = (comp == 0);
            break;
          case JSOP_STRICTNE: 
          case JSOP_NE:
            *result = (comp != 0);
            break;
          default:
            JS_NOT_REACHED("Unexpected op.");
            return false;
        }

        return true;
    }

    if (compareType_ == Compare_UInt32) {
        uint32_t lhsUint = uint32_t(lhs.toInt32());
        uint32_t rhsUint = uint32_t(rhs.toInt32());

        switch (jsop_) {
          case JSOP_LT:
            *result = (lhsUint < rhsUint);
            break;
          case JSOP_LE:
            *result = (lhsUint <= rhsUint);
            break;
          case JSOP_GT:
            *result = (lhsUint > rhsUint);
            break;
          case JSOP_GE:
            *result = (lhsUint >= rhsUint);
            break;
          case JSOP_EQ:
          case JSOP_STRICTEQ:
            *result = (lhsUint == rhsUint);
            break;
          case JSOP_NE:
          case JSOP_STRICTNE:
            *result = (lhsUint != rhsUint);
            break;
          default:
            JS_NOT_REACHED("Unexpected op.");
            return false;
        }

        return true;
    }

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

    if (tryFold(&result) || evaluateConstantOperands(&result)) {
        if (type() == MIRType_Int32)
            return MConstant::New(Int32Value(result));

        JS_ASSERT(type() == MIRType_Boolean);
        return MConstant::New(BooleanValue(result));
    }

    return this;
}

void
MNot::infer(JSContext *cx)
{
    JS_ASSERT(operandMightEmulateUndefined());

    if (!MaybeEmulatesUndefined(cx, getOperand(0)))
        markOperandCantEmulateUndefined();
}

MDefinition *
MNot::foldsTo(bool useValueNumbers)
{
    
    if (operand()->isConstant()) {
        const Value &v = operand()->toConstant()->value();
        if (type() == MIRType_Int32)
            return MConstant::New(Int32Value(!ToBoolean(v)));

        
        return MConstant::New(BooleanValue(!ToBoolean(v)));
    }

    
    if (operand()->type() == MIRType_Undefined || operand()->type() == MIRType_Null)
        return MConstant::New(BooleanValue(true));

    
    if (operand()->type() == MIRType_Object && !operandMightEmulateUndefined())
        return MConstant::New(BooleanValue(false));

    return this;
}

bool
MBoundsCheckLower::fallible()
{
    return !range() || range()->lower() < minimum_;
}

void
MBeta::printOpcode(FILE *fp)
{
    PrintOpcodeName(fp, op());
    fprintf(fp, " ");
    getOperand(0)->printName(fp);
    fprintf(fp, " ");

    Sprinter sp(GetIonContext()->cx);
    sp.init();
    comparison_->print(sp);
    fprintf(fp, "%s", sp.string());
}

void
MBeta::computeRange()
{
    bool emptyRange = false;

    Range *range = Range::intersect(val_->range(), comparison_, &emptyRange);
    if (emptyRange) {
        IonSpew(IonSpew_Range, "Marking block for inst %d unexitable", id());
        block()->setEarlyAbort();
    } else {
        setRange(range);
    }
}

bool
MNewObject::shouldUseVM() const
{
    return templateObject()->hasSingletonType() ||
           templateObject()->hasDynamicSlots();
}

bool
MNewArray::shouldUseVM() const
{
    JS_ASSERT(count() < JSObject::NELEMENTS_LIMIT);

    size_t maxArraySlots =
        gc::GetGCKindSlots(gc::FINALIZE_OBJECT_LAST) - ObjectElements::VALUES_PER_HEADER;

    
    
    
    bool allocating = isAllocating() && count() > maxArraySlots;

    return templateObject()->hasSingletonType() || allocating;
}

bool
MLoadFixedSlot::mightAlias(MDefinition *store)
{
    if (store->isStoreFixedSlot() && store->toStoreFixedSlot()->slot() != slot())
        return false;
    return true;
}

bool
MLoadSlot::mightAlias(MDefinition *store)
{
    if (store->isStoreSlot() && store->toStoreSlot()->slot() != slot())
        return false;
    return true;
}

void
InlinePropertyTable::trimTo(AutoObjectVector &targets, Vector<bool> &choiceSet)
{
    for (size_t i = 0; i < targets.length(); i++) {
        
        if (choiceSet[i])
            continue;

        JSFunction *target = targets[i]->toFunction();

        
        size_t j = 0;
        while (j < numEntries()) {
            if (entries_[j]->func == target)
                entries_.erase(&entries_[j]);
            else
                j++;
        }
    }
}

void
InlinePropertyTable::trimToAndMaybePatchTargets(AutoObjectVector &targets,
                                                AutoObjectVector &originals)
{
    IonSpew(IonSpew_Inlining, "Got inlineable property cache with %d cases",
            (int)numEntries());

    size_t i = 0;
    while (i < numEntries()) {
        bool foundFunc = false;
        
        
        for (size_t j = 0; j < originals.length(); j++) {
            if (entries_[i]->func == originals[j]) {
                if (entries_[i]->func != targets[j])
                    entries_[i] = new Entry(entries_[i]->typeObj, targets[j]->toFunction());
                foundFunc = true;
                break;
            }
        }
        if (!foundFunc)
            entries_.erase(&(entries_[i]));
        else
            i++;
    }

    IonSpew(IonSpew_Inlining, "%d inlineable cases left after trimming to %d targets",
            (int)numEntries(), (int)targets.length());
}

bool
InlinePropertyTable::hasFunction(JSFunction *func) const
{
    for (size_t i = 0; i < numEntries(); i++) {
        if (entries_[i]->func == func)
            return true;
    }
    return false;
}

types::StackTypeSet *
InlinePropertyTable::buildTypeSetForFunction(JSFunction *func) const
{
    LifoAlloc *alloc = GetIonContext()->temp->lifoAlloc();
    types::StackTypeSet *types = alloc->new_<types::StackTypeSet>();
    if (!types)
        return NULL;
    for (size_t i = 0; i < numEntries(); i++) {
        if (entries_[i]->func == func) {
            if (!types->addObject(types::Type::ObjectType(entries_[i]->typeObj).objectKey(), alloc))
                return NULL;
        }
    }
    return types;
}

bool
MInArray::needsNegativeIntCheck() const
{
    return !index()->range() || index()->range()->lower() < 0;
}

void *
MLoadTypedArrayElementStatic::base() const
{
    return TypedArray::viewData(typedArray_);
}

size_t
MLoadTypedArrayElementStatic::length() const
{
    return TypedArray::byteLength(typedArray_);
}

void *
MStoreTypedArrayElementStatic::base() const
{
    return TypedArray::viewData(typedArray_);
}

size_t
MStoreTypedArrayElementStatic::length() const
{
    return TypedArray::byteLength(typedArray_);
}

bool
MGetPropertyPolymorphic::mightAlias(MDefinition *store)
{
    
    

    if (!store->isStoreFixedSlot() && !store->isStoreSlot())
        return true;

    for (size_t i = 0; i < numShapes(); i++) {
        Shape *shape = this->shape(i);
        if (shape->slot() < shape->numFixedSlots()) {
            
            uint32_t slot = shape->slot();
            if (store->isStoreFixedSlot() && store->toStoreFixedSlot()->slot() != slot)
                continue;
            if (store->isStoreSlot())
                continue;
        } else {
            
            uint32_t slot = shape->slot() - shape->numFixedSlots();
            if (store->isStoreSlot() && store->toStoreSlot()->slot() != slot)
                continue;
            if (store->isStoreFixedSlot())
                continue;
        }

        return true;
    }

    return false;
}

MDefinition *
MAsmJSUnsignedToDouble::foldsTo(bool useValueNumbers)
{
    if (input()->isConstant()) {
        const Value &v = input()->toConstant()->value();
        if (v.isInt32())
            return MConstant::New(DoubleValue(uint32_t(v.toInt32())));
    }

    return this;
}

MAsmJSCall *
MAsmJSCall::New(Callee callee, const Args &args, MIRType resultType, size_t spIncrement)
{
    MAsmJSCall *call = new MAsmJSCall;
    call->spIncrement_ = spIncrement;
    call->callee_ = callee;
    call->setResultType(resultType);

    call->numArgs_ = args.length();
    call->argRegs_ = (AnyRegister *)GetIonContext()->temp->allocate(call->numArgs_ * sizeof(AnyRegister));
    if (!call->argRegs_)
        return NULL;
    for (size_t i = 0; i < call->numArgs_; i++)
        call->argRegs_[i] = args[i].reg;

    call->numOperands_ = call->numArgs_ + (callee.which() == Callee::Dynamic ? 1 : 0);
    call->operands_ = (MUse *)GetIonContext()->temp->allocate(call->numOperands_ * sizeof(MUse));
    if (!call->operands_)
        return NULL;
    for (size_t i = 0; i < call->numArgs_; i++)
        call->setOperand(i, args[i].def);
    if (callee.which() == Callee::Dynamic)
        call->setOperand(call->numArgs_, callee.dynamic());

    return call;
}

bool
ion::ElementAccessIsDenseNative(MDefinition *obj, MDefinition *id)
{
    if (obj->mightBeType(MIRType_String))
        return false;

    if (id->type() != MIRType_Int32 && id->type() != MIRType_Double)
        return false;

    types::StackTypeSet *types = obj->resultTypeSet();
    if (!types)
        return false;

    Class *clasp = types->getKnownClass();
    return clasp && clasp->isNative();
}

bool
ion::ElementAccessIsTypedArray(MDefinition *obj, MDefinition *id, int *arrayType)
{
    if (obj->mightBeType(MIRType_String))
        return false;

    if (id->type() != MIRType_Int32 && id->type() != MIRType_Double)
        return false;

    types::StackTypeSet *types = obj->resultTypeSet();
    if (!types)
        return false;

    *arrayType = types->getTypedArrayType();
    return *arrayType != TypedArray::TYPE_MAX;
}

bool
ion::ElementAccessIsPacked(JSContext *cx, MDefinition *obj)
{
    types::StackTypeSet *types = obj->resultTypeSet();
    return types && !types->hasObjectFlags(cx, types::OBJECT_FLAG_NON_PACKED);
}

bool
ion::ElementAccessHasExtraIndexedProperty(JSContext *cx, MDefinition *obj)
{
    types::StackTypeSet *types = obj->resultTypeSet();

    if (!types || types->hasObjectFlags(cx, types::OBJECT_FLAG_LENGTH_OVERFLOW))
        return true;

    return types::TypeCanHaveExtraIndexedProperties(cx, types);
}

MIRType
ion::DenseNativeElementType(JSContext *cx, MDefinition *obj)
{
    types::StackTypeSet *types = obj->resultTypeSet();
    MIRType elementType = MIRType_None;
    unsigned count = types->getObjectCount();

    for (unsigned i = 0; i < count; i++) {
        if (types::TypeObject *object = types->getTypeOrSingleObject(cx, i)) {
            if (object->unknownProperties())
                return MIRType_None;

            types::HeapTypeSet *elementTypes = object->getProperty(cx, JSID_VOID, false);
            if (!elementTypes)
                return MIRType_None;

            MIRType type = MIRTypeFromValueType(elementTypes->getKnownTypeTag(cx));
            if (type == MIRType_None)
                return MIRType_None;

            if (elementType == MIRType_None)
                elementType = type;
            else if (elementType != type)
                return MIRType_None;
        }
    }

    return elementType;
}

bool
ion::PropertyReadNeedsTypeBarrier(JSContext *cx, types::TypeObject *object, PropertyName *name,
                                  types::StackTypeSet *observed, bool updateObserved)
{
    
    
    
    
    

    if (object->unknownProperties())
        return true;

    jsid id = name ? types::IdToTypeId(NameToId(name)) : JSID_VOID;

    
    
    if (updateObserved && observed->empty() && observed->noConstraints() && !JSID_IS_VOID(id)) {
        JSObject *obj = object->singleton ? object->singleton : object->proto;

        while (obj) {
            if (!obj->isNative())
                break;

            Value v;
            if (HasDataProperty(cx, obj, id, &v)) {
                if (v.isUndefined())
                    break;
                observed->addType(cx, types::GetValueType(cx, v));
            }

            obj = obj->getProto();
        }
    }

    types::HeapTypeSet *property = object->getProperty(cx, id, false);
    if (!property)
        return true;

    
    
    
    
    if (!property->hasPropagatedProperty())
        object->getFromPrototypes(cx, id, property);

    if (!TypeSetIncludes(observed, MIRType_Value, property))
        return true;

    
    
    
    
    if (name && object->singleton && object->singleton->isNative()) {
        Shape *shape = object->singleton->nativeLookup(cx, name);
        if (shape &&
            shape->hasDefaultGetter() &&
            object->singleton->nativeGetSlot(shape->slot()).isUndefined())
        {
            return true;
        }
    }

    property->addFreeze(cx);
    return false;
}

bool
ion::PropertyReadNeedsTypeBarrier(JSContext *cx, MDefinition *obj, PropertyName *name,
                                  types::StackTypeSet *observed)
{
    if (observed->unknown())
        return false;

    types::TypeSet *types = obj->resultTypeSet();
    if (!types || types->unknownObject())
        return true;

    bool updateObserved = types->getObjectCount() == 1;
    for (size_t i = 0; i < types->getObjectCount(); i++) {
        types::TypeObject *object = types->getTypeOrSingleObject(cx, i);
        if (object && PropertyReadNeedsTypeBarrier(cx, object, name, observed, updateObserved))
            return true;
    }

    return false;
}

bool
ion::PropertyReadIsIdempotent(JSContext *cx, MDefinition *obj, PropertyName *name)
{
    

    jsid id = types::IdToTypeId(NameToId(name));

    types::TypeSet *types = obj->resultTypeSet();
    if (!types || types->unknownObject())
        return false;

    for (size_t i = 0; i < types->getObjectCount(); i++) {
        if (types::TypeObject *object = types->getTypeOrSingleObject(cx, i)) {
            if (object->unknownProperties())
                return false;

            
            types::HeapTypeSet *property = object->getProperty(cx, id, false);
            if (!property || property->isOwnProperty(cx, object, true))
                return false;
        }
    }

    return true;
}

void
ion::AddObjectsForPropertyRead(JSContext *cx, MDefinition *obj, PropertyName *name,
                               types::StackTypeSet *observed)
{
    
    

    JS_ASSERT(observed->noConstraints());

    types::StackTypeSet *types = obj->resultTypeSet();
    if (!types || types->unknownObject()) {
        observed->addType(cx, types::Type::AnyObjectType());
        return;
    }

    jsid id = name ? types::IdToTypeId(NameToId(name)) : JSID_VOID;

    for (size_t i = 0; i < types->getObjectCount(); i++) {
        types::TypeObject *object = types->getTypeOrSingleObject(cx, i);
        if (!object)
            continue;

        if (object->unknownProperties()) {
            observed->addType(cx, types::Type::AnyObjectType());
            return;
        }

        types::HeapTypeSet *property = object->getProperty(cx, id, false);
        if (property->unknownObject()) {
            observed->addType(cx, types::Type::AnyObjectType());
            return;
        }

        for (size_t i = 0; i < property->getObjectCount(); i++) {
            if (types::TypeObject *object = property->getTypeObject(i))
                observed->addType(cx, types::Type::ObjectType(object));
            else if (JSObject *object = property->getSingleObject(i))
                observed->addType(cx, types::Type::ObjectType(object));
        }
    }
}

static bool
TryAddTypeBarrierForWrite(JSContext *cx, MBasicBlock *current, types::StackTypeSet *objTypes,
                          jsid id, MDefinition **pvalue)
{
    
    
    

    
    
    
    types::HeapTypeSet *aggregateProperty = NULL;

    for (size_t i = 0; i < objTypes->getObjectCount(); i++) {
        types::TypeObject *object = objTypes->getTypeOrSingleObject(cx, i);
        if (!object)
            continue;

        if (object->unknownProperties())
            return false;

        types::HeapTypeSet *property = object->getProperty(cx, id, false);
        if (!property)
            return false;

        if (TypeSetIncludes(property, (*pvalue)->type(), (*pvalue)->resultTypeSet()))
            return false;

        
        
        
        property->addFreeze(cx);

        if (aggregateProperty) {
            if (!aggregateProperty->isSubset(property) || !property->isSubset(aggregateProperty))
                return false;
        } else {
            aggregateProperty = property;
        }
    }

    JS_ASSERT(aggregateProperty);

    MIRType propertyType = MIRTypeFromValueType(aggregateProperty->getKnownTypeTag(cx));
    switch (propertyType) {
      case MIRType_Boolean:
      case MIRType_Int32:
      case MIRType_Double:
      case MIRType_String: {
        
        
        if ((*pvalue)->type() != MIRType_Value) {
            
            
            JS_ASSERT((*pvalue)->type() != propertyType);
            return false;
        }
        MInstruction *ins = MUnbox::New(*pvalue, propertyType, MUnbox::Fallible);
        current->add(ins);
        *pvalue = ins;
        return true;
      }
      default:;
    }

    if ((*pvalue)->type() != MIRType_Value)
        return false;

    types::StackTypeSet *types = aggregateProperty->clone(GetIonContext()->temp->lifoAlloc());
    if (!types)
        return false;

    MInstruction *ins = MMonitorTypes::New(*pvalue, types);
    current->add(ins);
    return true;
}

static MInstruction *
AddTypeGuard(MBasicBlock *current, MDefinition *obj, types::TypeObject *typeObject,
             bool bailOnEquality)
{
    MGuardObjectType *guard = MGuardObjectType::New(obj, typeObject, bailOnEquality);
    current->add(guard);

    
    guard->setNotMovable();

    return guard;
}

bool
ion::PropertyWriteNeedsTypeBarrier(JSContext *cx, MBasicBlock *current, MDefinition **pobj,
                                   PropertyName *name, MDefinition **pvalue, bool canModify)
{
    
    
    
    
    

    types::StackTypeSet *types = (*pobj)->resultTypeSet();
    if (!types || types->unknownObject())
        return true;

    jsid id = name ? types::IdToTypeId(NameToId(name)) : JSID_VOID;

    
    
    
    

    bool success = true;
    for (size_t i = 0; i < types->getObjectCount(); i++) {
        types::TypeObject *object = types->getTypeOrSingleObject(cx, i);
        if (!object || object->unknownProperties())
            continue;

        types::HeapTypeSet *property = object->getProperty(cx, id, false);
        if (!property) {
            success = false;
            break;
        }
        if (!TypeSetIncludes(property, (*pvalue)->type(), (*pvalue)->resultTypeSet())) {
            
            
            
            
            if (!canModify)
                return true;
            success = TryAddTypeBarrierForWrite(cx, current, types, id, pvalue);
            break;
        }
    }

    if (success)
        return false;

    
    
    

    if (types->getObjectCount() <= 1)
        return true;

    types::TypeObject *excluded = NULL;
    for (size_t i = 0; i < types->getObjectCount(); i++) {
        types::TypeObject *object = types->getTypeOrSingleObject(cx, i);
        if (!object || object->unknownProperties())
            continue;

        types::HeapTypeSet *property = object->getProperty(cx, id, false);
        if (!property)
            return true;

        if (TypeSetIncludes(property, (*pvalue)->type(), (*pvalue)->resultTypeSet()))
            continue;

        if (!property->empty() || excluded)
            return true;
        excluded = object;
    }

    JS_ASSERT(excluded);

    *pobj = AddTypeGuard(current, *pobj, excluded,  true);
    return false;
}
