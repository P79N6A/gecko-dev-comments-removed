





#include "jit/CodeGenerator.h"

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/MathAlgorithms.h"

#include "jslibmath.h"
#include "jsmath.h"
#include "jsnum.h"
#include "jsprf.h"

#include "asmjs/AsmJSModule.h"
#include "builtin/Eval.h"
#include "builtin/TypedObject.h"
#ifdef JSGC_GENERATIONAL
# include "gc/Nursery.h"
#endif
#include "jit/BaselineCompiler.h"
#include "jit/IonCaches.h"
#include "jit/IonLinker.h"
#include "jit/IonOptimizationLevels.h"
#include "jit/JitcodeMap.h"
#include "jit/JitSpewer.h"
#include "jit/Lowering.h"
#include "jit/MIRGenerator.h"
#include "jit/MoveEmitter.h"
#include "jit/ParallelFunctions.h"
#include "jit/ParallelSafetyAnalysis.h"
#include "jit/RangeAnalysis.h"
#include "vm/ForkJoin.h"
#include "vm/TraceLogging.h"

#include "jsboolinlines.h"

#include "jit/ExecutionMode-inl.h"
#include "jit/shared/CodeGenerator-shared-inl.h"
#include "vm/Interpreter-inl.h"

using namespace js;
using namespace js::jit;

using mozilla::DebugOnly;
using mozilla::FloatingPoint;
using mozilla::Maybe;
using mozilla::NegativeInfinity;
using mozilla::PositiveInfinity;
using JS::GenericNaN;

namespace js {
namespace jit {



class OutOfLineUpdateCache :
  public OutOfLineCodeBase<CodeGenerator>,
  public IonCacheVisitor
{
  private:
    LInstruction *lir_;
    size_t cacheIndex_;
    AddCacheState state_;

  public:
    OutOfLineUpdateCache(LInstruction *lir, size_t cacheIndex)
      : lir_(lir),
        cacheIndex_(cacheIndex)
    { }

    void bind(MacroAssembler *masm) {
        
        
    }

    size_t getCacheIndex() const {
        return cacheIndex_;
    }
    LInstruction *lir() const {
        return lir_;
    }
    AddCacheState &state() {
        return state_;
    }

    bool accept(CodeGenerator *codegen) {
        return codegen->visitOutOfLineCache(this);
    }

    
#define VISIT_CACHE_FUNCTION(op)                                        \
    bool visit##op##IC(CodeGenerator *codegen) {                        \
        CodeGenerator::DataPtr<op##IC> ic(codegen, getCacheIndex());    \
        return codegen->visit##op##IC(this, ic);                        \
    }

    IONCACHE_KIND_LIST(VISIT_CACHE_FUNCTION)
#undef VISIT_CACHE_FUNCTION
};





bool
CodeGeneratorShared::addCache(LInstruction *lir, size_t cacheIndex)
{
    if (cacheIndex == SIZE_MAX)
        return false;

    DataPtr<IonCache> cache(this, cacheIndex);
    MInstruction *mir = lir->mirRaw()->toInstruction();
    if (mir->resumePoint())
        cache->setScriptedLocation(mir->block()->info().script(),
                                   mir->resumePoint()->pc());
    else
        cache->setIdempotent();

    OutOfLineUpdateCache *ool = new(alloc()) OutOfLineUpdateCache(lir, cacheIndex);
    if (!addOutOfLineCode(ool, mir))
        return false;

    
    cache->initializeAddCacheState(lir, &ool->state());

    cache->emitInitialJump(masm, ool->state());
    masm.bind(ool->rejoin());

    return true;
}

bool
CodeGenerator::visitOutOfLineCache(OutOfLineUpdateCache *ool)
{
    DataPtr<IonCache> cache(this, ool->getCacheIndex());

    
    cache->setFallbackLabel(masm.labelForPatch());
    cache->bindInitialJump(masm, ool->state());

    
    return cache->accept(this, ool);
}

StringObject *
MNewStringObject::templateObj() const {
    return &templateObj_->as<StringObject>();
}

CodeGenerator::CodeGenerator(MIRGenerator *gen, LIRGraph *graph, MacroAssembler *masm)
  : CodeGeneratorSpecific(gen, graph, masm)
  , ionScriptLabels_(gen->alloc())
  , scriptCounts_(nullptr)
{
}

CodeGenerator::~CodeGenerator()
{
    JS_ASSERT_IF(!gen->compilingAsmJS(), masm.numAsmJSAbsoluteLinks() == 0);
    js_delete(scriptCounts_);
}

typedef bool (*StringToNumberFn)(ThreadSafeContext *, JSString *, double *);
typedef bool (*StringToNumberParFn)(ForkJoinContext *, JSString *, double *);
static const VMFunctionsModal StringToNumberInfo = VMFunctionsModal(
    FunctionInfo<StringToNumberFn>(StringToNumber),
    FunctionInfo<StringToNumberParFn>(StringToNumberPar));

bool
CodeGenerator::visitValueToInt32(LValueToInt32 *lir)
{
    ValueOperand operand = ToValue(lir, LValueToInt32::Input);
    Register output = ToRegister(lir->output());
    FloatRegister temp = ToFloatRegister(lir->tempFloat());

    MDefinition *input;
    if (lir->mode() == LValueToInt32::NORMAL)
        input = lir->mirNormal()->input();
    else
        input = lir->mirTruncate()->input();

    Label fails;
    if (lir->mode() == LValueToInt32::TRUNCATE) {
        OutOfLineCode *oolDouble = oolTruncateDouble(temp, output, lir->mir());
        if (!oolDouble)
            return false;

        
        
        Label *stringEntry, *stringRejoin;
        Register stringReg;
        if (input->mightBeType(MIRType_String)) {
            stringReg = ToRegister(lir->temp());
            OutOfLineCode *oolString = oolCallVM(StringToNumberInfo, lir, (ArgList(), stringReg),
                                                 StoreFloatRegisterTo(temp));
            if (!oolString)
                return false;
            stringEntry = oolString->entry();
            stringRejoin = oolString->rejoin();
        } else {
            stringReg = InvalidReg;
            stringEntry = nullptr;
            stringRejoin = nullptr;
        }

        masm.truncateValueToInt32(operand, input, stringEntry, stringRejoin, oolDouble->entry(),
                                  stringReg, temp, output, &fails);
        masm.bind(oolDouble->rejoin());
    } else {
        masm.convertValueToInt32(operand, input, temp, output, &fails,
                                 lir->mirNormal()->canBeNegativeZero(),
                                 lir->mirNormal()->conversion());
    }

    return bailoutFrom(&fails, lir->snapshot());
}

bool
CodeGenerator::visitValueToDouble(LValueToDouble *lir)
{
    MToDouble *mir = lir->mir();
    ValueOperand operand = ToValue(lir, LValueToDouble::Input);
    FloatRegister output = ToFloatRegister(lir->output());

    Register tag = masm.splitTagForTest(operand);

    Label isDouble, isInt32, isBool, isNull, isUndefined, done;
    bool hasBoolean = false, hasNull = false, hasUndefined = false;

    masm.branchTestDouble(Assembler::Equal, tag, &isDouble);
    masm.branchTestInt32(Assembler::Equal, tag, &isInt32);

    if (mir->conversion() != MToFPInstruction::NumbersOnly) {
        masm.branchTestBoolean(Assembler::Equal, tag, &isBool);
        masm.branchTestUndefined(Assembler::Equal, tag, &isUndefined);
        hasBoolean = true;
        hasUndefined = true;
        if (mir->conversion() != MToFPInstruction::NonNullNonStringPrimitives) {
            masm.branchTestNull(Assembler::Equal, tag, &isNull);
            hasNull = true;
        }
    }

    if (!bailout(lir->snapshot()))
        return false;

    if (hasNull) {
        masm.bind(&isNull);
        masm.loadConstantDouble(0.0, output);
        masm.jump(&done);
    }

    if (hasUndefined) {
        masm.bind(&isUndefined);
        masm.loadConstantDouble(GenericNaN(), output);
        masm.jump(&done);
    }

    if (hasBoolean) {
        masm.bind(&isBool);
        masm.boolValueToDouble(operand, output);
        masm.jump(&done);
    }

    masm.bind(&isInt32);
    masm.int32ValueToDouble(operand, output);
    masm.jump(&done);

    masm.bind(&isDouble);
    masm.unboxDouble(operand, output);
    masm.bind(&done);

    return true;
}

bool
CodeGenerator::visitValueToFloat32(LValueToFloat32 *lir)
{
    MToFloat32 *mir = lir->mir();
    ValueOperand operand = ToValue(lir, LValueToFloat32::Input);
    FloatRegister output = ToFloatRegister(lir->output());

    Register tag = masm.splitTagForTest(operand);

    Label isDouble, isInt32, isBool, isNull, isUndefined, done;
    bool hasBoolean = false, hasNull = false, hasUndefined = false;

    masm.branchTestDouble(Assembler::Equal, tag, &isDouble);
    masm.branchTestInt32(Assembler::Equal, tag, &isInt32);

    if (mir->conversion() != MToFPInstruction::NumbersOnly) {
        masm.branchTestBoolean(Assembler::Equal, tag, &isBool);
        masm.branchTestUndefined(Assembler::Equal, tag, &isUndefined);
        hasBoolean = true;
        hasUndefined = true;
        if (mir->conversion() != MToFPInstruction::NonNullNonStringPrimitives) {
            masm.branchTestNull(Assembler::Equal, tag, &isNull);
            hasNull = true;
        }
    }

    if (!bailout(lir->snapshot()))
        return false;

    if (hasNull) {
        masm.bind(&isNull);
        masm.loadConstantFloat32(0.0f, output);
        masm.jump(&done);
    }

    if (hasUndefined) {
        masm.bind(&isUndefined);
        masm.loadConstantFloat32(float(GenericNaN()), output);
        masm.jump(&done);
    }

    if (hasBoolean) {
        masm.bind(&isBool);
        masm.boolValueToFloat32(operand, output);
        masm.jump(&done);
    }

    masm.bind(&isInt32);
    masm.int32ValueToFloat32(operand, output);
    masm.jump(&done);

    masm.bind(&isDouble);
    
    
#if defined(JS_CODEGEN_ARM) || defined(JS_CODEGEN_MIPS)
    masm.unboxDouble(operand, ScratchDoubleReg);
    masm.convertDoubleToFloat32(ScratchDoubleReg, output);
#else
    masm.unboxDouble(operand, output);
    masm.convertDoubleToFloat32(output, output);
#endif
    masm.bind(&done);

    return true;
}

bool
CodeGenerator::visitInt32ToDouble(LInt32ToDouble *lir)
{
    masm.convertInt32ToDouble(ToRegister(lir->input()), ToFloatRegister(lir->output()));
    return true;
}

bool
CodeGenerator::visitFloat32ToDouble(LFloat32ToDouble *lir)
{
    masm.convertFloat32ToDouble(ToFloatRegister(lir->input()), ToFloatRegister(lir->output()));
    return true;
}

bool
CodeGenerator::visitDoubleToFloat32(LDoubleToFloat32 *lir)
{
    masm.convertDoubleToFloat32(ToFloatRegister(lir->input()), ToFloatRegister(lir->output()));
    return true;
}

bool
CodeGenerator::visitInt32ToFloat32(LInt32ToFloat32 *lir)
{
    masm.convertInt32ToFloat32(ToRegister(lir->input()), ToFloatRegister(lir->output()));
    return true;
}

bool
CodeGenerator::visitDoubleToInt32(LDoubleToInt32 *lir)
{
    Label fail;
    FloatRegister input = ToFloatRegister(lir->input());
    Register output = ToRegister(lir->output());
    masm.convertDoubleToInt32(input, output, &fail, lir->mir()->canBeNegativeZero());
    if (!bailoutFrom(&fail, lir->snapshot()))
        return false;
    return true;
}

bool
CodeGenerator::visitFloat32ToInt32(LFloat32ToInt32 *lir)
{
    Label fail;
    FloatRegister input = ToFloatRegister(lir->input());
    Register output = ToRegister(lir->output());
    masm.convertFloat32ToInt32(input, output, &fail, lir->mir()->canBeNegativeZero());
    if (!bailoutFrom(&fail, lir->snapshot()))
        return false;
    return true;
}

void
CodeGenerator::emitOOLTestObject(Register objreg,
                                 Label *ifEmulatesUndefined,
                                 Label *ifDoesntEmulateUndefined,
                                 Register scratch)
{
    saveVolatile(scratch);
    masm.setupUnalignedABICall(1, scratch);
    masm.passABIArg(objreg);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, js::EmulatesUndefined));
    masm.storeCallResult(scratch);
    restoreVolatile(scratch);

    masm.branchIfTrueBool(scratch, ifEmulatesUndefined);
    masm.jump(ifDoesntEmulateUndefined);
}









class OutOfLineTestObject : public OutOfLineCodeBase<CodeGenerator>
{
    Register objreg_;
    Register scratch_;

    Label *ifEmulatesUndefined_;
    Label *ifDoesntEmulateUndefined_;

#ifdef DEBUG
    bool initialized() { return ifEmulatesUndefined_ != nullptr; }
#endif

  public:
    OutOfLineTestObject()
#ifdef DEBUG
      : ifEmulatesUndefined_(nullptr), ifDoesntEmulateUndefined_(nullptr)
#endif
    { }

    bool accept(CodeGenerator *codegen) MOZ_FINAL MOZ_OVERRIDE {
        MOZ_ASSERT(initialized());
        codegen->emitOOLTestObject(objreg_, ifEmulatesUndefined_, ifDoesntEmulateUndefined_,
                                   scratch_);
        return true;
    }

    
    
    
    void setInputAndTargets(Register objreg, Label *ifEmulatesUndefined, Label *ifDoesntEmulateUndefined,
                            Register scratch)
    {
        MOZ_ASSERT(!initialized());
        MOZ_ASSERT(ifEmulatesUndefined);
        objreg_ = objreg;
        scratch_ = scratch;
        ifEmulatesUndefined_ = ifEmulatesUndefined;
        ifDoesntEmulateUndefined_ = ifDoesntEmulateUndefined;
    }
};





class OutOfLineTestObjectWithLabels : public OutOfLineTestObject
{
    Label label1_;
    Label label2_;

  public:
    OutOfLineTestObjectWithLabels() { }

    Label *label1() { return &label1_; }
    Label *label2() { return &label2_; }
};

void
CodeGenerator::testObjectEmulatesUndefinedKernel(Register objreg,
                                                 Label *ifEmulatesUndefined,
                                                 Label *ifDoesntEmulateUndefined,
                                                 Register scratch, OutOfLineTestObject *ool)
{
    ool->setInputAndTargets(objreg, ifEmulatesUndefined, ifDoesntEmulateUndefined, scratch);

    
    
    
    masm.branchTestObjectTruthy(false, objreg, scratch, ool->entry(), ifEmulatesUndefined);
}

void
CodeGenerator::branchTestObjectEmulatesUndefined(Register objreg,
                                                 Label *ifEmulatesUndefined,
                                                 Label *ifDoesntEmulateUndefined,
                                                 Register scratch, OutOfLineTestObject *ool)
{
    MOZ_ASSERT(!ifDoesntEmulateUndefined->bound(),
               "ifDoesntEmulateUndefined will be bound to the fallthrough path");

    testObjectEmulatesUndefinedKernel(objreg, ifEmulatesUndefined, ifDoesntEmulateUndefined,
                                      scratch, ool);
    masm.bind(ifDoesntEmulateUndefined);
}

void
CodeGenerator::testObjectEmulatesUndefined(Register objreg,
                                           Label *ifEmulatesUndefined,
                                           Label *ifDoesntEmulateUndefined,
                                           Register scratch, OutOfLineTestObject *ool)
{
    testObjectEmulatesUndefinedKernel(objreg, ifEmulatesUndefined, ifDoesntEmulateUndefined,
                                      scratch, ool);
    masm.jump(ifDoesntEmulateUndefined);
}

void
CodeGenerator::testValueTruthyKernel(const ValueOperand &value,
                                     const LDefinition *scratch1, const LDefinition *scratch2,
                                     FloatRegister fr,
                                     Label *ifTruthy, Label *ifFalsy,
                                     OutOfLineTestObject *ool,
                                     MDefinition *valueMIR)
{
    
    
    
    
    
    bool mightBeUndefined = valueMIR->mightBeType(MIRType_Undefined);
    bool mightBeNull = valueMIR->mightBeType(MIRType_Null);
    bool mightBeBoolean = valueMIR->mightBeType(MIRType_Boolean);
    bool mightBeInt32 = valueMIR->mightBeType(MIRType_Int32);
    bool mightBeObject = valueMIR->mightBeType(MIRType_Object);
    bool mightBeString = valueMIR->mightBeType(MIRType_String);
    bool mightBeSymbol = valueMIR->mightBeType(MIRType_Symbol);
    bool mightBeDouble = valueMIR->mightBeType(MIRType_Double);
    int tagCount = int(mightBeUndefined) + int(mightBeNull) +
        int(mightBeBoolean) + int(mightBeInt32) + int(mightBeObject) +
        int(mightBeString) + int(mightBeSymbol) + int(mightBeDouble);

    MOZ_ASSERT_IF(!valueMIR->emptyResultTypeSet(), tagCount > 0);

    
    
    if (int(mightBeNull) + int(mightBeUndefined) == tagCount) {
        masm.jump(ifFalsy);
        return;
    }

    Register tag = masm.splitTagForTest(value);

    if (mightBeUndefined) {
        MOZ_ASSERT(tagCount > 1);
        masm.branchTestUndefined(Assembler::Equal, tag, ifFalsy);
        --tagCount;
    }

    if (mightBeNull) {
        MOZ_ASSERT(tagCount > 1);
        masm.branchTestNull(Assembler::Equal, tag, ifFalsy);
        --tagCount;
    }

    if (mightBeBoolean) {
        MOZ_ASSERT(tagCount != 0);
        Label notBoolean;
        if (tagCount != 1)
            masm.branchTestBoolean(Assembler::NotEqual, tag, &notBoolean);
        masm.branchTestBooleanTruthy(false, value, ifFalsy);
        if (tagCount != 1)
            masm.jump(ifTruthy);
        
        masm.bind(&notBoolean);
        --tagCount;
    }

    if (mightBeInt32) {
        MOZ_ASSERT(tagCount != 0);
        Label notInt32;
        if (tagCount != 1)
            masm.branchTestInt32(Assembler::NotEqual, tag, &notInt32);
        masm.branchTestInt32Truthy(false, value, ifFalsy);
        if (tagCount != 1)
            masm.jump(ifTruthy);
        
        masm.bind(&notInt32);
        --tagCount;
    }

    if (mightBeObject) {
        MOZ_ASSERT(tagCount != 0);
        if (ool) {
            Label notObject;

            if (tagCount != 1)
                masm.branchTestObject(Assembler::NotEqual, tag, &notObject);

            Register objreg = masm.extractObject(value, ToRegister(scratch1));
            testObjectEmulatesUndefined(objreg, ifFalsy, ifTruthy, ToRegister(scratch2), ool);

            masm.bind(&notObject);
        } else {
            if (tagCount != 1)
                masm.branchTestObject(Assembler::Equal, tag, ifTruthy);
            
        }
        --tagCount;
    } else {
        MOZ_ASSERT(!ool,
                   "We better not have an unused OOL path, since the code generator will try to "
                   "generate code for it but we never set up its labels, which will cause null "
                   "derefs of those labels.");
    }

    if (mightBeString) {
        
        MOZ_ASSERT(tagCount != 0);
        Label notString;
        if (tagCount != 1)
            masm.branchTestString(Assembler::NotEqual, tag, &notString);
        masm.branchTestStringTruthy(false, value, ifFalsy);
        if (tagCount != 1)
            masm.jump(ifTruthy);
        
        masm.bind(&notString);
        --tagCount;
    }

    if (mightBeSymbol) {
        
        MOZ_ASSERT(tagCount != 0);
        if (tagCount != 1)
            masm.branchTestSymbol(Assembler::Equal, tag, ifTruthy);
        
        --tagCount;
    }

    if (mightBeDouble) {
        MOZ_ASSERT(tagCount == 1);
        
        masm.unboxDouble(value, fr);
        masm.branchTestDoubleTruthy(false, fr, ifFalsy);
        --tagCount;
    }

    MOZ_ASSERT(tagCount == 0);

    
}

void
CodeGenerator::testValueTruthy(const ValueOperand &value,
                               const LDefinition *scratch1, const LDefinition *scratch2,
                               FloatRegister fr,
                               Label *ifTruthy, Label *ifFalsy,
                               OutOfLineTestObject *ool,
                               MDefinition *valueMIR)
{
    testValueTruthyKernel(value, scratch1, scratch2, fr, ifTruthy, ifFalsy, ool, valueMIR);
    masm.jump(ifTruthy);
}

Label *
CodeGenerator::getJumpLabelForBranch(MBasicBlock *block)
{
    
    block = skipTrivialBlocks(block);

    if (!labelForBackedgeWithImplicitCheck(block))
        return block->lir()->label();

    
    
    
    
    
    Label *res = GetIonContext()->temp->lifoAlloc()->new_<Label>();
    Label after;
    masm.jump(&after);
    masm.bind(res);
    jumpToBlock(block);
    masm.bind(&after);
    return res;
}

bool
CodeGenerator::visitTestOAndBranch(LTestOAndBranch *lir)
{
    MOZ_ASSERT(lir->mir()->operandMightEmulateUndefined(),
               "Objects which can't emulate undefined should have been constant-folded");

    OutOfLineTestObject *ool = new(alloc()) OutOfLineTestObject();
    if (!addOutOfLineCode(ool, lir->mir()))
        return false;

    Label *truthy = getJumpLabelForBranch(lir->ifTruthy());
    Label *falsy = getJumpLabelForBranch(lir->ifFalsy());

    testObjectEmulatesUndefined(ToRegister(lir->input()), falsy, truthy,
                                ToRegister(lir->temp()), ool);
    return true;

}

bool
CodeGenerator::visitTestVAndBranch(LTestVAndBranch *lir)
{
    OutOfLineTestObject *ool = nullptr;
    MDefinition *input = lir->mir()->input();
    
    
    
    
    if (lir->mir()->operandMightEmulateUndefined() && input->mightBeType(MIRType_Object)) {
        ool = new(alloc()) OutOfLineTestObject();
        if (!addOutOfLineCode(ool, lir->mir()))
            return false;
    }

    Label *truthy = getJumpLabelForBranch(lir->ifTruthy());
    Label *falsy = getJumpLabelForBranch(lir->ifFalsy());

    testValueTruthy(ToValue(lir, LTestVAndBranch::Input),
                    lir->temp1(), lir->temp2(),
                    ToFloatRegister(lir->tempFloat()),
                    truthy, falsy, ool, input);
    return true;
}

bool
CodeGenerator::visitFunctionDispatch(LFunctionDispatch *lir)
{
    MFunctionDispatch *mir = lir->mir();
    Register input = ToRegister(lir->input());
    Label *lastLabel;
    size_t casesWithFallback;

    
    if (!mir->hasFallback()) {
        JS_ASSERT(mir->numCases() > 0);
        casesWithFallback = mir->numCases();
        lastLabel = skipTrivialBlocks(mir->getCaseBlock(mir->numCases() - 1))->lir()->label();
    } else {
        casesWithFallback = mir->numCases() + 1;
        lastLabel = skipTrivialBlocks(mir->getFallback())->lir()->label();
    }

    
    for (size_t i = 0; i < casesWithFallback - 1; i++) {
        JS_ASSERT(i < mir->numCases());
        JSFunction *func = mir->getCase(i);
        LBlock *target = skipTrivialBlocks(mir->getCaseBlock(i))->lir();
        masm.branchPtr(Assembler::Equal, input, ImmGCPtr(func), target->label());
    }

    
    masm.jump(lastLabel);

    return true;
}

bool
CodeGenerator::visitTypeObjectDispatch(LTypeObjectDispatch *lir)
{
    MTypeObjectDispatch *mir = lir->mir();
    Register input = ToRegister(lir->input());
    Register temp = ToRegister(lir->temp());

    

    masm.loadPtr(Address(input, JSObject::offsetOfType()), temp);

    

    MacroAssembler::BranchGCPtr lastBranch;
    LBlock *lastBlock = nullptr;
    InlinePropertyTable *propTable = mir->propTable();
    for (size_t i = 0; i < mir->numCases(); i++) {
        JSFunction *func = mir->getCase(i);
        LBlock *target = skipTrivialBlocks(mir->getCaseBlock(i))->lir();

        DebugOnly<bool> found = false;
        for (size_t j = 0; j < propTable->numEntries(); j++) {
            if (propTable->getFunction(j) != func)
                continue;

            if (lastBranch.isInitialized())
                lastBranch.emit(masm);

            types::TypeObject *typeObj = propTable->getTypeObject(j);
            lastBranch = MacroAssembler::BranchGCPtr(Assembler::Equal, temp, ImmGCPtr(typeObj),
                                                     target->label());
            lastBlock = target;
            found = true;
        }
        JS_ASSERT(found);
    }

    

    LBlock *fallback = skipTrivialBlocks(mir->getFallback())->lir();
    if (!lastBranch.isInitialized()) {
        if (!isNextBlock(fallback))
            masm.jump(fallback->label());
        return true;
    }

    lastBranch.invertCondition();
    lastBranch.relink(fallback->label());
    lastBranch.emit(masm);

    if (!isNextBlock(lastBlock))
        masm.jump(lastBlock->label());

    return true;
}

bool
CodeGenerator::visitBooleanToString(LBooleanToString *lir)
{
    Register input = ToRegister(lir->input());
    Register output = ToRegister(lir->output());
    const JSAtomState &names = GetIonContext()->runtime->names();
    Label true_, done;

    masm.branchTest32(Assembler::NonZero, input, input, &true_);
    masm.movePtr(ImmGCPtr(names.false_), output);
    masm.jump(&done);

    masm.bind(&true_);
    masm.movePtr(ImmGCPtr(names.true_), output);

    masm.bind(&done);

    return true;
}

void
CodeGenerator::emitIntToString(Register input, Register output, Label *ool)
{
    masm.branch32(Assembler::AboveOrEqual, input, Imm32(StaticStrings::INT_STATIC_LIMIT), ool);

    
    masm.movePtr(ImmPtr(&GetIonContext()->runtime->staticStrings().intStaticTable), output);
    masm.loadPtr(BaseIndex(output, input, ScalePointer), output);
}

typedef JSFlatString *(*IntToStringFn)(ThreadSafeContext *, int);
typedef JSFlatString *(*IntToStringParFn)(ForkJoinContext *, int);
static const VMFunctionsModal IntToStringInfo = VMFunctionsModal(
    FunctionInfo<IntToStringFn>(Int32ToString<CanGC>),
    FunctionInfo<IntToStringParFn>(IntToStringPar));

bool
CodeGenerator::visitIntToString(LIntToString *lir)
{
    Register input = ToRegister(lir->input());
    Register output = ToRegister(lir->output());

    OutOfLineCode *ool = oolCallVM(IntToStringInfo, lir, (ArgList(), input),
                                   StoreRegisterTo(output));
    if (!ool)
        return false;

    emitIntToString(input, output, ool->entry());

    masm.bind(ool->rejoin());
    return true;
}

typedef JSString *(*DoubleToStringFn)(ThreadSafeContext *, double);
typedef JSString *(*DoubleToStringParFn)(ForkJoinContext *, double);
static const VMFunctionsModal DoubleToStringInfo = VMFunctionsModal(
    FunctionInfo<DoubleToStringFn>(NumberToString<CanGC>),
    FunctionInfo<DoubleToStringParFn>(DoubleToStringPar));

bool
CodeGenerator::visitDoubleToString(LDoubleToString *lir)
{
    FloatRegister input = ToFloatRegister(lir->input());
    Register temp = ToRegister(lir->tempInt());
    Register output = ToRegister(lir->output());

    OutOfLineCode *ool = oolCallVM(DoubleToStringInfo, lir, (ArgList(), input),
                                   StoreRegisterTo(output));
    if (!ool)
        return false;

    
    masm.convertDoubleToInt32(input, temp, ool->entry(), true);
    emitIntToString(temp, output, ool->entry());

    masm.bind(ool->rejoin());
    return true;
}

typedef JSString *(*PrimitiveToStringFn)(JSContext *, HandleValue);
typedef JSString *(*PrimitiveToStringParFn)(ForkJoinContext *, HandleValue);
static const VMFunctionsModal PrimitiveToStringInfo = VMFunctionsModal(
    FunctionInfo<PrimitiveToStringFn>(ToStringSlow),
    FunctionInfo<PrimitiveToStringParFn>(PrimitiveToStringPar));

bool
CodeGenerator::visitValueToString(LValueToString *lir)
{
    ValueOperand input = ToValue(lir, LValueToString::Input);
    Register output = ToRegister(lir->output());

    OutOfLineCode *ool = oolCallVM(PrimitiveToStringInfo, lir, (ArgList(), input),
                                   StoreRegisterTo(output));
    if (!ool)
        return false;

    Label done;
    Register tag = masm.splitTagForTest(input);
    const JSAtomState &names = GetIonContext()->runtime->names();

    
    if (lir->mir()->input()->mightBeType(MIRType_String)) {
        Label notString;
        masm.branchTestString(Assembler::NotEqual, tag, &notString);
        masm.unboxString(input, output);
        masm.jump(&done);
        masm.bind(&notString);
    }

    
    if (lir->mir()->input()->mightBeType(MIRType_Int32)) {
        Label notInteger;
        masm.branchTestInt32(Assembler::NotEqual, tag, &notInteger);
        Register unboxed = ToTempUnboxRegister(lir->tempToUnbox());
        unboxed = masm.extractInt32(input, unboxed);
        emitIntToString(unboxed, output, ool->entry());
        masm.jump(&done);
        masm.bind(&notInteger);
    }

    
    if (lir->mir()->input()->mightBeType(MIRType_Double)) {
        
        
        masm.branchTestDouble(Assembler::Equal, tag, ool->entry());
    }

    
    if (lir->mir()->input()->mightBeType(MIRType_Undefined)) {
        Label notUndefined;
        masm.branchTestUndefined(Assembler::NotEqual, tag, &notUndefined);
        masm.movePtr(ImmGCPtr(names.undefined), output);
        masm.jump(&done);
        masm.bind(&notUndefined);
    }

    
    if (lir->mir()->input()->mightBeType(MIRType_Null)) {
        Label notNull;
        masm.branchTestNull(Assembler::NotEqual, tag, &notNull);
        masm.movePtr(ImmGCPtr(names.null), output);
        masm.jump(&done);
        masm.bind(&notNull);
    }

    
    if (lir->mir()->input()->mightBeType(MIRType_Boolean)) {
        Label notBoolean, true_;
        masm.branchTestBoolean(Assembler::NotEqual, tag, &notBoolean);
        masm.branchTestBooleanTruthy(true, input, &true_);
        masm.movePtr(ImmGCPtr(names.false_), output);
        masm.jump(&done);
        masm.bind(&true_);
        masm.movePtr(ImmGCPtr(names.true_), output);
        masm.jump(&done);
        masm.bind(&notBoolean);
    }

    
    if (lir->mir()->input()->mightBeType(MIRType_Object)) {
        
        JS_ASSERT(lir->mir()->fallible());
        Label bail;
        masm.branchTestObject(Assembler::Equal, tag, &bail);
        if (!bailoutFrom(&bail, lir->snapshot()))
            return false;
    }

    
    if (lir->mir()->input()->mightBeType(MIRType_Symbol))
        masm.branchTestSymbol(Assembler::Equal, tag, ool->entry());

#ifdef DEBUG
    masm.assumeUnreachable("Unexpected type for MValueToString.");
#endif

    masm.bind(&done);
    masm.bind(ool->rejoin());
    return true;
}

typedef JSObject *(*CloneRegExpObjectFn)(JSContext *, JSObject *);
static const VMFunction CloneRegExpObjectInfo =
    FunctionInfo<CloneRegExpObjectFn>(CloneRegExpObject);

bool
CodeGenerator::visitRegExp(LRegExp *lir)
{
    pushArg(ImmGCPtr(lir->mir()->source()));
    return callVM(CloneRegExpObjectInfo, lir);
}

typedef bool (*RegExpExecRawFn)(JSContext *cx, HandleObject regexp,
                                HandleString input, MutableHandleValue output);
static const VMFunction RegExpExecRawInfo = FunctionInfo<RegExpExecRawFn>(regexp_exec_raw);

bool
CodeGenerator::visitRegExpExec(LRegExpExec *lir)
{
    pushArg(ToRegister(lir->string()));
    pushArg(ToRegister(lir->regexp()));
    return callVM(RegExpExecRawInfo, lir);
}

typedef bool (*RegExpTestRawFn)(JSContext *cx, HandleObject regexp,
                                HandleString input, bool *result);
static const VMFunction RegExpTestRawInfo = FunctionInfo<RegExpTestRawFn>(regexp_test_raw);

bool
CodeGenerator::visitRegExpTest(LRegExpTest *lir)
{
    pushArg(ToRegister(lir->string()));
    pushArg(ToRegister(lir->regexp()));
    return callVM(RegExpTestRawInfo, lir);
}

typedef JSString *(*RegExpReplaceFn)(JSContext *, HandleString, HandleObject, HandleString);
static const VMFunction RegExpReplaceInfo = FunctionInfo<RegExpReplaceFn>(RegExpReplace);

bool
CodeGenerator::visitRegExpReplace(LRegExpReplace *lir)
{
    if (lir->replacement()->isConstant())
        pushArg(ImmGCPtr(lir->replacement()->toConstant()->toString()));
    else
        pushArg(ToRegister(lir->replacement()));

    pushArg(ToRegister(lir->pattern()));

    if (lir->string()->isConstant())
        pushArg(ImmGCPtr(lir->string()->toConstant()->toString()));
    else
        pushArg(ToRegister(lir->string()));

    return callVM(RegExpReplaceInfo, lir);
}

typedef JSString *(*StringReplaceFn)(JSContext *, HandleString, HandleString, HandleString);
static const VMFunction StringReplaceInfo = FunctionInfo<StringReplaceFn>(StringReplace);

bool
CodeGenerator::visitStringReplace(LStringReplace *lir)
{
    if (lir->replacement()->isConstant())
        pushArg(ImmGCPtr(lir->replacement()->toConstant()->toString()));
    else
        pushArg(ToRegister(lir->replacement()));

    if (lir->pattern()->isConstant())
        pushArg(ImmGCPtr(lir->pattern()->toConstant()->toString()));
    else
        pushArg(ToRegister(lir->pattern()));

    if (lir->string()->isConstant())
        pushArg(ImmGCPtr(lir->string()->toConstant()->toString()));
    else
        pushArg(ToRegister(lir->string()));

    return callVM(StringReplaceInfo, lir);
}


typedef JSObject *(*LambdaFn)(JSContext *, HandleFunction, HandleObject);
static const VMFunction LambdaInfo = FunctionInfo<LambdaFn>(js::Lambda);

bool
CodeGenerator::visitLambdaForSingleton(LLambdaForSingleton *lir)
{
    pushArg(ToRegister(lir->scopeChain()));
    pushArg(ImmGCPtr(lir->mir()->info().fun));
    return callVM(LambdaInfo, lir);
}

bool
CodeGenerator::visitLambda(LLambda *lir)
{
    Register scopeChain = ToRegister(lir->scopeChain());
    Register output = ToRegister(lir->output());
    Register tempReg = ToRegister(lir->temp());
    const LambdaFunctionInfo &info = lir->mir()->info();

    OutOfLineCode *ool = oolCallVM(LambdaInfo, lir, (ArgList(), ImmGCPtr(info.fun), scopeChain),
                                   StoreRegisterTo(output));
    if (!ool)
        return false;

    JS_ASSERT(!info.singletonType);

    masm.createGCObject(output, tempReg, info.fun, gc::DefaultHeap, ool->entry());

    emitLambdaInit(output, scopeChain, info);

    masm.bind(ool->rejoin());
    return true;
}

typedef JSObject *(*LambdaArrowFn)(JSContext *, HandleFunction, HandleObject, HandleValue);
static const VMFunction LambdaArrowInfo = FunctionInfo<LambdaArrowFn>(js::LambdaArrow);

bool
CodeGenerator::visitLambdaArrow(LLambdaArrow *lir)
{
    Register scopeChain = ToRegister(lir->scopeChain());
    ValueOperand thisv = ToValue(lir, LLambdaArrow::ThisValue);
    Register output = ToRegister(lir->output());
    Register tempReg = ToRegister(lir->temp());
    const LambdaFunctionInfo &info = lir->mir()->info();

    OutOfLineCode *ool = oolCallVM(LambdaArrowInfo, lir,
                                   (ArgList(), ImmGCPtr(info.fun), scopeChain, thisv),
                                   StoreRegisterTo(output));
    if (!ool)
        return false;

    MOZ_ASSERT(!info.useNewTypeForClone);

    if (info.singletonType) {
        
        
        masm.jump(ool->entry());
        masm.bind(ool->rejoin());
        return true;
    }

    masm.createGCObject(output, tempReg, info.fun, gc::DefaultHeap, ool->entry());

    emitLambdaInit(output, scopeChain, info);

    
    MOZ_ASSERT(info.flags & JSFunction::EXTENDED);
    static_assert(FunctionExtended::NUM_EXTENDED_SLOTS == 2, "All slots must be initialized");
    static_assert(FunctionExtended::ARROW_THIS_SLOT == 0, "|this| must be stored in first slot");
    masm.storeValue(thisv, Address(output, FunctionExtended::offsetOfExtendedSlot(0)));
    masm.storeValue(UndefinedValue(), Address(output, FunctionExtended::offsetOfExtendedSlot(1)));

    masm.bind(ool->rejoin());
    return true;
}

