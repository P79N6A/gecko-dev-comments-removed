






#if !defined jsjaeger_poly_ic_h__ && defined JS_METHODJIT
#define jsjaeger_poly_ic_h__

#include "jscntxt.h"
#include "assembler/assembler/MacroAssembler.h"
#include "assembler/assembler/CodeLocation.h"
#include "js/Vector.h"
#include "methodjit/MethodJIT.h"
#include "methodjit/ICRepatcher.h"
#include "BaseAssembler.h"
#include "RematInfo.h"
#include "BaseCompiler.h"
#include "methodjit/ICLabels.h"
#include "assembler/moco/MocoStubs.h"

namespace js {
namespace mjit {
namespace ic {


static const uint32_t MAX_PIC_STUBS = 16;
static const uint32_t MAX_GETELEM_IC_STUBS = 17;

enum LookupStatus {
    Lookup_Error = 0,
    Lookup_Uncacheable,
    Lookup_Cacheable,
    Lookup_NoProperty
};

struct BaseIC : public MacroAssemblerTypedefs {

    
    CodeLocationLabel fastPathStart;

    
    CodeLocationLabel fastPathRejoin;

    
    CodeLocationLabel slowPathStart;

    
    CodeLocationCall slowPathCall;

    
    
    
    int32_t secondShapeGuard;

    
    bool hit : 1;
    bool slowCallPatched : 1;

    
    bool canCallHook : 1;

    
    bool forcedTypeBarrier : 1;

    
    bool disabled : 1;

    
    uint32_t stubsGenerated : 5;

    bool shouldUpdate(VMFrame &f);
    void spew(VMFrame &f, const char *event, const char *reason);
    LookupStatus disable(VMFrame &f, const char *reason, void *stub);
    void updatePCCounters(VMFrame &f, Assembler &masm);

  protected:
    void reset() {
        hit = false;
        slowCallPatched = false;
        forcedTypeBarrier = false;
        disabled = false;
        stubsGenerated = 0;
        secondShapeGuard = 0;
    }
};

class BasePolyIC : public BaseIC {
    typedef Vector<JSC::ExecutablePool *, 2, SystemAllocPolicy> ExecPoolVector;

    
    
    
    
    
    
    
    union {
        JSC::ExecutablePool *execPool;      
        ExecPoolVector *taggedExecPools;    
    } u;

    static bool isTagged(void *p) {
        return !!(intptr_t(p) & 1);
    }

    static ExecPoolVector *tag(ExecPoolVector *p) {
        JS_ASSERT(!isTagged(p));
        return (ExecPoolVector *)(intptr_t(p) | 1);
    }

    static ExecPoolVector *detag(ExecPoolVector *p) {
        JS_ASSERT(isTagged(p));
        return (ExecPoolVector *)(intptr_t(p) & ~1);
    }

    bool areZeroPools()     { return !u.execPool; }
    bool isOnePool()        { return u.execPool && !isTagged(u.execPool); }
    bool areMultiplePools() { return isTagged(u.taggedExecPools); }

    ExecPoolVector *multiplePools() {
        JS_ASSERT(areMultiplePools());
        return detag(u.taggedExecPools);
    }

  public:
    bool addPool(JSContext *cx, JSC::ExecutablePool *pool) {
        if (areZeroPools()) {
            u.execPool = pool;
            return true;
        }
        if (isOnePool()) {
            JSC::ExecutablePool *oldPool = u.execPool;
            JS_ASSERT(!isTagged(oldPool));
            ExecPoolVector *execPools = js_new<ExecPoolVector>(SystemAllocPolicy());
            if (!execPools)
                return false;
            if (!execPools->append(oldPool) || !execPools->append(pool)) {
                js_delete(execPools);
                return false;
            }
            u.taggedExecPools = tag(execPools);
            return true;
        }
        return multiplePools()->append(pool);
    }

  protected:
    void reset() {
        BaseIC::reset();
        if (areZeroPools()) {
            
        } else if (isOnePool()) {
            u.execPool->release();
            u.execPool = NULL;
        } else {
            ExecPoolVector *execPools = multiplePools();
            for (size_t i = 0; i < execPools->length(); i++)
                (*execPools)[i]->release();
            js_delete(execPools);
            u.execPool = NULL;
        }
        JS_ASSERT(areZeroPools());
    }
};

struct GetElementIC : public BasePolyIC {

    
    
    
    
    
    
    
    
    
    RegisterID typeReg   : 5;

    
    
