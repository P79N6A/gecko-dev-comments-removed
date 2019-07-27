





#include "jit/MIR.h"

#include "mozilla/FloatingPoint.h"
#include "mozilla/MathAlgorithms.h"

#include <ctype.h>

#include "jslibmath.h"
#include "jsstr.h"

#include "jit/BaselineInspector.h"
#include "jit/IonBuilder.h"
#include "jit/JitSpewer.h"
#include "jit/MIRGraph.h"
#include "jit/RangeAnalysis.h"

#include "jsatominlines.h"
#include "jsinferinlines.h"
#include "jsobjinlines.h"

using namespace js;
using namespace js::jit;

using mozilla::NumbersAreIdentical;
using mozilla::IsFloat32Representable;
using mozilla::Maybe;
using mozilla::DebugOnly;

#ifdef DEBUG
size_t MUse::index() const
{
    return consumer()->indexOf(this);
}
#endif

template<size_t Op> static void
ConvertDefinitionToDouble(TempAllocator &alloc, MDefinition *def, MInstruction *consumer)
{
    MInstruction *replace = MToDouble::New(alloc, def);
    consumer->replaceOperand(Op, replace);
    consumer->block()->insertBefore(consumer, replace);
}

static bool
CheckUsesAreFloat32Consumers(MInstruction *ins)
{
    bool allConsumerUses = true;
    for (MUseDefIterator use(ins); allConsumerUses && use; use++)
        allConsumerUses &= use.def()->canConsumeFloat32(use.use());
    return allConsumerUses;
}

void
MDefinition::PrintOpcodeName(FILE *fp, MDefinition::Opcode op)
{
    static const char * const names[] =
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

static MConstant *
EvaluateConstantOperands(TempAllocator &alloc, MBinaryInstruction *ins, bool *ptypeChange = nullptr)
{
    MDefinition *left = ins->getOperand(0);
    MDefinition *right = ins->getOperand(1);

    if (!left->isConstant() || !right->isConstant())
        return nullptr;

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
        ret = Int32Value(uint32_t(lhs.toInt32()) << (rhs.toInt32() & 0x1F));
        break;
      case MDefinition::Op_Rsh:
        ret = Int32Value(lhs.toInt32() >> (rhs.toInt32() & 0x1F));
        break;
      case MDefinition::Op_Ursh:
        ret.setNumber(uint32_t(lhs.toInt32()) >> (rhs.toInt32() & 0x1F));
        break;
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
        MOZ_CRASH("NYI");
    }

    
    
    if (ins->type() == MIRType_Double && ret.isInt32())
        ret.setDouble(ret.toNumber());

    if (ins->type() != MIRTypeFromValue(ret)) {
        if (ptypeChange)
            *ptypeChange = true;
        return nullptr;
    }

    return MConstant::New(alloc, ret);
}

static MMul *
EvaluateExactReciprocal(TempAllocator &alloc, MDiv *ins)
{
    
    if (!IsFloatingPointType(ins->type()))
        return nullptr;

    MDefinition *left = ins->getOperand(0);
    MDefinition *right = ins->getOperand(1);

    if (!right->isConstant())
        return nullptr;

    Value rhs = right->toConstant()->value();

    int32_t num;
    if (!mozilla::NumberIsInt32(rhs.toNumber(), &num))
        return nullptr;

    
    if (mozilla::Abs(num) & (mozilla::Abs(num) - 1))
        return nullptr;

    Value ret;
    ret.setDouble(1.0 / (double) num);
    MConstant *foldedRhs = MConstant::New(alloc, ret);
    foldedRhs->setResultType(ins->type());
    ins->block()->insertBefore(ins, foldedRhs);

    return MMul::New(alloc, left, foldedRhs, ins->type());
}

void
MDefinition::printName(FILE *fp) const
{
    PrintOpcodeName(fp, op());
    fprintf(fp, "%u", id());
}

HashNumber
MDefinition::addU32ToHash(HashNumber hash, uint32_t data)
{
    return data + (hash << 6) + (hash << 16) - hash;
}

HashNumber
MDefinition::valueHash() const
{
    HashNumber out = op();
    for (size_t i = 0, e = numOperands(); i < e; i++)
        out = addU32ToHash(out, getOperand(i)->id());
    if (MDefinition *dep = dependency())
        out = addU32ToHash(out, dep->id());
    return out;
}

bool
MDefinition::congruentIfOperandsEqual(const MDefinition *ins) const
{
    if (op() != ins->op())
        return false;

    if (type() != ins->type())
        return false;

    if (isEffectful() || ins->isEffectful())
        return false;

    if (numOperands() != ins->numOperands())
        return false;

    for (size_t i = 0, e = numOperands(); i < e; i++) {
        if (getOperand(i) != ins->getOperand(i))
            return false;
    }

    return true;
}

MDefinition *
MDefinition::foldsTo(TempAllocator &alloc)
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

void
MInstruction::setResumePoint(MResumePoint *resumePoint)
{
    JS_ASSERT(!resumePoint_);
    resumePoint_ = resumePoint;
    resumePoint_->setInstruction(this);
}

void
MInstruction::stealResumePoint(MInstruction *ins)
{
    MOZ_ASSERT(ins->resumePoint_->instruction() == ins);
    resumePoint_ = ins->resumePoint_;
    ins->resumePoint_ = nullptr;
    resumePoint_->replaceInstruction(this);
}

static bool
MaybeEmulatesUndefined(MDefinition *op)
{
    if (!op->mightBeType(MIRType_Object))
        return false;

    types::TemporaryTypeSet *types = op->resultTypeSet();
    if (!types)
        return true;

    return types->maybeEmulatesUndefined();
}

static bool
MaybeCallable(MDefinition *op)
{
    if (!op->mightBeType(MIRType_Object))
        return false;

    types::TemporaryTypeSet *types = op->resultTypeSet();
    if (!types)
        return true;

    return types->maybeCallable();
}

MTest *
MTest::New(TempAllocator &alloc, MDefinition *ins, MBasicBlock *ifTrue, MBasicBlock *ifFalse)
{
    return new(alloc) MTest(ins, ifTrue, ifFalse);
}

void
MTest::cacheOperandMightEmulateUndefined()
{
    JS_ASSERT(operandMightEmulateUndefined());

    if (!MaybeEmulatesUndefined(getOperand(0)))
        markOperandCantEmulateUndefined();
}

MDefinition *
MTest::foldsTo(TempAllocator &alloc)
{
    MDefinition *op = getOperand(0);

    if (op->isNot())
        return MTest::New(alloc, op->toNot()->input(), ifFalse(), ifTrue());

    return this;
}

void
MTest::filtersUndefinedOrNull(bool trueBranch, MDefinition **subject, bool *filtersUndefined,
                              bool *filtersNull)
{
    MDefinition *ins = getOperand(0);
    if (ins->isCompare()) {
        ins->toCompare()->filtersUndefinedOrNull(trueBranch, subject, filtersUndefined, filtersNull);
        return;
    }

    if (!trueBranch && ins->isNot()) {
        *subject = ins->getOperand(0);
        *filtersUndefined = *filtersNull = true;
        return;
    }

    if (trueBranch) {
        *subject = ins;
        *filtersUndefined = *filtersNull = true;
        return;
    }

    *filtersUndefined = *filtersNull = false;
    *subject = nullptr;
}

void
MDefinition::printOpcode(FILE *fp) const
{
    PrintOpcodeName(fp, op());
    for (size_t j = 0, e = numOperands(); j < e; j++) {
        fprintf(fp, " ");
        getOperand(j)->printName(fp);
    }
}

void
MDefinition::dump(FILE *fp) const
{
    printName(fp);
    fprintf(fp, " = ");
    printOpcode(fp);
    fprintf(fp, "\n");

    if (isInstruction()) {
        if (MResumePoint *resume = toInstruction()->resumePoint())
            resume->dump(fp);
    }
}

void
MDefinition::dump() const
{
    dump(stderr);
}

void
MDefinition::dumpLocation(FILE *fp) const
{
    MResumePoint *rp = nullptr;
    const char *linkWord = nullptr;
    if (isInstruction() && toInstruction()->resumePoint()) {
        rp = toInstruction()->resumePoint();
        linkWord = "at";
    } else {
        rp = block()->entryResumePoint();
        linkWord = "after";
    }

    while (rp) {
        JSScript *script = rp->block()->info().script();
        uint32_t lineno = PCToLineNumber(rp->block()->info().script(), rp->pc());
        fprintf(fp, "  %s %s:%d\n", linkWord, script->filename(), lineno);
        rp = rp->caller();
        linkWord = "in";
    }
}

void
MDefinition::dumpLocation() const
{
    dumpLocation(stderr);
}

#ifdef DEBUG
size_t
MDefinition::warmUpCounter() const
{
    size_t count = 0;
    for (MUseIterator i(uses_.begin()); i != uses_.end(); i++)
        count++;
    return count;
}

size_t
MDefinition::defWarmUpCounter() const
{
    size_t count = 0;
    for (MUseIterator i(uses_.begin()); i != uses_.end(); i++)
        if ((*i)->consumer()->isDefinition())
            count++;
    return count;
}
#endif

bool
MDefinition::hasOneUse() const
{
    MUseIterator i(uses_.begin());
    if (i == uses_.end())
        return false;
    i++;
    return i == uses_.end();
}

bool
MDefinition::hasOneDefUse() const
{
    bool hasOneDefUse = false;
    for (MUseIterator i(uses_.begin()); i != uses_.end(); i++) {
        if (!(*i)->consumer()->isDefinition())
            continue;

        
        if (hasOneDefUse)
            return false;

        
        hasOneDefUse = true;
    }

    return hasOneDefUse;
}

bool
MDefinition::hasDefUses() const
{
    for (MUseIterator i(uses_.begin()); i != uses_.end(); i++) {
        if ((*i)->consumer()->isDefinition())
            return true;
    }

    return false;
}

bool
MDefinition::hasLiveDefUses() const
{
    for (MUseIterator i(uses_.begin()); i != uses_.end(); i++) {
        MNode *ins = (*i)->consumer();
        if (ins->isDefinition()) {
            if (!ins->toDefinition()->isRecoveredOnBailout())
                return true;
        } else {
            MOZ_ASSERT(ins->isResumePoint());
            if (ins->toResumePoint()->isObservableOperand(*i))
                return true;
        }
    }

    return false;
}

void
MDefinition::replaceAllUsesWith(MDefinition *dom)
{
    for (size_t i = 0, e = numOperands(); i < e; ++i)
        getOperand(i)->setUseRemovedUnchecked();

    justReplaceAllUsesWith(dom);
}

void
MDefinition::justReplaceAllUsesWith(MDefinition *dom)
{
    JS_ASSERT(dom != nullptr);
    JS_ASSERT(dom != this);

    for (MUseIterator i(usesBegin()); i != usesEnd(); i++)
        i->setProducerUnchecked(dom);
    dom->uses_.takeElements(uses_);
}

bool
MDefinition::emptyResultTypeSet() const
{
    return resultTypeSet() && resultTypeSet()->empty();
}

MConstant *
MConstant::New(TempAllocator &alloc, const Value &v, types::CompilerConstraintList *constraints)
{
    return new(alloc) MConstant(v, constraints);
}

