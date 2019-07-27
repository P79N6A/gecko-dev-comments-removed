

















#ifndef asmjs_AsmJSModule_h
#define asmjs_AsmJSModule_h

#include "mozilla/Maybe.h"
#include "mozilla/Move.h"
#include "mozilla/PodOperations.h"

#include "jsscript.h"

#include "asmjs/AsmJSFrameIterator.h"
#include "asmjs/AsmJSValidate.h"
#include "builtin/SIMD.h"
#include "gc/Marking.h"
#include "jit/IonTypes.h"
#include "jit/MacroAssembler.h"
#ifdef JS_ION_PERF
# include "jit/PerfSpewer.h"
#endif
#include "jit/RegisterSets.h"
#include "jit/shared/Assembler-shared.h"
#include "vm/TypedArrayObject.h"

namespace js {

namespace frontend { class TokenStream; }

using JS::GenericNaN;


enum AsmJSCoercion
{
    AsmJS_ToInt32,
    AsmJS_ToNumber,
    AsmJS_FRound,
    AsmJS_ToInt32x4,
    AsmJS_ToFloat32x4
};


enum AsmJSMathBuiltinFunction
{
    AsmJSMathBuiltin_sin, AsmJSMathBuiltin_cos, AsmJSMathBuiltin_tan,
    AsmJSMathBuiltin_asin, AsmJSMathBuiltin_acos, AsmJSMathBuiltin_atan,
    AsmJSMathBuiltin_ceil, AsmJSMathBuiltin_floor, AsmJSMathBuiltin_exp,
    AsmJSMathBuiltin_log, AsmJSMathBuiltin_pow, AsmJSMathBuiltin_sqrt,
    AsmJSMathBuiltin_abs, AsmJSMathBuiltin_atan2, AsmJSMathBuiltin_imul,
    AsmJSMathBuiltin_fround, AsmJSMathBuiltin_min, AsmJSMathBuiltin_max,
    AsmJSMathBuiltin_clz32
};


enum AsmJSAtomicsBuiltinFunction
{
    AsmJSAtomicsBuiltin_compareExchange,
    AsmJSAtomicsBuiltin_load,
    AsmJSAtomicsBuiltin_store,
    AsmJSAtomicsBuiltin_fence,
    AsmJSAtomicsBuiltin_add,
    AsmJSAtomicsBuiltin_sub,
    AsmJSAtomicsBuiltin_and,
    AsmJSAtomicsBuiltin_or,
    AsmJSAtomicsBuiltin_xor
};


enum AsmJSSimdType
{
    AsmJSSimdType_int32x4,
    AsmJSSimdType_float32x4
};


enum AsmJSSimdOperation
{
#define ASMJSSIMDOPERATION(op) AsmJSSimdOperation_##op,
    FORALL_SIMD_OP(ASMJSSIMDOPERATION)
#undef ASMJSSIMDOPERATION
};



struct AsmJSFunctionLabels
{
    AsmJSFunctionLabels(jit::Label& entry, jit::Label& overflowExit)
      : entry(entry), overflowExit(overflowExit) {}

    jit::Label begin;
    jit::Label& entry;
    jit::Label profilingJump;
    jit::Label profilingEpilogue;
    jit::Label profilingReturn;
    jit::Label end;
    mozilla::Maybe<jit::Label> overflowThunk;
    jit::Label& overflowExit;
};











class AsmJSNumLit
{
  public:
    enum Which {
        Fixnum,
        NegativeInt,
        BigUnsigned,
        Double,
        Float,
        Int32x4,
        Float32x4,
        OutOfRangeInt = -1
    };

  private:
    Which which_;
    union {
        Value scalar_;
        jit::SimdConstant simd_;
    } value;

  public:
    static AsmJSNumLit Create(Which w, Value v) {
        AsmJSNumLit lit;
        lit.which_ = w;
        lit.value.scalar_ = v;
        MOZ_ASSERT(!lit.isSimd());
        return lit;
    }

    static AsmJSNumLit Create(Which w, jit::SimdConstant c) {
        AsmJSNumLit lit;
        lit.which_ = w;
        lit.value.simd_ = c;
        MOZ_ASSERT(lit.isSimd());
        return lit;
    }

    Which which() const {
        return which_;
    }

    int32_t toInt32() const {
        MOZ_ASSERT(which_ == Fixnum || which_ == NegativeInt || which_ == BigUnsigned);
        return value.scalar_.toInt32();
    }

    double toDouble() const {
        MOZ_ASSERT(which_ == Double);
        return value.scalar_.toDouble();
    }

    float toFloat() const {
        MOZ_ASSERT(which_ == Float);
        return float(value.scalar_.toDouble());
    }

    Value scalarValue() const {
        MOZ_ASSERT(which_ != OutOfRangeInt);
        return value.scalar_;
    }

    bool isSimd() const {
        return which_ == Int32x4 || which_ == Float32x4;
    }

    const jit::SimdConstant& simdValue() const {
        MOZ_ASSERT(isSimd());
        return value.simd_;
    }

    bool hasType() const {
        return which_ != OutOfRangeInt;
    }
};











class AsmJSModule
{
  public:
    class Global
    {
      public:
        enum Which { Variable, FFI, ArrayView, ArrayViewCtor, SharedArrayView, MathBuiltinFunction,
                     AtomicsBuiltinFunction, Constant, SimdCtor, SimdOperation, ByteLength };
        enum VarInitKind { InitConstant, InitImport };
        enum ConstantKind { GlobalConstant, MathConstant };

      private:
        struct Pod {
            Which which_;
            union {
                struct {
                    uint32_t index_;
                    VarInitKind initKind_;
                    union {
                        AsmJSCoercion coercion_;
                        AsmJSNumLit numLit_;
                    } u;
                } var;
                uint32_t ffiIndex_;
                Scalar::Type viewType_;
                AsmJSMathBuiltinFunction mathBuiltinFunc_;
                AsmJSAtomicsBuiltinFunction atomicsBuiltinFunc_;
                AsmJSSimdType simdCtorType_;
                struct {
                    AsmJSSimdType type_;
                    AsmJSSimdOperation which_;
                } simdOp;
                struct {
                    ConstantKind kind_;
                    double value_;
                } constant;
            } u;
        } pod;
        PropertyName* name_;

        friend class AsmJSModule;

        Global(Which which, PropertyName* name) {
            mozilla::PodZero(&pod);  
            pod.which_ = which;
            name_ = name;
            MOZ_ASSERT_IF(name_, name_->isTenured());
        }

        void trace(JSTracer* trc) {
            if (name_)
                TraceManuallyBarrieredEdge(trc, &name_, "asm.js global name");
            MOZ_ASSERT_IF(pod.which_ == Variable && pod.u.var.initKind_ == InitConstant,
                          !pod.u.var.u.numLit_.scalarValue().isMarkable());
        }