    RegisterID objReg    : 5;

    
    
    unsigned inlineTypeGuard  : 8;

    
    
    
    unsigned inlineShapeGuard : 8;

    
    
    
    bool inlineTypeGuardPatched : 1;

    
    
    
    
    
    bool inlineShapeGuardPatched : 1;

    
    
    

    
    bool typeRegHasBaseShape : 1;

    
    
    
    int32_t atomGuard : 8;          
    int32_t firstShapeGuard : 11;    
    int32_t secondShapeGuard : 11;   

    bool hasLastStringStub : 1;
    JITCode lastStringStub;

    
    
    
    
    
    
    
    ValueRemat idRemat;

    bool hasInlineTypeGuard() const {
        return !idRemat.isTypeKnown();
    }
    bool shouldPatchInlineTypeGuard() {
        return hasInlineTypeGuard() && !inlineTypeGuardPatched;
    }
    bool shouldPatchUnconditionalShapeGuard() {
        
        
        if (idRemat.isTypeKnown() && idRemat.knownType() != JSVAL_TYPE_INT32)
            return !inlineShapeGuardPatched;
        return false;
    }

    void purge(Repatcher &repatcher);
    LookupStatus update(VMFrame &f, HandleObject obj, HandleValue v, HandleId id, MutableHandleValue vp);
    LookupStatus attachGetProp(VMFrame &f, HandleObject obj, HandleValue v, HandlePropertyName name,
                               MutableHandleValue vp);
    LookupStatus attachTypedArray(VMFrame &f, HandleObject obj, HandleValue v, HandleId id,
                                  MutableHandleValue vp);
    LookupStatus disable(VMFrame &f, const char *reason);
    LookupStatus error(JSContext *cx);
    bool shouldUpdate(VMFrame &f);

  protected:
    void reset() {
        BasePolyIC::reset();
        inlineTypeGuardPatched = false;
        inlineShapeGuardPatched = false;
        typeRegHasBaseShape = false;
        hasLastStringStub = false;
    }
};

struct SetElementIC : public BaseIC {

    
    
    
    
    RegisterID objReg    : 5;

    
    int32_t objRemat       : MIN_STATE_REMAT_BITS;

    
    unsigned inlineShapeGuard : 6;

    
    bool inlineShapeGuardPatched : 1;

    
    unsigned inlineHoleGuard : 8;

    
    bool inlineHoleGuardPatched : 1;

    
    bool strictMode : 1;

    
    
    uint32_t volatileMask;

    
    
    bool hasConstantKey : 1;
    union {
        RegisterID keyReg;
        int32_t    keyValue;
    };

    
    ValueRemat vr;

    
    JSC::ExecutablePool *execPool;

    void purge(Repatcher &repatcher);
    LookupStatus attachTypedArray(VMFrame &f, JSObject *obj, int32_t key);
    LookupStatus update(VMFrame &f, const Value &objval, const Value &idval);
    LookupStatus disable(VMFrame &f, const char *reason);
    LookupStatus error(JSContext *cx);
    bool shouldUpdate(VMFrame &f);

  protected:
    void reset() {
        BaseIC::reset();
        if (execPool) {
            execPool->release();
            execPool = NULL;
        }
        inlineShapeGuardPatched = false;
        inlineHoleGuardPatched = false;
    }
};

struct PICInfo : public BasePolyIC {
    PICInfo() { reset(); }

    
    enum Kind
#ifdef _MSC_VER
    : uint8_t
#endif
    {
        GET,        
        SET,        
        NAME,       
        BIND,       
        XNAME       
    };

    union {
        struct {
            RegisterID typeReg  : 5;  
            bool hasTypeCheck   : 1;  

            
            int32_t typeCheckOffset;
        } get;
        ValueRemat vr;
    } u;

    
    
    
    JITCode lastStubStart;

    
    
    CodeLocationLabel lastPathStart() {
        if (!stubsGenerated)
            return fastPathStart;
        return CodeLocationLabel(lastStubStart.start());
    }

    CodeLocationLabel getFastShapeGuard() {
        return fastPathStart.labelAtOffset(shapeGuard);
    }

