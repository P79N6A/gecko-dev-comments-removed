












































#if !defined jsjaeger_retcon_h__ && defined JS_METHODJIT
#define jsjaeger_retcon_h__

#include "jscntxt.h"
#include "jsscript.h"
#include "MethodJIT.h"
#include "Compiler.h"

namespace js {
namespace mjit {







class AutoScriptRetrapper
{
  public:
    AutoScriptRetrapper(JSContext *cx, JSScript *script1) :
        script(script1), traps(cx) {};
    ~AutoScriptRetrapper();

    bool untrap(jsbytecode *pc);

  private:
    JSScript *script;
    Vector<jsbytecode*> traps;
};








class Recompiler {
    struct PatchableAddress {
        void **location;
        CallSite callSite;
    };
    
public:
    Recompiler(JSContext *cx, JSScript *script);
    
    bool recompile();

private:
    JSContext *cx;
    JSScript *script;
    
    PatchableAddress findPatch(JITScript *jit, void **location);
    void applyPatch(Compiler& c, PatchableAddress& toPatch);
    bool recompile(JSStackFrame *fp, Vector<PatchableAddress> &patches,
                   Vector<CallSite> &sites);
    bool saveTraps(JITScript *jit, Vector<CallSite> *sites);
};

} 
} 

#endif

