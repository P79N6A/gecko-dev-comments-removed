





#ifndef ion_AsmJSModule_h
#define ion_AsmJSModule_h

#ifdef JS_ION

#include "gc/Marking.h"
#include "ion/RegisterSets.h"

#include "jsscript.h"
#include "jstypedarrayinlines.h"

#if defined(JS_ION_PERF)
# include "ion/PerfSpewer.h"
#endif

#include "ion/IonMacroAssembler.h"

namespace js {


enum AsmJSCoercion
{
    AsmJS_ToInt32,
    AsmJS_ToNumber
};


enum AsmJSMathBuiltin
{
    AsmJSMathBuiltin_sin, AsmJSMathBuiltin_cos, AsmJSMathBuiltin_tan,
    AsmJSMathBuiltin_asin, AsmJSMathBuiltin_acos, AsmJSMathBuiltin_atan,
    AsmJSMathBuiltin_ceil, AsmJSMathBuiltin_floor, AsmJSMathBuiltin_exp,
    AsmJSMathBuiltin_log, AsmJSMathBuiltin_pow, AsmJSMathBuiltin_sqrt,
    AsmJSMathBuiltin_abs, AsmJSMathBuiltin_atan2, AsmJSMathBuiltin_imul
};











class AsmJSModule
{
  public:
    class Global
    {
      public:
        enum Which { Variable, FFI, ArrayView, MathBuiltin, Constant };
        enum VarInitKind { InitConstant, InitImport };

      private:
        Which which_;
        union {
            struct {
                uint32_t index_;
                VarInitKind initKind_;
                union {
                    Value constant_; 
                    AsmJSCoercion coercion_;
                } init;
            } var;
            uint32_t ffiIndex_;
            ArrayBufferView::ViewType viewType_;
            AsmJSMathBuiltin mathBuiltin_;
            double constantValue_;
        } u;
        RelocatablePtr<PropertyName> name_;

        friend class AsmJSModule;
        Global(Which which) : which_(which) {}

        void trace(JSTracer *trc) {
            if (name_)
                MarkString(trc, &name_, "asm.js global name");
            JS_ASSERT_IF(which_ == Variable && u.var.initKind_ == InitConstant,
                         !u.var.init.constant_.isMarkable());
        }

      public:
        Which which() const {
            return which_;
        }
        uint32_t varIndex() const {
            JS_ASSERT(which_ == Variable);
            return u.var.index_;
        }
        VarInitKind varInitKind() const {
            JS_ASSERT(which_ == Variable);
            return u.var.initKind_;
        }
        const Value &varInitConstant() const {
            JS_ASSERT(which_ == Variable);
            JS_ASSERT(u.var.initKind_ == InitConstant);
            return u.var.init.constant_;
        }
        AsmJSCoercion varImportCoercion() const {
            JS_ASSERT(which_ == Variable);
            JS_ASSERT(u.var.initKind_ == InitImport);
            return u.var.init.coercion_;
        }
        PropertyName *varImportField() const {
            JS_ASSERT(which_ == Variable);
            JS_ASSERT(u.var.initKind_ == InitImport);
            return name_;
        }
        PropertyName *ffiField() const {
            JS_ASSERT(which_ == FFI);
            return name_;
        }
        uint32_t ffiIndex() const {
            JS_ASSERT(which_ == FFI);
            return u.ffiIndex_;
        }
        PropertyName *viewName() const {
            JS_ASSERT(which_ == ArrayView);
            return name_;
        }
        ArrayBufferView::ViewType viewType() const {
            JS_ASSERT(which_ == ArrayView);
            return u.viewType_;
        }
        PropertyName *mathName() const {
            JS_ASSERT(which_ == MathBuiltin);
            return name_;
        }
        AsmJSMathBuiltin mathBuiltin() const {
            JS_ASSERT(which_ == MathBuiltin);
            return u.mathBuiltin_;
        }
        PropertyName *constantName() const {
            JS_ASSERT(which_ == Constant);
            return name_;
        }
        double constantValue() const {
            JS_ASSERT(which_ == Constant);
            return u.constantValue_;
        }
    };

    class Exit
    {
        unsigned ffiIndex_;

        union {
            unsigned codeOffset_;
            uint8_t *code_;
        } interp;

        union {
            unsigned codeOffset_;
            uint8_t *code_;
        } ion;