    CodeLocationLabel getSlowTypeCheck() {
        JS_ASSERT(isGet());
        return slowPathStart.labelAtOffset(u.get.typeCheckOffset);
    }

    
    
    JITCode lastCodeBlock(JITChunk *chunk) {
        if (!stubsGenerated)
            return JITCode(chunk->code.m_code.executableAddress(), chunk->code.m_size);
        return lastStubStart;
    }

    void updateLastPath(LinkerHelper &linker, Label label) {
        CodeLocationLabel loc = linker.locationOf(label);
        lastStubStart = JITCode(loc.executableAddress(), linker.size());
    }

    Kind kind : 3;

    
    
    bool shapeRegHasBaseShape : 1;

    
    bool hadUncacheable : 1;

    
    bool inlinePathPatched : 1;     

    RegisterID shapeReg : 5;        
    RegisterID objReg   : 5;        

    
    bool typeMonitored : 1;

    
    bool cached : 1;

    
    uint32_t shapeGuard;

    inline bool isSet() const {
        return kind == SET;
    }
    inline bool isGet() const {
        return kind == GET;
    }
    inline bool isBind() const {
        return kind == BIND;
    }
    inline bool isScopeName() const {
        return kind == NAME || kind == XNAME;
    }
    inline RegisterID typeReg() {
        JS_ASSERT(isGet());
        return u.get.typeReg;
    }
    inline bool hasTypeCheck() {
        JS_ASSERT(isGet());
        return u.get.hasTypeCheck;
    }
    inline bool shapeNeedsRemat() {
        return !shapeRegHasBaseShape;
    }

    union {
        GetPropLabels getPropLabels_;
        SetPropLabels setPropLabels_;
        BindNameLabels bindNameLabels_;
        ScopeNameLabels scopeNameLabels_;
    };
    void setLabels(const ic::GetPropLabels &labels) {
        JS_ASSERT(isGet());
        getPropLabels_ = labels;
    }
    void setLabels(const ic::SetPropLabels &labels) {
        JS_ASSERT(isSet());
        setPropLabels_ = labels;
    }
    void setLabels(const ic::BindNameLabels &labels) {
        JS_ASSERT(kind == BIND);
        bindNameLabels_ = labels;
    }
    void setLabels(const ic::ScopeNameLabels &labels) {
        JS_ASSERT(kind == NAME || kind == XNAME);
        scopeNameLabels_ = labels;
    }

    GetPropLabels &getPropLabels() {
        JS_ASSERT(isGet());
        return getPropLabels_;
    }
    SetPropLabels &setPropLabels() {
        JS_ASSERT(isSet());
        return setPropLabels_;
    }
    BindNameLabels &bindNameLabels() {
        JS_ASSERT(kind == BIND);
        return bindNameLabels_;
    }
    ScopeNameLabels &scopeNameLabels() {
        JS_ASSERT(kind == NAME || kind == XNAME);
        return scopeNameLabels_;
    }

    
    jsbytecode *pc;

    
    PropertyName *name;

  private:
    Shape *inlinePathShape_;

  public:
    void purge(Repatcher &repatcher);

    void setInlinePathShape(RawShape shape) {
        JS_ASSERT(!inlinePathShape_);
        inlinePathShape_ = shape;
    }

    RawShape getSingleShape() {
        if (disabled || hadUncacheable || stubsGenerated > 0)
            return NULL;
        return inlinePathShape_;
    }

  protected:
    
    
    void reset() {
        BasePolyIC::reset();
        inlinePathPatched = false;
        shapeRegHasBaseShape = true;
        hadUncacheable = false;
        inlinePathShape_ = NULL;
    }
};

#ifdef JS_POLYIC
void JS_FASTCALL GetProp(VMFrame &f, ic::PICInfo *);
void JS_FASTCALL SetPropOrName(VMFrame &f, ic::PICInfo *);
void JS_FASTCALL Name(VMFrame &f, ic::PICInfo *);
void JS_FASTCALL XName(VMFrame &f, ic::PICInfo *);
void JS_FASTCALL BindName(VMFrame &f, ic::PICInfo *);
void JS_FASTCALL GetElement(VMFrame &f, ic::GetElementIC *);
template <JSBool strict> void JS_FASTCALL SetElement(VMFrame &f, ic::SetElementIC *);
#endif

} 
} 
} 

#endif