MConstant *
MConstant::NewAsmJS(TempAllocator &alloc, const Value &v, MIRType type)
{
    JS_ASSERT(!IsSimdType(type));
    MConstant *constant = new(alloc) MConstant(v, nullptr);
    constant->setResultType(type);
    return constant;
}

MConstant *
MConstant::NewConstraintlessObject(TempAllocator &alloc, JSObject *v)
{
    return new(alloc) MConstant(v);
}

types::TemporaryTypeSet *
jit::MakeSingletonTypeSet(types::CompilerConstraintList *constraints, JSObject *obj)
{
    
    
    
    
    JS_ASSERT(constraints);
    types::TypeObjectKey *objType = types::TypeObjectKey::get(obj);
    objType->hasFlags(constraints, types::OBJECT_FLAG_UNKNOWN_PROPERTIES);

    LifoAlloc *alloc = GetIonContext()->temp->lifoAlloc();
    return alloc->new_<types::TemporaryTypeSet>(alloc, types::Type::ObjectType(obj));
}

MConstant::MConstant(const js::Value &vp, types::CompilerConstraintList *constraints)
  : value_(vp)
{
    setResultType(MIRTypeFromValue(vp));
    if (vp.isObject()) {
        
        
        setResultTypeSet(MakeSingletonTypeSet(constraints, &vp.toObject()));
    }

    setMovable();
}

MConstant::MConstant(JSObject *obj)
  : value_(ObjectValue(*obj))
{
    setResultType(MIRType_Object);
    setMovable();
}

HashNumber
MConstant::valueHash() const
{
    
    
    
    
    uint64_t bits = JSVAL_TO_IMPL(value_).asBits;
    return (HashNumber)bits ^ (HashNumber)(bits >> 32);
}

bool
MConstant::congruentTo(const MDefinition *ins) const
{
    if (!ins->isConstant())
        return false;
    return ins->toConstant()->value() == value();
}

void
MConstant::printOpcode(FILE *fp) const
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
      case MIRType_Float32:
      {
        float val = value().toDouble();
        fprintf(fp, "%f", val);
        break;
      }
      case MIRType_Object:
        if (value().toObject().is<JSFunction>()) {
            JSFunction *fun = &value().toObject().as<JSFunction>();
            if (fun->displayAtom()) {
                fputs("function ", fp);
                FileEscapedString(fp, fun->displayAtom(), 0);
            } else {
                fputs("unnamed function", fp);
            }
            if (fun->hasScript()) {
                JSScript *script = fun->nonLazyScript();
                fprintf(fp, " (%s:%d)",
                        script->filename() ? script->filename() : "", (int) script->lineno());
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
      case MIRType_MagicOptimizedArguments:
        fprintf(fp, "magic lazyargs");
        break;
      case MIRType_MagicHole:
        fprintf(fp, "magic hole");
        break;
      case MIRType_MagicIsConstructing:
        fprintf(fp, "magic is-constructing");
        break;
      case MIRType_MagicOptimizedOut:
        fprintf(fp, "magic optimized-out");
        break;
      default:
        MOZ_CRASH("unexpected type");
    }
}

bool
MConstant::canProduceFloat32() const
{
    if (!IsNumberType(type()))
        return false;

    if (type() == MIRType_Int32)
        return IsFloat32Representable(static_cast<double>(value_.toInt32()));
    if (type() == MIRType_Double)
        return IsFloat32Representable(value_.toDouble());
    return true;
}

MDefinition*
MSimdValueX4::foldsTo(TempAllocator &alloc)
{
    DebugOnly<MIRType> scalarType = SimdTypeToScalarType(type());
    for (size_t i = 0; i < 4; ++i) {
        MDefinition *op = getOperand(i);
        if (!op->isConstant())
            return this;
        JS_ASSERT(op->type() == scalarType);
    }

    SimdConstant cst;
    switch (type()) {
      case MIRType_Int32x4: {
        int32_t a[4];
        for (size_t i = 0; i < 4; ++i)
            a[i] = getOperand(i)->toConstant()->value().toInt32();
        cst = SimdConstant::CreateX4(a);
        break;
      }
      case MIRType_Float32x4: {
        float a[4];
        for (size_t i = 0; i < 4; ++i)
            a[i] = getOperand(i)->toConstant()->value().toNumber();
        cst = SimdConstant::CreateX4(a);
        break;
      }
      default: MOZ_CRASH("unexpected type in MSimdValueX4::foldsTo");
    }

    return MSimdConstant::New(alloc, cst, type());
}

MDefinition*
MSimdSplatX4::foldsTo(TempAllocator &alloc)
{
    DebugOnly<MIRType> scalarType = SimdTypeToScalarType(type());
    MDefinition *op = getOperand(0);
    if (!op->isConstant())
        return this;
    JS_ASSERT(op->type() == scalarType);

    SimdConstant cst;
    switch (type()) {
      case MIRType_Int32x4: {
        int32_t a[4];
        int32_t v = getOperand(0)->toConstant()->value().toInt32();
        for (size_t i = 0; i < 4; ++i)
            a[i] = v;
        cst = SimdConstant::CreateX4(a);
        break;
      }
      case MIRType_Float32x4: {
        float a[4];
        float v = getOperand(0)->toConstant()->value().toNumber();
        for (size_t i = 0; i < 4; ++i)
            a[i] = v;
        cst = SimdConstant::CreateX4(a);
        break;
      }
      default: MOZ_CRASH("unexpected type in MSimdSplatX4::foldsTo");
    }

    return MSimdConstant::New(alloc, cst, type());
}

MCloneLiteral *
MCloneLiteral::New(TempAllocator &alloc, MDefinition *obj)
{
    return new(alloc) MCloneLiteral(obj);
}

void
MControlInstruction::printOpcode(FILE *fp) const
{
    MDefinition::printOpcode(fp);
    for (size_t j = 0; j < numSuccessors(); j++)
        fprintf(fp, " block%u", getSuccessor(j)->id());
}

void
MCompare::printOpcode(FILE *fp) const
{
    MDefinition::printOpcode(fp);
    fprintf(fp, " %s", js_CodeName[jsop()]);
}

void
MConstantElements::printOpcode(FILE *fp) const
{
    PrintOpcodeName(fp, op());
    fprintf(fp, " %p", value());
}

void
MLoadTypedArrayElement::printOpcode(FILE *fp) const
{
    MDefinition::printOpcode(fp);
    fprintf(fp, " %s", ScalarTypeDescr::typeName(arrayType()));
}

void
MAssertRange::printOpcode(FILE *fp) const
{
    MDefinition::printOpcode(fp);
    Sprinter sp(GetIonContext()->cx);
    sp.init();
    assertedRange()->print(sp);
    fprintf(fp, " %s", sp.string());
}

const char *
MMathFunction::FunctionName(Function function)
{
    switch (function) {
      case Log:    return "Log";
      case Sin:    return "Sin";
      case Cos:    return "Cos";
      case Exp:    return "Exp";
      case Tan:    return "Tan";
      case ACos:   return "ACos";
      case ASin:   return "ASin";
      case ATan:   return "ATan";
      case Log10:  return "Log10";
      case Log2:   return "Log2";
      case Log1P:  return "Log1P";
      case ExpM1:  return "ExpM1";
      case CosH:   return "CosH";
      case SinH:   return "SinH";
      case TanH:   return "TanH";
      case ACosH:  return "ACosH";
      case ASinH:  return "ASinH";
      case ATanH:  return "ATanH";
      case Sign:   return "Sign";
      case Trunc:  return "Trunc";
      case Cbrt:   return "Cbrt";
      case Floor:  return "Floor";
      case Ceil:   return "Ceil";
      case Round:  return "Round";
      default:
        MOZ_CRASH("Unknown math function");
    }
}

void
MMathFunction::printOpcode(FILE *fp) const
{
    MDefinition::printOpcode(fp);
    fprintf(fp, " %s", FunctionName(function()));
}

MParameter *
MParameter::New(TempAllocator &alloc, int32_t index, types::TemporaryTypeSet *types)
{
    return new(alloc) MParameter(index, types);
}

void
MParameter::printOpcode(FILE *fp) const
{
    PrintOpcodeName(fp, op());
    if (index() == THIS_SLOT)
        fprintf(fp, " THIS_SLOT");
    else
        fprintf(fp, " %d", index());
}

HashNumber
MParameter::valueHash() const
{
    HashNumber hash = MDefinition::valueHash();
    hash = addU32ToHash(hash, index_);
    return hash;
}

bool
MParameter::congruentTo(const MDefinition *ins) const
{
    if (!ins->isParameter())
        return false;

    return ins->toParameter()->index() == index_;
}

MCall *
MCall::New(TempAllocator &alloc, JSFunction *target, size_t maxArgc, size_t numActualArgs,
           bool construct, bool isDOMCall)
{
    JS_ASSERT(maxArgc >= numActualArgs);
    MCall *ins;
    if (isDOMCall) {
        JS_ASSERT(!construct);
        ins = new(alloc) MCallDOMNative(target, numActualArgs);
    } else {
        ins = new(alloc) MCall(target, numActualArgs, construct);
    }
    if (!ins->init(alloc, maxArgc + NumNonArgumentOperands))
        return nullptr;
    return ins;
}

AliasSet
MCallDOMNative::getAliasSet() const
{
    const JSJitInfo *jitInfo = getJitInfo();

    JS_ASSERT(jitInfo->aliasSet() != JSJitInfo::AliasNone);
    
    
    
    if (jitInfo->aliasSet() != JSJitInfo::AliasDOMSets || !jitInfo->isTypedMethodJitInfo())
        return AliasSet::Store(AliasSet::Any);

    uint32_t argIndex = 0;
    const JSTypedMethodJitInfo *methodInfo =
        reinterpret_cast<const JSTypedMethodJitInfo*>(jitInfo);
    for (const JSJitInfo::ArgType *argType = methodInfo->argTypes;
         *argType != JSJitInfo::ArgTypeListEnd;
         ++argType, ++argIndex)
    {
        if (argIndex >= numActualArgs()) {
            
            continue;
        }
        
        MDefinition *arg = getArg(argIndex+1);
        MIRType actualType = arg->type();
        
        
        
        
        
        
        
        if ((actualType == MIRType_Value || actualType == MIRType_Object) ||
            (*argType & JSJitInfo::Object))
         {
             return AliasSet::Store(AliasSet::Any);
         }
    }

    
    
    return AliasSet::Load(AliasSet::DOMProperty);
}

void
MCallDOMNative::computeMovable()
{
    
    
    
    const JSJitInfo *jitInfo = getJitInfo();

    JS_ASSERT_IF(jitInfo->isMovable,
                 jitInfo->aliasSet() != JSJitInfo::AliasEverything);

    if (jitInfo->isMovable && !isEffectful())
        setMovable();
}

bool
MCallDOMNative::congruentTo(const MDefinition *ins) const
{
    if (!isMovable())
        return false;

    if (!ins->isCall())
        return false;

    const MCall *call = ins->toCall();

    if (!call->isCallDOMNative())
        return false;

    if (getSingleTarget() != call->getSingleTarget())
        return false;

    if (isConstructing() != call->isConstructing())
        return false;

    if (numActualArgs() != call->numActualArgs())
        return false;

    if (needsArgCheck() != call->needsArgCheck())
        return false;

    if (!congruentIfOperandsEqual(call))
        return false;

    
    JS_ASSERT(call->isMovable());

    return true;
}