      public:
        Exit(unsigned ffiIndex)
          : ffiIndex_(ffiIndex)
        {
          interp.codeOffset_ = 0;
          ion.codeOffset_ = 0;
        }
        unsigned ffiIndex() const {
            return ffiIndex_;
        }
        void initInterpOffset(unsigned off) {
            JS_ASSERT(!interp.codeOffset_);
            interp.codeOffset_ = off;
        }
        void initIonOffset(unsigned off) {
            JS_ASSERT(!ion.codeOffset_);
            ion.codeOffset_ = off;
        }
        void patch(uint8_t *baseAddress) {
            interp.code_ = baseAddress + interp.codeOffset_;
            ion.code_ = baseAddress + ion.codeOffset_;
        }
        uint8_t *interpCode() const {
            return interp.code_;
        }
        uint8_t *ionCode() const {
            return ion.code_;
        }
    };
#ifdef JS_CPU_ARM
    typedef int32_t (*CodePtr)(uint64_t *args, uint8_t *global);
#else
    typedef int32_t (*CodePtr)(uint64_t *args);
#endif

    typedef Vector<AsmJSCoercion, 0, SystemAllocPolicy> ArgCoercionVector;

    enum ReturnType { Return_Int32, Return_Double, Return_Void };

    class ExportedFunction
    {
      public:

      private:

        RelocatablePtr<JSFunction> fun_;
        RelocatablePtr<PropertyName> maybeFieldName_;
        ArgCoercionVector argCoercions_;
        ReturnType returnType_;
        bool hasCodePtr_;
        union {
            unsigned codeOffset_;
            CodePtr code_;
        } u;

        friend class AsmJSModule;

        ExportedFunction(JSFunction *fun,
                         PropertyName *maybeFieldName,
                         MoveRef<ArgCoercionVector> argCoercions,
                         ReturnType returnType)
          : fun_(fun),
            maybeFieldName_(maybeFieldName),
            argCoercions_(argCoercions),
            returnType_(returnType),
            hasCodePtr_(false)
        {
            u.codeOffset_ = 0;
        }

        void trace(JSTracer *trc) {
            MarkObject(trc, &fun_, "asm.js export name");
            if (maybeFieldName_)
                MarkString(trc, &maybeFieldName_, "asm.js export field");
        }

      public:
        ExportedFunction(MoveRef<ExportedFunction> rhs)
          : fun_(rhs->fun_),
            maybeFieldName_(rhs->maybeFieldName_),
            argCoercions_(Move(rhs->argCoercions_)),
            returnType_(rhs->returnType_),
            hasCodePtr_(rhs->hasCodePtr_),
            u(rhs->u)
        {}

        void initCodeOffset(unsigned off) {
            JS_ASSERT(!hasCodePtr_);
            JS_ASSERT(!u.codeOffset_);
            u.codeOffset_ = off;
        }
        void patch(uint8_t *baseAddress) {
            JS_ASSERT(!hasCodePtr_);
            JS_ASSERT(u.codeOffset_);
            hasCodePtr_ = true;
            u.code_ = JS_DATA_TO_FUNC_PTR(CodePtr, baseAddress + u.codeOffset_);
        }

        PropertyName *name() const {
            return fun_->name();
        }
        JSFunction *unclonedFunObj() const {
            return fun_;
        }
        PropertyName *maybeFieldName() const {
            return maybeFieldName_;
        }
        unsigned numArgs() const {
            return argCoercions_.length();
        }
        AsmJSCoercion argCoercion(unsigned i) const {
            return argCoercions_[i];
        }
        ReturnType returnType() const {
            return returnType_;
        }
        CodePtr code() const {
            JS_ASSERT(hasCodePtr_);
            return u.code_;
        }
    };

#if defined(MOZ_VTUNE) or defined(JS_ION_PERF)
    
    struct ProfiledFunction
    {
        JSAtom *name;
        unsigned startCodeOffset;
        unsigned endCodeOffset;
        unsigned lineno;
        unsigned columnIndex;

        ProfiledFunction(JSAtom *name, unsigned start, unsigned end,
                         unsigned line = 0U, unsigned column = 0U)
          : name(name),
            startCodeOffset(start),
            endCodeOffset(end),
            lineno(line),
            columnIndex(column)
        { }
    };
#endif

#if defined(JS_ION_PERF)
    struct ProfiledBlocksFunction : public ProfiledFunction
    {
        ion::PerfSpewer::BasicBlocksVector blocks;