void
CodeGenerator::emitLambdaInit(Register output, Register scopeChain,
                              const LambdaFunctionInfo &info)
{
    MOZ_ASSERT(!!(info.flags & JSFunction::ARROW) == !!(info.flags & JSFunction::EXTENDED));

    
    
    union {
        struct S {
            uint16_t nargs;
            uint16_t flags;
        } s;
        uint32_t word;
    } u;
    u.s.nargs = info.fun->nargs();
    u.s.flags = info.flags;

    JS_ASSERT(JSFunction::offsetOfFlags() == JSFunction::offsetOfNargs() + 2);
    masm.store32(Imm32(u.word), Address(output, JSFunction::offsetOfNargs()));
    masm.storePtr(ImmGCPtr(info.scriptOrLazyScript),
                  Address(output, JSFunction::offsetOfNativeOrScript()));
    masm.storePtr(scopeChain, Address(output, JSFunction::offsetOfEnvironment()));
    masm.storePtr(ImmGCPtr(info.fun->displayAtom()), Address(output, JSFunction::offsetOfAtom()));
}

bool
CodeGenerator::visitLambdaPar(LLambdaPar *lir)
{
    Register resultReg = ToRegister(lir->output());
    Register cxReg = ToRegister(lir->forkJoinContext());
    Register scopeChainReg = ToRegister(lir->scopeChain());
    Register tempReg1 = ToRegister(lir->getTemp0());
    Register tempReg2 = ToRegister(lir->getTemp1());
    const LambdaFunctionInfo &info = lir->mir()->info();

    JS_ASSERT(scopeChainReg != resultReg);

    if (!emitAllocateGCThingPar(lir, resultReg, cxReg, tempReg1, tempReg2, info.fun))
        return false;
    emitLambdaInit(resultReg, scopeChainReg, info);
    return true;
}

bool
CodeGenerator::visitLabel(LLabel *lir)
{
    return true;
}

bool
CodeGenerator::visitNop(LNop *lir)
{
    return true;
}

bool
CodeGenerator::visitOsiPoint(LOsiPoint *lir)
{
    
    

    JS_ASSERT(masm.framePushed() == frameSize());

    uint32_t osiCallPointOffset;
    if (!markOsiPoint(lir, &osiCallPointOffset))
        return false;

    LSafepoint *safepoint = lir->associatedSafepoint();
    JS_ASSERT(!safepoint->osiCallPointOffset());
    safepoint->setOsiCallPointOffset(osiCallPointOffset);

#ifdef DEBUG
    
    
    
    for (LInstructionReverseIterator iter(current->rbegin(lir)); iter != current->rend(); iter++) {
        if (*iter == lir || iter->isNop())
            continue;
        JS_ASSERT(!iter->isMoveGroup());
        JS_ASSERT(iter->safepoint() == safepoint);
        break;
    }
#endif

#ifdef CHECK_OSIPOINT_REGISTERS
    if (shouldVerifyOsiPointRegs(safepoint))
        verifyOsiPointRegs(safepoint);
#endif

    return true;
}

bool
CodeGenerator::visitGoto(LGoto *lir)
{
    jumpToBlock(lir->target());
    return true;
}



class OutOfLineInterruptCheckImplicit : public OutOfLineCodeBase<CodeGenerator>
{
  public:
    LBlock *block;
    LInterruptCheckImplicit *lir;

    OutOfLineInterruptCheckImplicit(LBlock *block, LInterruptCheckImplicit *lir)
      : block(block), lir(lir)
    { }

    bool accept(CodeGenerator *codegen) {
        return codegen->visitOutOfLineInterruptCheckImplicit(this);
    }
};

typedef bool (*InterruptCheckFn)(JSContext *);
typedef bool (*InterruptCheckParFn)(ForkJoinContext *);
static const VMFunctionsModal InterruptCheckInfo = VMFunctionsModal(
    FunctionInfo<InterruptCheckFn>(InterruptCheck),
    FunctionInfo<InterruptCheckParFn>(InterruptCheckPar));

bool
CodeGenerator::visitOutOfLineInterruptCheckImplicit(OutOfLineInterruptCheckImplicit *ool)
{
#ifdef CHECK_OSIPOINT_REGISTERS
    
    
    
    
    resetOsiPointRegs(ool->lir->safepoint());
#endif

    LInstructionIterator iter = ool->block->begin();
    for (; iter != ool->block->end(); iter++) {
        if (iter->isLabel()) {
            
        } else if (iter->isMoveGroup()) {
            
            
            
            visitMoveGroup(iter->toMoveGroup());
        } else {
            break;
        }
    }
    JS_ASSERT(*iter == ool->lir);

    saveLive(ool->lir);
    if (!callVM(InterruptCheckInfo, ool->lir))
        return false;
    restoreLive(ool->lir);
    masm.jump(ool->rejoin());

    return true;
}

bool
CodeGenerator::visitInterruptCheckImplicit(LInterruptCheckImplicit *lir)
{
    OutOfLineInterruptCheckImplicit *ool = new(alloc()) OutOfLineInterruptCheckImplicit(current, lir);
    if (!addOutOfLineCode(ool, lir->mir()))
        return false;

    lir->setOolEntry(ool->entry());
    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitTableSwitch(LTableSwitch *ins)
{
    MTableSwitch *mir = ins->mir();
    Label *defaultcase = skipTrivialBlocks(mir->getDefault())->lir()->label();
    const LAllocation *temp;

    if (mir->getOperand(0)->type() != MIRType_Int32) {
        temp = ins->tempInt()->output();

        
        
        masm.convertDoubleToInt32(ToFloatRegister(ins->index()), ToRegister(temp), defaultcase, false);
    } else {
        temp = ins->index();
    }

    return emitTableSwitchDispatch(mir, ToRegister(temp), ToRegisterOrInvalid(ins->tempPointer()));
}

bool
CodeGenerator::visitTableSwitchV(LTableSwitchV *ins)
{
    MTableSwitch *mir = ins->mir();
    Label *defaultcase = skipTrivialBlocks(mir->getDefault())->lir()->label();

    Register index = ToRegister(ins->tempInt());
    ValueOperand value = ToValue(ins, LTableSwitchV::InputValue);
    Register tag = masm.extractTag(value, index);
    masm.branchTestNumber(Assembler::NotEqual, tag, defaultcase);

    Label unboxInt, isInt;
    masm.branchTestInt32(Assembler::Equal, tag, &unboxInt);
    {
        FloatRegister floatIndex = ToFloatRegister(ins->tempFloat());
        masm.unboxDouble(value, floatIndex);
        masm.convertDoubleToInt32(floatIndex, index, defaultcase, false);
        masm.jump(&isInt);
    }

    masm.bind(&unboxInt);
    masm.unboxInt32(value, index);

    masm.bind(&isInt);

    return emitTableSwitchDispatch(mir, index, ToRegisterOrInvalid(ins->tempPointer()));
}

typedef JSObject *(*DeepCloneObjectLiteralFn)(JSContext *, HandleObject, NewObjectKind);
static const VMFunction DeepCloneObjectLiteralInfo =
    FunctionInfo<DeepCloneObjectLiteralFn>(DeepCloneObjectLiteral);

bool
CodeGenerator::visitCloneLiteral(LCloneLiteral *lir)
{
    pushArg(ImmWord(js::MaybeSingletonObject));
    pushArg(ToRegister(lir->getObjectLiteral()));
    return callVM(DeepCloneObjectLiteralInfo, lir);
}

bool
CodeGenerator::visitParameter(LParameter *lir)
{
    return true;
}

bool
CodeGenerator::visitCallee(LCallee *lir)
{
    Register callee = ToRegister(lir->output());
    Address ptr(StackPointer, frameSize() + IonJSFrameLayout::offsetOfCalleeToken());

    masm.loadFunctionFromCalleeToken(ptr, callee);
    return true;
}

bool
CodeGenerator::visitIsConstructing(LIsConstructing *lir)
{
    Register output = ToRegister(lir->output());
    Address calleeToken(StackPointer, frameSize() + IonJSFrameLayout::offsetOfCalleeToken());
    masm.loadPtr(calleeToken, output);

    
    MOZ_ASSERT(current->mir()->info().script()->functionNonDelazifying());

    
    
    static_assert(CalleeToken_Function == 0x0, "CalleeTokenTag value should match");
    static_assert(CalleeToken_FunctionConstructing == 0x1, "CalleeTokenTag value should match");
    masm.andPtr(Imm32(0x1), output);
    return true;
}

bool
CodeGenerator::visitStart(LStart *lir)
{
    return true;
}

bool
CodeGenerator::visitReturn(LReturn *lir)
{
#if defined(JS_NUNBOX32)
    DebugOnly<LAllocation *> type    = lir->getOperand(TYPE_INDEX);
    DebugOnly<LAllocation *> payload = lir->getOperand(PAYLOAD_INDEX);
    JS_ASSERT(ToRegister(type)    == JSReturnReg_Type);
    JS_ASSERT(ToRegister(payload) == JSReturnReg_Data);
#elif defined(JS_PUNBOX64)
    DebugOnly<LAllocation *> result = lir->getOperand(0);
    JS_ASSERT(ToRegister(result) == JSReturnReg);
#endif
    
    if (current->mir() != *gen->graph().poBegin())
        masm.jump(&returnLabel_);
    return true;
}

bool
CodeGenerator::visitOsrEntry(LOsrEntry *lir)
{
    
    masm.flushBuffer();
    setOsrEntryOffset(masm.size());

#ifdef JS_TRACE_LOGGING
    if (gen->info().executionMode() == SequentialExecution) {
        if (!emitTracelogStopEvent(TraceLogger::Baseline))
            return false;
        if (!emitTracelogStartEvent(TraceLogger::IonMonkey))
            return false;
    }
#endif

    
    uint32_t size = frameSize();
    if (size != 0)
        masm.subPtr(Imm32(size), StackPointer);
    return true;
}

bool
CodeGenerator::visitOsrScopeChain(LOsrScopeChain *lir)
{
    const LAllocation *frame   = lir->getOperand(0);
    const LDefinition *object  = lir->getDef(0);

    const ptrdiff_t frameOffset = BaselineFrame::reverseOffsetOfScopeChain();

    masm.loadPtr(Address(ToRegister(frame), frameOffset), ToRegister(object));
    return true;
}

bool
CodeGenerator::visitOsrArgumentsObject(LOsrArgumentsObject *lir)
{
    const LAllocation *frame   = lir->getOperand(0);
    const LDefinition *object  = lir->getDef(0);

    const ptrdiff_t frameOffset = BaselineFrame::reverseOffsetOfArgsObj();

    masm.loadPtr(Address(ToRegister(frame), frameOffset), ToRegister(object));
    return true;
}

bool
CodeGenerator::visitOsrValue(LOsrValue *value)
{
    const LAllocation *frame   = value->getOperand(0);
    const ValueOperand out     = ToOutValue(value);

    const ptrdiff_t frameOffset = value->mir()->frameOffset();

    masm.loadValue(Address(ToRegister(frame), frameOffset), out);
    return true;
}

bool
CodeGenerator::visitOsrReturnValue(LOsrReturnValue *lir)
{
    const LAllocation *frame   = lir->getOperand(0);
    const ValueOperand out     = ToOutValue(lir);

    Address flags = Address(ToRegister(frame), BaselineFrame::reverseOffsetOfFlags());
    Address retval = Address(ToRegister(frame), BaselineFrame::reverseOffsetOfReturnValue());

    masm.moveValue(UndefinedValue(), out);

    Label done;
    masm.branchTest32(Assembler::Zero, flags, Imm32(BaselineFrame::HAS_RVAL), &done);
    masm.loadValue(retval, out);
    masm.bind(&done);

    return true;
}

bool
CodeGenerator::visitStackArgT(LStackArgT *lir)
{
    const LAllocation *arg = lir->getArgument();
    MIRType argType = lir->type();
    uint32_t argslot = lir->argslot();
    JS_ASSERT(argslot - 1u < graph.argumentSlotCount());

    int32_t stack_offset = StackOffsetOfPassedArg(argslot);
    Address dest(StackPointer, stack_offset);

    if (arg->isFloatReg())
        masm.storeDouble(ToFloatRegister(arg), dest);
    else if (arg->isRegister())
        masm.storeValue(ValueTypeFromMIRType(argType), ToRegister(arg), dest);
    else
        masm.storeValue(*(arg->toConstant()), dest);

    uint32_t slot = StackOffsetToSlot(stack_offset);
    JS_ASSERT(slot - 1u < graph.totalSlotCount());
    return pushedArgumentSlots_.append(slot);
}

bool
CodeGenerator::visitStackArgV(LStackArgV *lir)
{
    ValueOperand val = ToValue(lir, 0);
    uint32_t argslot = lir->argslot();
    JS_ASSERT(argslot - 1u < graph.argumentSlotCount());

    int32_t stack_offset = StackOffsetOfPassedArg(argslot);

    masm.storeValue(val, Address(StackPointer, stack_offset));

    uint32_t slot = StackOffsetToSlot(stack_offset);
    JS_ASSERT(slot - 1u < graph.totalSlotCount());
    return pushedArgumentSlots_.append(slot);
}

bool
CodeGenerator::visitMoveGroup(LMoveGroup *group)
{
    if (!group->numMoves())
        return true;

    MoveResolver &resolver = masm.moveResolver();

    for (size_t i = 0; i < group->numMoves(); i++) {
        const LMove &move = group->getMove(i);

        const LAllocation *from = move.from();
        const LAllocation *to = move.to();
        LDefinition::Type type = move.type();

        
        JS_ASSERT(*from != *to);
        JS_ASSERT(!from->isConstant());
        MoveOp::Type moveType;
        switch (type) {
          case LDefinition::OBJECT:
          case LDefinition::SLOTS:
#ifdef JS_NUNBOX32
          case LDefinition::TYPE:
          case LDefinition::PAYLOAD:
#else
          case LDefinition::BOX:
#endif
          case LDefinition::GENERAL:    moveType = MoveOp::GENERAL;   break;
          case LDefinition::INT32:      moveType = MoveOp::INT32;     break;
          case LDefinition::FLOAT32:    moveType = MoveOp::FLOAT32;   break;
          case LDefinition::DOUBLE:     moveType = MoveOp::DOUBLE;    break;
          case LDefinition::INT32X4:    moveType = MoveOp::INT32X4;   break;
          case LDefinition::FLOAT32X4:  moveType = MoveOp::FLOAT32X4; break;
          default: MOZ_CRASH("Unexpected move type");
        }

        if (!resolver.addMove(toMoveOperand(from), toMoveOperand(to), moveType))
            return false;
    }

    if (!resolver.resolve())
        return false;

    MoveEmitter emitter(masm);
    emitter.emit(resolver);
    emitter.finish();

    return true;
}

bool
CodeGenerator::visitInteger(LInteger *lir)
{
    masm.move32(Imm32(lir->getValue()), ToRegister(lir->output()));
    return true;
}

bool
CodeGenerator::visitPointer(LPointer *lir)
{
    if (lir->kind() == LPointer::GC_THING)
        masm.movePtr(ImmGCPtr(lir->gcptr()), ToRegister(lir->output()));
    else
        masm.movePtr(ImmPtr(lir->ptr()), ToRegister(lir->output()));
    return true;
}

bool
CodeGenerator::visitSlots(LSlots *lir)
{
    Address slots(ToRegister(lir->object()), JSObject::offsetOfSlots());
    masm.loadPtr(slots, ToRegister(lir->output()));
    return true;
}

bool
CodeGenerator::visitLoadSlotT(LLoadSlotT *lir)
{
    Register base = ToRegister(lir->slots());
    int32_t offset = lir->mir()->slot() * sizeof(js::Value);
    AnyRegister result = ToAnyRegister(lir->output());

    masm.loadUnboxedValue(Address(base, offset), lir->mir()->type(), result);
    return true;
}

bool
CodeGenerator::visitLoadSlotV(LLoadSlotV *lir)
{
    ValueOperand dest = ToOutValue(lir);
    Register base = ToRegister(lir->input());
    int32_t offset = lir->mir()->slot() * sizeof(js::Value);

    masm.loadValue(Address(base, offset), dest);
    return true;
}

bool
CodeGenerator::visitStoreSlotT(LStoreSlotT *lir)
{
    Register base = ToRegister(lir->slots());
    int32_t offset = lir->mir()->slot() * sizeof(js::Value);
    Address dest(base, offset);

    if (lir->mir()->needsBarrier())
        emitPreBarrier(dest);

    MIRType valueType = lir->mir()->value()->type();
    ConstantOrRegister value;
    if (lir->value()->isConstant())
        value = ConstantOrRegister(*lir->value()->toConstant());
    else
        value = TypedOrValueRegister(valueType, ToAnyRegister(lir->value()));

    masm.storeUnboxedValue(value, valueType, dest, lir->mir()->slotType());
    return true;
}

bool
CodeGenerator::visitStoreSlotV(LStoreSlotV *lir)
{
    Register base = ToRegister(lir->slots());
    int32_t offset = lir->mir()->slot() * sizeof(Value);

    const ValueOperand value = ToValue(lir, LStoreSlotV::Value);

    if (lir->mir()->needsBarrier())
       emitPreBarrier(Address(base, offset));

    masm.storeValue(value, Address(base, offset));
    return true;
}

bool
CodeGenerator::emitGetPropertyPolymorphic(LInstruction *ins, Register obj, Register scratch,
                                          const TypedOrValueRegister &output)
{
    MGetPropertyPolymorphic *mir = ins->mirRaw()->toGetPropertyPolymorphic();
    JS_ASSERT(mir->numShapes() > 1);

    masm.loadObjShape(obj, scratch);

    Label done;
    for (size_t i = 0; i < mir->numShapes(); i++) {
        Label next;
        if (i == mir->numShapes() - 1) {
            if (!bailoutCmpPtr(Assembler::NotEqual, scratch, ImmGCPtr(mir->objShape(i)),
                               ins->snapshot()))
            {
                return false;
            }
        } else {
            masm.branchPtr(Assembler::NotEqual, scratch, ImmGCPtr(mir->objShape(i)), &next);
        }

        Shape *shape = mir->shape(i);
        if (shape->slot() < shape->numFixedSlots()) {
            
            masm.loadTypedOrValue(Address(obj, JSObject::getFixedSlotOffset(shape->slot())),
                                  output);
        } else {
            
            uint32_t offset = (shape->slot() - shape->numFixedSlots()) * sizeof(js::Value);
            masm.loadPtr(Address(obj, JSObject::offsetOfSlots()), scratch);
            masm.loadTypedOrValue(Address(scratch, offset), output);
        }

        if (i != mir->numShapes() - 1)
            masm.jump(&done);
        masm.bind(&next);
    }

    masm.bind(&done);
    return true;
}

bool
CodeGenerator::visitGetPropertyPolymorphicV(LGetPropertyPolymorphicV *ins)
{
    Register obj = ToRegister(ins->obj());
    ValueOperand output = GetValueOutput(ins);
    return emitGetPropertyPolymorphic(ins, obj, output.scratchReg(), output);
}

bool
CodeGenerator::visitGetPropertyPolymorphicT(LGetPropertyPolymorphicT *ins)
{
    Register obj = ToRegister(ins->obj());
    TypedOrValueRegister output(ins->mir()->type(), ToAnyRegister(ins->output()));
    Register temp = (output.type() == MIRType_Double)
                    ? ToRegister(ins->temp())
                    : output.typedReg().gpr();
    return emitGetPropertyPolymorphic(ins, obj, temp, output);
}

bool
CodeGenerator::emitSetPropertyPolymorphic(LInstruction *ins, Register obj, Register scratch,
                                          const ConstantOrRegister &value)
{
    MSetPropertyPolymorphic *mir = ins->mirRaw()->toSetPropertyPolymorphic();
    JS_ASSERT(mir->numShapes() > 1);

    masm.loadObjShape(obj, scratch);

    Label done;
    for (size_t i = 0; i < mir->numShapes(); i++) {
        Label next;
        if (i == mir->numShapes() - 1) {
            if (!bailoutCmpPtr(Assembler::NotEqual, scratch, ImmGCPtr(mir->objShape(i)),
                               ins->snapshot()))
            {
                return false;
            }
        } else {
            masm.branchPtr(Assembler::NotEqual, scratch, ImmGCPtr(mir->objShape(i)), &next);
        }

        Shape *shape = mir->shape(i);
        if (shape->slot() < shape->numFixedSlots()) {
            
            Address addr(obj, JSObject::getFixedSlotOffset(shape->slot()));
            if (mir->needsBarrier())
                emitPreBarrier(addr);
            masm.storeConstantOrRegister(value, addr);
        } else {
            
            masm.loadPtr(Address(obj, JSObject::offsetOfSlots()), scratch);
            Address addr(scratch, (shape->slot() - shape->numFixedSlots()) * sizeof(js::Value));
            if (mir->needsBarrier())
                emitPreBarrier(addr);
            masm.storeConstantOrRegister(value, addr);
        }

        if (i != mir->numShapes() - 1)
            masm.jump(&done);
        masm.bind(&next);
    }

    masm.bind(&done);
    return true;
}

bool
CodeGenerator::visitSetPropertyPolymorphicV(LSetPropertyPolymorphicV *ins)
{
    Register obj = ToRegister(ins->obj());
    Register temp = ToRegister(ins->temp());
    ValueOperand value = ToValue(ins, LSetPropertyPolymorphicV::Value);
    return emitSetPropertyPolymorphic(ins, obj, temp, TypedOrValueRegister(value));
}

bool
CodeGenerator::visitSetPropertyPolymorphicT(LSetPropertyPolymorphicT *ins)
{
    Register obj = ToRegister(ins->obj());
    Register temp = ToRegister(ins->temp());

    ConstantOrRegister value;
    if (ins->mir()->value()->isConstant())
        value = ConstantOrRegister(ins->mir()->value()->toConstant()->value());
    else
        value = TypedOrValueRegister(ins->mir()->value()->type(), ToAnyRegister(ins->value()));

    return emitSetPropertyPolymorphic(ins, obj, temp, value);
}

bool
CodeGenerator::visitElements(LElements *lir)
{
    Address elements(ToRegister(lir->object()), JSObject::offsetOfElements());
    masm.loadPtr(elements, ToRegister(lir->output()));
    return true;
}

typedef bool (*ConvertElementsToDoublesFn)(JSContext *, uintptr_t);
static const VMFunction ConvertElementsToDoublesInfo =
    FunctionInfo<ConvertElementsToDoublesFn>(ObjectElements::ConvertElementsToDoubles);

bool
CodeGenerator::visitConvertElementsToDoubles(LConvertElementsToDoubles *lir)
{
    Register elements = ToRegister(lir->elements());

    OutOfLineCode *ool = oolCallVM(ConvertElementsToDoublesInfo, lir,
                                   (ArgList(), elements), StoreNothing());
    if (!ool)
        return false;

    Address convertedAddress(elements, ObjectElements::offsetOfFlags());
    Imm32 bit(ObjectElements::CONVERT_DOUBLE_ELEMENTS);
    masm.branchTest32(Assembler::Zero, convertedAddress, bit, ool->entry());
    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitMaybeToDoubleElement(LMaybeToDoubleElement *lir)
{
    Register elements = ToRegister(lir->elements());
    Register value = ToRegister(lir->value());
    ValueOperand out = ToOutValue(lir);

    FloatRegister temp = ToFloatRegister(lir->tempFloat());
    Label convert, done;

    
    
    masm.branchTest32(Assembler::NonZero,
                      Address(elements, ObjectElements::offsetOfFlags()),
                      Imm32(ObjectElements::CONVERT_DOUBLE_ELEMENTS),
                      &convert);

    masm.tagValue(JSVAL_TYPE_INT32, value, out);
    masm.jump(&done);

    masm.bind(&convert);
    masm.convertInt32ToDouble(value, temp);
    masm.boxDouble(temp, out);

    masm.bind(&done);
    return true;
}

typedef bool (*CopyElementsForWriteFn)(ThreadSafeContext *, JSObject *);
static const VMFunction CopyElementsForWriteInfo =
    FunctionInfo<CopyElementsForWriteFn>(JSObject::CopyElementsForWrite);

bool
CodeGenerator::visitMaybeCopyElementsForWrite(LMaybeCopyElementsForWrite *lir)
{
    Register object = ToRegister(lir->object());
    Register temp = ToRegister(lir->temp());

    OutOfLineCode *ool = oolCallVM(CopyElementsForWriteInfo, lir,
                                   (ArgList(), object), StoreNothing());
    if (!ool)
        return false;

    masm.loadPtr(Address(object, JSObject::offsetOfElements()), temp);
    masm.branchTest32(Assembler::NonZero,
                      Address(temp, ObjectElements::offsetOfFlags()),
                      Imm32(ObjectElements::COPY_ON_WRITE),
                      ool->entry());
    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitFunctionEnvironment(LFunctionEnvironment *lir)
{
    Address environment(ToRegister(lir->function()), JSFunction::offsetOfEnvironment());
    masm.loadPtr(environment, ToRegister(lir->output()));
    return true;
}

bool
CodeGenerator::visitForkJoinContext(LForkJoinContext *lir)
{
    const Register tempReg = ToRegister(lir->getTempReg());

    masm.setupUnalignedABICall(0, tempReg);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ForkJoinContextPar));
    JS_ASSERT(ToRegister(lir->output()) == ReturnReg);
    return true;
}

bool
CodeGenerator::visitGuardThreadExclusive(LGuardThreadExclusive *lir)
{
    JS_ASSERT(gen->info().executionMode() == ParallelExecution);

    const Register tempReg = ToRegister(lir->getTempReg());
    masm.setupUnalignedABICall(2, tempReg);
    masm.passABIArg(ToRegister(lir->forkJoinContext()));
    masm.passABIArg(ToRegister(lir->object()));
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ParallelWriteGuard));

    return bailoutIfFalseBool(ReturnReg, lir->snapshot());
}

bool
CodeGenerator::visitGuardObjectIdentity(LGuardObjectIdentity *guard)
{
    Register obj = ToRegister(guard->input());

    Assembler::Condition cond =
        guard->mir()->bailOnEquality() ? Assembler::Equal : Assembler::NotEqual;
    return bailoutCmpPtr(cond, obj, ImmGCPtr(guard->mir()->singleObject()), guard->snapshot());
}

bool
CodeGenerator::visitGuardShapePolymorphic(LGuardShapePolymorphic *lir)
{
    const MGuardShapePolymorphic *mir = lir->mir();
    Register obj = ToRegister(lir->object());
    Register temp = ToRegister(lir->temp());

    MOZ_ASSERT(mir->numShapes() > 1);

    Label done;
    masm.loadObjShape(obj, temp);

    for (size_t i = 0; i < mir->numShapes(); i++) {
        Shape *shape = mir->getShape(i);
        if (i == mir->numShapes() - 1) {
            if (!bailoutCmpPtr(Assembler::NotEqual, temp, ImmGCPtr(shape), lir->snapshot()))
                return false;
        } else {
            masm.branchPtr(Assembler::Equal, temp, ImmGCPtr(shape), &done);
        }
    }

    masm.bind(&done);
    return true;
}

bool
CodeGenerator::visitTypeBarrierV(LTypeBarrierV *lir)
{
    ValueOperand operand = ToValue(lir, LTypeBarrierV::Input);
    Register scratch = ToTempRegisterOrInvalid(lir->temp());

    Label miss;
    masm.guardTypeSet(operand, lir->mir()->resultTypeSet(), lir->mir()->barrierKind(), scratch, &miss);
    if (!bailoutFrom(&miss, lir->snapshot()))
        return false;
    return true;
}

bool
CodeGenerator::visitTypeBarrierO(LTypeBarrierO *lir)
{
    MOZ_ASSERT(lir->mir()->barrierKind() != BarrierKind::TypeTagOnly);

    Register obj = ToRegister(lir->object());
    Register scratch = ToTempRegisterOrInvalid(lir->temp());

    Label miss;
    masm.guardObjectType(obj, lir->mir()->resultTypeSet(), scratch, &miss);
    if (!bailoutFrom(&miss, lir->snapshot()))
        return false;
    return true;
}

bool
CodeGenerator::visitMonitorTypes(LMonitorTypes *lir)
{
    ValueOperand operand = ToValue(lir, LMonitorTypes::Input);
    Register scratch = ToTempUnboxRegister(lir->temp());

    Label matched, miss;
    masm.guardTypeSet(operand, lir->mir()->typeSet(), lir->mir()->barrierKind(), scratch, &miss);
    if (!bailoutFrom(&miss, lir->snapshot()))
        return false;
    return true;
}

#ifdef JSGC_GENERATIONAL

class OutOfLineCallPostWriteBarrier : public OutOfLineCodeBase<CodeGenerator>
{
    LInstruction *lir_;
    const LAllocation *object_;

  public:
    OutOfLineCallPostWriteBarrier(LInstruction *lir, const LAllocation *object)
      : lir_(lir), object_(object)
    { }

    bool accept(CodeGenerator *codegen) {
        return codegen->visitOutOfLineCallPostWriteBarrier(this);
    }

    LInstruction *lir() const {
        return lir_;
    }
    const LAllocation *object() const {
        return object_;
    }
};

bool
CodeGenerator::visitOutOfLineCallPostWriteBarrier(OutOfLineCallPostWriteBarrier *ool)
{
    saveLiveVolatile(ool->lir());

    const LAllocation *obj = ool->object();

    GeneralRegisterSet regs = GeneralRegisterSet::Volatile();

    Register objreg;
    bool isGlobal = false;
    if (obj->isConstant()) {
        JSObject *object = &obj->toConstant()->toObject();
        isGlobal = object->is<GlobalObject>();
        objreg = regs.takeAny();
        masm.movePtr(ImmGCPtr(object), objreg);
    } else {
        objreg = ToRegister(obj);
        regs.takeUnchecked(objreg);
    }

    Register runtimereg = regs.takeAny();
    masm.mov(ImmPtr(GetIonContext()->runtime), runtimereg);

    void (*fun)(JSRuntime*, JSObject*) = isGlobal ? PostGlobalWriteBarrier : PostWriteBarrier;
    masm.setupUnalignedABICall(2, regs.takeAny());
    masm.passABIArg(runtimereg);
    masm.passABIArg(objreg);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, fun));

    restoreLiveVolatile(ool->lir());

    masm.jump(ool->rejoin());
    return true;
}
#endif

bool
CodeGenerator::visitPostWriteBarrierO(LPostWriteBarrierO *lir)
{
#ifdef JSGC_GENERATIONAL
    OutOfLineCallPostWriteBarrier *ool = new(alloc()) OutOfLineCallPostWriteBarrier(lir, lir->object());
    if (!addOutOfLineCode(ool, lir->mir()))
        return false;

    Register temp = ToTempRegisterOrInvalid(lir->temp());

    if (lir->object()->isConstant()) {
#ifdef DEBUG
        JS_ASSERT(!IsInsideNursery(&lir->object()->toConstant()->toObject()));
#endif
    } else {
        masm.branchPtrInNurseryRange(Assembler::Equal, ToRegister(lir->object()), temp,
                                     ool->rejoin());
    }

    masm.branchPtrInNurseryRange(Assembler::Equal, ToRegister(lir->value()), temp, ool->entry());

    masm.bind(ool->rejoin());
#endif
    return true;
}

bool
CodeGenerator::visitPostWriteBarrierV(LPostWriteBarrierV *lir)
{
#ifdef JSGC_GENERATIONAL
    OutOfLineCallPostWriteBarrier *ool = new(alloc()) OutOfLineCallPostWriteBarrier(lir, lir->object());
    if (!addOutOfLineCode(ool, lir->mir()))
        return false;

    Register temp = ToTempRegisterOrInvalid(lir->temp());

    if (lir->object()->isConstant()) {
#ifdef DEBUG
        JS_ASSERT(!IsInsideNursery(&lir->object()->toConstant()->toObject()));
#endif
    } else {
        masm.branchPtrInNurseryRange(Assembler::Equal, ToRegister(lir->object()), temp,
                                     ool->rejoin());
    }

    ValueOperand value = ToValue(lir, LPostWriteBarrierV::Input);
    masm.branchValueIsNurseryObject(Assembler::Equal, value, temp, ool->entry());

    masm.bind(ool->rejoin());
#endif
    return true;
}

bool
CodeGenerator::visitCallNative(LCallNative *call)
{
    JSFunction *target = call->getSingleTarget();
    JS_ASSERT(target);
    JS_ASSERT(target->isNative());

    int callargslot = call->argslot();
    int unusedStack = StackOffsetOfPassedArg(callargslot);

    
    const Register argContextReg   = ToRegister(call->getArgContextReg());
    const Register argUintNReg     = ToRegister(call->getArgUintNReg());
    const Register argVpReg        = ToRegister(call->getArgVpReg());

    
    const Register tempReg = ToRegister(call->getTempReg());

    DebugOnly<uint32_t> initialStack = masm.framePushed();

    masm.checkStackAlignment();

    
    
    
    
    
    

    
    masm.adjustStack(unusedStack);

    
    
    masm.Push(ObjectValue(*target));

    
    
    
    
    
    ExecutionMode executionMode = gen->info().executionMode();
    masm.loadContext(argContextReg, tempReg, executionMode);
    masm.move32(Imm32(call->numStackArgs()), argUintNReg);
    masm.movePtr(StackPointer, argVpReg);

    masm.Push(argUintNReg);

    
    uint32_t safepointOffset;
    if (!masm.buildFakeExitFrame(tempReg, &safepointOffset))
        return false;
    masm.enterFakeExitFrame(argContextReg, tempReg, executionMode,
                            IonNativeExitFrameLayout::Token());

    if (!markSafepointAt(safepointOffset, call))
        return false;

    
    masm.setupUnalignedABICall(3, tempReg);
    masm.passABIArg(argContextReg);
    masm.passABIArg(argUintNReg);
    masm.passABIArg(argVpReg);

    switch (executionMode) {
      case SequentialExecution:
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, target->native()));
        break;

      case ParallelExecution:
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, target->parallelNative()));
        break;

      default:
        MOZ_CRASH("No such execution mode");
    }

    
    masm.branchIfFalseBool(ReturnReg, masm.failureLabel(executionMode));

    
    masm.loadValue(Address(StackPointer, IonNativeExitFrameLayout::offsetOfResult()), JSReturnOperand);

    
    

    
    masm.adjustStack(IonNativeExitFrameLayout::Size() - unusedStack);
    JS_ASSERT(masm.framePushed() == initialStack);

    dropArguments(call->numStackArgs() + 1);
    return true;
}

bool
CodeGenerator::visitCallDOMNative(LCallDOMNative *call)
{
    JSFunction *target = call->getSingleTarget();
    JS_ASSERT(target);
    JS_ASSERT(target->isNative());
    JS_ASSERT(target->jitInfo());
    JS_ASSERT(call->mir()->isCallDOMNative());

    int callargslot = call->argslot();
    int unusedStack = StackOffsetOfPassedArg(callargslot);

    
    const Register argJSContext = ToRegister(call->getArgJSContext());
    const Register argObj       = ToRegister(call->getArgObj());
    const Register argPrivate   = ToRegister(call->getArgPrivate());
    const Register argArgs      = ToRegister(call->getArgArgs());

    DebugOnly<uint32_t> initialStack = masm.framePushed();

    masm.checkStackAlignment();

    
    
    
    
    
    

    
    
    masm.adjustStack(unusedStack);
    
    Register obj = masm.extractObject(Address(StackPointer, 0), argObj);
    JS_ASSERT(obj == argObj);

    
    
    masm.Push(ObjectValue(*target));

    
    
    
    JS_STATIC_ASSERT(JSJitMethodCallArgsTraits::offsetOfArgv == 0);
    JS_STATIC_ASSERT(JSJitMethodCallArgsTraits::offsetOfArgc ==
                     IonDOMMethodExitFrameLayoutTraits::offsetOfArgcFromArgv);
    masm.computeEffectiveAddress(Address(StackPointer, 2 * sizeof(Value)), argArgs);

    
    masm.loadPrivate(Address(obj, JSObject::getFixedSlotOffset(0)), argPrivate);

    
    masm.Push(Imm32(call->numStackArgs()));

    
    masm.Push(argArgs);
    
    masm.movePtr(StackPointer, argArgs);

    
    
    
    masm.Push(argObj);
    masm.movePtr(StackPointer, argObj);

    
    uint32_t safepointOffset;
    if (!masm.buildFakeExitFrame(argJSContext, &safepointOffset))
        return false;
    masm.enterFakeExitFrame(IonDOMMethodExitFrameLayout::Token());

    if (!markSafepointAt(safepointOffset, call))
        return false;

    
    masm.setupUnalignedABICall(4, argJSContext);

    masm.loadJSContext(argJSContext);

    masm.passABIArg(argJSContext);
    masm.passABIArg(argObj);
    masm.passABIArg(argPrivate);
    masm.passABIArg(argArgs);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, target->jitInfo()->method));

    if (target->jitInfo()->isInfallible) {
        masm.loadValue(Address(StackPointer, IonDOMMethodExitFrameLayout::offsetOfResult()),
                       JSReturnOperand);
    } else {
        
        masm.branchIfFalseBool(ReturnReg, masm.exceptionLabel());

        
        masm.loadValue(Address(StackPointer, IonDOMMethodExitFrameLayout::offsetOfResult()),
                       JSReturnOperand);
    }

    
    

    
    masm.adjustStack(IonDOMMethodExitFrameLayout::Size() - unusedStack);
    JS_ASSERT(masm.framePushed() == initialStack);

    dropArguments(call->numStackArgs() + 1);
    return true;
}

