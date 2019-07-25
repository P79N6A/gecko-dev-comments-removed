








































#ifndef jsion_caches_h__
#define jsion_caches_h__

#include "IonCode.h"
#include "TypeOracle.h"
#include "IonRegisters.h"

struct JSFunction;
struct JSScript;

namespace js {
namespace ion {

class IonCacheGetProperty;
































class IonCache
{
  protected:

    enum Kind {
        Invalid = 0,

        
        GetProperty
    } kind : 8;

    bool pure_ : 1;
    bool idempotent_ : 1;
    size_t stubCount_ : 6;

    CodeLocationJump initialJump_;
    CodeLocationJump lastJump_;
    CodeLocationLabel cacheLabel_;

    
    static const size_t REJOIN_LABEL_OFFSET = 0;

    union {
        struct {
            Register object;
            JSAtom *atom;
        } getprop;
    } input;

    TypedOrValueRegister output;

    
    
    RegisterSet liveRegs;

    
    JSScript *script;
    jsbytecode *pc;

    void init(Kind kind, RegisterSet liveRegs, TypedOrValueRegister output,
              CodeOffsetJump initialJump,
              CodeOffsetLabel rejoinLabel,
              CodeOffsetLabel cacheLabel) {
        this->kind = kind;
        this->liveRegs = liveRegs;
        this->output = output;
        this->initialJump_ = initialJump;
        this->lastJump_ = initialJump;
        this->cacheLabel_ = cacheLabel;

        JS_ASSERT(rejoinLabel.offset() == initialJump.offset() + REJOIN_LABEL_OFFSET);
    }

    void loadResult(MacroAssembler &masm, Address address);

  public:

    IonCache() { PodZero(this); }

    void updateBaseAddress(IonCode *code) {
        initialJump_.repoint(code);
        lastJump_.repoint(code);
        cacheLabel_.repoint(code);
    }

    CodeLocationJump lastJump() const { return lastJump_; }
    CodeLocationLabel cacheLabel() const { return cacheLabel_; }

    CodeLocationLabel rejoinLabel() const {
        return CodeLocationLabel(lastJump().raw() + REJOIN_LABEL_OFFSET);
    }

    bool pure() { return pure_; }
    bool idempotent() { return idempotent_; }

    void updateLastJump(CodeLocationJump jump) { lastJump_ = jump; }

    size_t stubCount() const { return stubCount_; }
    void incrementStubCount() {
        
        stubCount_++;
        JS_ASSERT(stubCount_);
    }

    IonCacheGetProperty &toGetProperty() {
        JS_ASSERT(kind == GetProperty);
        return * (IonCacheGetProperty *) this;
    }

    void setScriptedLocation(JSScript *script, jsbytecode *pc) {
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
                        Register object, JSAtom *atom,
                        TypedOrValueRegister output)
    {
        init(GetProperty, liveRegs, output, initialJump, rejoinLabel, cacheLabel);
        this->input.getprop.object = object;
        this->input.getprop.atom = atom;
    }

    Register object() const { return input.getprop.object; }
    JSAtom *atom() const { return input.getprop.atom; }

    bool attachNative(JSContext *cx, JSObject *obj, const Shape *shape);
};

extern const VMFunction GetPropertyCacheFun;

} 
} 

#endif 
