






#ifndef SharedContext_h__
#define SharedContext_h__

#include "jstypes.h"
#include "jsatom.h"
#include "jsopcode.h"
#include "jsscript.h"
#include "jsprvtd.h"
#include "jspubtd.h"

#include "frontend/ParseMaps.h"
#include "frontend/ParseNode.h"
#include "vm/ScopeObject.h"

namespace js {
namespace frontend {


class AnyContextFlags {

    
    friend struct SharedContext;

    
    bool            hasExplicitUseStrict:1;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    bool            bindingsAccessedDynamically:1;

  public:
    AnyContextFlags()
     :  hasExplicitUseStrict(false),
        bindingsAccessedDynamically(false)
    { }
};

class FunctionContextFlags {

    
    friend struct FunctionBox;

    
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







struct SharedContext {
    JSContext       *const context;

    const bool isFunction;          

  private:
    FunctionBox *const funbox_;     


    const RootedObject scopeChain_; 


  public:
    AnyContextFlags anyCxFlags;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    StrictMode strictModeState;

    
    
    inline SharedContext(JSContext *cx, JSObject *scopeChain, FunctionBox *funbox,
                         StrictMode sms);

    bool hasExplicitUseStrict()        const { return anyCxFlags.hasExplicitUseStrict; }
    bool bindingsAccessedDynamically() const { return anyCxFlags.bindingsAccessedDynamically; }

    void setExplicitUseStrict()           { anyCxFlags.hasExplicitUseStrict        = true; }
    void setBindingsAccessedDynamically() { anyCxFlags.bindingsAccessedDynamically = true; }

    FunctionBox *funbox()  const { JS_ASSERT(isFunction);  return funbox_; }
    JSObject *scopeChain() const { JS_ASSERT(!isFunction); return scopeChain_; }

    
    inline bool needStrictChecks();
    inline bool inStrictMode();
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

struct FunctionBox
{
    ObjectBox       objbox;
    FunctionBox     *siblings;
    FunctionBox     *kids;
    Bindings        bindings;               
    size_t          bufStart;
    size_t          bufEnd;
    uint16_t        ndefaults;
    StrictMode      strictModeState;
    bool            inWith:1;               

    bool            inGenexpLambda:1;       

    AnyContextFlags anyCxFlags;
    FunctionContextFlags funCxFlags;

    FunctionBox(ObjectBox *traceListHead, JSFunction *fun, ParseContext *pc, StrictMode sms);

    JSFunction *fun() const { return objbox.object->toFunction(); }

    void recursivelySetStrictMode(StrictMode strictness);

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
