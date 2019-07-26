






#if !defined(jsion_baseline_ic_h__) && defined(JS_ION)
#define jsion_baseline_ic_h__

#include "jscntxt.h"
#include "jscompartment.h"
#include "jsopcode.h"
#include "gc/Heap.h"
#include "BaselineJIT.h"
#include "BaselineRegisters.h"

namespace js {
namespace ion {















































































































































































class ICStub;




class ICEntry
{
  private:
    
    
    uint32_t            returnOffset_;

    
    uint32_t            pcOffset_;

    
    ICStub *            firstStub_;

  public:
    ICEntry(uint32_t pcOffset)
      : returnOffset_(), pcOffset_(pcOffset), firstStub_(NULL)
    {}

    CodeOffsetLabel returnOffset() const {
        return CodeOffsetLabel(returnOffset_);
    }

    void setReturnOffset(CodeOffsetLabel offset) {
        JS_ASSERT(offset.offset() <= (size_t) UINT32_MAX);
        returnOffset_ = (uint32_t) offset.offset();
    }

    void fixupReturnOffset(MacroAssembler &masm) {
        CodeOffsetLabel offset = returnOffset();
        offset.fixup(&masm);
        JS_ASSERT(offset.offset() <= UINT32_MAX);
        returnOffset_ = (uint32_t) offset.offset();
    }

    uint32_t pcOffset() const {
        return pcOffset_;
    }

    jsbytecode *pc(JSScript *script) const {
        return script->code + pcOffset_;
    }

    ICStub *firstStub() const {
        return firstStub_;
    }

    void setFirstStub(ICStub *stub) {
        firstStub_ = stub;
    }

    static inline size_t offsetOfFirstStub() {
        return offsetof(ICEntry, firstStub_);
    }

    inline ICStub **addressOfFirstStub() {
        return &firstStub_;
    }
};


#define IC_STUB_KIND_LIST(_)    \
                                \
    _(TypeMonitor_Fallback)     \
    _(TypeUpdate_Fallback)      \
                                \
    _(Compare_Fallback)         \
    _(Compare_Int32)            \
                                \
    _(ToBool_Fallback)          \
    _(ToBool_Bool)              \
    _(ToBool_Int32)             \
                                \
    _(ToNumber_Fallback)        \
                                \
    _(BinaryArith_Fallback)     \
    _(BinaryArith_Int32)        \
                                \
    _(Call_Fallback)            \
                                \
    _(GetElem_Fallback)         \
    _(GetElem_Dense)            \
                                \
    _(SetElem_Fallback)         \
    _(SetElem_Dense)

#define FORWARD_DECLARE_STUBS(kindName) class IC##kindName;
    IC_STUB_KIND_LIST(FORWARD_DECLARE_STUBS)
#undef FORWARD_DECLARE_STUBS

class ICFallbackStub;
class ICMonitoredStub;
class ICMonitoredFallbackStub;




class ICStub
{
  public:
    enum Kind {
        INVALID = 0,
#define DEF_ENUM_KIND(kindName) kindName,
        IC_STUB_KIND_LIST(DEF_ENUM_KIND)
#undef DEF_ENUM_KIND
        LIMIT
    };

    static inline bool IsValidKind(Kind k) {
        return (k > INVALID) && (k < LIMIT);
    }

  protected:
    
    
    
    bool     isFallback_  : 1;
    bool     isMonitored_ : 1;
    Kind     kind_        : 14;

    
    uint8_t *stubCode_;

    
    
    ICStub *next_;

    inline ICStub(Kind kind, IonCode *stubCode)
      : isFallback_(false),
        isMonitored_(false),
        kind_(kind),
        stubCode_(stubCode->raw()),
        next_(NULL)
    {
        JS_ASSERT(stubCode != NULL);
    }

    inline ICStub(Kind kind, bool isFallback, IonCode *stubCode)
      : isFallback_(isFallback),
        isMonitored_(false),
        kind_(kind),
        stubCode_(stubCode->raw()),
        next_(NULL)
    {
        JS_ASSERT(stubCode != NULL);
    }

    inline ICStub(Kind kind, bool isFallback, bool isMonitored, IonCode *stubCode)
      : isFallback_(isFallback),
        isMonitored_(isMonitored),
        kind_(kind),
        stubCode_(stubCode->raw()),
        next_(NULL)
    {
        JS_ASSERT(stubCode != NULL);
    }

