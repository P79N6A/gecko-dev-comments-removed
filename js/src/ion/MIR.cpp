







































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
    printOpcode(fp);
    fprintf(fp, "%u", id());
    fprintf(fp, ":v");
}

void
MInstruction::printOpcode(FILE *fp)
{
    PrintOpcodeName(fp, op());
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

void
MInstruction::removeUse(MUse *prev, MUse *use)
{
    if (!prev) {
        JS_ASSERT(uses_ = use);
        uses_ = use->next();
    } else {
        JS_ASSERT(prev->next() == use);
        prev->next_ = use->next();
    }
}

void
MInstruction::replaceOperand(MUse *prev, MUse *use, MInstruction *ins)
{
    MInstruction *used = getOperand(use->index())->ins();
    if (used == ins)
        return;

    used->removeUse(prev, use);
    setOperand(use->index(), ins);
    ins->addUse(use, ins);
}

void
MInstruction::setOperand(size_t index, MInstruction *ins)
{
    getOperand(index)->setInstruction(ins);
}

MConstant *
MConstant::New(MIRGenerator *gen, const Value &v)
{
    return new (gen->temp()) MConstant(v);
}

MConstant::MConstant(const js::Value &vp)
  : value_(vp)
{
    setType(MIRTypeFromValue(vp));
    switch (type()) {
      case MIRType_Int32:
      case MIRType_Boolean:
      case MIRType_Magic:
        
        break;
      case MIRType_Double:
        
        break;
      case MIRType_String:
      case MIRType_Object:
      case MIRType_Function:
        
        break;
      case MIRType_Undefined:
      case MIRType_Null:
        
        
        break;
      default:
        JS_NOT_REACHED("unexpected constant type");
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
        ins = ins->toCopy()->getOperand(0)->ins();

    MCopy *copy = new (gen->temp()) MCopy();
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
        if (getOperand(i)->ins() == ins)
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

