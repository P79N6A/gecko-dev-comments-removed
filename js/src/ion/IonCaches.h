








































#ifndef jsion_caches_h__
#define jsion_caches_h__

#include "IonCode.h"
#include "TypeOracle.h"
#include "Registers.h"

struct JSFunction;
struct JSScript;

namespace js {
namespace ion {

class IonCacheGetProperty;
class IonCacheSetProperty;
class IonCacheGetElement;
class IonCacheBindName;
class IonCacheName;
































struct TypedOrValueRegisterSpace
{
    AlignedStorage2<TypedOrValueRegister> data_;
    TypedOrValueRegister &data() {
        return *data_.addr();
    }
    const TypedOrValueRegister &data() const {
        return *data_.addr();
    }
};

struct ConstantOrRegisterSpace
{
    AlignedStorage2<ConstantOrRegister> data_;
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
        NameTypeOf
    };

  protected:
    Kind kind_ : 8;
    bool pure_ : 1;
    bool idempotent_ : 1;
    size_t stubCount_ : 6;

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
            bool hasDenseArrayStub : 1;
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
    
    
    void reset();

    CodeLocationJump lastJump() const { return lastJump_; }
    CodeLocationLabel cacheLabel() const { return cacheLabel_; }

    CodeLocationLabel rejoinLabel() const {
        uint8 *ptr = initialJump_.raw();
#ifdef JS_CPU_ARM
        uint32 i = 0;
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

    void setScriptedLocation(JSScript *script, jsbytecode *pc) {
        JS_ASSERT(!idempotent_);
        this->script = script;
        this->pc = pc;
    }

    void getScriptedLocation(JSScript **pscript, jsbytecode **ppc) {
        *pscript = script;
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
                        TypedOrValueRegister output)
    {
        init(GetProperty, liveRegs, initialJump, rejoinLabel, cacheLabel);
        u.getprop.object = object;
        u.getprop.name = name;
        u.getprop.output.data() = output;
    }

    Register object() const { return u.getprop.object; }
    PropertyName *name() const { return u.getprop.name; }
    TypedOrValueRegister output() const { return u.getprop.output.data(); }

    bool attachNative(JSContext *cx, JSObject *obj, JSObject *holder, const Shape *shape);
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

    bool attachNativeExisting(JSContext *cx, JSObject *obj, const Shape *shape);
    bool attachNativeAdding(JSContext *cx, JSObject *obj, const Shape *oldshape, const Shape *newshape,
                            const Shape *propshape);
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
        u.getelem.hasDenseArrayStub = false;
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
    bool hasDenseArrayStub() const {
        return u.getelem.hasDenseArrayStub;
    }
    void setHasDenseArrayStub() {
        JS_ASSERT(!hasDenseArrayStub());
        u.getelem.hasDenseArrayStub = true;
    }

    bool attachGetProp(JSContext *cx, JSObject *obj, const Value &idval, PropertyName *name,
                       Value *res);
    bool attachDenseArray(JSContext *cx, JSObject *obj, const Value &idval, Value *res);
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

    bool attachGlobal(JSContext *cx, JSObject *scopeChain);
    bool attachNonGlobal(JSContext *cx, JSObject *scopeChain, JSObject *holder);
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

    bool attach(JSContext *cx, HandleObject scopeChain, HandleObject obj, Shape *shape);
};

bool
GetPropertyCache(JSContext *cx, size_t cacheIndex, HandleObject obj, Value *vp);

bool
SetPropertyCache(JSContext *cx, size_t cacheIndex, HandleObject obj, HandleValue value,
                 bool isSetName);

bool
GetElementCache(JSContext *cx, size_t cacheIndex, JSObject *obj, const Value &idval, Value *res);

JSObject *
BindNameCache(JSContext *cx, size_t cacheIndex, HandleObject scopeChain);

bool
GetNameCache(JSContext *cx, size_t cacheIndex, HandleObject scopeChain, Value *vp);

} 
} 

#endif 
