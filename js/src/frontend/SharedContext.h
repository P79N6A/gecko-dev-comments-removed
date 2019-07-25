






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

class ContextFlags {

    
    friend struct SharedContext;
    friend struct FunctionBox;

    
    bool            hasExplicitUseStrict:1;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    bool            bindingsAccessedDynamically:1;

    
    bool            funIsGenerator:1;

    
    
    bool            funMightAliasLocals:1;

    
    
    
    
    
    
    
    bool            funHasExtensibleScope:1;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    bool            funArgumentsHasLocalBinding:1;

    
    
    
    
    
    
    
    
    
    bool            funDefinitelyNeedsArgsObj:1;

  public:
    ContextFlags(JSContext *cx)
     :  hasExplicitUseStrict(false),
        bindingsAccessedDynamically(false),
        funIsGenerator(false),
        funMightAliasLocals(false),
        funHasExtensibleScope(false),
        funArgumentsHasLocalBinding(false),
        funDefinitelyNeedsArgsObj(false)
    { }
};







struct SharedContext {
    JSContext       *const context;

  private:
    const RootedFunction fun_;      

    FunctionBox *const funbox_;     



    const RootedObject scopeChain_; 

  public:
    ContextFlags    cxFlags;


    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    StrictMode::StrictModeState strictModeState;

    
    
    inline SharedContext(JSContext *cx, JSObject *scopeChain, JSFunction *fun, FunctionBox *funbox,
                         StrictMode::StrictModeState sms);

    
    
    
    
#define INFUNC JS_ASSERT(inFunction())

    bool hasExplicitUseStrict()        const {         return cxFlags.hasExplicitUseStrict; }
    bool bindingsAccessedDynamically() const {         return cxFlags.bindingsAccessedDynamically; }
    bool funIsGenerator()              const { INFUNC; return cxFlags.funIsGenerator; }
    bool funMightAliasLocals()         const {         return cxFlags.funMightAliasLocals; }
    bool funHasExtensibleScope()       const {         return cxFlags.funHasExtensibleScope; }
    bool funArgumentsHasLocalBinding() const { INFUNC; return cxFlags.funArgumentsHasLocalBinding; }
    bool funDefinitelyNeedsArgsObj()   const { INFUNC; return cxFlags.funDefinitelyNeedsArgsObj; }

    void setExplicitUseStrict()               {         cxFlags.hasExplicitUseStrict        = true; }
    void setBindingsAccessedDynamically()     {         cxFlags.bindingsAccessedDynamically = true; }
    void setFunIsGenerator()                  { INFUNC; cxFlags.funIsGenerator              = true; }
    void setFunMightAliasLocals()             {         cxFlags.funMightAliasLocals         = true; }
    void setFunHasExtensibleScope()           {         cxFlags.funHasExtensibleScope       = true; }
    void setFunArgumentsHasLocalBinding()     { INFUNC; cxFlags.funArgumentsHasLocalBinding = true; }
    void setFunDefinitelyNeedsArgsObj()       { JS_ASSERT(cxFlags.funArgumentsHasLocalBinding);
                                                INFUNC; cxFlags.funDefinitelyNeedsArgsObj   = true; }

#undef INFUNC

    bool inFunction() const { return !!fun_; }

    JSFunction *fun()      const { JS_ASSERT(inFunction());  return fun_; }
    FunctionBox *funbox()  const { JS_ASSERT(inFunction());  return funbox_; }
    JSObject *scopeChain() const { JS_ASSERT(!inFunction()); return scopeChain_; }

    
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

struct FunctionBox : public ObjectBox
{
    ParseNode       *node;
    FunctionBox     *siblings;
    FunctionBox     *kids;
    FunctionBox     *parent;
    Bindings        bindings;               
    size_t          bufStart;
    size_t          bufEnd;
    uint16_t        level;
    uint16_t        ndefaults;
    StrictMode::StrictModeState strictModeState;
    bool            inLoop:1;               
    bool            inWith:1;               

    bool            inGenexpLambda:1;       

    ContextFlags    cxFlags;

    FunctionBox(ObjectBox* traceListHead, JSObject *obj, ParseNode *fn, ParseContext *pc,
                StrictMode::StrictModeState sms);

    bool funIsGenerator()        const { return cxFlags.funIsGenerator; }
    bool funHasExtensibleScope() const { return cxFlags.funHasExtensibleScope; }

    JSFunction *function() const { return (JSFunction *) object; }

    



    bool inAnyDynamicScope() const;

    void recursivelySetStrictMode(StrictMode::StrictModeState strictness);
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
