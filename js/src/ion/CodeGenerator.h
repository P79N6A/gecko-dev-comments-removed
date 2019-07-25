








































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

    bool visitCaptureAllocations(LCaptureAllocations *lir);
    bool visitGoto(LGoto *lir);
    bool visitParameter(LParameter *lir);
    bool visitStart(LStart *lir);
    bool visitReturn(LReturn *ret);
    bool visitOsrEntry(LOsrEntry *lir);
    bool visitStackArg(LStackArg *lir);
    bool visitValueToInt32(LValueToInt32 *lir);
    bool visitValueToDouble(LValueToDouble *lir);
    bool visitInt32ToDouble(LInt32ToDouble *lir);
    bool visitTestVAndBranch(LTestVAndBranch *lir);
    bool visitTruncateDToInt32(LTruncateDToInt32 *lir);
    bool visitInteger(LInteger *lir);
    bool visitPointer(LPointer *lir);
    bool visitSlots(LSlots *lir);
    bool visitStoreSlotV(LStoreSlotV *store);
    bool visitElements(LElements *lir);
    bool visitTypeBarrier(LTypeBarrier *lir);
    bool visitDoubleToInt32(LDoubleToInt32 *lir);
    bool visitNewArray(LNewArray *builder);
    bool visitArrayLength(LArrayLength *lir);
    bool visitStringLength(LStringLength *lir);
    bool visitInitializedLength(LInitializedLength *lir);
    bool visitLoadPropertyGeneric(LLoadPropertyGeneric *ins);
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

