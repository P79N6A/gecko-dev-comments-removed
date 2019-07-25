






































#if !defined jsjaeger_bytecodeAnalyzer_h__ && defined JS_METHODJIT
#define jsjaeger_bytecodeAnalyzer_h__

#include "jsapi.h"
#include "jscntxt.h"
#include "jsscript.h"
#include "jsopcode.h"

namespace js
{
    struct OpcodeStatus
    {
        bool   visited;         
        bool   exceptionEntry;  
        bool   safePoint;       
        bool   trap;            
        bool   inTryBlock;      
        uint32 nincoming;       
        uint32 stackDepth;      
    };

    class BytecodeAnalyzer
    {
        JSContext    *cx;
        JSScript     *script;
        OpcodeStatus *ops;
        Vector<jsbytecode *, 16, ContextAllocPolicy> doList;

        
        bool usesRval;

      public:
        BytecodeAnalyzer(JSContext *cx, JSScript *script)
          : cx(cx), script(script), ops(NULL),
            doList(ContextAllocPolicy(cx)),
            usesRval(false)
        {
        }
        ~BytecodeAnalyzer();

        bool analyze(uint32 offs);
        bool addEdge(jsbytecode *pc, int32 offset, uint32 stackDepth);

      public:

        bool usesReturnValue() const { return usesRval; }

        inline const OpcodeStatus & operator [](uint32 offs) const {
            JS_ASSERT(offs < script->length);
            return ops[offs];
        }

        inline OpcodeStatus & operator [](uint32 offs) {
            JS_ASSERT(offs < script->length);
            return ops[offs];
        }

        inline const OpcodeStatus & operator [](jsbytecode *pc) const {
            JS_ASSERT(pc < script->code + script->length);
            return ops[pc - script->code];
        }

        inline OpcodeStatus & operator [](jsbytecode *pc) {
            JS_ASSERT(pc < script->code + script->length);
            return ops[pc - script->code];
        }

        inline bool OOM() { return !ops; }

        bool analyze();
    };
}

#endif 

