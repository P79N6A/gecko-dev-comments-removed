






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

struct StmtInfo;

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
    JSContext       *context;

    uint32_t        bodyid;         
    uint32_t        blockidGen;     

    StmtInfo        *topStmt;       
    StmtInfo        *topScopeStmt;  
    Rooted<StaticBlockObject *> blockChain;
                                    

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

    unsigned argumentsLocal() const;

    bool inFunction() const { return !!fun_; }

    JSFunction *fun()      const { JS_ASSERT(inFunction());  return fun_; }
    FunctionBox *funbox()  const { JS_ASSERT(inFunction());  return funbox_; }
    JSObject *scopeChain() const { JS_ASSERT(!inFunction()); return scopeChain_; }

    unsigned blockid();

    
    
    
    
    
    
    
    bool atBodyLevel();

    
    
    inline bool needStrictChecks();
};

typedef HashSet<JSAtom *> FuncStmtSet;
struct Parser;

struct TreeContext {                
    SharedContext   *sc;            

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

    inline TreeContext(Parser *prs, SharedContext *sc, unsigned staticLevel);
    inline ~TreeContext();

    inline bool init();
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

inline bool
STMT_TYPE_IN_RANGE(uint16_t type, StmtType begin, StmtType end)
{
    return begin <= type && type <= end;
}




















#define STMT_TYPE_MAYBE_SCOPE(type)                                           \
    (type != STMT_WITH &&                                                     \
     STMT_TYPE_IN_RANGE(type, STMT_BLOCK, STMT_SUBROUTINE))

#define STMT_TYPE_LINKS_SCOPE(type)                                           \
    STMT_TYPE_IN_RANGE(type, STMT_WITH, STMT_CATCH)

#define STMT_TYPE_IS_TRYING(type)                                             \
    STMT_TYPE_IN_RANGE(type, STMT_TRY, STMT_SUBROUTINE)

#define STMT_TYPE_IS_LOOP(type) ((type) >= STMT_DO_LOOP)

#define STMT_MAYBE_SCOPE(stmt)  STMT_TYPE_MAYBE_SCOPE((stmt)->type)
#define STMT_LINKS_SCOPE(stmt)  (STMT_TYPE_LINKS_SCOPE((stmt)->type) ||       \
                                 ((stmt)->flags & SIF_SCOPE))
#define STMT_IS_TRYING(stmt)    STMT_TYPE_IS_TRYING((stmt)->type)
#define STMT_IS_LOOP(stmt)      STMT_TYPE_IS_LOOP((stmt)->type)

struct StmtInfo {
    uint16_t        type;           
    uint16_t        flags;          
    uint32_t        blockid;        
    ptrdiff_t       update;         
    ptrdiff_t       breaks;         
    ptrdiff_t       continues;      
    RootedAtom      label;          
    Rooted<StaticBlockObject *> blockObj; 
    StmtInfo        *down;          
    StmtInfo        *downScope;     

    StmtInfo(JSContext *cx) : label(cx), blockObj(cx) {}
};

#define SIF_SCOPE        0x0001     /* statement has its own lexical scope */
#define SIF_BODY_BLOCK   0x0002     /* STMT_BLOCK type is a function body */
#define SIF_FOR_BLOCK    0x0004     /* for (let ...) induced block scope */

#define SET_STATEMENT_TOP(stmt, top)                                          \
    ((stmt)->update = (top), (stmt)->breaks = (stmt)->continues = (-1))

namespace frontend {

bool
GenerateBlockId(SharedContext *sc, uint32_t &blockid);




void
PushStatement(SharedContext *sc, StmtInfo *stmt, StmtType type, ptrdiff_t top);






void
PushBlockScope(SharedContext *sc, StmtInfo *stmt, StaticBlockObject &blockObj, ptrdiff_t top);





void
PopStatementSC(SharedContext *sc);















StmtInfo *
LexicalLookup(SharedContext *sc, JSAtom *atom, int *slotp, StmtInfo *stmt = NULL);

} 

} 

#endif 