  public:

    inline Kind kind() const {
        return static_cast<Kind>(kind_);
    }

    inline bool isFallback() const {
        return isFallback_;
    }

    inline bool isMonitored() const {
        return isMonitored_ && !isFallback_;
    }

    inline bool isMonitoredFallback() const {
        return isMonitored_ && isFallback_;
    }

    inline const ICFallbackStub *toFallbackStub() const {
        JS_ASSERT(isFallback());
        return reinterpret_cast<const ICFallbackStub *>(this);
    }

    inline ICFallbackStub *toFallbackStub() {
        JS_ASSERT(isFallback());
        return reinterpret_cast<ICFallbackStub *>(this);
    }

    inline const ICMonitoredStub *toMonitoredStub() const {
        JS_ASSERT(isMonitored());
        return reinterpret_cast<const ICMonitoredStub *>(this);
    }

    inline ICMonitoredStub *toMonitoredStub() {
        JS_ASSERT(isMonitored());
        return reinterpret_cast<ICMonitoredStub *>(this);
    }

    inline const ICMonitoredFallbackStub *toMonitoredFallbackStub() const {
        JS_ASSERT(isMonitoredFallback());
        return reinterpret_cast<const ICMonitoredFallbackStub *>(this);
    }

    inline ICMonitoredFallbackStub *toMonitoredFallbackStub() {
        JS_ASSERT(isMonitoredFallback());
        return reinterpret_cast<ICMonitoredFallbackStub *>(this);
    }

#define KIND_METHODS(kindName)   \
    inline bool is##kindName() const { return kind() == kindName; } \
    inline const IC##kindName *to##kindName() const { \
        JS_ASSERT(is##kindName()); \
        return reinterpret_cast<const IC##kindName *>(this); \
    } \
    inline IC##kindName *to##kindName() { \
        JS_ASSERT(is##kindName()); \
        return reinterpret_cast<IC##kindName *>(this); \
    }
    IC_STUB_KIND_LIST(KIND_METHODS)
#undef KIND_METHODS

    inline ICStub *next() const {
        return next_;
    }

    inline bool hasNext() const {
        return next_ != NULL;
    }

    inline void setNext(ICStub *stub) {
        next_ = stub;
    }

    inline ICStub **addressOfNext() {
        return &next_;
    }

    inline IonCode *ionCode() {
        return IonCode::FromExecutable(stubCode_);
    }

    static inline size_t offsetOfNext() {
        return offsetof(ICStub, next_);
    }

    static inline size_t offsetOfStubCode() {
        return offsetof(ICStub, stubCode_);
    }
};

class ICFallbackStub : public ICStub
{
  protected:
    
    

    
    ICEntry *           icEntry_;

    
    uint32_t            numOptimizedStubs_;

    
    
    
    
    
    ICStub **           lastStubPtrAddr_;

    ICFallbackStub(Kind kind, IonCode *stubCode)
      : ICStub(kind, true, false, stubCode),
        icEntry_(NULL),
        numOptimizedStubs_(0),
        lastStubPtrAddr_(NULL) {}

    ICFallbackStub(Kind kind, bool isMonitored, IonCode *stubCode)
      : ICStub(kind, true, isMonitored, stubCode),
        icEntry_(NULL),
        numOptimizedStubs_(0),
        lastStubPtrAddr_(NULL) {}

  public:
    inline ICEntry *icEntry() const {
        return icEntry_;
    }

    inline size_t numOptimizedStubs() const {
        return (size_t) numOptimizedStubs_;
    }

    
    
    
    
    void fixupICEntry(ICEntry *icEntry) {
        JS_ASSERT(icEntry_ == NULL);
        JS_ASSERT(lastStubPtrAddr_ == NULL);
        icEntry_ = icEntry;
        lastStubPtrAddr_ = icEntry_->addressOfFirstStub();
    }

    
    void addNewStub(ICStub *stub) {
        JS_ASSERT(*lastStubPtrAddr_ == this);
        JS_ASSERT(stub->next() == NULL);
        stub->setNext(this);
        *lastStubPtrAddr_ = stub;
        lastStubPtrAddr_ = stub->addressOfNext();
        numOptimizedStubs_++;
    }
    bool hasStub(ICStub::Kind kind) {
        ICStub *stub = icEntry_->firstStub();
        do {
            if (stub->kind() == kind)
                return true;

            stub = stub->next();
        } while (stub);

        return false;
    }
};



class ICMonitoredStub : public ICStub
{
  protected:
    
