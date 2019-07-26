






#ifndef jsion_caches_h__
#define jsion_caches_h__

#include "IonCode.h"
#include "TypeOracle.h"
#include "Registers.h"

class JSFunction;
class JSScript;

namespace js {
namespace ion {

class IonCacheGetProperty;
class IonCacheSetProperty;
class IonCacheGetElement;
class IonCacheBindName;
class IonCacheName;
class IonCacheCallsiteClone;
































struct TypedOrValueRegisterSpace
{
    mozilla::AlignedStorage2<TypedOrValueRegister> data_;
    TypedOrValueRegister &data() {
        return *data_.addr();
    }
    const TypedOrValueRegister &data() const {
        return *data_.addr();
    }
};

struct ConstantOrRegisterSpace
{
    mozilla::AlignedStorage2<ConstantOrRegister> data_;
    ConstantOrRegister &data() {
        return *data_.addr();
    }
    const ConstantOrRegister &data() const {
        return *data_.addr();
    }
};

class IonCache
{
  public:
    enum Kind {
        Invalid = 0,
        GetProperty,
        SetProperty,
        GetElement,
        BindName,
        Name,
        NameTypeOf,
        CallsiteClone
    };

  protected:
    Kind kind_ : 8;
    bool pure_ : 1;
    bool idempotent_ : 1;
    bool disabled_ : 1;
    size_t stubCount_ : 5;

    CodeLocationJump initialJump_;
    CodeLocationJump lastJump_;
    CodeLocationLabel cacheLabel_;

    
#ifdef JS_CPU_ARM
    static const size_t REJOIN_LABEL_OFFSET = 4;
#else
    static const size_t REJOIN_LABEL_OFFSET = 0;
#endif
    union {
        struct {
            Register object;
            PropertyName *name;
            TypedOrValueRegisterSpace output;
            bool allowGetters : 1;
            bool hasArrayLengthStub : 1;
            bool hasTypedArrayLengthStub : 1;
        } getprop;
        struct {
            Register object;
            PropertyName *name;
            ConstantOrRegisterSpace value;
            bool strict;
        } setprop;
        struct {
            Register object;
            ConstantOrRegisterSpace index;
            TypedOrValueRegisterSpace output;
            bool monitoredResult : 1;
            bool hasDenseStub : 1;
        } getelem;
        struct {
            Register scopeChain;
            PropertyName *name;
            Register output;
        } bindname;
        struct {
            Register scopeChain;
            PropertyName *name;
            TypedOrValueRegisterSpace output;
        } name;
        struct {
            Register callee;
            Register output;
            JSScript *callScript;
            jsbytecode *callPc;
        } callsiteclone;
    } u;

    
    
    RegisterSet liveRegs;

    
    JSScript *script;
    jsbytecode *pc;

    void init(Kind kind, RegisterSet liveRegs,
              CodeOffsetJump initialJump,
              CodeOffsetLabel rejoinLabel,
              CodeOffsetLabel cacheLabel) {
        this->kind_ = kind;
        this->liveRegs = liveRegs;
        this->initialJump_ = initialJump;
        this->lastJump_ = initialJump;
        this->cacheLabel_ = cacheLabel;

        JS_ASSERT(rejoinLabel.offset() == initialJump.offset() + REJOIN_LABEL_OFFSET);
    }

  public:

    IonCache() { PodZero(this); }

    void updateBaseAddress(IonCode *code, MacroAssembler &masm);

    
    void disable();
    inline bool isDisabled() const {
        return disabled_;
    }
    
    
    void reset();

    CodeLocationJump lastJump() const { return lastJump_; }
    CodeLocationLabel cacheLabel() const { return cacheLabel_; }

    CodeLocationLabel rejoinLabel() const {
        uint8_t *ptr = initialJump_.raw();
#ifdef JS_CPU_ARM
        uint32_t i = 0;
        while (i < REJOIN_LABEL_OFFSET)
            ptr = Assembler::nextInstruction(ptr, &i);
#endif
        return CodeLocationLabel(ptr);
    }

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

    void updateLastJump(CodeLocationJump jump) {
        lastJump_ = jump;
    }

    size_t stubCount() const {
        return stubCount_;
    }
    void incrementStubCount() {
        
        stubCount_++;
        JS_ASSERT(stubCount_);
    }