const JSJitInfo *
MCallDOMNative::getJitInfo() const
{
    JS_ASSERT(getSingleTarget() && getSingleTarget()->isNative());

    const JSJitInfo *jitInfo = getSingleTarget()->jitInfo();
    JS_ASSERT(jitInfo);

    return jitInfo;
}

MApplyArgs *
MApplyArgs::New(TempAllocator &alloc, JSFunction *target, MDefinition *fun, MDefinition *argc,
                MDefinition *self)
{
    return new(alloc) MApplyArgs(target, fun, argc, self);
}

MDefinition*
MStringLength::foldsTo(TempAllocator &alloc)
{
    if ((type() == MIRType_Int32) && (string()->isConstant())) {
        Value value = string()->toConstant()->value();
        JSAtom *atom = &value.toString()->asAtom();
        return MConstant::New(alloc, Int32Value(atom->length()));
    }

    return this;
}

static bool
EnsureFloatInputOrConvert(MUnaryInstruction *owner, TempAllocator &alloc)
{
    MDefinition *input = owner->input();
    if (!input->canProduceFloat32()) {
        if (input->type() == MIRType_Float32)
            ConvertDefinitionToDouble<0>(alloc, input, owner);
        return false;
    }
    return true;
}

void
MFloor::trySpecializeFloat32(TempAllocator &alloc)
{
    JS_ASSERT(type() == MIRType_Int32);
    if (EnsureFloatInputOrConvert(this, alloc))
        setPolicyType(MIRType_Float32);
}

void
MCeil::trySpecializeFloat32(TempAllocator &alloc)
{
    JS_ASSERT(type() == MIRType_Int32);
    if (EnsureFloatInputOrConvert(this, alloc))
        setPolicyType(MIRType_Float32);
}

void
MRound::trySpecializeFloat32(TempAllocator &alloc)
{
    JS_ASSERT(type() == MIRType_Int32);
    if (EnsureFloatInputOrConvert(this, alloc))
        setPolicyType(MIRType_Float32);
}

MCompare *
MCompare::New(TempAllocator &alloc, MDefinition *left, MDefinition *right, JSOp op)
{
    return new(alloc) MCompare(left, right, op);
}

MCompare *
MCompare::NewAsmJS(TempAllocator &alloc, MDefinition *left, MDefinition *right, JSOp op,
                   CompareType compareType)
{
    JS_ASSERT(compareType == Compare_Int32 || compareType == Compare_UInt32 ||
              compareType == Compare_Double || compareType == Compare_Float32);
    MCompare *comp = new(alloc) MCompare(left, right, op);
    comp->compareType_ = compareType;
    comp->operandMightEmulateUndefined_ = false;
    comp->setResultType(MIRType_Int32);
    return comp;
}

MTableSwitch *
MTableSwitch::New(TempAllocator &alloc, MDefinition *ins, int32_t low, int32_t high)
{
    return new(alloc) MTableSwitch(alloc, ins, low, high);
}

MGoto *
MGoto::New(TempAllocator &alloc, MBasicBlock *target)
{
    JS_ASSERT(target);
    return new(alloc) MGoto(target);
}

void
MUnbox::printOpcode(FILE *fp) const
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
      case MIRType_Symbol: fprintf(fp, "to Symbol"); break;
      case MIRType_Object: fprintf(fp, "to Object"); break;
      default: break;
    }

    switch (mode()) {
      case Fallible: fprintf(fp, " (fallible)"); break;
      case Infallible: fprintf(fp, " (infallible)"); break;
      case TypeBarrier: fprintf(fp, " (typebarrier)"); break;
      default: break;
    }
}

void
MTypeBarrier::printOpcode(FILE *fp) const
{
    PrintOpcodeName(fp, op());
    fprintf(fp, " ");
    getOperand(0)->printName(fp);
}

#ifdef DEBUG
void
MPhi::assertLoopPhi() const
{
    
    
    MBasicBlock *pred = block()->getPredecessor(0);
    MBasicBlock *back = block()->getPredecessor(1);
    JS_ASSERT(pred == block()->loopPredecessor());
    JS_ASSERT(pred->successorWithPhis() == block());
    JS_ASSERT(pred->positionInPhiSuccessor() == 0);
    JS_ASSERT(back == block()->backedge());
    JS_ASSERT(back->successorWithPhis() == block());
    JS_ASSERT(back->positionInPhiSuccessor() == 1);
}
#endif

void
MPhi::removeOperand(size_t index)
{
    JS_ASSERT(index < numOperands());
    JS_ASSERT(getUseFor(index)->index() == index);
    JS_ASSERT(getUseFor(index)->consumer() == this);

    
    
    
    size_t length = inputs_.length();
    for (size_t i = index; i < length - 1; i++)
        inputs_[i].replaceProducer(inputs_[i + 1].producer());

    
    inputs_[length - 1].discardProducer();
    inputs_.shrinkBy(1);
}

void
MPhi::removeAllOperands()
{
    for (size_t i = 0; i < inputs_.length(); i++)
        inputs_[i].discardProducer();
    inputs_.clear();
}

MDefinition *
MPhi::operandIfRedundant()
{
    JS_ASSERT(inputs_.length() != 0);

    
    
    
    MDefinition *first = getOperand(0);
    for (size_t i = 1, e = numOperands(); i < e; i++) {
        MDefinition *op = getOperand(i);
        if (op != first && op != this)
            return nullptr;
    }
    return first;
}

MDefinition *
MPhi::foldsTo(TempAllocator &alloc)
{
    if (MDefinition *def = operandIfRedundant())
        return def;

    return this;
}

bool
MPhi::congruentTo(const MDefinition *ins) const
{
    if (!ins->isPhi())
        return false;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (ins->block() != block())
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

static inline types::TemporaryTypeSet *
MakeMIRTypeSet(MIRType type)
{
    JS_ASSERT(type != MIRType_Value);
    types::Type ntype = type == MIRType_Object
                        ? types::Type::AnyObjectType()
                        : types::Type::PrimitiveType(ValueTypeFromMIRType(type));
    LifoAlloc *alloc = GetIonContext()->temp->lifoAlloc();
    return alloc->new_<types::TemporaryTypeSet>(alloc, ntype);
}

bool
jit::MergeTypes(MIRType *ptype, types::TemporaryTypeSet **ptypeSet,
                MIRType newType, types::TemporaryTypeSet *newTypeSet)
{
    if (newTypeSet && newTypeSet->empty())
        return true;
    if (newType != *ptype) {
        if (IsNumberType(newType) && IsNumberType(*ptype)) {
            *ptype = MIRType_Double;
        } else if (*ptype != MIRType_Value) {
            if (!*ptypeSet) {
                *ptypeSet = MakeMIRTypeSet(*ptype);
                if (!*ptypeSet)
                    return false;
            }
            *ptype = MIRType_Value;
        } else if (*ptypeSet && (*ptypeSet)->empty()) {
            *ptype = newType;
        }
    }
    if (*ptypeSet) {
        LifoAlloc *alloc = GetIonContext()->temp->lifoAlloc();
        if (!newTypeSet && newType != MIRType_Value) {
            newTypeSet = MakeMIRTypeSet(newType);
            if (!newTypeSet)
                return false;
        }
        if (newTypeSet) {
            if (!newTypeSet->isSubset(*ptypeSet))
                *ptypeSet = types::TypeSet::unionSets(*ptypeSet, newTypeSet, alloc);
        } else {
            *ptypeSet = nullptr;
        }
    }
    return true;
}

bool
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
    types::TemporaryTypeSet *resultTypeSet = this->resultTypeSet();

    for (size_t i = start; i < inputs_.length(); i++) {
        MDefinition *def = getOperand(i);
        if (!MergeTypes(&resultType, &resultTypeSet, def->type(), def->resultTypeSet()))
            return false;
    }

    setResultType(resultType);
    setResultTypeSet(resultTypeSet);
    return true;
}

bool
MPhi::addBackedgeType(MIRType type, types::TemporaryTypeSet *typeSet)
{
    JS_ASSERT(!specialized_);

    if (hasBackedgeType_) {
        MIRType resultType = this->type();
        types::TemporaryTypeSet *resultTypeSet = this->resultTypeSet();

        if (!MergeTypes(&resultType, &resultTypeSet, type, typeSet))
            return false;

        setResultType(resultType);
        setResultTypeSet(resultTypeSet);
    } else {
        setResultType(type);
        setResultTypeSet(typeSet);
        hasBackedgeType_ = true;
    }
    return true;
}