typedef bool (*GetIntrinsicValueFn)(JSContext *cx, HandlePropertyName, MutableHandleValue);
static const VMFunction GetIntrinsicValueInfo =
    FunctionInfo<GetIntrinsicValueFn>(GetIntrinsicValue);

bool
CodeGenerator::visitCallGetIntrinsicValue(LCallGetIntrinsicValue *lir)
{
    pushArg(ImmGCPtr(lir->mir()->name()));
    return callVM(GetIntrinsicValueInfo, lir);
}

typedef bool (*InvokeFunctionFn)(JSContext *, HandleObject, uint32_t, Value *, Value *);
static const VMFunction InvokeFunctionInfo = FunctionInfo<InvokeFunctionFn>(InvokeFunction);

bool
CodeGenerator::emitCallInvokeFunction(LInstruction *call, Register calleereg,
                                      uint32_t argc, uint32_t unusedStack)
{
    
    
    masm.freeStack(unusedStack);

    pushArg(StackPointer); 
    pushArg(Imm32(argc));  
    pushArg(calleereg);    

    if (!callVM(InvokeFunctionInfo, call))
        return false;

    
    masm.reserveStack(unusedStack);
    return true;
}

bool
CodeGenerator::visitCallGeneric(LCallGeneric *call)
{
    Register calleereg = ToRegister(call->getFunction());
    Register objreg    = ToRegister(call->getTempObject());
    Register nargsreg  = ToRegister(call->getNargsReg());
    uint32_t unusedStack = StackOffsetOfPassedArg(call->argslot());
    ExecutionMode executionMode = gen->info().executionMode();
    Label invoke, thunk, makeCall, end;

    
    JS_ASSERT(!call->hasSingleTarget());

    
    JitCode *argumentsRectifier = gen->jitRuntime()->getArgumentsRectifier(executionMode);

    masm.checkStackAlignment();

    
    masm.loadObjClass(calleereg, nargsreg);
    masm.branchPtr(Assembler::NotEqual, nargsreg, ImmPtr(&JSFunction::class_), &invoke);

    
    
    if (call->mir()->isConstructing())
        masm.branchIfNotInterpretedConstructor(calleereg, nargsreg, &invoke);
    else
        masm.branchIfFunctionHasNoScript(calleereg, &invoke);

    
    masm.loadPtr(Address(calleereg, JSFunction::offsetOfNativeOrScript()), objreg);

    
    masm.loadBaselineOrIonRaw(objreg, objreg, executionMode, &invoke);

    
    masm.freeStack(unusedStack);

    
    uint32_t descriptor = MakeFrameDescriptor(masm.framePushed(), JitFrame_IonJS);
    masm.Push(Imm32(call->numActualArgs()));
    masm.PushCalleeToken(calleereg, call->mir()->isConstructing());
    masm.Push(Imm32(descriptor));

    
    masm.load16ZeroExtend(Address(calleereg, JSFunction::offsetOfNargs()), nargsreg);
    masm.branch32(Assembler::Above, nargsreg, Imm32(call->numStackArgs()), &thunk);
    masm.jump(&makeCall);

    
    masm.bind(&thunk);
    {
        JS_ASSERT(ArgumentsRectifierReg != objreg);
        masm.movePtr(ImmGCPtr(argumentsRectifier), objreg); 
        masm.loadPtr(Address(objreg, JitCode::offsetOfCode()), objreg);
        masm.move32(Imm32(call->numStackArgs()), ArgumentsRectifierReg);
    }

    
    masm.bind(&makeCall);
    uint32_t callOffset = masm.callIon(objreg);
    if (!markSafepointAt(callOffset, call))
        return false;

    
    
    int prefixGarbage = sizeof(IonJSFrameLayout) - sizeof(void *);
    masm.adjustStack(prefixGarbage - unusedStack);
    masm.jump(&end);

    
    masm.bind(&invoke);
    switch (executionMode) {
      case SequentialExecution:
        if (!emitCallInvokeFunction(call, calleereg, call->numActualArgs(), unusedStack))
            return false;
        break;

      case ParallelExecution:
        if (!emitCallToUncompiledScriptPar(call, calleereg))
            return false;
        break;

      default:
        MOZ_CRASH("No such execution mode");
    }

    masm.bind(&end);

    
    
    if (call->mir()->isConstructing()) {
        Label notPrimitive;
        masm.branchTestPrimitive(Assembler::NotEqual, JSReturnOperand, &notPrimitive);
        masm.loadValue(Address(StackPointer, unusedStack), JSReturnOperand);
        masm.bind(&notPrimitive);
    }

    dropArguments(call->numStackArgs() + 1);
    return true;
}

typedef bool (*CallToUncompiledScriptParFn)(ForkJoinContext *, JSObject *);
static const VMFunction CallToUncompiledScriptParInfo =
    FunctionInfo<CallToUncompiledScriptParFn>(CallToUncompiledScriptPar);



bool
CodeGenerator::emitCallToUncompiledScriptPar(LInstruction *lir, Register calleeReg)
{
    pushArg(calleeReg);
    if (!callVM(CallToUncompiledScriptParInfo, lir))
        return false;
    masm.assumeUnreachable("CallToUncompiledScriptParInfo always returns false.");
    return true;
}

bool
CodeGenerator::visitCallKnown(LCallKnown *call)
{
    Register calleereg = ToRegister(call->getFunction());
    Register objreg    = ToRegister(call->getTempObject());
    uint32_t unusedStack = StackOffsetOfPassedArg(call->argslot());
    DebugOnly<JSFunction *> target = call->getSingleTarget();
    ExecutionMode executionMode = gen->info().executionMode();
    Label end, uncompiled;

    
    JS_ASSERT(!target->isNative());
    
    JS_ASSERT(target->nargs() <= call->numStackArgs());

    JS_ASSERT_IF(call->mir()->isConstructing(), target->isInterpretedConstructor());

    masm.checkStackAlignment();

    
    
    masm.branchIfFunctionHasNoScript(calleereg, &uncompiled);

    
    masm.loadPtr(Address(calleereg, JSFunction::offsetOfNativeOrScript()), objreg);

    
    if (call->mir()->needsArgCheck())
        masm.loadBaselineOrIonRaw(objreg, objreg, executionMode, &uncompiled);
    else
        masm.loadBaselineOrIonNoArgCheck(objreg, objreg, executionMode, &uncompiled);

    
    masm.freeStack(unusedStack);

    
    uint32_t descriptor = MakeFrameDescriptor(masm.framePushed(), JitFrame_IonJS);
    masm.Push(Imm32(call->numActualArgs()));
    masm.PushCalleeToken(calleereg, call->mir()->isConstructing());
    masm.Push(Imm32(descriptor));

    
    uint32_t callOffset = masm.callIon(objreg);
    if (!markSafepointAt(callOffset, call))
        return false;

    
    
    int prefixGarbage = sizeof(IonJSFrameLayout) - sizeof(void *);
    masm.adjustStack(prefixGarbage - unusedStack);
    masm.jump(&end);

    
    masm.bind(&uncompiled);
    switch (executionMode) {
      case SequentialExecution:
        if (!emitCallInvokeFunction(call, calleereg, call->numActualArgs(), unusedStack))
            return false;
        break;

      case ParallelExecution:
        if (!emitCallToUncompiledScriptPar(call, calleereg))
            return false;
        break;

      default:
        MOZ_CRASH("No such execution mode");
    }

    masm.bind(&end);

    
    
    if (call->mir()->isConstructing()) {
        Label notPrimitive;
        masm.branchTestPrimitive(Assembler::NotEqual, JSReturnOperand, &notPrimitive);
        masm.loadValue(Address(StackPointer, unusedStack), JSReturnOperand);
        masm.bind(&notPrimitive);
    }

    dropArguments(call->numStackArgs() + 1);
    return true;
}

bool
CodeGenerator::emitCallInvokeFunction(LApplyArgsGeneric *apply, Register extraStackSize)
{
    Register objreg = ToRegister(apply->getTempObject());
    JS_ASSERT(objreg != extraStackSize);

    
    masm.movePtr(StackPointer, objreg);
    masm.Push(extraStackSize);

    pushArg(objreg);                           
    pushArg(ToRegister(apply->getArgc()));     
    pushArg(ToRegister(apply->getFunction())); 

    
    if (!callVM(InvokeFunctionInfo, apply, &extraStackSize))
        return false;

    masm.Pop(extraStackSize);
    return true;
}



void
CodeGenerator::emitPushArguments(LApplyArgsGeneric *apply, Register extraStackSpace)
{
    
    Register argcreg = ToRegister(apply->getArgc());

    Register copyreg = ToRegister(apply->getTempObject());
    size_t argvOffset = frameSize() + IonJSFrameLayout::offsetOfActualArgs();
    Label end;

    
    masm.movePtr(argcreg, extraStackSpace);
    masm.branchTestPtr(Assembler::Zero, argcreg, argcreg, &end);

    
    {
        Register count = extraStackSpace; 
        Label loop;
        masm.bind(&loop);

        
        
        BaseIndex disp(StackPointer, argcreg, ScaleFromElemWidth(sizeof(Value)), argvOffset - sizeof(void*));

        
        
        masm.loadPtr(disp, copyreg);
        masm.push(copyreg);

        
        if (sizeof(Value) == 2 * sizeof(void*)) {
            masm.loadPtr(disp, copyreg);
            masm.push(copyreg);
        }

        masm.decBranchPtr(Assembler::NonZero, count, Imm32(1), &loop);
    }

    
    masm.movePtr(argcreg, extraStackSpace);
    masm.lshiftPtr(Imm32::ShiftOf(ScaleFromElemWidth(sizeof(Value))), extraStackSpace);

    
    masm.bind(&end);

    
    masm.addPtr(Imm32(sizeof(Value)), extraStackSpace);
    masm.pushValue(ToValue(apply, LApplyArgsGeneric::ThisIndex));
}

void
CodeGenerator::emitPopArguments(LApplyArgsGeneric *apply, Register extraStackSpace)
{
    
    masm.freeStack(extraStackSpace);
}

bool
CodeGenerator::visitApplyArgsGeneric(LApplyArgsGeneric *apply)
{
    
    Register calleereg = ToRegister(apply->getFunction());

    
    Register objreg = ToRegister(apply->getTempObject());
    Register copyreg = ToRegister(apply->getTempCopy());

    
    Register argcreg = ToRegister(apply->getArgc());

    
    if (!apply->hasSingleTarget()) {
        masm.loadObjClass(calleereg, objreg);

        ImmPtr ptr = ImmPtr(&JSFunction::class_);
        if (!bailoutCmpPtr(Assembler::NotEqual, objreg, ptr, apply->snapshot()))
            return false;
    }

    
    emitPushArguments(apply, copyreg);

    masm.checkStackAlignment();

    
    ExecutionMode executionMode = gen->info().executionMode();
    if (apply->hasSingleTarget()) {
        JSFunction *target = apply->getSingleTarget();
        if (target->isNative()) {
            if (!emitCallInvokeFunction(apply, copyreg))
                return false;
            emitPopArguments(apply, copyreg);
            return true;
        }
    }

    Label end, invoke;

    
    if (!apply->hasSingleTarget()) {
        masm.branchIfFunctionHasNoScript(calleereg, &invoke);
    } else {
        
        JS_ASSERT(!apply->getSingleTarget()->isNative());
    }

    
    masm.loadPtr(Address(calleereg, JSFunction::offsetOfNativeOrScript()), objreg);

    
    masm.loadBaselineOrIonRaw(objreg, objreg, executionMode, &invoke);

    
    {
        
        unsigned pushed = masm.framePushed();
        masm.addPtr(Imm32(pushed), copyreg);
        masm.makeFrameDescriptor(copyreg, JitFrame_IonJS);

        masm.Push(argcreg);
        masm.Push(calleereg);
        masm.Push(copyreg); 

        Label underflow, rejoin;

        
        if (!apply->hasSingleTarget()) {
            masm.load16ZeroExtend(Address(calleereg, JSFunction::offsetOfNargs()), copyreg);
            masm.branch32(Assembler::Below, argcreg, copyreg, &underflow);
        } else {
            masm.branch32(Assembler::Below, argcreg, Imm32(apply->getSingleTarget()->nargs()),
                          &underflow);
        }

        
        
        masm.jump(&rejoin);

        
        {
            masm.bind(&underflow);

            
            JitCode *argumentsRectifier = gen->jitRuntime()->getArgumentsRectifier(executionMode);

            JS_ASSERT(ArgumentsRectifierReg != objreg);
            masm.movePtr(ImmGCPtr(argumentsRectifier), objreg); 
            masm.loadPtr(Address(objreg, JitCode::offsetOfCode()), objreg);
            masm.movePtr(argcreg, ArgumentsRectifierReg);
        }

        masm.bind(&rejoin);

        
        uint32_t callOffset = masm.callIon(objreg);
        if (!markSafepointAt(callOffset, apply))
            return false;

        
        masm.loadPtr(Address(StackPointer, 0), copyreg);
        masm.rshiftPtr(Imm32(FRAMESIZE_SHIFT), copyreg);
        masm.subPtr(Imm32(pushed), copyreg);

        
        
        int prefixGarbage = sizeof(IonJSFrameLayout) - sizeof(void *);
        masm.adjustStack(prefixGarbage);
        masm.jump(&end);
    }

    
    {
        masm.bind(&invoke);
        if (!emitCallInvokeFunction(apply, copyreg))
            return false;
    }

    
    masm.bind(&end);
    emitPopArguments(apply, copyreg);

    return true;
}

typedef bool (*ArraySpliceDenseFn)(JSContext *, HandleObject, uint32_t, uint32_t);
static const VMFunction ArraySpliceDenseInfo = FunctionInfo<ArraySpliceDenseFn>(ArraySpliceDense);

bool
CodeGenerator::visitArraySplice(LArraySplice *lir)
{
    pushArg(ToRegister(lir->getDeleteCount()));
    pushArg(ToRegister(lir->getStart()));
    pushArg(ToRegister(lir->getObject()));
    return callVM(ArraySpliceDenseInfo, lir);
}

bool
CodeGenerator::visitBail(LBail *lir)
{
    return bailout(lir->snapshot());
}

bool
CodeGenerator::visitUnreachable(LUnreachable *lir)
{
    masm.assumeUnreachable("end-of-block assumed unreachable");
    return true;
}

bool
CodeGenerator::visitGetDynamicName(LGetDynamicName *lir)
{
    Register scopeChain = ToRegister(lir->getScopeChain());
    Register name = ToRegister(lir->getName());
    Register temp1 = ToRegister(lir->temp1());
    Register temp2 = ToRegister(lir->temp2());
    Register temp3 = ToRegister(lir->temp3());

    masm.loadJSContext(temp3);

    
    masm.adjustStack(-int32_t(sizeof(Value)));
    masm.movePtr(StackPointer, temp2);

    masm.setupUnalignedABICall(4, temp1);
    masm.passABIArg(temp3);
    masm.passABIArg(scopeChain);
    masm.passABIArg(name);
    masm.passABIArg(temp2);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, GetDynamicName));

    const ValueOperand out = ToOutValue(lir);

    masm.loadValue(Address(StackPointer, 0), out);
    masm.adjustStack(sizeof(Value));

    Label undefined;
    masm.branchTestUndefined(Assembler::Equal, out, &undefined);
    return bailoutFrom(&undefined, lir->snapshot());
}

bool
CodeGenerator::emitFilterArgumentsOrEval(LInstruction *lir, Register string,
                                         Register temp1, Register temp2)
{
    masm.loadJSContext(temp2);

    masm.setupUnalignedABICall(2, temp1);
    masm.passABIArg(temp2);
    masm.passABIArg(string);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, FilterArgumentsOrEval));

    Label bail;
    masm.branchIfFalseBool(ReturnReg, &bail);
    return bailoutFrom(&bail, lir->snapshot());
}

bool
CodeGenerator::visitFilterArgumentsOrEvalS(LFilterArgumentsOrEvalS *lir)
{
    return emitFilterArgumentsOrEval(lir, ToRegister(lir->getString()),
                                     ToRegister(lir->temp1()),
                                     ToRegister(lir->temp2()));
}

bool
CodeGenerator::visitFilterArgumentsOrEvalV(LFilterArgumentsOrEvalV *lir)
{
    ValueOperand input = ToValue(lir, LFilterArgumentsOrEvalV::Input);

    
    Label done;
    masm.branchTestString(Assembler::NotEqual, input, &done);

    if (!emitFilterArgumentsOrEval(lir, masm.extractString(input, ToRegister(lir->temp3())),
                                   ToRegister(lir->temp1()), ToRegister(lir->temp2())))
    {
        return false;
    }

    masm.bind(&done);
    return true;
}

typedef bool (*DirectEvalSFn)(JSContext *, HandleObject, HandleScript, HandleValue, HandleString,
                              jsbytecode *, MutableHandleValue);
static const VMFunction DirectEvalStringInfo = FunctionInfo<DirectEvalSFn>(DirectEvalStringFromIon);

bool
CodeGenerator::visitCallDirectEvalS(LCallDirectEvalS *lir)
{
    Register scopeChain = ToRegister(lir->getScopeChain());
    Register string = ToRegister(lir->getString());

    pushArg(ImmPtr(lir->mir()->pc()));
    pushArg(string);
    pushArg(ToValue(lir, LCallDirectEvalS::ThisValue));
    pushArg(ImmGCPtr(gen->info().script()));
    pushArg(scopeChain);

    return callVM(DirectEvalStringInfo, lir);
}

typedef bool (*DirectEvalVFn)(JSContext *, HandleObject, HandleScript, HandleValue, HandleValue,
                              jsbytecode *, MutableHandleValue);
static const VMFunction DirectEvalValueInfo = FunctionInfo<DirectEvalVFn>(DirectEvalValueFromIon);

bool
CodeGenerator::visitCallDirectEvalV(LCallDirectEvalV *lir)
{
    Register scopeChain = ToRegister(lir->getScopeChain());

    pushArg(ImmPtr(lir->mir()->pc()));
    pushArg(ToValue(lir, LCallDirectEvalV::Argument));
    pushArg(ToValue(lir, LCallDirectEvalV::ThisValue));
    pushArg(ImmGCPtr(gen->info().script()));
    pushArg(scopeChain);

    return callVM(DirectEvalValueInfo, lir);
}


static const uint32_t EntryTempMask = Registers::TempMask & ~(1 << OsrFrameReg.code());

bool
CodeGenerator::generateArgumentsChecks(bool bailout)
{
    
    
    
    

    MIRGraph &mir = gen->graph();
    MResumePoint *rp = mir.entryResumePoint();

    
    
    

    
    
    
    
    
    uint32_t frameSizeLeft = frameSize();
    while (frameSizeLeft > 4096) {
        masm.reserveStack(4096);
        masm.store32(Imm32(0), Address(StackPointer, 0));
        frameSizeLeft -= 4096;
    }
    masm.reserveStack(frameSizeLeft);

    
    Register temp = GeneralRegisterSet(EntryTempMask).getAny();

    CompileInfo &info = gen->info();

    Label miss;
    for (uint32_t i = info.startArgSlot(); i < info.endArgSlot(); i++) {
        
        MParameter *param = rp->getOperand(i)->toParameter();
        const types::TypeSet *types = param->resultTypeSet();
        if (!types || types->unknown())
            continue;

        
        
        
        
        int32_t offset = ArgToStackOffset((i - info.startArgSlot()) * sizeof(Value));
        masm.guardTypeSet(Address(StackPointer, offset), types, BarrierKind::TypeSet, temp, &miss);
    }

    if (miss.used()) {
        if (bailout) {
            if (!bailoutFrom(&miss, graph.entrySnapshot()))
                return false;
        } else {
            Label success;
            masm.jump(&success);
            masm.bind(&miss);
            masm.assumeUnreachable("Argument check fail.");
            masm.bind(&success);
        }
    }

    masm.freeStack(frameSize());

    return true;
}


class CheckOverRecursedFailure : public OutOfLineCodeBase<CodeGenerator>
{
    LInstruction *lir_;

  public:
    explicit CheckOverRecursedFailure(LInstruction *lir)
      : lir_(lir)
    { }

    bool accept(CodeGenerator *codegen) {
        return codegen->visitCheckOverRecursedFailure(this);
    }

    LInstruction *lir() const {
        return lir_;
    }
};

bool
CodeGenerator::visitCheckOverRecursed(LCheckOverRecursed *lir)
{
    
    if (omitOverRecursedCheck())
        return true;

    
    
    
    
    
    
    
    

    
    
    const void *limitAddr = GetIonContext()->runtime->addressOfJitStackLimit();

    CheckOverRecursedFailure *ool = new(alloc()) CheckOverRecursedFailure(lir);
    if (!addOutOfLineCode(ool, lir->mir()))
        return false;

    
    masm.branchPtr(Assembler::AboveOrEqual, AbsoluteAddress(limitAddr), StackPointer, ool->entry());
    masm.bind(ool->rejoin());

    return true;
}

typedef bool (*DefVarOrConstFn)(JSContext *, HandlePropertyName, unsigned, HandleObject);
static const VMFunction DefVarOrConstInfo =
    FunctionInfo<DefVarOrConstFn>(DefVarOrConst);

bool
CodeGenerator::visitDefVar(LDefVar *lir)
{
    Register scopeChain = ToRegister(lir->scopeChain());

    pushArg(scopeChain); 
    pushArg(Imm32(lir->mir()->attrs())); 
    pushArg(ImmGCPtr(lir->mir()->name())); 

    if (!callVM(DefVarOrConstInfo, lir))
        return false;

    return true;
}

typedef bool (*DefFunOperationFn)(JSContext *, HandleScript, HandleObject, HandleFunction);
static const VMFunction DefFunOperationInfo = FunctionInfo<DefFunOperationFn>(DefFunOperation);

bool
CodeGenerator::visitDefFun(LDefFun *lir)
{
    Register scopeChain = ToRegister(lir->scopeChain());

    pushArg(ImmGCPtr(lir->mir()->fun()));
    pushArg(scopeChain);
    pushArg(ImmGCPtr(current->mir()->info().script()));

    return callVM(DefFunOperationInfo, lir);
}

typedef bool (*CheckOverRecursedFn)(JSContext *);
typedef bool (*CheckOverRecursedParFn)(ForkJoinContext *);
static const VMFunctionsModal CheckOverRecursedInfo = VMFunctionsModal(
    FunctionInfo<CheckOverRecursedFn>(CheckOverRecursed),
    FunctionInfo<CheckOverRecursedParFn>(CheckOverRecursedPar));

bool
CodeGenerator::visitCheckOverRecursedFailure(CheckOverRecursedFailure *ool)
{
    
    

    
    
    
    saveLive(ool->lir());

    if (!callVM(CheckOverRecursedInfo, ool->lir()))
        return false;

    restoreLive(ool->lir());
    masm.jump(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitCheckOverRecursedPar(LCheckOverRecursedPar *lir)
{
    
    
    
    
    
    

    Register cxReg = ToRegister(lir->forkJoinContext());
    Register tempReg = ToRegister(lir->getTempReg());

    masm.loadPtr(Address(cxReg, offsetof(ForkJoinContext, perThreadData)), tempReg);
    masm.loadPtr(Address(tempReg, offsetof(PerThreadData, jitStackLimit)), tempReg);

    
    CheckOverRecursedFailure *ool = new(alloc()) CheckOverRecursedFailure(lir);
    if (!addOutOfLineCode(ool, lir->mir()))
        return false;

    masm.branchPtr(Assembler::BelowOrEqual, StackPointer, tempReg, ool->entry());
    masm.checkInterruptFlagPar(tempReg, ool->entry());
    masm.bind(ool->rejoin());

    return true;
}

bool
CodeGenerator::visitInterruptCheckPar(LInterruptCheckPar *lir)
{
    
    OutOfLineCode *ool = oolCallVM(InterruptCheckInfo, lir, (ArgList()), StoreNothing());
    if (!ool)
        return false;

    Register tempReg = ToRegister(lir->getTempReg());
    masm.checkInterruptFlagPar(tempReg, ool->entry());
    masm.bind(ool->rejoin());
    return true;
}

IonScriptCounts *
CodeGenerator::maybeCreateScriptCounts()
{
    
    
    
    if (!GetIonContext()->runtime->profilingScripts())
        return nullptr;

    IonScriptCounts *counts = nullptr;

    CompileInfo *outerInfo = &gen->info();
    JSScript *script = outerInfo->script();

    counts = js_new<IonScriptCounts>();
    if (!counts || !counts->init(graph.numBlocks())) {
        js_delete(counts);
        return nullptr;
    }

    for (size_t i = 0; i < graph.numBlocks(); i++) {
        MBasicBlock *block = graph.getBlock(i)->mir();

        uint32_t offset = 0;
        char *description = nullptr;
        if (script) {
            if (MResumePoint *resume = block->entryResumePoint()) {
                
                
                
                
                while (resume->caller())
                    resume = resume->caller();
                offset = script->pcToOffset(resume->pc());

                if (block->entryResumePoint()->caller()) {
                    
                    JSScript *innerScript = block->info().script();
                    description = (char *) js_calloc(200);
                    if (description) {
                        JS_snprintf(description, 200, "%s:%d",
                                    innerScript->filename(), innerScript->lineno());
                    }
                }
            }
        }

        if (!counts->block(i).init(block->id(), offset, description, block->numSuccessors())) {
            js_delete(counts);
            return nullptr;
        }
        for (size_t j = 0; j < block->numSuccessors(); j++)
            counts->block(i).setSuccessor(j, skipTrivialBlocks(block->getSuccessor(j))->id());
    }

    scriptCounts_ = counts;
    return counts;
}


struct ScriptCountBlockState
{
    IonBlockCounts &block;
    MacroAssembler &masm;

    Sprinter printer;

  public:
    ScriptCountBlockState(IonBlockCounts *block, MacroAssembler *masm)
      : block(*block), masm(*masm), printer(GetIonContext()->cx)
    {
    }

    bool init()
    {
        if (!printer.init())
            return false;

        
        
        
        masm.inc64(AbsoluteAddress(block.addressOfHitCount()));

        
        masm.setPrinter(&printer);

        return true;
    }

    void visitInstruction(LInstruction *ins)
    {
        
        
        if (const char *extra = ins->extraName())
            printer.printf("[%s:%s]\n", ins->opName(), extra);
        else
            printer.printf("[%s]\n", ins->opName());
    }

    ~ScriptCountBlockState()
    {
        masm.setPrinter(nullptr);

        block.setCode(printer.string());
    }
};

#ifdef DEBUG
bool
CodeGenerator::branchIfInvalidated(Register temp, Label *invalidated)
{
    CodeOffsetLabel label = masm.movWithPatch(ImmWord(uintptr_t(-1)), temp);
    if (!ionScriptLabels_.append(label))
        return false;

    
    masm.branch32(Assembler::NotEqual,
                  Address(temp, IonScript::offsetOfRefcount()),
                  Imm32(0),
                  invalidated);
    return true;
}

bool
CodeGenerator::emitObjectOrStringResultChecks(LInstruction *lir, MDefinition *mir)
{
    if (lir->numDefs() == 0)
        return true;

    JS_ASSERT(lir->numDefs() == 1);
    Register output = ToRegister(lir->getDef(0));

    GeneralRegisterSet regs(GeneralRegisterSet::All());
    regs.take(output);

    Register temp = regs.takeAny();
    masm.push(temp);

    
    
    Label done;
    if (!branchIfInvalidated(temp, &done))
        return false;

    if (mir->type() == MIRType_Object &&
        mir->resultTypeSet() &&
        !mir->resultTypeSet()->unknownObject())
    {
        
        Label miss, ok;
        if (mir->resultTypeSet()->getObjectCount() > 0)
            masm.guardObjectType(output, mir->resultTypeSet(), temp, &miss);
        else
            masm.jump(&miss);
        masm.jump(&ok);

        masm.bind(&miss);
        masm.assumeUnreachable("MIR instruction returned object with unexpected type");

        masm.bind(&ok);
    }

    
    if (gen->info().executionMode() != ParallelExecution) {
        saveVolatile();
        masm.setupUnalignedABICall(2, temp);
        masm.loadJSContext(temp);
        masm.passABIArg(temp);
        masm.passABIArg(output);
        masm.callWithABINoProfiling(mir->type() == MIRType_Object
                                    ? JS_FUNC_TO_DATA_PTR(void *, AssertValidObjectPtr)
                                    : mir->type() == MIRType_String
                                      ? JS_FUNC_TO_DATA_PTR(void *, AssertValidStringPtr)
                                      : JS_FUNC_TO_DATA_PTR(void *, AssertValidSymbolPtr));
        restoreVolatile();
    }

    masm.bind(&done);
    masm.pop(temp);
    return true;
}

bool
CodeGenerator::emitValueResultChecks(LInstruction *lir, MDefinition *mir)
{
    if (lir->numDefs() == 0)
        return true;

    JS_ASSERT(lir->numDefs() == BOX_PIECES);
    if (!lir->getDef(0)->output()->isRegister())
        return true;

    ValueOperand output = ToOutValue(lir);

    GeneralRegisterSet regs(GeneralRegisterSet::All());
    regs.take(output);

    Register temp1 = regs.takeAny();
    Register temp2 = regs.takeAny();
    masm.push(temp1);
    masm.push(temp2);

    
    
    Label done;
    if (!branchIfInvalidated(temp1, &done))
        return false;

    if (mir->resultTypeSet() && !mir->resultTypeSet()->unknown()) {
        
        Label miss, ok;
        masm.guardTypeSet(output, mir->resultTypeSet(), BarrierKind::TypeSet, temp1, &miss);
        masm.jump(&ok);

        masm.bind(&miss);
        masm.assumeUnreachable("MIR instruction returned value with unexpected type");

        masm.bind(&ok);
    }

    
    if (gen->info().executionMode() != ParallelExecution) {
        saveVolatile();

        masm.pushValue(output);
        masm.movePtr(StackPointer, temp1);

        masm.setupUnalignedABICall(2, temp2);
        masm.loadJSContext(temp2);
        masm.passABIArg(temp2);
        masm.passABIArg(temp1);
        masm.callWithABINoProfiling(JS_FUNC_TO_DATA_PTR(void *, AssertValidValue));
        masm.popValue(output);
        restoreVolatile();
    }

    masm.bind(&done);
    masm.pop(temp2);
    masm.pop(temp1);
    return true;
}

bool
CodeGenerator::emitDebugResultChecks(LInstruction *ins)
{
    

    MDefinition *mir = ins->mirRaw();
    if (!mir)
        return true;

    switch (mir->type()) {
      case MIRType_Object:
      case MIRType_String:
      case MIRType_Symbol:
        return emitObjectOrStringResultChecks(ins, mir);
      case MIRType_Value:
        return emitValueResultChecks(ins, mir);
      default:
        return true;
    }
}
#endif

bool
CodeGenerator::generateBody()
{
    IonScriptCounts *counts = maybeCreateScriptCounts();

#if defined(JS_ION_PERF)
    PerfSpewer *perfSpewer = &perfSpewer_;
    if (gen->compilingAsmJS())
        perfSpewer = &gen->perfSpewer();
#endif

    for (size_t i = 0; i < graph.numBlocks(); i++) {
        current = graph.getBlock(i);

        
        
        
        if (current->isTrivial())
            continue;

        JitSpew(JitSpew_Codegen, "# block%lu%s:", i,
                current->mir()->isLoopHeader() ? " (loop header)" : "");

        masm.bind(current->label());

        mozilla::Maybe<ScriptCountBlockState> blockCounts;
        if (counts) {
            blockCounts.emplace(&counts->block(i), &masm);
            if (!blockCounts->init())
                return false;
        }

#if defined(JS_ION_PERF)
        perfSpewer->startBasicBlock(current->mir(), masm);
#endif

        for (LInstructionIterator iter = current->begin(); iter != current->end(); iter++) {
#ifdef DEBUG
            JitSpewStart(JitSpew_Codegen, "instruction %s", iter->opName());
            if (const char *extra = iter->extraName())
                JitSpewCont(JitSpew_Codegen, ":%s", extra);
            JitSpewFin(JitSpew_Codegen);
#endif

            if (counts)
                blockCounts->visitInstruction(*iter);

            if (iter->safepoint() && pushedArgumentSlots_.length()) {
                if (!markArgumentSlots(iter->safepoint()))
                    return false;
            }

#ifdef CHECK_OSIPOINT_REGISTERS
            if (iter->safepoint())
                resetOsiPointRegs(iter->safepoint());
#endif

            if (iter->mirRaw()) {
                
                if (iter->mirRaw()->trackedSite().hasTree()) {
                    if (!addNativeToBytecodeEntry(iter->mirRaw()->trackedSite()))
                        return false;
                }
            }

            if (!iter->accept(this))
                return false;

#ifdef DEBUG
            if (!counts && !emitDebugResultChecks(*iter))
                return false;
#endif
        }
        if (masm.oom())
            return false;

#if defined(JS_ION_PERF)
        perfSpewer->endBasicBlock(masm);
#endif
    }

    JS_ASSERT(pushedArgumentSlots_.empty());
    return true;
}


class OutOfLineNewArray : public OutOfLineCodeBase<CodeGenerator>
{
    LNewArray *lir_;

  public:
    explicit OutOfLineNewArray(LNewArray *lir)
      : lir_(lir)
    { }

    bool accept(CodeGenerator *codegen) {
        return codegen->visitOutOfLineNewArray(this);
    }

    LNewArray *lir() const {
        return lir_;
    }
};

typedef JSObject *(*NewInitArrayFn)(JSContext *, uint32_t, types::TypeObject *);
static const VMFunction NewInitArrayInfo =
    FunctionInfo<NewInitArrayFn>(NewInitArray);

bool
CodeGenerator::visitNewArrayCallVM(LNewArray *lir)
{
    JS_ASSERT(gen->info().executionMode() == SequentialExecution);

    Register objReg = ToRegister(lir->output());

    JS_ASSERT(!lir->isCall());
    saveLive(lir);

    JSObject *templateObject = lir->mir()->templateObject();
    types::TypeObject *type =
        templateObject->hasSingletonType() ? nullptr : templateObject->type();

    pushArg(ImmGCPtr(type));
    pushArg(Imm32(lir->mir()->count()));

    if (!callVM(NewInitArrayInfo, lir))
        return false;

    if (ReturnReg != objReg)
        masm.movePtr(ReturnReg, objReg);

    restoreLive(lir);

    return true;
}

typedef JSObject *(*NewDerivedTypedObjectFn)(JSContext *,
                                             HandleObject type,
                                             HandleObject owner,
                                             int32_t offset);
static const VMFunction CreateDerivedTypedObjInfo =
    FunctionInfo<NewDerivedTypedObjectFn>(CreateDerivedTypedObj);

bool
CodeGenerator::visitNewDerivedTypedObject(LNewDerivedTypedObject *lir)
{
    
    JS_ASSERT(gen->info().executionMode() == SequentialExecution);

    pushArg(ToRegister(lir->offset()));
    pushArg(ToRegister(lir->owner()));
    pushArg(ToRegister(lir->type()));
    return callVM(CreateDerivedTypedObjInfo, lir);
}

bool CodeGenerator::visitAtan2D(LAtan2D *lir)
{
    Register temp = ToRegister(lir->temp());
    FloatRegister y = ToFloatRegister(lir->y());
    FloatRegister x = ToFloatRegister(lir->x());

    masm.setupUnalignedABICall(2, temp);
    masm.passABIArg(y, MoveOp::DOUBLE);
    masm.passABIArg(x, MoveOp::DOUBLE);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ecmaAtan2), MoveOp::DOUBLE);

    JS_ASSERT(ToFloatRegister(lir->output()) == ReturnDoubleReg);
    return true;
}

bool CodeGenerator::visitHypot(LHypot *lir)
{
    Register temp = ToRegister(lir->temp());
    FloatRegister x = ToFloatRegister(lir->x());
    FloatRegister y = ToFloatRegister(lir->y());

    masm.setupUnalignedABICall(2, temp);
    masm.passABIArg(x, MoveOp::DOUBLE);
    masm.passABIArg(y, MoveOp::DOUBLE);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ecmaHypot), MoveOp::DOUBLE);

    JS_ASSERT(ToFloatRegister(lir->output()) == ReturnDoubleReg);
    return true;
}

