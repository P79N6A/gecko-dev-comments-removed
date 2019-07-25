








































#include "MIR.h"
#include "MIRGraph.h"
#include "IonLIR.h"
#include "IonLIR-inl.h"

using namespace js;
using namespace js::ion;

LSnapshot::LSnapshot(MSnapshot *mir)
  : numSlots_(mir->numOperands() * BOX_PIECES),
    slots_(NULL),
    mir_(mir)
{ }

bool
LSnapshot::init(MIRGenerator *gen)
{
    slots_ = gen->allocate<LAllocation>(numSlots_);
    return !!slots_;
}

LSnapshot *
LSnapshot::New(MIRGenerator *gen, MSnapshot *mir)
{
    LSnapshot *snapshot = new LSnapshot(mir);
    if (!snapshot->init(gen))
        return NULL;
    return snapshot;
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
    "p",            
    "o",            
    "f",            
    "t",            
    "d",            
    "x"             
};

static void
PrintDefinition(FILE *fp, const LDefinition &def)
{
    fprintf(fp, "[%s:%d", TypeChars[def.type()], def.virtualRegister());
    if (def.policy() == LDefinition::PRESET) {
        switch (def.output()->kind()) {
          case LAllocation::CONSTANT_VALUE:
          case LAllocation::CONSTANT_INDEX:
            fprintf(fp, " (c)");
            break;
          case LAllocation::GPR:
            fprintf(fp, " (%s)", def.output()->toGeneralReg()->reg().name());
            break;
          case LAllocation::FPU:
            fprintf(fp, " (%s)", def.output()->toFloatReg()->reg().name());
            break;
          case LAllocation::ARGUMENT:
            fprintf(fp, " (arg %d)", def.output()->toArgument()->index());
            break;
          default:
            JS_NOT_REACHED("unexpected preset allocation type");
            break;
        }
    } else if (def.policy() == LDefinition::CAN_REUSE_INPUT) {
        fprintf(fp, " (r?)");
    } else if (def.policy() == LDefinition::MUST_REUSE_INPUT) {
        fprintf(fp, " (rr)");
    }
    fprintf(fp, "]");
}

static void
PrintUse(FILE *fp, LUse *use)
{
    fprintf(fp, "(v%d:", use->virtualRegister());
    if (use->policy() == LUse::ANY) {
        fprintf(fp, "*");
    } else if (use->policy() == LUse::REGISTER) {
        fprintf(fp, "r");
    } else {
        
        
        
        fprintf(fp, "%s", RegisterCodes::GetName(RegisterCodes::Code(use->registerCode())));
    }
    if (use->killedAtStart())
        fprintf(fp, "!");
    fprintf(fp, ")");
}

void
LInstruction::printOperands(FILE *fp)
{
    for (size_t i = 0; i < numOperands(); i++) {
        fprintf(fp, " ");
        LAllocation *a = getOperand(i);
        switch (a->kind()) {
          case LAllocation::CONSTANT_VALUE:
          case LAllocation::CONSTANT_INDEX:
            fprintf(fp, "(c)");
            break;
          case LAllocation::GPR:
            fprintf(fp, "(=%s)", a->toGeneralReg()->reg().name());
            break;
          case LAllocation::FPU:
            fprintf(fp, "(=%s)", a->toFloatReg()->reg().name());
            break;
          case LAllocation::STACK_SLOT:
            fprintf(fp, "(stack:i%d)", a->toStackSlot()->slot());
            break;
          case LAllocation::DOUBLE_SLOT:
            fprintf(fp, "(stack:d%d)", a->toStackSlot()->slot());
            break;
          case LAllocation::ARGUMENT:
            fprintf(fp, "(arg:%d)", a->toArgument()->index());
            break;
          case LAllocation::USE:
            PrintUse(fp, a->toUse());
            break;
          default:
            JS_NOT_REACHED("what?");
            break;
        }
        if (i != numOperands() - 1)
            fprintf(fp, ",");
    }
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
}