      public:
        Global() {}
        Which which() const {
            return pod.which_;
        }
        uint32_t varIndex() const {
            MOZ_ASSERT(pod.which_ == Variable);
            return pod.u.var.index_;
        }
        VarInitKind varInitKind() const {
            MOZ_ASSERT(pod.which_ == Variable);
            return pod.u.var.initKind_;
        }
        const AsmJSNumLit& varInitNumLit() const {
            MOZ_ASSERT(pod.which_ == Variable);
            MOZ_ASSERT(pod.u.var.initKind_ == InitConstant);
            return pod.u.var.u.numLit_;
        }
        AsmJSCoercion varInitCoercion() const {
            MOZ_ASSERT(pod.which_ == Variable);
            MOZ_ASSERT(pod.u.var.initKind_ == InitImport);
            return pod.u.var.u.coercion_;
        }
        PropertyName* varImportField() const {
            MOZ_ASSERT(pod.which_ == Variable);
            MOZ_ASSERT(pod.u.var.initKind_ == InitImport);
            return name_;
        }
        PropertyName* ffiField() const {
            MOZ_ASSERT(pod.which_ == FFI);
            return name_;
        }
        uint32_t ffiIndex() const {
            MOZ_ASSERT(pod.which_ == FFI);
            return pod.u.ffiIndex_;
        }
        
        
        
        
        PropertyName* maybeViewName() const {
            MOZ_ASSERT(pod.which_ == ArrayView || pod.which_ == SharedArrayView || pod.which_ == ArrayViewCtor);
            return name_;
        }
        Scalar::Type viewType() const {
            MOZ_ASSERT(pod.which_ == ArrayView || pod.which_ == SharedArrayView || pod.which_ == ArrayViewCtor);
            return pod.u.viewType_;
        }
        PropertyName* mathName() const {
            MOZ_ASSERT(pod.which_ == MathBuiltinFunction);
            return name_;
        }
        PropertyName* atomicsName() const {
            MOZ_ASSERT(pod.which_ == AtomicsBuiltinFunction);
            return name_;
        }
        AsmJSMathBuiltinFunction mathBuiltinFunction() const {
            MOZ_ASSERT(pod.which_ == MathBuiltinFunction);
            return pod.u.mathBuiltinFunc_;
        }
        AsmJSAtomicsBuiltinFunction atomicsBuiltinFunction() const {
            MOZ_ASSERT(pod.which_ == AtomicsBuiltinFunction);
            return pod.u.atomicsBuiltinFunc_;
        }
        AsmJSSimdType simdCtorType() const {
            MOZ_ASSERT(pod.which_ == SimdCtor);
            return pod.u.simdCtorType_;
        }
        PropertyName* simdCtorName() const {
            MOZ_ASSERT(pod.which_ == SimdCtor);
            return name_;
        }
        PropertyName* simdOperationName() const {
            MOZ_ASSERT(pod.which_ == SimdOperation);
            return name_;
        }
        AsmJSSimdOperation simdOperation() const {
            MOZ_ASSERT(pod.which_ == SimdOperation);
            return pod.u.simdOp.which_;
        }
        AsmJSSimdType simdOperationType() const {
            MOZ_ASSERT(pod.which_ == SimdOperation);
            return pod.u.simdOp.type_;
        }
        PropertyName* constantName() const {
            MOZ_ASSERT(pod.which_ == Constant);
            return name_;
        }
        ConstantKind constantKind() const {
            MOZ_ASSERT(pod.which_ == Constant);
            return pod.u.constant.kind_;
        }
        double constantValue() const {
            MOZ_ASSERT(pod.which_ == Constant);
            return pod.u.constant.value_;
        }

        size_t serializedSize() const;
        uint8_t* serialize(uint8_t* cursor) const;
        const uint8_t* deserialize(ExclusiveContext* cx, const uint8_t* cursor);
        bool clone(ExclusiveContext* cx, Global* out) const;
    };

    class Exit
    {
        unsigned ffiIndex_;
        unsigned globalDataOffset_;
        unsigned interpCodeOffset_;
        unsigned jitCodeOffset_;

        friend class AsmJSModule;

      public:
        Exit() {}
        Exit(unsigned ffiIndex, unsigned globalDataOffset)
          : ffiIndex_(ffiIndex), globalDataOffset_(globalDataOffset),
            interpCodeOffset_(0), jitCodeOffset_(0)
        {}
        unsigned ffiIndex() const {
            return ffiIndex_;
        }
        unsigned globalDataOffset() const {
            return globalDataOffset_;
        }
        void initInterpOffset(unsigned off) {
            MOZ_ASSERT(!interpCodeOffset_);
            interpCodeOffset_ = off;
        }
        void initJitOffset(unsigned off) {
            MOZ_ASSERT(!jitCodeOffset_);
            jitCodeOffset_ = off;
        }
        void updateOffsets(jit::MacroAssembler& masm) {
            interpCodeOffset_ = masm.actualOffset(interpCodeOffset_);
            jitCodeOffset_ = masm.actualOffset(jitCodeOffset_);
        }

        size_t serializedSize() const;
        uint8_t* serialize(uint8_t* cursor) const;
        const uint8_t* deserialize(ExclusiveContext* cx, const uint8_t* cursor);
        bool clone(ExclusiveContext* cx, Exit* out) const;
    };

    struct EntryArg {
        uint64_t lo;
        uint64_t hi;
    };

    typedef int32_t (*CodePtr)(EntryArg* args, uint8_t* global);

    
    
    
    struct ExitDatum
    {
        uint8_t* exit;
        jit::BaselineScript* baselineScript;
        HeapPtrFunction fun;
    };

    typedef Vector<AsmJSCoercion, 0, SystemAllocPolicy> ArgCoercionVector;

    enum ReturnType { Return_Int32, Return_Double, Return_Int32x4, Return_Float32x4, Return_Void };

    class ExportedFunction
    {
        PropertyName* name_;
        PropertyName* maybeFieldName_;
        ArgCoercionVector argCoercions_;
        struct Pod {
            bool isChangeHeap_;
            ReturnType returnType_;
            uint32_t codeOffset_;
            uint32_t startOffsetInModule_;  
            uint32_t endOffsetInModule_;    
        } pod;

        friend class AsmJSModule;

        ExportedFunction(PropertyName* name,
                         uint32_t startOffsetInModule, uint32_t endOffsetInModule,
                         PropertyName* maybeFieldName,
                         ArgCoercionVector&& argCoercions,
                         ReturnType returnType)
        {
            MOZ_ASSERT(name->isTenured());
            MOZ_ASSERT_IF(maybeFieldName, maybeFieldName->isTenured());
            name_ = name;
            maybeFieldName_ = maybeFieldName;
            argCoercions_ = mozilla::Move(argCoercions);
            mozilla::PodZero(&pod);  
            pod.isChangeHeap_ = false;
            pod.returnType_ = returnType;
            pod.codeOffset_ = UINT32_MAX;
            pod.startOffsetInModule_ = startOffsetInModule;
            pod.endOffsetInModule_ = endOffsetInModule;
        }

        ExportedFunction(PropertyName* name,
                         uint32_t startOffsetInModule, uint32_t endOffsetInModule,
                         PropertyName* maybeFieldName)
        {
            MOZ_ASSERT(name->isTenured());
            MOZ_ASSERT_IF(maybeFieldName, maybeFieldName->isTenured());
            name_ = name;
            maybeFieldName_ = maybeFieldName;
            mozilla::PodZero(&pod);  
            pod.isChangeHeap_ = true;
            pod.startOffsetInModule_ = startOffsetInModule;
            pod.endOffsetInModule_ = endOffsetInModule;
        }

