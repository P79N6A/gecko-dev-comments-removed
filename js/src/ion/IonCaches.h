






#ifndef jsion_caches_h__
#define jsion_caches_h__

#include "IonCode.h"
#include "TypeOracle.h"
#include "Registers.h"

class JSFunction;
class JSScript;

namespace js {
namespace ion {

#define IONCACHE_KIND_LIST(_)                                   \
    _(GetProperty)                                              \
    _(SetProperty)                                              \
    _(GetElement)                                               \
    _(BindName)                                                 \
    _(Name)                                                     \
    _(CallsiteClone)


#define FORWARD_DECLARE(kind) class kind##IC;
IONCACHE_KIND_LIST(FORWARD_DECLARE)
#undef FORWARD_DECLARE

class IonCacheVisitor
{
  public:
#define VISIT_INS(op)                                               \
    virtual bool visit##op##IC(CodeGenerator *codegen, op##IC *) {  \
        JS_NOT_REACHED("NYI: " #op "IC");                           \
        return false;                                               \
    }

    IONCACHE_KIND_LIST(VISIT_INS)
#undef VISIT_INS
};


































































class IonCache
{
  public:
    enum Kind {
#   define DEFINE_CACHEKINDS(ickind) Cache_##ickind,
        IONCACHE_KIND_LIST(DEFINE_CACHEKINDS)
#   undef DEFINE_CACHEKINDS
        Cache_Invalid
    };

    
#   define CACHEKIND_CASTS(ickind)                                      \
    bool is##ickind() const {                                           \
        return kind() == Cache_##ickind;                                \
    }                                                                   \
    inline ickind##IC &to##ickind();

    IONCACHE_KIND_LIST(CACHEKIND_CASTS)
#   undef CACHEKIND_CASTS

    virtual Kind kind() const = 0;

    virtual bool accept(CodeGenerator *codegen, IonCacheVisitor *visitor) = 0;

  public:

    static const char *CacheName(Kind kind);

  protected:
    bool pure_ : 1;
    bool idempotent_ : 1;
    bool disabled_ : 1;
    size_t stubCount_ : 5;

    CodeLocationJump initialJump_;
    CodeLocationJump lastJump_;
    CodeLocationLabel fallbackLabel_;

    
#ifdef JS_CPU_ARM
    static const size_t REJOIN_LABEL_OFFSET = 4;
#else
    static const size_t REJOIN_LABEL_OFFSET = 0;
#endif

    
    JSScript *script;
    jsbytecode *pc;

  private:
    static const size_t MAX_STUBS;
    void incrementStubCount() {
        
        stubCount_++;
        JS_ASSERT(stubCount_);
    }

    CodeLocationLabel fallbackLabel() const {
        return fallbackLabel_;
    }
    CodeLocationLabel rejoinLabel() const {
        uint8_t *ptr = initialJump_.raw();
#ifdef JS_CPU_ARM
        uint32_t i = 0;
        while (i < REJOIN_LABEL_OFFSET)
            ptr = Assembler::nextInstruction(ptr, &i);
#endif
        return CodeLocationLabel(ptr);
    }

  public:

    IonCache()
      : pure_(false),
        idempotent_(false),
        disabled_(false),
        stubCount_(0),
        initialJump_(),
        lastJump_(),
        fallbackLabel_(),
        script(NULL),
        pc(NULL)
    {
    }

    void disable();
    inline bool isDisabled() const {
        return disabled_;
    }

    
    
    
    
    void setInlineJump(CodeOffsetJump initialJump, CodeOffsetLabel rejoinLabel) {
        initialJump_ = initialJump;
        lastJump_ = initialJump;

        JS_ASSERT(rejoinLabel.offset() == initialJump.offset() + REJOIN_LABEL_OFFSET);
    }

    
    
    
    void setFallbackLabel(CodeOffsetLabel fallbackLabel) {
        fallbackLabel_ = fallbackLabel;
    }

    
    void updateBaseAddress(IonCode *code, MacroAssembler &masm);

    
    void reset();

    bool canAttachStub() const {
        return stubCount_ < MAX_STUBS;
    }

    enum LinkStatus {
        LINK_ERROR,
        CACHE_FLUSHED,
        LINK_GOOD
    };

    
    
    
    
    LinkStatus linkCode(JSContext *cx, MacroAssembler &masm, IonScript *ion, IonCode **code);

    
    
    void attachStub(MacroAssembler &masm, IonCode *code, CodeOffsetJump &rejoinOffset,
                    CodeOffsetJump *exitOffset, CodeOffsetLabel *stubOffset = NULL);

    
    