        ProfiledBlocksFunction(JSAtom *name, unsigned start, unsigned end, ion::PerfSpewer::BasicBlocksVector &blocksVector)
          : ProfiledFunction(name, start, end), blocks(Move(blocksVector))
        { }

        ProfiledBlocksFunction(const ProfiledBlocksFunction &copy)
          : ProfiledFunction(copy.name, copy.startCodeOffset, copy.endCodeOffset), blocks(Move(copy.blocks))
        { }
    };
#endif

    
    
    struct PostLinkFailureInfo
    {
        CompileOptions      options_;
        ScriptSource *      scriptSource_;
        uint32_t            bufStart_;      
        uint32_t            bufEnd_;        

        PostLinkFailureInfo(JSContext *cx)
          : options_(cx),
            scriptSource_(),
            bufStart_(),
            bufEnd_()
        { }

        void init(CompileOptions options, ScriptSource *scriptSource,
                  uint32_t bufStart, uint32_t bufEnd)
        {
            options_      = options;
            scriptSource_ = scriptSource;
            bufStart_     = bufStart;
            bufEnd_       = bufEnd;

            scriptSource_->incref();
        }

        ~PostLinkFailureInfo() {
            if (scriptSource_)
                scriptSource_->decref();
        }
    };

  private:
    typedef Vector<ExportedFunction, 0, SystemAllocPolicy> ExportedFunctionVector;
    typedef Vector<Global, 0, SystemAllocPolicy> GlobalVector;
    typedef Vector<Exit, 0, SystemAllocPolicy> ExitVector;
    typedef Vector<ion::AsmJSHeapAccess, 0, SystemAllocPolicy> HeapAccessVector;
#if defined(JS_CPU_ARM)
    typedef Vector<ion::AsmJSBoundsCheck, 0, SystemAllocPolicy> BoundsCheckVector;
#endif
    typedef Vector<ion::IonScriptCounts *, 0, SystemAllocPolicy> FunctionCountsVector;
#if defined(MOZ_VTUNE) or defined(JS_ION_PERF)
    typedef Vector<ProfiledFunction, 0, SystemAllocPolicy> ProfiledFunctionVector;
#endif

    GlobalVector                          globals_;
    ExitVector                            exits_;
    ExportedFunctionVector                exports_;
    HeapAccessVector                      heapAccesses_;
#if defined(JS_CPU_ARM)
    BoundsCheckVector                     boundsChecks_;
#endif
#if defined(MOZ_VTUNE)
    ProfiledFunctionVector                profiledFunctions_;
#endif
#if defined(JS_ION_PERF)
    ProfiledFunctionVector                perfProfiledFunctions_;
    Vector<ProfiledBlocksFunction, 0, SystemAllocPolicy> perfProfiledBlocksFunctions_;
#endif

    uint32_t                              numGlobalVars_;
    uint32_t                              numFFIs_;
    uint32_t                              numFuncPtrTableElems_;
    bool                                  hasArrayView_;

    ScopedReleasePtr<JSC::ExecutablePool> codePool_;
    uint8_t *                             code_;
    uint8_t *                             operationCallbackExit_;
    size_t                                functionBytes_;
    size_t                                codeBytes_;
    size_t                                totalBytes_;

    bool                                  linked_;
    HeapPtr<ArrayBufferObject>            maybeHeap_;

    HeapPtrPropertyName                   globalArgumentName_;
    HeapPtrPropertyName                   importArgumentName_;
    HeapPtrPropertyName                   bufferArgumentName_;

    PostLinkFailureInfo                   postLinkFailureInfo_;

    FunctionCountsVector                  functionCounts_;

  public:
    explicit AsmJSModule(JSContext *cx)
      : numGlobalVars_(0),
        numFFIs_(0),
        numFuncPtrTableElems_(0),
        hasArrayView_(false),
        code_(NULL),
        operationCallbackExit_(NULL),
        functionBytes_(0),
        codeBytes_(0),
        totalBytes_(0),
        linked_(false),
        maybeHeap_(),
        postLinkFailureInfo_(cx)
    {}

    ~AsmJSModule();

