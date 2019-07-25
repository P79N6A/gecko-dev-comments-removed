






































#if !defined jsjaeger_poly_ic_h__ && defined JS_METHODJIT
#define jsjaeger_poly_ic_h__

#include "jscntxt.h"
#include "jstl.h"
#include "jsvector.h"
#include "assembler/assembler/MacroAssembler.h"
#include "assembler/assembler/CodeLocation.h"
#include "methodjit/MethodJIT.h"
#include "BaseAssembler.h"
#include "RematInfo.h"
#include "BaseCompiler.h"
#include "assembler/moco/MocoStubs.h"

namespace js {
namespace mjit {
namespace ic {


static const uint32 MAX_PIC_STUBS = 16;
static const uint32 MAX_GETELEM_IC_STUBS = 17;


#if defined JS_CPU_X86
static const int32 SETPROP_INLINE_SHAPE_OFFSET     =   6; 
static const int32 SETPROP_INLINE_SHAPE_JUMP       =  12; 
static const int32 SETPROP_DSLOTS_BEFORE_CONSTANT  = -23; 
static const int32 SETPROP_DSLOTS_BEFORE_KTYPE     = -19; 
static const int32 SETPROP_DSLOTS_BEFORE_DYNAMIC   = -15; 
static const int32 SETPROP_INLINE_STORE_DYN_TYPE   =  -6; 
static const int32 SETPROP_INLINE_STORE_DYN_DATA   =   0; 
static const int32 SETPROP_INLINE_STORE_KTYPE_TYPE = -10; 
static const int32 SETPROP_INLINE_STORE_KTYPE_DATA =   0; 
static const int32 SETPROP_INLINE_STORE_CONST_TYPE = -14; 
static const int32 SETPROP_INLINE_STORE_CONST_DATA =  -4; 
static const int32 SETPROP_STUB_SHAPE_JUMP         =  12; 
#elif defined JS_CPU_X64
static const int32 SETPROP_INLINE_STORE_VALUE      =   0; 
static const int32 SETPROP_INLINE_SHAPE_JUMP       =   6; 
#endif


#if defined JS_CPU_X86
static const int32 GETPROP_DSLOTS_LOAD         = -15; 
static const int32 GETPROP_TYPE_LOAD           =  -6; 
static const int32 GETPROP_DATA_LOAD           =   0; 
static const int32 GETPROP_INLINE_TYPE_GUARD   =  12; 
static const int32 GETPROP_INLINE_SHAPE_OFFSET =   6; 
static const int32 GETPROP_INLINE_SHAPE_JUMP   =  12; 
static const int32 GETPROP_STUB_SHAPE_JUMP     =  12; 
#elif defined JS_CPU_X64
static const int32 GETPROP_INLINE_TYPE_GUARD   =  19; 
static const int32 GETPROP_INLINE_SHAPE_JUMP   =   6; 
#endif


#if defined JS_CPU_X86
static const int32 GETELEM_DSLOTS_LOAD         = -15; 
static const int32 GETELEM_TYPE_LOAD           =  -6; 
static const int32 GETELEM_DATA_LOAD           =   0; 
static const int32 GETELEM_INLINE_SHAPE_OFFSET =   6; 
static const int32 GETELEM_INLINE_SHAPE_JUMP   =  12; 
static const int32 GETELEM_INLINE_ATOM_OFFSET  =  18; 
static const int32 GETELEM_INLINE_ATOM_JUMP    =  24; 
static const int32 GETELEM_STUB_ATOM_JUMP      =  12; 
static const int32 GETELEM_STUB_SHAPE_JUMP     =  24; 
#elif defined JS_CPU_X64
static const int32 GETELEM_INLINE_SHAPE_JUMP   =   6; 
static const int32 GETELEM_INLINE_ATOM_JUMP    =   9; 
static const int32 GETELEM_STUB_ATOM_JUMP      =  19; 
#endif


#if defined JS_CPU_X86
static const int32 SCOPENAME_JUMP_OFFSET = 5; 
#elif defined JS_CPU_X64
static const int32 SCOPENAME_JUMP_OFFSET = 5; 
#endif


#if defined JS_CPU_X86
static const int32 BINDNAME_INLINE_JUMP_OFFSET = 10; 
static const int32 BINDNAME_STUB_JUMP_OFFSET   =  5; 
#elif defined JS_CPU_X64
static const int32 BINDNAME_STUB_JUMP_OFFSET   =  5; 
#endif

void PurgePICs(JSContext *cx);





#if defined JS_CPU_X64
union PICLabels {
    
