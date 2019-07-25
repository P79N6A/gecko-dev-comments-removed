






































#if !defined jsjaeger_poly_ic_h__ && defined JS_METHODJIT && defined JS_POLYIC
#define jsjaeger_poly_ic_h__

#include "jscntxt.h"
#include "jstl.h"
#include "jsvector.h"
#include "assembler/assembler/MacroAssembler.h"
#include "assembler/assembler/CodeLocation.h"
#include "methodjit/MethodJIT.h"
#include "RematInfo.h"

namespace js {
namespace mjit {
namespace ic {


static const uint32 MAX_PIC_STUBS = 16;


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
        
        int32 dslotsLoadOffset : 8;

        
        int32 inlineShapeOffset : 8;
        
        
        int32 inlineAtomOffset : 8;

        
        int32 inlineValueOffset : 8;

        
        
        
        int32 stubShapeJump : 8;
    } getelem;

    
    struct {
        
        int32 inlineJumpOffset : 8;
    } bindname;
};
#endif

struct BaseIC {
    
    JSC::CodeLocationLabel fastPathStart;

    
    JSC::CodeLocationLabel fastPathRejoin;

    
    JSC::CodeLocationLabel slowPathStart;

    
    JSC::CodeLocationCall slowPathCall;

    
    JSC::CodeLocationLabel lastStubStart;

    typedef Vector<JSC::ExecutablePool *, 0, SystemAllocPolicy> ExecPoolVector;

    
    ExecPoolVector execPools;

    
    
    JSC::CodeLocationLabel lastPathStart() {
        return stubsGenerated > 0 ? lastStubStart : fastPathStart;
    }

    
    bool hit : 1;

    
    uint32 stubsGenerated : 5;

    
    void releasePools() {
        for (JSC::ExecutablePool **pExecPool = execPools.begin();
             pExecPool != execPools.end();
             ++pExecPool) {
            (*pExecPool)->release();
        }
    }

    void reset() {
        hit = false;
        stubsGenerated = 0;
        releasePools();
        execPools.clear();
    }
};

struct PICInfo : public BaseIC {
    typedef JSC::MacroAssembler::RegisterID RegisterID;

    
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
        GETELEM,    
        XNAME       
    };

    union {
        struct {
            RegisterID typeReg  : 5;  
            bool hasTypeCheck   : 1;  

            
            int32 typeCheckOffset;

            
            int32 objRemat      : MIN_STATE_REMAT_BITS;
            bool objNeedsRemat  : 1;
            RegisterID idReg    : 5;  
        } get;
        ValueRemat vr;
    } u;

    
    
    
    int secondShapeGuard : 11;

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
        return kind == GET || kind == CALL || kind == GETELEM;
    }
    inline RegisterID typeReg() {
        JS_ASSERT(isGet());
        return u.get.typeReg;
    }
    inline bool hasTypeCheck() {
        JS_ASSERT(isGet());
        return u.get.hasTypeCheck;
    }
    inline const StateRemat objRemat() const {
        JS_ASSERT(isGet());
        return StateRemat::FromInt32(u.get.objRemat);
    }
    inline bool objNeedsRemat() {
        JS_ASSERT(isGet());
        return u.get.objNeedsRemat;
    }
    inline bool shapeNeedsRemat() {
        return !shapeRegHasBaseShape;
    }
    inline bool isFastCall() {
        JS_ASSERT(kind == CALL);
        return !hasTypeCheck();
    }

    inline void setObjRemat(const StateRemat &sr) {
        JS_ASSERT(isGet());
        u.get.objRemat = sr.toInt32();
        JS_ASSERT(u.get.objRemat == sr.toInt32());
    }

#if defined JS_CPU_X64
    
    PICLabels labels;
#endif

    
    jsbytecode *pc;
    
    
    JSAtom *atom;

    bool shouldGenerate() {
        return stubsGenerated < MAX_PIC_STUBS || !inlinePathPatched;
    }

    
    
    void reset() {
        inlinePathPatched = false;
        if (kind == GET || kind == CALL || kind == GETELEM)
            u.get.objNeedsRemat = false;
        secondShapeGuard = 0;
        shapeRegHasBaseShape = true;
        BaseIC::reset();
    }
};

void PurgePICs(JSContext *cx, JSScript *script);
void JS_FASTCALL GetProp(VMFrame &f, ic::PICInfo *);
void JS_FASTCALL GetElem(VMFrame &f, ic::PICInfo *);
void JS_FASTCALL SetProp(VMFrame &f, ic::PICInfo *);
void JS_FASTCALL CallProp(VMFrame &f, ic::PICInfo *);
void JS_FASTCALL Name(VMFrame &f, ic::PICInfo *);
void JS_FASTCALL XName(VMFrame &f, ic::PICInfo *);
void JS_FASTCALL BindName(VMFrame &f, ic::PICInfo *);

} 
} 
} 

#endif 