    void trace(JSTracer *trc) {
        for (unsigned i = 0; i < globals_.length(); i++)
            globals_[i].trace(trc);
        for (unsigned i = 0; i < exports_.length(); i++)
            exports_[i].trace(trc);
        for (unsigned i = 0; i < exits_.length(); i++) {
            if (exitIndexToGlobalDatum(i).fun)
                MarkObject(trc, &exitIndexToGlobalDatum(i).fun, "asm.js imported function");
        }
        if (maybeHeap_)
            MarkObject(trc, &maybeHeap_, "asm.js heap");

        if (globalArgumentName_)
            MarkString(trc, &globalArgumentName_, "asm.js global argument name");
        if (importArgumentName_)
            MarkString(trc, &importArgumentName_, "asm.js import argument name");
        if (bufferArgumentName_)
            MarkString(trc, &bufferArgumentName_, "asm.js buffer argument name");
    }

    bool addGlobalVarInitConstant(const Value &v, uint32_t *globalIndex) {
        JS_ASSERT(!v.isMarkable());
        if (numGlobalVars_ == UINT32_MAX)
            return false;
        Global g(Global::Variable);
        g.u.var.initKind_ = Global::InitConstant;
        g.u.var.init.constant_ = v;
        g.u.var.index_ = *globalIndex = numGlobalVars_++;
        return globals_.append(g);
    }
    bool addGlobalVarImport(PropertyName *fieldName, AsmJSCoercion coercion, uint32_t *globalIndex) {
        Global g(Global::Variable);
        g.u.var.initKind_ = Global::InitImport;
        g.u.var.init.coercion_ = coercion;
        g.u.var.index_ = *globalIndex = numGlobalVars_++;
        g.name_ = fieldName;
        return globals_.append(g);
    }
    bool incrementNumFuncPtrTableElems(uint32_t numElems) {
        if (UINT32_MAX - numFuncPtrTableElems_ < numElems)
            return false;
        numFuncPtrTableElems_ += numElems;
        return true;
    }
    bool addFFI(PropertyName *field, uint32_t *ffiIndex) {
        if (numFFIs_ == UINT32_MAX)
            return false;
        Global g(Global::FFI);
        g.u.ffiIndex_ = *ffiIndex = numFFIs_++;
        g.name_ = field;
        return globals_.append(g);
    }
    bool addArrayView(ArrayBufferView::ViewType vt, PropertyName *field) {
        hasArrayView_ = true;
        Global g(Global::ArrayView);
        g.u.viewType_ = vt;
        g.name_ = field;
        return globals_.append(g);
    }
    bool addMathBuiltin(AsmJSMathBuiltin mathBuiltin, PropertyName *field) {
        Global g(Global::MathBuiltin);
        g.u.mathBuiltin_ = mathBuiltin;
        g.name_ = field;
        return globals_.append(g);
    }
    bool addGlobalConstant(double value, PropertyName *fieldName) {
        Global g(Global::Constant);
        g.u.constantValue_ = value;
        g.name_ = fieldName;
        return globals_.append(g);
    }
    bool addExit(unsigned ffiIndex, unsigned *exitIndex) {
        *exitIndex = unsigned(exits_.length());
        return exits_.append(Exit(ffiIndex));
    }
    bool addFunctionCounts(ion::IonScriptCounts *counts) {
        return functionCounts_.append(counts);
    }

    bool addExportedFunction(JSFunction *fun, PropertyName *maybeFieldName,
                             MoveRef<ArgCoercionVector> argCoercions, ReturnType returnType)
    {
        ExportedFunction func(fun, maybeFieldName, argCoercions, returnType);
        return exports_.append(Move(func));
    }
    unsigned numExportedFunctions() const {
        return exports_.length();
    }
    const ExportedFunction &exportedFunction(unsigned i) const {
        return exports_[i];
    }
    ExportedFunction &exportedFunction(unsigned i) {
        return exports_[i];
    }
#ifdef MOZ_VTUNE
    bool trackProfiledFunction(JSAtom *name, unsigned startCodeOffset, unsigned endCodeOffset) {
        ProfiledFunction func(name, startCodeOffset, endCodeOffset);
        return profiledFunctions_.append(func);
    }
    unsigned numProfiledFunctions() const {
        return profiledFunctions_.length();
    }
    const ProfiledFunction &profiledFunction(unsigned i) const {
        return profiledFunctions_[i];
    }
#endif
#ifdef JS_ION_PERF
    bool trackPerfProfiledFunction(JSAtom *name, unsigned startCodeOffset, unsigned endCodeOffset,
                                   unsigned line, unsigned column)
    {
        ProfiledFunction func(name, startCodeOffset, endCodeOffset, line, column);
        return perfProfiledFunctions_.append(func);
    }
    unsigned numPerfFunctions() const {
        return perfProfiledFunctions_.length();
    }
    const ProfiledFunction &perfProfiledFunction(unsigned i) const {
        return perfProfiledFunctions_[i];
    }

