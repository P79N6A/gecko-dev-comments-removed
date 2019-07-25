








































#ifndef jsion_ion_lowering_h__
#define jsion_ion_lowering_h__




#include "IonAllocPolicy.h"
#include "LIR.h"
#include "MOpcodes.h"

#if defined(JS_CPU_X86)
# include "x86/Lowering-x86.h"
#elif defined(JS_CPU_X64)
# include "x64/Lowering-x64.h"
#elif defined(JS_CPU_ARM)
# include "arm/Lowering-arm.h"
#else
# error "CPU!"
#endif

namespace js {
namespace ion {

class LIRGenerator : public LIRGeneratorSpecific
{
    void updateResumeState(MInstruction *ins);
    void updateResumeState(MBasicBlock *block);

    
    uint32 argslots_;
    
    uint32 maxargslots_;

  public:
    LIRGenerator(MIRGenerator *gen, MIRGraph &graph, LIRGraph &lirGraph)
      : LIRGeneratorSpecific(gen, graph, lirGraph),
        argslots_(0), maxargslots_(0)
    { }

    bool generate();

  private:
    bool lowerBitOp(JSOp op, MInstruction *ins);
    bool lowerShiftOp(JSOp op, MInstruction *ins);
    bool precreatePhi(LBlock *block, MPhi *phi);
    bool definePhis();

    
    void allocateArguments(uint32 argc);
    
    
    uint32 getArgumentSlot(uint32 argnum);
    uint32 getArgumentSlotForCall() { return argslots_; }
    
    void freeArguments(uint32 argc);

    
    bool emitWriteBarrier(MInstruction *ins, MDefinition *input);

  public:
    bool visitInstruction(MInstruction *ins);
    bool visitBlock(MBasicBlock *block);

    
    
    bool visitParameter(MParameter *param);
    bool visitGoto(MGoto *ins);
    bool visitNewArray(MNewArray *ins);
    bool visitPrepareCall(MPrepareCall *ins);
    bool visitPassArg(MPassArg *arg);
    bool visitCall(MCall *call);
    bool visitTest(MTest *test);
    bool visitCompare(MCompare *comp);
    bool visitBitNot(MBitNot *ins);
    bool visitBitAnd(MBitAnd *ins);
    bool visitBitOr(MBitOr *ins);
    bool visitBitXor(MBitXor *ins);
    bool visitLsh(MLsh *ins);
    bool visitRsh(MRsh *ins);
    bool visitUrsh(MUrsh *ins);
    bool visitAdd(MAdd *ins);
    bool visitSub(MSub *ins);
    bool visitMul(MMul *ins);
    bool visitDiv(MDiv *ins);
    bool visitStart(MStart *start);
    bool visitOsrEntry(MOsrEntry *entry);
    bool visitOsrValue(MOsrValue *value);
    bool visitToDouble(MToDouble *convert);
    bool visitToInt32(MToInt32 *convert);
    bool visitTruncateToInt32(MTruncateToInt32 *truncate);
    bool visitCopy(MCopy *ins);
    bool visitImplicitThis(MImplicitThis *ins);
    bool visitSlots(MSlots *ins);
    bool visitElements(MElements *ins);
    bool visitLoadSlot(MLoadSlot *ins);
    bool visitStoreSlot(MStoreSlot *ins);
    bool visitTypeBarrier(MTypeBarrier *ins);
    bool visitInitializedLength(MInitializedLength *ins);
    bool visitBoundsCheck(MBoundsCheck *ins);
    bool visitLoadElement(MLoadElement *ins);
    bool visitStoreElement(MStoreElement *ins);
    bool visitGuardClass(MGuardClass *ins);
    bool visitLoadProperty(MLoadProperty *ins);
};

} 
} 

#endif 

