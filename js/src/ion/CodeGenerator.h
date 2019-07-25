








































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
class OutOfLineGetPropertyCache;

class CodeGenerator : public CodeGeneratorSpecific
{
    bool generateArgumentsChecks();
    bool generateBody();

  public:
    CodeGenerator(MIRGenerator *gen, LIRGraph &graph);

  public:
    bool generate();

    bool visitLabel(LLabel *lir);
    bool visitNop(LNop *lir);
    bool visitCaptureAllocations(LCaptureAllocations *lir);
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
    bool visitAddV(LAddV *lir);
    bool visitConcat(LConcat *lir);
    bool visitFunctionEnvironment(LFunctionEnvironment *lir);
    bool visitCallGetProperty(LCallGetProperty *lir);
    bool visitCallGetName(LCallGetName *lir);
    bool visitCallGetNameTypeOf(LCallGetNameTypeOf *lir);
    bool visitGetPropertyCacheV(LGetPropertyCacheV *load) { return visitGetPropertyCache(load); }
    bool visitGetPropertyCacheT(LGetPropertyCacheT *load) { return visitGetPropertyCache(load); }

    bool visitCheckOverRecursed(LCheckOverRecursed *lir);
    bool visitCheckOverRecursedFailure(CheckOverRecursedFailure *ool);

    bool visitUnboxDouble(LUnboxDouble *lir);
    bool visitOutOfLineUnboxDouble(OutOfLineUnboxDouble *ool);
    bool visitOutOfLineGetPropertyCache(OutOfLineGetPropertyCache *ool);

  private:
    bool visitGetPropertyCache(LInstruction *load);
};

} 
} 

#endif 