bool
MPhi::typeIncludes(MDefinition *def)
{
    if (def->type() == MIRType_Int32 && this->type() == MIRType_Double)
        return true;

    if (types::TemporaryTypeSet *types = def->resultTypeSet()) {
        if (this->resultTypeSet())
            return types->isSubset(this->resultTypeSet());
        if (this->type() == MIRType_Value || types->empty())
            return true;
        return this->type() == types->getKnownMIRType();
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

    inputs_.append(MUse());
    inputs_.back().init(ins, this);
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

    inputs_.back().init(ins, this);

    if (ptypeChange) {
        MIRType resultType = this->type();
        types::TemporaryTypeSet *resultTypeSet = this->resultTypeSet();

        if (!MergeTypes(&resultType, &resultTypeSet, ins->type(), ins->resultTypeSet()))
            return false;

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

void
MCall::addArg(size_t argnum, MDefinition *arg)
{
    
    
    
    initOperand(argnum + NumNonArgumentOperands, arg);
}

void
MBitNot::infer()
{
    if (getOperand(0)->mightBeType(MIRType_Object) || getOperand(0)->mightBeType(MIRType_Symbol))
        specialization_ = MIRType_None;
    else
        specialization_ = MIRType_Int32;
}

static inline bool
IsConstant(MDefinition *def, double v)
{
    if (!def->isConstant())
        return false;

    return NumbersAreIdentical(def->toConstant()->value().toNumber(), v);
}

MDefinition *
MBinaryBitwiseInstruction::foldsTo(TempAllocator &alloc)
{
    if (specialization_ != MIRType_Int32)
        return this;

    if (MDefinition *folded = EvaluateConstantOperands(alloc, this))
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

    if (lhs == rhs)
        return foldIfEqual();

    return this;
}

void
MBinaryBitwiseInstruction::infer(BaselineInspector *, jsbytecode *)
{
    if (getOperand(0)->mightBeType(MIRType_Object) || getOperand(0)->mightBeType(MIRType_Symbol) ||
        getOperand(1)->mightBeType(MIRType_Object) || getOperand(1)->mightBeType(MIRType_Symbol))
    {
        specialization_ = MIRType_None;
    } else {
        specializeAsInt32();
    }
}

void
MBinaryBitwiseInstruction::specializeAsInt32()
{
    specialization_ = MIRType_Int32;
    JS_ASSERT(type() == MIRType_Int32);

    if (isBitOr() || isBitAnd() || isBitXor())
        setCommutative();
}

void
MShiftInstruction::infer(BaselineInspector *, jsbytecode *)
{
    if (getOperand(0)->mightBeType(MIRType_Object) || getOperand(1)->mightBeType(MIRType_Object) ||
        getOperand(0)->mightBeType(MIRType_Symbol) || getOperand(1)->mightBeType(MIRType_Symbol))
        specialization_ = MIRType_None;
    else
        specialization_ = MIRType_Int32;
}

void
MUrsh::infer(BaselineInspector *inspector, jsbytecode *pc)
{
    if (getOperand(0)->mightBeType(MIRType_Object) || getOperand(1)->mightBeType(MIRType_Object) ||
        getOperand(0)->mightBeType(MIRType_Symbol) || getOperand(1)->mightBeType(MIRType_Symbol))
    {
        specialization_ = MIRType_None;
        setResultType(MIRType_Value);
        return;
    }

    if (inspector->hasSeenDoubleResult(pc)) {
        specialization_ = MIRType_Double;
        setResultType(MIRType_Double);
        return;
    }

    specialization_ = MIRType_Int32;
    setResultType(MIRType_Int32);
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

            

            
            
            
            MDefinition *first = use_def->toAdd()->getOperand(0);
            MDefinition *second = use_def->toAdd()->getOperand(1);
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
            for (size_t i = 2, e = use_def->numOperands(); i < e; i++) {
                if (use_def->getOperand(i) == def)
                    return true;
            }
            break;
          case MDefinition::Op_BoundsCheck:
            
            if (use_def->toBoundsCheck()->getOperand(1) == def)
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
MBinaryArithInstruction::foldsTo(TempAllocator &alloc)
{
    if (specialization_ == MIRType_None)
        return this;

    MDefinition *lhs = getOperand(0);
    MDefinition *rhs = getOperand(1);
    if (MDefinition *folded = EvaluateConstantOperands(alloc, this))
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

void
MBinaryArithInstruction::trySpecializeFloat32(TempAllocator &alloc)
{
    
    if (specialization_ == MIRType_Int32)
        return;

    MDefinition *left = lhs();
    MDefinition *right = rhs();

    if (!left->canProduceFloat32() || !right->canProduceFloat32()
        || !CheckUsesAreFloat32Consumers(this))
    {
        if (left->type() == MIRType_Float32)
            ConvertDefinitionToDouble<0>(alloc, left, this);
        if (right->type() == MIRType_Float32)
            ConvertDefinitionToDouble<1>(alloc, right, this);
        return;
    }

    specialization_ = MIRType_Float32;
    setResultType(MIRType_Float32);
}

bool
MAbs::fallible() const
{
    return !implicitTruncate_ && (!range() || !range()->hasInt32Bounds());
}

void
MAbs::trySpecializeFloat32(TempAllocator &alloc)
{
    
    if (input()->type() == MIRType_Int32)
        return;

    if (!input()->canProduceFloat32() || !CheckUsesAreFloat32Consumers(this)) {
        if (input()->type() == MIRType_Float32)
            ConvertDefinitionToDouble<0>(alloc, input(), this);
        return;
    }

    setResultType(MIRType_Float32);
    specialization_ = MIRType_Float32;
}

MDefinition *
MDiv::foldsTo(TempAllocator &alloc)
{
    if (specialization_ == MIRType_None)
        return this;

    if (MDefinition *folded = EvaluateConstantOperands(alloc, this))
        return folded;

    if (MDefinition *folded = EvaluateExactReciprocal(alloc, this))
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
MDiv::fallible() const
{
    return !isTruncated();
}

MDefinition *
MMod::foldsTo(TempAllocator &alloc)
{
    if (specialization_ == MIRType_None)
        return this;

    if (MDefinition *folded = EvaluateConstantOperands(alloc, this))
        return folded;

    return this;
}

void
MMod::analyzeEdgeCasesForward()
{
    
    if (specialization_ != MIRType_Int32)
        return;

    if (rhs()->isConstant() && !rhs()->toConstant()->value().isInt32(0))
        canBeDivideByZero_ = false;

    if (rhs()->isConstant()) {
        int32_t n = rhs()->toConstant()->value().toInt32();
        if (n > 0 && !IsPowerOfTwo(n))
            canBePowerOfTwoDivisor_ = false;
    }
}

bool
MMod::fallible() const
{
    return !isTruncated() &&
           (isUnsigned() || canBeDivideByZero() || canBeNegativeDividend());
}

void
MMathFunction::trySpecializeFloat32(TempAllocator &alloc)
{
    if (!input()->canProduceFloat32() || !CheckUsesAreFloat32Consumers(this)) {
        if (input()->type() == MIRType_Float32)
            ConvertDefinitionToDouble<0>(alloc, input(), this);
        return;
    }

    setResultType(MIRType_Float32);
    setPolicyType(MIRType_Float32);
}

bool
MAdd::fallible() const
{
    
    
    if (truncateKind() >= IndirectTruncate)
        return false;
    if (range() && range()->hasInt32Bounds())
        return false;
    return true;
}

bool
MSub::fallible() const
{
    
    if (truncateKind() >= IndirectTruncate)
        return false;
    if (range() && range()->hasInt32Bounds())
        return false;
    return true;
}

MDefinition *
MMul::foldsTo(TempAllocator &alloc)
{
    MDefinition *out = MBinaryArithInstruction::foldsTo(alloc);
    if (out != this)
        return out;

    if (specialization() != MIRType_Int32)
        return this;

    if (lhs() == rhs())
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
    
    if (mode_ == Integer && ins->mode() != Integer)
        mode_ = Normal;
    return true;
}

bool
MMul::canOverflow() const
{
    if (isTruncated())
        return false;
    return !range() || !range()->hasInt32Bounds();
}

bool
MUrsh::fallible() const
{
    if (bailoutsDisabled())
        return false;
    return !range() || !range()->hasInt32Bounds();
}

static inline bool
SimpleArithOperand(MDefinition *op)
{
    return !op->mightBeType(MIRType_Object)
        && !op->mightBeType(MIRType_String)
        && !op->mightBeType(MIRType_Symbol)
        && !op->mightBeType(MIRType_MagicOptimizedArguments)
        && !op->mightBeType(MIRType_MagicHole)
        && !op->mightBeType(MIRType_MagicIsConstructing);
}

void
MBinaryArithInstruction::infer(TempAllocator &alloc, BaselineInspector *inspector, jsbytecode *pc)
{
    JS_ASSERT(this->type() == MIRType_Value);

    specialization_ = MIRType_None;

    
    
    
    if (getOperand(0)->mightBeType(MIRType_Object) || getOperand(1)->mightBeType(MIRType_Object))
        return;
    if (getOperand(0)->mightBeType(MIRType_Symbol) || getOperand(1)->mightBeType(MIRType_Symbol))
        return;

    
    
    if (!SimpleArithOperand(getOperand(0)) || !SimpleArithOperand(getOperand(1)))
        return inferFallback(inspector, pc);

    
    MIRType lhs = getOperand(0)->type();
    MIRType rhs = getOperand(1)->type();

    
    
    if (lhs == MIRType_Int32 && rhs == MIRType_Int32)
        setResultType(MIRType_Int32);
    
    
    else if (IsFloatingPointType(lhs) || IsFloatingPointType(rhs))
        setResultType(MIRType_Double);
    else
        return inferFallback(inspector, pc);

    
    if (inspector->hasSeenDoubleResult(pc))
        setResultType(MIRType_Double);

    
    
    if ((isMul() || isDiv()) && lhs == MIRType_Int32 && rhs == MIRType_Int32) {
        bool typeChange = false;
        EvaluateConstantOperands(alloc, this, &typeChange);
        if (typeChange)
            setResultType(MIRType_Double);
    }

    JS_ASSERT(lhs < MIRType_String || lhs == MIRType_Value);
    JS_ASSERT(rhs < MIRType_String || rhs == MIRType_Value);

    MIRType rval = this->type();

    
    if (lhs == MIRType_Value || rhs == MIRType_Value) {
        if (!IsFloatingPointType(rval)) {
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

    
    
    
    if (getOperand(0)->emptyResultTypeSet() || getOperand(1)->emptyResultTypeSet()) {
        LifoAlloc *alloc = GetIonContext()->temp->lifoAlloc();
        types::TemporaryTypeSet *types = alloc->new_<types::TemporaryTypeSet>();
        if (types)
            setResultTypeSet(types);
    }
}

static bool
SafelyCoercesToDouble(MDefinition *op)
{
    
    
    return SimpleArithOperand(op) && !op->mightBeType(MIRType_Null);
}

static bool
ObjectOrSimplePrimitive(MDefinition *op)
{
    
    return !op->mightBeType(MIRType_String)
        && !op->mightBeType(MIRType_Symbol)
        && !op->mightBeType(MIRType_Double)
        && !op->mightBeType(MIRType_Float32)
        && !op->mightBeType(MIRType_MagicOptimizedArguments)
        && !op->mightBeType(MIRType_MagicHole)
        && !op->mightBeType(MIRType_MagicIsConstructing);
}

static bool
CanDoValueBitwiseCmp(MDefinition *lhs, MDefinition *rhs, bool looseEq)
{
    
    
    if (!ObjectOrSimplePrimitive(lhs) || !ObjectOrSimplePrimitive(rhs))
        return false;

    
    if (MaybeEmulatesUndefined(lhs) || MaybeEmulatesUndefined(rhs))
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
      case Compare_Int32MaybeCoerceBoth:
      case Compare_Int32MaybeCoerceLHS:
      case Compare_Int32MaybeCoerceRHS:
        return MIRType_Int32;
      case Compare_Double:
      case Compare_DoubleMaybeCoerceLHS:
      case Compare_DoubleMaybeCoerceRHS:
        return MIRType_Double;
      case Compare_Float32:
        return MIRType_Float32;
      case Compare_String:
      case Compare_StrictString:
        return MIRType_String;
      case Compare_Object:
        return MIRType_Object;
      case Compare_Unknown:
      case Compare_Value:
        return MIRType_Value;
      default:
        MOZ_CRASH("No known conversion");
    }
}

static inline bool
MustBeUInt32(MDefinition *def, MDefinition **pwrapped)
{
    if (def->isUrsh()) {
        *pwrapped = def->toUrsh()->getOperand(0);
        MDefinition *rhs = def->toUrsh()->getOperand(1);
        return !def->toUrsh()->bailoutsDisabled()
            && rhs->isConstant()
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

bool
MBinaryInstruction::tryUseUnsignedOperands()
{
    MDefinition *newlhs, *newrhs;
    if (MustBeUInt32(getOperand(0), &newlhs) && MustBeUInt32(getOperand(1), &newrhs)) {
        if (newlhs->type() != MIRType_Int32 || newrhs->type() != MIRType_Int32)
            return false;
        if (newlhs != getOperand(0)) {
            getOperand(0)->setImplicitlyUsedUnchecked();
            replaceOperand(0, newlhs);
        }
        if (newrhs != getOperand(1)) {
            getOperand(1)->setImplicitlyUsedUnchecked();
            replaceOperand(1, newrhs);
        }
        return true;
    }
    return false;
}

void
MCompare::infer(BaselineInspector *inspector, jsbytecode *pc)
{
    JS_ASSERT(operandMightEmulateUndefined());

    if (!MaybeEmulatesUndefined(getOperand(0)) && !MaybeEmulatesUndefined(getOperand(1)))
        markNoOperandEmulatesUndefined();

    MIRType lhs = getOperand(0)->type();
    MIRType rhs = getOperand(1)->type();

    bool looseEq = jsop() == JSOP_EQ || jsop() == JSOP_NE;
    bool strictEq = jsop() == JSOP_STRICTEQ || jsop() == JSOP_STRICTNE;
    bool relationalEq = !(looseEq || strictEq);

    
    if (tryUseUnsignedOperands()) {
        compareType_ = Compare_UInt32;
        return;
    }

    
    if ((lhs == MIRType_Int32 && rhs == MIRType_Int32) ||
        (lhs == MIRType_Boolean && rhs == MIRType_Boolean))
    {
        compareType_ = Compare_Int32MaybeCoerceBoth;
        return;
    }

    
    if (!strictEq &&
        (lhs == MIRType_Int32 || lhs == MIRType_Boolean) &&
        (rhs == MIRType_Int32 || rhs == MIRType_Boolean))
    {
        compareType_ = Compare_Int32MaybeCoerceBoth;
        return;
    }

    
    if (IsNumberType(lhs) && IsNumberType(rhs)) {
        compareType_ = Compare_Double;
        return;
    }

    
    if (!strictEq && IsFloatingPointType(lhs) && SafelyCoercesToDouble(getOperand(1))) {
        compareType_ = Compare_DoubleMaybeCoerceRHS;
        return;
    }
    if (!strictEq && IsFloatingPointType(rhs) && SafelyCoercesToDouble(getOperand(0))) {
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

    
    if (!relationalEq && CanDoValueBitwiseCmp(getOperand(0), getOperand(1), looseEq)) {
        compareType_ = Compare_Value;
        return;
    }

    
    
    
    
    

    if (!strictEq)
        compareType_ = inspector->expectedCompareType(pc);
}

MBitNot *
MBitNot::New(TempAllocator &alloc, MDefinition *input)
{
    return new(alloc) MBitNot(input);
}

MBitNot *
MBitNot::NewAsmJS(TempAllocator &alloc, MDefinition *input)
{
    MBitNot *ins = new(alloc) MBitNot(input);
    ins->specialization_ = MIRType_Int32;
    JS_ASSERT(ins->type() == MIRType_Int32);
    return ins;
}

MDefinition *
MBitNot::foldsTo(TempAllocator &alloc)
{
    if (specialization_ != MIRType_Int32)
        return this;

    MDefinition *input = getOperand(0);

    if (input->isConstant()) {
        js::Value v = Int32Value(~(input->toConstant()->value().toInt32()));
        return MConstant::New(alloc, v);
    }

    if (input->isBitNot() && input->toBitNot()->specialization_ == MIRType_Int32) {
        JS_ASSERT(input->toBitNot()->getOperand(0)->type() == MIRType_Int32);
        return input->toBitNot()->getOperand(0); 
    }

    return this;
}

MDefinition *
MTypeOf::foldsTo(TempAllocator &alloc)
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
      case MIRType_Symbol:
        type = JSTYPE_SYMBOL;
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
      case MIRType_Object:
        if (!inputMaybeCallableOrEmulatesUndefined()) {
            
            
            type = JSTYPE_OBJECT;
            break;
        }
        
      default:
        return this;
    }

    return MConstant::New(alloc, StringValue(TypeName(type, GetIonContext()->runtime->names())));
}

void
MTypeOf::cacheInputMaybeCallableOrEmulatesUndefined()
{
    JS_ASSERT(inputMaybeCallableOrEmulatesUndefined());

    if (!MaybeEmulatesUndefined(input()) && !MaybeCallable(input()))
        markInputNotCallableOrEmulatesUndefined();
}

MBitAnd *
MBitAnd::New(TempAllocator &alloc, MDefinition *left, MDefinition *right)
{
    return new(alloc) MBitAnd(left, right);
}

MBitAnd *
MBitAnd::NewAsmJS(TempAllocator &alloc, MDefinition *left, MDefinition *right)
{
    MBitAnd *ins = new(alloc) MBitAnd(left, right);
    ins->specializeAsInt32();
    return ins;
}

MBitOr *
MBitOr::New(TempAllocator &alloc, MDefinition *left, MDefinition *right)
{
    return new(alloc) MBitOr(left, right);
}

MBitOr *
MBitOr::NewAsmJS(TempAllocator &alloc, MDefinition *left, MDefinition *right)
{
    MBitOr *ins = new(alloc) MBitOr(left, right);
    ins->specializeAsInt32();
    return ins;
}

MBitXor *
MBitXor::New(TempAllocator &alloc, MDefinition *left, MDefinition *right)
{
    return new(alloc) MBitXor(left, right);
}

MBitXor *
MBitXor::NewAsmJS(TempAllocator &alloc, MDefinition *left, MDefinition *right)
{
    MBitXor *ins = new(alloc) MBitXor(left, right);
    ins->specializeAsInt32();
    return ins;
}

MLsh *
MLsh::New(TempAllocator &alloc, MDefinition *left, MDefinition *right)
{
    return new(alloc) MLsh(left, right);
}

MLsh *
MLsh::NewAsmJS(TempAllocator &alloc, MDefinition *left, MDefinition *right)
{
    MLsh *ins = new(alloc) MLsh(left, right);
    ins->specializeAsInt32();
    return ins;
}

MRsh *
MRsh::New(TempAllocator &alloc, MDefinition *left, MDefinition *right)
{
    return new(alloc) MRsh(left, right);
}

MRsh *
MRsh::NewAsmJS(TempAllocator &alloc, MDefinition *left, MDefinition *right)
{
    MRsh *ins = new(alloc) MRsh(left, right);
    ins->specializeAsInt32();
    return ins;
}

MUrsh *
MUrsh::New(TempAllocator &alloc, MDefinition *left, MDefinition *right)
{
    return new(alloc) MUrsh(left, right);
}

MUrsh *
MUrsh::NewAsmJS(TempAllocator &alloc, MDefinition *left, MDefinition *right)
{
    MUrsh *ins = new(alloc) MUrsh(left, right);
    ins->specializeAsInt32();

    
    
    
    
    
    ins->bailoutsDisabled_ = true;

    return ins;
}

MResumePoint *
MResumePoint::New(TempAllocator &alloc, MBasicBlock *block, jsbytecode *pc, MResumePoint *parent,
                  Mode mode)
{
    MResumePoint *resume = new(alloc) MResumePoint(block, pc, parent, mode);
    if (!resume->init(alloc))
        return nullptr;
    resume->inherit(block);
    return resume;
}

MResumePoint *
MResumePoint::New(TempAllocator &alloc, MBasicBlock *block, jsbytecode *pc, MResumePoint *parent,
                  Mode mode, const MDefinitionVector &operands)
{
    MResumePoint *resume = new(alloc) MResumePoint(block, pc, parent, mode);

    if (!resume->operands_.init(alloc, operands.length()))
        return nullptr;
    for (size_t i = 0; i < operands.length(); i++)
        resume->initOperand(i, operands[i]);

    return resume;
}

MResumePoint *
MResumePoint::Copy(TempAllocator &alloc, MResumePoint *src)
{
    MResumePoint *resume = new(alloc) MResumePoint(src->block(), src->pc(),
                                                   src->caller(), src->mode());
    
    
    if (!resume->operands_.init(alloc, src->stackDepth()))
        return nullptr;
    for (size_t i = 0; i < resume->stackDepth(); i++)
        resume->initOperand(i, src->getOperand(i));
    return resume;
}

MResumePoint::MResumePoint(MBasicBlock *block, jsbytecode *pc, MResumePoint *caller,
                           Mode mode)
  : MNode(block),
    pc_(pc),
    caller_(caller),
    instruction_(nullptr),
    mode_(mode)
{
    block->addResumePoint(this);
}

bool MResumePoint::init(TempAllocator &alloc)
{
    return operands_.init(alloc, block()->stackDepth());
}

void
MResumePoint::inherit(MBasicBlock *block)
{
    
    for (size_t i = 0; i < stackDepth(); i++)
        initOperand(i, block->getSlot(i));
}

void
MResumePoint::dump(FILE *fp) const
{
    fprintf(fp, "resumepoint mode=");

    switch (mode()) {
      case MResumePoint::ResumeAt:
        fprintf(fp, "At");
        break;
      case MResumePoint::ResumeAfter:
        fprintf(fp, "After");
        break;
      case MResumePoint::Outer:
        fprintf(fp, "Outer");
        break;
    }

    if (MResumePoint *c = caller())
        fprintf(fp, " (caller in block%u)", c->block()->id());

    for (size_t i = 0; i < numOperands(); i++) {
        fprintf(fp, " ");
        if (operands_[i].hasProducer())
            getOperand(i)->printName(fp);
        else
            fprintf(fp, "(null)");
    }
    fprintf(fp, "\n");
}

void
MResumePoint::dump() const
{
    dump(stderr);
}

bool
MResumePoint::isObservableOperand(MUse *u) const
{
    return isObservableOperand(indexOf(u));
}

bool
MResumePoint::isObservableOperand(size_t index) const
{
    return block()->info().isObservableSlot(index);
}

MDefinition *
MToInt32::foldsTo(TempAllocator &alloc)
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
MTruncateToInt32::foldsTo(TempAllocator &alloc)
{
    MDefinition *input = getOperand(0);
    if (input->type() == MIRType_Int32)
        return input;

    if (input->type() == MIRType_Double && input->isConstant()) {
        const Value &v = input->toConstant()->value();
        int32_t ret = ToInt32(v.toDouble());
        return MConstant::New(alloc, Int32Value(ret));
    }

    return this;
}

MDefinition *
MToDouble::foldsTo(TempAllocator &alloc)
{
    MDefinition *in = input();
    if (in->type() == MIRType_Double)
        return in;

    if (in->isConstant()) {
        const Value &v = in->toConstant()->value();
        if (v.isNumber()) {
            double out = v.toNumber();
            return MConstant::New(alloc, DoubleValue(out));
        }
    }

    return this;
}

MDefinition *
MToFloat32::foldsTo(TempAllocator &alloc)
{
    if (input()->type() == MIRType_Float32)
        return input();

    
    if (input()->isToDouble() && input()->toToDouble()->input()->type() == MIRType_Float32)
        return input()->toToDouble()->input();

    if (input()->isConstant()) {
        const Value &v = input()->toConstant()->value();
        if (v.isNumber()) {
            float out = v.toNumber();
            MConstant *c = MConstant::New(alloc, DoubleValue(out));
            c->setResultType(MIRType_Float32);
            return c;
        }
    }
    return this;
}

MDefinition *
MToString::foldsTo(TempAllocator &alloc)
{
    MDefinition *in = input();
    if (in->type() == MIRType_String)
        return in;
    return this;
}

MDefinition *
MClampToUint8::foldsTo(TempAllocator &alloc)
{
    if (input()->isConstant()) {
        const Value &v = input()->toConstant()->value();
        if (v.isDouble()) {
            int32_t clamped = ClampDoubleToUint8(v.toDouble());
            return MConstant::New(alloc, Int32Value(clamped));
        }
        if (v.isInt32()) {
            int32_t clamped = ClampIntForUint8Array(v.toInt32());
            return MConstant::New(alloc, Int32Value(clamped));
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
          case MIRType_Float32:
          case MIRType_String:
          case MIRType_Symbol:
          case MIRType_Boolean:
            *result = (op == JSOP_NE || op == JSOP_STRICTNE);
            return true;
          default:
            MOZ_CRASH("Unexpected type");
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
          case MIRType_Float32:
          case MIRType_String:
          case MIRType_Symbol:
          case MIRType_Object:
          case MIRType_Null:
          case MIRType_Undefined:
            *result = (op == JSOP_STRICTNE);
            return true;
          case MIRType_Boolean:
            
            MOZ_CRASH("Wrong specialization");
          default:
            MOZ_CRASH("Unexpected type");
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
          case MIRType_Float32:
          case MIRType_Symbol:
          case MIRType_Object:
          case MIRType_Null:
          case MIRType_Undefined:
            *result = (op == JSOP_STRICTNE);
            return true;
          case MIRType_String:
            
            MOZ_CRASH("Wrong specialization");
          default:
            MOZ_CRASH("Unexpected type");
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
        if (left != right)
            comp = CompareAtoms(&lhs.toString()->asAtom(), &rhs.toString()->asAtom());

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
            MOZ_CRASH("Unexpected op.");
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
          case JSOP_STRICTEQ: 
          case JSOP_EQ:
            *result = (lhsUint == rhsUint);
            break;
          case JSOP_STRICTNE: 
          case JSOP_NE:
            *result = (lhsUint != rhsUint);
            break;
          default:
            MOZ_CRASH("Unexpected op.");
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
      case JSOP_STRICTEQ: 
      case JSOP_EQ:
        *result = (lhs.toNumber() == rhs.toNumber());
        break;
      case JSOP_STRICTNE: 
      case JSOP_NE:
        *result = (lhs.toNumber() != rhs.toNumber());
        break;
      default:
        return false;
    }

    return true;
}

MDefinition *
MCompare::foldsTo(TempAllocator &alloc)
{
    bool result;

    if (tryFold(&result) || evaluateConstantOperands(&result)) {
        if (type() == MIRType_Int32)
            return MConstant::New(alloc, Int32Value(result));

        JS_ASSERT(type() == MIRType_Boolean);
        return MConstant::New(alloc, BooleanValue(result));
    }

    return this;
}

void
MCompare::trySpecializeFloat32(TempAllocator &alloc)
{
    MDefinition *lhs = getOperand(0);
    MDefinition *rhs = getOperand(1);

    if (lhs->canProduceFloat32() && rhs->canProduceFloat32() && compareType_ == Compare_Double) {
        compareType_ = Compare_Float32;
    } else {
        if (lhs->type() == MIRType_Float32)
            ConvertDefinitionToDouble<0>(alloc, lhs, this);
        if (rhs->type() == MIRType_Float32)
            ConvertDefinitionToDouble<1>(alloc, rhs, this);
    }
}

void
MCompare::filtersUndefinedOrNull(bool trueBranch, MDefinition **subject, bool *filtersUndefined,
                                 bool *filtersNull)
{
    *filtersNull = *filtersUndefined = false;
    *subject = nullptr;

    if (compareType() != Compare_Undefined && compareType() != Compare_Null)
        return;

    JS_ASSERT(jsop() == JSOP_STRICTNE || jsop() == JSOP_NE ||
              jsop() == JSOP_STRICTEQ || jsop() == JSOP_EQ);

    
    if (!trueBranch && (jsop() == JSOP_STRICTNE || jsop() == JSOP_NE))
        return;

    
    if (trueBranch && (jsop() == JSOP_STRICTEQ || jsop() == JSOP_EQ))
        return;

    if (jsop() == JSOP_STRICTEQ || jsop() == JSOP_STRICTNE) {
        *filtersUndefined = compareType() == Compare_Undefined;
        *filtersNull = compareType() == Compare_Null;
    } else {
        *filtersUndefined = *filtersNull = true;
    }

    *subject = lhs();
}

void
MNot::cacheOperandMightEmulateUndefined()
{
    JS_ASSERT(operandMightEmulateUndefined());

    if (!MaybeEmulatesUndefined(getOperand(0)))
        markOperandCantEmulateUndefined();
}

MDefinition *
MNot::foldsTo(TempAllocator &alloc)
{
    
    if (input()->isConstant()) {
        bool result = input()->toConstant()->valueToBoolean();
        if (type() == MIRType_Int32)
            return MConstant::New(alloc, Int32Value(!result));

        
        return MConstant::New(alloc, BooleanValue(!result));
    }

    
    if (input()->type() == MIRType_Undefined || input()->type() == MIRType_Null)
        return MConstant::New(alloc, BooleanValue(true));

    
    if (input()->type() == MIRType_Object && !operandMightEmulateUndefined())
        return MConstant::New(alloc, BooleanValue(false));

    return this;
}

void
MNot::trySpecializeFloat32(TempAllocator &alloc)
{
    MDefinition *in = input();
    if (!in->canProduceFloat32() && in->type() == MIRType_Float32)
        ConvertDefinitionToDouble<0>(alloc, in, this);
}

void
MBeta::printOpcode(FILE *fp) const
{
    MDefinition::printOpcode(fp);

    Sprinter sp(GetIonContext()->cx);
    sp.init();
    comparison_->print(sp);
    fprintf(fp, " %s", sp.string());
}

bool
MNewObject::shouldUseVM() const
{
    JSObject *obj = templateObject();
    return obj->hasSingletonType() || obj->hasDynamicSlots();
}

MObjectState::MObjectState(MDefinition *obj)
{
    
    setRecoveredOnBailout();
    JSObject *templateObject = obj->toNewObject()->templateObject();
    numSlots_ = templateObject->slotSpan();
    numFixedSlots_ = templateObject->numFixedSlots();
}

bool
MObjectState::init(TempAllocator &alloc, MDefinition *obj)
{
    if (!MVariadicInstruction::init(alloc, numSlots() + 1))
        return false;
    initOperand(0, obj);
    return true;
}

MObjectState *
MObjectState::New(TempAllocator &alloc, MDefinition *obj, MDefinition *undefinedVal)
{
    MObjectState *res = new(alloc) MObjectState(obj);
    if (!res || !res->init(alloc, obj))
        return nullptr;
    for (size_t i = 0; i < res->numSlots(); i++)
        res->initSlot(i, undefinedVal);
    return res;
}

MObjectState *
MObjectState::Copy(TempAllocator &alloc, MObjectState *state)
{
    MDefinition *obj = state->object();
    MObjectState *res = new(alloc) MObjectState(obj);
    if (!res || !res->init(alloc, obj))
        return nullptr;
    for (size_t i = 0; i < res->numSlots(); i++)
        res->initSlot(i, state->getSlot(i));
    return res;
}

bool
MNewArray::shouldUseVM() const
{
    JS_ASSERT(count() < JSObject::NELEMENTS_LIMIT);

    size_t arraySlots =
        gc::GetGCKindSlots(templateObject()->tenuredGetAllocKind()) - ObjectElements::VALUES_PER_HEADER;

    
    
    bool allocating = allocatingBehaviour() != NewArray_Unallocating && count() > arraySlots;

    return templateObject()->hasSingletonType() || allocating;
}

bool
MLoadFixedSlot::mightAlias(const MDefinition *store) const
{
    if (store->isStoreFixedSlot() && store->toStoreFixedSlot()->slot() != slot())
        return false;
    return true;
}

MDefinition *
MLoadFixedSlot::foldsTo(TempAllocator &alloc)
{
    if (!dependency() || !dependency()->isStoreFixedSlot())
        return this;

    MStoreFixedSlot *store = dependency()->toStoreFixedSlot();
    if (!store->block()->dominates(block()))
        return this;

    if (store->object() != object())
        return this;

    if (store->slot() != slot())
        return this;

    if (store->value()->type() != type())
        return this;

    return store->value();
}

bool
MAsmJSLoadHeap::mightAlias(const MDefinition *def) const
{
    if (def->isAsmJSStoreHeap()) {
        const MAsmJSStoreHeap *store = def->toAsmJSStoreHeap();
        if (store->viewType() != viewType())
            return true;
        if (!ptr()->isConstant() || !store->ptr()->isConstant())
            return true;
        const MConstant *otherPtr = store->ptr()->toConstant();
        return ptr()->toConstant()->value() == otherPtr->value();
    }
    return true;
}

bool
MAsmJSLoadHeap::congruentTo(const MDefinition *ins) const
{
    if (!ins->isAsmJSLoadHeap())
        return false;
    const MAsmJSLoadHeap *load = ins->toAsmJSLoadHeap();
    return load->viewType() == viewType() && congruentIfOperandsEqual(load);
}

bool
MAsmJSLoadGlobalVar::mightAlias(const MDefinition *def) const
{
    if (def->isAsmJSStoreGlobalVar()) {
        const MAsmJSStoreGlobalVar *store = def->toAsmJSStoreGlobalVar();
        return store->globalDataOffset() == globalDataOffset_;
    }
    return true;
}

HashNumber
MAsmJSLoadGlobalVar::valueHash() const
{
    HashNumber hash = MDefinition::valueHash();
    hash = addU32ToHash(hash, globalDataOffset_);
    return hash;
}

bool
MAsmJSLoadGlobalVar::congruentTo(const MDefinition *ins) const
{
    if (ins->isAsmJSLoadGlobalVar()) {
        const MAsmJSLoadGlobalVar *load = ins->toAsmJSLoadGlobalVar();
        return globalDataOffset_ == load->globalDataOffset_;
    }
    return false;
}

MDefinition *
MAsmJSLoadGlobalVar::foldsTo(TempAllocator &alloc)
{
    if (!dependency() || !dependency()->isAsmJSStoreGlobalVar())
        return this;

    MAsmJSStoreGlobalVar *store = dependency()->toAsmJSStoreGlobalVar();
    if (!store->block()->dominates(block()))
        return this;

    if (store->globalDataOffset() != globalDataOffset())
        return this;

    if (store->value()->type() != type())
        return this;

    return store->value();
}

HashNumber
MAsmJSLoadFuncPtr::valueHash() const
{
    HashNumber hash = MDefinition::valueHash();
    hash = addU32ToHash(hash, globalDataOffset_);
    return hash;
}

bool
MAsmJSLoadFuncPtr::congruentTo(const MDefinition *ins) const
{
    if (ins->isAsmJSLoadFuncPtr()) {
        const MAsmJSLoadFuncPtr *load = ins->toAsmJSLoadFuncPtr();
        return globalDataOffset_ == load->globalDataOffset_;
    }
    return false;
}

HashNumber
MAsmJSLoadFFIFunc::valueHash() const
{
    HashNumber hash = MDefinition::valueHash();
    hash = addU32ToHash(hash, globalDataOffset_);
    return hash;
}

bool
MAsmJSLoadFFIFunc::congruentTo(const MDefinition *ins) const
{
    if (ins->isAsmJSLoadFFIFunc()) {
        const MAsmJSLoadFFIFunc *load = ins->toAsmJSLoadFFIFunc();
        return globalDataOffset_ == load->globalDataOffset_;
    }
    return false;
}

bool
MLoadSlot::mightAlias(const MDefinition *store) const
{
    if (store->isStoreSlot() && store->toStoreSlot()->slot() != slot())
        return false;
    return true;
}

HashNumber
MLoadSlot::valueHash() const
{
    HashNumber hash = MDefinition::valueHash();
    hash = addU32ToHash(hash, slot_);
    return hash;
}

MDefinition *
MLoadSlot::foldsTo(TempAllocator &alloc)
{
    if (!dependency() || !dependency()->isStoreSlot())
        return this;

    MStoreSlot *store = dependency()->toStoreSlot();
    if (!store->block()->dominates(block()))
        return this;

    if (store->slots() != slots())
        return this;

    if (store->value()->type() != type())
        return this;

    return store->value();
}

MDefinition *
MLoadElement::foldsTo(TempAllocator &alloc)
{
    if (!dependency() || !dependency()->isStoreElement())
        return this;

    MStoreElement *store = dependency()->toStoreElement();
    if (!store->block()->dominates(block()))
        return this;

    if (store->elements() != elements())
        return this;

    if (store->index() != index())
        return this;

    if (store->value()->type() != type())
        return this;

    return store->value();
}

bool
MGuardShapePolymorphic::congruentTo(const MDefinition *ins) const
{
    if (!ins->isGuardShapePolymorphic())
        return false;

    const MGuardShapePolymorphic *other = ins->toGuardShapePolymorphic();
    if (numShapes() != other->numShapes())
        return false;

    for (size_t i = 0; i < numShapes(); i++) {
        if (getShape(i) != other->getShape(i))
            return false;
    }

    return congruentIfOperandsEqual(ins);
}

void
InlinePropertyTable::trimTo(ObjectVector &targets, BoolVector &choiceSet)
{
    for (size_t i = 0; i < targets.length(); i++) {
        
        if (choiceSet[i])
            continue;

        JSFunction *target = &targets[i]->as<JSFunction>();

        
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
InlinePropertyTable::trimToTargets(ObjectVector &targets)
{
    JitSpew(JitSpew_Inlining, "Got inlineable property cache with %d cases",
            (int)numEntries());

    size_t i = 0;
    while (i < numEntries()) {
        bool foundFunc = false;
        for (size_t j = 0; j < targets.length(); j++) {
            if (entries_[i]->func == targets[j]) {
                foundFunc = true;
                break;
            }
        }
        if (!foundFunc)
            entries_.erase(&(entries_[i]));
        else
            i++;
    }

    JitSpew(JitSpew_Inlining, "%d inlineable cases left after trimming to %d targets",
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

types::TemporaryTypeSet *
InlinePropertyTable::buildTypeSetForFunction(JSFunction *func) const
{
    LifoAlloc *alloc = GetIonContext()->temp->lifoAlloc();
    types::TemporaryTypeSet *types = alloc->new_<types::TemporaryTypeSet>();
    if (!types)
        return nullptr;
    for (size_t i = 0; i < numEntries(); i++) {
        if (entries_[i]->func == func)
            types->addType(types::Type::ObjectType(entries_[i]->typeObj), alloc);
    }
    return types;
}

void *
MLoadTypedArrayElementStatic::base() const
{
    return typedArray_->viewData();
}

size_t
MLoadTypedArrayElementStatic::length() const
{
    return typedArray_->byteLength();
}

void *
MStoreTypedArrayElementStatic::base() const
{
    return typedArray_->viewData();
}

bool
MGetElementCache::allowDoubleResult() const
{
    if (!resultTypeSet())
        return true;

    return resultTypeSet()->hasType(types::Type::DoubleType());
}

size_t
MStoreTypedArrayElementStatic::length() const
{
    return typedArray_->byteLength();
}

bool
MGetPropertyPolymorphic::mightAlias(const MDefinition *store) const
{
    
    

    if (!store->isStoreFixedSlot() && !store->isStoreSlot())
        return true;

    for (size_t i = 0; i < numShapes(); i++) {
        const Shape *shape = this->shape(i);
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

void
MGetPropertyCache::setBlock(MBasicBlock *block)
{
    MDefinition::setBlock(block);
    
    if (!location_.pc) {
        location_.pc = block->trackedPc();
        location_.script = block->info().script();
    }
}

bool
MGetPropertyCache::updateForReplacement(MDefinition *ins) {
    MGetPropertyCache *other = ins->toGetPropertyCache();
    location_.append(&other->location_);
    return true;
}

MDefinition *
MAsmJSUnsignedToDouble::foldsTo(TempAllocator &alloc)
{
    if (input()->isConstant()) {
        const Value &v = input()->toConstant()->value();
        if (v.isInt32())
            return MConstant::New(alloc, DoubleValue(uint32_t(v.toInt32())));
    }

    return this;
}

MDefinition *
MAsmJSUnsignedToFloat32::foldsTo(TempAllocator &alloc)
{
    if (input()->isConstant()) {
        const Value &v = input()->toConstant()->value();
        if (v.isInt32()) {
            double dval = double(uint32_t(v.toInt32()));
            if (IsFloat32Representable(dval))
                return MConstant::NewAsmJS(alloc, JS::Float32Value(float(dval)), MIRType_Float32);
        }
    }

    return this;
}

MAsmJSCall *
MAsmJSCall::New(TempAllocator &alloc, const CallSiteDesc &desc, Callee callee,
                const Args &args, MIRType resultType, size_t spIncrement)
{
    MAsmJSCall *call = new(alloc) MAsmJSCall(desc, callee, spIncrement);
    call->setResultType(resultType);

    if (!call->argRegs_.init(alloc, args.length()))
        return nullptr;
    for (size_t i = 0; i < call->argRegs_.length(); i++)
        call->argRegs_[i] = args[i].reg;

    if (!call->init(alloc, call->argRegs_.length() + (callee.which() == Callee::Dynamic ? 1 : 0)))
        return nullptr;
    
    for (size_t i = 0; i < call->argRegs_.length(); i++)
        call->initOperand(i, args[i].def);
    if (callee.which() == Callee::Dynamic)
        call->initOperand(call->argRegs_.length(), callee.dynamic());

    return call;
}

void
MSqrt::trySpecializeFloat32(TempAllocator &alloc) {
    if (!input()->canProduceFloat32() || !CheckUsesAreFloat32Consumers(this)) {
        if (input()->type() == MIRType_Float32)
            ConvertDefinitionToDouble<0>(alloc, input(), this);
        return;
    }

    setResultType(MIRType_Float32);
    setPolicyType(MIRType_Float32);
}

MDefinition *
MClz::foldsTo(TempAllocator &alloc)
{
    if (num()->isConstant()) {
        int32_t n = num()->toConstant()->value().toInt32();
        if (n == 0)
            return MConstant::New(alloc, Int32Value(32));
        return MConstant::New(alloc, Int32Value(mozilla::CountLeadingZeroes32(n)));
    }

    return this;
}

MDefinition *
MBoundsCheck::foldsTo(TempAllocator &alloc)
{
    if (index()->isConstant() && length()->isConstant()) {
       uint32_t len = length()->toConstant()->value().toInt32();
       uint32_t idx = index()->toConstant()->value().toInt32();
       if (idx + uint32_t(minimum()) < len && idx + uint32_t(maximum()) < len)
           return index();
    }

    return this;
}

MDefinition *
MArrayJoin::foldsTo(TempAllocator &alloc) {
    
    return this;

    MDefinition *arr = array();

    if (!arr->isStringSplit())
        return this;

    this->setRecoveredOnBailout();
    if (arr->hasLiveDefUses()) {
        this->setNotRecoveredOnBailout();
        return this;
    }

    
    
    
    
    
    MDefinition *string = arr->toStringSplit()->string();
    MDefinition *pattern = arr->toStringSplit()->separator();
    MDefinition *replacement = sep();

    setNotRecoveredOnBailout();
    return MStringReplace::New(alloc, string, pattern, replacement);
}

bool
jit::ElementAccessIsDenseNative(MDefinition *obj, MDefinition *id)
{
    if (obj->mightBeType(MIRType_String))
        return false;

    if (id->type() != MIRType_Int32 && id->type() != MIRType_Double)
        return false;

    types::TemporaryTypeSet *types = obj->resultTypeSet();
    if (!types)
        return false;

    
    const Class *clasp = types->getKnownClass();
    return clasp && clasp->isNative() && !IsTypedArrayClass(clasp);
}

bool
jit::ElementAccessIsTypedArray(MDefinition *obj, MDefinition *id,
                               Scalar::Type *arrayType)
{
    if (obj->mightBeType(MIRType_String))
        return false;

    if (id->type() != MIRType_Int32 && id->type() != MIRType_Double)
        return false;

    types::TemporaryTypeSet *types = obj->resultTypeSet();
    if (!types)
        return false;

    *arrayType = types->getTypedArrayType();
    return *arrayType != Scalar::TypeMax;
}

bool
jit::ElementAccessIsPacked(types::CompilerConstraintList *constraints, MDefinition *obj)
{
    types::TemporaryTypeSet *types = obj->resultTypeSet();
    return types && !types->hasObjectFlags(constraints, types::OBJECT_FLAG_NON_PACKED);
}

bool
jit::ElementAccessMightBeCopyOnWrite(types::CompilerConstraintList *constraints, MDefinition *obj)
{
    types::TemporaryTypeSet *types = obj->resultTypeSet();
    return !types || types->hasObjectFlags(constraints, types::OBJECT_FLAG_COPY_ON_WRITE);
}

bool
jit::ElementAccessHasExtraIndexedProperty(types::CompilerConstraintList *constraints,
                                          MDefinition *obj)
{
    types::TemporaryTypeSet *types = obj->resultTypeSet();

    if (!types || types->hasObjectFlags(constraints, types::OBJECT_FLAG_LENGTH_OVERFLOW))
        return true;

    return types::TypeCanHaveExtraIndexedProperties(constraints, types);
}

MIRType
jit::DenseNativeElementType(types::CompilerConstraintList *constraints, MDefinition *obj)
{
    types::TemporaryTypeSet *types = obj->resultTypeSet();
    MIRType elementType = MIRType_None;
    unsigned count = types->getObjectCount();

    for (unsigned i = 0; i < count; i++) {
        types::TypeObjectKey *object = types->getObject(i);
        if (!object)
            continue;

        if (object->unknownProperties())
            return MIRType_None;

        types::HeapTypeSetKey elementTypes = object->property(JSID_VOID);

        MIRType type = elementTypes.knownMIRType(constraints);
        if (type == MIRType_None)
            return MIRType_None;

        if (elementType == MIRType_None)
            elementType = type;
        else if (elementType != type)
            return MIRType_None;
    }

    return elementType;
}

static BarrierKind
PropertyReadNeedsTypeBarrier(types::CompilerConstraintList *constraints,
                             types::TypeObjectKey *object, PropertyName *name,
                             types::TypeSet *observed)
{
    
    
    
    
    
    
    
    
    if (object->unknownProperties() || observed->empty() ||
        object->clasp()->isProxy())
    {
        return BarrierKind::TypeSet;
    }

    jsid id = name ? NameToId(name) : JSID_VOID;
    types::HeapTypeSetKey property = object->property(id);
    if (property.maybeTypes()) {
        if (!TypeSetIncludes(observed, MIRType_Value, property.maybeTypes())) {
            
            
            if (property.maybeTypes()->objectsAreSubset(observed)) {
                property.freeze(constraints);
                return BarrierKind::TypeTagOnly;
            }
            return BarrierKind::TypeSet;
        }
    }

    
    
    
    
    if (JSObject *obj = object->singleton()) {
        if (name && types::CanHaveEmptyPropertyTypesForOwnProperty(obj) &&
            (!property.maybeTypes() || property.maybeTypes()->empty()))
        {
            return BarrierKind::TypeSet;
        }
    }

    property.freeze(constraints);
    return BarrierKind::NoBarrier;
}

BarrierKind
jit::PropertyReadNeedsTypeBarrier(types::CompilerConstraintList *constraints,
                                  types::TypeObjectKey *object, PropertyName *name,
                                  types::TemporaryTypeSet *observed, bool updateObserved)
{
    
    
    if (updateObserved && observed->empty() && name) {
        JSObject *obj;
        if (object->singleton())
            obj = object->singleton();
        else if (object->hasTenuredProto())
            obj = object->proto().toObjectOrNull();
        else
            obj = nullptr;

        while (obj) {
            if (!obj->getClass()->isNative())
                break;

            types::TypeObjectKey *typeObj = types::TypeObjectKey::get(obj);

            if (!typeObj->unknownProperties()) {
                types::HeapTypeSetKey property = typeObj->property(NameToId(name));
                if (property.maybeTypes()) {
                    types::TypeSet::TypeList types;
                    if (!property.maybeTypes()->enumerateTypes(&types))
                        break;
                    if (types.length()) {
                        
                        observed->addType(types[0], GetIonContext()->temp->lifoAlloc());
                        break;
                    }
                }
            }

            if (!obj->hasTenuredProto())
                break;
            obj = obj->getProto();
        }
    }

    return PropertyReadNeedsTypeBarrier(constraints, object, name, observed);
}

BarrierKind
jit::PropertyReadNeedsTypeBarrier(types::CompilerConstraintList *constraints,
                                  MDefinition *obj, PropertyName *name,
                                  types::TemporaryTypeSet *observed)
{
    if (observed->unknown())
        return BarrierKind::NoBarrier;

    types::TypeSet *types = obj->resultTypeSet();
    if (!types || types->unknownObject())
        return BarrierKind::TypeSet;

    BarrierKind res = BarrierKind::NoBarrier;

    bool updateObserved = types->getObjectCount() == 1;
    for (size_t i = 0; i < types->getObjectCount(); i++) {
        types::TypeObjectKey *object = types->getObject(i);
        if (object) {
            BarrierKind kind = PropertyReadNeedsTypeBarrier(constraints, object, name,
                                                            observed, updateObserved);
            if (kind == BarrierKind::TypeSet)
                return BarrierKind::TypeSet;

            if (kind == BarrierKind::TypeTagOnly) {
                MOZ_ASSERT(res == BarrierKind::NoBarrier || res == BarrierKind::TypeTagOnly);
                res = BarrierKind::TypeTagOnly;
            } else {
                MOZ_ASSERT(kind == BarrierKind::NoBarrier);
            }
        }
    }

    return res;
}

BarrierKind
jit::PropertyReadOnPrototypeNeedsTypeBarrier(types::CompilerConstraintList *constraints,
                                             MDefinition *obj, PropertyName *name,
                                             types::TemporaryTypeSet *observed)
{
    if (observed->unknown())
        return BarrierKind::NoBarrier;

    types::TypeSet *types = obj->resultTypeSet();
    if (!types || types->unknownObject())
        return BarrierKind::TypeSet;

    BarrierKind res = BarrierKind::NoBarrier;

    for (size_t i = 0; i < types->getObjectCount(); i++) {
        types::TypeObjectKey *object = types->getObject(i);
        if (!object)
            continue;
        while (true) {
            if (!object->hasTenuredProto())
                return BarrierKind::TypeSet;
            if (!object->proto().isObject())
                break;
            object = types::TypeObjectKey::get(object->proto().toObject());
            BarrierKind kind = PropertyReadNeedsTypeBarrier(constraints, object, name, observed);
            if (kind == BarrierKind::TypeSet)
                return BarrierKind::TypeSet;

            if (kind == BarrierKind::TypeTagOnly) {
                MOZ_ASSERT(res == BarrierKind::NoBarrier || res == BarrierKind::TypeTagOnly);
                res = BarrierKind::TypeTagOnly;
            } else {
                MOZ_ASSERT(kind == BarrierKind::NoBarrier);
            }
        }
    }

    return res;
}

bool
jit::PropertyReadIsIdempotent(types::CompilerConstraintList *constraints,
                              MDefinition *obj, PropertyName *name)
{
    

    types::TypeSet *types = obj->resultTypeSet();
    if (!types || types->unknownObject())
        return false;

    for (size_t i = 0; i < types->getObjectCount(); i++) {
        types::TypeObjectKey *object = types->getObject(i);
        if (object) {
            if (object->unknownProperties())
                return false;

            
            types::HeapTypeSetKey property = object->property(NameToId(name));
            if (property.nonData(constraints))
                return false;
        }
    }

    return true;
}

void
jit::AddObjectsForPropertyRead(MDefinition *obj, PropertyName *name,
                               types::TemporaryTypeSet *observed)
{
    
    

    LifoAlloc *alloc = GetIonContext()->temp->lifoAlloc();

    types::TemporaryTypeSet *types = obj->resultTypeSet();
    if (!types || types->unknownObject()) {
        observed->addType(types::Type::AnyObjectType(), alloc);
        return;
    }

    for (size_t i = 0; i < types->getObjectCount(); i++) {
        types::TypeObjectKey *object = types->getObject(i);
        if (!object)
            continue;

        if (object->unknownProperties()) {
            observed->addType(types::Type::AnyObjectType(), alloc);
            return;
        }

        jsid id = name ? NameToId(name) : JSID_VOID;
        types::HeapTypeSetKey property = object->property(id);
        types::HeapTypeSet *types = property.maybeTypes();
        if (!types)
            continue;

        if (types->unknownObject()) {
            observed->addType(types::Type::AnyObjectType(), alloc);
            return;
        }

        for (size_t i = 0; i < types->getObjectCount(); i++) {
            types::TypeObjectKey *object = types->getObject(i);
            if (object)
                observed->addType(types::Type::ObjectType(object), alloc);
        }
    }
}

static bool
TryAddTypeBarrierForWrite(TempAllocator &alloc, types::CompilerConstraintList *constraints,
                          MBasicBlock *current, types::TemporaryTypeSet *objTypes,
                          PropertyName *name, MDefinition **pvalue)
{
    
    
    

    
    
    
    Maybe<types::HeapTypeSetKey> aggregateProperty;

    for (size_t i = 0; i < objTypes->getObjectCount(); i++) {
        types::TypeObjectKey *object = objTypes->getObject(i);
        if (!object)
            continue;

        if (object->unknownProperties())
            return false;

        jsid id = name ? NameToId(name) : JSID_VOID;
        types::HeapTypeSetKey property = object->property(id);
        if (!property.maybeTypes() || property.couldBeConstant(constraints))
            return false;

        if (TypeSetIncludes(property.maybeTypes(), (*pvalue)->type(), (*pvalue)->resultTypeSet()))
            return false;

        
        
        
        property.freeze(constraints);

        if (!aggregateProperty) {
            aggregateProperty.emplace(property);
        } else {
            if (!aggregateProperty->maybeTypes()->isSubset(property.maybeTypes()) ||
                !property.maybeTypes()->isSubset(aggregateProperty->maybeTypes()))
            {
                return false;
            }
        }
    }

    JS_ASSERT(aggregateProperty);

    MIRType propertyType = aggregateProperty->knownMIRType(constraints);
    switch (propertyType) {
      case MIRType_Boolean:
      case MIRType_Int32:
      case MIRType_Double:
      case MIRType_String:
      case MIRType_Symbol: {
        
        
        if (!(*pvalue)->mightBeType(propertyType)) {
            
            
            JS_ASSERT_IF((*pvalue)->type() != MIRType_Value, (*pvalue)->type() != propertyType);
            return false;
        }
        MInstruction *ins = MUnbox::New(alloc, *pvalue, propertyType, MUnbox::Fallible);
        current->add(ins);
        *pvalue = ins;
        return true;
      }
      default:;
    }

    if ((*pvalue)->type() != MIRType_Value)
        return false;

    types::TemporaryTypeSet *types = aggregateProperty->maybeTypes()->clone(alloc.lifoAlloc());
    if (!types)
        return false;

    
    
    BarrierKind kind = BarrierKind::TypeSet;
    if ((*pvalue)->resultTypeSet() && (*pvalue)->resultTypeSet()->objectsAreSubset(types))
        kind = BarrierKind::TypeTagOnly;

    MInstruction *ins = MMonitorTypes::New(alloc, *pvalue, types, kind);
    current->add(ins);
    return true;
}

static MInstruction *
AddTypeGuard(TempAllocator &alloc, MBasicBlock *current, MDefinition *obj,
             types::TypeObjectKey *type, bool bailOnEquality)
{
    MInstruction *guard;

    if (type->isTypeObject())
        guard = MGuardObjectType::New(alloc, obj, type->asTypeObject(), bailOnEquality);
    else
        guard = MGuardObjectIdentity::New(alloc, obj, type->asSingleObject(), bailOnEquality);

    current->add(guard);

    
    guard->setNotMovable();

    return guard;
}


bool
jit::CanWriteProperty(types::CompilerConstraintList *constraints,
                      types::HeapTypeSetKey property, MDefinition *value)
{
    if (property.couldBeConstant(constraints))
        return false;
    return TypeSetIncludes(property.maybeTypes(), value->type(), value->resultTypeSet());
}

bool
jit::PropertyWriteNeedsTypeBarrier(TempAllocator &alloc, types::CompilerConstraintList *constraints,
                                   MBasicBlock *current, MDefinition **pobj,
                                   PropertyName *name, MDefinition **pvalue, bool canModify)
{
    
    
    
    
    

    types::TemporaryTypeSet *types = (*pobj)->resultTypeSet();
    if (!types || types->unknownObject())
        return true;

    
    
    
    

    bool success = true;
    for (size_t i = 0; i < types->getObjectCount(); i++) {
        types::TypeObjectKey *object = types->getObject(i);
        if (!object || object->unknownProperties())
            continue;

        
        
        if (IsTypedArrayClass(object->clasp()))
            continue;

        jsid id = name ? NameToId(name) : JSID_VOID;
        types::HeapTypeSetKey property = object->property(id);
        if (!CanWriteProperty(constraints, property, *pvalue)) {
            
            
            
            
            if (!canModify)
                return true;
            success = TryAddTypeBarrierForWrite(alloc, constraints, current, types, name, pvalue);
            break;
        }
    }

    if (success)
        return false;

    
    
    

    if (types->getObjectCount() <= 1)
        return true;

    types::TypeObjectKey *excluded = nullptr;
    for (size_t i = 0; i < types->getObjectCount(); i++) {
        types::TypeObjectKey *object = types->getObject(i);
        if (!object || object->unknownProperties())
            continue;
        if (IsTypedArrayClass(object->clasp()))
            continue;

        jsid id = name ? NameToId(name) : JSID_VOID;
        types::HeapTypeSetKey property = object->property(id);
        if (CanWriteProperty(constraints, property, *pvalue))
            continue;

        if ((property.maybeTypes() && !property.maybeTypes()->empty()) || excluded)
            return true;
        excluded = object;
    }

    JS_ASSERT(excluded);

    *pobj = AddTypeGuard(alloc, current, *pobj, excluded,  true);
    return false;
}