    ICStub *            firstMonitorStub_;

    ICMonitoredStub(Kind kind, IonCode *stubCode, ICStub *firstMonitorStub);

  public:
    inline void updateFirstMonitorStub(ICStub *monitorStub) {
        
        
        JS_ASSERT(firstMonitorStub_ && firstMonitorStub_->isTypeMonitor_Fallback());
        firstMonitorStub_ = monitorStub;
    }

    inline ICStub *firstMonitorStub() const {
        return firstMonitorStub_;
    }

    static inline size_t offsetOfFirstMonitorStub() {
        return offsetof(ICMonitoredStub, firstMonitorStub_);
    }
};


class ICMonitoredFallbackStub : public ICFallbackStub
{
  protected:
    
    ICTypeMonitor_Fallback *    fallbackMonitorStub_;

    ICMonitoredFallbackStub(Kind kind, IonCode *stubCode)
      : ICFallbackStub(kind, true, stubCode),
        fallbackMonitorStub_(NULL) {}

  public:
    bool initMonitoringChain(JSContext *cx);

    inline ICTypeMonitor_Fallback *fallbackMonitorStub() const {
        return fallbackMonitorStub_;
    }
};


class ICStubCompiler
{
  protected:
    JSContext *     cx;
    ICStub::Kind    kind;

    
    virtual int32_t getKey() const {
        return static_cast<int32_t>(kind);
    }

    virtual bool generateStubCode(MacroAssembler &masm) = 0;
    IonCode *getStubCode();

    ICStubCompiler(JSContext *cx, ICStub::Kind kind)
      : cx(cx), kind(kind) {}

    
    bool callVM(const VMFunction &fun, MacroAssembler &masm);

    inline GeneralRegisterSet availableGeneralRegs(size_t numInputs) const {
        GeneralRegisterSet regs(GeneralRegisterSet::All());
        JS_ASSERT(!regs.has(BaselineStackReg));
#ifdef JS_CPU_ARM
        JS_ASSERT(!regs.has(BaselineTailCallReg));
#endif
        regs.take(BaselineFrameReg);
        regs.take(BaselineStubReg);
#ifdef JS_CPU_X64
        regs.take(ExtractTemp0);
        regs.take(ExtractTemp1);
#endif

        switch (numInputs) {
          case 0:
            break;
          case 1:
            regs.take(R0);
            break;
          case 2:
            regs.take(R0);
            regs.take(R1);
            break;
          default:
            JS_NOT_REACHED("Invalid numInputs");
        }

        return regs;
    }

  public:
    virtual ICStub *getStub() = 0;
};



class ICMultiStubCompiler : public ICStubCompiler
{
  protected:
    JSOp            op;

    
    
    virtual int32_t getKey() const {
        return static_cast<int32_t>(kind) | (static_cast<int32_t>(op) << 16);
    }

    ICMultiStubCompiler(JSContext *cx, ICStub::Kind kind, JSOp op)
      : ICStubCompiler(cx, kind), op(op) {}
};







class ICTypeMonitor_Fallback : public ICStub
{
    
    ICMonitoredFallbackStub *   mainFallbackStub_;

    
    ICStub *                    firstMonitorStub_;

    
    
    
    ICStub **                   lastMonitorStubPtrAddr_;

    
    uint32_t                    numOptimizedMonitorStubs_;

    ICTypeMonitor_Fallback(IonCode *stubCode, ICMonitoredFallbackStub *mainFallbackStub)
      : ICStub(ICStub::TypeMonitor_Fallback, stubCode),
        mainFallbackStub_(mainFallbackStub),
        firstMonitorStub_(this),
        lastMonitorStubPtrAddr_(NULL),
        numOptimizedMonitorStubs_(0)
    { }

    void addOptimizedMonitorStub(ICStub *stub) {
        stub->setNext(this);

        if (numOptimizedMonitorStubs_ == 0) {
            JS_ASSERT(lastMonitorStubPtrAddr_ == NULL);
            JS_ASSERT(firstMonitorStub_ == this);
            firstMonitorStub_ = stub;
        } else {
            JS_ASSERT(lastMonitorStubPtrAddr_ != NULL);
            JS_ASSERT(firstMonitorStub_ != NULL);
            *lastMonitorStubPtrAddr_ = stub;
        }

        lastMonitorStubPtrAddr_ = stub->addressOfNext();
        numOptimizedMonitorStubs_++;
    }

