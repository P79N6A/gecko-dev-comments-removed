





#ifndef jit_CompileInfo_inl_h
#define jit_CompileInfo_inl_h

#include "jit/CompileInfo.h"
#include "jit/IonAllocPolicy.h"

#include "jsscriptinlines.h"

namespace js {
namespace jit {

inline RegExpObject *
CompileInfo::getRegExp(jsbytecode *pc) const
{
    return script_->getRegExp(pc);
}

inline JSFunction *
CompileInfo::getFunction(jsbytecode *pc) const
{
    return script_->getFunction(GET_UINT32_INDEX(pc));
}

InlineScriptTree *
InlineScriptTree::New(TempAllocator *allocator, InlineScriptTree *callerTree,
                      jsbytecode *callerPc, JSScript *script)
{
    JS_ASSERT_IF(!callerTree, !callerPc);
    JS_ASSERT_IF(callerTree, callerTree->script()->containsPC(callerPc));

    
    void *treeMem = allocator->allocate(sizeof(InlineScriptTree));
    if (!treeMem)
        return nullptr;

    
    return new (treeMem) InlineScriptTree(callerTree, callerPc, script);
}

InlineScriptTree *
InlineScriptTree::addCallee(TempAllocator *allocator, jsbytecode *callerPc,
                            JSScript *calleeScript)
{
    JS_ASSERT(script_ && script_->containsPC(callerPc));
    InlineScriptTree *calleeTree = New(allocator, this, callerPc, calleeScript);
    if (!calleeTree)
        return nullptr;

    calleeTree->nextCallee_ = children_;
    children_ = calleeTree;
    return calleeTree;
}

} 
} 

#endif 
