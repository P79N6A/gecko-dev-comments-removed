








































#ifndef jsion_codegen_h__
#define jsion_codegen_h__

#if defined(JS_CPU_X86)
# include "x86/CodeGenerator-x86.h"
#elif defined(JS_CPU_X64)
# include "x64/CodeGenerator-x64.h"
#elif defined(JS_CPU_ARM)
# include "arm/CodeGenerator-arm.h"
#else
#error "CPU Not Supported"
#endif

namespace js {
namespace ion {

class CheckOverRecursedFailure;
class OutOfLineUnboxDouble;
class OutOfLineCache;
class OutOfLineStoreElementHole;

class CodeGenerator : public CodeGeneratorSpecific
{
    bool generateArgumentsChecks();
    bool generateInvalidateEpilogue();
    bool generateBody();

  public:
    CodeGenerator(MIRGenerator *gen, LIRGraph &graph);

  public:
    bool generate();

    bool visitLabel(LLabel *lir);
    bool visitNop(LNop *lir);
    bool visitOsiPoint(LOsiPoint *lir);
    bool visitGoto(LGoto *lir);
    bool visitParameter(LParameter *lir);
    bool visitCallee(LCallee *lir);
    bool visitStart(LStart *lir);
    bool visitReturn(LReturn *ret);
    bool visitOsrEntry(LOsrEntry *lir);
    bool visitOsrScopeChain(LOsrScopeChain *lir);
    bool visitStackArg(LStackArg *lir);
    bool visitValueToInt32(LValueToInt32 *lir);
    bool visitValueToDouble(LValueToDouble *lir);
    bool visitInt32ToDouble(LInt32ToDouble *lir);
    bool visitTestVAndBranch(LTestVAndBranch *lir);
    bool visitTruncateDToInt32(LTruncateDToInt32 *lir);
    bool visitIntToString(LIntToString *lir);
    bool visitInteger(LInteger *lir);
    bool visitRegExp(LRegExp *lir);
    bool visitPointer(LPointer *lir);
    bool visitSlots(LSlots *lir);
    bool visitStoreSlotV(LStoreSlotV *store);
    bool visitElements(LElements *lir);
    bool visitTypeBarrier(LTypeBarrier *lir);
    bool visitCallGeneric(LCallGeneric *lir);
    bool visitDoubleToInt32(LDoubleToInt32 *lir);
    bool visitNewArray(LNewArray *builder);
    bool visitArrayLength(LArrayLength *lir);
    bool visitStringLength(LStringLength *lir);
    bool visitInitializedLength(LInitializedLength *lir);
    bool visitBoundsCheck(LBoundsCheck *lir);
    bool visitBoundsCheckRange(LBoundsCheckRange *lir);
    bool visitBoundsCheckLower(LBoundsCheckLower *lir);
    bool visitLoadFixedSlotV(LLoadFixedSlotV *ins);
    bool visitLoadFixedSlotT(LLoadFixedSlotT *ins);
    bool visitStoreFixedSlotV(LStoreFixedSlotV *ins);
    bool visitStoreFixedSlotT(LStoreFixedSlotT *ins);
    bool visitBinaryV(LBinaryV *lir);
    bool visitCompareV(LCompareV *lir);
    bool visitConcat(LConcat *lir);
    bool visitFunctionEnvironment(LFunctionEnvironment *lir);
    bool visitCallGetProperty(LCallGetProperty *lir);
    bool visitCallGetName(LCallGetName *lir);
    bool visitCallGetNameTypeOf(LCallGetNameTypeOf *lir);
    bool visitCallGetElement(LCallGetElement *lir);
    bool visitCallSetElement(LCallSetElement *lir);
    bool visitThrow(LThrow *lir);
    bool visitLoadElementV(LLoadElementV *load);
    bool visitLoadElementHole(LLoadElementHole *lir);
    bool visitStoreElementT(LStoreElementT *lir);
    bool visitStoreElementV(LStoreElementV *lir);
    bool visitStoreElementHoleT(LStoreElementHoleT *lir);
    bool visitStoreElementHoleV(LStoreElementHoleV *lir);

    bool visitCheckOverRecursed(LCheckOverRecursed *lir);
    bool visitCheckOverRecursedFailure(CheckOverRecursedFailure *ool);

    bool visitUnboxDouble(LUnboxDouble *lir);
    bool visitOutOfLineUnboxDouble(OutOfLineUnboxDouble *ool);
    bool visitOutOfLineCacheGetProperty(OutOfLineCache *ool);
    bool visitOutOfLineCacheSetProperty(OutOfLineCache *ool);
    bool visitOutOfLineStoreElementHole(OutOfLineStoreElementHole *ool);

    bool visitGetPropertyCacheV(LGetPropertyCacheV *ins) {
        return visitCache(ins);
    }
    bool visitGetPropertyCacheT(LGetPropertyCacheT *ins) {
        return visitCache(ins);
    }
    bool visitCacheSetPropertyV(LCacheSetPropertyV *ins) {
        return visitCache(ins);
    }
    bool visitCacheSetPropertyT(LCacheSetPropertyT *ins) {
        return visitCache(ins);
    }

    bool visitCallSetPropertyV(LCallSetPropertyV *ins) {
        return visitCallSetProperty(ins);
    }
    bool visitCallSetPropertyT(LCallSetPropertyT *ins) {
        return visitCallSetProperty(ins);
    }

  private:
    bool visitCache(LInstruction *load);
    bool visitCallSetProperty(LInstruction *ins);

    ConstantOrRegister getSetPropertyValue(LInstruction *ins);
};

} 
} 

#endif 

