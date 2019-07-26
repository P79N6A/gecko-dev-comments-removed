






#ifndef jsion_compileinfo_h__
#define jsion_compileinfo_h__

#include "Registers.h"

namespace js {
namespace ion {

inline unsigned
CountArgSlots(JSFunction *fun)
{
    return fun ? fun->nargs + 2 : 1; 
}

enum ExecutionMode {
    
    SequentialExecution = 0,

    
    
    ParallelExecution
};


class CompileInfo
{
  public:
    CompileInfo(RawScript script, JSFunction *fun, jsbytecode *osrPc, bool constructing,
                ExecutionMode executionMode)
      : script_(script), fun_(fun), osrPc_(osrPc), constructing_(constructing),
        executionMode_(executionMode)
    {
        JS_ASSERT_IF(osrPc, JSOp(*osrPc) == JSOP_LOOPENTRY);
        nimplicit_ = 1  + (fun ? 1 : 0);
        nargs_ = fun ? fun->nargs : 0;
        nlocals_ = script->nfixed;
        nstack_ = script->nslots - script->nfixed;
        nslots_ = nimplicit_ + nargs_ + nlocals_ + nstack_;
    }

    CompileInfo(unsigned nlocals)
      : script_(NULL), fun_(NULL), osrPc_(NULL), constructing_(false)
    {
        nimplicit_ = 0;
        nargs_ = 0;
        nlocals_ = nlocals;
        nstack_ = 1;  
        nslots_ = nlocals_ + nstack_;
    }

    RawScript script() const {
        return script_;
    }
    JSFunction *fun() const {
        return fun_;
    }
    bool constructing() const {
        return constructing_;
    }
    jsbytecode *osrPc() {
        return osrPc_;
    }

    bool hasOsrAt(jsbytecode *pc) {
        JS_ASSERT(JSOp(*pc) == JSOP_LOOPENTRY);
        return pc == osrPc();
    }

    jsbytecode *startPC() const {
        return script_->code;
    }
    jsbytecode *limitPC() const {
        return script_->code + script_->length;
    }

    const char *filename() const {
        return script_->filename();
    }
    unsigned lineno() const {
        return script_->lineno;
    }
    unsigned lineno(JSContext *cx, jsbytecode *pc) const {
        return PCToLineNumber(script_, pc);
    }

    

    inline JSAtom *getAtom(jsbytecode *pc) const;
    inline PropertyName *getName(jsbytecode *pc) const;
    inline RegExpObject *getRegExp(jsbytecode *pc) const;
    inline JSObject *getObject(jsbytecode *pc) const;
    inline JSFunction *getFunction(jsbytecode *pc) const;
    inline const Value &getConst(jsbytecode *pc) const;
    inline jssrcnote *getNote(JSContext *cx, jsbytecode *pc) const;

    
    unsigned nslots() const {
        return nslots_;
    }

    unsigned nargs() const {
        return nargs_;
    }
    unsigned nlocals() const {
        return nlocals_;
    }
    unsigned ninvoke() const {
        return nslots_ - nstack_;
    }

    uint32_t scopeChainSlot() const {
        JS_ASSERT(script());
        return 0;
    }
    uint32_t thisSlot() const {
        JS_ASSERT(fun());
        return 1;
    }
    uint32_t firstArgSlot() const {
        return nimplicit_;
    }
    uint32_t argSlot(uint32_t i) const {
        JS_ASSERT(i < nargs_);
        return nimplicit_ + i;
    }
    uint32_t firstLocalSlot() const {
        return nimplicit_ + nargs_;
    }
    uint32_t localSlot(uint32_t i) const {
        return firstLocalSlot() + i;
    }
    uint32_t firstStackSlot() const {
        return firstLocalSlot() + nlocals();
    }
    uint32_t stackSlot(uint32_t i) const {
        return firstStackSlot() + i;
    }

    bool hasArguments() {
        return script()->argumentsHasVarBinding();
    }

    ExecutionMode executionMode() const {
        return executionMode_;
    }

    bool isParallelExecution() const {
        return executionMode_ == ParallelExecution;
    }

  private:
    unsigned nimplicit_;
    unsigned nargs_;
    unsigned nlocals_;
    unsigned nstack_;
    unsigned nslots_;
    JSScript *script_;
    JSFunction *fun_;
    jsbytecode *osrPc_;
    bool constructing_;
    ExecutionMode executionMode_;
};

} 
} 

#endif