bool
CodeGenerator::visitNewArray(LNewArray *lir)
{
    JS_ASSERT(gen->info().executionMode() == SequentialExecution);
    Register objReg = ToRegister(lir->output());
    Register tempReg = ToRegister(lir->temp());
    JSObject *templateObject = lir->mir()->templateObject();
    DebugOnly<uint32_t> count = lir->mir()->count();

    JS_ASSERT(count < JSObject::NELEMENTS_LIMIT);

    if (lir->mir()->shouldUseVM())
        return visitNewArrayCallVM(lir);

    OutOfLineNewArray *ool = new(alloc()) OutOfLineNewArray(lir);
    if (!addOutOfLineCode(ool, lir->mir()))
        return false;

    masm.createGCObject(objReg, tempReg, templateObject, lir->mir()->initialHeap(), ool->entry());

    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitOutOfLineNewArray(OutOfLineNewArray *ool)
{
    if (!visitNewArrayCallVM(ool->lir()))
        return false;
    masm.jump(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitNewArrayCopyOnWrite(LNewArrayCopyOnWrite *lir)
{
    Register objReg = ToRegister(lir->output());
    Register tempReg = ToRegister(lir->temp());
    JSObject *templateObject = lir->mir()->templateObject();
    gc::InitialHeap initialHeap = lir->mir()->initialHeap();

    
    OutOfLineCode *ool = oolCallVM(NewArrayCopyOnWriteInfo, lir,
                                   (ArgList(), ImmGCPtr(templateObject), Imm32(initialHeap)),
                                   StoreRegisterTo(objReg));
    if (!ool)
        return false;

    masm.createGCObject(objReg, tempReg, templateObject, initialHeap, ool->entry());

    masm.bind(ool->rejoin());
    return true;
}


class OutOfLineNewObject : public OutOfLineCodeBase<CodeGenerator>
{
    LNewObject *lir_;

  public:
    explicit OutOfLineNewObject(LNewObject *lir)
      : lir_(lir)
    { }

    bool accept(CodeGenerator *codegen) {
        return codegen->visitOutOfLineNewObject(this);
    }

    LNewObject *lir() const {
        return lir_;
    }
};

typedef JSObject *(*NewInitObjectFn)(JSContext *, HandleObject);
static const VMFunction NewInitObjectInfo = FunctionInfo<NewInitObjectFn>(NewInitObject);

typedef JSObject *(*NewInitObjectWithClassPrototypeFn)(JSContext *, HandleObject);
static const VMFunction NewInitObjectWithClassPrototypeInfo =
    FunctionInfo<NewInitObjectWithClassPrototypeFn>(NewInitObjectWithClassPrototype);

bool
CodeGenerator::visitNewObjectVMCall(LNewObject *lir)
{
    JS_ASSERT(gen->info().executionMode() == SequentialExecution);

    Register objReg = ToRegister(lir->output());

    JS_ASSERT(!lir->isCall());
    saveLive(lir);

    pushArg(ImmGCPtr(lir->mir()->templateObject()));

    
    
    
    
    if (lir->mir()->templateObjectIsClassPrototype()) {
        if (!callVM(NewInitObjectWithClassPrototypeInfo, lir))
            return false;
    } else if (!callVM(NewInitObjectInfo, lir)) {
        return false;
    }

    if (ReturnReg != objReg)
        masm.movePtr(ReturnReg, objReg);

    restoreLive(lir);
    return true;
}

static bool
ShouldInitFixedSlots(LInstruction *lir, JSObject *templateObj)
{
    
    
    
    

    uint32_t nfixed = templateObj->numUsedFixedSlots();
    if (nfixed == 0)
        return false;

    
    
    
    for (uint32_t slot = 0; slot < nfixed; slot++) {
        if (!templateObj->getSlot(slot).isUndefined())
            return true;
    }

    
    
    MOZ_ASSERT(nfixed <= JSObject::MAX_FIXED_SLOTS);
    static_assert(JSObject::MAX_FIXED_SLOTS <= 32, "Slot bits must fit in 32 bits");
    uint32_t initializedSlots = 0;
    uint32_t numInitialized = 0;

    MInstruction *allocMir = lir->mirRaw()->toInstruction();
    MBasicBlock *block = allocMir->block();

    
    MInstructionIterator iter = block->begin(allocMir);
    MOZ_ASSERT(*iter == allocMir);
    iter++;

    while (true) {
        for (; iter != block->end(); iter++) {
            if (iter->isNop() || iter->isConstant() || iter->isPostWriteBarrier()) {
                
                continue;
            }

            if (iter->isStoreFixedSlot()) {
                MStoreFixedSlot *store = iter->toStoreFixedSlot();
                if (store->object() != allocMir)
                    return true;

                
                
                
                
                store->setNeedsBarrier(false);

                uint32_t slot = store->slot();
                MOZ_ASSERT(slot < nfixed);
                if ((initializedSlots & (1 << slot)) == 0) {
                    numInitialized++;
                    initializedSlots |= (1 << slot);

                    if (numInitialized == nfixed) {
                        
                        MOZ_ASSERT(mozilla::CountPopulation32(initializedSlots) == nfixed);
                        return false;
                    }
                }
                continue;
            }

            if (iter->isGoto()) {
                block = iter->toGoto()->target();
                if (block->numPredecessors() != 1)
                    return true;
                break;
            }

            
            return true;
        }
        iter = block->begin();
    }

    MOZ_CRASH("Shouldn't get here");
}

bool
CodeGenerator::visitNewObject(LNewObject *lir)
{
    JS_ASSERT(gen->info().executionMode() == SequentialExecution);
    Register objReg = ToRegister(lir->output());
    Register tempReg = ToRegister(lir->temp());
    JSObject *templateObject = lir->mir()->templateObject();

    if (lir->mir()->shouldUseVM())
        return visitNewObjectVMCall(lir);

    OutOfLineNewObject *ool = new(alloc()) OutOfLineNewObject(lir);
    if (!addOutOfLineCode(ool, lir->mir()))
        return false;

    bool initFixedSlots = ShouldInitFixedSlots(lir, templateObject);
    masm.createGCObject(objReg, tempReg, templateObject, lir->mir()->initialHeap(), ool->entry(),
                        initFixedSlots);

    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitOutOfLineNewObject(OutOfLineNewObject *ool)
{
    if (!visitNewObjectVMCall(ool->lir()))
        return false;
    masm.jump(ool->rejoin());
    return true;
}

typedef js::DeclEnvObject *(*NewDeclEnvObjectFn)(JSContext *, HandleFunction, gc::InitialHeap);
static const VMFunction NewDeclEnvObjectInfo =
    FunctionInfo<NewDeclEnvObjectFn>(DeclEnvObject::createTemplateObject);

bool
CodeGenerator::visitNewDeclEnvObject(LNewDeclEnvObject *lir)
{
    Register objReg = ToRegister(lir->output());
    Register tempReg = ToRegister(lir->temp());
    JSObject *templateObj = lir->mir()->templateObj();
    CompileInfo &info = lir->mir()->block()->info();

    
    OutOfLineCode *ool = oolCallVM(NewDeclEnvObjectInfo, lir,
                                   (ArgList(), ImmGCPtr(info.funMaybeLazy()),
                                    Imm32(gc::DefaultHeap)),
                                   StoreRegisterTo(objReg));
    if (!ool)
        return false;

    bool initFixedSlots = ShouldInitFixedSlots(lir, templateObj);
    masm.createGCObject(objReg, tempReg, templateObj, gc::DefaultHeap, ool->entry(),
                        initFixedSlots);

    masm.bind(ool->rejoin());
    return true;
}

typedef JSObject *(*NewCallObjectFn)(JSContext *, HandleShape, HandleTypeObject);
static const VMFunction NewCallObjectInfo =
    FunctionInfo<NewCallObjectFn>(NewCallObject);

bool
CodeGenerator::visitNewCallObject(LNewCallObject *lir)
{
    Register objReg = ToRegister(lir->output());
    Register tempReg = ToRegister(lir->temp());

    JSObject *templateObj = lir->mir()->templateObject();

    OutOfLineCode *ool = oolCallVM(NewCallObjectInfo, lir,
                                   (ArgList(), ImmGCPtr(templateObj->lastProperty()),
                                               ImmGCPtr(templateObj->type())),
                                   StoreRegisterTo(objReg));
    if (!ool)
        return false;

    
    bool initFixedSlots = ShouldInitFixedSlots(lir, templateObj);
    masm.createGCObject(objReg, tempReg, templateObj, gc::DefaultHeap, ool->entry(),
                        initFixedSlots);

    masm.bind(ool->rejoin());
    return true;
}

typedef JSObject *(*NewSingletonCallObjectFn)(JSContext *, HandleShape);
static const VMFunction NewSingletonCallObjectInfo =
    FunctionInfo<NewSingletonCallObjectFn>(NewSingletonCallObject);

bool
CodeGenerator::visitNewSingletonCallObject(LNewSingletonCallObject *lir)
{
    Register objReg = ToRegister(lir->output());

    JSObject *templateObj = lir->mir()->templateObject();

    OutOfLineCode *ool;
    ool = oolCallVM(NewSingletonCallObjectInfo, lir,
                    (ArgList(), ImmGCPtr(templateObj->lastProperty())),
                    StoreRegisterTo(objReg));
    if (!ool)
        return false;

    
    
    
    masm.jump(ool->entry());
    masm.bind(ool->rejoin());

    return true;
}

bool
CodeGenerator::visitNewCallObjectPar(LNewCallObjectPar *lir)
{
    Register resultReg = ToRegister(lir->output());
    Register cxReg = ToRegister(lir->forkJoinContext());
    Register tempReg1 = ToRegister(lir->getTemp0());
    Register tempReg2 = ToRegister(lir->getTemp1());
    JSObject *templateObj = lir->mir()->templateObj();

    return emitAllocateGCThingPar(lir, resultReg, cxReg, tempReg1, tempReg2, templateObj);
}

typedef JSObject *(*ExtendArrayParFn)(ForkJoinContext*, JSObject*, uint32_t);
static const VMFunction ExtendArrayParInfo =
    FunctionInfo<ExtendArrayParFn>(ExtendArrayPar);

bool
CodeGenerator::visitNewDenseArrayPar(LNewDenseArrayPar *lir)
{
    Register cxReg = ToRegister(lir->forkJoinContext());
    Register lengthReg = ToRegister(lir->length());
    Register tempReg0 = ToRegister(lir->getTemp0());
    Register tempReg1 = ToRegister(lir->getTemp1());
    Register tempReg2 = ToRegister(lir->getTemp2());
    JSObject *templateObj = lir->mir()->templateObject();

    if (!emitAllocateGCThingPar(lir, tempReg2, cxReg, tempReg0, tempReg1, templateObj))
        return false;

    
    

    saveLive(lir);
    pushArg(lengthReg);
    pushArg(tempReg2);
    if (!callVM(ExtendArrayParInfo, lir))
        return false;
    storeResultTo(ToRegister(lir->output()));
    restoreLive(lir);

    return true;
}

typedef JSObject *(*NewStringObjectFn)(JSContext *, HandleString);
static const VMFunction NewStringObjectInfo = FunctionInfo<NewStringObjectFn>(NewStringObject);

bool
CodeGenerator::visitNewStringObject(LNewStringObject *lir)
{
    Register input = ToRegister(lir->input());
    Register output = ToRegister(lir->output());
    Register temp = ToRegister(lir->temp());

    StringObject *templateObj = lir->mir()->templateObj();

    OutOfLineCode *ool = oolCallVM(NewStringObjectInfo, lir, (ArgList(), input),
                                   StoreRegisterTo(output));
    if (!ool)
        return false;

    masm.createGCObject(output, temp, templateObj, gc::DefaultHeap, ool->entry());

    masm.loadStringLength(input, temp);

    masm.storeValue(JSVAL_TYPE_STRING, input, Address(output, StringObject::offsetOfPrimitiveValue()));
    masm.storeValue(JSVAL_TYPE_INT32, temp, Address(output, StringObject::offsetOfLength()));

    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitNewPar(LNewPar *lir)
{
    Register objReg = ToRegister(lir->output());
    Register cxReg = ToRegister(lir->forkJoinContext());
    Register tempReg1 = ToRegister(lir->getTemp0());
    Register tempReg2 = ToRegister(lir->getTemp1());
    JSObject *templateObject = lir->mir()->templateObject();
    return emitAllocateGCThingPar(lir, objReg, cxReg, tempReg1, tempReg2, templateObject);
}

#ifndef JSGC_FJGENERATIONAL
class OutOfLineNewGCThingPar : public OutOfLineCodeBase<CodeGenerator>
{
public:
    LInstruction *lir;
    gc::AllocKind allocKind;
    Register objReg;
    Register cxReg;

    OutOfLineNewGCThingPar(LInstruction *lir, gc::AllocKind allocKind, Register objReg,
                           Register cxReg)
      : lir(lir), allocKind(allocKind), objReg(objReg), cxReg(cxReg)
    {}

    bool accept(CodeGenerator *codegen) {
        return codegen->visitOutOfLineNewGCThingPar(this);
    }
};
#endif 

typedef JSObject *(*NewGCThingParFn)(ForkJoinContext *, js::gc::AllocKind allocKind);
static const VMFunction NewGCThingParInfo =
    FunctionInfo<NewGCThingParFn>(NewGCThingPar);

bool
CodeGenerator::emitAllocateGCThingPar(LInstruction *lir, Register objReg, Register cxReg,
                                      Register tempReg1, Register tempReg2, JSObject *templateObj)
{
    JS_ASSERT(lir->mirRaw());
    JS_ASSERT(lir->mirRaw()->isInstruction());

    gc::AllocKind allocKind = templateObj->tenuredGetAllocKind();
#ifdef JSGC_FJGENERATIONAL
    OutOfLineCode *ool = oolCallVM(NewGCThingParInfo, lir,
                                   (ArgList(), Imm32(allocKind)), StoreRegisterTo(objReg));
    if (!ool)
        return false;
#else
    OutOfLineNewGCThingPar *ool = new(alloc()) OutOfLineNewGCThingPar(lir, allocKind, objReg, cxReg);
    if (!ool || !addOutOfLineCode(ool, lir->mirRaw()->toInstruction()))
        return false;
#endif

    masm.newGCThingPar(objReg, cxReg, tempReg1, tempReg2, templateObj, ool->entry());
    masm.bind(ool->rejoin());
    masm.initGCThing(objReg, tempReg1, templateObj);
    return true;
}

#ifndef JSGC_FJGENERATIONAL
bool
CodeGenerator::visitOutOfLineNewGCThingPar(OutOfLineNewGCThingPar *ool)
{
    
    
    
    
    Register out = ool->objReg;

    saveVolatile(out);
    masm.setupUnalignedABICall(2, out);
    masm.passABIArg(ool->cxReg);
    masm.move32(Imm32(ool->allocKind), out);
    masm.passABIArg(out);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, NewGCThingPar));
    masm.storeCallResult(out);
    restoreVolatile(out);

    return bailoutTestPtr(Assembler::Zero, out, out, ool->lir->snapshot());
}
#endif 

typedef bool(*InitElemFn)(JSContext *cx, HandleObject obj,
                          HandleValue id, HandleValue value);
static const VMFunction InitElemInfo =
    FunctionInfo<InitElemFn>(InitElemOperation);

bool
CodeGenerator::visitInitElem(LInitElem *lir)
{
    Register objReg = ToRegister(lir->getObject());

    pushArg(ToValue(lir, LInitElem::ValueIndex));
    pushArg(ToValue(lir, LInitElem::IdIndex));
    pushArg(objReg);

    return callVM(InitElemInfo, lir);
}

typedef bool (*InitElemGetterSetterFn)(JSContext *, jsbytecode *, HandleObject, HandleValue,
                                       HandleObject);
static const VMFunction InitElemGetterSetterInfo =
    FunctionInfo<InitElemGetterSetterFn>(InitGetterSetterOperation);

bool
CodeGenerator::visitInitElemGetterSetter(LInitElemGetterSetter *lir)
{
    Register obj = ToRegister(lir->object());
    Register value = ToRegister(lir->value());

    pushArg(value);
    pushArg(ToValue(lir, LInitElemGetterSetter::IdIndex));
    pushArg(obj);
    pushArg(ImmPtr(lir->mir()->resumePoint()->pc()));

    return callVM(InitElemGetterSetterInfo, lir);
}

typedef bool(*MutatePrototypeFn)(JSContext *cx, HandleObject obj, HandleValue value);
static const VMFunction MutatePrototypeInfo =
    FunctionInfo<MutatePrototypeFn>(MutatePrototype);

bool
CodeGenerator::visitMutateProto(LMutateProto *lir)
{
    Register objReg = ToRegister(lir->getObject());

    pushArg(ToValue(lir, LMutateProto::ValueIndex));
    pushArg(objReg);

    return callVM(MutatePrototypeInfo, lir);
}

typedef bool(*InitPropFn)(JSContext *cx, HandleObject obj,
                          HandlePropertyName name, HandleValue value);
static const VMFunction InitPropInfo =
    FunctionInfo<InitPropFn>(InitProp);

bool
CodeGenerator::visitInitProp(LInitProp *lir)
{
    Register objReg = ToRegister(lir->getObject());

    pushArg(ToValue(lir, LInitProp::ValueIndex));
    pushArg(ImmGCPtr(lir->mir()->propertyName()));
    pushArg(objReg);

    return callVM(InitPropInfo, lir);
}

typedef bool(*InitPropGetterSetterFn)(JSContext *, jsbytecode *, HandleObject, HandlePropertyName,
                                      HandleObject);
static const VMFunction InitPropGetterSetterInfo =
    FunctionInfo<InitPropGetterSetterFn>(InitGetterSetterOperation);

bool
CodeGenerator::visitInitPropGetterSetter(LInitPropGetterSetter *lir)
{
    Register obj = ToRegister(lir->object());
    Register value = ToRegister(lir->value());

    pushArg(value);
    pushArg(ImmGCPtr(lir->mir()->name()));
    pushArg(obj);
    pushArg(ImmPtr(lir->mir()->resumePoint()->pc()));

    return callVM(InitPropGetterSetterInfo, lir);
}

typedef bool (*CreateThisFn)(JSContext *cx, HandleObject callee, MutableHandleValue rval);
static const VMFunction CreateThisInfoCodeGen = FunctionInfo<CreateThisFn>(CreateThis);

bool
CodeGenerator::visitCreateThis(LCreateThis *lir)
{
    const LAllocation *callee = lir->getCallee();

    if (callee->isConstant())
        pushArg(ImmGCPtr(&callee->toConstant()->toObject()));
    else
        pushArg(ToRegister(callee));

    return callVM(CreateThisInfoCodeGen, lir);
}

static JSObject *
CreateThisForFunctionWithProtoWrapper(JSContext *cx, js::HandleObject callee, JSObject *proto)
{
    return CreateThisForFunctionWithProto(cx, callee, proto);
}

typedef JSObject *(*CreateThisWithProtoFn)(JSContext *cx, HandleObject callee, JSObject *proto);
static const VMFunction CreateThisWithProtoInfo =
FunctionInfo<CreateThisWithProtoFn>(CreateThisForFunctionWithProtoWrapper);

bool
CodeGenerator::visitCreateThisWithProto(LCreateThisWithProto *lir)
{
    const LAllocation *callee = lir->getCallee();
    const LAllocation *proto = lir->getPrototype();

    if (proto->isConstant())
        pushArg(ImmGCPtr(&proto->toConstant()->toObject()));
    else
        pushArg(ToRegister(proto));

    if (callee->isConstant())
        pushArg(ImmGCPtr(&callee->toConstant()->toObject()));
    else
        pushArg(ToRegister(callee));

    return callVM(CreateThisWithProtoInfo, lir);
}

typedef JSObject *(*NewGCObjectFn)(JSContext *cx, gc::AllocKind allocKind,
                                   gc::InitialHeap initialHeap);
static const VMFunction NewGCObjectInfo =
    FunctionInfo<NewGCObjectFn>(js::jit::NewGCObject);

bool
CodeGenerator::visitCreateThisWithTemplate(LCreateThisWithTemplate *lir)
{
    JSObject *templateObject = lir->mir()->templateObject();
    gc::AllocKind allocKind = templateObject->tenuredGetAllocKind();
    gc::InitialHeap initialHeap = lir->mir()->initialHeap();
    Register objReg = ToRegister(lir->output());
    Register tempReg = ToRegister(lir->temp());

    OutOfLineCode *ool = oolCallVM(NewGCObjectInfo, lir,
                                   (ArgList(), Imm32(allocKind), Imm32(initialHeap)),
                                   StoreRegisterTo(objReg));
    if (!ool)
        return false;

    
    masm.newGCThing(objReg, tempReg, templateObject, lir->mir()->initialHeap(), ool->entry());

    
    masm.bind(ool->rejoin());

    bool initFixedSlots = ShouldInitFixedSlots(lir, templateObject);
    masm.initGCThing(objReg, tempReg, templateObject, initFixedSlots);

    return true;
}

typedef JSObject *(*NewIonArgumentsObjectFn)(JSContext *cx, IonJSFrameLayout *frame, HandleObject);
static const VMFunction NewIonArgumentsObjectInfo =
    FunctionInfo<NewIonArgumentsObjectFn>((NewIonArgumentsObjectFn) ArgumentsObject::createForIon);

bool
CodeGenerator::visitCreateArgumentsObject(LCreateArgumentsObject *lir)
{
    
    JS_ASSERT(lir->mir()->block()->id() == 0);

    const LAllocation *callObj = lir->getCallObject();
    Register temp = ToRegister(lir->getTemp(0));

    masm.movePtr(StackPointer, temp);
    masm.addPtr(Imm32(frameSize()), temp);

    pushArg(ToRegister(callObj));
    pushArg(temp);
    return callVM(NewIonArgumentsObjectInfo, lir);
}

bool
CodeGenerator::visitGetArgumentsObjectArg(LGetArgumentsObjectArg *lir)
{
    Register temp = ToRegister(lir->getTemp(0));
    Register argsObj = ToRegister(lir->getArgsObject());
    ValueOperand out = ToOutValue(lir);

    masm.loadPrivate(Address(argsObj, ArgumentsObject::getDataSlotOffset()), temp);
    Address argAddr(temp, ArgumentsData::offsetOfArgs() + lir->mir()->argno() * sizeof(Value));
    masm.loadValue(argAddr, out);
#ifdef DEBUG
    Label success;
    masm.branchTestMagic(Assembler::NotEqual, out, &success);
    masm.assumeUnreachable("Result from ArgumentObject shouldn't be JSVAL_TYPE_MAGIC.");
    masm.bind(&success);
#endif
    return true;
}

bool
CodeGenerator::visitSetArgumentsObjectArg(LSetArgumentsObjectArg *lir)
{
    Register temp = ToRegister(lir->getTemp(0));
    Register argsObj = ToRegister(lir->getArgsObject());
    ValueOperand value = ToValue(lir, LSetArgumentsObjectArg::ValueIndex);

    masm.loadPrivate(Address(argsObj, ArgumentsObject::getDataSlotOffset()), temp);
    Address argAddr(temp, ArgumentsData::offsetOfArgs() + lir->mir()->argno() * sizeof(Value));
    emitPreBarrier(argAddr);
#ifdef DEBUG
    Label success;
    masm.branchTestMagic(Assembler::NotEqual, argAddr, &success);
    masm.assumeUnreachable("Result in ArgumentObject shouldn't be JSVAL_TYPE_MAGIC.");
    masm.bind(&success);
#endif
    masm.storeValue(value, argAddr);
    return true;
}

bool
CodeGenerator::visitReturnFromCtor(LReturnFromCtor *lir)
{
    ValueOperand value = ToValue(lir, LReturnFromCtor::ValueIndex);
    Register obj = ToRegister(lir->getObject());
    Register output = ToRegister(lir->output());

    Label valueIsObject, end;

    masm.branchTestObject(Assembler::Equal, value, &valueIsObject);

    
    masm.movePtr(obj, output);
    masm.jump(&end);

    
    masm.bind(&valueIsObject);
    Register payload = masm.extractObject(value, output);
    if (payload != output)
        masm.movePtr(payload, output);

    masm.bind(&end);
    return true;
}

typedef JSObject *(*BoxNonStrictThisFn)(JSContext *, HandleValue);
static const VMFunction BoxNonStrictThisInfo = FunctionInfo<BoxNonStrictThisFn>(BoxNonStrictThis);

bool
CodeGenerator::visitComputeThis(LComputeThis *lir)
{
    ValueOperand value = ToValue(lir, LComputeThis::ValueIndex);
    Register output = ToRegister(lir->output());

    OutOfLineCode *ool = oolCallVM(BoxNonStrictThisInfo, lir, (ArgList(), value),
                                   StoreRegisterTo(output));
    if (!ool)
        return false;

    masm.branchTestObject(Assembler::NotEqual, value, ool->entry());
    masm.unboxObject(value, output);

    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitLoadArrowThis(LLoadArrowThis *lir)
{
    Register callee = ToRegister(lir->callee());
    ValueOperand output = ToOutValue(lir);
    masm.loadValue(Address(callee, FunctionExtended::offsetOfArrowThisSlot()), output);
    return true;
}

bool
CodeGenerator::visitArrayLength(LArrayLength *lir)
{
    Address length(ToRegister(lir->elements()), ObjectElements::offsetOfLength());
    masm.load32(length, ToRegister(lir->output()));
    return true;
}

bool
CodeGenerator::visitSetArrayLength(LSetArrayLength *lir)
{
    Address length(ToRegister(lir->elements()), ObjectElements::offsetOfLength());
    Int32Key newLength = ToInt32Key(lir->index());

    masm.bumpKey(&newLength, 1);
    masm.storeKey(newLength, length);
    
    masm.bumpKey(&newLength, -1);
    return true;
}

bool
CodeGenerator::visitTypedArrayLength(LTypedArrayLength *lir)
{
    Register obj = ToRegister(lir->object());
    Register out = ToRegister(lir->output());
    masm.unboxInt32(Address(obj, TypedArrayObject::lengthOffset()), out);
    return true;
}

bool
CodeGenerator::visitTypedArrayElements(LTypedArrayElements *lir)
{
    Register obj = ToRegister(lir->object());
    Register out = ToRegister(lir->output());
    masm.loadPtr(Address(obj, TypedArrayObject::dataOffset()), out);
    return true;
}

bool
CodeGenerator::visitNeuterCheck(LNeuterCheck *lir)
{
    Register obj = ToRegister(lir->object());
    Register temp = ToRegister(lir->temp());

    masm.extractObject(Address(obj, TypedObject::offsetOfOwnerSlot()), temp);
    masm.unboxInt32(Address(temp, ArrayBufferObject::flagsOffset()), temp);

    Imm32 flag(ArrayBufferObject::neuteredFlag());
    if (!bailoutTest32(Assembler::NonZero, temp, flag, lir->snapshot()))
        return false;

    return true;
}

bool
CodeGenerator::visitTypedObjectProto(LTypedObjectProto *lir)
{
    Register obj = ToRegister(lir->object());
    JS_ASSERT(ToRegister(lir->output()) == ReturnReg);

    
    
    

    const Register tempReg = ToRegister(lir->temp());
    masm.setupUnalignedABICall(1, tempReg);
    masm.passABIArg(obj);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, TypedObjectProto));
    return true;
}

bool
CodeGenerator::visitTypedObjectElements(LTypedObjectElements *lir)
{
    Register obj = ToRegister(lir->object());
    Register out = ToRegister(lir->output());
    masm.loadPtr(Address(obj, TypedObject::offsetOfDataSlot()), out);
    return true;
}

bool
CodeGenerator::visitSetTypedObjectOffset(LSetTypedObjectOffset *lir)
{
    Register object = ToRegister(lir->object());
    Register offset = ToRegister(lir->offset());
    Register temp0 = ToRegister(lir->temp0());

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    masm.unboxInt32(Address(object, TypedObject::offsetOfByteOffsetSlot()), temp0);

    
    masm.subPtr(offset, temp0);

    
    masm.subPtr(temp0, Address(object, TypedObject::offsetOfDataSlot()));

    
    masm.storeValue(JSVAL_TYPE_INT32, offset,
                    Address(object, TypedObject::offsetOfByteOffsetSlot()));

    return true;
}

bool
CodeGenerator::visitStringLength(LStringLength *lir)
{
    Register input = ToRegister(lir->string());
    Register output = ToRegister(lir->output());

    masm.loadStringLength(input, output);
    return true;
}

bool
CodeGenerator::visitMinMaxI(LMinMaxI *ins)
{
    Register first = ToRegister(ins->first());
    Register output = ToRegister(ins->output());

    JS_ASSERT(first == output);

    Label done;
    Assembler::Condition cond = ins->mir()->isMax()
                                ? Assembler::GreaterThan
                                : Assembler::LessThan;

    if (ins->second()->isConstant()) {
        masm.branch32(cond, first, Imm32(ToInt32(ins->second())), &done);
        masm.move32(Imm32(ToInt32(ins->second())), output);
    } else {
        masm.branch32(cond, first, ToRegister(ins->second()), &done);
        masm.move32(ToRegister(ins->second()), output);
    }

    masm.bind(&done);
    return true;
}

bool
CodeGenerator::visitAbsI(LAbsI *ins)
{
    Register input = ToRegister(ins->input());
    Label positive;

    JS_ASSERT(input == ToRegister(ins->output()));
    masm.branchTest32(Assembler::NotSigned, input, input, &positive);
    masm.neg32(input);
#ifdef JS_CODEGEN_MIPS
    LSnapshot *snapshot = ins->snapshot();
    if (snapshot && !bailoutCmp32(Assembler::Equal, input, Imm32(INT32_MIN), snapshot))
        return false;
#else
    if (ins->snapshot() && !bailoutIf(Assembler::Overflow, ins->snapshot()))
        return false;
#endif
    masm.bind(&positive);

    return true;
}

bool
CodeGenerator::visitPowI(LPowI *ins)
{
    FloatRegister value = ToFloatRegister(ins->value());
    Register power = ToRegister(ins->power());
    Register temp = ToRegister(ins->temp());

    JS_ASSERT(power != temp);

    
    
    
    masm.setupUnalignedABICall(2, temp);
    masm.passABIArg(value, MoveOp::DOUBLE);
    masm.passABIArg(power);

    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, js::powi), MoveOp::DOUBLE);
    JS_ASSERT(ToFloatRegister(ins->output()) == ReturnDoubleReg);

    return true;
}

bool
CodeGenerator::visitPowD(LPowD *ins)
{
    FloatRegister value = ToFloatRegister(ins->value());
    FloatRegister power = ToFloatRegister(ins->power());
    Register temp = ToRegister(ins->temp());

    masm.setupUnalignedABICall(2, temp);
    masm.passABIArg(value, MoveOp::DOUBLE);
    masm.passABIArg(power, MoveOp::DOUBLE);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ecmaPow), MoveOp::DOUBLE);

    JS_ASSERT(ToFloatRegister(ins->output()) == ReturnDoubleReg);
    return true;
}

bool
CodeGenerator::visitRandom(LRandom *ins)
{
    Register temp = ToRegister(ins->temp());
    Register temp2 = ToRegister(ins->temp2());

    masm.loadJSContext(temp);

    masm.setupUnalignedABICall(1, temp2);
    masm.passABIArg(temp);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, math_random_no_outparam), MoveOp::DOUBLE);

    JS_ASSERT(ToFloatRegister(ins->output()) == ReturnDoubleReg);
    return true;
}

bool
CodeGenerator::visitMathFunctionD(LMathFunctionD *ins)
{
    Register temp = ToRegister(ins->temp());
    FloatRegister input = ToFloatRegister(ins->input());
    JS_ASSERT(ToFloatRegister(ins->output()) == ReturnDoubleReg);

    const MathCache *mathCache = ins->mir()->cache();

    masm.setupUnalignedABICall(mathCache ? 2 : 1, temp);
    if (mathCache) {
        masm.movePtr(ImmPtr(mathCache), temp);
        masm.passABIArg(temp);
    }
    masm.passABIArg(input, MoveOp::DOUBLE);

#   define MAYBE_CACHED(fcn) (mathCache ? (void*)fcn ## _impl : (void*)fcn ## _uncached)

    void *funptr = nullptr;
    switch (ins->mir()->function()) {
      case MMathFunction::Log:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_log));
        break;
      case MMathFunction::Sin:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_sin));
        break;
      case MMathFunction::Cos:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_cos));
        break;
      case MMathFunction::Exp:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_exp));
        break;
      case MMathFunction::Tan:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_tan));
        break;
      case MMathFunction::ATan:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_atan));
        break;
      case MMathFunction::ASin:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_asin));
        break;
      case MMathFunction::ACos:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_acos));
        break;
      case MMathFunction::Log10:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_log10));
        break;
      case MMathFunction::Log2:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_log2));
        break;
      case MMathFunction::Log1P:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_log1p));
        break;
      case MMathFunction::ExpM1:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_expm1));
        break;
      case MMathFunction::CosH:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_cosh));
        break;
      case MMathFunction::SinH:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_sinh));
        break;
      case MMathFunction::TanH:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_tanh));
        break;
      case MMathFunction::ACosH:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_acosh));
        break;
      case MMathFunction::ASinH:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_asinh));
        break;
      case MMathFunction::ATanH:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_atanh));
        break;
      case MMathFunction::Sign:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_sign));
        break;
      case MMathFunction::Trunc:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_trunc));
        break;
      case MMathFunction::Cbrt:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_cbrt));
        break;
      case MMathFunction::Floor:
        funptr = JS_FUNC_TO_DATA_PTR(void *, js::math_floor_impl);
        break;
      case MMathFunction::Ceil:
        funptr = JS_FUNC_TO_DATA_PTR(void *, js::math_ceil_impl);
        break;
      case MMathFunction::Round:
        funptr = JS_FUNC_TO_DATA_PTR(void *, js::math_round_impl);
        break;
      default:
        MOZ_CRASH("Unknown math function");
    }

#   undef MAYBE_CACHED

    masm.callWithABI(funptr, MoveOp::DOUBLE);
    return true;
}

bool
CodeGenerator::visitMathFunctionF(LMathFunctionF *ins)
{
    Register temp = ToRegister(ins->temp());
    FloatRegister input = ToFloatRegister(ins->input());
    JS_ASSERT(ToFloatRegister(ins->output()) == ReturnFloat32Reg);

    masm.setupUnalignedABICall(1, temp);
    masm.passABIArg(input, MoveOp::FLOAT32);

    void *funptr = nullptr;
    switch (ins->mir()->function()) {
      case MMathFunction::Floor: funptr = JS_FUNC_TO_DATA_PTR(void *, floorf);           break;
      case MMathFunction::Round: funptr = JS_FUNC_TO_DATA_PTR(void *, math_roundf_impl); break;
      case MMathFunction::Ceil:  funptr = JS_FUNC_TO_DATA_PTR(void *, ceilf);            break;
      default:
        MOZ_CRASH("Unknown or unsupported float32 math function");
    }

    masm.callWithABI(funptr, MoveOp::FLOAT32);
    return true;
}

bool
CodeGenerator::visitModD(LModD *ins)
{
    FloatRegister lhs = ToFloatRegister(ins->lhs());
    FloatRegister rhs = ToFloatRegister(ins->rhs());
    Register temp = ToRegister(ins->temp());

    JS_ASSERT(ToFloatRegister(ins->output()) == ReturnDoubleReg);

    masm.setupUnalignedABICall(2, temp);
    masm.passABIArg(lhs, MoveOp::DOUBLE);
    masm.passABIArg(rhs, MoveOp::DOUBLE);

    if (gen->compilingAsmJS())
        masm.callWithABI(AsmJSImm_ModD, MoveOp::DOUBLE);
    else
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, NumberMod), MoveOp::DOUBLE);
    return true;
}

typedef bool (*BinaryFn)(JSContext *, MutableHandleValue, MutableHandleValue, MutableHandleValue);
typedef bool (*BinaryParFn)(ForkJoinContext *, HandleValue, HandleValue, MutableHandleValue);

static const VMFunction AddInfo = FunctionInfo<BinaryFn>(js::AddValues);
static const VMFunction SubInfo = FunctionInfo<BinaryFn>(js::SubValues);
static const VMFunction MulInfo = FunctionInfo<BinaryFn>(js::MulValues);
static const VMFunction DivInfo = FunctionInfo<BinaryFn>(js::DivValues);
static const VMFunction ModInfo = FunctionInfo<BinaryFn>(js::ModValues);
static const VMFunctionsModal UrshInfo = VMFunctionsModal(
    FunctionInfo<BinaryFn>(js::UrshValues),
    FunctionInfo<BinaryParFn>(UrshValuesPar));

bool
CodeGenerator::visitBinaryV(LBinaryV *lir)
{
    pushArg(ToValue(lir, LBinaryV::RhsInput));
    pushArg(ToValue(lir, LBinaryV::LhsInput));

    switch (lir->jsop()) {
      case JSOP_ADD:
        return callVM(AddInfo, lir);

      case JSOP_SUB:
        return callVM(SubInfo, lir);

      case JSOP_MUL:
        return callVM(MulInfo, lir);

      case JSOP_DIV:
        return callVM(DivInfo, lir);

      case JSOP_MOD:
        return callVM(ModInfo, lir);

      case JSOP_URSH:
        return callVM(UrshInfo, lir);

      default:
        MOZ_CRASH("Unexpected binary op");
    }
}

typedef bool (*StringCompareFn)(JSContext *, HandleString, HandleString, bool *);
typedef bool (*StringCompareParFn)(ForkJoinContext *, HandleString, HandleString, bool *);
static const VMFunctionsModal StringsEqualInfo = VMFunctionsModal(
    FunctionInfo<StringCompareFn>(jit::StringsEqual<true>),
    FunctionInfo<StringCompareParFn>(jit::StringsEqualPar));
static const VMFunctionsModal StringsNotEqualInfo = VMFunctionsModal(
    FunctionInfo<StringCompareFn>(jit::StringsEqual<false>),
    FunctionInfo<StringCompareParFn>(jit::StringsUnequalPar));

