








































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

    bool useBoxAtStart(LInstruction *lir, size_t n, MDefinition *mir,
                       LUse::Policy policy = LUse::REGISTER) {
        return useBox(lir, n, mir, policy, true);
    }

    bool lowerBitOp(JSOp op, MInstruction *ins);
    bool lowerShiftOp(JSOp op, MInstruction *ins);
    bool lowerBinaryV(JSOp op, MBinaryInstruction *ins);
    bool precreatePhi(LBlock *block, MPhi *phi);
    bool definePhis();

    
    void allocateArguments(uint32 argc);
    
    
    uint32 getArgumentSlot(uint32 argnum);
    uint32 getArgumentSlotForCall() { return argslots_; }
    
    void freeArguments(uint32 argc);

  public:
    bool visitInstruction(MInstruction *ins);
    bool visitBlock(MBasicBlock *block);

    
    
    bool visitParameter(MParameter *param);
    bool visitCallee(MCallee *callee);
    bool visitGoto(MGoto *ins);
    bool visitNewArray(MNewArray *ins);
    bool visitNewObject(MNewObject *ins);
    bool visitCheckOverRecursed(MCheckOverRecursed *ins);
    bool visitDefVar(MDefVar *ins);
    bool visitPrepareCall(MPrepareCall *ins);
    bool visitPassArg(MPassArg *arg);
    bool visitCall(MCall *call);
    bool visitTest(MTest *test);
    bool visitCompare(MCompare *comp);
    bool visitTypeOf(MTypeOf *ins);
    bool visitToId(MToId *ins);
    bool visitBitNot(MBitNot *ins);
    bool visitBitAnd(MBitAnd *ins);
    bool visitBitOr(MBitOr *ins);
    bool visitBitXor(MBitXor *ins);
    bool visitLsh(MLsh *ins);
    bool visitRsh(MRsh *ins);
    bool visitUrsh(MUrsh *ins);
    bool visitRound(MRound *ins);
    bool visitAbs(MAbs *ins);
    bool visitAdd(MAdd *ins);
    bool visitSub(MSub *ins);
    bool visitMul(MMul *ins);
    bool visitDiv(MDiv *ins);
    bool visitMod(MMod *ins);
    bool visitConcat(MConcat *ins);
    bool visitCharCodeAt(MCharCodeAt *ins);
    bool visitFromCharCode(MFromCharCode *ins);
    bool visitStart(MStart *start);
    bool visitOsrEntry(MOsrEntry *entry);
    bool visitOsrValue(MOsrValue *value);
    bool visitOsrScopeChain(MOsrScopeChain *object);
    bool visitToDouble(MToDouble *convert);
    bool visitToInt32(MToInt32 *convert);
    bool visitTruncateToInt32(MTruncateToInt32 *truncate);
    bool visitToString(MToString *convert);
    bool visitRegExp(MRegExp *ins);
    bool visitLambda(MLambda *ins);
    bool visitLambdaJoinableForCall(MLambdaJoinableForCall *ins);
    bool visitLambdaJoinableForSet(MLambdaJoinableForSet *ins);
    bool visitImplicitThis(MImplicitThis *ins);
    bool visitSlots(MSlots *ins);
    bool visitElements(MElements *ins);
    bool visitFlatClosureUpvars(MFlatClosureUpvars *ins);
    bool visitLoadSlot(MLoadSlot *ins);
    bool visitFunctionEnvironment(MFunctionEnvironment *ins);
    bool visitStoreSlot(MStoreSlot *ins);
    bool visitTypeBarrier(MTypeBarrier *ins);
    bool visitMonitorTypes(MMonitorTypes *ins);
    bool visitArrayLength(MArrayLength *ins);
    bool visitInitializedLength(MInitializedLength *ins);
    bool visitSetInitializedLength(MSetInitializedLength *ins);
    bool visitNot(MNot *ins);
    bool visitBoundsCheck(MBoundsCheck *ins);
    bool visitBoundsCheckLower(MBoundsCheckLower *ins);
    bool visitLoadElement(MLoadElement *ins);
    bool visitLoadElementHole(MLoadElementHole *ins);
    bool visitStoreElement(MStoreElement *ins);
    bool visitStoreElementHole(MStoreElementHole *ins);
    bool visitLoadFixedSlot(MLoadFixedSlot *ins);
    bool visitStoreFixedSlot(MStoreFixedSlot *ins);
    bool visitGetPropertyCache(MGetPropertyCache *ins);
    bool visitBindNameCache(MBindNameCache *ins);
    bool visitGuardClass(MGuardClass *ins);
    bool visitCallGetProperty(MCallGetProperty *ins);
    bool visitCallGetName(MCallGetName *ins);
    bool visitCallGetNameTypeOf(MCallGetNameTypeOf *ins);
    bool visitCallGetElement(MCallGetElement *ins);
    bool visitCallSetElement(MCallSetElement *ins);
    bool visitSetPropertyCache(MSetPropertyCache *ins);
    bool visitCallSetProperty(MCallSetProperty *ins);
    bool visitIteratorStart(MIteratorStart *ins);
    bool visitIteratorNext(MIteratorNext *ins);
    bool visitIteratorMore(MIteratorMore *ins);
    bool visitIteratorEnd(MIteratorEnd *ins);
    bool visitStringLength(MStringLength *ins);
    bool visitThrow(MThrow *ins);

    bool visitGuardObject(MGuardObject *ins) {
        
        return true;
    }
};

} 
} 

#endif 