    struct {
        
        int32 dslotsLoadOffset : 8;

        
        int32 inlineShapeOffset : 8;

        
        
        
        int32 stubShapeJump : 8;
    } setprop;

    
    struct {
        
        int32 dslotsLoadOffset : 8;

        
        int32 inlineShapeOffset : 8;
    
        
        int32 inlineValueOffset : 8;

        
        
        
        int32 stubShapeJump : 8;
    } getprop;

    
    struct {
        
        int32 inlineJumpOffset : 8;
    } bindname;
};
#endif

enum LookupStatus {
    Lookup_Error = 0,
    Lookup_Uncacheable,
    Lookup_Cacheable
};

struct BaseIC : public MacroAssemblerTypedefs {
    BaseIC() { }

    
    CodeLocationLabel fastPathStart;

    
    CodeLocationLabel fastPathRejoin;

    
    CodeLocationLabel slowPathStart;

    
    CodeLocationCall slowPathCall;

    
    bool hit : 1;
    bool slowCallPatched : 1;

    
    uint32 stubsGenerated : 5;

    
    
    
    int secondShapeGuard : 11;

    
    JSOp op : 9;

    void reset() {
        hit = false;
        slowCallPatched = false;
        stubsGenerated = 0;
        secondShapeGuard = 0;
    }
    bool shouldUpdate(JSContext *cx);
    void spew(JSContext *cx, const char *event, const char *reason);
    LookupStatus disable(JSContext *cx, const char *reason, void *stub);
    bool isCallOp();
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
    BasePolyIC() {
        u.execPool = NULL;
    }

    ~BasePolyIC() {
        releasePools();
        if (areMultiplePools())
            delete multiplePools();
    }

    void reset() {
        BaseIC::reset();
        releasePools();
        if (areZeroPools()) {
            
        } else if (isOnePool()) {
            u.execPool = NULL;
        } else {
            multiplePools()->clear();
        }
    }

    void releasePools() {
        if (areZeroPools()) {
            
        } else if (isOnePool()) {
            u.execPool->release();
        } else {
            ExecPoolVector *execPools = multiplePools();
            for (size_t i = 0; i < execPools->length(); i++)
                (*execPools)[i]->release();
        }
    }

    bool addPool(JSContext *cx, JSC::ExecutablePool *pool) {
        if (areZeroPools()) {
            u.execPool = pool;
            return true;
        }
        if (isOnePool()) {
            JSC::ExecutablePool *oldPool = u.execPool;
            JS_ASSERT(!isTagged(oldPool));
            ExecPoolVector *execPools = new ExecPoolVector(SystemAllocPolicy()); 
            if (!execPools)
                return false;
            if (!execPools->append(oldPool) || !execPools->append(pool)) {
                delete execPools;
                return false;
            }
            u.taggedExecPools = tag(execPools);
            return true;
        }
        return multiplePools()->append(pool); 
    }
};

struct GetElementIC : public BasePolyIC {
    GetElementIC() { reset(); }

    
    
    
    
    
    
    
    
    
    RegisterID typeReg   : 5;

    
    
    RegisterID objReg    : 5;

    
    
    unsigned inlineTypeGuard  : 8;

    
    
    
    unsigned inlineClaspGuard : 8;

    
    
    
    bool inlineTypeGuardPatched : 1;

    
    
    
    
    
    bool inlineClaspGuardPatched : 1;

    
    
    

    
    bool typeRegHasBaseShape : 1;

    
    
    
    int atomGuard : 8;          
    int firstShapeGuard : 8;    
    int secondShapeGuard : 8;   

    bool hasLastStringStub : 1;
    JITCode lastStringStub;

    
    
    
    
    
    
    
    ValueRemat idRemat;

    bool hasInlineTypeGuard() const {
        return !idRemat.isTypeKnown();
    }
    bool shouldPatchInlineTypeGuard() {
        return hasInlineTypeGuard() && !inlineTypeGuardPatched;
    }
    bool shouldPatchUnconditionalClaspGuard() {
        return !hasInlineTypeGuard() && !inlineClaspGuardPatched;
    }