bool
CodeGenerator::emitCompareS(LInstruction *lir, JSOp op, Register left, Register right,
                            Register output)
{
    JS_ASSERT(lir->isCompareS() || lir->isCompareStrictS());

    OutOfLineCode *ool = nullptr;

    if (op == JSOP_EQ || op == JSOP_STRICTEQ) {
        ool = oolCallVM(StringsEqualInfo, lir, (ArgList(), left, right),  StoreRegisterTo(output));
    } else {
        JS_ASSERT(op == JSOP_NE || op == JSOP_STRICTNE);
        ool = oolCallVM(StringsNotEqualInfo, lir, (ArgList(), left, right), StoreRegisterTo(output));
    }
    if (!ool)
        return false;

    masm.compareStrings(op, left, right, output, ool->entry());

    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitCompareStrictS(LCompareStrictS *lir)
{
    JSOp op = lir->mir()->jsop();
    JS_ASSERT(op == JSOP_STRICTEQ || op == JSOP_STRICTNE);

    const ValueOperand leftV = ToValue(lir, LCompareStrictS::Lhs);
    Register right = ToRegister(lir->right());
    Register output = ToRegister(lir->output());
    Register tempToUnbox = ToTempUnboxRegister(lir->tempToUnbox());

    Label string, done;

    masm.branchTestString(Assembler::Equal, leftV, &string);
    masm.move32(Imm32(op == JSOP_STRICTNE), output);
    masm.jump(&done);

    masm.bind(&string);
    Register left = masm.extractString(leftV, tempToUnbox);
    if (!emitCompareS(lir, op, left, right, output))
        return false;

    masm.bind(&done);

    return true;
}

bool
CodeGenerator::visitCompareS(LCompareS *lir)
{
    JSOp op = lir->mir()->jsop();
    Register left = ToRegister(lir->left());
    Register right = ToRegister(lir->right());
    Register output = ToRegister(lir->output());

    return emitCompareS(lir, op, left, right, output);
}

typedef bool (*CompareFn)(JSContext *, MutableHandleValue, MutableHandleValue, bool *);
typedef bool (*CompareParFn)(ForkJoinContext *, MutableHandleValue, MutableHandleValue, bool *);
static const VMFunctionsModal EqInfo = VMFunctionsModal(
    FunctionInfo<CompareFn>(jit::LooselyEqual<true>),
    FunctionInfo<CompareParFn>(jit::LooselyEqualPar));
static const VMFunctionsModal NeInfo = VMFunctionsModal(
    FunctionInfo<CompareFn>(jit::LooselyEqual<false>),
    FunctionInfo<CompareParFn>(jit::LooselyUnequalPar));
static const VMFunctionsModal StrictEqInfo = VMFunctionsModal(
    FunctionInfo<CompareFn>(jit::StrictlyEqual<true>),
    FunctionInfo<CompareParFn>(jit::StrictlyEqualPar));
static const VMFunctionsModal StrictNeInfo = VMFunctionsModal(
    FunctionInfo<CompareFn>(jit::StrictlyEqual<false>),
    FunctionInfo<CompareParFn>(jit::StrictlyUnequalPar));
static const VMFunctionsModal LtInfo = VMFunctionsModal(
    FunctionInfo<CompareFn>(jit::LessThan),
    FunctionInfo<CompareParFn>(jit::LessThanPar));
static const VMFunctionsModal LeInfo = VMFunctionsModal(
    FunctionInfo<CompareFn>(jit::LessThanOrEqual),
    FunctionInfo<CompareParFn>(jit::LessThanOrEqualPar));
static const VMFunctionsModal GtInfo = VMFunctionsModal(
    FunctionInfo<CompareFn>(jit::GreaterThan),
    FunctionInfo<CompareParFn>(jit::GreaterThanPar));
static const VMFunctionsModal GeInfo = VMFunctionsModal(
    FunctionInfo<CompareFn>(jit::GreaterThanOrEqual),
    FunctionInfo<CompareParFn>(jit::GreaterThanOrEqualPar));

bool
CodeGenerator::visitCompareVM(LCompareVM *lir)
{
    pushArg(ToValue(lir, LBinaryV::RhsInput));
    pushArg(ToValue(lir, LBinaryV::LhsInput));

    switch (lir->mir()->jsop()) {
      case JSOP_EQ:
        return callVM(EqInfo, lir);

      case JSOP_NE:
        return callVM(NeInfo, lir);

      case JSOP_STRICTEQ:
        return callVM(StrictEqInfo, lir);

      case JSOP_STRICTNE:
        return callVM(StrictNeInfo, lir);

      case JSOP_LT:
        return callVM(LtInfo, lir);

      case JSOP_LE:
        return callVM(LeInfo, lir);

      case JSOP_GT:
        return callVM(GtInfo, lir);

      case JSOP_GE:
        return callVM(GeInfo, lir);

      default:
        MOZ_CRASH("Unexpected compare op");
    }
}

bool
CodeGenerator::visitIsNullOrLikeUndefined(LIsNullOrLikeUndefined *lir)
{
    JSOp op = lir->mir()->jsop();
    MCompare::CompareType compareType = lir->mir()->compareType();
    JS_ASSERT(compareType == MCompare::Compare_Undefined ||
              compareType == MCompare::Compare_Null);

    const ValueOperand value = ToValue(lir, LIsNullOrLikeUndefined::Value);
    Register output = ToRegister(lir->output());

    if (op == JSOP_EQ || op == JSOP_NE) {
        MOZ_ASSERT(lir->mir()->lhs()->type() != MIRType_Object ||
                   lir->mir()->operandMightEmulateUndefined(),
                   "Operands which can't emulate undefined should have been folded");

        OutOfLineTestObjectWithLabels *ool = nullptr;
        Maybe<Label> label1, label2;
        Label *nullOrLikeUndefined;
        Label *notNullOrLikeUndefined;
        if (lir->mir()->operandMightEmulateUndefined()) {
            ool = new(alloc()) OutOfLineTestObjectWithLabels();
            if (!addOutOfLineCode(ool, lir->mir()))
                return false;
            nullOrLikeUndefined = ool->label1();
            notNullOrLikeUndefined = ool->label2();
        } else {
            label1.emplace();
            label2.emplace();
            nullOrLikeUndefined = label1.ptr();
            notNullOrLikeUndefined = label2.ptr();
        }

        Register tag = masm.splitTagForTest(value);

        masm.branchTestNull(Assembler::Equal, tag, nullOrLikeUndefined);
        masm.branchTestUndefined(Assembler::Equal, tag, nullOrLikeUndefined);

        if (ool) {
            
            
            masm.branchTestObject(Assembler::NotEqual, tag, notNullOrLikeUndefined);

            Register objreg = masm.extractObject(value, ToTempUnboxRegister(lir->tempToUnbox()));
            branchTestObjectEmulatesUndefined(objreg, nullOrLikeUndefined, notNullOrLikeUndefined,
                                              ToRegister(lir->temp()), ool);
            
        }

        Label done;

        
        
        masm.move32(Imm32(op == JSOP_NE), output);
        masm.jump(&done);

        masm.bind(nullOrLikeUndefined);
        masm.move32(Imm32(op == JSOP_EQ), output);

        
        masm.bind(&done);
        return true;
    }

    JS_ASSERT(op == JSOP_STRICTEQ || op == JSOP_STRICTNE);

    Assembler::Condition cond = JSOpToCondition(compareType, op);
    if (compareType == MCompare::Compare_Null)
        masm.testNullSet(cond, value, output);
    else
        masm.testUndefinedSet(cond, value, output);

    return true;
}

bool
CodeGenerator::visitIsNullOrLikeUndefinedAndBranch(LIsNullOrLikeUndefinedAndBranch *lir)
{
    JSOp op = lir->cmpMir()->jsop();
    MCompare::CompareType compareType = lir->cmpMir()->compareType();
    JS_ASSERT(compareType == MCompare::Compare_Undefined ||
              compareType == MCompare::Compare_Null);

    const ValueOperand value = ToValue(lir, LIsNullOrLikeUndefinedAndBranch::Value);

    if (op == JSOP_EQ || op == JSOP_NE) {
        MBasicBlock *ifTrue;
        MBasicBlock *ifFalse;

        if (op == JSOP_EQ) {
            ifTrue = lir->ifTrue();
            ifFalse = lir->ifFalse();
        } else {
            
            ifTrue = lir->ifFalse();
            ifFalse = lir->ifTrue();
            op = JSOP_EQ;
        }

        MOZ_ASSERT(lir->cmpMir()->lhs()->type() != MIRType_Object ||
                   lir->cmpMir()->operandMightEmulateUndefined(),
                   "Operands which can't emulate undefined should have been folded");

        OutOfLineTestObject *ool = nullptr;
        if (lir->cmpMir()->operandMightEmulateUndefined()) {
            ool = new(alloc()) OutOfLineTestObject();
            if (!addOutOfLineCode(ool, lir->cmpMir()))
                return false;
        }

        Register tag = masm.splitTagForTest(value);

        Label *ifTrueLabel = getJumpLabelForBranch(ifTrue);
        Label *ifFalseLabel = getJumpLabelForBranch(ifFalse);

        masm.branchTestNull(Assembler::Equal, tag, ifTrueLabel);
        masm.branchTestUndefined(Assembler::Equal, tag, ifTrueLabel);

        if (ool) {
            masm.branchTestObject(Assembler::NotEqual, tag, ifFalseLabel);

            
            Register objreg = masm.extractObject(value, ToTempUnboxRegister(lir->tempToUnbox()));
            Register scratch = ToRegister(lir->temp());
            testObjectEmulatesUndefined(objreg, ifTrueLabel, ifFalseLabel, scratch, ool);
        } else {
            masm.jump(ifFalseLabel);
        }
        return true;
    }

    JS_ASSERT(op == JSOP_STRICTEQ || op == JSOP_STRICTNE);

    Assembler::Condition cond = JSOpToCondition(compareType, op);
    if (compareType == MCompare::Compare_Null)
        testNullEmitBranch(cond, value, lir->ifTrue(), lir->ifFalse());
    else
        testUndefinedEmitBranch(cond, value, lir->ifTrue(), lir->ifFalse());

    return true;
}

bool
CodeGenerator::visitEmulatesUndefined(LEmulatesUndefined *lir)
{
    MOZ_ASSERT(lir->mir()->compareType() == MCompare::Compare_Undefined ||
               lir->mir()->compareType() == MCompare::Compare_Null);
    MOZ_ASSERT(lir->mir()->lhs()->type() == MIRType_Object);
    MOZ_ASSERT(lir->mir()->operandMightEmulateUndefined(),
               "If the object couldn't emulate undefined, this should have been folded.");

    JSOp op = lir->mir()->jsop();
    MOZ_ASSERT(op == JSOP_EQ || op == JSOP_NE, "Strict equality should have been folded");

    OutOfLineTestObjectWithLabels *ool = new(alloc()) OutOfLineTestObjectWithLabels();
    if (!addOutOfLineCode(ool, lir->mir()))
        return false;

    Label *emulatesUndefined = ool->label1();
    Label *doesntEmulateUndefined = ool->label2();

    Register objreg = ToRegister(lir->input());
    Register output = ToRegister(lir->output());
    branchTestObjectEmulatesUndefined(objreg, emulatesUndefined, doesntEmulateUndefined,
                                      output, ool);

    Label done;

    masm.move32(Imm32(op == JSOP_NE), output);
    masm.jump(&done);

    masm.bind(emulatesUndefined);
    masm.move32(Imm32(op == JSOP_EQ), output);
    masm.bind(&done);
    return true;
}

bool
CodeGenerator::visitEmulatesUndefinedAndBranch(LEmulatesUndefinedAndBranch *lir)
{
    MOZ_ASSERT(lir->cmpMir()->compareType() == MCompare::Compare_Undefined ||
               lir->cmpMir()->compareType() == MCompare::Compare_Null);
    MOZ_ASSERT(lir->cmpMir()->operandMightEmulateUndefined(),
               "Operands which can't emulate undefined should have been folded");

    JSOp op = lir->cmpMir()->jsop();
    MOZ_ASSERT(op == JSOP_EQ || op == JSOP_NE, "Strict equality should have been folded");

    OutOfLineTestObject *ool = new(alloc()) OutOfLineTestObject();
    if (!addOutOfLineCode(ool, lir->cmpMir()))
        return false;

    Label *equal;
    Label *unequal;

    {
        MBasicBlock *ifTrue;
        MBasicBlock *ifFalse;

        if (op == JSOP_EQ) {
            ifTrue = lir->ifTrue();
            ifFalse = lir->ifFalse();
        } else {
            
            ifTrue = lir->ifFalse();
            ifFalse = lir->ifTrue();
            op = JSOP_EQ;
        }

        equal = getJumpLabelForBranch(ifTrue);
        unequal = getJumpLabelForBranch(ifFalse);
    }

    Register objreg = ToRegister(lir->input());

    testObjectEmulatesUndefined(objreg, equal, unequal, ToRegister(lir->temp()), ool);
    return true;
}

typedef JSString *(*ConcatStringsFn)(ThreadSafeContext *, HandleString, HandleString);
typedef JSString *(*ConcatStringsParFn)(ForkJoinContext *, HandleString, HandleString);
static const VMFunctionsModal ConcatStringsInfo = VMFunctionsModal(
    FunctionInfo<ConcatStringsFn>(ConcatStrings<CanGC>),
    FunctionInfo<ConcatStringsParFn>(ConcatStringsPar));

bool
CodeGenerator::emitConcat(LInstruction *lir, Register lhs, Register rhs, Register output)
{
    OutOfLineCode *ool = oolCallVM(ConcatStringsInfo, lir, (ArgList(), lhs, rhs),
                                   StoreRegisterTo(output));
    if (!ool)
        return false;

    ExecutionMode mode = gen->info().executionMode();
    JitCode *stringConcatStub = gen->compartment->jitCompartment()->stringConcatStubNoBarrier(mode);
    masm.call(stringConcatStub);
    masm.branchTestPtr(Assembler::Zero, output, output, ool->entry());

    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitConcat(LConcat *lir)
{
    Register lhs = ToRegister(lir->lhs());
    Register rhs = ToRegister(lir->rhs());

    Register output = ToRegister(lir->output());

    JS_ASSERT(lhs == CallTempReg0);
    JS_ASSERT(rhs == CallTempReg1);
    JS_ASSERT(ToRegister(lir->temp1()) == CallTempReg0);
    JS_ASSERT(ToRegister(lir->temp2()) == CallTempReg1);
    JS_ASSERT(ToRegister(lir->temp3()) == CallTempReg2);
    JS_ASSERT(ToRegister(lir->temp4()) == CallTempReg3);
    JS_ASSERT(ToRegister(lir->temp5()) == CallTempReg4);
    JS_ASSERT(output == CallTempReg5);

    return emitConcat(lir, lhs, rhs, output);
}

bool
CodeGenerator::visitConcatPar(LConcatPar *lir)
{
    DebugOnly<Register> cx = ToRegister(lir->forkJoinContext());
    Register lhs = ToRegister(lir->lhs());
    Register rhs = ToRegister(lir->rhs());
    Register output = ToRegister(lir->output());

    JS_ASSERT(lhs == CallTempReg0);
    JS_ASSERT(rhs == CallTempReg1);
    JS_ASSERT((Register)cx == CallTempReg4);
    JS_ASSERT(ToRegister(lir->temp1()) == CallTempReg0);
    JS_ASSERT(ToRegister(lir->temp2()) == CallTempReg1);
    JS_ASSERT(ToRegister(lir->temp3()) == CallTempReg2);
    JS_ASSERT(ToRegister(lir->temp4()) == CallTempReg3);
    JS_ASSERT(output == CallTempReg5);

    return emitConcat(lir, lhs, rhs, output);
}

static void
CopyStringChars(MacroAssembler &masm, Register to, Register from, Register len, Register scratch,
                size_t fromWidth, size_t toWidth)
{
    
    

#ifdef DEBUG
    Label ok;
    masm.branch32(Assembler::GreaterThan, len, Imm32(0), &ok);
    masm.assumeUnreachable("Length should be greater than 0.");
    masm.bind(&ok);
#endif

    MOZ_ASSERT(fromWidth == 1 || fromWidth == 2);
    MOZ_ASSERT(toWidth == 1 || toWidth == 2);
    MOZ_ASSERT_IF(toWidth == 1, fromWidth == 1);

    Label start;
    masm.bind(&start);
    if (fromWidth == 2)
        masm.load16ZeroExtend(Address(from, 0), scratch);
    else
        masm.load8ZeroExtend(Address(from, 0), scratch);
    if (toWidth == 2)
        masm.store16(scratch, Address(to, 0));
    else
        masm.store8(scratch, Address(to, 0));
    masm.addPtr(Imm32(fromWidth), from);
    masm.addPtr(Imm32(toWidth), to);
    masm.branchSub32(Assembler::NonZero, Imm32(1), len, &start);
}

static void
CopyStringCharsMaybeInflate(MacroAssembler &masm, Register input, Register destChars,
                            Register temp1, Register temp2)
{
    
    

    Label isLatin1, done;
    masm.loadStringLength(input, temp1);
    masm.branchTest32(Assembler::NonZero, Address(input, JSString::offsetOfFlags()),
                      Imm32(JSString::LATIN1_CHARS_BIT), &isLatin1);
    {
        masm.loadStringChars(input, input);
        CopyStringChars(masm, destChars, input, temp1, temp2, sizeof(jschar), sizeof(jschar));
        masm.jump(&done);
    }
    masm.bind(&isLatin1);
    {
        masm.loadStringChars(input, input);
        CopyStringChars(masm, destChars, input, temp1, temp2, sizeof(char), sizeof(jschar));
    }
    masm.bind(&done);
}

static void
ConcatFatInlineString(MacroAssembler &masm, Register lhs, Register rhs, Register output,
                      Register temp1, Register temp2, Register temp3, Register forkJoinContext,
                      ExecutionMode mode, Label *failure, Label *failurePopTemps, bool isTwoByte)
{
    

    
    masm.branchIfRope(lhs, failure);
    masm.branchIfRope(rhs, failure);

    
    switch (mode) {
      case SequentialExecution:
        masm.newGCFatInlineString(output, temp1, failure);
        break;
      case ParallelExecution:
        masm.push(temp1);
        masm.push(temp2);
        masm.newGCFatInlineStringPar(output, forkJoinContext, temp1, temp2, failurePopTemps);
        masm.pop(temp2);
        masm.pop(temp1);
        break;
      default:
        MOZ_CRASH("No such execution mode");
    }

    
    uint32_t flags = JSString::INIT_FAT_INLINE_FLAGS;
    if (!isTwoByte)
        flags |= JSString::LATIN1_CHARS_BIT;
    masm.store32(Imm32(flags), Address(output, JSString::offsetOfFlags()));
    masm.store32(temp2, Address(output, JSString::offsetOfLength()));

    
    masm.computeEffectiveAddress(Address(output, JSInlineString::offsetOfInlineStorage()), temp2);

    {
        
        
        
        if (mode == ParallelExecution)
            masm.push(temp3);

        
        
        if (isTwoByte) {
            CopyStringCharsMaybeInflate(masm, lhs, temp2, temp1, temp3);
        } else {
            masm.loadStringLength(lhs, temp3);
            masm.loadStringChars(lhs, lhs);
            CopyStringChars(masm, temp2, lhs, temp3, temp1, sizeof(char), sizeof(char));
        }

        
        if (isTwoByte) {
            CopyStringCharsMaybeInflate(masm, rhs, temp2, temp1, temp3);
        } else {
            masm.loadStringLength(rhs, temp3);
            masm.loadStringChars(rhs, rhs);
            CopyStringChars(masm, temp2, rhs, temp3, temp1, sizeof(char), sizeof(char));
        }

        
        if (isTwoByte)
            masm.store16(Imm32(0), Address(temp2, 0));
        else
            masm.store8(Imm32(0), Address(temp2, 0));

        if (mode == ParallelExecution)
            masm.pop(temp3);
    }

    masm.ret();
}

JitCode *
JitCompartment::generateStringConcatStub(JSContext *cx, ExecutionMode mode)
{
    MacroAssembler masm(cx);

    Register lhs = CallTempReg0;
    Register rhs = CallTempReg1;
    Register temp1 = CallTempReg2;
    Register temp2 = CallTempReg3;
    Register temp3 = CallTempReg4;
    Register output = CallTempReg5;

    
    
    
    Register forkJoinContext = CallTempReg4;

    Label failure, failurePopTemps;

    
    Label leftEmpty;
    masm.loadStringLength(lhs, temp1);
    masm.branchTest32(Assembler::Zero, temp1, temp1, &leftEmpty);

    
    Label rightEmpty;
    masm.loadStringLength(rhs, temp2);
    masm.branchTest32(Assembler::Zero, temp2, temp2, &rightEmpty);

    masm.add32(temp1, temp2);

    
    
    Label isFatInlineTwoByte, isFatInlineLatin1;
    masm.load32(Address(lhs, JSString::offsetOfFlags()), temp1);
    masm.and32(Address(rhs, JSString::offsetOfFlags()), temp1);

    Label isLatin1, notInline;
    masm.branchTest32(Assembler::NonZero, temp1, Imm32(JSString::LATIN1_CHARS_BIT), &isLatin1);
    {
        masm.branch32(Assembler::BelowOrEqual, temp2, Imm32(JSFatInlineString::MAX_LENGTH_TWO_BYTE),
                      &isFatInlineTwoByte);
        masm.jump(&notInline);
    }
    masm.bind(&isLatin1);
    {
        masm.branch32(Assembler::BelowOrEqual, temp2, Imm32(JSFatInlineString::MAX_LENGTH_LATIN1),
                      &isFatInlineLatin1);
    }
    masm.bind(&notInline);

    

    
    masm.branch32(Assembler::Above, temp2, Imm32(JSString::MAX_LENGTH), &failure);

    
    switch (mode) {
      case SequentialExecution:
        masm.newGCString(output, temp3, &failure);
        break;
      case ParallelExecution:
        masm.push(temp1);
        masm.push(temp2);
        masm.newGCStringPar(output, forkJoinContext, temp1, temp2, &failurePopTemps);
        masm.pop(temp2);
        masm.pop(temp1);
        break;
      default:
        MOZ_CRASH("No such execution mode");
    }

    
    
    
    static_assert(JSString::ROPE_FLAGS == 0, "Rope flags must be 0");
    masm.and32(Imm32(JSString::LATIN1_CHARS_BIT), temp1);
    masm.store32(temp1, Address(output, JSString::offsetOfFlags()));
    masm.store32(temp2, Address(output, JSString::offsetOfLength()));

    
    masm.storePtr(lhs, Address(output, JSRope::offsetOfLeft()));
    masm.storePtr(rhs, Address(output, JSRope::offsetOfRight()));
    masm.ret();

    masm.bind(&leftEmpty);
    masm.mov(rhs, output);
    masm.ret();

    masm.bind(&rightEmpty);
    masm.mov(lhs, output);
    masm.ret();

    masm.bind(&isFatInlineTwoByte);
    ConcatFatInlineString(masm, lhs, rhs, output, temp1, temp2, temp3, forkJoinContext,
                          mode, &failure, &failurePopTemps, true);

    masm.bind(&isFatInlineLatin1);
    ConcatFatInlineString(masm, lhs, rhs, output, temp1, temp2, temp3, forkJoinContext,
                          mode, &failure, &failurePopTemps, false);

    masm.bind(&failurePopTemps);
    masm.pop(temp2);
    masm.pop(temp1);

    masm.bind(&failure);
    masm.movePtr(ImmPtr(nullptr), output);
    masm.ret();

    Linker linker(masm);
    AutoFlushICache afc("StringConcatStub");
    JitCode *code = linker.newCode<CanGC>(cx, OTHER_CODE);

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(code, "StringConcatStub");
#endif

    return code;
}

JitCode *
JitRuntime::generateMallocStub(JSContext *cx)
{
    const Register regReturn = CallTempReg0;
    const Register regNBytes = CallTempReg0;

    MacroAssembler masm(cx);

    RegisterSet regs = RegisterSet::Volatile();
    regs.takeUnchecked(regNBytes);
    masm.PushRegsInMask(regs);

    const Register regTemp = regs.takeGeneral();
    const Register regRuntime = regTemp;
    regs.add(regTemp);
    JS_ASSERT(regTemp != regNBytes);

    masm.setupUnalignedABICall(2, regTemp);
    masm.movePtr(ImmPtr(cx->runtime()), regRuntime);
    masm.passABIArg(regRuntime);
    masm.passABIArg(regNBytes);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, MallocWrapper));
    masm.storeCallResult(regReturn);

    masm.PopRegsInMask(regs);
    masm.ret();

    Linker linker(masm);
    AutoFlushICache afc("MallocStub");
    JitCode *code = linker.newCode<NoGC>(cx, OTHER_CODE);

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(code, "MallocStub");
#endif

    return code;
}

JitCode *
JitRuntime::generateFreeStub(JSContext *cx)
{
    const Register regSlots = CallTempReg0;

    MacroAssembler masm(cx);

    RegisterSet regs = RegisterSet::Volatile();
    regs.takeUnchecked(regSlots);
    masm.PushRegsInMask(regs);

    const Register regTemp = regs.takeGeneral();
    regs.add(regTemp);
    JS_ASSERT(regTemp != regSlots);

    masm.setupUnalignedABICall(1, regTemp);
    masm.passABIArg(regSlots);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, js_free));

    masm.PopRegsInMask(regs);

    masm.ret();

    Linker linker(masm);
    AutoFlushICache afc("FreeStub");
    JitCode *code = linker.newCode<NoGC>(cx, OTHER_CODE);

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(code, "FreeStub");
#endif

    return code;
}

typedef bool (*CharCodeAtFn)(JSContext *, HandleString, int32_t, uint32_t *);
static const VMFunction CharCodeAtInfo = FunctionInfo<CharCodeAtFn>(jit::CharCodeAt);

bool
CodeGenerator::visitCharCodeAt(LCharCodeAt *lir)
{
    Register str = ToRegister(lir->str());
    Register index = ToRegister(lir->index());
    Register output = ToRegister(lir->output());

    OutOfLineCode *ool = oolCallVM(CharCodeAtInfo, lir, (ArgList(), str, index), StoreRegisterTo(output));
    if (!ool)
        return false;

    masm.branchIfRope(str, ool->entry());
    masm.loadStringChar(str, index, output);

    masm.bind(ool->rejoin());
    return true;
}

typedef JSFlatString *(*StringFromCharCodeFn)(JSContext *, int32_t);
static const VMFunction StringFromCharCodeInfo = FunctionInfo<StringFromCharCodeFn>(jit::StringFromCharCode);

bool
CodeGenerator::visitFromCharCode(LFromCharCode *lir)
{
    Register code = ToRegister(lir->code());
    Register output = ToRegister(lir->output());

    OutOfLineCode *ool = oolCallVM(StringFromCharCodeInfo, lir, (ArgList(), code), StoreRegisterTo(output));
    if (!ool)
        return false;

    
    masm.branch32(Assembler::AboveOrEqual, code, Imm32(StaticStrings::UNIT_STATIC_LIMIT),
                  ool->entry());

    masm.movePtr(ImmPtr(&GetIonContext()->runtime->staticStrings().unitStaticTable), output);
    masm.loadPtr(BaseIndex(output, code, ScalePointer), output);

    masm.bind(ool->rejoin());
    return true;
}

typedef JSObject *(*StringSplitFn)(JSContext *, HandleTypeObject, HandleString, HandleString);
static const VMFunction StringSplitInfo = FunctionInfo<StringSplitFn>(js::str_split_string);

bool
CodeGenerator::visitStringSplit(LStringSplit *lir)
{
    pushArg(ToRegister(lir->separator()));
    pushArg(ToRegister(lir->string()));
    pushArg(ImmGCPtr(lir->mir()->typeObject()));

    return callVM(StringSplitInfo, lir);
}

bool
CodeGenerator::visitInitializedLength(LInitializedLength *lir)
{
    Address initLength(ToRegister(lir->elements()), ObjectElements::offsetOfInitializedLength());
    masm.load32(initLength, ToRegister(lir->output()));
    return true;
}

bool
CodeGenerator::visitSetInitializedLength(LSetInitializedLength *lir)
{
    Address initLength(ToRegister(lir->elements()), ObjectElements::offsetOfInitializedLength());
    Int32Key index = ToInt32Key(lir->index());

    masm.bumpKey(&index, 1);
    masm.storeKey(index, initLength);
    
    masm.bumpKey(&index, -1);
    return true;
}

bool
CodeGenerator::visitNotO(LNotO *lir)
{
    MOZ_ASSERT(lir->mir()->operandMightEmulateUndefined(),
               "This should be constant-folded if the object can't emulate undefined.");

    OutOfLineTestObjectWithLabels *ool = new(alloc()) OutOfLineTestObjectWithLabels();
    if (!addOutOfLineCode(ool, lir->mir()))
        return false;

    Label *ifEmulatesUndefined = ool->label1();
    Label *ifDoesntEmulateUndefined = ool->label2();

    Register objreg = ToRegister(lir->input());
    Register output = ToRegister(lir->output());
    branchTestObjectEmulatesUndefined(objreg, ifEmulatesUndefined, ifDoesntEmulateUndefined,
                                      output, ool);
    

    Label join;

    masm.move32(Imm32(0), output);
    masm.jump(&join);

    masm.bind(ifEmulatesUndefined);
    masm.move32(Imm32(1), output);

    masm.bind(&join);
    return true;
}

bool
CodeGenerator::visitNotV(LNotV *lir)
{
    Maybe<Label> ifTruthyLabel, ifFalsyLabel;
    Label *ifTruthy;
    Label *ifFalsy;

    OutOfLineTestObjectWithLabels *ool = nullptr;
    MDefinition *operand = lir->mir()->input();
    
    
    
    
    if (lir->mir()->operandMightEmulateUndefined() && operand->mightBeType(MIRType_Object)) {
        ool = new(alloc()) OutOfLineTestObjectWithLabels();
        if (!addOutOfLineCode(ool, lir->mir()))
            return false;
        ifTruthy = ool->label1();
        ifFalsy = ool->label2();
    } else {
        ifTruthyLabel.emplace();
        ifFalsyLabel.emplace();
        ifTruthy = ifTruthyLabel.ptr();
        ifFalsy = ifFalsyLabel.ptr();
    }

    testValueTruthyKernel(ToValue(lir, LNotV::Input), lir->temp1(), lir->temp2(),
                          ToFloatRegister(lir->tempFloat()),
                          ifTruthy, ifFalsy, ool, operand);

    Label join;
    Register output = ToRegister(lir->output());

    
    
    masm.bind(ifTruthy);
    masm.move32(Imm32(0), output);
    masm.jump(&join);

    masm.bind(ifFalsy);
    masm.move32(Imm32(1), output);

    
    masm.bind(&join);
    return true;
}

bool
CodeGenerator::visitBoundsCheck(LBoundsCheck *lir)
{
    if (lir->index()->isConstant()) {
        
        uint32_t index = ToInt32(lir->index());
        if (lir->length()->isConstant()) {
            uint32_t length = ToInt32(lir->length());
            if (index < length)
                return true;
            return bailout(lir->snapshot());
        }
        return bailoutCmp32(Assembler::BelowOrEqual, ToOperand(lir->length()), Imm32(index),
                            lir->snapshot());
    }
    if (lir->length()->isConstant()) {
        return bailoutCmp32(Assembler::AboveOrEqual, ToRegister(lir->index()),
                             Imm32(ToInt32(lir->length())), lir->snapshot());
    }
    return bailoutCmp32(Assembler::BelowOrEqual, ToOperand(lir->length()),
                        ToRegister(lir->index()), lir->snapshot());
}

bool
CodeGenerator::visitBoundsCheckRange(LBoundsCheckRange *lir)
{
    int32_t min = lir->mir()->minimum();
    int32_t max = lir->mir()->maximum();
    JS_ASSERT(max >= min);

    Register temp = ToRegister(lir->getTemp(0));
    if (lir->index()->isConstant()) {
        int32_t nmin, nmax;
        int32_t index = ToInt32(lir->index());
        if (SafeAdd(index, min, &nmin) && SafeAdd(index, max, &nmax) && nmin >= 0) {
            return bailoutCmp32(Assembler::BelowOrEqual, ToOperand(lir->length()), Imm32(nmax),
                                lir->snapshot());
        }
        masm.mov(ImmWord(index), temp);
    } else {
        masm.mov(ToRegister(lir->index()), temp);
    }

    
    
    
    if (min != max) {
        if (min != 0) {
            Label bail;
            masm.branchAdd32(Assembler::Overflow, Imm32(min), temp, &bail);
            if (!bailoutFrom(&bail, lir->snapshot()))
                return false;
        }

        if (!bailoutCmp32(Assembler::LessThan, temp, Imm32(0), lir->snapshot()))
            return false;

        if (min != 0) {
            int32_t diff;
            if (SafeSub(max, min, &diff))
                max = diff;
            else
                masm.sub32(Imm32(min), temp);
        }
    }

    
    
    
    
    
    if (max != 0) {
        if (max < 0) {
            Label bail;
            masm.branchAdd32(Assembler::Overflow, Imm32(max), temp, &bail);
            if (!bailoutFrom(&bail, lir->snapshot()))
                return false;
        } else {
            masm.add32(Imm32(max), temp);
        }
    }

    return bailoutCmp32(Assembler::BelowOrEqual, ToOperand(lir->length()), temp, lir->snapshot());
}

bool
CodeGenerator::visitBoundsCheckLower(LBoundsCheckLower *lir)
{
    int32_t min = lir->mir()->minimum();
    return bailoutCmp32(Assembler::LessThan, ToRegister(lir->index()), Imm32(min),
                        lir->snapshot());
}

class OutOfLineStoreElementHole : public OutOfLineCodeBase<CodeGenerator>
{
    LInstruction *ins_;
    Label rejoinStore_;

  public:
    explicit OutOfLineStoreElementHole(LInstruction *ins)
      : ins_(ins)
    {
        JS_ASSERT(ins->isStoreElementHoleV() || ins->isStoreElementHoleT());
    }

    bool accept(CodeGenerator *codegen) {
        return codegen->visitOutOfLineStoreElementHole(this);
    }
    LInstruction *ins() const {
        return ins_;
    }
    Label *rejoinStore() {
        return &rejoinStore_;
    }
};

bool
CodeGenerator::emitStoreHoleCheck(Register elements, const LAllocation *index, LSnapshot *snapshot)
{
    Label bail;
    if (index->isConstant()) {
        masm.branchTestMagic(Assembler::Equal,
                             Address(elements, ToInt32(index) * sizeof(js::Value)), &bail);
    } else {
        masm.branchTestMagic(Assembler::Equal,
                             BaseIndex(elements, ToRegister(index), TimesEight), &bail);
    }
    return bailoutFrom(&bail, snapshot);
}

void
CodeGenerator::emitStoreElementTyped(const LAllocation *value, MIRType valueType, MIRType elementType,
                                     Register elements, const LAllocation *index)
{
    ConstantOrRegister v;
    if (value->isConstant())
        v = ConstantOrRegister(*value->toConstant());
    else
        v = TypedOrValueRegister(valueType, ToAnyRegister(value));

    if (index->isConstant()) {
        Address dest(elements, ToInt32(index) * sizeof(js::Value));
        masm.storeUnboxedValue(v, valueType, dest, elementType);
    } else {
        BaseIndex dest(elements, ToRegister(index), TimesEight);
        masm.storeUnboxedValue(v, valueType, dest, elementType);
    }
}

bool
CodeGenerator::visitStoreElementT(LStoreElementT *store)
{
    Register elements = ToRegister(store->elements());
    const LAllocation *index = store->index();

    if (store->mir()->needsBarrier())
        emitPreBarrier(elements, index);

    if (store->mir()->needsHoleCheck() && !emitStoreHoleCheck(elements, index, store->snapshot()))
        return false;

    emitStoreElementTyped(store->value(), store->mir()->value()->type(), store->mir()->elementType(),
                          elements, index);
    return true;
}

bool
CodeGenerator::visitStoreElementV(LStoreElementV *lir)
{
    const ValueOperand value = ToValue(lir, LStoreElementV::Value);
    Register elements = ToRegister(lir->elements());
    const LAllocation *index = lir->index();

    if (lir->mir()->needsBarrier())
        emitPreBarrier(elements, index);

    if (lir->mir()->needsHoleCheck() && !emitStoreHoleCheck(elements, index, lir->snapshot()))
        return false;

    if (lir->index()->isConstant())
        masm.storeValue(value, Address(elements, ToInt32(lir->index()) * sizeof(js::Value)));
    else
        masm.storeValue(value, BaseIndex(elements, ToRegister(lir->index()), TimesEight));
    return true;
}