    bool trackPerfProfiledBlocks(JSAtom *name, unsigned startCodeOffset, unsigned endCodeOffset, ion::PerfSpewer::BasicBlocksVector &basicBlocks) {
        ProfiledBlocksFunction func(name, startCodeOffset, endCodeOffset, basicBlocks);
        return perfProfiledBlocksFunctions_.append(func);
    }
    unsigned numPerfBlocksFunctions() const {
        return perfProfiledBlocksFunctions_.length();
    }
    const ProfiledBlocksFunction perfProfiledBlocksFunction(unsigned i) const {
        return perfProfiledBlocksFunctions_[i];
    }
#endif
    bool hasArrayView() const {
        return hasArrayView_;
    }
    unsigned numFFIs() const {
        return numFFIs_;
    }
    unsigned numGlobalVars() const {
        return numGlobalVars_;
    }
    unsigned numGlobals() const {
        return globals_.length();
    }
    Global &global(unsigned i) {
        return globals_[i];
    }
    unsigned numFuncPtrTableElems() const {
        return numFuncPtrTableElems_;
    }
    unsigned numExits() const {
        return exits_.length();
    }
    Exit &exit(unsigned i) {
        return exits_[i];
    }
    const Exit &exit(unsigned i) const {
        return exits_[i];
    }
    unsigned numFunctionCounts() const {
        return functionCounts_.length();
    }
    ion::IonScriptCounts *functionCounts(unsigned i) {
        return functionCounts_[i];
    }

    
    
    
    struct ExitDatum
    {
        uint8_t *exit;
        HeapPtrFunction fun;
    };

    
    
    
    
    
    
    
    
    
    
    
    
    uint8_t *globalData() const {
        JS_ASSERT(code_);
        return code_ + codeBytes_;
    }

    size_t globalDataBytes() const {
        return sizeof(void*) +
               numGlobalVars_ * sizeof(uint64_t) +
               numFuncPtrTableElems_ * sizeof(void*) +
               exits_.length() * sizeof(ExitDatum);
    }
    unsigned heapOffset() const {
        return 0;
    }
    uint8_t *&heapDatum() const {
        return *(uint8_t**)(globalData() + heapOffset());
    }
    unsigned globalVarIndexToGlobalDataOffset(unsigned i) const {
        JS_ASSERT(i < numGlobalVars_);
        return sizeof(void*) +
               i * sizeof(uint64_t);
    }
    void *globalVarIndexToGlobalDatum(unsigned i) const {
        return (void *)(globalData() + globalVarIndexToGlobalDataOffset(i));
    }
    unsigned funcPtrIndexToGlobalDataOffset(unsigned i) const {
        return sizeof(void*) +
               numGlobalVars_ * sizeof(uint64_t) +
               i * sizeof(void*);
    }
    void *&funcPtrIndexToGlobalDatum(unsigned i) const {
        return *(void **)(globalData() + funcPtrIndexToGlobalDataOffset(i));
    }
    unsigned exitIndexToGlobalDataOffset(unsigned exitIndex) const {
        JS_ASSERT(exitIndex < exits_.length());
        return sizeof(void*) +
               numGlobalVars_ * sizeof(uint64_t) +
               numFuncPtrTableElems_ * sizeof(void*) +
               exitIndex * sizeof(ExitDatum);
    }
    ExitDatum &exitIndexToGlobalDatum(unsigned exitIndex) const {
        return *(ExitDatum *)(globalData() + exitIndexToGlobalDataOffset(exitIndex));
    }

    void setFunctionBytes(size_t functionBytes) {
        JS_ASSERT(functionBytes % AsmJSPageSize == 0);
        functionBytes_ = functionBytes;
    }
    size_t functionBytes() const {
        JS_ASSERT(functionBytes_);
        JS_ASSERT(functionBytes_ % AsmJSPageSize == 0);
        return functionBytes_;
    }
    bool containsPC(void *pc) const {
        uint8_t *code = functionCode();
        return pc >= code && pc < (code + functionBytes());
    }