    bool linkAndAttachStub(JSContext *cx, MacroAssembler &masm, IonScript *ion,
                           const char *attachKind, CodeOffsetJump &rejoinOffset,
                           CodeOffsetJump *exitOffset, CodeOffsetLabel *stubOffset = NULL);

    bool pure() {
        return pure_;
    }
    bool idempotent() {
        return idempotent_;
    }
    void setIdempotent() {
        JS_ASSERT(!idempotent_);
        JS_ASSERT(!script);
        JS_ASSERT(!pc);
        idempotent_ = true;
    }

    void setScriptedLocation(UnrootedScript script, jsbytecode *pc) {
        JS_ASSERT(!idempotent_);
        this->script = script;
        this->pc = pc;
    }

    void getScriptedLocation(MutableHandleScript pscript, jsbytecode **ppc) {
        pscript.set(script);
        *ppc = pc;
    }
};



#define CACHE_HEADER(ickind)                                        \
    Kind kind() const {                                             \
        return IonCache::Cache_##ickind;                            \
    }                                                               \
                                                                    \
    bool accept(CodeGenerator *codegen, IonCacheVisitor *visitor) { \
        return visitor->visit##ickind##IC(codegen, this);           \
    }                                                               \
                                                                    \
    static const VMFunction UpdateInfo;




class GetPropertyIC : public IonCache
{
  protected:
    
    
    RegisterSet liveRegs_;

    Register object_;
    PropertyName *name_;
    TypedOrValueRegister output_;
    bool allowGetters_ : 1;
    bool hasArrayLengthStub_ : 1;
    bool hasTypedArrayLengthStub_ : 1;

  public:
    GetPropertyIC(RegisterSet liveRegs,
                  Register object, PropertyName *name,
                  TypedOrValueRegister output,
                  bool allowGetters)
      : liveRegs_(liveRegs),
        object_(object),
        name_(name),
        output_(output),
        allowGetters_(allowGetters),
        hasArrayLengthStub_(false),
        hasTypedArrayLengthStub_(false)
    {
    }

    CACHE_HEADER(GetProperty)

    Register object() const {
        return object_;
    }
    PropertyName *name() const {
        return name_;
    }
    TypedOrValueRegister output() const {
        return output_;
    }
    bool allowGetters() const {
        return allowGetters_;
    }
    bool hasArrayLengthStub() const {
        return hasArrayLengthStub_;
    }
    bool hasTypedArrayLengthStub() const {
        return hasTypedArrayLengthStub_;
    }

    bool attachReadSlot(JSContext *cx, IonScript *ion, JSObject *obj, JSObject *holder,
                        HandleShape shape);
    bool attachCallGetter(JSContext *cx, IonScript *ion, JSObject *obj, JSObject *holder,
                          HandleShape shape,
                          const SafepointIndex *safepointIndex, void *returnAddr);
    bool attachArrayLength(JSContext *cx, IonScript *ion, JSObject *obj);
    bool attachTypedArrayLength(JSContext *cx, IonScript *ion, JSObject *obj);

    static bool update(JSContext *cx, size_t cacheIndex, HandleObject obj, MutableHandleValue vp);
};

class SetPropertyIC : public IonCache
{
  protected:
    
    
    RegisterSet liveRegs_;

    Register object_;
    PropertyName *name_;
    ConstantOrRegister value_;
    bool isSetName_;
    bool strict_;

  public:
    SetPropertyIC(RegisterSet liveRegs, Register object, PropertyName *name,
                  ConstantOrRegister value, bool isSetName, bool strict)
      : liveRegs_(liveRegs),
        object_(object),
        name_(name),
        value_(value),
        isSetName_(isSetName),
        strict_(strict)
    {
    }

    CACHE_HEADER(SetProperty)

    Register object() const {
        return object_;
    }
    PropertyName *name() const {
        return name_;
    }
    ConstantOrRegister value() const {
        return value_;
    }
    bool isSetName() const {
        return isSetName_;
    }
    bool strict() const {
        return strict_;
    }

    bool attachNativeExisting(JSContext *cx, IonScript *ion, HandleObject obj, HandleShape shape);
    bool attachSetterCall(JSContext *cx, IonScript *ion, HandleObject obj,
                          HandleObject holder, HandleShape shape, void *returnAddr);
    bool attachNativeAdding(JSContext *cx, IonScript *ion, JSObject *obj, HandleShape oldshape,
                            HandleShape newshape, HandleShape propshape);

    static bool
    update(JSContext *cx, size_t cacheIndex, HandleObject obj, HandleValue value);
};

class GetElementIC : public IonCache
{
  protected:
    Register object_;
    ConstantOrRegister index_;
    TypedOrValueRegister output_;
    bool monitoredResult_ : 1;
    bool hasDenseStub_ : 1;

