








































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

class OutOfLineNewArray;
class OutOfLineNewObject;
class OutOfLineCreateThis;
class CheckOverRecursedFailure;
class OutOfLineUnboxDouble;
class OutOfLineCache;
class OutOfLineStoreElementHole;
class OutOfLineTypeOfV;
class OutOfLineLoadTypedArray;

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
    bool visitOsiPoint(LOsiPoint *lir);
    bool visitGoto(LGoto *lir);
    bool visitParameter(LParameter *lir);
    bool visitCallee(LCallee *lir);
    bool visitStart(LStart *lir);
    bool visitReturn(LReturn *ret);
    bool visitDefVar(LDefVar *lir);
    bool visitOsrEntry(LOsrEntry *lir);
    bool visitOsrScopeChain(LOsrScopeChain *lir);
    bool visitStackArg(LStackArg *lir);
    bool visitValueToInt32(LValueToInt32 *lir);
    bool visitValueToDouble(LValueToDouble *lir);
    bool visitInt32ToDouble(LInt32ToDouble *lir);
    bool visitTestVAndBranch(LTestVAndBranch *lir);
    bool visitIntToString(LIntToString *lir);
    bool visitInteger(LInteger *lir);
    bool visitRegExp(LRegExp *lir);
    bool visitLambda(LLambda *lir);
    bool visitLambdaForSingleton(LLambdaForSingleton *lir);
    bool visitPointer(LPointer *lir);
    bool visitSlots(LSlots *lir);
    bool visitStoreSlotV(LStoreSlotV *store);
    bool visitElements(LElements *lir);
    bool visitTypeBarrier(LTypeBarrier *lir);
    bool visitMonitorTypes(LMonitorTypes *lir);
    bool visitCallNative(LCallNative *call);
    bool emitCallInvokeFunction(LCallGeneric *call, uint32 unusedStack);
    bool visitCallGeneric(LCallGeneric *call);
    bool visitCallConstructor(LCallConstructor *call);
    bool visitDoubleToInt32(LDoubleToInt32 *lir);
    bool visitNewArrayCallVM(LNewArray *lir);
    bool visitNewArray(LNewArray *lir);
    bool visitOutOfLineNewArray(OutOfLineNewArray *ool);
    bool visitNewObjectVMCall(LNewObject *lir);
    bool visitNewObject(LNewObject *lir);
    bool visitOutOfLineNewObject(OutOfLineNewObject *ool);
    bool visitNewCallObject(LNewCallObject *lir);
    bool visitInitProp(LInitProp *lir);
    bool visitCreateThisVMCall(LCreateThis *lir);
    bool visitCreateThis(LCreateThis *lir);
    bool visitOutOfLineCreateThis(OutOfLineCreateThis *ool);
    bool visitArrayLength(LArrayLength *lir);
    bool visitTypedArrayLength(LTypedArrayLength *lir);
    bool visitTypedArrayElements(LTypedArrayElements *lir);
    bool visitStringLength(LStringLength *lir);
    bool visitInitializedLength(LInitializedLength *lir);
    bool visitSetInitializedLength(LSetInitializedLength *lir);
    bool visitNotV(LNotV *ins);
    bool visitBoundsCheck(LBoundsCheck *lir);
    bool visitBoundsCheckRange(LBoundsCheckRange *lir);
    bool visitBoundsCheckLower(LBoundsCheckLower *lir);
    bool visitLoadFixedSlotV(LLoadFixedSlotV *ins);
    bool visitLoadFixedSlotT(LLoadFixedSlotT *ins);
    bool visitStoreFixedSlotV(LStoreFixedSlotV *ins);
    bool visitStoreFixedSlotT(LStoreFixedSlotT *ins);
    bool visitAbsI(LAbsI *lir);
    bool visitMathFunctionD(LMathFunctionD *ins);
    bool visitBinaryV(LBinaryV *lir);
    bool visitCompareS(LCompareS *lir);
    bool visitCompareV(LCompareV *lir);
    bool visitIsNullOrUndefined(LIsNullOrUndefined *lir);
    bool visitIsNullOrUndefinedAndBranch(LIsNullOrUndefinedAndBranch *lir);
    bool visitConcat(LConcat *lir);
    bool visitCharCodeAt(LCharCodeAt *lir);
    bool visitFromCharCode(LFromCharCode *lir);
    bool visitFunctionEnvironment(LFunctionEnvironment *lir);
    bool visitCallGetProperty(LCallGetProperty *lir);
    bool visitCallGetElement(LCallGetElement *lir);
    bool visitCallSetElement(LCallSetElement *lir);
    bool visitThrow(LThrow *lir);
    bool visitTypeOfV(LTypeOfV *lir);
    bool visitOutOfLineTypeOfV(OutOfLineTypeOfV *ool);
    bool visitToIdV(LToIdV *lir);
    bool visitLoadElementV(LLoadElementV *load);
    bool visitLoadElementHole(LLoadElementHole *lir);
    bool visitStoreElementT(LStoreElementT *lir);
    bool visitStoreElementV(LStoreElementV *lir);
    bool visitStoreElementHoleT(LStoreElementHoleT *lir);
    bool visitStoreElementHoleV(LStoreElementHoleV *lir);
    bool emitArrayPopShift(LInstruction *lir, const MArrayPopShift *mir, Register obj,
                           Register elementsTemp, Register lengthTemp, TypedOrValueRegister out);
    bool visitArrayPopShiftV(LArrayPopShiftV *lir);
    bool visitArrayPopShiftT(LArrayPopShiftT *lir);
    bool emitArrayPush(LInstruction *lir, const MArrayPush *mir, Register obj,
                       ConstantOrRegister value, Register elementsTemp, Register length);
    bool visitArrayPushV(LArrayPushV *lir);
    bool visitArrayPushT(LArrayPushT *lir);
    bool visitLoadTypedArrayElement(LLoadTypedArrayElement *lir);
    bool visitLoadTypedArrayElementHole(LLoadTypedArrayElementHole *lir);
    bool visitStoreTypedArrayElement(LStoreTypedArrayElement *lir);
    bool visitClampIToUint8(LClampIToUint8 *lir);
    bool visitClampDToUint8(LClampDToUint8 *lir);
    bool visitClampVToUint8(LClampVToUint8 *lir);
    bool visitOutOfLineLoadTypedArray(OutOfLineLoadTypedArray *ool);
    bool visitCallIteratorStart(LCallIteratorStart *lir);
    bool visitIteratorStart(LIteratorStart *lir);
    bool visitIteratorNext(LIteratorNext *lir);
    bool visitIteratorMore(LIteratorMore *lir);
    bool visitIteratorEnd(LIteratorEnd *lir);
    bool visitArgumentsLength(LArgumentsLength *lir);
    bool visitGetArgument(LGetArgument *lir);
    bool visitCallSetProperty(LCallSetProperty *ins);
    bool visitCallDeleteProperty(LCallDeleteProperty *lir);
    bool visitBitNotV(LBitNotV *lir);
    bool visitBitOpV(LBitOpV *lir);
    bool emitInstanceOf(LInstruction *ins, Register rhs);
    bool visitInstanceOfO(LInstanceOfO *ins);
    bool visitInstanceOfV(LInstanceOfV *ins);
    bool visitGuardObject(LGuardObject *lir);

    bool visitCheckOverRecursed(LCheckOverRecursed *lir);
    bool visitCheckOverRecursedFailure(CheckOverRecursedFailure *ool);

    bool visitUnboxDouble(LUnboxDouble *lir);
    bool visitOutOfLineUnboxDouble(OutOfLineUnboxDouble *ool);
    bool visitOutOfLineStoreElementHole(OutOfLineStoreElementHole *ool);

    bool visitOutOfLineCacheGetProperty(OutOfLineCache *ool);
    bool visitOutOfLineGetElementCache(OutOfLineCache *ool);
    bool visitOutOfLineSetPropertyCache(OutOfLineCache *ool);
    bool visitOutOfLineBindNameCache(OutOfLineCache *ool);
    bool visitOutOfLineGetNameCache(OutOfLineCache *ool);

    bool visitGetPropertyCacheV(LGetPropertyCacheV *ins) {
        return visitCache(ins);
    }
    bool visitGetPropertyCacheT(LGetPropertyCacheT *ins) {
        return visitCache(ins);
    }
    bool visitGetElementCacheV(LGetElementCacheV *ins) {
        return visitCache(ins);
    }
    bool visitBindNameCache(LBindNameCache *ins) {
        return visitCache(ins);
    }
    bool visitSetPropertyCacheV(LSetPropertyCacheV *ins) {
        return visitCache(ins);
    }
    bool visitSetPropertyCacheT(LSetPropertyCacheT *ins) {
        return visitCache(ins);
    }
    bool visitGetNameCache(LGetNameCache *ins) {
        return visitCache(ins);
    }

  private:
    bool visitCache(LInstruction *load);
    bool visitCallSetProperty(LInstruction *ins);

    ConstantOrRegister getSetPropertyValue(LInstruction *ins);
    bool generateBranchV(const ValueOperand &value, Label *ifTrue, Label *ifFalse, FloatRegister fr);
};

} 
} 

#endif 