bool
CodeGenerator::visitStoreElementHoleT(LStoreElementHoleT *lir)
{
    OutOfLineStoreElementHole *ool = new(alloc()) OutOfLineStoreElementHole(lir);
    if (!addOutOfLineCode(ool, lir->mir()))
        return false;

    Register elements = ToRegister(lir->elements());
    const LAllocation *index = lir->index();

    
    Address initLength(elements, ObjectElements::offsetOfInitializedLength());
    masm.branchKey(Assembler::BelowOrEqual, initLength, ToInt32Key(index), ool->entry());

    if (lir->mir()->needsBarrier())
        emitPreBarrier(elements, index);

    masm.bind(ool->rejoinStore());
    emitStoreElementTyped(lir->value(), lir->mir()->value()->type(), lir->mir()->elementType(),
                          elements, index);

    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitStoreElementHoleV(LStoreElementHoleV *lir)
{
    OutOfLineStoreElementHole *ool = new(alloc()) OutOfLineStoreElementHole(lir);
    if (!addOutOfLineCode(ool, lir->mir()))
        return false;

    Register elements = ToRegister(lir->elements());
    const LAllocation *index = lir->index();
    const ValueOperand value = ToValue(lir, LStoreElementHoleV::Value);

    
    Address initLength(elements, ObjectElements::offsetOfInitializedLength());
    masm.branchKey(Assembler::BelowOrEqual, initLength, ToInt32Key(index), ool->entry());

    if (lir->mir()->needsBarrier())
        emitPreBarrier(elements, index);

    masm.bind(ool->rejoinStore());
    if (lir->index()->isConstant())
        masm.storeValue(value, Address(elements, ToInt32(lir->index()) * sizeof(js::Value)));
    else
        masm.storeValue(value, BaseIndex(elements, ToRegister(lir->index()), TimesEight));

    masm.bind(ool->rejoin());
    return true;
}

typedef bool (*SetDenseElementFn)(JSContext *, HandleObject, int32_t, HandleValue,
                                  bool strict);
typedef bool (*SetDenseElementParFn)(ForkJoinContext *, HandleObject, int32_t, HandleValue, bool);
static const VMFunctionsModal SetDenseElementInfo = VMFunctionsModal(
    FunctionInfo<SetDenseElementFn>(SetDenseElement),
    FunctionInfo<SetDenseElementParFn>(SetDenseElementPar));

bool
CodeGenerator::visitOutOfLineStoreElementHole(OutOfLineStoreElementHole *ool)
{
    Register object, elements;
    LInstruction *ins = ool->ins();
    const LAllocation *index;
    MIRType valueType;
    ConstantOrRegister value;

    if (ins->isStoreElementHoleV()) {
        LStoreElementHoleV *store = ins->toStoreElementHoleV();
        object = ToRegister(store->object());
        elements = ToRegister(store->elements());
        index = store->index();
        valueType = store->mir()->value()->type();
        value = TypedOrValueRegister(ToValue(store, LStoreElementHoleV::Value));
    } else {
        LStoreElementHoleT *store = ins->toStoreElementHoleT();
        object = ToRegister(store->object());
        elements = ToRegister(store->elements());
        index = store->index();
        valueType = store->mir()->value()->type();
        if (store->value()->isConstant())
            value = ConstantOrRegister(*store->value()->toConstant());
        else
            value = TypedOrValueRegister(valueType, ToAnyRegister(store->value()));
    }

    
    
    
    Label callStub;
#ifdef JS_CODEGEN_MIPS
    
    Address initLength(elements, ObjectElements::offsetOfInitializedLength());
    masm.branchKey(Assembler::NotEqual, initLength, ToInt32Key(index), &callStub);
#else
    masm.j(Assembler::NotEqual, &callStub);
#endif

    Int32Key key = ToInt32Key(index);

    
    masm.branchKey(Assembler::BelowOrEqual, Address(elements, ObjectElements::offsetOfCapacity()),
                   key, &callStub);

    
    
    masm.bumpKey(&key, 1);
    masm.storeKey(key, Address(elements, ObjectElements::offsetOfInitializedLength()));

    
    Label dontUpdate;
    masm.branchKey(Assembler::AboveOrEqual, Address(elements, ObjectElements::offsetOfLength()),
                   key, &dontUpdate);
    masm.storeKey(key, Address(elements, ObjectElements::offsetOfLength()));
    masm.bind(&dontUpdate);

    masm.bumpKey(&key, -1);

    if (ins->isStoreElementHoleT() && valueType != MIRType_Double) {
        
        
        
        emitStoreElementTyped(ins->toStoreElementHoleT()->value(), valueType, MIRType_None,
                              elements, index);
        masm.jump(ool->rejoin());
    } else {
        
        masm.jump(ool->rejoinStore());
    }

    masm.bind(&callStub);
    saveLive(ins);

    pushArg(Imm32(current->mir()->strict()));
    pushArg(value);
    if (index->isConstant())
        pushArg(Imm32(ToInt32(index)));
    else
        pushArg(ToRegister(index));
    pushArg(object);
    if (!callVM(SetDenseElementInfo, ins))
        return false;

    restoreLive(ins);
    masm.jump(ool->rejoin());
    return true;
}

typedef bool (*ArrayPopShiftFn)(JSContext *, HandleObject, MutableHandleValue);
static const VMFunction ArrayPopDenseInfo = FunctionInfo<ArrayPopShiftFn>(jit::ArrayPopDense);
static const VMFunction ArrayShiftDenseInfo = FunctionInfo<ArrayPopShiftFn>(jit::ArrayShiftDense);

bool
CodeGenerator::emitArrayPopShift(LInstruction *lir, const MArrayPopShift *mir, Register obj,
                                 Register elementsTemp, Register lengthTemp, TypedOrValueRegister out)
{
    OutOfLineCode *ool;

    if (mir->mode() == MArrayPopShift::Pop) {
        ool = oolCallVM(ArrayPopDenseInfo, lir, (ArgList(), obj), StoreValueTo(out));
        if (!ool)
            return false;
    } else {
        JS_ASSERT(mir->mode() == MArrayPopShift::Shift);
        ool = oolCallVM(ArrayShiftDenseInfo, lir, (ArgList(), obj), StoreValueTo(out));
        if (!ool)
            return false;
    }

    
    masm.branchTestNeedsIncrementalBarrier(Assembler::NonZero, ool->entry());

    
    masm.loadPtr(Address(obj, JSObject::offsetOfElements()), elementsTemp);
    masm.load32(Address(elementsTemp, ObjectElements::offsetOfLength()), lengthTemp);

    
    Int32Key key = Int32Key(lengthTemp);
    Address initLength(elementsTemp, ObjectElements::offsetOfInitializedLength());
    masm.branchKey(Assembler::NotEqual, initLength, key, ool->entry());

    
    
    
    Label done;
    if (mir->maybeUndefined()) {
        Label notEmpty;
        masm.branchTest32(Assembler::NonZero, lengthTemp, lengthTemp, &notEmpty);
        masm.moveValue(UndefinedValue(), out.valueReg());
        masm.jump(&done);
        masm.bind(&notEmpty);
    } else {
        masm.branchTest32(Assembler::Zero, lengthTemp, lengthTemp, ool->entry());
    }

    masm.bumpKey(&key, -1);

    if (mir->mode() == MArrayPopShift::Pop) {
        masm.loadElementTypedOrValue(BaseIndex(elementsTemp, lengthTemp, TimesEight), out,
                                     mir->needsHoleCheck(), ool->entry());
    } else {
        JS_ASSERT(mir->mode() == MArrayPopShift::Shift);
        masm.loadElementTypedOrValue(Address(elementsTemp, 0), out, mir->needsHoleCheck(),
                                     ool->entry());
    }

    
    
    
    
    Address elementFlags(elementsTemp, ObjectElements::offsetOfFlags());
    Imm32 bit(ObjectElements::NONWRITABLE_ARRAY_LENGTH);
    masm.branchTest32(Assembler::NonZero, elementFlags, bit, ool->entry());

    
    masm.store32(lengthTemp, Address(elementsTemp, ObjectElements::offsetOfLength()));
    masm.store32(lengthTemp, Address(elementsTemp, ObjectElements::offsetOfInitializedLength()));

    if (mir->mode() == MArrayPopShift::Shift) {
        
        RegisterSet temps;
        temps.add(elementsTemp);
        temps.add(lengthTemp);

        saveVolatile(temps);
        masm.setupUnalignedABICall(1, lengthTemp);
        masm.passABIArg(obj);
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, js::ArrayShiftMoveElements));
        restoreVolatile(temps);
    }

    masm.bind(&done);
    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitArrayPopShiftV(LArrayPopShiftV *lir)
{
    Register obj = ToRegister(lir->object());
    Register elements = ToRegister(lir->temp0());
    Register length = ToRegister(lir->temp1());
    TypedOrValueRegister out(ToOutValue(lir));
    return emitArrayPopShift(lir, lir->mir(), obj, elements, length, out);
}

bool
CodeGenerator::visitArrayPopShiftT(LArrayPopShiftT *lir)
{
    Register obj = ToRegister(lir->object());
    Register elements = ToRegister(lir->temp0());
    Register length = ToRegister(lir->temp1());
    TypedOrValueRegister out(lir->mir()->type(), ToAnyRegister(lir->output()));
    return emitArrayPopShift(lir, lir->mir(), obj, elements, length, out);
}

typedef bool (*ArrayPushDenseFn)(JSContext *, HandleObject, HandleValue, uint32_t *);
static const VMFunction ArrayPushDenseInfo =
    FunctionInfo<ArrayPushDenseFn>(jit::ArrayPushDense);