    void reset() {
        BasePolyIC::reset();
        inlineTypeGuardPatched = false;
        inlineClaspGuardPatched = false;
        typeRegHasBaseShape = false;
        hasLastStringStub = false;
    }
    void purge(Repatcher &repatcher);
    LookupStatus update(JSContext *cx, JSObject *obj, const Value &v, jsid id, Value *vp);
    LookupStatus attachGetProp(JSContext *cx, JSObject *obj, const Value &v, jsid id,
                               Value *vp);
    LookupStatus disable(JSContext *cx, const char *reason);
    LookupStatus error(JSContext *cx);
    bool shouldUpdate(JSContext *cx);
};

struct SetElementIC : public BaseIC {
    SetElementIC() : execPool(NULL) { reset(); }
    ~SetElementIC() {
        if (execPool)
            execPool->release();
    }

    
    
    
    
    RegisterID objReg    : 5;

    
    int32 objRemat       : MIN_STATE_REMAT_BITS;

    
    unsigned inlineClaspGuard : 6;

    
    bool inlineClaspGuardPatched : 1;

    
    unsigned inlineHoleGuard : 8;

    
    bool inlineHoleGuardPatched : 1;

    
    bool strictMode : 1;

    
    
    bool hasConstantKey : 1;
    union {
        RegisterID keyReg;
        int32      keyValue;
    };

    
    ValueRemat vr;

    
    JSC::ExecutablePool *execPool;

    void reset() {
        BaseIC::reset();
        if (execPool != NULL)
            execPool->release();
        execPool = NULL;
        inlineClaspGuardPatched = false;
        inlineHoleGuardPatched = false;
    }
    void purge(Repatcher &repatcher);
    LookupStatus attachHoleStub(JSContext *cx, JSObject *obj, int32 key);
    LookupStatus update(JSContext *cx, const Value &objval, const Value &idval);
    LookupStatus disable(JSContext *cx, const char *reason);
    LookupStatus error(JSContext *cx);
};

struct PICInfo : public BasePolyIC {
    PICInfo() { reset(); }

    
    enum Kind
#ifdef _MSC_VER
    : uint8_t
#endif
    {
        GET,        
        CALL,       
        SET,        
        SETMETHOD,  
        NAME,       
        BIND,       
        XNAME       
    };

    union {
        struct {
            RegisterID typeReg  : 5;  
            bool hasTypeCheck   : 1;  

            
            int32 typeCheckOffset;
        } get;
        ValueRemat vr;
    } u;

    
    
    
    JITCode lastStubStart;

    
    
    CodeLocationLabel lastPathStart() {
        if (!stubsGenerated)
            return fastPathStart;
        return CodeLocationLabel(lastStubStart.start());
    }

    
    
    JITCode lastCodeBlock(JITScript *jit) {
        if (!stubsGenerated)
            return JITCode(jit->code.m_code.executableAddress(), jit->code.m_size);
        return lastStubStart;
    }

    void updateLastPath(LinkerHelper &linker, Label label) {
        CodeLocationLabel loc = linker.locationOf(label);
        lastStubStart = JITCode(loc.executableAddress(), linker.size());
    }

    Kind kind : 3;

    
    
    bool shapeRegHasBaseShape : 1;

    
    bool usePropCache : 1;

    
    bool inlinePathPatched : 1;     

    RegisterID shapeReg : 5;        
    RegisterID objReg   : 5;        

    
    uint32 shapeGuard;
    
    inline bool isSet() const {
        return kind == SET || kind == SETMETHOD;
    }
    inline bool isGet() const {
        return kind == GET || kind == CALL;
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
    inline bool isFastCall() {
        JS_ASSERT(kind == CALL);
        return !hasTypeCheck();
    }

#if defined JS_CPU_X64
    
    PICLabels labels;
#endif

    
    jsbytecode *pc;
    
    
    JSAtom *atom;

    
    
    void reset() {
        BasePolyIC::reset();
        inlinePathPatched = false;
        shapeRegHasBaseShape = true;
    }
};

#ifdef JS_POLYIC
void PurgePICs(JSContext *cx, JSScript *script);
void JS_FASTCALL GetProp(VMFrame &f, ic::PICInfo *);
void JS_FASTCALL SetProp(VMFrame &f, ic::PICInfo *);
void JS_FASTCALL CallProp(VMFrame &f, ic::PICInfo *);
void JS_FASTCALL Name(VMFrame &f, ic::PICInfo *);
void JS_FASTCALL XName(VMFrame &f, ic::PICInfo *);
void JS_FASTCALL BindName(VMFrame &f, ic::PICInfo *);
void JS_FASTCALL GetElement(VMFrame &f, ic::GetElementIC *);
void JS_FASTCALL CallElement(VMFrame &f, ic::GetElementIC *);
template <JSBool strict> void JS_FASTCALL SetElement(VMFrame &f, ic::SetElementIC *);
#endif

} 
} 
} 

#endif 

