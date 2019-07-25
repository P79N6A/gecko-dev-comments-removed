







































#include "MIR.h"

using namespace js;
using namespace js::ion;

MIRGenerator::MIRGenerator(JSContext *cx, TempAllocator &temp, JSScript *script, JSFunction *fun,
                           MIRGraph &graph)
  : cx(cx),
    script(script),
    pc(NULL),
    temp_(temp),
    fun_(fun),
    graph_(graph)
{
    nslots_ = script->nslots + (fun ? fun->nargs + 2 : 0);
}

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
    if (assumedType() != MIRType_None)
        fprintf(fp, "%s:", MirTypeNames[assumedType()]);
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
    JS_ASSERT(use->ins() == this);

    MUse *old = use.unlink();
    setOperand(old->index(), ins);
    ins->addUse(old);
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
MConstant::New(MIRGenerator *gen, const Value &v)
{
    return new (gen->temp()) MConstant(v);
}

MConstant::MConstant(const js::Value &vp)
  : value_(vp)
{
    setResultType(MIRTypeFromValue(vp));
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
MParameter::New(MIRGenerator *gen, int32 index)
{
    JS_ASSERT(index >= CALLEE_SLOT && index < int32(gen->fun()->nargs));
    return new (gen->temp()) MParameter(index);
}

MCopy *
MCopy::New(MIRGenerator *gen, MInstruction *ins)
{
    if (!ins)
        return NULL;

    
    if (ins->isCopy())
        ins = ins->toCopy()->getInput(0);

    MCopy *copy = new (gen->temp()) MCopy(ins);
    if (!copy || !copy->init(gen, ins))
        return NULL;

    return copy;
}

MTest *
MTest::New(MIRGenerator *gen, MInstruction *ins, MBasicBlock *ifTrue, MBasicBlock *ifFalse)
{
    if (!ins || !ifTrue || !ifFalse)
        return NULL;
    MTest *test = new (gen->temp()) MTest(ifTrue, ifFalse);
    if (!test || !test->initOperand(gen, 0, ins))
        return NULL;
    return test;
}

MGoto *
MGoto::New(MIRGenerator *gen, MBasicBlock *target)
{
    if (!target)
        return NULL;
    return new (gen->temp()) MGoto(target);
}

MPhi *
MPhi::New(MIRGenerator *gen, uint32 slot)
{
    return new (gen->temp()) MPhi(gen->cx, slot);
}

bool
MPhi::addInput(MIRGenerator *gen, MInstruction *ins)
{
    for (size_t i = 0; i < inputs_.length(); i++) {
        if (getInput(i) == ins)
            return true;
    }

    if (!ins->addUse(gen, this, inputs_.length()))
        return false;

    MOperand *operand = MOperand::New(gen, ins);
    if (!operand || !inputs_.append(operand))
        return false;
    return true;
}

MReturn *
MReturn::New(MIRGenerator *gen, MInstruction *ins)
{
    MReturn *ret = new (gen->temp()) MReturn();
    if (!ret || !ret->initOperand(gen, 0, ins))
        return NULL;
    return ret;
}

MBitAnd *
MBitAnd::New(MIRGenerator *gen, MInstruction *left, MInstruction *right)
{
    MBitAnd *ins = new (gen->temp()) MBitAnd();
    if (!ins || !ins->init(gen, left, right))
        return NULL;
    return ins;
}

