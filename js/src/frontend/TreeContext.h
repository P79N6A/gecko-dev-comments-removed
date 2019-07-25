







































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

JS_ENUM_HEADER(TreeContextFlags, uint32_t)
{
    
    TCF_IN_FUNCTION =                          0x1,

    
    TCF_RETURN_EXPR =                          0x2,

    
    TCF_RETURN_VOID =                          0x4,

    
    TCF_IN_FOR_INIT =                          0x8,

    
    TCF_FUN_HEAVYWEIGHT =                     0x10,

    
    TCF_FUN_IS_GENERATOR =                    0x20,

    
    TCF_HAS_FUNCTION_STMT =                   0x40,

    
    TCF_GENEXP_LAMBDA =                       0x80,

    
    TCF_COMPILE_N_GO =                       0x100,

    
    TCF_NO_SCRIPT_RVAL =                     0x200,

    
    
    
    
    
    
    
    
    
    
    TCF_DECL_DESTRUCTURING =                 0x400,

    
    
    
    
    
    TCF_STRICT_MODE_CODE =                   0x800,

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    TCF_BINDINGS_ACCESSED_DYNAMICALLY =     0x1000,

    
    TCF_COMPILE_FOR_EVAL =                  0x2000,

    
    
    TCF_FUN_MIGHT_ALIAS_LOCALS =            0x4000,

    
    TCF_HAS_SINGLETONS =                    0x8000,

    
    TCF_IN_WITH =                          0x10000,

    
    
    
    
    
    
    
    TCF_FUN_EXTENSIBLE_SCOPE =             0x20000,

    
    TCF_NEED_SCRIPT_GLOBAL =               0x40000,

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    TCF_ARGUMENTS_HAS_LOCAL_BINDING =      0x80000,

    
    
    
    
    
    
    
    
    
    TCF_DEFINITELY_NEEDS_ARGS_OBJ =       0x100000

} JS_ENUM_FOOTER(TreeContextFlags);


static const uint32_t TCF_RETURN_FLAGS = TCF_RETURN_EXPR | TCF_RETURN_VOID;


static const uint32_t TCF_FUN_FLAGS = TCF_FUN_HEAVYWEIGHT |
                                      TCF_FUN_IS_GENERATOR |
                                      TCF_BINDINGS_ACCESSED_DYNAMICALLY |
                                      TCF_FUN_MIGHT_ALIAS_LOCALS |
                                      TCF_STRICT_MODE_CODE |
                                      TCF_FUN_EXTENSIBLE_SCOPE |
                                      TCF_ARGUMENTS_HAS_LOCAL_BINDING |
                                      TCF_DEFINITELY_NEEDS_ARGS_OBJ;

typedef HashSet<JSAtom *> FuncStmtSet;

struct Parser;
struct StmtInfo;

struct TreeContext {                
    uint32_t        flags;          
    uint32_t        bodyid;         
    uint32_t        blockidGen;     
    uint32_t        parenDepth;     

    uint32_t        yieldCount;     

    StmtInfo        *topStmt;       
    StmtInfo        *topScopeStmt;  
    RootedVar<StaticBlockObject *> blockChain;
                                    


    ParseNode       *blockNode;     

    AtomDecls       decls;          
    Parser          *parser;        
    ParseNode       *yieldNode;     


    ParseNode       *argumentsNode; 



  private:
    RootedVarFunction fun_;         

    RootedVarObject   scopeChain_;  

  public:
    JSFunction *fun() const {
        JS_ASSERT(inFunction());
        return fun_;
    }
    void setFunction(JSFunction *fun) {
        JS_ASSERT(inFunction());
        fun_ = fun;
    }
    JSObject *scopeChain() const {
        JS_ASSERT(!inFunction());
        return scopeChain_;
    }
    void setScopeChain(JSObject *scopeChain) {
        JS_ASSERT(!inFunction());
        scopeChain_ = scopeChain;
    }

    OwnedAtomDefnMapPtr lexdeps;    

    TreeContext     *parent;        

    unsigned        staticLevel;    

    FunctionBox     *funbox;        


    FunctionBox     *functionList;

    ParseNode       *innermostWith; 

    Bindings        bindings;       

    Bindings::StackRoot bindingsRoot; 

    FuncStmtSet *funcStmts;         



    void trace(JSTracer *trc);

    inline TreeContext(Parser *prs);
    inline ~TreeContext();

    
    
    
    
    enum InitBehavior {
        USED_AS_TREE_CONTEXT,
        USED_AS_CODE_GENERATOR
    };

    bool init(JSContext *cx, InitBehavior ib = USED_AS_TREE_CONTEXT) {
        if (cx->hasRunOption(JSOPTION_STRICT_MODE))
            flags |= TCF_STRICT_MODE_CODE;
        if (ib == USED_AS_CODE_GENERATOR)
            return true;
        return decls.init() && lexdeps.ensureMap(cx);
    }

    unsigned blockid();

    
    
    
    
    
    
    
    bool atBodyLevel();

    bool inStrictMode() const {
        return flags & TCF_STRICT_MODE_CODE;
    }

    
    
    inline bool needStrictChecks();

    bool compileAndGo() const { return flags & TCF_COMPILE_N_GO; }
    bool inFunction() const { return flags & TCF_IN_FUNCTION; }

    void noteBindingsAccessedDynamically() {
        flags |= TCF_BINDINGS_ACCESSED_DYNAMICALLY;
    }

    bool bindingsAccessedDynamically() const {
        return flags & TCF_BINDINGS_ACCESSED_DYNAMICALLY;
    }

    void noteMightAliasLocals() {
        flags |= TCF_FUN_MIGHT_ALIAS_LOCALS;
    }

    bool mightAliasLocals() const {
        return flags & TCF_FUN_MIGHT_ALIAS_LOCALS;
    }

    void noteArgumentsHasLocalBinding() {
        flags |= TCF_ARGUMENTS_HAS_LOCAL_BINDING;
    }

    bool argumentsHasLocalBinding() const {
        return flags & TCF_ARGUMENTS_HAS_LOCAL_BINDING;
    }

    unsigned argumentsLocalSlot() const;

    void noteDefinitelyNeedsArgsObj() {
        JS_ASSERT(argumentsHasLocalBinding());
        flags |= TCF_DEFINITELY_NEEDS_ARGS_OBJ;
    }

    bool definitelyNeedsArgsObj() const {
        return flags & TCF_DEFINITELY_NEEDS_ARGS_OBJ;
    }

    void noteHasExtensibleScope() {
        flags |= TCF_FUN_EXTENSIBLE_SCOPE;
    }

    bool hasExtensibleScope() const {
        return flags & TCF_FUN_EXTENSIBLE_SCOPE;
    }

    ParseNode *freeTree(ParseNode *pn);
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
    RootedVarAtom   label;          
    RootedVar<StaticBlockObject *> blockObj; 
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
SetStaticLevel(TreeContext *tc, unsigned staticLevel);

bool
GenerateBlockId(TreeContext *tc, uint32_t &blockid);




void
PushStatement(TreeContext *tc, StmtInfo *stmt, StmtType type, ptrdiff_t top);






void
PushBlockScope(TreeContext *tc, StmtInfo *stmt, StaticBlockObject &blockObj, ptrdiff_t top);





void
PopStatementTC(TreeContext *tc);















StmtInfo *
LexicalLookup(TreeContext *tc, JSAtom *atom, int *slotp, StmtInfo *stmt = NULL);

} 

} 

#endif 