    bool addHeapAccesses(const ion::AsmJSHeapAccessVector &accesses) {
        return heapAccesses_.append(accesses);
    }
    unsigned numHeapAccesses() const {
        return heapAccesses_.length();
    }
    ion::AsmJSHeapAccess &heapAccess(unsigned i) {
        return heapAccesses_[i];
    }
    const ion::AsmJSHeapAccess &heapAccess(unsigned i) const {
        return heapAccesses_[i];
    }
#if defined(JS_CPU_ARM)
    bool addBoundsChecks(const ion::AsmJSBoundsCheckVector &checks) {
        return boundsChecks_.append(checks);
    }
    void convertBoundsChecksToActualOffset(ion::MacroAssembler &masm) {
        for (unsigned i = 0; i < boundsChecks_.length(); i++)
            boundsChecks_[i].setOffset(masm.actualOffset(boundsChecks_[i].offset()));
    }

    void patchBoundsChecks(unsigned heapSize) {
        ion::AutoFlushCache afc("patchBoundsCheck");
        int bits = -1;
        JS_CEILING_LOG2(bits, heapSize);
        if (bits == -1) {
            
            return;
        }

        for (unsigned i = 0; i < boundsChecks_.length(); i++)
            ion::Assembler::updateBoundsCheck(bits, (ion::Instruction*)(boundsChecks_[i].offset() + code_));

    }
    unsigned numBoundsChecks() const {
        return boundsChecks_.length();
    }
    const ion::AsmJSBoundsCheck &boundsCheck(unsigned i) const {
        return boundsChecks_[i];
    }
#endif



    void takeOwnership(JSC::ExecutablePool *pool, uint8_t *code, size_t codeBytes, size_t totalBytes) {
        JS_ASSERT(uintptr_t(code) % AsmJSPageSize == 0);
        codePool_ = pool;
        code_ = code;
        codeBytes_ = codeBytes;
        totalBytes_ = totalBytes;
    }
    uint8_t *functionCode() const {
        JS_ASSERT(code_);
        JS_ASSERT(uintptr_t(code_) % AsmJSPageSize == 0);
        return code_;
    }

    void setOperationCallbackExit(uint8_t *ptr) {
        operationCallbackExit_ = ptr;
    }
    uint8_t *operationCallbackExit() const {
        return operationCallbackExit_;
    }

    void setIsLinked(Handle<ArrayBufferObject*> maybeHeap) {
        JS_ASSERT(!linked_);
        linked_ = true;
        maybeHeap_ = maybeHeap;
        heapDatum() = maybeHeap_ ? maybeHeap_->dataPointer() : NULL;
    }
    bool isLinked() const {
        return linked_;
    }
    uint8_t *maybeHeap() const {
        JS_ASSERT(linked_);
        return heapDatum();
    }
    size_t heapLength() const {
        JS_ASSERT(linked_);
        return maybeHeap_ ? maybeHeap_->byteLength() : 0;
    }

    void initGlobalArgumentName(PropertyName *n) { globalArgumentName_ = n; }
    void initImportArgumentName(PropertyName *n) { importArgumentName_ = n; }
    void initBufferArgumentName(PropertyName *n) { bufferArgumentName_ = n; }

    PropertyName *globalArgumentName() const { return globalArgumentName_; }
    PropertyName *importArgumentName() const { return importArgumentName_; }
    PropertyName *bufferArgumentName() const { return bufferArgumentName_; }

    void initPostLinkFailureInfo(CompileOptions options,
                                 ScriptSource *scriptSource, uint32_t bufStart, uint32_t bufEnd) {
        postLinkFailureInfo_.init(options, scriptSource, bufStart, bufEnd);
    }

    const PostLinkFailureInfo &postLinkFailureInfo() const {
        return postLinkFailureInfo_;
    }

    size_t exitDatumToExitIndex(ExitDatum *exit) const {
        ExitDatum *first = &exitIndexToGlobalDatum(0);
        JS_ASSERT(exit >= first && exit < first + numExits());
        return exit - first;
    }

    void detachIonCompilation(size_t exitIndex) const {
        ExitDatum &exitDatum = exitIndexToGlobalDatum(exitIndex);
        exitDatum.exit = exit(exitIndex).interpCode();
    }
};




extern AsmJSModule &
AsmJSModuleObjectToModule(JSObject *obj);

extern bool
IsAsmJSModuleObject(JSObject *obj);

extern JSObject &
AsmJSModuleObject(JSFunction *moduleFun);

extern void
SetAsmJSModuleObject(JSFunction *moduleFun, JSObject *moduleObj);

}  

#endif  

#endif
