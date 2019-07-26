





#ifndef jit_CompileInfo_h
#define jit_CompileInfo_h

#include "jsfun.h"

#include "jit/Registers.h"
#include "vm/ScopeObject.h"

namespace js {
namespace jit {

inline unsigned
StartArgSlot(JSScript *script)
{
    
    
    

    
    

    
    return 2 + (script->argumentsHasVarBinding() ? 1 : 0);
}

inline unsigned
CountArgSlots(JSScript *script, JSFunction *fun)
{
    
    
    
    

    
    return StartArgSlot(script) + (fun ? fun->nargs() + 1 : 0);
}


class CompileInfo
{
  public:
    CompileInfo(JSScript *script, JSFunction *fun, jsbytecode *osrPc, bool constructing,
                ExecutionMode executionMode, bool scriptNeedsArgsObj)
      : script_(script), fun_(fun), osrPc_(osrPc), constructing_(constructing),
        executionMode_(executionMode), scriptNeedsArgsObj_(scriptNeedsArgsObj)
    {
        JS_ASSERT_IF(osrPc, JSOp(*osrPc) == JSOP_LOOPENTRY);

        
        
        
        
        if (fun_) {
            fun_ = fun_->nonLazyScript()->functionNonDelazifying();
            JS_ASSERT(fun_->isTenured());
        }

        osrStaticScope_ = osrPc ? script->getStaticScope(osrPc) : nullptr;

        nimplicit_ = StartArgSlot(script)                   
                   + (fun ? 1 : 0);                         
        nargs_ = fun ? fun->nargs() : 0;
        nfixedvars_ = script->nfixedvars();
        nlocals_ = script->nfixed();
        nstack_ = script->nslots() - script->nfixed();
        nslots_ = nimplicit_ + nargs_ + nlocals_ + nstack_;
    }

    CompileInfo(unsigned nlocals, ExecutionMode executionMode)
      : script_(nullptr), fun_(nullptr), osrPc_(nullptr), osrStaticScope_(nullptr),
        constructing_(false), executionMode_(executionMode), scriptNeedsArgsObj_(false)
    {
        nimplicit_ = 0;
        nargs_ = 0;
        nfixedvars_ = 0;
        nlocals_ = nlocals;
        nstack_ = 1;  
        nslots_ = nlocals_ + nstack_;
    }

    JSScript *script() const {
        return script_;
    }
    JSFunction *funMaybeLazy() const {
        return fun_;
    }
    bool constructing() const {
        return constructing_;
    }
    jsbytecode *osrPc() {
        return osrPc_;
    }
    NestedScopeObject *osrStaticScope() const {
        return osrStaticScope_;
    }

    bool hasOsrAt(jsbytecode *pc) {
        JS_ASSERT(JSOp(*pc) == JSOP_LOOPENTRY);
        return pc == osrPc();
    }

    jsbytecode *startPC() const {
        return script_->code();
    }
    jsbytecode *limitPC() const {
        return script_->codeEnd();
    }

    const char *filename() const {
        return script_->filename();
    }

    unsigned lineno() const {
        return script_->lineno();
    }
    unsigned lineno(jsbytecode *pc) const {
        return PCToLineNumber(script_, pc);
    }

    

    JSAtom *getAtom(jsbytecode *pc) const {
        return script_->getAtom(GET_UINT32_INDEX(pc));
    }

    PropertyName *getName(jsbytecode *pc) const {
        return script_->getName(GET_UINT32_INDEX(pc));
    }

    inline RegExpObject *getRegExp(jsbytecode *pc) const;

    JSObject *getObject(jsbytecode *pc) const {
        return script_->getObject(GET_UINT32_INDEX(pc));
    }

    inline JSFunction *getFunction(jsbytecode *pc) const;

    const Value &getConst(jsbytecode *pc) const {
        return script_->getConst(GET_UINT32_INDEX(pc));
    }

    jssrcnote *getNote(GSNCache &gsn, jsbytecode *pc) const {
        return GetSrcNote(gsn, script(), pc);
    }

    
    unsigned nslots() const {
        return nslots_;
    }

    
    
    unsigned nimplicit() const {
        return nimplicit_;
    }
    
    unsigned nargs() const {
        return nargs_;
    }
    
    
    unsigned nfixedvars() const {
        return nfixedvars_;
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
    uint32_t returnValueSlot() const {
        JS_ASSERT(script());
        return 1;
    }
    uint32_t argsObjSlot() const {
        JS_ASSERT(hasArguments());
        return 2;
    }
    uint32_t thisSlot() const {
        JS_ASSERT(funMaybeLazy());
        JS_ASSERT(nimplicit_ > 0);
        return nimplicit_ - 1;
    }
    uint32_t firstArgSlot() const {
        return nimplicit_;
    }
    uint32_t argSlotUnchecked(uint32_t i) const {
        
        
        JS_ASSERT(i < nargs_);
        return nimplicit_ + i;
    }
    uint32_t argSlot(uint32_t i) const {
        
        
        
        JS_ASSERT(!argsObjAliasesFormals());
        return argSlotUnchecked(i);
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

    uint32_t startArgSlot() const {
        JS_ASSERT(script());
        return StartArgSlot(script());
    }
    uint32_t endArgSlot() const {
        JS_ASSERT(script());
        return CountArgSlots(script(), funMaybeLazy());
    }

    uint32_t totalSlots() const {
        JS_ASSERT(script() && funMaybeLazy());
        return nimplicit() + nargs() + nlocals();
    }

    bool isSlotAliased(uint32_t index, NestedScopeObject *staticScope) const {
        JS_ASSERT(index >= startArgSlot());

        if (funMaybeLazy() && index == thisSlot())
            return false;

        uint32_t arg = index - firstArgSlot();
        if (arg < nargs())
            return script()->formalIsAliased(arg);

        uint32_t local = index - firstLocalSlot();
        if (local < nlocals()) {
            
            if (local < nfixedvars())
                return script()->varIsAliased(local);

            
            for (; staticScope; staticScope = staticScope->enclosingNestedScope()) {
                if (!staticScope->is<StaticBlockObject>())
                    continue;
                StaticBlockObject &blockObj = staticScope->as<StaticBlockObject>();
                if (blockObj.localOffset() < local) {
                    if (local - blockObj.localOffset() < blockObj.slotCount())
                        return blockObj.isAliased(local - blockObj.localOffset());
                    return false;
                }
            }

            
            return false;
        }

        JS_ASSERT(index >= firstStackSlot());
        return false;
    }

    bool isSlotAliasedAtEntry(uint32_t index) const {
        return isSlotAliased(index, nullptr);
    }
    bool isSlotAliasedAtOsr(uint32_t index) const {
        return isSlotAliased(index, osrStaticScope());
    }

    bool hasArguments() const {
        return script()->argumentsHasVarBinding();
    }
    bool argumentsAliasesFormals() const {
        return script()->argumentsAliasesFormals();
    }
    bool needsArgsObj() const {
        return scriptNeedsArgsObj_;
    }
    bool argsObjAliasesFormals() const {
        return scriptNeedsArgsObj_ && !script()->strict();
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
    unsigned nfixedvars_;
    unsigned nlocals_;
    unsigned nstack_;
    unsigned nslots_;
    JSScript *script_;
    JSFunction *fun_;
    jsbytecode *osrPc_;
    NestedScopeObject *osrStaticScope_;
    bool constructing_;
    ExecutionMode executionMode_;

    
    
    
    bool scriptNeedsArgsObj_;
};

} 
} 

#endif 
