






































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

void PurgePICs(JSContext *cx);

struct PICInfo {
    typedef JSC::MacroAssembler::RegisterID RegisterID;

    
    enum Kind
#ifdef _MSC_VER
    : uint8_t
#endif
    {
        GET,
        CALL,
        SET,
        NAME,
        BIND
    };

    union {
        
        struct {
            RegisterID typeReg  : 5;  
            bool hasTypeCheck   : 1;  

            
            uint16 typeCheckOffset : 9;

            
            uint32 objRemat    : 20;
            bool objNeedsRemat : 1;

            
            
            
            int secondShapeGuard : 8;
        } get;
        ValueRemat vr;
    } u;

    Kind kind : 3;

    
    
    bool shapeRegHasBaseShape : 1;

    
    bool hit : 1;                   
    bool inlinePathPatched : 1;     

    RegisterID shapeReg : 5;        
    RegisterID objReg   : 5;        

    inline bool isGet() {
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
    inline uint32 objRemat() {
        JS_ASSERT(isGet());
        return u.get.objRemat;
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

    
    uint32 stubsGenerated : 5;

    
    int shapeGuard : 8;
    
    
    JSAtom *atom;

    
    JSC::CodeLocationLabel fastPathStart;

    
    JSC::CodeLocationLabel storeBack;

    
    uint16 callReturn : 9;

    
    JSC::CodeLocationLabel slowPathStart;

    
    JSC::CodeLocationLabel lastStubStart;

    typedef Vector<JSC::ExecutablePool *, 0, SystemAllocPolicy> ExecPoolVector;

    
    ExecPoolVector execPools;

    
    
    JSC::CodeLocationLabel lastPathStart() {
        return stubsGenerated > 0 ? lastStubStart : fastPathStart;
    }

    bool shouldGenerate() {
        return stubsGenerated < MAX_PIC_STUBS || !inlinePathPatched;
    }

    
    void releasePools() {
        for (JSC::ExecutablePool **pExecPool = execPools.begin();
             pExecPool != execPools.end();
             ++pExecPool)
        {
            (*pExecPool)->release();
        }
    }

    
    
    void reset() {
        hit = false;
        inlinePathPatched = false;
        if (kind == GET) {
            u.get.secondShapeGuard = 0;
            u.get.objNeedsRemat = false;
        }
        shapeRegHasBaseShape = true;
        stubsGenerated = 0;
        releasePools();
        execPools.clear();
    }
};

void PurgePICs(JSContext *cx, JSScript *script);
void JS_FASTCALL GetProp(VMFrame &f, uint32 index);
void JS_FASTCALL SetProp(VMFrame &f, uint32 index);
void JS_FASTCALL CallProp(VMFrame &f, uint32 index);
void JS_FASTCALL Name(VMFrame &f, uint32 index);
void JS_FASTCALL BindName(VMFrame &f, uint32 index);

}
} 
} 

#endif 

