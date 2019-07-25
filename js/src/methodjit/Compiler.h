






































#if !defined jsjaeger_compiler_h__ && defined JS_METHODJIT
#define jsjaeger_compiler_h__

#include "jscntxt.h"
#include "jstl.h"
#include "BytecodeAnalyzer.h"
#include "MethodJIT.h"
#include "assembler/assembler/MacroAssembler.h"
#include "FrameState.h"
#include "CodeGenerator.h"
#include "CompilerBase.h"

namespace js {
namespace mjit {

class Compiler : public CompilerBase
{
    typedef JSC::MacroAssembler::Label Label;
    typedef JSC::MacroAssembler::ImmPtr ImmPtr;
    typedef JSC::MacroAssembler::RegisterID RegisterID;
    typedef JSC::MacroAssembler::Address Address;
    typedef JSC::MacroAssembler::Jump Jump;
    typedef JSC::MacroAssembler MacroAssembler;

    struct BranchPatch {
        BranchPatch(const Jump &j, jsbytecode *pc)
          : jump(j), pc(pc)
        { }

        Jump jump;
        jsbytecode *pc;
    };

    JSContext *cx;
    JSScript *script;
    JSObject *scopeChain;
    JSObject *globalObj;
    JSFunction *fun;
    BytecodeAnalyzer analysis;
    Label *jumpMap;
    jsbytecode *PC;
    MacroAssembler masm;
    FrameState frame;
    CodeGenerator cg;
    js::Vector<BranchPatch, 64> branchPatches;

  public:
    
    
    enum { LengthAtomIndex = uint32(-2) };

    Compiler(JSContext *cx, JSScript *script, JSFunction *fun, JSObject *scopeChain);
    ~Compiler();

    CompileStatus Compile();

  private:
    CompileStatus generatePrologue();
    CompileStatus generateMethod();
    CompileStatus generateEpilogue();
    CompileStatus finishThisUp();

    
    const Label &labelOf(jsbytecode *pc);
    uint32 fullAtomIndex(jsbytecode *pc);
    void jumpInScript(Jump j, jsbytecode *pc);

    
    void jsop_bindname(uint32 index);
    void jsop_setglobal(uint32 index);
    void jsop_getglobal(uint32 index);
    void emitReturn();
};

} 
} 

#endif
