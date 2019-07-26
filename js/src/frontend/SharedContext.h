





#ifndef frontend_SharedContext_h
#define frontend_SharedContext_h

#include "jstypes.h"
#include "jsatom.h"
#include "jsopcode.h"
#include "jsscript.h"
#include "jsprvtd.h"
#include "jspubtd.h"

#include "builtin/Module.h"
#include "frontend/ParseMaps.h"
#include "frontend/ParseNode.h"
#include "frontend/TokenStream.h"
#include "vm/ScopeObject.h"

namespace js {
namespace frontend {


class AnyContextFlags
{
    
    friend class SharedContext;

    
    bool            hasExplicitUseStrict:1;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    bool            bindingsAccessedDynamically:1;

    
    
    
    bool            hasDebuggerStatement:1;

  public:
    AnyContextFlags()
     :  hasExplicitUseStrict(false),
        bindingsAccessedDynamically(false),
        hasDebuggerStatement(false)
    { }
};

class FunctionContextFlags
{
    
    friend class FunctionBox;

    
    bool isGenerator:1;

    
    
    bool mightAliasLocals:1;

    
    
    
    
    
    
    
    bool hasExtensibleScope:1;

    
    
    bool needsDeclEnvObject:1;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    bool argumentsHasLocalBinding:1;

    
    
    
    
    
    
    
    
    
    bool definitelyNeedsArgsObj:1;

  public:
    FunctionContextFlags()
     :  isGenerator(false),
        mightAliasLocals(false),
        hasExtensibleScope(false),
        needsDeclEnvObject(false),
        argumentsHasLocalBinding(false),
        definitelyNeedsArgsObj(false)
    { }
};

class GlobalSharedContext;







class SharedContext
{
  public:
    ExclusiveContext *const context;
    AnyContextFlags anyCxFlags;
    bool strict;
    bool extraWarnings;

    
    
    SharedContext(ExclusiveContext *cx, bool strict, bool extraWarnings)
      : context(cx),
        anyCxFlags(),
        strict(strict),
        extraWarnings(extraWarnings)
    {}

    virtual ObjectBox *toObjectBox() = 0;
    inline bool isGlobalSharedContext() { return toObjectBox() == NULL; }
    inline bool isModuleBox() { return toObjectBox() && toObjectBox()->isModuleBox(); }
    inline bool isFunctionBox() { return toObjectBox() && toObjectBox()->isFunctionBox(); }
    inline GlobalSharedContext *asGlobalSharedContext();
    inline ModuleBox *asModuleBox();
    inline FunctionBox *asFunctionBox();

    bool hasExplicitUseStrict()        const { return anyCxFlags.hasExplicitUseStrict; }
    bool bindingsAccessedDynamically() const { return anyCxFlags.bindingsAccessedDynamically; }
    bool hasDebuggerStatement()        const { return anyCxFlags.hasDebuggerStatement; }

    void setExplicitUseStrict()           { anyCxFlags.hasExplicitUseStrict        = true; }
    void setBindingsAccessedDynamically() { anyCxFlags.bindingsAccessedDynamically = true; }
    void setHasDebuggerStatement()        { anyCxFlags.hasDebuggerStatement        = true; }

    
    inline bool needStrictChecks();
};

class GlobalSharedContext : public SharedContext
{
  private:
    const RootedObject scopeChain_; 

  public:
    GlobalSharedContext(ExclusiveContext *cx, JSObject *scopeChain,
                        bool strict, bool extraWarnings)
      : SharedContext(cx, strict, extraWarnings),
        scopeChain_(cx, scopeChain)
    {}

    ObjectBox *toObjectBox() { return NULL; }
    JSObject *scopeChain() const { return scopeChain_; }
};

inline GlobalSharedContext *
SharedContext::asGlobalSharedContext()
{
    JS_ASSERT(isGlobalSharedContext());
    return static_cast<GlobalSharedContext*>(this);
}

class ModuleBox : public ObjectBox, public SharedContext
{
  public:
    Bindings bindings;

    ModuleBox(ExclusiveContext *cx, ObjectBox *traceListHead, Module *module,
              ParseContext<FullParseHandler> *pc, bool extraWarnings);
    ObjectBox *toObjectBox() { return this; }
    Module *module() const { return &object->as<Module>(); }
};

inline ModuleBox *
SharedContext::asModuleBox()
{
    JS_ASSERT(isModuleBox());
    return static_cast<ModuleBox*>(this);
}

class FunctionBox : public ObjectBox, public SharedContext
{
  public:
    Bindings        bindings;               
    uint32_t        bufStart;
    uint32_t        bufEnd;
    uint32_t        startLine;
    uint32_t        startColumn;
    uint32_t        asmStart;               
    uint16_t        ndefaults;
    bool            inWith:1;               
    bool            inGenexpLambda:1;       
    bool            useAsm:1;               
    bool            insideUseAsm:1;         

    
    bool            usesArguments:1;  
    bool            usesApply:1;      

    FunctionContextFlags funCxFlags;

