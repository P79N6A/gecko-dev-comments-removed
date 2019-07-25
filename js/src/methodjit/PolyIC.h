






































#if !defined jsjaeger_poly_ic_h__ && defined JS_METHODJIT
#define jsjaeger_poly_ic_h__

#include "jscntxt.h"
#include "jstl.h"
#include "jsvector.h"
#include "assembler/assembler/MacroAssembler.h"
#include "assembler/assembler/CodeLocation.h"
#include "methodjit/MethodJIT.h"

#define ENABLE_PIC 1

namespace js {
namespace mjit {
namespace ic {

static const uint32 MAX_PIC_STUBS = 16;

void PurgePICs(JSContext *cx);

struct PICInfo {
    typedef JSC::MacroAssembler::RegisterID RegisterID;

    
    enum Kind {
        GET,
        CALL,
        SET
    };

    Kind kind : 2;

    
    bool hit : 1;                   
    bool inlinePathPatched : 1;     

    
    
    bool shapeRegHasBaseShape : 1;
    RegisterID shapeReg : 5;        
    RegisterID objReg   : 5;        

    
    bool startsWithShapeLoad : 1;

    
    uint32 stubsGenerated : 8;

    
    int shapeGuard : 8;

    
    
    
    int secondShapeGuard : 8;

    
    uint32 atomIndex;

    
    uint32 objRemat    : 20;
    bool objNeedsRemat : 1;

    
    JSC::CodeLocationLabel fastPathStart;

    
    JSC::CodeLocationLabel storeBack;

    
    uint8 callReturn;

    
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
        objNeedsRemat = false;
        shapeRegHasBaseShape = true;
        secondShapeGuard = 0;
        stubsGenerated = 0;
        releasePools();
        execPools.clear();
    }
};

void PurgePICs(JSContext *cx, JSScript *script);
void JS_FASTCALL GetProp(VMFrame &f, uint32 index);

}
} 
} 

#endif 

