







































#ifndef jsion_compileinfo_h__
#define jsion_compileinfo_h__

namespace js {
namespace ion {

inline uintN
CountArgSlots(JSFunction *fun)
{
    return fun ? fun->nargs + 2 : 1; 
}


class CompileInfo
{
  public:
    CompileInfo(JSScript *script, JSFunction *fun, jsbytecode *osrPc)
      : script_(script), fun_(fun), osrPc_(osrPc)
    {
        JS_ASSERT_IF(osrPc, JSOp(*osrPc) == JSOP_LOOPENTRY);
        nslots_ = script->nslots + CountArgSlots(fun);
    }

    JSScript *script() const {
        return script_;
    }
    JSFunction *fun() const {
        return fun_;
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
        return script_->filename;
    }
    uintN lineno() const {
        return script_->lineno;
    }
    uintN lineno(JSContext *cx, jsbytecode *pc) const {
        return js_PCToLineNumber(cx, script_, pc);
    }

    

    JSAtom *getAtom(jsbytecode *pc) const {
        return script_->getAtom(GET_UINT32_INDEX(pc));
    }
    RegExpObject *getRegExp(jsbytecode *pc) const {
        return script_->getRegExp(GET_UINT32_INDEX(pc));
    }
    JSObject *getObject(jsbytecode *pc) const {
        return script_->getObject(GET_UINT32_INDEX(pc));
    }
    JSFunction *getFunction(jsbytecode *pc) const {
        return script_->getFunction(GET_UINT32_INDEX(pc));
    }
    const Value &getConst(jsbytecode *pc) const {
        return script_->getConst(GET_UINT32_INDEX(pc));
    }
    jssrcnote *getNote(JSContext *cx, jsbytecode *pc) const {
        return js_GetSrcNoteCached(cx, script(), pc);
    }

    
    uintN nslots() const {
        return nslots_;
    }

    uintN nargs() const {
        return fun()->nargs;
    }
    uintN nlocals() const {
        return script()->nfixed;
    }
    uintN ninvoke() const {
        return nlocals() + CountArgSlots(fun());
    }

    uint32 scopeChainSlot() const {
        return 0;
    }
    uint32 thisSlot() const {
        JS_ASSERT(fun());
        return 1;
    }
    uint32 firstArgSlot() const {
        JS_ASSERT(fun());
        return 2;
    }
    uint32 argSlot(uint32 i) const {
        return firstArgSlot() + i;
    }
    uint32 firstLocalSlot() const {
        return CountArgSlots(fun());
    }
    uint32 localSlot(uint32 i) const {
        return firstLocalSlot() + i;
    }
    uint32 firstStackSlot() const {
        return firstLocalSlot() + nlocals();
    }
    uint32 stackSlot(uint32 i) const {
        return firstStackSlot() + i;
    }

  private:
    JSScript *script_;
    JSFunction *fun_;
    uintN nslots_;
    jsbytecode *osrPc_;
};

} 
} 

#endif
