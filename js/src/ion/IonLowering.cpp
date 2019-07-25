








































#include "IonBuilder.h"
#include "MIRGraph.h"
#include "Ion.h"
#include "IonSpew.h"
#include "IonAnalysis.h"

using namespace js;
using namespace js::ion;



class LoweringPhase : public MInstructionVisitor
{
    MIRGenerator *gen;
    MIRGraph &graph;

  public:
    LoweringPhase(MIRGenerator *gen, MIRGraph &graph)
      : gen(gen),
        graph(graph)
    { }

    bool lowerBlock(MBasicBlock *block);
    bool lowerInstruction(MInstruction *ins);
    bool analyze();
    bool insertBox(MInstruction *def, MOperand *operand);
    bool insertUnbox(MInstruction *def, MOperand *operand, MIRType type);
    bool insertConversion(MInstruction *def, MOperand *operand, MIRType type);

#define VISITOR(op) bool visit(M##op *ins) { return false; }
    MIR_OPCODE_LIST(VISITOR)
#undef VISITOR
};

bool
LoweringPhase::insertBox(MInstruction *def, MOperand *operand)
{
    return false;
}

bool
LoweringPhase::insertUnbox(MInstruction *def, MOperand *operand, MIRType type)
{
    return false;
}

bool
LoweringPhase::insertConversion(MInstruction *def, MOperand *operand, MIRType type)
{
    return false;
}

static inline bool
IsConversionPure(MIRType from, MIRType to)
{
    return from != MIRType_Object;
}

bool
LoweringPhase::lowerInstruction(MInstruction *ins)
{
    MInstruction *narrowed = NULL;
    MIRType usedAs = ins->usedAsType();
    if (usedAs != MIRType_Value && usedAs != ins->type() &&
        IsConversionPure(ins->type(), usedAs)) {
        
        
        
        
        
        
        
        narrowed = MConvert::New(gen, ins, usedAs);
        if (!narrowed)
            return false;
    }

    MUseIterator uses(ins);
    while (uses.more()) {
        MInstruction *use = uses->ins();
        MIRType required = use->requiredInputType(uses->index());

        
        
        JS_ASSERT(ins->type() < MIRType_Any);
        JS_ASSERT(required < MIRType_None);

        
        
        
        
        if (required == ins->type() || (required == MIRType_Any && !narrowed)) {
            uses.next();
            continue;
        }

        
        
        
        
        
        
        if (required == usedAs || (required == MIRType_Any && narrowed)) {
            JS_ASSERT(narrowed);
            use->replaceOperand(uses, narrowed);
            continue;
        }

        
        
        JS_ASSERT_IF(usedAs != MIRType_Value, required == MIRType_Value);

        
        if (required == MIRType_Value) {
            MBox *box = MBox::New(gen, ins);
            use->replaceOperand(uses, box);
            continue;
        }

        
        
        
        
        
        
        JS_ASSERT(required < MIRType_Value);
        MConvert *converted = MConvert::New(gen, ins, required);
        use->replaceOperand(uses, converted);
    }

    
    
    return ins->accept(this);
}

bool
LoweringPhase::lowerBlock(MBasicBlock *block)
{
    
    for (size_t i = 0; i < block->numPhis(); i++) {
        if (!lowerInstruction(block->getPhi(i)))
            return false;
    }
    for (MInstructionIterator i = block->begin(); i != block->end(); i++) {
        if (!lowerInstruction(*i))
            return false;
    }
    return true;
}

bool
LoweringPhase::analyze()
{
    for (size_t i = 0; i < graph.numBlocks(); i++) {
        if (!lowerBlock(graph.getBlock(i)))
            return false;
    }
    return true;
}

bool
ion::Lower(MIRGenerator *gen, MIRGraph &graph)
{
    LoweringPhase phase(gen, graph);
    return phase.analyze();
}