    IonCacheGetProperty &toGetProperty() {
        JS_ASSERT(kind_ == GetProperty);
        return *(IonCacheGetProperty *)this;
    }
    IonCacheSetProperty &toSetProperty() {
        JS_ASSERT(kind_ == SetProperty);
        return *(IonCacheSetProperty *)this;
    }
    IonCacheGetElement &toGetElement() {
        JS_ASSERT(kind_ == GetElement);
        return *(IonCacheGetElement *)this;
    }
    IonCacheBindName &toBindName() {
        JS_ASSERT(kind_ == BindName);
        return *(IonCacheBindName *)this;
    }
    IonCacheName &toName() {
        JS_ASSERT(kind_ == Name || kind_ == NameTypeOf);
        return *(IonCacheName *)this;
    }
    IonCacheCallsiteClone &toCallsiteClone() {
        JS_ASSERT(kind_ == CallsiteClone);
        return *(IonCacheCallsiteClone *)this;
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

inline IonCache &
IonScript::getCache(size_t index) {
    JS_ASSERT(index < numCaches());
    return cacheList()[index];
}




class IonCacheGetProperty : public IonCache
{
  public:
    IonCacheGetProperty(CodeOffsetJump initialJump,
                        CodeOffsetLabel rejoinLabel,
                        CodeOffsetLabel cacheLabel,
                        RegisterSet liveRegs,
                        Register object, PropertyName *name,
                        TypedOrValueRegister output,
                        bool allowGetters)
    {
        init(GetProperty, liveRegs, initialJump, rejoinLabel, cacheLabel);
        u.getprop.object = object;
        u.getprop.name = name;
        u.getprop.output.data() = output;
        u.getprop.allowGetters = allowGetters;
        u.getprop.hasArrayLengthStub = false;
        u.getprop.hasTypedArrayLengthStub = false;
    }

    Register object() const { return u.getprop.object; }
    PropertyName *name() const { return u.getprop.name; }
    TypedOrValueRegister output() const { return u.getprop.output.data(); }
    bool allowGetters() const { return u.getprop.allowGetters; }
    bool hasArrayLengthStub() const { return u.getprop.hasArrayLengthStub; }
    bool hasTypedArrayLengthStub() const { return u.getprop.hasTypedArrayLengthStub; }

    bool attachReadSlot(JSContext *cx, IonScript *ion, JSObject *obj, JSObject *holder,
                        HandleShape shape);
    bool attachCallGetter(JSContext *cx, IonScript *ion, JSObject *obj, JSObject *holder,
                          HandleShape shape,
                          const SafepointIndex *safepointIndex, void *returnAddr);
    bool attachArrayLength(JSContext *cx, IonScript *ion, JSObject *obj);
    bool attachTypedArrayLength(JSContext *cx, IonScript *ion, JSObject *obj);
};

class IonCacheSetProperty : public IonCache
{
  public:
    IonCacheSetProperty(CodeOffsetJump initialJump,
                        CodeOffsetLabel rejoinLabel,
                        CodeOffsetLabel cacheLabel,
                        RegisterSet liveRegs,
                        Register object, PropertyName *name,
                        ConstantOrRegister value,
                        bool strict)
    {
        init(SetProperty, liveRegs, initialJump, rejoinLabel, cacheLabel);
        u.setprop.object = object;
        u.setprop.name = name;
        u.setprop.value.data() = value;
        u.setprop.strict = strict;
    }

    Register object() const { return u.setprop.object; }
    PropertyName *name() const { return u.setprop.name; }
    ConstantOrRegister value() const { return u.setprop.value.data(); }
    bool strict() const { return u.setprop.strict; }

    bool attachNativeExisting(JSContext *cx, IonScript *ion, HandleObject obj, HandleShape shape);
    bool attachSetterCall(JSContext *cx, IonScript *ion, HandleObject obj,
                          HandleObject holder, HandleShape shape, void *returnAddr);
    bool attachNativeAdding(JSContext *cx, IonScript *ion, JSObject *obj, HandleShape oldshape,
                            HandleShape newshape, HandleShape propshape);
};

class IonCacheGetElement : public IonCache
{
  public:
    IonCacheGetElement(CodeOffsetJump initialJump,
                       CodeOffsetLabel rejoinLabel,
                       CodeOffsetLabel cacheLabel,
                       RegisterSet liveRegs,
                       Register object, ConstantOrRegister index,
                       TypedOrValueRegister output, bool monitoredResult)
    {
        init(GetElement, liveRegs, initialJump, rejoinLabel, cacheLabel);
        u.getelem.object = object;
        u.getelem.index.data() = index;
        u.getelem.output.data() = output;
        u.getelem.monitoredResult = monitoredResult;
        u.getelem.hasDenseStub = false;
    }

    Register object() const {
        return u.getelem.object;
    }
    ConstantOrRegister index() const {
        return u.getelem.index.data();
    }
    TypedOrValueRegister output() const {
        return u.getelem.output.data();
    }
    bool monitoredResult() const {
        return u.getelem.monitoredResult;
    }
    bool hasDenseStub() const {
        return u.getelem.hasDenseStub;
    }
    void setHasDenseStub() {
        JS_ASSERT(!hasDenseStub());
        u.getelem.hasDenseStub = true;
    }

    bool attachGetProp(JSContext *cx, IonScript *ion, HandleObject obj, const Value &idval, HandlePropertyName name);
    bool attachDenseElement(JSContext *cx, IonScript *ion, JSObject *obj, const Value &idval);
};

class IonCacheBindName : public IonCache
{
  public:
    IonCacheBindName(CodeOffsetJump initialJump,
                     CodeOffsetLabel rejoinLabel,
                     CodeOffsetLabel cacheLabel,
                     RegisterSet liveRegs,
                     Register scopeChain, PropertyName *name,
                     Register output)
    {
        init(BindName, liveRegs, initialJump, rejoinLabel, cacheLabel);
        u.bindname.scopeChain = scopeChain;
        u.bindname.name = name;
        u.bindname.output = output;
    }

    Register scopeChainReg() const {
        return u.bindname.scopeChain;
    }
    HandlePropertyName name() const {
        return HandlePropertyName::fromMarkedLocation(&u.bindname.name);
    }
    Register outputReg() const {
        return u.bindname.output;
    }

    bool attachGlobal(JSContext *cx, IonScript *ion, JSObject *scopeChain);
    bool attachNonGlobal(JSContext *cx, IonScript *ion, JSObject *scopeChain, JSObject *holder);
};

class IonCacheName : public IonCache
{
  public:
    IonCacheName(Kind kind,
                 CodeOffsetJump initialJump,
                 CodeOffsetLabel rejoinLabel,
                 CodeOffsetLabel cacheLabel,
                 RegisterSet liveRegs,
                 Register scopeChain, PropertyName *name,
                 TypedOrValueRegister output)
    {
        init(kind, liveRegs, initialJump, rejoinLabel, cacheLabel);
        u.name.scopeChain = scopeChain;
        u.name.name = name;
        u.name.output.data() = output;
    }

    Register scopeChainReg() const {
        return u.name.scopeChain;
    }
    HandlePropertyName name() const {
        return HandlePropertyName::fromMarkedLocation(&u.name.name);
    }
    TypedOrValueRegister outputReg() const {
        return u.name.output.data();
    }
    bool isTypeOf() const {
        return kind_ == NameTypeOf;
    }

    bool attach(JSContext *cx, IonScript *ion, HandleObject scopeChain, HandleObject obj,
                HandleShape shape);
};

class IonCacheCallsiteClone : public IonCache
{
  public:
    IonCacheCallsiteClone(CodeOffsetJump initialJump,
                          CodeOffsetLabel rejoinLabel,
                          CodeOffsetLabel cacheLabel,
                          RegisterSet liveRegs,
                          Register callee, JSScript *callScript, jsbytecode *callPc,
                          Register output)
    {
        init(CallsiteClone, liveRegs, initialJump, rejoinLabel, cacheLabel);
        u.callsiteclone.callee = callee;
        u.callsiteclone.callScript = callScript;
        u.callsiteclone.callPc = callPc;
        u.callsiteclone.output = output;
    }

    Register calleeReg() const {
        return u.callsiteclone.callee;
    }
    HandleScript callScript() const {
        return HandleScript::fromMarkedLocation(&u.callsiteclone.callScript);
    }
    jsbytecode *callPc() const {
        return u.callsiteclone.callPc;
    }
    Register outputReg() const {
        return u.callsiteclone.output;
    }

    bool attach(JSContext *cx, IonScript *ion, HandleFunction original, HandleFunction clone);
};

bool
GetPropertyCache(JSContext *cx, size_t cacheIndex, HandleObject obj, MutableHandleValue vp);

bool
SetPropertyCache(JSContext *cx, size_t cacheIndex, HandleObject obj, HandleValue value,
                 bool isSetName);

bool
GetElementCache(JSContext *cx, size_t cacheIndex, HandleObject obj, HandleValue idval,
                MutableHandleValue vp);

JSObject *
BindNameCache(JSContext *cx, size_t cacheIndex, HandleObject scopeChain);

bool
GetNameCache(JSContext *cx, size_t cacheIndex, HandleObject scopeChain, MutableHandleValue vp);

JSObject *
CallsiteCloneCache(JSContext *cx, size_t cacheIndex, HandleObject callee);

} 
} 

#endif 
