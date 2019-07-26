






#ifndef SharedContext_h__
#define SharedContext_h__

#include "jstypes.h"
#include "jsatom.h"
#include "jsopcode.h"
#include "jsscript.h"
#include "jsprvtd.h"
#include "jspubtd.h"

#include "builtin/Module.h"
#include "frontend/ParseMaps.h"
#include "frontend/ParseNode.h"
#include "vm/ScopeObject.h"

namespace js {
namespace frontend {


class AnyContextFlags
{
    
    friend class SharedContext;

    
    bool            hasExplicitUseStrict:1;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    bool            bindingsAccessedDynamically:1;

  public:
    AnyContextFlags()
     :  hasExplicitUseStrict(false),
        bindingsAccessedDynamically(false)
    { }
};

class FunctionContextFlags
{
    
    friend class FunctionBox;

    
    bool            isGenerator:1;

    
    
    bool            mightAliasLocals:1;

    
    
    
    
    
    
    
    bool            hasExtensibleScope:1;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    bool            argumentsHasLocalBinding:1;

    
    
    
    
    
    
    
    
    
    bool            definitelyNeedsArgsObj:1;

  public:
    FunctionContextFlags()
     :  isGenerator(false),
        mightAliasLocals(false),
        hasExtensibleScope(false),
        argumentsHasLocalBinding(false),
        definitelyNeedsArgsObj(false)
    { }
};

class GlobalSharedContext;







class SharedContext
{
  public:
    JSContext *const context;
    AnyContextFlags anyCxFlags;
    bool strict;

    
    
    inline SharedContext(JSContext *cx, bool strict);

    virtual ObjectBox *toObjectBox() = 0;
    inline bool isGlobalSharedContext() { return toObjectBox() == NULL; }
    inline bool isModuleBox() { return toObjectBox() && toObjectBox()->isModuleBox(); }
    inline bool isFunctionBox() { return toObjectBox() && toObjectBox()->isFunctionBox(); }
    inline GlobalSharedContext *asGlobalSharedContext();
    inline ModuleBox *asModuleBox();
    inline FunctionBox *asFunctionBox();

    bool hasExplicitUseStrict()        const { return anyCxFlags.hasExplicitUseStrict; }
    bool bindingsAccessedDynamically() const { return anyCxFlags.bindingsAccessedDynamically; }

    void setExplicitUseStrict()           { anyCxFlags.hasExplicitUseStrict        = true; }
    void setBindingsAccessedDynamically() { anyCxFlags.bindingsAccessedDynamically = true; }

    
    inline bool needStrictChecks();
};

class GlobalSharedContext : public SharedContext
{
  private:
    const RootedObject scopeChain_; 

  public:
    inline GlobalSharedContext(JSContext *cx, JSObject *scopeChain, bool strict);

    ObjectBox *toObjectBox() { return NULL; }
    JSObject *scopeChain() const { return scopeChain_; }
};


class ModuleBox : public ObjectBox, public SharedContext {
public:
    Bindings bindings;

    ModuleBox(JSContext *cx, ObjectBox *traceListHead, Module *module, ParseContext *pc);
    ObjectBox *toObjectBox() { return this; }
    Module *module() const { return &object->asModule(); }
};

class FunctionBox : public ObjectBox, public SharedContext
{
  public:
    Bindings        bindings;               
    size_t          bufStart;
    size_t          bufEnd;
    uint16_t        ndefaults;
    bool            inWith:1;               
    bool            inGenexpLambda:1;       

    FunctionContextFlags funCxFlags;

    FunctionBox(JSContext *cx, ObjectBox* traceListHead, JSFunction *fun, ParseContext *pc,
                bool strict);

    ObjectBox *toObjectBox() { return this; }
    JSFunction *function() const { return object->toFunction(); }

    bool isGenerator()              const { return funCxFlags.isGenerator; }
    bool mightAliasLocals()         const { return funCxFlags.mightAliasLocals; }
    bool hasExtensibleScope()       const { return funCxFlags.hasExtensibleScope; }
    bool argumentsHasLocalBinding() const { return funCxFlags.argumentsHasLocalBinding; }
    bool definitelyNeedsArgsObj()   const { return funCxFlags.definitelyNeedsArgsObj; }

    void setIsGenerator()                  { funCxFlags.isGenerator              = true; }
    void setMightAliasLocals()             { funCxFlags.mightAliasLocals         = true; }
    void setHasExtensibleScope()           { funCxFlags.hasExtensibleScope       = true; }
    void setArgumentsHasLocalBinding()     { funCxFlags.argumentsHasLocalBinding = true; }
    void setDefinitelyNeedsArgsObj()       { JS_ASSERT(funCxFlags.argumentsHasLocalBinding);
                                             funCxFlags.definitelyNeedsArgsObj   = true; }
};










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

    StmtInfoBase(JSContext *cx)
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
PushStatement(ContextT *ct, typename ContextT::StmtInfo *stmt, StmtType type);

template <class ContextT>
void
FinishPushBlockScope(ContextT *ct, typename ContextT::StmtInfo *stmt, StaticBlockObject &blockObj);




template <class ContextT>
void
FinishPopStatement(ContextT *ct);















template <class ContextT>
typename ContextT::StmtInfo *
LexicalLookup(ContextT *ct, HandleAtom atom, int *slotp, typename ContextT::StmtInfo *stmt);

} 

} 

#endif 
