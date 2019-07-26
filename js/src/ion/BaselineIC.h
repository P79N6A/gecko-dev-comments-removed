






#if !defined(jsion_baseline_ic_h__) && defined(JS_ION)
#define jsion_baseline_ic_h__

#include "jscntxt.h"
#include "jscompartment.h"
#include "jsopcode.h"
#include "gc/Heap.h"
#include "BaselineJIT.h"

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
    _(Compare_Fallback)         \
    _(Compare_Int32)            \
                                \
    _(ToBool_Fallback)          \
    _(ToBool_Bool)              \
                                \
    _(ToNumber_Fallback)        \
                                \
    _(BinaryArith_Fallback)     \
    _(BinaryArith_Int32)

#define FORWARD_DECLARE_STUBS(kindName) class IC##kindName;
    IC_STUB_KIND_LIST(FORWARD_DECLARE_STUBS)
#undef FORWARD_DECLARE_STUBS

class ICFallbackStub;




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
    
    uint16_t kind_;

    
    void *stubCode_;

    
    
    ICStub *next_;

    inline static uint16_t packKindAndFallbackFlag(Kind kind, bool isFallback) {
        uint16_t val = static_cast<uint16_t>(kind);
        JS_ASSERT(!(val & 0x8000U));
        if (isFallback)
            val |= 0x8000U;
        return val;
    }

    inline ICStub(Kind kind, IonCode *stubCode)
      : kind_(packKindAndFallbackFlag(kind, false)),
        stubCode_(stubCode->raw()),
        next_(NULL)
    {
        JS_ASSERT(stubCode != NULL);
    }

    inline ICStub(Kind kind, bool isFallback, IonCode *stubCode)
      : kind_(packKindAndFallbackFlag(kind, isFallback)),
        stubCode_(stubCode->raw()),
        next_(NULL)
    {
        JS_ASSERT(stubCode != NULL);
    }

  public:

    inline Kind kind() const {
        return static_cast<Kind>(kind_ & 0x7fffU);
    }

    inline bool isFallback() const {
        return !!(kind_ & 0x8000U);
    }

    inline const ICFallbackStub *toFallbackStub() const {
        return reinterpret_cast<const ICFallbackStub *>(this);
    }

    inline ICFallbackStub *toFallbackStub()  {
        return reinterpret_cast<ICFallbackStub *>(this);
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
      : ICStub(kind, true, stubCode),
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
};


class ICStubCompiler
{
  protected:
    JSContext *     cx;
    ICStub::Kind    kind;

    
    virtual int32_t getKey() const {
        return static_cast<int32_t>(kind);
    }

    virtual IonCode *generateStubCode() = 0;
    IonCode *getStubCode() {
        IonCompartment *ion = cx->compartment->ionCompartment();
        uint32_t stubKey = getKey();
        IonCode *stubCode = ion->getStubCode(stubKey);
        if (stubCode)
            return stubCode;

        Rooted<IonCode *> newStubCode(cx, generateStubCode());
        if (!newStubCode)
            return NULL;

        if (!ion->putStubCode(stubKey, newStubCode))
            return NULL;

        return newStubCode;
    }

    ICStubCompiler(JSContext *cx, ICStub::Kind kind)
      : cx(cx), kind(kind) {}

    
    bool callVM(const VMFunction &fun, MacroAssembler &masm);

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
        IonCode *generateStubCode();

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
        IonCode *generateStubCode();

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
        IonCode *generateStubCode();

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
        IonCode *generateStubCode();

      public:
        Compiler(JSContext *cx)
          : ICStubCompiler(cx, ICStub::ToBool_Bool) {}

        ICStub *getStub() {
            return ICToBool_Bool::New(getStubCode());
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
        IonCode *generateStubCode();

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
        IonCode *generateStubCode();

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

    
    class Compiler : public ICMultiStubCompiler {
      protected:
        IonCode *generateStubCode();

      public:
        Compiler(JSContext *cx, JSOp op)
          : ICMultiStubCompiler(cx, ICStub::BinaryArith_Int32, op) {}

        ICStub *getStub() {
            return ICBinaryArith_Int32::New(getStubCode());
        }
    };
};


} 
} 

#endif