  public:
    GetElementIC(Register object, ConstantOrRegister index,
                 TypedOrValueRegister output, bool monitoredResult)
      : object_(object),
        index_(index),
        output_(output),
        monitoredResult_(monitoredResult),
        hasDenseStub_(false)
    {
    }

    CACHE_HEADER(GetElement)

    Register object() const {
        return object_;
    }
    ConstantOrRegister index() const {
        return index_;
    }
    TypedOrValueRegister output() const {
        return output_;
    }
    bool monitoredResult() const {
        return monitoredResult_;
    }
    bool hasDenseStub() const {
        return hasDenseStub_;
    }
    void setHasDenseStub() {
        JS_ASSERT(!hasDenseStub());
        hasDenseStub_ = true;
    }

    bool attachGetProp(JSContext *cx, IonScript *ion, HandleObject obj, const Value &idval, HandlePropertyName name);
    bool attachDenseElement(JSContext *cx, IonScript *ion, JSObject *obj, const Value &idval);
    bool attachTypedArrayElement(JSContext *cx, IonScript *ion, JSObject *obj, const Value &idval);

    static bool
    update(JSContext *cx, size_t cacheIndex, HandleObject obj, HandleValue idval,
                MutableHandleValue vp);
};

class BindNameIC : public IonCache
{
  protected:
    Register scopeChain_;
    PropertyName *name_;
    Register output_;

  public:
    BindNameIC(Register scopeChain, PropertyName *name, Register output)
      : scopeChain_(scopeChain),
        name_(name),
        output_(output)
    {
    }

    CACHE_HEADER(BindName)

    Register scopeChainReg() const {
        return scopeChain_;
    }
    HandlePropertyName name() const {
        return HandlePropertyName::fromMarkedLocation(&name_);
    }
    Register outputReg() const {
        return output_;
    }

    bool attachGlobal(JSContext *cx, IonScript *ion, JSObject *scopeChain);
    bool attachNonGlobal(JSContext *cx, IonScript *ion, JSObject *scopeChain, JSObject *holder);

    static JSObject *
    update(JSContext *cx, size_t cacheIndex, HandleObject scopeChain);
};

class NameIC : public IonCache
{
  protected:
    bool typeOf_;
    Register scopeChain_;
    PropertyName *name_;
    TypedOrValueRegister output_;

  public:
    NameIC(bool typeOf,
           Register scopeChain, PropertyName *name,
           TypedOrValueRegister output)
      : typeOf_(typeOf),
        scopeChain_(scopeChain),
        name_(name),
        output_(output)
    {
    }

    CACHE_HEADER(Name)

    Register scopeChainReg() const {
        return scopeChain_;
    }
    HandlePropertyName name() const {
        return HandlePropertyName::fromMarkedLocation(&name_);
    }
    TypedOrValueRegister outputReg() const {
        return output_;
    }
    bool isTypeOf() const {
        return typeOf_;
    }

    bool attach(JSContext *cx, IonScript *ion, HandleObject scopeChain, HandleObject obj,
                HandleShape shape);

    static bool
    update(JSContext *cx, size_t cacheIndex, HandleObject scopeChain, MutableHandleValue vp);
};

class CallsiteCloneIC : public IonCache
{
  protected:
    Register callee_;
    Register output_;
    JSScript *callScript_;
    jsbytecode *callPc_;

  public:
    CallsiteCloneIC(Register callee, JSScript *callScript, jsbytecode *callPc, Register output)
      : callee_(callee),
        output_(output),
        callScript_(callScript),
        callPc_(callPc)
    {
    }

    CACHE_HEADER(CallsiteClone)

    Register calleeReg() const {
        return callee_;
    }
    HandleScript callScript() const {
        return HandleScript::fromMarkedLocation(&callScript_);
    }
    jsbytecode *callPc() const {
        return callPc_;
    }
    Register outputReg() const {
        return output_;
    }

    bool attach(JSContext *cx, IonScript *ion, HandleFunction original, HandleFunction clone);

    static JSObject *update(JSContext *cx, size_t cacheIndex, HandleObject callee);
};

#undef CACHE_HEADER


#define CACHE_CASTS(ickind)                                             \
    ickind##IC &IonCache::to##ickind()                                  \
    {                                                                   \
        JS_ASSERT(is##ickind());                                        \
        return *static_cast<ickind##IC *>(this);                        \
    }
IONCACHE_KIND_LIST(CACHE_CASTS)
#undef OPCODE_CASTS

} 
} 

#endif