        void trace(JSTracer* trc) {
            TraceManuallyBarrieredEdge(trc, &name_, "asm.js export name");
            if (maybeFieldName_)
                TraceManuallyBarrieredEdge(trc, &maybeFieldName_, "asm.js export field");
        }

      public:
        ExportedFunction() {}
        ExportedFunction(ExportedFunction&& rhs) {
            name_ = rhs.name_;
            maybeFieldName_ = rhs.maybeFieldName_;
            argCoercions_ = mozilla::Move(rhs.argCoercions_);
            mozilla::PodZero(&pod);  
            pod = rhs.pod;
        }

        PropertyName* name() const {
            return name_;
        }
        PropertyName* maybeFieldName() const {
            return maybeFieldName_;
        }
        uint32_t startOffsetInModule() const {
            return pod.startOffsetInModule_;
        }
        uint32_t endOffsetInModule() const {
            return pod.endOffsetInModule_;
        }

        bool isChangeHeap() const {
            return pod.isChangeHeap_;
        }

        void initCodeOffset(unsigned off) {
            MOZ_ASSERT(!isChangeHeap());
            MOZ_ASSERT(pod.codeOffset_ == UINT32_MAX);
            pod.codeOffset_ = off;
        }
        void updateCodeOffset(jit::MacroAssembler& masm) {
            MOZ_ASSERT(!isChangeHeap());
            pod.codeOffset_ = masm.actualOffset(pod.codeOffset_);
        }

        unsigned numArgs() const {
            MOZ_ASSERT(!isChangeHeap());
            return argCoercions_.length();
        }
        AsmJSCoercion argCoercion(unsigned i) const {
            MOZ_ASSERT(!isChangeHeap());
            return argCoercions_[i];
        }
        ReturnType returnType() const {
            MOZ_ASSERT(!isChangeHeap());
            return pod.returnType_;
        }

        size_t serializedSize() const;
        uint8_t* serialize(uint8_t* cursor) const;
        const uint8_t* deserialize(ExclusiveContext* cx, const uint8_t* cursor);
        bool clone(ExclusiveContext* cx, ExportedFunction* out) const;
    };

    class CodeRange
    {
        uint32_t nameIndex_;
        uint32_t lineNumber_;
        uint32_t begin_;
        uint32_t profilingReturn_;
        uint32_t end_;
        union {
            struct {
                uint8_t kind_;
                uint8_t beginToEntry_;
                uint8_t profilingJumpToProfilingReturn_;
                uint8_t profilingEpilogueToProfilingReturn_;
            } func;
            struct {
                uint8_t kind_;
                uint16_t target_;
            } thunk;
            uint8_t kind_;
        } u;

        void setDeltas(uint32_t entry, uint32_t profilingJump, uint32_t profilingEpilogue);

      public:
        enum Kind { Function, Entry, JitFFI, SlowFFI, Interrupt, Thunk, Inline };

        CodeRange() {}
        CodeRange(uint32_t nameIndex, uint32_t lineNumber, const AsmJSFunctionLabels& l);
        CodeRange(Kind kind, uint32_t begin, uint32_t end);
        CodeRange(Kind kind, uint32_t begin, uint32_t profilingReturn, uint32_t end);
        CodeRange(AsmJSExit::BuiltinKind builtin, uint32_t begin, uint32_t pret, uint32_t end);
        void updateOffsets(jit::MacroAssembler& masm);

        Kind kind() const { return Kind(u.kind_); }
        bool isFunction() const { return kind() == Function; }
        bool isEntry() const { return kind() == Entry; }
        bool isFFI() const { return kind() == JitFFI || kind() == SlowFFI; }
        bool isInterrupt() const { return kind() == Interrupt; }
        bool isThunk() const { return kind() == Thunk; }

        uint32_t begin() const {
            return begin_;
        }
        uint32_t entry() const {
            MOZ_ASSERT(isFunction());
            return begin_ + u.func.beginToEntry_;
        }
        uint32_t end() const {
            return end_;
        }
        uint32_t profilingJump() const {
            MOZ_ASSERT(isFunction());
            return profilingReturn_ - u.func.profilingJumpToProfilingReturn_;
        }
        uint32_t profilingEpilogue() const {
            MOZ_ASSERT(isFunction());
            return profilingReturn_ - u.func.profilingEpilogueToProfilingReturn_;
        }
        uint32_t profilingReturn() const {
            MOZ_ASSERT(isFunction() || isFFI() || isInterrupt() || isThunk());
            return profilingReturn_;
        }
        uint32_t functionNameIndex() const {
            MOZ_ASSERT(isFunction());
            return nameIndex_;
        }
        PropertyName* functionName(const AsmJSModule& module) const {
            MOZ_ASSERT(isFunction());
            return module.names_[nameIndex_].name();
        }
        const char* functionProfilingLabel(const AsmJSModule& module) const {
            MOZ_ASSERT(isFunction());
            return module.profilingLabels_[nameIndex_].get();
        }
        uint32_t functionLineNumber() const {
            MOZ_ASSERT(isFunction());
            return lineNumber_;
        }
        AsmJSExit::BuiltinKind thunkTarget() const {
            MOZ_ASSERT(isThunk());
            return AsmJSExit::BuiltinKind(u.thunk.target_);
        }
    };

    class FuncPtrTable
    {
        uint32_t globalDataOffset_;
        uint32_t numElems_;
      public:
        FuncPtrTable() {}
        FuncPtrTable(uint32_t globalDataOffset, uint32_t numElems)
          : globalDataOffset_(globalDataOffset), numElems_(numElems)
        {}
        uint32_t globalDataOffset() const { return globalDataOffset_; }
        uint32_t numElems() const { return numElems_; }
    };

    class Name
    {
        PropertyName* name_;
      public:
        Name() : name_(nullptr) {}
        MOZ_IMPLICIT Name(PropertyName* name) : name_(name) {}
        PropertyName* name() const { return name_; }
        PropertyName*& name() { return name_; }
        size_t serializedSize() const;
        uint8_t* serialize(uint8_t* cursor) const;
        const uint8_t* deserialize(ExclusiveContext* cx, const uint8_t* cursor);
        bool clone(ExclusiveContext* cx, Name* out) const;
    };

    typedef mozilla::UniquePtr<char[], JS::FreePolicy> ProfilingLabel;

#if defined(MOZ_VTUNE) || defined(JS_ION_PERF)
    
    struct ProfiledFunction
    {
        PropertyName* name;
        struct Pod {
            unsigned startCodeOffset;
            unsigned endCodeOffset;
            unsigned lineno;
            unsigned columnIndex;
        } pod;

        explicit ProfiledFunction()
          : name(nullptr)
        { }

        ProfiledFunction(PropertyName* name, unsigned start, unsigned end,
                         unsigned line = 0, unsigned column = 0)
          : name(name)
        {
            MOZ_ASSERT(name->isTenured());

            pod.startCodeOffset = start;
            pod.endCodeOffset = end;
            pod.lineno = line;
            pod.columnIndex = column;
        }

        void trace(JSTracer* trc) {
            if (name)
                TraceManuallyBarrieredEdge(trc, &name, "asm.js profiled function name");
        }