bool
CodeGenerator::emitArrayPush(LInstruction *lir, const MArrayPush *mir, Register obj,
                             ConstantOrRegister value, Register elementsTemp, Register length)
{
    OutOfLineCode *ool = oolCallVM(ArrayPushDenseInfo, lir, (ArgList(), obj, value), StoreRegisterTo(length));
    if (!ool)
        return false;

    
    masm.loadPtr(Address(obj, JSObject::offsetOfElements()), elementsTemp);
    masm.load32(Address(elementsTemp, ObjectElements::offsetOfLength()), length);

    Int32Key key = Int32Key(length);
    Address initLength(elementsTemp, ObjectElements::offsetOfInitializedLength());
    Address capacity(elementsTemp, ObjectElements::offsetOfCapacity());

    
    masm.branchKey(Assembler::NotEqual, initLength, key, ool->entry());

    
    masm.branchKey(Assembler::BelowOrEqual, capacity, key, ool->entry());

    masm.storeConstantOrRegister(value, BaseIndex(elementsTemp, length, TimesEight));

    masm.bumpKey(&key, 1);
    masm.store32(length, Address(elementsTemp, ObjectElements::offsetOfLength()));
    masm.store32(length, Address(elementsTemp, ObjectElements::offsetOfInitializedLength()));

    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitArrayPushV(LArrayPushV *lir)
{
    Register obj = ToRegister(lir->object());
    Register elementsTemp = ToRegister(lir->temp());
    Register length = ToRegister(lir->output());
    ConstantOrRegister value = TypedOrValueRegister(ToValue(lir, LArrayPushV::Value));
    return emitArrayPush(lir, lir->mir(), obj, value, elementsTemp, length);
}

bool
CodeGenerator::visitArrayPushT(LArrayPushT *lir)
{
    Register obj = ToRegister(lir->object());
    Register elementsTemp = ToRegister(lir->temp());
    Register length = ToRegister(lir->output());
    ConstantOrRegister value;
    if (lir->value()->isConstant())
        value = ConstantOrRegister(*lir->value()->toConstant());
    else
        value = TypedOrValueRegister(lir->mir()->value()->type(), ToAnyRegister(lir->value()));
    return emitArrayPush(lir, lir->mir(), obj, value, elementsTemp, length);
}

typedef JSObject *(*ArrayConcatDenseFn)(JSContext *, HandleObject, HandleObject, HandleObject);
static const VMFunction ArrayConcatDenseInfo = FunctionInfo<ArrayConcatDenseFn>(ArrayConcatDense);

bool
CodeGenerator::visitArrayConcat(LArrayConcat *lir)
{
    Register lhs = ToRegister(lir->lhs());
    Register rhs = ToRegister(lir->rhs());
    Register temp1 = ToRegister(lir->temp1());
    Register temp2 = ToRegister(lir->temp2());

    
    
    
    Label fail, call;
    masm.loadPtr(Address(lhs, JSObject::offsetOfElements()), temp1);
    masm.load32(Address(temp1, ObjectElements::offsetOfInitializedLength()), temp2);
    masm.branch32(Assembler::NotEqual, Address(temp1, ObjectElements::offsetOfLength()), temp2, &fail);

    masm.loadPtr(Address(rhs, JSObject::offsetOfElements()), temp1);
    masm.load32(Address(temp1, ObjectElements::offsetOfInitializedLength()), temp2);
    masm.branch32(Assembler::NotEqual, Address(temp1, ObjectElements::offsetOfLength()), temp2, &fail);

    
    masm.createGCObject(temp1, temp2, lir->mir()->templateObj(), lir->mir()->initialHeap(), &fail);
    masm.jump(&call);
    {
        masm.bind(&fail);
        masm.movePtr(ImmPtr(nullptr), temp1);
    }
    masm.bind(&call);

    pushArg(temp1);
    pushArg(ToRegister(lir->rhs()));
    pushArg(ToRegister(lir->lhs()));
    return callVM(ArrayConcatDenseInfo, lir);
}

typedef JSString *(*ArrayJoinFn)(JSContext *, HandleObject, HandleString);
static const VMFunction ArrayJoinInfo = FunctionInfo<ArrayJoinFn>(jit::ArrayJoin);

bool
CodeGenerator::visitArrayJoin(LArrayJoin *lir)
{
    pushArg(ToRegister(lir->separator()));
    pushArg(ToRegister(lir->array()));

    return callVM(ArrayJoinInfo, lir);
}

typedef JSObject *(*GetIteratorObjectFn)(JSContext *, HandleObject, uint32_t);
static const VMFunction GetIteratorObjectInfo = FunctionInfo<GetIteratorObjectFn>(GetIteratorObject);

bool
CodeGenerator::visitCallIteratorStart(LCallIteratorStart *lir)
{
    pushArg(Imm32(lir->mir()->flags()));
    pushArg(ToRegister(lir->object()));
    return callVM(GetIteratorObjectInfo, lir);
}

bool
CodeGenerator::visitIteratorStart(LIteratorStart *lir)
{
    const Register obj = ToRegister(lir->object());
    const Register output = ToRegister(lir->output());

    uint32_t flags = lir->mir()->flags();

    OutOfLineCode *ool = oolCallVM(GetIteratorObjectInfo, lir,
                                   (ArgList(), obj, Imm32(flags)), StoreRegisterTo(output));
    if (!ool)
        return false;

    const Register temp1 = ToRegister(lir->temp1());
    const Register temp2 = ToRegister(lir->temp2());
    const Register niTemp = ToRegister(lir->temp3()); 

    
    JS_ASSERT(flags == JSITER_ENUMERATE);

    
    masm.loadPtr(AbsoluteAddress(GetIonContext()->runtime->addressOfLastCachedNativeIterator()), output);
    masm.branchTestPtr(Assembler::Zero, output, output, ool->entry());

    
    masm.loadObjPrivate(output, JSObject::ITER_CLASS_NFIXED_SLOTS, niTemp);

    
    masm.branchTest32(Assembler::NonZero, Address(niTemp, offsetof(NativeIterator, flags)),
                      Imm32(JSITER_ACTIVE|JSITER_UNREUSABLE), ool->entry());

    
    masm.loadPtr(Address(niTemp, offsetof(NativeIterator, shapes_array)), temp2);

    
    masm.loadObjShape(obj, temp1);
    masm.branchPtr(Assembler::NotEqual, Address(temp2, 0), temp1, ool->entry());

    
    masm.loadObjProto(obj, temp1);
    masm.loadObjShape(temp1, temp1);
    masm.branchPtr(Assembler::NotEqual, Address(temp2, sizeof(Shape *)), temp1, ool->entry());

    
    
    
    masm.loadObjProto(obj, temp1);
    masm.loadObjProto(temp1, temp1);
    masm.branchTestPtr(Assembler::NonZero, temp1, temp1, ool->entry());

    
    
    masm.branchPtr(Assembler::NotEqual,
                   Address(obj, JSObject::offsetOfElements()),
                   ImmPtr(js::emptyObjectElements),
                   ool->entry());

    
    
    {
#ifdef JSGC_GENERATIONAL
        
        
        
        Address objAddr(niTemp, offsetof(NativeIterator, obj));
        masm.branchPtr(Assembler::NotEqual, objAddr, obj, ool->entry());
#else
        Label noBarrier;
        masm.branchTestNeedsIncrementalBarrier(Assembler::Zero, &noBarrier);

        Address objAddr(niTemp, offsetof(NativeIterator, obj));
        masm.branchPtr(Assembler::NotEqual, objAddr, obj, ool->entry());

        masm.bind(&noBarrier);
#endif 
    }

    
    masm.storePtr(obj, Address(niTemp, offsetof(NativeIterator, obj)));
    masm.or32(Imm32(JSITER_ACTIVE), Address(niTemp, offsetof(NativeIterator, flags)));

    
    masm.loadPtr(AbsoluteAddress(gen->compartment->addressOfEnumerators()), temp1);

    
    masm.storePtr(temp1, Address(niTemp, NativeIterator::offsetOfNext()));

    
    masm.loadPtr(Address(temp1, NativeIterator::offsetOfPrev()), temp2);
    masm.storePtr(temp2, Address(niTemp, NativeIterator::offsetOfPrev()));

    
    masm.storePtr(niTemp, Address(temp2, NativeIterator::offsetOfNext()));

    
    masm.storePtr(niTemp, Address(temp1, NativeIterator::offsetOfPrev()));

    masm.bind(ool->rejoin());
    return true;
}

static void
LoadNativeIterator(MacroAssembler &masm, Register obj, Register dest, Label *failures)
{
    JS_ASSERT(obj != dest);

    
    masm.branchTestObjClass(Assembler::NotEqual, obj, dest, &PropertyIteratorObject::class_, failures);

    
    masm.loadObjPrivate(obj, JSObject::ITER_CLASS_NFIXED_SLOTS, dest);
}

typedef bool (*IteratorNextFn)(JSContext *, HandleObject, MutableHandleValue);
static const VMFunction IteratorNextInfo = FunctionInfo<IteratorNextFn>(js_IteratorNext);

bool
CodeGenerator::visitIteratorNext(LIteratorNext *lir)
{
    const Register obj = ToRegister(lir->object());
    const Register temp = ToRegister(lir->temp());
    const ValueOperand output = ToOutValue(lir);

    OutOfLineCode *ool = oolCallVM(IteratorNextInfo, lir, (ArgList(), obj), StoreValueTo(output));
    if (!ool)
        return false;

    LoadNativeIterator(masm, obj, temp, ool->entry());

    masm.branchTest32(Assembler::NonZero, Address(temp, offsetof(NativeIterator, flags)),
                      Imm32(JSITER_FOREACH), ool->entry());

    
    masm.loadPtr(Address(temp, offsetof(NativeIterator, props_cursor)), output.scratchReg());
    masm.loadPtr(Address(output.scratchReg(), 0), output.scratchReg());
    masm.tagValue(JSVAL_TYPE_STRING, output.scratchReg(), output);

    
    masm.addPtr(Imm32(sizeof(JSString *)), Address(temp, offsetof(NativeIterator, props_cursor)));

    masm.bind(ool->rejoin());
    return true;
}

typedef bool (*IteratorMoreFn)(JSContext *, HandleObject, bool *);
static const VMFunction IteratorMoreInfo = FunctionInfo<IteratorMoreFn>(jit::IteratorMore);

bool
CodeGenerator::visitIteratorMore(LIteratorMore *lir)
{
    const Register obj = ToRegister(lir->object());
    const Register output = ToRegister(lir->output());
    const Register temp = ToRegister(lir->temp());

    OutOfLineCode *ool = oolCallVM(IteratorMoreInfo, lir,
                                   (ArgList(), obj), StoreRegisterTo(output));
    if (!ool)
        return false;

    LoadNativeIterator(masm, obj, output, ool->entry());

    masm.branchTest32(Assembler::NonZero, Address(output, offsetof(NativeIterator, flags)),
                      Imm32(JSITER_FOREACH), ool->entry());

    
    masm.loadPtr(Address(output, offsetof(NativeIterator, props_end)), temp);
    masm.cmpPtrSet(Assembler::LessThan, Address(output, offsetof(NativeIterator, props_cursor)),
                   temp, output);

    masm.bind(ool->rejoin());
    return true;
}

typedef bool (*CloseIteratorFn)(JSContext *, HandleObject);
static const VMFunction CloseIteratorInfo = FunctionInfo<CloseIteratorFn>(CloseIterator);

bool
CodeGenerator::visitIteratorEnd(LIteratorEnd *lir)
{
    const Register obj = ToRegister(lir->object());
    const Register temp1 = ToRegister(lir->temp1());
    const Register temp2 = ToRegister(lir->temp2());
    const Register temp3 = ToRegister(lir->temp3());

    OutOfLineCode *ool = oolCallVM(CloseIteratorInfo, lir, (ArgList(), obj), StoreNothing());
    if (!ool)
        return false;

    LoadNativeIterator(masm, obj, temp1, ool->entry());

    masm.branchTest32(Assembler::Zero, Address(temp1, offsetof(NativeIterator, flags)),
                      Imm32(JSITER_ENUMERATE), ool->entry());

    
    masm.and32(Imm32(~JSITER_ACTIVE), Address(temp1, offsetof(NativeIterator, flags)));

    
    masm.loadPtr(Address(temp1, offsetof(NativeIterator, props_array)), temp2);
    masm.storePtr(temp2, Address(temp1, offsetof(NativeIterator, props_cursor)));

    
    const Register next = temp2;
    const Register prev = temp3;
    masm.loadPtr(Address(temp1, NativeIterator::offsetOfNext()), next);
    masm.loadPtr(Address(temp1, NativeIterator::offsetOfPrev()), prev);
    masm.storePtr(prev, Address(next, NativeIterator::offsetOfPrev()));
    masm.storePtr(next, Address(prev, NativeIterator::offsetOfNext()));
#ifdef DEBUG
    masm.storePtr(ImmPtr(nullptr), Address(temp1, NativeIterator::offsetOfNext()));
    masm.storePtr(ImmPtr(nullptr), Address(temp1, NativeIterator::offsetOfPrev()));
#endif

    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitArgumentsLength(LArgumentsLength *lir)
{
    
    Register argc = ToRegister(lir->output());
    Address ptr(StackPointer, frameSize() + IonJSFrameLayout::offsetOfNumActualArgs());

    masm.loadPtr(ptr, argc);
    return true;
}

bool
CodeGenerator::visitGetFrameArgument(LGetFrameArgument *lir)
{
    ValueOperand result = GetValueOutput(lir);
    const LAllocation *index = lir->index();
    size_t argvOffset = frameSize() + IonJSFrameLayout::offsetOfActualArgs();

    if (index->isConstant()) {
        int32_t i = index->toConstant()->toInt32();
        Address argPtr(StackPointer, sizeof(Value) * i + argvOffset);
        masm.loadValue(argPtr, result);
    } else {
        Register i = ToRegister(index);
        BaseIndex argPtr(StackPointer, i, ScaleFromElemWidth(sizeof(Value)), argvOffset);
        masm.loadValue(argPtr, result);
    }
    return true;
}

bool
CodeGenerator::visitSetFrameArgumentT(LSetFrameArgumentT *lir)
{
    size_t argOffset = frameSize() + IonJSFrameLayout::offsetOfActualArgs() +
                       (sizeof(Value) * lir->mir()->argno());

    MIRType type = lir->mir()->value()->type();

    if (type == MIRType_Double) {
        
        FloatRegister input = ToFloatRegister(lir->input());
        masm.storeDouble(input, Address(StackPointer, argOffset));

    } else {
        Register input = ToRegister(lir->input());
        masm.storeValue(ValueTypeFromMIRType(type), input, Address(StackPointer, argOffset));
    }
    return true;
}

bool
CodeGenerator:: visitSetFrameArgumentC(LSetFrameArgumentC *lir)
{
    size_t argOffset = frameSize() + IonJSFrameLayout::offsetOfActualArgs() +
                       (sizeof(Value) * lir->mir()->argno());
    masm.storeValue(lir->val(), Address(StackPointer, argOffset));
    return true;
}

bool
CodeGenerator:: visitSetFrameArgumentV(LSetFrameArgumentV *lir)
{
    const ValueOperand val = ToValue(lir, LSetFrameArgumentV::Input);
    size_t argOffset = frameSize() + IonJSFrameLayout::offsetOfActualArgs() +
                       (sizeof(Value) * lir->mir()->argno());
    masm.storeValue(val, Address(StackPointer, argOffset));
    return true;
}

typedef bool (*RunOnceScriptPrologueFn)(JSContext *, HandleScript);
static const VMFunction RunOnceScriptPrologueInfo =
    FunctionInfo<RunOnceScriptPrologueFn>(js::RunOnceScriptPrologue);

bool
CodeGenerator::visitRunOncePrologue(LRunOncePrologue *lir)
{
    pushArg(ImmGCPtr(lir->mir()->block()->info().script()));
    return callVM(RunOnceScriptPrologueInfo, lir);
}


typedef JSObject *(*InitRestParameterFn)(JSContext *, uint32_t, Value *, HandleObject,
                                         HandleObject);
typedef JSObject *(*InitRestParameterParFn)(ForkJoinContext *, uint32_t, Value *,
                                            HandleObject, HandleObject);
static const VMFunctionsModal InitRestParameterInfo = VMFunctionsModal(
    FunctionInfo<InitRestParameterFn>(InitRestParameter),
    FunctionInfo<InitRestParameterParFn>(InitRestParameterPar));

bool
CodeGenerator::emitRest(LInstruction *lir, Register array, Register numActuals,
                        Register temp0, Register temp1, unsigned numFormals,
                        JSObject *templateObject, bool saveAndRestore, Register resultreg)
{
    
    size_t actualsOffset = frameSize() + IonJSFrameLayout::offsetOfActualArgs();
    masm.movePtr(StackPointer, temp1);
    masm.addPtr(Imm32(sizeof(Value) * numFormals + actualsOffset), temp1);

    
    Label emptyLength, joinLength;
    masm.movePtr(numActuals, temp0);
    masm.branch32(Assembler::LessThanOrEqual, temp0, Imm32(numFormals), &emptyLength);
    masm.sub32(Imm32(numFormals), temp0);
    masm.jump(&joinLength);
    {
        masm.bind(&emptyLength);
        masm.move32(Imm32(0), temp0);
    }
    masm.bind(&joinLength);

    if (saveAndRestore)
        saveLive(lir);

    pushArg(array);
    pushArg(ImmGCPtr(templateObject));
    pushArg(temp1);
    pushArg(temp0);

    bool result = callVM(InitRestParameterInfo, lir);

    if (saveAndRestore) {
        storeResultTo(resultreg);
        restoreLive(lir);
    }

    return result;
}

bool
CodeGenerator::visitRest(LRest *lir)
{
    Register numActuals = ToRegister(lir->numActuals());
    Register temp0 = ToRegister(lir->getTemp(0));
    Register temp1 = ToRegister(lir->getTemp(1));
    Register temp2 = ToRegister(lir->getTemp(2));
    unsigned numFormals = lir->mir()->numFormals();
    JSObject *templateObject = lir->mir()->templateObject();

    Label joinAlloc, failAlloc;
    masm.createGCObject(temp2, temp0, templateObject, gc::DefaultHeap, &failAlloc);
    masm.jump(&joinAlloc);
    {
        masm.bind(&failAlloc);
        masm.movePtr(ImmPtr(nullptr), temp2);
    }
    masm.bind(&joinAlloc);

    return emitRest(lir, temp2, numActuals, temp0, temp1, numFormals, templateObject, false, ToRegister(lir->output()));
}




bool
CodeGenerator::visitRestPar(LRestPar *lir)
{
    Register numActuals = ToRegister(lir->numActuals());
    Register cx = ToRegister(lir->forkJoinContext());
    Register temp0 = ToRegister(lir->getTemp(0));
    Register temp1 = ToRegister(lir->getTemp(1));
    Register temp2 = ToRegister(lir->getTemp(2));
    unsigned numFormals = lir->mir()->numFormals();
    JSObject *templateObject = lir->mir()->templateObject();

    if (!emitAllocateGCThingPar(lir, temp2, cx, temp0, temp1, templateObject))
        return false;

    return emitRest(lir, temp2, numActuals, temp0, temp1, numFormals, templateObject, true, ToRegister(lir->output()));
}

bool
CodeGenerator::generateAsmJS(AsmJSFunctionLabels *labels)
{
    JitSpew(JitSpew_Codegen, "# Emitting asm.js code");

    
    sps_.disable();

    if (!omitOverRecursedCheck())
        labels->overflowThunk.emplace();

    GenerateAsmJSFunctionPrologue(masm, frameSize(), labels);

    if (!generateBody())
        return false;

    masm.bind(&returnLabel_);
    GenerateAsmJSFunctionEpilogue(masm, frameSize(), labels);

#if defined(JS_ION_PERF)
    
    gen->perfSpewer().noteEndInlineCode(masm);
#endif

    if (!generateOutOfLineCode())
        return false;

    masm.bind(&labels->end);

    
    
    
    
    
    
    
    
    JS_ASSERT(snapshots_.listSize() == 0);
    JS_ASSERT(snapshots_.RVATableSize() == 0);
    JS_ASSERT(recovers_.size() == 0);
    JS_ASSERT(bailouts_.empty());
    JS_ASSERT(graph.numConstants() == 0);
    JS_ASSERT(safepointIndices_.empty());
    JS_ASSERT(osiIndices_.empty());
    JS_ASSERT(cacheList_.empty());
    JS_ASSERT(safepoints_.size() == 0);
    return true;
}

bool
CodeGenerator::generate()
{
    JitSpew(JitSpew_Codegen, "# Emitting code for script %s:%d",
            gen->info().script()->filename(),
            gen->info().script()->lineno());

    
    
    InlineScriptTree *tree = gen->info().inlineScriptTree();
    jsbytecode *startPC = tree->script()->code();
    if (!addNativeToBytecodeEntry(BytecodeSite(tree, startPC)))
        return false;

    if (!snapshots_.init())
        return false;

    if (!safepoints_.init(gen->alloc(), graph.totalSlotCount()))
        return false;

#ifdef JS_TRACE_LOGGING
    if (!gen->compilingAsmJS() && gen->info().executionMode() == SequentialExecution) {
        if (!emitTracelogScriptStart())
            return false;
        if (!emitTracelogStartEvent(TraceLogger::IonMonkey))
            return false;
    }
#endif

    
    
    
    if (!generateArgumentsChecks())
        return false;

    if (frameClass_ != FrameSizeClass::None()) {
        deoptTable_ = gen->jitRuntime()->getBailoutTable(frameClass_);
        if (!deoptTable_)
            return false;
    }

#ifdef JS_TRACE_LOGGING
    Label skip;
    masm.jump(&skip);
#endif

    
    masm.flushBuffer();
    setSkipArgCheckEntryOffset(masm.size());

#ifdef JS_TRACE_LOGGING
    if (!gen->compilingAsmJS() && gen->info().executionMode() == SequentialExecution) {
        if (!emitTracelogScriptStart())
            return false;
        if (!emitTracelogStartEvent(TraceLogger::IonMonkey))
            return false;
    }
    masm.bind(&skip);
#endif

#ifdef DEBUG
    
    if (!generateArgumentsChecks( false))
        return false;
#endif

    if (!generatePrologue())
        return false;

    
    if (!addNativeToBytecodeEntry(BytecodeSite(tree, startPC)))
        return false;

    if (!generateBody())
        return false;

    
    if (!addNativeToBytecodeEntry(BytecodeSite(tree, startPC)))
        return false;

    if (!generateEpilogue())
        return false;

    
    if (!addNativeToBytecodeEntry(BytecodeSite(tree, startPC)))
        return false;

    if (!generateInvalidateEpilogue())
        return false;
#if defined(JS_ION_PERF)
    
    perfSpewer_.noteEndInlineCode(masm);
#endif

    
    
    if (!generateOutOfLineCode())
        return false;

    
    if (!addNativeToBytecodeEntry(BytecodeSite(tree, startPC)))
        return false;

    
    dumpNativeToBytecodeEntries();

    return !masm.oom();
}

struct AutoDiscardIonCode
{
    JSContext *cx;
    types::RecompileInfo *recompileInfo;
    IonScript *ionScript;
    bool keep;

    AutoDiscardIonCode(JSContext *cx, types::RecompileInfo *recompileInfo)
      : cx(cx), recompileInfo(recompileInfo), ionScript(nullptr), keep(false) {}

    ~AutoDiscardIonCode() {
        if (keep)
            return;

        
        
        if (ionScript)
            js_free(ionScript);

        recompileInfo->compilerOutput(cx->zone()->types)->invalidate();
    }

    void keepIonCode() {
        keep = true;
    }
};

bool
CodeGenerator::link(JSContext *cx, types::CompilerConstraintList *constraints)
{
    RootedScript script(cx, gen->info().script());
    ExecutionMode executionMode = gen->info().executionMode();
    OptimizationLevel optimizationLevel = gen->optimizationInfo().level();

    JS_ASSERT_IF(HasIonScript(script, executionMode), executionMode == SequentialExecution);

    
    
    if (HasIonScript(script, executionMode)) {
        JS_ASSERT(GetIonScript(script, executionMode)->isRecompiling());
        
        
        if (!Invalidate(cx, script, SequentialExecution,
                         false,  false))
        {
            return false;
        }
    }

    if (scriptCounts_ && !script->hasScriptCounts() && !script->initScriptCounts(cx))
        return false;

    
    
    types::RecompileInfo recompileInfo;
    if (!types::FinishCompilation(cx, script, executionMode, constraints, &recompileInfo))
        return true;

    uint32_t scriptFrameSize = frameClass_ == FrameSizeClass::None()
                           ? frameDepth_
                           : FrameSizeClass::FromDepth(frameDepth_).frameSize();

    
    encodeSafepoints();

    
    
    CallTargetVector callTargets(alloc());
    if (executionMode == ParallelExecution)
        AddPossibleCallees(cx, graph.mir(), callTargets);

    AutoDiscardIonCode discardIonCode(cx, &recompileInfo);

    IonScript *ionScript =
      IonScript::New(cx, recompileInfo,
                     graph.totalSlotCount(), scriptFrameSize,
                     snapshots_.listSize(), snapshots_.RVATableSize(),
                     recovers_.size(), bailouts_.length(), graph.numConstants(),
                     safepointIndices_.length(), osiIndices_.length(),
                     cacheList_.length(), runtimeData_.length(),
                     safepoints_.size(), callTargets.length(),
                     patchableBackedges_.length(), optimizationLevel);
    if (!ionScript)
        return false;
    discardIonCode.ionScript = ionScript;

    
    
    
    
    JSRuntime::AutoLockForInterrupt lock(cx->runtime());

    
    
    cx->runtime()->jitRuntime()->ensureIonCodeAccessible(cx->runtime());

    
    
    
    
    
    
    
    Linker linker(masm);
    AutoFlushICache afc("IonLink");
    JitCode *code = (executionMode == SequentialExecution)
                    ? linker.newCodeForIonScript(cx)
                    : linker.newCode<CanGC>(cx, ION_CODE);
    if (!code)
        return false;

    
    if (isNativeToBytecodeMapEnabled()) {
        
        if (!generateCompactNativeToBytecodeMap(cx, code))
            return false;

        uint8_t *ionTableAddr = ((uint8_t *) nativeToBytecodeMap_) + nativeToBytecodeTableOffset_;
        JitcodeIonTable *ionTable = (JitcodeIonTable *) ionTableAddr;

        
        JitcodeGlobalEntry::IonEntry entry;
        if (!ionTable->makeIonEntry(cx, code, nativeToBytecodeScriptListLength_,
                                    nativeToBytecodeScriptList_, entry))
        {
            return false;
        }

        
        JitcodeGlobalTable *globalTable = cx->runtime()->jitRuntime()->getJitcodeGlobalTable();
        if (!globalTable->addEntry(entry)) {
            
            entry.destroy();
            return false;
        }

        
        code->setHasBytecodeMap();
    }

    if (cx->runtime()->spsProfiler.enabled()) {
        const char *filename = script->filename();
        if (filename == nullptr)
            filename = "<unknown>";
        unsigned len = strlen(filename) + 50;
        char *buf = js_pod_malloc<char>(len);
        if (!buf)
            return false;
        JS_snprintf(buf, len, "Ion compiled %s:%d", filename, (int) script->lineno());
        cx->runtime()->spsProfiler.markEvent(buf);
        js_free(buf);
    }

    ionScript->setMethod(code);
    ionScript->setSkipArgCheckEntryOffset(getSkipArgCheckEntryOffset());

    
    if (sps_.enabled())
        ionScript->setHasSPSInstrumentation();

    SetIonScript(script, executionMode, ionScript);

    
    
    
    if (!callTargets.empty())
        ionScript->setHasUncompiledCallTarget();

    invalidateEpilogueData_.fixup(&masm);
    Assembler::PatchDataWithValueCheck(CodeLocationLabel(code, invalidateEpilogueData_),
                                       ImmPtr(ionScript),
                                       ImmPtr((void*)-1));

    JitSpew(JitSpew_Codegen, "Created IonScript %p (raw %p)",
            (void *) ionScript, (void *) code->raw());

    ionScript->setInvalidationEpilogueDataOffset(invalidateEpilogueData_.offset());
    ionScript->setOsrPc(gen->info().osrPc());
    ionScript->setOsrEntryOffset(getOsrEntryOffset());
    ptrdiff_t real_invalidate = masm.actualOffset(invalidate_.offset());
    ionScript->setInvalidationEpilogueOffset(real_invalidate);

    ionScript->setDeoptTable(deoptTable_);

#if defined(JS_ION_PERF)
    if (PerfEnabled())
        perfSpewer_.writeProfile(script, code, masm);
#endif

    for (size_t i = 0; i < ionScriptLabels_.length(); i++) {
        ionScriptLabels_[i].fixup(&masm);
        Assembler::PatchDataWithValueCheck(CodeLocationLabel(code, ionScriptLabels_[i]),
                                           ImmPtr(ionScript),
                                           ImmPtr((void*)-1));
    }

    
    if (runtimeData_.length())
        ionScript->copyRuntimeData(&runtimeData_[0]);
    if (cacheList_.length())
        ionScript->copyCacheEntries(&cacheList_[0], masm);

    
    if (safepointIndices_.length())
        ionScript->copySafepointIndices(&safepointIndices_[0], masm);
    if (safepoints_.size())
        ionScript->copySafepoints(&safepoints_);

    
    if (bailouts_.length())
        ionScript->copyBailoutTable(&bailouts_[0]);
    if (osiIndices_.length())
        ionScript->copyOsiIndices(&osiIndices_[0], masm);
    if (snapshots_.listSize())
        ionScript->copySnapshots(&snapshots_);
    MOZ_ASSERT_IF(snapshots_.listSize(), recovers_.size());
    if (recovers_.size())
        ionScript->copyRecovers(&recovers_);
    if (graph.numConstants())
        ionScript->copyConstants(graph.constantPool());
    if (callTargets.length() > 0)
        ionScript->copyCallTargetEntries(callTargets.begin());
    if (patchableBackedges_.length() > 0)
        ionScript->copyPatchableBackedges(cx, code, patchableBackedges_.begin(), masm);

#ifdef JS_TRACE_LOGGING
    TraceLogger *logger = TraceLoggerForMainThread(cx->runtime());
    for (uint32_t i = 0; i < patchableTraceLoggers_.length(); i++) {
        patchableTraceLoggers_[i].fixup(&masm);
        Assembler::PatchDataWithValueCheck(CodeLocationLabel(code, patchableTraceLoggers_[i]),
                                           ImmPtr(logger),
                                           ImmPtr(nullptr));
    }
    uint32_t scriptId = TraceLogCreateTextId(logger, script);
    for (uint32_t i = 0; i < patchableTLScripts_.length(); i++) {
        patchableTLScripts_[i].fixup(&masm);
        Assembler::PatchDataWithValueCheck(CodeLocationLabel(code, patchableTLScripts_[i]),
                                           ImmPtr((void *) uintptr_t(scriptId)),
                                           ImmPtr((void *)0));
    }
#endif

    switch (executionMode) {
      case SequentialExecution:
        
        
        
        if (cx->zone()->needsIncrementalBarrier())
            ionScript->toggleBarriers(true);
        break;
      case ParallelExecution:
        
        
        break;
      default:
        MOZ_CRASH("No such execution mode");
    }

    
    if (IonScriptCounts *counts = extractScriptCounts())
        script->addIonCounts(counts);

    
    discardIonCode.keepIonCode();

    return true;
}


class OutOfLineUnboxFloatingPoint : public OutOfLineCodeBase<CodeGenerator>
{
    LUnboxFloatingPoint *unboxFloatingPoint_;

  public:
    explicit OutOfLineUnboxFloatingPoint(LUnboxFloatingPoint *unboxFloatingPoint)
      : unboxFloatingPoint_(unboxFloatingPoint)
    { }

    bool accept(CodeGenerator *codegen) {
        return codegen->visitOutOfLineUnboxFloatingPoint(this);
    }

    LUnboxFloatingPoint *unboxFloatingPoint() const {
        return unboxFloatingPoint_;
    }
};

bool
CodeGenerator::visitUnboxFloatingPoint(LUnboxFloatingPoint *lir)
{
    const ValueOperand box = ToValue(lir, LUnboxFloatingPoint::Input);
    const LDefinition *result = lir->output();

    
    
    OutOfLineUnboxFloatingPoint *ool = new(alloc()) OutOfLineUnboxFloatingPoint(lir);
    if (!addOutOfLineCode(ool, lir->mir()))
        return false;

    FloatRegister resultReg = ToFloatRegister(result);
    masm.branchTestDouble(Assembler::NotEqual, box, ool->entry());
    masm.unboxDouble(box, resultReg);
    if (lir->type() == MIRType_Float32)
        masm.convertDoubleToFloat32(resultReg, resultReg);
    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitOutOfLineUnboxFloatingPoint(OutOfLineUnboxFloatingPoint *ool)
{
    LUnboxFloatingPoint *ins = ool->unboxFloatingPoint();
    const ValueOperand value = ToValue(ins, LUnboxFloatingPoint::Input);

    if (ins->mir()->fallible()) {
        Label bail;
        masm.branchTestInt32(Assembler::NotEqual, value, &bail);
        if (!bailoutFrom(&bail, ins->snapshot()))
            return false;
    }
    masm.int32ValueToFloatingPoint(value, ToFloatRegister(ins->output()), ins->type());
    masm.jump(ool->rejoin());
    return true;
}

typedef bool (*GetPropertyFn)(JSContext *, HandleValue, HandlePropertyName, MutableHandleValue);
static const VMFunction GetPropertyInfo = FunctionInfo<GetPropertyFn>(GetProperty);
static const VMFunction CallPropertyInfo = FunctionInfo<GetPropertyFn>(CallProperty);

bool
CodeGenerator::visitCallGetProperty(LCallGetProperty *lir)
{
    pushArg(ImmGCPtr(lir->mir()->name()));
    pushArg(ToValue(lir, LCallGetProperty::Value));

    if (lir->mir()->callprop())
        return callVM(CallPropertyInfo, lir);
    return callVM(GetPropertyInfo, lir);
}

typedef bool (*GetOrCallElementFn)(JSContext *, MutableHandleValue, HandleValue, MutableHandleValue);
static const VMFunction GetElementInfo = FunctionInfo<GetOrCallElementFn>(js::GetElement);
static const VMFunction CallElementInfo = FunctionInfo<GetOrCallElementFn>(js::CallElement);

bool
CodeGenerator::visitCallGetElement(LCallGetElement *lir)
{
    pushArg(ToValue(lir, LCallGetElement::RhsInput));
    pushArg(ToValue(lir, LCallGetElement::LhsInput));

    JSOp op = JSOp(*lir->mir()->resumePoint()->pc());

    if (op == JSOP_GETELEM) {
        return callVM(GetElementInfo, lir);
    } else {
        JS_ASSERT(op == JSOP_CALLELEM);
        return callVM(CallElementInfo, lir);
    }
}

typedef bool (*SetObjectElementFn)(JSContext *, HandleObject, HandleValue, HandleValue,
                                   bool strict);
typedef bool (*SetElementParFn)(ForkJoinContext *, HandleObject, HandleValue, HandleValue, bool);
static const VMFunctionsModal SetObjectElementInfo = VMFunctionsModal(
    FunctionInfo<SetObjectElementFn>(SetObjectElement),
    FunctionInfo<SetElementParFn>(SetElementPar));

bool
CodeGenerator::visitCallSetElement(LCallSetElement *lir)
{
    pushArg(Imm32(current->mir()->strict()));
    pushArg(ToValue(lir, LCallSetElement::Value));
    pushArg(ToValue(lir, LCallSetElement::Index));
    pushArg(ToRegister(lir->getOperand(0)));
    return callVM(SetObjectElementInfo, lir);
}

typedef bool (*InitElementArrayFn)(JSContext *, jsbytecode *, HandleObject, uint32_t, HandleValue);
static const VMFunction InitElementArrayInfo = FunctionInfo<InitElementArrayFn>(js::InitElementArray);

bool
CodeGenerator::visitCallInitElementArray(LCallInitElementArray *lir)
{
    pushArg(ToValue(lir, LCallInitElementArray::Value));
    pushArg(Imm32(lir->mir()->index()));
    pushArg(ToRegister(lir->getOperand(0)));
    pushArg(ImmPtr(lir->mir()->resumePoint()->pc()));
    return callVM(InitElementArrayInfo, lir);
}

bool
CodeGenerator::visitLoadFixedSlotV(LLoadFixedSlotV *ins)
{
    const Register obj = ToRegister(ins->getOperand(0));
    size_t slot = ins->mir()->slot();
    ValueOperand result = GetValueOutput(ins);

    masm.loadValue(Address(obj, JSObject::getFixedSlotOffset(slot)), result);
    return true;
}

bool
CodeGenerator::visitLoadFixedSlotT(LLoadFixedSlotT *ins)
{
    const Register obj = ToRegister(ins->getOperand(0));
    size_t slot = ins->mir()->slot();
    AnyRegister result = ToAnyRegister(ins->getDef(0));
    MIRType type = ins->mir()->type();

    masm.loadUnboxedValue(Address(obj, JSObject::getFixedSlotOffset(slot)), type, result);

    return true;
}

bool
CodeGenerator::visitStoreFixedSlotV(LStoreFixedSlotV *ins)
{
    const Register obj = ToRegister(ins->getOperand(0));
    size_t slot = ins->mir()->slot();

    const ValueOperand value = ToValue(ins, LStoreFixedSlotV::Value);

    Address address(obj, JSObject::getFixedSlotOffset(slot));
    if (ins->mir()->needsBarrier())
        emitPreBarrier(address);

    masm.storeValue(value, address);

    return true;
}

bool
CodeGenerator::visitStoreFixedSlotT(LStoreFixedSlotT *ins)
{
    const Register obj = ToRegister(ins->getOperand(0));
    size_t slot = ins->mir()->slot();

    const LAllocation *value = ins->value();
    MIRType valueType = ins->mir()->value()->type();

    ConstantOrRegister nvalue = value->isConstant()
                              ? ConstantOrRegister(*value->toConstant())
                              : TypedOrValueRegister(valueType, ToAnyRegister(value));

    Address address(obj, JSObject::getFixedSlotOffset(slot));
    if (ins->mir()->needsBarrier())
        emitPreBarrier(address);

    masm.storeConstantOrRegister(nvalue, address);

    return true;
}

bool
CodeGenerator::visitCallsiteCloneCache(LCallsiteCloneCache *ins)
{
    const MCallsiteCloneCache *mir = ins->mir();
    Register callee = ToRegister(ins->callee());
    Register output = ToRegister(ins->output());

    CallsiteCloneIC cache(callee, mir->block()->info().script(), mir->callPc(), output);
    cache.setProfilerLeavePC(mir->profilerLeavePc());
    return addCache(ins, allocateCache(cache));
}

typedef JSObject *(*CallsiteCloneICFn)(JSContext *, size_t, HandleObject);
const VMFunction CallsiteCloneIC::UpdateInfo =
    FunctionInfo<CallsiteCloneICFn>(CallsiteCloneIC::update);

bool
CodeGenerator::visitCallsiteCloneIC(OutOfLineUpdateCache *ool, DataPtr<CallsiteCloneIC> &ic)
{
    LInstruction *lir = ool->lir();
    saveLive(lir);

    pushArg(ic->calleeReg());
    pushArg(Imm32(ool->getCacheIndex()));
    if (!callVM(CallsiteCloneIC::UpdateInfo, lir))
        return false;
    StoreRegisterTo(ic->outputReg()).generate(this);
    restoreLiveIgnore(lir, StoreRegisterTo(ic->outputReg()).clobbered());

    masm.jump(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitGetNameCache(LGetNameCache *ins)
{
    RegisterSet liveRegs = ins->safepoint()->liveRegs();
    Register scopeChain = ToRegister(ins->scopeObj());
    TypedOrValueRegister output(GetValueOutput(ins));
    bool isTypeOf = ins->mir()->accessKind() != MGetNameCache::NAME;

    NameIC cache(liveRegs, isTypeOf, scopeChain, ins->mir()->name(), output);
    cache.setProfilerLeavePC(ins->mir()->profilerLeavePc());
    return addCache(ins, allocateCache(cache));
}

typedef bool (*NameICFn)(JSContext *, size_t, HandleObject, MutableHandleValue);
const VMFunction NameIC::UpdateInfo = FunctionInfo<NameICFn>(NameIC::update);

bool
CodeGenerator::visitNameIC(OutOfLineUpdateCache *ool, DataPtr<NameIC> &ic)
{
    LInstruction *lir = ool->lir();
    saveLive(lir);

    pushArg(ic->scopeChainReg());
    pushArg(Imm32(ool->getCacheIndex()));
    if (!callVM(NameIC::UpdateInfo, lir))
        return false;
    StoreValueTo(ic->outputReg()).generate(this);
    restoreLiveIgnore(lir, StoreValueTo(ic->outputReg()).clobbered());

    masm.jump(ool->rejoin());
    return true;
}

bool
CodeGenerator::addGetPropertyCache(LInstruction *ins, RegisterSet liveRegs, Register objReg,
                                   PropertyName *name, TypedOrValueRegister output,
                                   bool monitoredResult, jsbytecode *profilerLeavePc)
{
    switch (gen->info().executionMode()) {
      case SequentialExecution: {
        GetPropertyIC cache(liveRegs, objReg, name, output, monitoredResult);
        cache.setProfilerLeavePC(profilerLeavePc);
        return addCache(ins, allocateCache(cache));
      }
      case ParallelExecution: {
        GetPropertyParIC cache(objReg, name, output);
        cache.setProfilerLeavePC(profilerLeavePc);
        return addCache(ins, allocateCache(cache));
      }
      default:
        MOZ_CRASH("Bad execution mode");
    }
}

bool
CodeGenerator::addSetPropertyCache(LInstruction *ins, RegisterSet liveRegs, Register objReg,
                                   PropertyName *name, ConstantOrRegister value, bool strict,
                                   bool needsTypeBarrier, jsbytecode *profilerLeavePc)
{
    switch (gen->info().executionMode()) {
      case SequentialExecution: {
          SetPropertyIC cache(liveRegs, objReg, name, value, strict, needsTypeBarrier);
            cache.setProfilerLeavePC(profilerLeavePc);
          return addCache(ins, allocateCache(cache));
      }
      case ParallelExecution: {
          SetPropertyParIC cache(objReg, name, value, strict, needsTypeBarrier);
            cache.setProfilerLeavePC(profilerLeavePc);
          return addCache(ins, allocateCache(cache));
      }
      default:
        MOZ_CRASH("Bad execution mode");
    }
}

bool
CodeGenerator::addSetElementCache(LInstruction *ins, Register obj, Register unboxIndex,
                                  Register temp, FloatRegister tempDouble,
                                  FloatRegister tempFloat32, ValueOperand index,
                                  ConstantOrRegister value, bool strict, bool guardHoles,
                                  jsbytecode *profilerLeavePc)
{
    switch (gen->info().executionMode()) {
      case SequentialExecution: {
          SetElementIC cache(obj, unboxIndex, temp, tempDouble, tempFloat32, index, value, strict,
                           guardHoles);
        cache.setProfilerLeavePC(profilerLeavePc);
        return addCache(ins, allocateCache(cache));
      }
      case ParallelExecution: {
          SetElementParIC cache(obj, unboxIndex, temp, tempDouble, tempFloat32, index, value, strict,
                              guardHoles);
        cache.setProfilerLeavePC(profilerLeavePc);
        return addCache(ins, allocateCache(cache));
      }
      default:
        MOZ_CRASH("Bad execution mode");
    }
}

bool
CodeGenerator::visitGetPropertyCacheV(LGetPropertyCacheV *ins)
{
    RegisterSet liveRegs = ins->safepoint()->liveRegs();
    Register objReg = ToRegister(ins->getOperand(0));
    PropertyName *name = ins->mir()->name();
    bool monitoredResult = ins->mir()->monitoredResult();
    TypedOrValueRegister output = TypedOrValueRegister(GetValueOutput(ins));

    return addGetPropertyCache(ins, liveRegs, objReg, name, output, monitoredResult,
                               ins->mir()->profilerLeavePc());
}

bool
CodeGenerator::visitGetPropertyCacheT(LGetPropertyCacheT *ins)
{
    RegisterSet liveRegs = ins->safepoint()->liveRegs();
    Register objReg = ToRegister(ins->getOperand(0));
    PropertyName *name = ins->mir()->name();
    bool monitoredResult = ins->mir()->monitoredResult();
    TypedOrValueRegister output(ins->mir()->type(), ToAnyRegister(ins->getDef(0)));

    return addGetPropertyCache(ins, liveRegs, objReg, name, output, monitoredResult,
                               ins->mir()->profilerLeavePc());
}

typedef bool (*GetPropertyICFn)(JSContext *, size_t, HandleObject, MutableHandleValue);
const VMFunction GetPropertyIC::UpdateInfo =
    FunctionInfo<GetPropertyICFn>(GetPropertyIC::update);

bool
CodeGenerator::visitGetPropertyIC(OutOfLineUpdateCache *ool, DataPtr<GetPropertyIC> &ic)
{
    LInstruction *lir = ool->lir();

    if (ic->idempotent()) {
        size_t numLocs;
        CacheLocationList &cacheLocs = lir->mirRaw()->toGetPropertyCache()->location();
        size_t locationBase = addCacheLocations(cacheLocs, &numLocs);
        ic->setLocationInfo(locationBase, numLocs);
    }

    saveLive(lir);

    pushArg(ic->object());
    pushArg(Imm32(ool->getCacheIndex()));
    if (!callVM(GetPropertyIC::UpdateInfo, lir))
        return false;
    StoreValueTo(ic->output()).generate(this);
    restoreLiveIgnore(lir, StoreValueTo(ic->output()).clobbered());

    masm.jump(ool->rejoin());
    return true;
}

typedef bool (*GetPropertyParICFn)(ForkJoinContext *, size_t, HandleObject, MutableHandleValue);
const VMFunction GetPropertyParIC::UpdateInfo =
    FunctionInfo<GetPropertyParICFn>(GetPropertyParIC::update);

bool
CodeGenerator::visitGetPropertyParIC(OutOfLineUpdateCache *ool, DataPtr<GetPropertyParIC> &ic)
{
    LInstruction *lir = ool->lir();
    saveLive(lir);

    pushArg(ic->object());
    pushArg(Imm32(ool->getCacheIndex()));
    if (!callVM(GetPropertyParIC::UpdateInfo, lir))
        return false;
    StoreValueTo(ic->output()).generate(this);
    restoreLiveIgnore(lir, StoreValueTo(ic->output()).clobbered());

    masm.jump(ool->rejoin());
    return true;
}

bool
CodeGenerator::addGetElementCache(LInstruction *ins, Register obj, ConstantOrRegister index,
                                  TypedOrValueRegister output, bool monitoredResult,
                                  bool allowDoubleResult, jsbytecode *profilerLeavePc)
{
    switch (gen->info().executionMode()) {
      case SequentialExecution: {
        RegisterSet liveRegs = ins->safepoint()->liveRegs();
        GetElementIC cache(liveRegs, obj, index, output, monitoredResult, allowDoubleResult);
        cache.setProfilerLeavePC(profilerLeavePc);
        return addCache(ins, allocateCache(cache));
      }
      case ParallelExecution: {
        GetElementParIC cache(obj, index, output, monitoredResult, allowDoubleResult);
        cache.setProfilerLeavePC(profilerLeavePc);
        return addCache(ins, allocateCache(cache));
      }
      default:
        MOZ_CRASH("No such execution mode");
    }
}

bool
CodeGenerator::visitGetElementCacheV(LGetElementCacheV *ins)
{
    Register obj = ToRegister(ins->object());
    ConstantOrRegister index = TypedOrValueRegister(ToValue(ins, LGetElementCacheV::Index));
    TypedOrValueRegister output = TypedOrValueRegister(GetValueOutput(ins));
    const MGetElementCache *mir = ins->mir();

    return addGetElementCache(ins, obj, index, output, mir->monitoredResult(),
                              mir->allowDoubleResult(), mir->profilerLeavePc());
}

bool
CodeGenerator::visitGetElementCacheT(LGetElementCacheT *ins)
{
    Register obj = ToRegister(ins->object());
    ConstantOrRegister index = TypedOrValueRegister(MIRType_Int32, ToAnyRegister(ins->index()));
    TypedOrValueRegister output(ins->mir()->type(), ToAnyRegister(ins->output()));
    const MGetElementCache *mir = ins->mir();

    return addGetElementCache(ins, obj, index, output, mir->monitoredResult(),
                              mir->allowDoubleResult(), mir->profilerLeavePc());
}

typedef bool (*GetElementICFn)(JSContext *, size_t, HandleObject, HandleValue, MutableHandleValue);
const VMFunction GetElementIC::UpdateInfo =
    FunctionInfo<GetElementICFn>(GetElementIC::update);

bool
CodeGenerator::visitGetElementIC(OutOfLineUpdateCache *ool, DataPtr<GetElementIC> &ic)
{
    LInstruction *lir = ool->lir();
    saveLive(lir);

    pushArg(ic->index());
    pushArg(ic->object());
    pushArg(Imm32(ool->getCacheIndex()));
    if (!callVM(GetElementIC::UpdateInfo, lir))
        return false;
    StoreValueTo(ic->output()).generate(this);
    restoreLiveIgnore(lir, StoreValueTo(ic->output()).clobbered());

    masm.jump(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitSetElementCacheV(LSetElementCacheV *ins)
{
    Register obj = ToRegister(ins->object());
    Register unboxIndex = ToTempUnboxRegister(ins->tempToUnboxIndex());
    Register temp = ToRegister(ins->temp());
    FloatRegister tempDouble = ToFloatRegister(ins->tempDouble());
    FloatRegister tempFloat32 = ToFloatRegister(ins->tempFloat32());
    ValueOperand index = ToValue(ins, LSetElementCacheV::Index);
    ConstantOrRegister value = TypedOrValueRegister(ToValue(ins, LSetElementCacheV::Value));

    return addSetElementCache(ins, obj, unboxIndex, temp, tempDouble, tempFloat32, index, value,
                              ins->mir()->strict(), ins->mir()->guardHoles(),
                              ins->mir()->profilerLeavePc());
}

bool
CodeGenerator::visitSetElementCacheT(LSetElementCacheT *ins)
{
    Register obj = ToRegister(ins->object());
    Register unboxIndex = ToTempUnboxRegister(ins->tempToUnboxIndex());
    Register temp = ToRegister(ins->temp());
    FloatRegister tempDouble = ToFloatRegister(ins->tempDouble());
    FloatRegister tempFloat32 = ToFloatRegister(ins->tempFloat32());
    ValueOperand index = ToValue(ins, LSetElementCacheT::Index);
    ConstantOrRegister value;
    const LAllocation *tmp = ins->value();
    if (tmp->isConstant())
        value = *tmp->toConstant();
    else
        value = TypedOrValueRegister(ins->mir()->value()->type(), ToAnyRegister(tmp));

    return addSetElementCache(ins, obj, unboxIndex, temp, tempDouble, tempFloat32, index, value,
                              ins->mir()->strict(), ins->mir()->guardHoles(),
                              ins->mir()->profilerLeavePc());
}

typedef bool (*SetElementICFn)(JSContext *, size_t, HandleObject, HandleValue, HandleValue);
const VMFunction SetElementIC::UpdateInfo =
    FunctionInfo<SetElementICFn>(SetElementIC::update);

bool
CodeGenerator::visitSetElementIC(OutOfLineUpdateCache *ool, DataPtr<SetElementIC> &ic)
{
    LInstruction *lir = ool->lir();
    saveLive(lir);

    pushArg(ic->value());
    pushArg(ic->index());
    pushArg(ic->object());
    pushArg(Imm32(ool->getCacheIndex()));
    if (!callVM(SetElementIC::UpdateInfo, lir))
        return false;
    restoreLive(lir);

    masm.jump(ool->rejoin());
    return true;
}

typedef bool (*SetElementParICFn)(ForkJoinContext *, size_t, HandleObject, HandleValue, HandleValue);
const VMFunction SetElementParIC::UpdateInfo =
    FunctionInfo<SetElementParICFn>(SetElementParIC::update);

bool
CodeGenerator::visitSetElementParIC(OutOfLineUpdateCache *ool, DataPtr<SetElementParIC> &ic)
{
    LInstruction *lir = ool->lir();
    saveLive(lir);

    pushArg(ic->value());
    pushArg(ic->index());
    pushArg(ic->object());
    pushArg(Imm32(ool->getCacheIndex()));
    if (!callVM(SetElementParIC::UpdateInfo, lir))
        return false;
    restoreLive(lir);

    masm.jump(ool->rejoin());
    return true;
}

typedef bool (*GetElementParICFn)(ForkJoinContext *, size_t, HandleObject, HandleValue,
                                  MutableHandleValue);
const VMFunction GetElementParIC::UpdateInfo =
    FunctionInfo<GetElementParICFn>(GetElementParIC::update);

bool
CodeGenerator::visitGetElementParIC(OutOfLineUpdateCache *ool, DataPtr<GetElementParIC> &ic)
{
    LInstruction *lir = ool->lir();
    saveLive(lir);

    pushArg(ic->index());
    pushArg(ic->object());
    pushArg(Imm32(ool->getCacheIndex()));
    if (!callVM(GetElementParIC::UpdateInfo, lir))
        return false;
    StoreValueTo(ic->output()).generate(this);
    restoreLiveIgnore(lir, StoreValueTo(ic->output()).clobbered());

    masm.jump(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitBindNameCache(LBindNameCache *ins)
{
    Register scopeChain = ToRegister(ins->scopeChain());
    Register output = ToRegister(ins->output());
    BindNameIC cache(scopeChain, ins->mir()->name(), output);
    cache.setProfilerLeavePC(ins->mir()->profilerLeavePc());

    return addCache(ins, allocateCache(cache));
}

typedef JSObject *(*BindNameICFn)(JSContext *, size_t, HandleObject);
const VMFunction BindNameIC::UpdateInfo =
    FunctionInfo<BindNameICFn>(BindNameIC::update);

bool
CodeGenerator::visitBindNameIC(OutOfLineUpdateCache *ool, DataPtr<BindNameIC> &ic)
{
    LInstruction *lir = ool->lir();
    saveLive(lir);

    pushArg(ic->scopeChainReg());
    pushArg(Imm32(ool->getCacheIndex()));
    if (!callVM(BindNameIC::UpdateInfo, lir))
        return false;
    StoreRegisterTo(ic->outputReg()).generate(this);
    restoreLiveIgnore(lir, StoreRegisterTo(ic->outputReg()).clobbered());

    masm.jump(ool->rejoin());
    return true;
}

typedef bool (*SetPropertyFn)(JSContext *, HandleObject,
                              HandlePropertyName, const HandleValue, bool, jsbytecode *);
typedef bool (*SetPropertyParFn)(ForkJoinContext *, HandleObject,
                                 HandlePropertyName, const HandleValue, bool, jsbytecode *);
static const VMFunctionsModal SetPropertyInfo = VMFunctionsModal(
    FunctionInfo<SetPropertyFn>(SetProperty),
    FunctionInfo<SetPropertyParFn>(SetPropertyPar));

bool
CodeGenerator::visitCallSetProperty(LCallSetProperty *ins)
{
    ConstantOrRegister value = TypedOrValueRegister(ToValue(ins, LCallSetProperty::Value));

    const Register objReg = ToRegister(ins->getOperand(0));

    pushArg(ImmPtr(ins->mir()->resumePoint()->pc()));
    pushArg(Imm32(ins->mir()->strict()));

    pushArg(value);
    pushArg(ImmGCPtr(ins->mir()->name()));
    pushArg(objReg);

    return callVM(SetPropertyInfo, ins);
}

typedef bool (*DeletePropertyFn)(JSContext *, HandleValue, HandlePropertyName, bool *);
static const VMFunction DeletePropertyStrictInfo =
    FunctionInfo<DeletePropertyFn>(DeleteProperty<true>);
static const VMFunction DeletePropertyNonStrictInfo =
    FunctionInfo<DeletePropertyFn>(DeleteProperty<false>);

bool
CodeGenerator::visitCallDeleteProperty(LCallDeleteProperty *lir)
{
    pushArg(ImmGCPtr(lir->mir()->name()));
    pushArg(ToValue(lir, LCallDeleteProperty::Value));

    if (lir->mir()->block()->info().script()->strict())
        return callVM(DeletePropertyStrictInfo, lir);

    return callVM(DeletePropertyNonStrictInfo, lir);
}

typedef bool (*DeleteElementFn)(JSContext *, HandleValue, HandleValue, bool *);
static const VMFunction DeleteElementStrictInfo =
    FunctionInfo<DeleteElementFn>(DeleteElement<true>);
static const VMFunction DeleteElementNonStrictInfo =
    FunctionInfo<DeleteElementFn>(DeleteElement<false>);

bool
CodeGenerator::visitCallDeleteElement(LCallDeleteElement *lir)
{
    pushArg(ToValue(lir, LCallDeleteElement::Index));
    pushArg(ToValue(lir, LCallDeleteElement::Value));

    if (lir->mir()->block()->info().script()->strict())
        return callVM(DeleteElementStrictInfo, lir);

    return callVM(DeleteElementNonStrictInfo, lir);
}

bool
CodeGenerator::visitSetPropertyCacheV(LSetPropertyCacheV *ins)
{
    RegisterSet liveRegs = ins->safepoint()->liveRegs();
    Register objReg = ToRegister(ins->getOperand(0));
    ConstantOrRegister value = TypedOrValueRegister(ToValue(ins, LSetPropertyCacheV::Value));

    return addSetPropertyCache(ins, liveRegs, objReg, ins->mir()->name(), value,
                               ins->mir()->strict(), ins->mir()->needsTypeBarrier(),
                               ins->mir()->profilerLeavePc());
}

bool
CodeGenerator::visitSetPropertyCacheT(LSetPropertyCacheT *ins)
{
    RegisterSet liveRegs = ins->safepoint()->liveRegs();
    Register objReg = ToRegister(ins->getOperand(0));
    ConstantOrRegister value;

    if (ins->getOperand(1)->isConstant())
        value = ConstantOrRegister(*ins->getOperand(1)->toConstant());
    else
        value = TypedOrValueRegister(ins->valueType(), ToAnyRegister(ins->getOperand(1)));

    return addSetPropertyCache(ins, liveRegs, objReg, ins->mir()->name(), value,
                               ins->mir()->strict(), ins->mir()->needsTypeBarrier(),
                               ins->mir()->profilerLeavePc());
}

typedef bool (*SetPropertyICFn)(JSContext *, size_t, HandleObject, HandleValue);
const VMFunction SetPropertyIC::UpdateInfo =
    FunctionInfo<SetPropertyICFn>(SetPropertyIC::update);

bool
CodeGenerator::visitSetPropertyIC(OutOfLineUpdateCache *ool, DataPtr<SetPropertyIC> &ic)
{
    LInstruction *lir = ool->lir();
    saveLive(lir);

    pushArg(ic->value());
    pushArg(ic->object());
    pushArg(Imm32(ool->getCacheIndex()));
    if (!callVM(SetPropertyIC::UpdateInfo, lir))
        return false;
    restoreLive(lir);

    masm.jump(ool->rejoin());
    return true;
}

typedef bool (*SetPropertyParICFn)(ForkJoinContext *, size_t, HandleObject, HandleValue);
const VMFunction SetPropertyParIC::UpdateInfo =
    FunctionInfo<SetPropertyParICFn>(SetPropertyParIC::update);

bool
CodeGenerator::visitSetPropertyParIC(OutOfLineUpdateCache *ool, DataPtr<SetPropertyParIC> &ic)
{
    LInstruction *lir = ool->lir();
    saveLive(lir);

    pushArg(ic->value());
    pushArg(ic->object());
    pushArg(Imm32(ool->getCacheIndex()));
    if (!callVM(SetPropertyParIC::UpdateInfo, lir))
        return false;
    restoreLive(lir);

    masm.jump(ool->rejoin());
    return true;
}

typedef bool (*ThrowFn)(JSContext *, HandleValue);
static const VMFunction ThrowInfoCodeGen = FunctionInfo<ThrowFn>(js::Throw);

bool
CodeGenerator::visitThrow(LThrow *lir)
{
    pushArg(ToValue(lir, LThrow::Value));
    return callVM(ThrowInfoCodeGen, lir);
}

typedef bool (*BitNotFn)(JSContext *, HandleValue, int *p);
typedef bool (*BitNotParFn)(ForkJoinContext *, HandleValue, int32_t *);
static const VMFunctionsModal BitNotInfo = VMFunctionsModal(
    FunctionInfo<BitNotFn>(BitNot),
    FunctionInfo<BitNotParFn>(BitNotPar));

bool
CodeGenerator::visitBitNotV(LBitNotV *lir)
{
    pushArg(ToValue(lir, LBitNotV::Input));
    return callVM(BitNotInfo, lir);
}

typedef bool (*BitopFn)(JSContext *, HandleValue, HandleValue, int *p);
typedef bool (*BitopParFn)(ForkJoinContext *, HandleValue, HandleValue, int32_t *);
static const VMFunctionsModal BitAndInfo = VMFunctionsModal(
    FunctionInfo<BitopFn>(BitAnd),
    FunctionInfo<BitopParFn>(BitAndPar));
static const VMFunctionsModal BitOrInfo = VMFunctionsModal(
    FunctionInfo<BitopFn>(BitOr),
    FunctionInfo<BitopParFn>(BitOrPar));
static const VMFunctionsModal BitXorInfo = VMFunctionsModal(
    FunctionInfo<BitopFn>(BitXor),
    FunctionInfo<BitopParFn>(BitXorPar));
static const VMFunctionsModal BitLhsInfo = VMFunctionsModal(
    FunctionInfo<BitopFn>(BitLsh),
    FunctionInfo<BitopParFn>(BitLshPar));
static const VMFunctionsModal BitRhsInfo = VMFunctionsModal(
    FunctionInfo<BitopFn>(BitRsh),
    FunctionInfo<BitopParFn>(BitRshPar));

bool
CodeGenerator::visitBitOpV(LBitOpV *lir)
{
    pushArg(ToValue(lir, LBitOpV::RhsInput));
    pushArg(ToValue(lir, LBitOpV::LhsInput));

    switch (lir->jsop()) {
      case JSOP_BITAND:
        return callVM(BitAndInfo, lir);
      case JSOP_BITOR:
        return callVM(BitOrInfo, lir);
      case JSOP_BITXOR:
        return callVM(BitXorInfo, lir);
      case JSOP_LSH:
        return callVM(BitLhsInfo, lir);
      case JSOP_RSH:
        return callVM(BitRhsInfo, lir);
      default:
        break;
    }
    MOZ_CRASH("unexpected bitop");
}

class OutOfLineTypeOfV : public OutOfLineCodeBase<CodeGenerator>
{
    LTypeOfV *ins_;

  public:
    explicit OutOfLineTypeOfV(LTypeOfV *ins)
      : ins_(ins)
    { }

    bool accept(CodeGenerator *codegen) {
        return codegen->visitOutOfLineTypeOfV(this);
    }
    LTypeOfV *ins() const {
        return ins_;
    }
};

bool
CodeGenerator::visitTypeOfV(LTypeOfV *lir)
{
    const ValueOperand value = ToValue(lir, LTypeOfV::Input);
    Register output = ToRegister(lir->output());
    Register tag = masm.splitTagForTest(value);

    const JSAtomState &names = GetIonContext()->runtime->names();
    Label done;

    MDefinition *input = lir->mir()->input();

    bool testObject = input->mightBeType(MIRType_Object);
    bool testNumber = input->mightBeType(MIRType_Int32) || input->mightBeType(MIRType_Double);
    bool testBoolean = input->mightBeType(MIRType_Boolean);
    bool testUndefined = input->mightBeType(MIRType_Undefined);
    bool testNull = input->mightBeType(MIRType_Null);
    bool testString = input->mightBeType(MIRType_String);
    bool testSymbol = input->mightBeType(MIRType_Symbol);

    unsigned numTests = unsigned(testObject) + unsigned(testNumber) + unsigned(testBoolean) +
        unsigned(testUndefined) + unsigned(testNull) + unsigned(testString) + unsigned(testSymbol);

    MOZ_ASSERT_IF(!input->emptyResultTypeSet(), numTests > 0);

    OutOfLineTypeOfV *ool = nullptr;
    if (testObject) {
        if (lir->mir()->inputMaybeCallableOrEmulatesUndefined()) {
            
            
            ool = new(alloc()) OutOfLineTypeOfV(lir);
            if (!addOutOfLineCode(ool, lir->mir()))
                return false;

            if (numTests > 1)
                masm.branchTestObject(Assembler::Equal, tag, ool->entry());
            else
                masm.jump(ool->entry());
        } else {
            
            
            Label notObject;
            if (numTests > 1)
                masm.branchTestObject(Assembler::NotEqual, tag, &notObject);
            masm.movePtr(ImmGCPtr(names.object), output);
            if (numTests > 1)
                masm.jump(&done);
            masm.bind(&notObject);
        }
        numTests--;
    }

    if (testNumber) {
        Label notNumber;
        if (numTests > 1)
            masm.branchTestNumber(Assembler::NotEqual, tag, &notNumber);
        masm.movePtr(ImmGCPtr(names.number), output);
        if (numTests > 1)
            masm.jump(&done);
        masm.bind(&notNumber);
        numTests--;
    }

    if (testUndefined) {
        Label notUndefined;
        if (numTests > 1)
            masm.branchTestUndefined(Assembler::NotEqual, tag, &notUndefined);
        masm.movePtr(ImmGCPtr(names.undefined), output);
        if (numTests > 1)
            masm.jump(&done);
        masm.bind(&notUndefined);
        numTests--;
    }

    if (testNull) {
        Label notNull;
        if (numTests > 1)
            masm.branchTestNull(Assembler::NotEqual, tag, &notNull);
        masm.movePtr(ImmGCPtr(names.object), output);
        if (numTests > 1)
            masm.jump(&done);
        masm.bind(&notNull);
        numTests--;
    }

    if (testBoolean) {
        Label notBoolean;
        if (numTests > 1)
            masm.branchTestBoolean(Assembler::NotEqual, tag, &notBoolean);
        masm.movePtr(ImmGCPtr(names.boolean), output);
        if (numTests > 1)
            masm.jump(&done);
        masm.bind(&notBoolean);
        numTests--;
    }

    if (testString) {
        Label notString;
        if (numTests > 1)
            masm.branchTestString(Assembler::NotEqual, tag, &notString);
        masm.movePtr(ImmGCPtr(names.string), output);
        if (numTests > 1)
            masm.jump(&done);
        masm.bind(&notString);
        numTests--;
    }

    if (testSymbol) {
        Label notSymbol;
        if (numTests > 1)
            masm.branchTestSymbol(Assembler::NotEqual, tag, &notSymbol);
        masm.movePtr(ImmGCPtr(names.symbol), output);
        if (numTests > 1)
            masm.jump(&done);
        masm.bind(&notSymbol);
        numTests--;
    }

    MOZ_ASSERT(numTests == 0);

    masm.bind(&done);
    if (ool)
        masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitOutOfLineTypeOfV(OutOfLineTypeOfV *ool)
{
    LTypeOfV *ins = ool->ins();

    ValueOperand input = ToValue(ins, LTypeOfV::Input);
    Register temp = ToTempUnboxRegister(ins->tempToUnbox());
    Register output = ToRegister(ins->output());

    Register obj = masm.extractObject(input, temp);

    saveVolatile(output);
    masm.setupUnalignedABICall(2, output);
    masm.passABIArg(obj);
    masm.movePtr(ImmPtr(GetIonContext()->runtime), output);
    masm.passABIArg(output);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, js::TypeOfObjectOperation));
    masm.storeCallResult(output);
    restoreVolatile(output);

    masm.jump(ool->rejoin());
    return true;
}

typedef bool (*ToIdFn)(JSContext *, HandleScript, jsbytecode *, HandleValue, HandleValue,
                       MutableHandleValue);
static const VMFunction ToIdInfo = FunctionInfo<ToIdFn>(ToIdOperation);

bool
CodeGenerator::visitToIdV(LToIdV *lir)
{
    Label notInt32;
    FloatRegister temp = ToFloatRegister(lir->tempFloat());
    const ValueOperand out = ToOutValue(lir);
    ValueOperand index = ToValue(lir, LToIdV::Index);

    OutOfLineCode *ool = oolCallVM(ToIdInfo, lir,
                                   (ArgList(),
                                   ImmGCPtr(current->mir()->info().script()),
                                   ImmPtr(lir->mir()->resumePoint()->pc()),
                                   ToValue(lir, LToIdV::Object),
                                   ToValue(lir, LToIdV::Index)),
                                   StoreValueTo(out));

    Register tag = masm.splitTagForTest(index);

    masm.branchTestInt32(Assembler::NotEqual, tag, &notInt32);
    masm.moveValue(index, out);
    masm.jump(ool->rejoin());

    masm.bind(&notInt32);
    masm.branchTestDouble(Assembler::NotEqual, tag, ool->entry());
    masm.unboxDouble(index, temp);
    masm.convertDoubleToInt32(temp, out.scratchReg(), ool->entry(), true);
    masm.tagValue(JSVAL_TYPE_INT32, out.scratchReg(), out);

    masm.bind(ool->rejoin());
    return true;
}

template<typename T>
bool
CodeGenerator::emitLoadElementT(LLoadElementT *lir, const T &source)
{
    if (LIRGenerator::allowTypedElementHoleCheck()) {
        if (lir->mir()->needsHoleCheck()) {
            Label bail;
            masm.branchTestMagic(Assembler::Equal, source, &bail);
            if (!bailoutFrom(&bail, lir->snapshot()))
                return false;
        }
    } else {
        MOZ_ASSERT(!lir->mir()->needsHoleCheck());
    }

    AnyRegister output = ToAnyRegister(lir->output());
    if (lir->mir()->loadDoubles())
        masm.loadDouble(source, output.fpu());
    else
        masm.loadUnboxedValue(source, lir->mir()->type(), output);
    return true;
}

bool
CodeGenerator::visitLoadElementT(LLoadElementT *lir)
{
    Register elements = ToRegister(lir->elements());
    const LAllocation *index = lir->index();
    if (index->isConstant())
        return emitLoadElementT(lir, Address(elements, ToInt32(index) * sizeof(js::Value)));
    return emitLoadElementT(lir, BaseIndex(elements, ToRegister(index), TimesEight));
}

bool
CodeGenerator::visitLoadElementV(LLoadElementV *load)
{
    Register elements = ToRegister(load->elements());
    const ValueOperand out = ToOutValue(load);

    if (load->index()->isConstant())
        masm.loadValue(Address(elements, ToInt32(load->index()) * sizeof(Value)), out);
    else
        masm.loadValue(BaseIndex(elements, ToRegister(load->index()), TimesEight), out);

    if (load->mir()->needsHoleCheck()) {
        Label testMagic;
        masm.branchTestMagic(Assembler::Equal, out, &testMagic);
        if (!bailoutFrom(&testMagic, load->snapshot()))
            return false;
    }

    return true;
}

bool
CodeGenerator::visitLoadElementHole(LLoadElementHole *lir)
{
    Register elements = ToRegister(lir->elements());
    Register initLength = ToRegister(lir->initLength());
    const ValueOperand out = ToOutValue(lir);

    const MLoadElementHole *mir = lir->mir();

    
    
    Label undefined, done;
    if (lir->index()->isConstant()) {
        masm.branch32(Assembler::BelowOrEqual, initLength, Imm32(ToInt32(lir->index())), &undefined);
        masm.loadValue(Address(elements, ToInt32(lir->index()) * sizeof(Value)), out);
    } else {
        masm.branch32(Assembler::BelowOrEqual, initLength, ToRegister(lir->index()), &undefined);
        masm.loadValue(BaseIndex(elements, ToRegister(lir->index()), TimesEight), out);
    }

    
    
    if (lir->mir()->needsHoleCheck())
        masm.branchTestMagic(Assembler::NotEqual, out, &done);
    else
        masm.jump(&done);

    masm.bind(&undefined);

    if (mir->needsNegativeIntCheck()) {
        if (lir->index()->isConstant()) {
            if (ToInt32(lir->index()) < 0 && !bailout(lir->snapshot()))
                return false;
        } else {
            Label negative;
            masm.branch32(Assembler::LessThan, ToRegister(lir->index()), Imm32(0), &negative);
            if (!bailoutFrom(&negative, lir->snapshot()))
                return false;
        }
    }

    masm.moveValue(UndefinedValue(), out);
    masm.bind(&done);
    return true;
}

bool
CodeGenerator::visitLoadTypedArrayElement(LLoadTypedArrayElement *lir)
{
    Register elements = ToRegister(lir->elements());
    Register temp = lir->temp()->isBogusTemp() ? InvalidReg : ToRegister(lir->temp());
    AnyRegister out = ToAnyRegister(lir->output());

    Scalar::Type arrayType = lir->mir()->arrayType();
    int width = Scalar::byteSize(arrayType);

    Label fail;
    if (lir->index()->isConstant()) {
        Address source(elements, ToInt32(lir->index()) * width);
        masm.loadFromTypedArray(arrayType, source, out, temp, &fail);
    } else {
        BaseIndex source(elements, ToRegister(lir->index()), ScaleFromElemWidth(width));
        masm.loadFromTypedArray(arrayType, source, out, temp, &fail);
    }

    if (fail.used() && !bailoutFrom(&fail, lir->snapshot()))
        return false;

    return true;
}

bool
CodeGenerator::visitLoadTypedArrayElementHole(LLoadTypedArrayElementHole *lir)
{
    Register object = ToRegister(lir->object());
    const ValueOperand out = ToOutValue(lir);

    
    Register scratch = out.scratchReg();
    Int32Key key = ToInt32Key(lir->index());
    masm.unboxInt32(Address(object, TypedArrayObject::lengthOffset()), scratch);

    
    Label inbounds, done;
    masm.branchKey(Assembler::Above, scratch, key, &inbounds);
    masm.moveValue(UndefinedValue(), out);
    masm.jump(&done);

    
    masm.bind(&inbounds);
    masm.loadPtr(Address(object, TypedArrayObject::dataOffset()), scratch);

    Scalar::Type arrayType = lir->mir()->arrayType();
    int width = Scalar::byteSize(arrayType);

    Label fail;
    if (key.isConstant()) {
        Address source(scratch, key.constant() * width);
        masm.loadFromTypedArray(arrayType, source, out, lir->mir()->allowDouble(),
                                out.scratchReg(), &fail);
    } else {
        BaseIndex source(scratch, key.reg(), ScaleFromElemWidth(width));
        masm.loadFromTypedArray(arrayType, source, out, lir->mir()->allowDouble(),
                                out.scratchReg(), &fail);
    }

    if (fail.used() && !bailoutFrom(&fail, lir->snapshot()))
        return false;

    masm.bind(&done);
    return true;
}

template <typename T>
static inline void
StoreToTypedArray(MacroAssembler &masm, Scalar::Type arrayType, const LAllocation *value, const T &dest)
{
    if (arrayType == Scalar::Float32 || arrayType == Scalar::Float64) {
        masm.storeToTypedFloatArray(arrayType, ToFloatRegister(value), dest);
    } else {
        if (value->isConstant())
            masm.storeToTypedIntArray(arrayType, Imm32(ToInt32(value)), dest);
        else
            masm.storeToTypedIntArray(arrayType, ToRegister(value), dest);
    }
}

bool
CodeGenerator::visitStoreTypedArrayElement(LStoreTypedArrayElement *lir)
{
    Register elements = ToRegister(lir->elements());
    const LAllocation *value = lir->value();

    Scalar::Type arrayType = lir->mir()->arrayType();
    int width = Scalar::byteSize(arrayType);

    if (lir->index()->isConstant()) {
        Address dest(elements, ToInt32(lir->index()) * width);
        StoreToTypedArray(masm, arrayType, value, dest);
    } else {
        BaseIndex dest(elements, ToRegister(lir->index()), ScaleFromElemWidth(width));
        StoreToTypedArray(masm, arrayType, value, dest);
    }

    return true;
}

bool
CodeGenerator::visitStoreTypedArrayElementHole(LStoreTypedArrayElementHole *lir)
{
    Register elements = ToRegister(lir->elements());
    const LAllocation *value = lir->value();

    Scalar::Type arrayType = lir->mir()->arrayType();
    int width = Scalar::byteSize(arrayType);

    bool guardLength = true;
    if (lir->index()->isConstant() && lir->length()->isConstant()) {
        uint32_t idx = ToInt32(lir->index());
        uint32_t len = ToInt32(lir->length());
        if (idx >= len)
            return true;
        guardLength = false;
    }
    Label skip;
    if (lir->index()->isConstant()) {
        uint32_t idx = ToInt32(lir->index());
        if (guardLength)
            masm.branch32(Assembler::BelowOrEqual, ToOperand(lir->length()), Imm32(idx), &skip);
        Address dest(elements, idx * width);
        StoreToTypedArray(masm, arrayType, value, dest);
    } else {
        Register idxReg = ToRegister(lir->index());
        JS_ASSERT(guardLength);
        if (lir->length()->isConstant())
            masm.branch32(Assembler::AboveOrEqual, idxReg, Imm32(ToInt32(lir->length())), &skip);
        else
            masm.branch32(Assembler::BelowOrEqual, ToOperand(lir->length()), idxReg, &skip);
        BaseIndex dest(elements, ToRegister(lir->index()), ScaleFromElemWidth(width));
        StoreToTypedArray(masm, arrayType, value, dest);
    }
    if (guardLength)
        masm.bind(&skip);

    return true;
}

bool
CodeGenerator::visitClampIToUint8(LClampIToUint8 *lir)
{
    Register output = ToRegister(lir->output());
    JS_ASSERT(output == ToRegister(lir->input()));
    masm.clampIntToUint8(output);
    return true;
}

bool
CodeGenerator::visitClampDToUint8(LClampDToUint8 *lir)
{
    FloatRegister input = ToFloatRegister(lir->input());
    Register output = ToRegister(lir->output());
    masm.clampDoubleToUint8(input, output);
    return true;
}

bool
CodeGenerator::visitClampVToUint8(LClampVToUint8 *lir)
{
    ValueOperand operand = ToValue(lir, LClampVToUint8::Input);
    FloatRegister tempFloat = ToFloatRegister(lir->tempFloat());
    Register output = ToRegister(lir->output());
    MDefinition *input = lir->mir()->input();

    Label *stringEntry, *stringRejoin;
    if (input->mightBeType(MIRType_String)) {
        OutOfLineCode *oolString = oolCallVM(StringToNumberInfo, lir, (ArgList(), output),
                                             StoreFloatRegisterTo(tempFloat));
        if (!oolString)
            return false;
        stringEntry = oolString->entry();
        stringRejoin = oolString->rejoin();
    } else {
        stringEntry = nullptr;
        stringRejoin = nullptr;
    }

    Label fails;
    masm.clampValueToUint8(operand, input,
                           stringEntry, stringRejoin,
                           output, tempFloat, output, &fails);

    if (!bailoutFrom(&fails, lir->snapshot()))
        return false;

    return true;
}

typedef bool (*OperatorInFn)(JSContext *, HandleValue, HandleObject, bool *);
static const VMFunction OperatorInInfo = FunctionInfo<OperatorInFn>(OperatorIn);

bool
CodeGenerator::visitIn(LIn *ins)
{
    pushArg(ToRegister(ins->rhs()));
    pushArg(ToValue(ins, LIn::LHS));

    return callVM(OperatorInInfo, ins);
}

typedef bool (*OperatorInIFn)(JSContext *, uint32_t, HandleObject, bool *);
static const VMFunction OperatorInIInfo = FunctionInfo<OperatorInIFn>(OperatorInI);

bool
CodeGenerator::visitInArray(LInArray *lir)
{
    const MInArray *mir = lir->mir();
    Register elements = ToRegister(lir->elements());
    Register initLength = ToRegister(lir->initLength());
    Register output = ToRegister(lir->output());

    
    Label falseBranch, done, trueBranch;

    OutOfLineCode *ool = nullptr;
    Label* failedInitLength = &falseBranch;

    if (lir->index()->isConstant()) {
        int32_t index = ToInt32(lir->index());

        JS_ASSERT_IF(index < 0, mir->needsNegativeIntCheck());
        if (mir->needsNegativeIntCheck()) {
            ool = oolCallVM(OperatorInIInfo, lir,
                            (ArgList(), Imm32(index), ToRegister(lir->object())),
                            StoreRegisterTo(output));
            failedInitLength = ool->entry();
        }

        masm.branch32(Assembler::BelowOrEqual, initLength, Imm32(index), failedInitLength);
        if (mir->needsHoleCheck()) {
            Address address = Address(elements, index * sizeof(Value));
            masm.branchTestMagic(Assembler::Equal, address, &falseBranch);
        }
    } else {
        Label negativeIntCheck;
        Register index = ToRegister(lir->index());

        if (mir->needsNegativeIntCheck())
            failedInitLength = &negativeIntCheck;

        masm.branch32(Assembler::BelowOrEqual, initLength, index, failedInitLength);
        if (mir->needsHoleCheck()) {
            BaseIndex address = BaseIndex(elements, ToRegister(lir->index()), TimesEight);
            masm.branchTestMagic(Assembler::Equal, address, &falseBranch);
        }
        masm.jump(&trueBranch);

        if (mir->needsNegativeIntCheck()) {
            masm.bind(&negativeIntCheck);
            ool = oolCallVM(OperatorInIInfo, lir,
                            (ArgList(), index, ToRegister(lir->object())),
                            StoreRegisterTo(output));

            masm.branch32(Assembler::LessThan, index, Imm32(0), ool->entry());
            masm.jump(&falseBranch);
        }
    }

    masm.bind(&trueBranch);
    masm.move32(Imm32(1), output);
    masm.jump(&done);

    masm.bind(&falseBranch);
    masm.move32(Imm32(0), output);
    masm.bind(&done);

    if (ool)
        masm.bind(ool->rejoin());

    return true;
}

bool
CodeGenerator::visitInstanceOfO(LInstanceOfO *ins)
{
    return emitInstanceOf(ins, ins->mir()->prototypeObject());
}

bool
CodeGenerator::visitInstanceOfV(LInstanceOfV *ins)
{
    return emitInstanceOf(ins, ins->mir()->prototypeObject());
}


static bool
IsDelegateObject(JSContext *cx, HandleObject protoObj, HandleObject obj, bool *res)
{
    return IsDelegateOfObject(cx, protoObj, obj, res);
}

typedef bool (*IsDelegateObjectFn)(JSContext *, HandleObject, HandleObject, bool *);
static const VMFunction IsDelegateObjectInfo = FunctionInfo<IsDelegateObjectFn>(IsDelegateObject);

bool
CodeGenerator::emitInstanceOf(LInstruction *ins, JSObject *prototypeObject)
{
    
    

    Label done;
    Register output = ToRegister(ins->getDef(0));

    
    Register objReg;
    if (ins->isInstanceOfV()) {
        Label isObject;
        ValueOperand lhsValue = ToValue(ins, LInstanceOfV::LHS);
        masm.branchTestObject(Assembler::Equal, lhsValue, &isObject);
        masm.mov(ImmWord(0), output);
        masm.jump(&done);
        masm.bind(&isObject);
        objReg = masm.extractObject(lhsValue, output);
    } else {
        objReg = ToRegister(ins->toInstanceOfO()->lhs());
    }

    
    
    

    
    masm.loadObjProto(objReg, output);

    Label testLazy;
    {
        Label loopPrototypeChain;
        masm.bind(&loopPrototypeChain);

        
        Label notPrototypeObject;
        masm.branchPtr(Assembler::NotEqual, output, ImmGCPtr(prototypeObject), &notPrototypeObject);
        masm.mov(ImmWord(1), output);
        masm.jump(&done);
        masm.bind(&notPrototypeObject);

        JS_ASSERT(uintptr_t(TaggedProto::LazyProto) == 1);

        
        masm.branchPtr(Assembler::BelowOrEqual, output, ImmWord(1), &testLazy);

        
        masm.loadObjProto(output, output);

        masm.jump(&loopPrototypeChain);
    }

    
    
    
    
    

    OutOfLineCode *ool = oolCallVM(IsDelegateObjectInfo, ins,
                                   (ArgList(), ImmGCPtr(prototypeObject), objReg),
                                   StoreRegisterTo(output));

    
    Label regenerate, *lazyEntry;
    if (objReg != output) {
        lazyEntry = ool->entry();
    } else {
        masm.bind(&regenerate);
        lazyEntry = &regenerate;
        if (ins->isInstanceOfV()) {
            ValueOperand lhsValue = ToValue(ins, LInstanceOfV::LHS);
            objReg = masm.extractObject(lhsValue, output);
        } else {
            objReg = ToRegister(ins->toInstanceOfO()->lhs());
        }
        JS_ASSERT(objReg == output);
        masm.jump(ool->entry());
    }

    masm.bind(&testLazy);
    masm.branchPtr(Assembler::Equal, output, ImmWord(1), lazyEntry);

    masm.bind(&done);
    masm.bind(ool->rejoin());
    return true;
}

typedef bool (*HasInstanceFn)(JSContext *, HandleObject, HandleValue, bool *);
static const VMFunction HasInstanceInfo = FunctionInfo<HasInstanceFn>(js::HasInstance);

bool
CodeGenerator::visitCallInstanceOf(LCallInstanceOf *ins)
{
    ValueOperand lhs = ToValue(ins, LCallInstanceOf::LHS);
    Register rhs = ToRegister(ins->rhs());
    JS_ASSERT(ToRegister(ins->output()) == ReturnReg);

    pushArg(lhs);
    pushArg(rhs);
    return callVM(HasInstanceInfo, ins);
}

bool
CodeGenerator::visitGetDOMProperty(LGetDOMProperty *ins)
{
    const Register JSContextReg = ToRegister(ins->getJSContextReg());
    const Register ObjectReg = ToRegister(ins->getObjectReg());
    const Register PrivateReg = ToRegister(ins->getPrivReg());
    const Register ValueReg = ToRegister(ins->getValueReg());

    Label haveValue;
    if (ins->mir()->valueMayBeInSlot()) {
        size_t slot = ins->mir()->domMemberSlotIndex();
        
        
        
        if (slot < JSObject::MAX_FIXED_SLOTS) {
            masm.loadValue(Address(ObjectReg, JSObject::getFixedSlotOffset(slot)),
                           JSReturnOperand);
        } else {
            
            slot -= JSObject::MAX_FIXED_SLOTS;
            
            masm.loadPtr(Address(ObjectReg, JSObject::offsetOfSlots()),
                         PrivateReg);
            masm.loadValue(Address(PrivateReg, slot*sizeof(js::Value)),
                           JSReturnOperand);
        }
        masm.branchTestUndefined(Assembler::NotEqual, JSReturnOperand, &haveValue);
    }

    DebugOnly<uint32_t> initialStack = masm.framePushed();

    masm.checkStackAlignment();

    
    
    masm.Push(UndefinedValue());
    
    
    JS_STATIC_ASSERT(sizeof(JSJitGetterCallArgs) == sizeof(Value*));
    masm.movePtr(StackPointer, ValueReg);

    masm.Push(ObjectReg);

    
    masm.loadPrivate(Address(ObjectReg, JSObject::getFixedSlotOffset(0)), PrivateReg);

    
    masm.movePtr(StackPointer, ObjectReg);

    uint32_t safepointOffset;
    if (!masm.buildFakeExitFrame(JSContextReg, &safepointOffset))
        return false;
    masm.enterFakeExitFrame(IonDOMExitFrameLayout::GetterToken());

    if (!markSafepointAt(safepointOffset, ins))
        return false;

    masm.setupUnalignedABICall(4, JSContextReg);

    masm.loadJSContext(JSContextReg);

    masm.passABIArg(JSContextReg);
    masm.passABIArg(ObjectReg);
    masm.passABIArg(PrivateReg);
    masm.passABIArg(ValueReg);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ins->mir()->fun()));

    if (ins->mir()->isInfallible()) {
        masm.loadValue(Address(StackPointer, IonDOMExitFrameLayout::offsetOfResult()),
                       JSReturnOperand);
    } else {
        masm.branchIfFalseBool(ReturnReg, masm.exceptionLabel());

        masm.loadValue(Address(StackPointer, IonDOMExitFrameLayout::offsetOfResult()),
                       JSReturnOperand);
    }
    masm.adjustStack(IonDOMExitFrameLayout::Size());

    masm.bind(&haveValue);

    JS_ASSERT(masm.framePushed() == initialStack);
    return true;
}

bool
CodeGenerator::visitGetDOMMember(LGetDOMMember *ins)
{
    
    
    
    
    
    Register object = ToRegister(ins->object());
    size_t slot = ins->mir()->domMemberSlotIndex();
    ValueOperand result = GetValueOutput(ins);

    masm.loadValue(Address(object, JSObject::getFixedSlotOffset(slot)), result);
    return true;
}

bool
CodeGenerator::visitSetDOMProperty(LSetDOMProperty *ins)
{
    const Register JSContextReg = ToRegister(ins->getJSContextReg());
    const Register ObjectReg = ToRegister(ins->getObjectReg());
    const Register PrivateReg = ToRegister(ins->getPrivReg());
    const Register ValueReg = ToRegister(ins->getValueReg());

    DebugOnly<uint32_t> initialStack = masm.framePushed();

    masm.checkStackAlignment();

    
    ValueOperand argVal = ToValue(ins, LSetDOMProperty::Value);
    masm.Push(argVal);
    
    
    JS_STATIC_ASSERT(sizeof(JSJitSetterCallArgs) == sizeof(Value*));
    masm.movePtr(StackPointer, ValueReg);

    masm.Push(ObjectReg);

    
    masm.loadPrivate(Address(ObjectReg, JSObject::getFixedSlotOffset(0)), PrivateReg);

    
    masm.movePtr(StackPointer, ObjectReg);

    uint32_t safepointOffset;
    if (!masm.buildFakeExitFrame(JSContextReg, &safepointOffset))
        return false;
    masm.enterFakeExitFrame(IonDOMExitFrameLayout::SetterToken());

    if (!markSafepointAt(safepointOffset, ins))
        return false;

    masm.setupUnalignedABICall(4, JSContextReg);

    masm.loadJSContext(JSContextReg);

    masm.passABIArg(JSContextReg);
    masm.passABIArg(ObjectReg);
    masm.passABIArg(PrivateReg);
    masm.passABIArg(ValueReg);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ins->mir()->fun()));

    masm.branchIfFalseBool(ReturnReg, masm.exceptionLabel());

    masm.adjustStack(IonDOMExitFrameLayout::Size());

    JS_ASSERT(masm.framePushed() == initialStack);
    return true;
}

typedef bool(*SPSFn)(JSContext *, HandleScript);
static const VMFunction SPSEnterInfo = FunctionInfo<SPSFn>(SPSEnter);
static const VMFunction SPSExitInfo = FunctionInfo<SPSFn>(SPSExit);

bool
CodeGenerator::visitProfilerStackOp(LProfilerStackOp *lir)
{
    Register temp = ToRegister(lir->temp()->output());

    switch (lir->type()) {
        case MProfilerStackOp::Enter:
            if (gen->options.spsSlowAssertionsEnabled()) {
                saveLive(lir);
                pushArg(ImmGCPtr(lir->script()));
                if (!callVM(SPSEnterInfo, lir))
                    return false;
                restoreLive(lir);
                sps_.pushManual(lir->script(), masm, temp,  false);
                return true;
            }

            return sps_.push(lir->script(), masm, temp,  false);

        case MProfilerStackOp::Exit:
            if (gen->options.spsSlowAssertionsEnabled()) {
                saveLive(lir);
                pushArg(ImmGCPtr(lir->script()));
                
                
                
                sps_.skipNextReenter();
                if (!callVM(SPSExitInfo, lir))
                    return false;
                restoreLive(lir);
                return true;
            }

            sps_.pop(masm, temp,  false);
            return true;

        default:
            MOZ_CRASH("invalid LProfilerStackOp type");
    }
}

bool
CodeGenerator::visitIsCallable(LIsCallable *ins)
{
    Register object = ToRegister(ins->object());
    Register output = ToRegister(ins->output());

    masm.loadObjClass(object, output);

    
    Label notFunction, done, notCall;
    masm.branchPtr(Assembler::NotEqual, output, ImmPtr(&JSFunction::class_), &notFunction);
    masm.move32(Imm32(1), output);
    masm.jump(&done);

    masm.bind(&notFunction);
    masm.cmpPtrSet(Assembler::NonZero, Address(output, offsetof(js::Class, call)), ImmPtr(nullptr), output);
    masm.bind(&done);

    return true;
}

bool
CodeGenerator::visitIsObject(LIsObject *ins)
{
    Register output = ToRegister(ins->output());
    ValueOperand value = ToValue(ins, LIsObject::Input);
    masm.testObjectSet(Assembler::Equal, value, output);
    return true;
}

void
CodeGenerator::loadOutermostJSScript(Register reg)
{
    
    
    

    MIRGraph &graph = current->mir()->graph();
    MBasicBlock *entryBlock = graph.entryBlock();
    masm.movePtr(ImmGCPtr(entryBlock->info().script()), reg);
}

void
CodeGenerator::loadJSScriptForBlock(MBasicBlock *block, Register reg)
{
    
    

    JSScript *script = block->info().script();
    masm.movePtr(ImmGCPtr(script), reg);
}

bool
CodeGenerator::visitHaveSameClass(LHaveSameClass *ins)
{
    Register lhs = ToRegister(ins->lhs());
    Register rhs = ToRegister(ins->rhs());
    Register temp = ToRegister(ins->getTemp(0));
    Register output = ToRegister(ins->output());

    masm.loadObjClass(lhs, temp);
    masm.loadObjClass(rhs, output);
    masm.cmpPtrSet(Assembler::Equal, temp, output, output);

    return true;
}

bool
CodeGenerator::visitHasClass(LHasClass *ins)
{
    Register lhs = ToRegister(ins->lhs());
    Register output = ToRegister(ins->output());

    masm.loadObjClass(lhs, output);
    masm.cmpPtrSet(Assembler::Equal, output, ImmPtr(ins->mir()->getClass()), output);

    return true;
}

bool
CodeGenerator::visitAsmJSCall(LAsmJSCall *ins)
{
    MAsmJSCall *mir = ins->mir();

#if defined(JS_CODEGEN_ARM)
    if (!UseHardFpABI() && mir->callee().which() == MAsmJSCall::Callee::Builtin) {
        
        
        
        
        
        for (unsigned i = 0, e = ins->numOperands(); i < e; i++) {
            LAllocation *a = ins->getOperand(i);
            if (a->isFloatReg()) {
                FloatRegister fr = ToFloatRegister(a);
                if (fr.isDouble()) {
                    uint32_t srcId = fr.singleOverlay().id();
                    masm.ma_vxfer(fr, Register::FromCode(srcId), Register::FromCode(srcId + 1));
                } else {
                    uint32_t srcId = fr.id();
                    masm.ma_vxfer(fr, Register::FromCode(srcId));
                }
            }
        }
    }
#endif

    if (mir->spIncrement())
        masm.freeStack(mir->spIncrement());

    JS_ASSERT((sizeof(AsmJSFrame) + masm.framePushed()) % AsmJSStackAlignment == 0);

#ifdef DEBUG
    static_assert(AsmJSStackAlignment >= ABIStackAlignment,
                  "The asm.js stack alignment should subsume the ABI-required alignment");
    static_assert(AsmJSStackAlignment % ABIStackAlignment == 0,
                  "The asm.js stack alignment should subsume the ABI-required alignment");
    Label ok;
    masm.branchTestPtr(Assembler::Zero, StackPointer, Imm32(AsmJSStackAlignment - 1), &ok);
    masm.breakpoint();
    masm.bind(&ok);
#endif

    MAsmJSCall::Callee callee = mir->callee();
    switch (callee.which()) {
      case MAsmJSCall::Callee::Internal:
        masm.call(mir->desc(), callee.internal());
        break;
      case MAsmJSCall::Callee::Dynamic:
        masm.call(mir->desc(), ToRegister(ins->getOperand(mir->dynamicCalleeOperandIndex())));
        break;
      case MAsmJSCall::Callee::Builtin:
        masm.call(AsmJSImmPtr(callee.builtin()));
        break;
    }

    if (mir->spIncrement())
        masm.reserveStack(mir->spIncrement());

    postAsmJSCall(ins);
    return true;
}

bool
CodeGenerator::visitAsmJSParameter(LAsmJSParameter *lir)
{
    return true;
}

bool
CodeGenerator::visitAsmJSReturn(LAsmJSReturn *lir)
{
    
    if (current->mir() != *gen->graph().poBegin())
        masm.jump(&returnLabel_);
    return true;
}

bool
CodeGenerator::visitAsmJSVoidReturn(LAsmJSVoidReturn *lir)
{
    
    if (current->mir() != *gen->graph().poBegin())
        masm.jump(&returnLabel_);
    return true;
}

bool
CodeGenerator::emitAssertRangeI(const Range *r, Register input)
{
    
    if (r->hasInt32LowerBound() && r->lower() > INT32_MIN) {
        Label success;
        masm.branch32(Assembler::GreaterThanOrEqual, input, Imm32(r->lower()), &success);
        masm.assumeUnreachable("Integer input should be equal or higher than Lowerbound.");
        masm.bind(&success);
    }

    
    if (r->hasInt32UpperBound() && r->upper() < INT32_MAX) {
        Label success;
        masm.branch32(Assembler::LessThanOrEqual, input, Imm32(r->upper()), &success);
        masm.assumeUnreachable("Integer input should be lower or equal than Upperbound.");
        masm.bind(&success);
    }

    
    
    

    return true;
}

bool
CodeGenerator::emitAssertRangeD(const Range *r, FloatRegister input, FloatRegister temp)
{
    
    if (r->hasInt32LowerBound()) {
        Label success;
        masm.loadConstantDouble(r->lower(), temp);
        if (r->canBeNaN())
            masm.branchDouble(Assembler::DoubleUnordered, input, input, &success);
        masm.branchDouble(Assembler::DoubleGreaterThanOrEqual, input, temp, &success);
        masm.assumeUnreachable("Double input should be equal or higher than Lowerbound.");
        masm.bind(&success);
    }
    
    if (r->hasInt32UpperBound()) {
        Label success;
        masm.loadConstantDouble(r->upper(), temp);
        if (r->canBeNaN())
            masm.branchDouble(Assembler::DoubleUnordered, input, input, &success);
        masm.branchDouble(Assembler::DoubleLessThanOrEqual, input, temp, &success);
        masm.assumeUnreachable("Double input should be lower or equal than Upperbound.");
        masm.bind(&success);
    }

    
    

    if (!r->hasInt32Bounds() && !r->canBeInfiniteOrNaN() &&
        r->exponent() < FloatingPoint<double>::kExponentBias)
    {
        
        Label exponentLoOk;
        masm.loadConstantDouble(pow(2.0, r->exponent() + 1), temp);
        masm.branchDouble(Assembler::DoubleUnordered, input, input, &exponentLoOk);
        masm.branchDouble(Assembler::DoubleLessThanOrEqual, input, temp, &exponentLoOk);
        masm.assumeUnreachable("Check for exponent failed.");
        masm.bind(&exponentLoOk);

        Label exponentHiOk;
        masm.loadConstantDouble(-pow(2.0, r->exponent() + 1), temp);
        masm.branchDouble(Assembler::DoubleUnordered, input, input, &exponentHiOk);
        masm.branchDouble(Assembler::DoubleGreaterThanOrEqual, input, temp, &exponentHiOk);
        masm.assumeUnreachable("Check for exponent failed.");
        masm.bind(&exponentHiOk);
    } else if (!r->hasInt32Bounds() && !r->canBeNaN()) {
        
        Label notnan;
        masm.branchDouble(Assembler::DoubleOrdered, input, input, &notnan);
        masm.assumeUnreachable("Input shouldn't be NaN.");
        masm.bind(&notnan);

        
        if (!r->canBeInfiniteOrNaN()) {
            Label notposinf;
            masm.loadConstantDouble(PositiveInfinity<double>(), temp);
            masm.branchDouble(Assembler::DoubleLessThan, input, temp, &notposinf);
            masm.assumeUnreachable("Input shouldn't be +Inf.");
            masm.bind(&notposinf);

            Label notneginf;
            masm.loadConstantDouble(NegativeInfinity<double>(), temp);
            masm.branchDouble(Assembler::DoubleGreaterThan, input, temp, &notneginf);
            masm.assumeUnreachable("Input shouldn't be -Inf.");
            masm.bind(&notneginf);
        }
    }

    return true;
}

bool
CodeGenerator::visitAssertRangeI(LAssertRangeI *ins)
{
    Register input = ToRegister(ins->input());
    const Range *r = ins->range();

    return emitAssertRangeI(r, input);
}

bool
CodeGenerator::visitAssertRangeD(LAssertRangeD *ins)
{
    FloatRegister input = ToFloatRegister(ins->input());
    FloatRegister temp = ToFloatRegister(ins->temp());
    const Range *r = ins->range();

    return emitAssertRangeD(r, input, temp);
}

bool
CodeGenerator::visitAssertRangeF(LAssertRangeF *ins)
{
    FloatRegister input = ToFloatRegister(ins->input());
    FloatRegister temp = ToFloatRegister(ins->temp());
    FloatRegister dest = input;
    if (hasMultiAlias())
        dest = ToFloatRegister(ins->armtemp());

    const Range *r = ins->range();

    masm.convertFloat32ToDouble(input, dest);
    bool success = emitAssertRangeD(r, dest, temp);
    if (dest == input)
        masm.convertDoubleToFloat32(input, input);
    return success;
}

bool
CodeGenerator::visitAssertRangeV(LAssertRangeV *ins)
{
    const Range *r = ins->range();
    const ValueOperand value = ToValue(ins, LAssertRangeV::Input);
    Register tag = masm.splitTagForTest(value);
    Label done;

    {
        Label isNotInt32;
        masm.branchTestInt32(Assembler::NotEqual, tag, &isNotInt32);
        Register unboxInt32 = ToTempUnboxRegister(ins->temp());
        Register input = masm.extractInt32(value, unboxInt32);
        emitAssertRangeI(r, input);
        masm.jump(&done);
        masm.bind(&isNotInt32);
    }

    {
        Label isNotDouble;
        masm.branchTestDouble(Assembler::NotEqual, tag, &isNotDouble);
        FloatRegister input = ToFloatRegister(ins->floatTemp1());
        FloatRegister temp = ToFloatRegister(ins->floatTemp2());
        masm.unboxDouble(value, input);
        emitAssertRangeD(r, input, temp);
        masm.jump(&done);
        masm.bind(&isNotDouble);
    }

    masm.assumeUnreachable("Incorrect range for Value.");
    masm.bind(&done);
    return true;
}

bool
CodeGenerator::visitInterruptCheck(LInterruptCheck *lir)
{
    OutOfLineCode *ool = oolCallVM(InterruptCheckInfo, lir, (ArgList()), StoreNothing());
    if (!ool)
        return false;

    AbsoluteAddress interruptAddr(GetIonContext()->runtime->addressOfInterrupt());
    masm.branch32(Assembler::NotEqual, interruptAddr, Imm32(0), ool->entry());
    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitAsmJSInterruptCheck(LAsmJSInterruptCheck *lir)
{
    Register scratch = ToRegister(lir->scratch());
    masm.movePtr(AsmJSImmPtr(AsmJSImm_RuntimeInterrupt), scratch);
    masm.load8ZeroExtend(Address(scratch, 0), scratch);
    Label rejoin;
    masm.branch32(Assembler::Equal, scratch, Imm32(0), &rejoin);
    {
        uint32_t stackFixup = ComputeByteAlignment(masm.framePushed() + sizeof(AsmJSFrame),
                                                   ABIStackAlignment);
        masm.reserveStack(stackFixup);
        masm.call(lir->funcDesc(), lir->interruptExit());
        masm.freeStack(stackFixup);
    }
    masm.bind(&rejoin);
    return true;
}

typedef bool (*RecompileFn)(JSContext *);
static const VMFunction RecompileFnInfo = FunctionInfo<RecompileFn>(Recompile);

bool
CodeGenerator::visitRecompileCheck(LRecompileCheck *ins)
{
    Label done;
    Register tmp = ToRegister(ins->scratch());
    OutOfLineCode *ool = oolCallVM(RecompileFnInfo, ins, (ArgList()), StoreRegisterTo(tmp));

    
    masm.movePtr(ImmPtr(ins->mir()->script()->addressOfUseCount()), tmp);
    Address ptr(tmp, 0);
    masm.add32(Imm32(1), tmp);
    masm.branch32(Assembler::BelowOrEqual, ptr, Imm32(ins->mir()->recompileThreshold()), &done);

    
    CodeOffsetLabel label = masm.movWithPatch(ImmWord(uintptr_t(-1)), tmp);
    if (!ionScriptLabels_.append(label))
        return false;
    masm.branch32(Assembler::Equal,
                  Address(tmp, IonScript::offsetOfRecompiling()),
                  Imm32(0),
                  ool->entry());
    masm.bind(ool->rejoin());
    masm.bind(&done);

    return true;
}

} 
} 
