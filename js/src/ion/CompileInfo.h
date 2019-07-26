






#ifndef jsion_compileinfo_h__
#define jsion_compileinfo_h__

namespace js {
namespace ion {

inline unsigned
CountArgSlots(JSFunction *fun)
{
    return fun ? fun->nargs + 2 : 1; 
}

enum ExecutionMode {
    
    SequentialExecution,

    
    
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
        nslots_ = script->nslots + CountArgSlots(fun);
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
        return fun()->nargs;
    }
    unsigned nlocals() const {
        return script()->nfixed;
    }
    unsigned ninvoke() const {
        return nlocals() + CountArgSlots(fun());
    }

    uint32_t scopeChainSlot() const {
        return 0;
    }
    uint32_t thisSlot() const {
        JS_ASSERT(fun());
        return 1;
    }
    uint32_t firstArgSlot() const {
        JS_ASSERT(fun());
        return 2;
    }
    uint32_t argSlot(uint32_t i) const {
        return firstArgSlot() + i;
    }
    uint32_t firstLocalSlot() const {
        return CountArgSlots(fun());
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
    JSScript *script_;
    JSFunction *fun_;
    unsigned nslots_;
    jsbytecode *osrPc_;
    bool constructing_;
    ExecutionMode executionMode_;
};

} 
} 

#endif