        size_t serializedSize() const;
        uint8_t* serialize(uint8_t* cursor) const;
        const uint8_t* deserialize(ExclusiveContext* cx, const uint8_t* cursor);
    };
#endif

#if defined(JS_ION_PERF)
    struct ProfiledBlocksFunction : public ProfiledFunction
    {
        unsigned endInlineCodeOffset;
        jit::BasicBlocksVector blocks;

        ProfiledBlocksFunction(PropertyName* name, unsigned start, unsigned endInline, unsigned end,
                               jit::BasicBlocksVector& blocksVector)
          : ProfiledFunction(name, start, end), endInlineCodeOffset(endInline),
            blocks(mozilla::Move(blocksVector))
        {
            MOZ_ASSERT(name->isTenured());
        }

        ProfiledBlocksFunction(ProfiledBlocksFunction&& copy)
          : ProfiledFunction(copy.name, copy.pod.startCodeOffset, copy.pod.endCodeOffset),
            endInlineCodeOffset(copy.endInlineCodeOffset), blocks(mozilla::Move(copy.blocks))
        { }
    };
#endif

    struct RelativeLink
    {
        enum Kind
        {
            RawPointer,
            CodeLabel,
            InstructionImmediate
        };

        RelativeLink()
        { }

        explicit RelativeLink(Kind kind)
        {
#if defined(JS_CODEGEN_MIPS)
            kind_ = kind;
#elif defined(JS_CODEGEN_ARM)
            
            
            MOZ_ASSERT(kind == CodeLabel || kind == RawPointer);
#endif

        }

        bool isRawPointerPatch() {
#if defined(JS_CODEGEN_MIPS)
            return kind_ == RawPointer;
#else
            return true;
#endif
        }

#ifdef JS_CODEGEN_MIPS
        Kind kind_;
#endif
        uint32_t patchAtOffset;
        uint32_t targetOffset;
    };

    typedef Vector<RelativeLink, 0, SystemAllocPolicy> RelativeLinkVector;

    typedef Vector<uint32_t, 0, SystemAllocPolicy> OffsetVector;

    class AbsoluteLinkArray
    {
        OffsetVector array_[jit::AsmJSImm_Limit];

      public:
        OffsetVector& operator[](size_t i) {
            MOZ_ASSERT(i < jit::AsmJSImm_Limit);
            return array_[i];
        }
        const OffsetVector& operator[](size_t i) const {
            MOZ_ASSERT(i < jit::AsmJSImm_Limit);
            return array_[i];
        }

        size_t serializedSize() const;
        uint8_t* serialize(uint8_t* cursor) const;
        const uint8_t* deserialize(ExclusiveContext* cx, const uint8_t* cursor);
        bool clone(ExclusiveContext* cx, AbsoluteLinkArray* out) const;

        size_t sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf) const;
    };

    
    
    
    
    struct StaticLinkData
    {
        uint32_t interruptExitOffset;
        uint32_t outOfBoundsExitOffset;
        RelativeLinkVector relativeLinks;
        AbsoluteLinkArray absoluteLinks;

        size_t serializedSize() const;
        uint8_t* serialize(uint8_t* cursor) const;
        const uint8_t* deserialize(ExclusiveContext* cx, const uint8_t* cursor);
        bool clone(ExclusiveContext* cx, StaticLinkData* out) const;

        size_t sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf) const;
    };

  private:
    struct Pod {
        size_t                            funcPtrTableAndExitBytes_;
        size_t                            functionBytes_; 
        size_t                            codeBytes_;     
        size_t                            totalBytes_;    
        uint32_t                          minHeapLength_;
        uint32_t                          maxHeapLength_;
        uint32_t                          heapLengthMask_;
        uint32_t                          numGlobalScalarVars_;
        uint32_t                          numGlobalSimdVars_;
        uint32_t                          numFFIs_;
        uint32_t                          srcLength_;
        uint32_t                          srcLengthWithRightBrace_;
        bool                              strict_;
        bool                              hasArrayView_;
        bool                              isSharedView_;
        bool                              hasFixedMinHeapLength_;
        bool                              usesSignalHandlers_;
    } pod;

    
    
    
    const uint32_t                        srcStart_;
    const uint32_t                        srcBodyStart_;

    Vector<Global,                 0, SystemAllocPolicy> globals_;
    Vector<Exit,                   0, SystemAllocPolicy> exits_;
    Vector<ExportedFunction,       0, SystemAllocPolicy> exports_;
    Vector<jit::CallSite,          0, SystemAllocPolicy> callSites_;
    Vector<CodeRange,              0, SystemAllocPolicy> codeRanges_;
    Vector<FuncPtrTable,           0, SystemAllocPolicy> funcPtrTables_;
    Vector<uint32_t,               0, SystemAllocPolicy> builtinThunkOffsets_;
    Vector<Name,                   0, SystemAllocPolicy> names_;
    Vector<ProfilingLabel,         0, SystemAllocPolicy> profilingLabels_;
    Vector<jit::AsmJSHeapAccess,   0, SystemAllocPolicy> heapAccesses_;
    Vector<jit::IonScriptCounts*,  0, SystemAllocPolicy> functionCounts_;
#if defined(MOZ_VTUNE) || defined(JS_ION_PERF)
    Vector<ProfiledFunction,       0, SystemAllocPolicy> profiledFunctions_;
#endif
#if defined(JS_ION_PERF)
    Vector<ProfiledBlocksFunction, 0, SystemAllocPolicy> perfProfiledBlocksFunctions_;