  public:
    static inline ICTypeMonitor_Fallback *New(IonCode *code, ICMonitoredFallbackStub *mainFbStub) {
        return new ICTypeMonitor_Fallback(code, mainFbStub);
    }

    inline ICFallbackStub *mainFallbackStub() const {
        return mainFallbackStub_;
    }

    inline ICStub *firstMonitorStub() const {
        return firstMonitorStub_;
    }

    inline uint32_t numOptimizedMonitorStubs() const {
        return numOptimizedMonitorStubs_;
    }

    
    
    bool addMonitorStubForValue(JSContext *cx, HandleValue val);

    
    class Compiler : public ICStubCompiler {
        ICMonitoredFallbackStub *mainFallbackStub_;

      protected:
        bool generateStubCode(MacroAssembler &masm);

      public:
        Compiler(JSContext *cx, ICMonitoredFallbackStub *mainFallbackStub)
          : ICStubCompiler(cx, ICStub::TypeMonitor_Fallback),
            mainFallbackStub_(mainFallbackStub)
        { }

        ICTypeMonitor_Fallback *getStub() {
            return ICTypeMonitor_Fallback::New(getStubCode(), mainFallbackStub_);
        }
    };
};





class ICTypeUpdate_Fallback : public ICStub
{
    uint8_t *entryPoint_;

    ICTypeUpdate_Fallback(IonCode *stubCode, uint8_t *entryPoint)
      : ICStub(ICStub::TypeUpdate_Fallback, stubCode),
        entryPoint_(entryPoint)
    { }

  public:
    static inline ICTypeUpdate_Fallback *New(IonCode *code, uint8_t *entryPoint) {
        return new ICTypeUpdate_Fallback(code, entryPoint);
    }

    static inline size_t offsetOfEntryPoint() {
        return offsetof(ICTypeUpdate_Fallback, entryPoint_);
    }

    
    class Compiler : public ICStubCompiler {
      CodeLocationLabel fallbackEntryPoint_;

      protected:
        bool generateStubCode(MacroAssembler &masm);

      public:
        Compiler(JSContext *cx, CodeLocationLabel fallbackEntryPoint)
          : ICStubCompiler(cx, ICStub::TypeUpdate_Fallback),
            fallbackEntryPoint_(fallbackEntryPoint)
        { }

        ICStub *getStub() {
            
            return ICTypeUpdate_Fallback::New(getStubCode(), fallbackEntryPoint_.raw());
        }
    };
};





class ICCompare_Fallback : public ICFallbackStub
{
    ICCompare_Fallback(IonCode *stubCode)
      : ICFallbackStub(ICStub::Compare_Fallback, stubCode) {}

  public:
    static const uint32_t MAX_OPTIMIZED_STUBS = 8;

    static inline ICCompare_Fallback *New(IonCode *code) {
        return new ICCompare_Fallback(code);
    }

    
    class Compiler : public ICStubCompiler {
      protected:
        bool generateStubCode(MacroAssembler &masm);

      public:
        Compiler(JSContext *cx)
          : ICStubCompiler(cx, ICStub::Compare_Fallback) {}

        ICStub *getStub() {
            return ICCompare_Fallback::New(getStubCode());
        }
    };
};

class ICCompare_Int32 : public ICFallbackStub
{
    ICCompare_Int32(IonCode *stubCode)
      : ICFallbackStub(ICStub::Compare_Int32, stubCode) {}

  public:
    static inline ICCompare_Int32 *New(IonCode *code) {
        return new ICCompare_Int32(code);
    }

    
    class Compiler : public ICMultiStubCompiler {
      protected:
        bool generateStubCode(MacroAssembler &masm);

      public:
        Compiler(JSContext *cx, JSOp op)
          : ICMultiStubCompiler(cx, ICStub::Compare_Int32, op) {}

        ICStub *getStub() {
            return ICCompare_Int32::New(getStubCode());
        }
    };
};




class ICToBool_Fallback : public ICFallbackStub
{
    ICToBool_Fallback(IonCode *stubCode)
      : ICFallbackStub(ICStub::ToBool_Fallback, stubCode) {}

