






#ifndef TreeContext_h__
#define TreeContext_h__

#include "jstypes.h"
#include "jsatom.h"
#include "jsopcode.h"
#include "jsscript.h"
#include "jsprvtd.h"
#include "jspubtd.h"

#include "frontend/ParseMaps.h"

#include "vm/ScopeObject.h"

typedef struct BindData BindData;

namespace js {

class ContextFlags {

    
    friend struct SharedContext;
    friend struct FunctionBox;

    
    
    
    
    
    bool            inStrictMode:1;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    bool            bindingsAccessedDynamically:1;

    
    bool            funIsHeavyweight:1;

    
    bool            funIsGenerator:1;

    
    
    bool            funMightAliasLocals:1;

    
    
    
    
    
    
    
    bool            funHasExtensibleScope:1;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    bool            funArgumentsHasLocalBinding:1;

    
    
    
    
    
    
    
    
    
    bool            funDefinitelyNeedsArgsObj:1;

  public:
    ContextFlags(JSContext *cx)
      : inStrictMode(cx->hasRunOption(JSOPTION_STRICT_MODE)),
        bindingsAccessedDynamically(false),
        funIsHeavyweight(false),
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
    Bindings        bindings;       

    Bindings::AutoRooter bindingsRoot; 

    ContextFlags    cxFlags;

    
    
    inline SharedContext(JSContext *cx, JSObject *scopeChain, JSFunction *fun, FunctionBox *funbox);

    
    
    
    
#define INFUNC JS_ASSERT(inFunction())

    bool inStrictMode()                const {         return cxFlags.inStrictMode; }
    bool bindingsAccessedDynamically() const {         return cxFlags.bindingsAccessedDynamically; }
    bool funIsHeavyweight()            const { INFUNC; return cxFlags.funIsHeavyweight; }
    bool funIsGenerator()              const { INFUNC; return cxFlags.funIsGenerator; }
    bool funMightAliasLocals()         const {         return cxFlags.funMightAliasLocals; }
    bool funHasExtensibleScope()       const {         return cxFlags.funHasExtensibleScope; }
    bool funArgumentsHasLocalBinding() const { INFUNC; return cxFlags.funArgumentsHasLocalBinding; }
    bool funDefinitelyNeedsArgsObj()   const { INFUNC; return cxFlags.funDefinitelyNeedsArgsObj; }

    void setInStrictMode()                  {         cxFlags.inStrictMode                = true; }
    void setBindingsAccessedDynamically()   {         cxFlags.bindingsAccessedDynamically = true; }
    void setFunIsHeavyweight()              {         cxFlags.funIsHeavyweight            = true; }
    void setFunIsGenerator()                { INFUNC; cxFlags.funIsGenerator              = true; }
    void setFunMightAliasLocals()           {         cxFlags.funMightAliasLocals         = true; }
    void setFunHasExtensibleScope()         {         cxFlags.funHasExtensibleScope       = true; }
    void setFunArgumentsHasLocalBinding()   { INFUNC; cxFlags.funArgumentsHasLocalBinding = true; }
    void setFunDefinitelyNeedsArgsObj()     { JS_ASSERT(cxFlags.funArgumentsHasLocalBinding);
                                              INFUNC; cxFlags.funDefinitelyNeedsArgsObj   = true; }

#undef INFUNC

    bool inFunction() const { return !!fun_; }

    JSFunction *fun()      const { JS_ASSERT(inFunction());  return fun_; }
    FunctionBox *funbox()  const { JS_ASSERT(inFunction());  return funbox_; }
    JSObject *scopeChain() const { JS_ASSERT(!inFunction()); return scopeChain_; }

    
    
    inline bool needStrictChecks();
};

typedef HashSet<JSAtom *> FuncStmtSet;
struct Parser;
struct StmtInfoTC;

struct TreeContext {                

    typedef StmtInfoTC StmtInfo;

    SharedContext   *sc;            

    uint32_t        bodyid;         
    uint32_t        blockidGen;     

    StmtInfoTC      *topStmt;       
    StmtInfoTC      *topScopeStmt;  
    Rooted<StaticBlockObject *> blockChain;
                                    

    const unsigned  staticLevel;    

    uint32_t        parenDepth;     

    uint32_t        yieldCount;     

    ParseNode       *blockNode;     

    AtomDecls       decls;          
    ParseNode       *yieldNode;     


    FunctionBox     *functionList;

  private:
    TreeContext     **parserTC;     



  public:
    OwnedAtomDefnMapPtr lexdeps;    

    TreeContext     *parent;        

    ParseNode       *innermostWith; 

    FuncStmtSet     *funcStmts;     



    



    bool            hasReturnExpr:1; 
    bool            hasReturnVoid:1; 

    bool            inForInit:1;    

    
    
    
    
    
    
    
    
    
    bool            inDeclDestructuring:1;

    void trace(JSTracer *trc);

    inline TreeContext(Parser *prs, SharedContext *sc, unsigned staticLevel, uint32_t bodyid);
    inline ~TreeContext();

    inline bool init();

    unsigned blockid();

    
    
    
    
    
    
    
    bool atBodyLevel();
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

struct StmtInfoTC : public StmtInfoBase {
    StmtInfoTC      *down;          
    StmtInfoTC      *downScope;     

    uint32_t        blockid;        

    
    bool            isFunctionBodyBlock;

    StmtInfoTC(JSContext *cx) : StmtInfoBase(cx), isFunctionBodyBlock(false) {}
};

namespace frontend {

bool
GenerateBlockId(TreeContext *tc, uint32_t &blockid);


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
LexicalLookup(ContextT *ct, JSAtom *atom, int *slotp, typename ContextT::StmtInfo *stmt);

} 

} 

#endif 