#endif

    ScriptSource *                        scriptSource_;
    PropertyName *                        globalArgumentName_;
    PropertyName *                        importArgumentName_;
    PropertyName *                        bufferArgumentName_;
    uint8_t *                             code_;
    uint8_t *                             interruptExit_;
    uint8_t *                             outOfBoundsExit_;
    StaticLinkData                        staticLinkData_;
    HeapPtrArrayBufferObjectMaybeShared   maybeHeap_;
    AsmJSModule **                        prevLinked_;
    AsmJSModule *                         nextLinked_;
    bool                                  dynamicallyLinked_;
    bool                                  loadedFromCache_;
    bool                                  profilingEnabled_;
    bool                                  interrupted_;

    void restoreHeapToInitialState(ArrayBufferObjectMaybeShared* maybePrevBuffer);
    void restoreToInitialState(ArrayBufferObjectMaybeShared* maybePrevBuffer, uint8_t* prevCode,
                               ExclusiveContext* cx);

  public:
    explicit AsmJSModule(ScriptSource* scriptSource, uint32_t srcStart, uint32_t srcBodyStart,
                         bool strict, bool canUseSignalHandlers);
    void trace(JSTracer* trc);
    ~AsmJSModule();

    
    bool isFinishedWithModulePrologue() const { return pod.funcPtrTableAndExitBytes_ != SIZE_MAX; }
    bool isFinishedWithFunctionBodies() const { return pod.functionBytes_ != UINT32_MAX; }
    bool isFinished() const { return !!code_; }
    bool isStaticallyLinked() const { return !!interruptExit_; }
    bool isDynamicallyLinked() const { return dynamicallyLinked_; }

    
    

    ScriptSource* scriptSource() const {
        MOZ_ASSERT(scriptSource_);
        return scriptSource_;
    }
    bool strict() const {
        return pod.strict_;
    }
    bool usesSignalHandlersForInterrupt() const {
        return pod.usesSignalHandlers_;
    }
    bool usesSignalHandlersForOOB() const {
#if defined(ASMJS_MAY_USE_SIGNAL_HANDLERS_FOR_OOB)
        return pod.usesSignalHandlers_;
#else
        return false;
#endif
    }
    bool loadedFromCache() const {
        return loadedFromCache_;
    }

    
    
    
    
    
    uint32_t srcStart() const {
        return srcStart_;
    }

    
    
    uint32_t srcBodyStart() const {
        return srcBodyStart_;
    }

    
    
    uint32_t minHeapLength() const {
        return pod.minHeapLength_;
    }
    uint32_t maxHeapLength() const {
        return pod.maxHeapLength_;
    }
    uint32_t heapLengthMask() const {
        MOZ_ASSERT(pod.hasFixedMinHeapLength_);
        return pod.heapLengthMask_;
    }
    unsigned numFunctionCounts() const {
        return functionCounts_.length();
    }
    jit::IonScriptCounts* functionCounts(unsigned i) {
        return functionCounts_[i];
    }

    
    void addSizeOfMisc(mozilla::MallocSizeOf mallocSizeOf, size_t* asmJSModuleCode,
                       size_t* asmJSModuleData);

    
    
    

    void initGlobalArgumentName(PropertyName* n) {
        MOZ_ASSERT(!isFinishedWithModulePrologue());
        MOZ_ASSERT_IF(n, n->isTenured());
        globalArgumentName_ = n;
    }
    void initImportArgumentName(PropertyName* n) {
        MOZ_ASSERT(!isFinishedWithModulePrologue());
        MOZ_ASSERT_IF(n, n->isTenured());
        importArgumentName_ = n;
    }
    void initBufferArgumentName(PropertyName* n) {
        MOZ_ASSERT(!isFinishedWithModulePrologue());
        MOZ_ASSERT_IF(n, n->isTenured());
        bufferArgumentName_ = n;
    }
    PropertyName* globalArgumentName() const {
        return globalArgumentName_;
    }
    PropertyName* importArgumentName() const {
        return importArgumentName_;
    }
    PropertyName* bufferArgumentName() const {
        return bufferArgumentName_;
    }
    bool addGlobalVarInit(const AsmJSNumLit& lit, uint32_t* globalIndex) {
        MOZ_ASSERT(!isFinishedWithModulePrologue());
        Global g(Global::Variable, nullptr);
        g.pod.u.var.initKind_ = Global::InitConstant;
        g.pod.u.var.u.numLit_ = lit;

        if (lit.isSimd()) {
            if (pod.numGlobalSimdVars_ == UINT32_MAX)
                return false;
            *globalIndex = pod.numGlobalSimdVars_++;
        } else {
            if (pod.numGlobalScalarVars_ == UINT32_MAX)
                return false;
            *globalIndex = pod.numGlobalScalarVars_++;
        }

        g.pod.u.var.index_ = *globalIndex;
        return globals_.append(g);
    }
    static bool IsSimdCoercion(AsmJSCoercion c) {
        switch (c) {
          case AsmJS_ToInt32:
          case AsmJS_ToNumber:
          case AsmJS_FRound:
            return false;
          case AsmJS_ToInt32x4:
          case AsmJS_ToFloat32x4:
            return true;
        }
        MOZ_MAKE_COMPILER_ASSUME_IS_UNREACHABLE("unexpected AsmJSCoercion");
    }
    bool addGlobalVarImport(PropertyName* name, AsmJSCoercion coercion, uint32_t* globalIndex) {
        MOZ_ASSERT(!isFinishedWithModulePrologue());
        Global g(Global::Variable, name);
        g.pod.u.var.initKind_ = Global::InitImport;
        g.pod.u.var.u.coercion_ = coercion;
        *globalIndex = IsSimdCoercion(coercion) ? pod.numGlobalSimdVars_++
                                                : pod.numGlobalScalarVars_++;
        g.pod.u.var.index_ = *globalIndex;
        return globals_.append(g);
    }
    bool addFFI(PropertyName* field, uint32_t* ffiIndex) {
        MOZ_ASSERT(!isFinishedWithModulePrologue());
        if (pod.numFFIs_ == UINT32_MAX)
            return false;
        Global g(Global::FFI, field);
        g.pod.u.ffiIndex_ = *ffiIndex = pod.numFFIs_++;
        return globals_.append(g);
    }
    bool addArrayView(Scalar::Type vt, PropertyName* maybeField, bool isSharedView) {
        MOZ_ASSERT(!isFinishedWithModulePrologue());
        MOZ_ASSERT(!pod.hasArrayView_ || (pod.isSharedView_ == isSharedView));
        pod.hasArrayView_ = true;
        pod.isSharedView_ = isSharedView;
        Global g(Global::ArrayView, maybeField);
        g.pod.u.viewType_ = vt;
        return globals_.append(g);
    }
    bool addArrayViewCtor(Scalar::Type vt, PropertyName* field, bool isSharedView) {
        MOZ_ASSERT(!isFinishedWithModulePrologue());
        MOZ_ASSERT(field);
        MOZ_ASSERT(!pod.isSharedView_ || isSharedView);
        pod.isSharedView_ = isSharedView;
        Global g(Global::ArrayViewCtor, field);
        g.pod.u.viewType_ = vt;
        return globals_.append(g);
    }
    bool addByteLength() {
        MOZ_ASSERT(!isFinishedWithModulePrologue());
        Global g(Global::ByteLength, nullptr);
        return globals_.append(g);
    }
    bool addMathBuiltinFunction(AsmJSMathBuiltinFunction func, PropertyName* field) {
        MOZ_ASSERT(!isFinishedWithModulePrologue());
        Global g(Global::MathBuiltinFunction, field);
        g.pod.u.mathBuiltinFunc_ = func;
        return globals_.append(g);
    }
    bool addMathBuiltinConstant(double value, PropertyName* field) {
        MOZ_ASSERT(!isFinishedWithModulePrologue());
        Global g(Global::Constant, field);
        g.pod.u.constant.value_ = value;
        g.pod.u.constant.kind_ = Global::MathConstant;
        return globals_.append(g);
    }
    bool addAtomicsBuiltinFunction(AsmJSAtomicsBuiltinFunction func, PropertyName* field) {
        MOZ_ASSERT(!isFinishedWithModulePrologue());
        Global g(Global::AtomicsBuiltinFunction, field);
        g.pod.u.atomicsBuiltinFunc_ = func;
        return globals_.append(g);
    }
    bool addSimdCtor(AsmJSSimdType type, PropertyName* field) {
        Global g(Global::SimdCtor, field);
        g.pod.u.simdCtorType_ = type;
        return globals_.append(g);
    }
    bool addSimdOperation(AsmJSSimdType type, AsmJSSimdOperation op, PropertyName* field) {
        Global g(Global::SimdOperation, field);
        g.pod.u.simdOp.type_ = type;
        g.pod.u.simdOp.which_ = op;
        return globals_.append(g);
    }
    bool addGlobalConstant(double value, PropertyName* name) {
        MOZ_ASSERT(!isFinishedWithModulePrologue());
        Global g(Global::Constant, name);
        g.pod.u.constant.value_ = value;
        g.pod.u.constant.kind_ = Global::GlobalConstant;
        return globals_.append(g);
    }
    unsigned numGlobals() const {
        return globals_.length();
    }
    Global& global(unsigned i) {
        return globals_[i];
    }
    bool isValidViewSharedness(bool shared) const {
        if (pod.hasArrayView_)
            return pod.isSharedView_ == shared;
        return !pod.isSharedView_ || shared;
    }

    

    void startFunctionBodies() {
        MOZ_ASSERT(!isFinishedWithModulePrologue());
        pod.funcPtrTableAndExitBytes_ = 0;
        MOZ_ASSERT(isFinishedWithModulePrologue());
    }

    
    

    bool hasArrayView() const {
        MOZ_ASSERT(isFinishedWithModulePrologue());
        return pod.hasArrayView_;
    }
    bool isSharedView() const {
        MOZ_ASSERT(pod.hasArrayView_);
        return pod.isSharedView_;
    }
    void addChangeHeap(uint32_t mask, uint32_t min, uint32_t max) {
        MOZ_ASSERT(isFinishedWithModulePrologue());
        MOZ_ASSERT(!pod.hasFixedMinHeapLength_);
        MOZ_ASSERT(IsValidAsmJSHeapLength(mask + 1));
        MOZ_ASSERT(min >= RoundUpToNextValidAsmJSHeapLength(0));
        MOZ_ASSERT(max <= pod.maxHeapLength_);
        MOZ_ASSERT(min <= max);
        pod.heapLengthMask_ = mask;
        pod.minHeapLength_ = min;
        pod.maxHeapLength_ = max;
        pod.hasFixedMinHeapLength_ = true;
    }
    bool tryRequireHeapLengthToBeAtLeast(uint32_t len) {
        MOZ_ASSERT(isFinishedWithModulePrologue() && !isFinishedWithFunctionBodies());
        if (pod.hasFixedMinHeapLength_ && len > pod.minHeapLength_)
            return false;
        if (len > pod.maxHeapLength_)
            return false;
        len = RoundUpToNextValidAsmJSHeapLength(len);
        if (len > pod.minHeapLength_)
            pod.minHeapLength_ = len;
        return true;
    }
    bool addCodeRange(CodeRange::Kind kind, uint32_t begin, uint32_t end) {
        return codeRanges_.append(CodeRange(kind, begin, end));
    }
    bool addCodeRange(CodeRange::Kind kind, uint32_t begin, uint32_t pret, uint32_t end) {
        return codeRanges_.append(CodeRange(kind, begin, pret, end));
    }
    bool addFunctionCodeRange(PropertyName* name, uint32_t lineNumber,
                              const AsmJSFunctionLabels& labels)
    {
        MOZ_ASSERT(!isFinished());
        MOZ_ASSERT(name->isTenured());
        if (names_.length() >= UINT32_MAX)
            return false;
        uint32_t nameIndex = names_.length();
        return names_.append(name) && codeRanges_.append(CodeRange(nameIndex, lineNumber, labels));
    }
    bool addBuiltinThunkCodeRange(AsmJSExit::BuiltinKind builtin, uint32_t begin,
                                  uint32_t profilingReturn, uint32_t end)
    {
        return builtinThunkOffsets_.append(begin) &&
               codeRanges_.append(CodeRange(builtin, begin, profilingReturn, end));
    }
    bool addExit(unsigned ffiIndex, unsigned* exitIndex) {
        MOZ_ASSERT(isFinishedWithModulePrologue() && !isFinishedWithFunctionBodies());
        if (SIZE_MAX - pod.funcPtrTableAndExitBytes_ < sizeof(ExitDatum))
            return false;
        uint32_t globalDataOffset = globalDataBytes();
        JS_STATIC_ASSERT(sizeof(ExitDatum) % sizeof(void*) == 0);
        pod.funcPtrTableAndExitBytes_ += sizeof(ExitDatum);
        *exitIndex = unsigned(exits_.length());
        return exits_.append(Exit(ffiIndex, globalDataOffset));
    }
    unsigned numExits() const {
        MOZ_ASSERT(isFinishedWithModulePrologue());
        return exits_.length();
    }
    Exit& exit(unsigned i) {
        MOZ_ASSERT(isFinishedWithModulePrologue());
        return exits_[i];
    }
    const Exit& exit(unsigned i) const {
        MOZ_ASSERT(isFinishedWithModulePrologue());
        return exits_[i];
    }
    bool addFuncPtrTable(unsigned numElems, uint32_t* globalDataOffset) {
        MOZ_ASSERT(isFinishedWithModulePrologue() && !isFinished());
        MOZ_ASSERT(IsPowerOfTwo(numElems));
        if (SIZE_MAX - pod.funcPtrTableAndExitBytes_ < numElems * sizeof(void*))
            return false;
        *globalDataOffset = globalDataBytes();
        if (!funcPtrTables_.append(FuncPtrTable(*globalDataOffset, numElems)))
            return false;
        pod.funcPtrTableAndExitBytes_ += numElems * sizeof(void*);
        return true;
    }
    bool addFunctionCounts(jit::IonScriptCounts* counts) {
        MOZ_ASSERT(isFinishedWithModulePrologue() && !isFinishedWithFunctionBodies());
        return functionCounts_.append(counts);
    }