  public:
    static const uint32_t MAX_OPTIMIZED_STUBS = 8;

    static inline ICToBool_Fallback *New(IonCode *code) {
        return new ICToBool_Fallback(code);
    }

    
    class Compiler : public ICStubCompiler {
      protected:
        bool generateStubCode(MacroAssembler &masm);

      public:
        Compiler(JSContext *cx)
          : ICStubCompiler(cx, ICStub::ToBool_Fallback) {}

        ICStub *getStub() {
            return ICToBool_Fallback::New(getStubCode());
        }
    };
};

class ICToBool_Bool : public ICStub
{
    ICToBool_Bool(IonCode *stubCode)
      : ICStub(ICStub::ToBool_Bool, stubCode) {}

  public:
    static inline ICToBool_Bool *New(IonCode *code) {
        return new ICToBool_Bool(code);
    }

    
    class Compiler : public ICStubCompiler {
      protected:
        bool generateStubCode(MacroAssembler &masm);

      public:
        Compiler(JSContext *cx)
          : ICStubCompiler(cx, ICStub::ToBool_Bool) {}

        ICStub *getStub() {
            return ICToBool_Bool::New(getStubCode());
        }
    };
};

class ICToBool_Int32 : public ICStub
{
    ICToBool_Int32(IonCode *stubCode)
      : ICStub(ICStub::ToBool_Int32, stubCode) {}

  public:
    static inline ICToBool_Int32 *New(IonCode *code) {
        return new ICToBool_Int32(code);
    }

    
    class Compiler : public ICStubCompiler {
      protected:
        bool generateStubCode(MacroAssembler &masm);

      public:
        Compiler(JSContext *cx)
          : ICStubCompiler(cx, ICStub::ToBool_Int32) {}

        ICStub *getStub() {
            return ICToBool_Int32::New(getStubCode());
        }
    };
};




class ICToNumber_Fallback : public ICFallbackStub
{
    ICToNumber_Fallback(IonCode *stubCode)
      : ICFallbackStub(ICStub::ToNumber_Fallback, stubCode) {}

  public:
    static inline ICToNumber_Fallback *New(IonCode *code) {
        return new ICToNumber_Fallback(code);
    }

    
    class Compiler : public ICStubCompiler {
      protected:
        bool generateStubCode(MacroAssembler &masm);

      public:
        Compiler(JSContext *cx)
          : ICStubCompiler(cx, ICStub::ToNumber_Fallback) {}

        ICStub *getStub() {
            return ICToNumber_Fallback::New(getStubCode());
        }
    };
};






class ICBinaryArith_Fallback : public ICFallbackStub
{
    ICBinaryArith_Fallback(IonCode *stubCode)
      : ICFallbackStub(BinaryArith_Fallback, stubCode) {}

  public:
    static const uint32_t MAX_OPTIMIZED_STUBS = 8;

    static inline ICBinaryArith_Fallback *New(IonCode *code) {
        return new ICBinaryArith_Fallback(code);
    }

    
    class Compiler : public ICStubCompiler {
      protected:
        bool generateStubCode(MacroAssembler &masm);

      public:
        Compiler(JSContext *cx)
          : ICStubCompiler(cx, ICStub::BinaryArith_Fallback) {}

        ICStub *getStub() {
            return ICBinaryArith_Fallback::New(getStubCode());
        }
    };
};

class ICBinaryArith_Int32 : public ICStub
{
    ICBinaryArith_Int32(IonCode *stubCode)
      : ICStub(BinaryArith_Int32, stubCode) {}

  public:
    static inline ICBinaryArith_Int32 *New(IonCode *code) {
        return new ICBinaryArith_Int32(code);
    }

    
    class Compiler : public ICStubCompiler {
      protected:
        JSOp op_;
        bool allowDouble_;

        bool generateStubCode(MacroAssembler &masm);

        
        virtual int32_t getKey() const {
            return (static_cast<int32_t>(kind) | (static_cast<int32_t>(op_) << 16) |
                    (static_cast<int32_t>(allowDouble_) << 24));
        }

      public:
        Compiler(JSContext *cx, JSOp op, bool allowDouble)
          : ICStubCompiler(cx, ICStub::BinaryArith_Int32),
            op_(op), allowDouble_(allowDouble) {}

