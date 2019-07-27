





#ifndef jit_CompileInfo_h
#define jit_CompileInfo_h

#include "jsfun.h"

#include "jit/Registers.h"
#include "vm/ScopeObject.h"

namespace js {
namespace jit {

class TempAllocator;

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








class InlineScriptTree {
    
    InlineScriptTree *caller_;

    
    jsbytecode *callerPc_;

    
    JSScript *script_;

    
    InlineScriptTree *children_;
    InlineScriptTree *nextCallee_;

  public:
    InlineScriptTree(InlineScriptTree *caller, jsbytecode *callerPc, JSScript *script)
      : caller_(caller), callerPc_(callerPc), script_(script),
        children_(nullptr), nextCallee_(nullptr)
    {}

    static InlineScriptTree *New(TempAllocator *allocator, InlineScriptTree *caller,
                                 jsbytecode *callerPc, JSScript *script);

    InlineScriptTree *addCallee(TempAllocator *allocator, jsbytecode *callerPc,
                                 JSScript *calleeScript);

    InlineScriptTree *caller() const {
        return caller_;
    }

    bool isOutermostCaller() const {
        return caller_ == nullptr;
    }
    bool hasCaller() const {
        return caller_ != nullptr;
    }
    InlineScriptTree *outermostCaller() {
        if (isOutermostCaller())
            return this;
        return caller_->outermostCaller();
    }

    jsbytecode *callerPc() const {
        return callerPc_;
    }

    JSScript *script() const {
        return script_;
    }

    bool hasChildren() const {
        return children_ != nullptr;
    }
    InlineScriptTree *firstChild() const {
        JS_ASSERT(hasChildren());
        return children_;
    }

    bool hasNextCallee() const {
        return nextCallee_ != nullptr;
    }
    InlineScriptTree *nextCallee() const {
        JS_ASSERT(hasNextCallee());
        return nextCallee_;
    }

    unsigned depth() const {
        if (isOutermostCaller())
            return 1;
        return 1 + caller_->depth();
    }
};

class BytecodeSite {
    
    InlineScriptTree *tree_;

    
    jsbytecode *pc_;

  public:
    BytecodeSite()
      : tree_(nullptr), pc_(nullptr)
    {}

    BytecodeSite(InlineScriptTree *tree, jsbytecode *pc)
      : tree_(tree), pc_(pc)
    {
        JS_ASSERT(tree_ != nullptr);
        JS_ASSERT(pc_ != nullptr);
    }

    bool hasTree() const {
        return tree_ != nullptr;
    }

    InlineScriptTree *tree() const {
        return tree_;
    }

    jsbytecode *pc() const {
        return pc_;
    }

    JSScript *script() const {
        return tree_ ? tree_->script() : nullptr;
    }
};



class CompileInfo
{
  public:
    CompileInfo(JSScript *script, JSFunction *fun, jsbytecode *osrPc, bool constructing,
                ExecutionMode executionMode, bool scriptNeedsArgsObj,
                InlineScriptTree *inlineScriptTree)
      : script_(script), fun_(fun), osrPc_(osrPc), constructing_(constructing),
        executionMode_(executionMode), scriptNeedsArgsObj_(scriptNeedsArgsObj),
        inlineScriptTree_(inlineScriptTree)
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
        nbodyfixed_ = script->nbodyfixed();
        nlocals_ = script->nfixed();
        fixedLexicalBegin_ = script->fixedLexicalBegin();
        nstack_ = script->nslots() - script->nfixed();
        nslots_ = nimplicit_ + nargs_ + nlocals_ + nstack_;
    }

    CompileInfo(unsigned nlocals, ExecutionMode executionMode)
      : script_(nullptr), fun_(nullptr), osrPc_(nullptr), osrStaticScope_(nullptr),
        constructing_(false), executionMode_(executionMode), scriptNeedsArgsObj_(false),
        inlineScriptTree_(nullptr)
    {
        nimplicit_ = 0;
        nargs_ = 0;
        nbodyfixed_ = 0;
        nlocals_ = nlocals;
        nstack_ = 1;  
        nslots_ = nlocals_ + nstack_;
        fixedLexicalBegin_ = nlocals;
    }

    JSScript *script() const {
        return script_;
    }
    bool compilingAsmJS() const {
        return script() == nullptr;
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
    InlineScriptTree *inlineScriptTree() const {
        return inlineScriptTree_;
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
    
    
    unsigned nbodyfixed() const {
        return nbodyfixed_;
    }
    
    
    unsigned nlocals() const {
        return nlocals_;
    }
    unsigned ninvoke() const {
        return nslots_ - nstack_;
    }
    
    unsigned fixedLexicalBegin() const {
        return fixedLexicalBegin_;
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
            
            if (local < nbodyfixed())
                return script()->bodyLevelLocalIsAliased(local);

            
            for (; staticScope; staticScope = staticScope->enclosingNestedScope()) {
                if (!staticScope->is<StaticBlockObject>())
                    continue;
                StaticBlockObject &blockObj = staticScope->as<StaticBlockObject>();
                if (blockObj.localOffset() < local) {
                    if (local - blockObj.localOffset() < blockObj.numVariables())
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

    bool executionModeIsAnalysis() const {
        return executionMode_ == DefinitePropertiesAnalysis || executionMode_ == ArgumentsUsageAnalysis;
    }

    bool isParallelExecution() const {
        return executionMode_ == ParallelExecution;
    }

    
    
    
    
    bool isObservableSlot(uint32_t slot) const {
        if (isObservableFrameSlot(slot))
            return true;

        if (isObservableArgumentSlot(slot))
            return true;

        return false;
    }

    bool isObservableFrameSlot(uint32_t slot) const {
        if (!funMaybeLazy())
            return false;

        
        if (slot == thisSlot())
            return true;

        
        
        
        
        if (hasArguments() && (slot == scopeChainSlot() || slot == argsObjSlot()))
            return true;

        return false;
    }

    bool isObservableArgumentSlot(uint32_t slot) const {
        if (!funMaybeLazy())
            return false;

        
        
        if ((hasArguments() || !script()->strict()) &&
            firstArgSlot() <= slot && slot - firstArgSlot() < nargs())
        {
            return true;
        }

        return false;
    }

    
    
    
    bool isRecoverableOperand(uint32_t slot) const {
        if (isObservableFrameSlot(slot))
            return false;

        if (needsArgsObj() && isObservableArgumentSlot(slot))
            return false;

        return true;
    }

  private:
    unsigned nimplicit_;
    unsigned nargs_;
    unsigned nbodyfixed_;
    unsigned nlocals_;
    unsigned nstack_;
    unsigned nslots_;
    unsigned fixedLexicalBegin_;
    JSScript *script_;
    JSFunction *fun_;
    jsbytecode *osrPc_;
    NestedScopeObject *osrStaticScope_;
    bool constructing_;
    ExecutionMode executionMode_;

    
    
    
    bool scriptNeedsArgsObj_;

    InlineScriptTree *inlineScriptTree_;
};

} 
} 

#endif 