#if defined(MOZ_VTUNE) || defined(JS_ION_PERF)
    bool addProfiledFunction(PropertyName* name, unsigned codeStart, unsigned codeEnd,
                             unsigned line, unsigned column)
    {
        MOZ_ASSERT(isFinishedWithModulePrologue() && !isFinishedWithFunctionBodies());
        ProfiledFunction func(name, codeStart, codeEnd, line, column);
        return profiledFunctions_.append(func);
    }
    unsigned numProfiledFunctions() const {
        MOZ_ASSERT(isFinishedWithModulePrologue());
        return profiledFunctions_.length();
    }
    ProfiledFunction& profiledFunction(unsigned i) {
        MOZ_ASSERT(isFinishedWithModulePrologue());
        return profiledFunctions_[i];
    }
#endif
#ifdef JS_ION_PERF
    bool addProfiledBlocks(PropertyName* name, unsigned codeBegin, unsigned inlineEnd,
                           unsigned codeEnd, jit::BasicBlocksVector& basicBlocks)
    {
        MOZ_ASSERT(isFinishedWithModulePrologue() && !isFinishedWithFunctionBodies());
        ProfiledBlocksFunction func(name, codeBegin, inlineEnd, codeEnd, basicBlocks);
        return perfProfiledBlocksFunctions_.append(mozilla::Move(func));
    }
    unsigned numPerfBlocksFunctions() const {
        MOZ_ASSERT(isFinishedWithModulePrologue());
        return perfProfiledBlocksFunctions_.length();
    }
    ProfiledBlocksFunction& perfProfiledBlocksFunction(unsigned i) {
        MOZ_ASSERT(isFinishedWithModulePrologue());
        return perfProfiledBlocksFunctions_[i];
    }