    template <typename ParseHandler>
    FunctionBox(ExclusiveContext *cx, ObjectBox* traceListHead, JSFunction *fun,
                ParseContext<ParseHandler> *pc,
                bool strict, bool extraWarnings);

    ObjectBox *toObjectBox() { return this; }
    JSFunction *function() const { return &object->as<JSFunction>(); }

    bool isGenerator()              const { return funCxFlags.isGenerator; }
    bool mightAliasLocals()         const { return funCxFlags.mightAliasLocals; }
    bool hasExtensibleScope()       const { return funCxFlags.hasExtensibleScope; }
    bool needsDeclEnvObject()       const { return funCxFlags.needsDeclEnvObject; }
    bool argumentsHasLocalBinding() const { return funCxFlags.argumentsHasLocalBinding; }
    bool definitelyNeedsArgsObj()   const { return funCxFlags.definitelyNeedsArgsObj; }

    void setIsGenerator()                  { funCxFlags.isGenerator              = true; }
    void setMightAliasLocals()             { funCxFlags.mightAliasLocals         = true; }
    void setHasExtensibleScope()           { funCxFlags.hasExtensibleScope       = true; }
    void setNeedsDeclEnvObject()           { funCxFlags.needsDeclEnvObject       = true; }
    void setArgumentsHasLocalBinding()     { funCxFlags.argumentsHasLocalBinding = true; }
    void setDefinitelyNeedsArgsObj()       { JS_ASSERT(funCxFlags.argumentsHasLocalBinding);
                                             funCxFlags.definitelyNeedsArgsObj   = true; }

    
    
    bool useAsmOrInsideUseAsm() const {
        return useAsm || insideUseAsm;
    }

    void setStart(const TokenStream &tokenStream) {
        bufStart = tokenStream.currentToken().pos.begin;
        startLine = tokenStream.getLineno();
        startColumn = tokenStream.getColumn();
    }

    bool isHeavyweight()
    {
        
        return bindings.hasAnyAliasedBindings() ||
               hasExtensibleScope() ||
               needsDeclEnvObject();
    }
};

inline FunctionBox *
SharedContext::asFunctionBox()
{
    JS_ASSERT(isFunctionBox());
    return static_cast<FunctionBox*>(this);
}










enum StmtType {
    STMT_LABEL,                 
    STMT_IF,                    
    STMT_ELSE,                  
    STMT_SEQ,                   
    STMT_BLOCK,                 
    STMT_SWITCH,                
    STMT_WITH,                  
    STMT_CATCH,                 
    STMT_TRY,                   
    STMT_FINALLY,               
    STMT_SUBROUTINE,            
    STMT_DO_LOOP,               
    STMT_FOR_LOOP,              
    STMT_FOR_IN_LOOP,           
    STMT_WHILE_LOOP,            
    STMT_LIMIT
};



























struct StmtInfoBase {
    uint16_t        type;           

    



    bool isBlockScope:1;

    
    bool isForLetBlock:1;

    RootedAtom      label;          
    Rooted<StaticBlockObject *> blockObj; 

    StmtInfoBase(ExclusiveContext *cx)
        : isBlockScope(false), isForLetBlock(false), label(cx), blockObj(cx)
    {}

    bool maybeScope() const {
        return STMT_BLOCK <= type && type <= STMT_SUBROUTINE && type != STMT_WITH;
    }

    bool linksScope() const {
        return (STMT_WITH <= type && type <= STMT_CATCH) || isBlockScope;
    }

    bool isLoop() const {
        return type >= STMT_DO_LOOP;
    }

    bool isTrying() const {
        return STMT_TRY <= type && type <= STMT_SUBROUTINE;
    }
};


template <class ContextT>
void
PushStatement(ContextT *ct, typename ContextT::StmtInfo *stmt, StmtType type)
{
    stmt->type = type;
    stmt->isBlockScope = false;
    stmt->isForLetBlock = false;
    stmt->label = NULL;
    stmt->blockObj = NULL;
    stmt->down = ct->topStmt;
    ct->topStmt = stmt;
    if (stmt->linksScope()) {
        stmt->downScope = ct->topScopeStmt;
        ct->topScopeStmt = stmt;
    } else {
        stmt->downScope = NULL;
    }
}

template <class ContextT>
void
FinishPushBlockScope(ContextT *ct, typename ContextT::StmtInfo *stmt, StaticBlockObject &blockObj)
{
    stmt->isBlockScope = true;
    stmt->downScope = ct->topScopeStmt;
    ct->topScopeStmt = stmt;
    ct->blockChain = &blockObj;
    stmt->blockObj = &blockObj;
}




template <class ContextT>
void
FinishPopStatement(ContextT *ct)
{
    typename ContextT::StmtInfo *stmt = ct->topStmt;
    ct->topStmt = stmt->down;
    if (stmt->linksScope()) {
        ct->topScopeStmt = stmt->downScope;
        if (stmt->isBlockScope)
            ct->blockChain = stmt->blockObj->enclosingBlock();
    }
}















template <class ContextT>
typename ContextT::StmtInfo *
LexicalLookup(ContextT *ct, HandleAtom atom, int *slotp, typename ContextT::StmtInfo *stmt);

} 

} 

#endif 