        ICStub *getStub() {
            return ICBinaryArith_Int32::New(getStubCode());
        }
    };
};




class ICGetElem_Fallback : public ICMonitoredFallbackStub
{
    ICGetElem_Fallback(IonCode *stubCode)
      : ICMonitoredFallbackStub(ICStub::GetElem_Fallback, stubCode)
    { }

  public:
    static const uint32_t MAX_OPTIMIZED_STUBS = 8;

    static inline ICGetElem_Fallback *New(IonCode *code) {
        return new ICGetElem_Fallback(code);
    }

    
    class Compiler : public ICStubCompiler {
      protected:
        bool generateStubCode(MacroAssembler &masm);

      public:
        Compiler(JSContext *cx)
          : ICStubCompiler(cx, ICStub::GetElem_Fallback)
        { }

        ICStub *getStub() {
            ICGetElem_Fallback *stub = ICGetElem_Fallback::New(getStubCode());
            if (!stub)
                return NULL;
            if (!stub->initMonitoringChain(cx)) {
                delete stub;
                return NULL;
            }
            return stub;
        }
    };
};

class ICGetElem_Dense : public ICMonitoredStub
{
    ICGetElem_Dense(IonCode *stubCode, ICStub *firstMonitorStub)
      : ICMonitoredStub(GetElem_Dense, stubCode, firstMonitorStub) {}

  public:
    static inline ICGetElem_Dense *New(IonCode *code, ICStub *firstMonitorStub) {
        return new ICGetElem_Dense(code, firstMonitorStub);
    }

    class Compiler : public ICStubCompiler {
      ICStub *firstMonitorStub_;

      protected:
        bool generateStubCode(MacroAssembler &masm);

      public:
        Compiler(JSContext *cx, ICStub *firstMonitorStub)
          : ICStubCompiler(cx, ICStub::GetElem_Dense),
            firstMonitorStub_(firstMonitorStub) {}

        ICStub *getStub() {
            return ICGetElem_Dense::New(getStubCode(), firstMonitorStub_);
        }
    };
};




class ICSetElem_Fallback : public ICFallbackStub
{
    ICSetElem_Fallback(IonCode *stubCode)
      : ICFallbackStub(ICStub::SetElem_Fallback, stubCode)
    { }

  public:
    static const uint32_t MAX_OPTIMIZED_STUBS = 8;

    static inline ICSetElem_Fallback *New(IonCode *code) {
        return new ICSetElem_Fallback(code);
    }

    
    class Compiler : public ICStubCompiler {
      protected:
        bool generateStubCode(MacroAssembler &masm);

      public:
        Compiler(JSContext *cx)
          : ICStubCompiler(cx, ICStub::SetElem_Fallback)
        { }

        ICStub *getStub() {
            return ICSetElem_Fallback::New(getStubCode());
        }
    };
};

class ICSetElem_Dense : public ICStub
{
    ICSetElem_Dense(IonCode *stubCode)
      : ICStub(SetElem_Dense, stubCode) {}

  public:
    static inline ICSetElem_Dense *New(IonCode *code) {
        return new ICSetElem_Dense(code);
    }

    class Compiler : public ICStubCompiler {
      protected:
        bool generateStubCode(MacroAssembler &masm);

      public:
        Compiler(JSContext *cx)
          : ICStubCompiler(cx, ICStub::SetElem_Dense) {}

        ICStub *getStub() {
            return ICSetElem_Dense::New(getStubCode());
        }
    };
};







class ICCallStubCompiler : public ICStubCompiler
{
  protected:
    ICCallStubCompiler(JSContext *cx, ICStub::Kind kind)
      : ICStubCompiler(cx, kind)
    { }

    void pushCallArguments(MacroAssembler &masm, Register argcReg);
};

class ICCall_Fallback : public ICFallbackStub
{
    ICCall_Fallback(IonCode *stubCode)
      : ICFallbackStub(ICStub::Call_Fallback, stubCode)
    { }

  public:
    static inline ICCall_Fallback *New(IonCode *code) {
        return new ICCall_Fallback(code);
    }

    
    class Compiler : public ICCallStubCompiler {
      protected:
        bool generateStubCode(MacroAssembler &masm);

      public:
        Compiler(JSContext *cx)
          : ICCallStubCompiler(cx, ICStub::Call_Fallback)
        { }

        ICStub *getStub() {
            return ICCall_Fallback::New(getStubCode());
        }
    };
};


} 
} 

#endif