#endif

    

    
    
    void finishFunctionBodies(size_t functionBytes) {
        MOZ_ASSERT(isFinishedWithModulePrologue() && !isFinishedWithFunctionBodies());
        pod.functionBytes_ = functionBytes;
        MOZ_ASSERT(isFinishedWithFunctionBodies());
    }

    
    
    
    

    bool addExportedFunction(PropertyName* name,
                             uint32_t funcSrcBegin,
                             uint32_t funcSrcEnd,
                             PropertyName* maybeFieldName,
                             ArgCoercionVector&& argCoercions,
                             ReturnType returnType)
    {
        
        
        
        MOZ_ASSERT(isFinishedWithFunctionBodies() && !isFinished());
        MOZ_ASSERT(srcStart_ < funcSrcBegin);
        MOZ_ASSERT(funcSrcBegin < funcSrcEnd);
        ExportedFunction func(name, funcSrcBegin - srcStart_, funcSrcEnd - srcStart_,
                              maybeFieldName, mozilla::Move(argCoercions), returnType);
        return exports_.length() < UINT32_MAX && exports_.append(mozilla::Move(func));
    }
    bool addExportedChangeHeap(PropertyName* name,
                               uint32_t funcSrcBegin,
                               uint32_t funcSrcEnd,
                               PropertyName* maybeFieldName)
    {
        
        MOZ_ASSERT(isFinishedWithFunctionBodies() && !isFinished());
        MOZ_ASSERT(srcStart_ < funcSrcBegin);
        MOZ_ASSERT(funcSrcBegin < funcSrcEnd);
        ExportedFunction func(name, funcSrcBegin - srcStart_, funcSrcEnd - srcStart_,
                              maybeFieldName);
        return exports_.length() < UINT32_MAX && exports_.append(mozilla::Move(func));
    }
    unsigned numExportedFunctions() const {
        MOZ_ASSERT(isFinishedWithFunctionBodies());
        return exports_.length();
    }
    const ExportedFunction& exportedFunction(unsigned i) const {
        MOZ_ASSERT(isFinishedWithFunctionBodies());
        return exports_[i];
    }
    ExportedFunction& exportedFunction(unsigned i) {
        MOZ_ASSERT(isFinishedWithFunctionBodies());
        return exports_[i];
    }

    

    
    
    
    
    bool finish(ExclusiveContext* cx,
                frontend::TokenStream& tokenStream,
                jit::MacroAssembler& masm,
                const jit::Label& interruptLabel,
                const jit::Label& outOfBoundsLabel);

    
    

    unsigned numFFIs() const {
        MOZ_ASSERT(isFinished());
        return pod.numFFIs_;
    }
    uint32_t srcEndBeforeCurly() const {
        MOZ_ASSERT(isFinished());
        return srcStart_ + pod.srcLength_;
    }
    uint32_t srcEndAfterCurly() const {
        MOZ_ASSERT(isFinished());
        return srcStart_ + pod.srcLengthWithRightBrace_;
    }
    uint8_t* codeBase() const {
        MOZ_ASSERT(isFinished());
        MOZ_ASSERT(uintptr_t(code_) % AsmJSPageSize == 0);
        return code_;
    }
    size_t functionBytes() const {
        MOZ_ASSERT(isFinished());
        return pod.functionBytes_;
    }
    size_t codeBytes() const {
        MOZ_ASSERT(isFinished());
        return pod.codeBytes_;
    }
    bool containsFunctionPC(void* pc) const {
        MOZ_ASSERT(isFinished());
        return pc >= code_ && pc < (code_ + functionBytes());
    }
    bool containsCodePC(void* pc) const {
        MOZ_ASSERT(isFinished());
        return pc >= code_ && pc < (code_ + codeBytes());
    }
  private:
    uint8_t* interpExitTrampoline(const Exit& exit) const {
        MOZ_ASSERT(isFinished());
        MOZ_ASSERT(exit.interpCodeOffset_);
        return code_ + exit.interpCodeOffset_;
    }
    uint8_t* jitExitTrampoline(const Exit& exit) const {
        MOZ_ASSERT(isFinished());
        MOZ_ASSERT(exit.jitCodeOffset_);
        return code_ + exit.jitCodeOffset_;
    }
  public:

    
    
    const jit::CallSite* lookupCallSite(void* returnAddress) const;

    
    
    const CodeRange* lookupCodeRange(void* pc) const;

    
    
    const jit::AsmJSHeapAccess* lookupHeapAccess(void* pc) const;

    
    
    
    
    
    
    
    
    
    
    
    
    size_t offsetOfGlobalData() const {
        MOZ_ASSERT(isFinished());
        return pod.codeBytes_;
    }
    uint8_t* globalData() const {
        MOZ_ASSERT(isFinished());
        return code_ + offsetOfGlobalData();
    }
    size_t globalSimdVarsOffset() const {
        return AlignBytes( sizeof(void*) +
                           sizeof(void*) +
                           sizeof(double) +
                           sizeof(float),
                          jit::Simd128DataSize);
    }
    size_t globalDataBytes() const {
        return globalSimdVarsOffset() +
                pod.numGlobalSimdVars_ * jit::Simd128DataSize +
                pod.numGlobalScalarVars_ * sizeof(uint64_t) +
                pod.funcPtrTableAndExitBytes_;
    }
    static unsigned activationGlobalDataOffset() {
        JS_STATIC_ASSERT(jit::AsmJSActivationGlobalDataOffset == 0);
        return 0;
    }
    AsmJSActivation*& activation() const {
        return *(AsmJSActivation**)(globalData() + activationGlobalDataOffset());
    }
    bool active() const {
        return activation() != nullptr;
    }
    static unsigned heapGlobalDataOffset() {
        JS_STATIC_ASSERT(jit::AsmJSHeapGlobalDataOffset == sizeof(void*));
        return sizeof(void*);
    }
    uint8_t*& heapDatum() const {
        MOZ_ASSERT(isFinished());
        return *(uint8_t**)(globalData() + heapGlobalDataOffset());
    }
    static unsigned nan64GlobalDataOffset() {
        static_assert(jit::AsmJSNaN64GlobalDataOffset % sizeof(double) == 0,
                      "Global data NaN should be aligned");
        return heapGlobalDataOffset() + sizeof(void*);
    }
    static unsigned nan32GlobalDataOffset() {
        static_assert(jit::AsmJSNaN32GlobalDataOffset % sizeof(double) == 0,
                      "Global data NaN should be aligned");
        return nan64GlobalDataOffset() + sizeof(double);
    }
    void initGlobalNaN() {
        MOZ_ASSERT(jit::AsmJSNaN64GlobalDataOffset == nan64GlobalDataOffset());
        MOZ_ASSERT(jit::AsmJSNaN32GlobalDataOffset == nan32GlobalDataOffset());
        *(double*)(globalData() + nan64GlobalDataOffset()) = GenericNaN();
        *(float*)(globalData() + nan32GlobalDataOffset()) = GenericNaN();
    }
    unsigned globalSimdVarIndexToGlobalDataOffset(unsigned i) const {
        MOZ_ASSERT(isFinishedWithModulePrologue());
        MOZ_ASSERT(i < pod.numGlobalSimdVars_);
        return globalSimdVarsOffset() +
               i * jit::Simd128DataSize;
    }
    unsigned globalScalarVarIndexToGlobalDataOffset(unsigned i) const {
        MOZ_ASSERT(isFinishedWithModulePrologue());
        MOZ_ASSERT(i < pod.numGlobalScalarVars_);
        return globalSimdVarsOffset() +
               pod.numGlobalSimdVars_ * jit::Simd128DataSize +
               i * sizeof(uint64_t);
    }
    void* globalScalarVarIndexToGlobalDatum(unsigned i) const {
        MOZ_ASSERT(isFinished());
        return (void*)(globalData() + globalScalarVarIndexToGlobalDataOffset(i));
    }
    void* globalSimdVarIndexToGlobalDatum(unsigned i) const {
        MOZ_ASSERT(isFinished());
        return (void*)(globalData() + globalSimdVarIndexToGlobalDataOffset(i));
    }
    void* globalVarToGlobalDatum(const Global& g) const {
        unsigned index = g.varIndex();
        if (g.varInitKind() == Global::VarInitKind::InitConstant) {
            return g.varInitNumLit().isSimd()
                   ? globalSimdVarIndexToGlobalDatum(index)
                   : globalScalarVarIndexToGlobalDatum(index);
        }

        MOZ_ASSERT(g.varInitKind() == Global::VarInitKind::InitImport);
        return IsSimdCoercion(g.varInitCoercion())
               ? globalSimdVarIndexToGlobalDatum(index)
               : globalScalarVarIndexToGlobalDatum(index);
    }
    uint8_t** globalDataOffsetToFuncPtrTable(unsigned globalDataOffset) const {
        MOZ_ASSERT(isFinished());
        MOZ_ASSERT(globalDataOffset < globalDataBytes());
        return (uint8_t**)(globalData() + globalDataOffset);
    }
    unsigned exitIndexToGlobalDataOffset(unsigned exitIndex) const {
        MOZ_ASSERT(isFinishedWithModulePrologue());
        return exits_[exitIndex].globalDataOffset();
    }
    ExitDatum& exitIndexToGlobalDatum(unsigned exitIndex) const {
        MOZ_ASSERT(isFinished());
        return *(ExitDatum*)(globalData() + exitIndexToGlobalDataOffset(exitIndex));
    }
    bool exitIsOptimized(unsigned exitIndex) const {
        MOZ_ASSERT(isFinished());
        ExitDatum& exitDatum = exitIndexToGlobalDatum(exitIndex);
        return exitDatum.exit != interpExitTrampoline(exit(exitIndex));
    }
    void optimizeExit(unsigned exitIndex, jit::BaselineScript* baselineScript) const {
        MOZ_ASSERT(!exitIsOptimized(exitIndex));
        ExitDatum& exitDatum = exitIndexToGlobalDatum(exitIndex);
        exitDatum.exit = jitExitTrampoline(exit(exitIndex));
        exitDatum.baselineScript = baselineScript;
    }
    void detachJitCompilation(size_t exitIndex) const {
        MOZ_ASSERT(isFinished());
        ExitDatum& exitDatum = exitIndexToGlobalDatum(exitIndex);
        exitDatum.exit = interpExitTrampoline(exit(exitIndex));
        exitDatum.baselineScript = nullptr;
    }

    
    

    bool addRelativeLink(RelativeLink link) {
        MOZ_ASSERT(isFinished() && !isStaticallyLinked());
        return staticLinkData_.relativeLinks.append(link);
    }

    
    
    
    size_t serializedSize() const;
    uint8_t* serialize(uint8_t* cursor) const;
    const uint8_t* deserialize(ExclusiveContext* cx, const uint8_t* cursor);

    
    
    
    void setAutoFlushICacheRange();

    

    
    
    
    void staticallyLink(ExclusiveContext* cx);

    
    
    
    
    
    
    
    void setIsDynamicallyLinked(JSRuntime* rt) {
        MOZ_ASSERT(!isDynamicallyLinked());
        dynamicallyLinked_ = true;
        nextLinked_ = rt->linkedAsmJSModules;
        prevLinked_ = &rt->linkedAsmJSModules;
        if (nextLinked_)
            nextLinked_->prevLinked_ = &nextLinked_;
        rt->linkedAsmJSModules = this;
        MOZ_ASSERT(isDynamicallyLinked());
    }

    void initHeap(Handle<ArrayBufferObjectMaybeShared*> heap, JSContext* cx);
    bool changeHeap(Handle<ArrayBufferObject*> newHeap, JSContext* cx);
    bool detachHeap(JSContext* cx);

    bool clone(JSContext* cx, ScopedJSDeletePtr<AsmJSModule>* moduleOut) const;

    
    

    AsmJSModule* nextLinked() const {
        MOZ_ASSERT(isDynamicallyLinked());
        return nextLinked_;
    }
    bool hasDetachedHeap() const {
        MOZ_ASSERT(isDynamicallyLinked());
        return hasArrayView() && !heapDatum();
    }
    CodePtr entryTrampoline(const ExportedFunction& func) const {
        MOZ_ASSERT(isDynamicallyLinked());
        MOZ_ASSERT(!func.isChangeHeap());
        return JS_DATA_TO_FUNC_PTR(CodePtr, code_ + func.pod.codeOffset_);
    }
    uint8_t* interruptExit() const {
        MOZ_ASSERT(isDynamicallyLinked());
        return interruptExit_;
    }
    uint8_t* outOfBoundsExit() const {
        MOZ_ASSERT(isDynamicallyLinked());
        return outOfBoundsExit_;
    }
    uint8_t* maybeHeap() const {
        MOZ_ASSERT(isDynamicallyLinked());
        return heapDatum();
    }
    ArrayBufferObjectMaybeShared* maybeHeapBufferObject() const {
        MOZ_ASSERT(isDynamicallyLinked());
        return maybeHeap_;
    }
    size_t heapLength() const {
        MOZ_ASSERT(isDynamicallyLinked());
        return maybeHeap_ ? maybeHeap_->byteLength() : 0;
    }
    bool profilingEnabled() const {
        MOZ_ASSERT(isDynamicallyLinked());
        return profilingEnabled_;
    }
    void setProfilingEnabled(bool enabled, JSContext* cx);
    void setInterrupted(bool interrupted) {
        MOZ_ASSERT(isDynamicallyLinked());
        interrupted_ = interrupted;
    }
};


extern JS::AsmJSCacheResult
StoreAsmJSModuleInCache(AsmJSParser& parser,
                        const AsmJSModule& module,
                        ExclusiveContext* cx);





extern bool
LookupAsmJSModuleInCache(ExclusiveContext* cx,
                         AsmJSParser& parser,
                         ScopedJSDeletePtr<AsmJSModule>* module,
                         ScopedJSFreePtr<char>* compilationTimeReport);


extern bool
OnDetachAsmJSArrayBuffer(JSContext* cx, Handle<ArrayBufferObject*> buffer);





class AsmJSModuleObject : public NativeObject
{
    static const unsigned MODULE_SLOT = 0;

  public:
    static const unsigned RESERVED_SLOTS = 1;

    
    
    static AsmJSModuleObject* create(ExclusiveContext* cx, ScopedJSDeletePtr<AsmJSModule>* module);

    AsmJSModule& module() const;

    void addSizeOfMisc(mozilla::MallocSizeOf mallocSizeOf, size_t* asmJSModuleCode,
                       size_t* asmJSModuleData) {
        module().addSizeOfMisc(mallocSizeOf, asmJSModuleCode, asmJSModuleData);
    }

    static const Class class_;
};

}  

#endif 
