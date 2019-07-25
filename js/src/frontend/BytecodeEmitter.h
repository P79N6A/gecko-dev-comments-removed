







































#ifndef BytecodeEmitter_h__
#define BytecodeEmitter_h__




#include "jstypes.h"
#include "jsatom.h"
#include "jsopcode.h"
#include "jsscript.h"
#include "jsprvtd.h"
#include "jspubtd.h"

#include "frontend/Parser.h"
#include "frontend/ParseMaps.h"

#include "jsatominlines.h"

namespace js {










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
    union {
        JSAtom      *label;         
        StaticBlockObject *blockObj;
    };
    StmtInfo        *down;          
    StmtInfo        *downScope;     
};

#define SIF_SCOPE        0x0001     /* statement has its own lexical scope */
#define SIF_BODY_BLOCK   0x0002     /* STMT_BLOCK type is a function body */
#define SIF_FOR_BLOCK    0x0004     /* for (let ...) induced block scope */









#define CATCHNOTE(stmt)  ((stmt).update)
#define GOSUBS(stmt)     ((stmt).breaks)
#define GUARDJUMP(stmt)  ((stmt).continues)

#define SET_STATEMENT_TOP(stmt, top)                                          \
    ((stmt)->update = (top), (stmt)->breaks = (stmt)->continues = (-1))

#define TCF_COMPILING           0x01 /* TreeContext is BytecodeEmitter */
#define TCF_IN_FUNCTION         0x02 /* parsing inside function body */
#define TCF_RETURN_EXPR         0x04 /* function has 'return expr;' */
#define TCF_RETURN_VOID         0x08 /* function has 'return;' */
#define TCF_IN_FOR_INIT         0x10 /* parsing init expr of for; exclude 'in' */
#define TCF_FUN_SETS_OUTER_NAME 0x20 /* function set outer name (lexical or free) */
#define TCF_FUN_PARAM_ARGUMENTS 0x40 /* function has parameter named arguments */
#define TCF_FUN_USES_ARGUMENTS  0x80 /* function uses arguments except as a
                                        parameter name */
#define TCF_FUN_HEAVYWEIGHT    0x100 /* function needs Call object per call */
#define TCF_FUN_IS_GENERATOR   0x200 /* parsed yield statement in function */
#define TCF_FUN_USES_OWN_NAME  0x400 /* named function expression that uses its
                                        own name */
#define TCF_HAS_FUNCTION_STMT  0x800 /* block contains a function statement */
#define TCF_GENEXP_LAMBDA     0x1000 /* flag lambda from generator expression */
#define TCF_COMPILE_N_GO      0x2000 /* compile-and-go mode of script, can
                                        optimize name references based on scope
                                        chain */
#define TCF_NO_SCRIPT_RVAL    0x4000 /* API caller does not want result value
                                        from global script */
#define TCF_HAS_SHARPS        0x8000 /* source contains sharp defs or uses */












#define TCF_DECL_DESTRUCTURING  0x10000






#define TCF_STRICT_MODE_CODE    0x20000







#define TCF_FUN_MODULE_PATTERN 0x200000








#define TCF_FUN_ENTRAINS_SCOPES 0x400000


#define TCF_FUN_CALLS_EVAL       0x800000


#define TCF_FUN_MUTATES_PARAMETER 0x1000000




#define TCF_COMPILE_FOR_EVAL     0x2000000





#define TCF_FUN_MIGHT_ALIAS_LOCALS  0x4000000




#define TCF_HAS_SINGLETONS       0x8000000




#define TCF_IN_WITH             0x10000000









#define TCF_FUN_EXTENSIBLE_SCOPE 0x20000000




#define TCF_NEED_SCRIPT_GLOBAL 0x40000000




#define TCF_RETURN_FLAGS        (TCF_RETURN_EXPR | TCF_RETURN_VOID)




#define TCF_FUN_FLAGS           (TCF_FUN_SETS_OUTER_NAME |                    \
                                 TCF_FUN_USES_ARGUMENTS  |                    \
                                 TCF_FUN_PARAM_ARGUMENTS |                    \
                                 TCF_FUN_HEAVYWEIGHT     |                    \
                                 TCF_FUN_IS_GENERATOR    |                    \
                                 TCF_FUN_USES_OWN_NAME   |                    \
                                 TCF_HAS_SHARPS          |                    \
                                 TCF_FUN_CALLS_EVAL      |                    \
                                 TCF_FUN_MIGHT_ALIAS_LOCALS |                 \
                                 TCF_FUN_MUTATES_PARAMETER |                  \
                                 TCF_STRICT_MODE_CODE    |                    \
                                 TCF_FUN_EXTENSIBLE_SCOPE)

struct BytecodeEmitter;

struct TreeContext {                
    uint32_t        flags;          
    uint32_t        bodyid;         
    uint32_t        blockidGen;     
    uint32_t        parenDepth;     

    uint32_t        yieldCount;     

    uint32_t        argumentsCount; 

    StmtInfo        *topStmt;       
    StmtInfo        *topScopeStmt;  
    StaticBlockObject *blockChain;  


    ParseNode       *blockNode;     

    AtomDecls       decls;          
    Parser          *parser;        
    ParseNode       *yieldNode;     


    ParseNode       *argumentsNode; 



  private:
    union {
        JSFunction  *fun_;          

        JSObject    *scopeChain_;   
    };

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
    uintN           staticLevel;    

    FunctionBox     *funbox;        


    FunctionBox     *functionList;

    ParseNode       *innermostWith; 

    Bindings        bindings;       

    Bindings::StackRoot bindingsRoot; 

    void trace(JSTracer *trc);

    inline TreeContext(Parser *prs);
    inline ~TreeContext();

    





    enum InitBehavior {
        USED_AS_TREE_CONTEXT,
        USED_AS_CODE_GENERATOR
    };

    bool init(JSContext *cx, InitBehavior ib = USED_AS_TREE_CONTEXT) {
        if (ib == USED_AS_CODE_GENERATOR)
            return true;
        return decls.init() && lexdeps.ensureMap(cx);
    }

    uintN blockid() { return topStmt ? topStmt->blockid : bodyid; }

    







    bool atBodyLevel() { return !topStmt || (topStmt->flags & SIF_BODY_BLOCK); }

    
    bool inStatement(StmtType type);

    bool inStrictMode() const {
        return flags & TCF_STRICT_MODE_CODE;
    }

    inline bool needStrictChecks();

    



    int sharpSlotBase;
    bool ensureSharpSlots();

    
    
    
    bool skipSpansGenerator(unsigned skip);

    bool compileAndGo() const { return flags & TCF_COMPILE_N_GO; }
    bool inFunction() const { return flags & TCF_IN_FUNCTION; }

    bool compiling() const { return flags & TCF_COMPILING; }
    inline BytecodeEmitter *asBytecodeEmitter();

    bool usesArguments() const {
        return flags & TCF_FUN_USES_ARGUMENTS;
    }

    void noteCallsEval() {
        flags |= TCF_FUN_CALLS_EVAL;
    }

    bool callsEval() const {
        return flags & TCF_FUN_CALLS_EVAL;
    }

    void noteMightAliasLocals() {
        flags |= TCF_FUN_MIGHT_ALIAS_LOCALS;
    }

    bool mightAliasLocals() const {
        return flags & TCF_FUN_MIGHT_ALIAS_LOCALS;
    }

    void noteParameterMutation() {
        JS_ASSERT(inFunction());
        flags |= TCF_FUN_MUTATES_PARAMETER;
    }

    bool mutatesParameter() const {
        JS_ASSERT(inFunction());
        return flags & TCF_FUN_MUTATES_PARAMETER;
    }

    void noteArgumentsUse(ParseNode *pn) {
        JS_ASSERT(inFunction());
        countArgumentsUse(pn);
        flags |= TCF_FUN_USES_ARGUMENTS;
        if (funbox)
            funbox->node->pn_dflags |= PND_FUNARG;
    }

    void countArgumentsUse(ParseNode *pn) {
        JS_ASSERT(pn->pn_atom == parser->context->runtime->atomState.argumentsAtom);
        argumentsCount++;
        argumentsNode = pn;
    }

    bool needsEagerArguments() const {
        return inStrictMode() && ((usesArguments() && mutatesParameter()) || callsEval());
    }

    void noteHasExtensibleScope() {
        flags |= TCF_FUN_EXTENSIBLE_SCOPE;
    }

    bool hasExtensibleScope() const {
        return flags & TCF_FUN_EXTENSIBLE_SCOPE;
    }

    ParseNode *freeTree(ParseNode *pn) { return parser->freeTree(pn); }
};





inline bool TreeContext::needStrictChecks() {
    return parser->context->hasStrictOption() || inStrictMode();
}

namespace frontend {

bool
SetStaticLevel(TreeContext *tc, uintN staticLevel);

bool
GenerateBlockId(TreeContext *tc, uint32_t &blockid);

} 

struct JumpTarget;





struct SpanDep {
    ptrdiff_t       top;        
    ptrdiff_t       offset;     
    ptrdiff_t       before;     
    JumpTarget      *target;    
};







struct JumpTarget {
    ptrdiff_t       offset;     
    int             balance;    
    JumpTarget      *kids[2];   
};

#define JT_LEFT                 0
#define JT_RIGHT                1
#define JT_OTHER_DIR(dir)       (1 - (dir))
#define JT_IMBALANCE(dir)       (((dir) << 1) - 1)
#define JT_DIR(imbalance)       (((imbalance) + 1) >> 1)






#define JT_TAG_BIT              ((jsword) 1)
#define JT_UNTAG_SHIFT          1
#define JT_SET_TAG(jt)          ((JumpTarget *)((jsword)(jt) | JT_TAG_BIT))
#define JT_CLR_TAG(jt)          ((JumpTarget *)((jsword)(jt) & ~JT_TAG_BIT))
#define JT_HAS_TAG(jt)          ((jsword)(jt) & JT_TAG_BIT)

#define BITS_PER_PTRDIFF        (sizeof(ptrdiff_t) * JS_BITS_PER_BYTE)
#define BITS_PER_BPDELTA        (BITS_PER_PTRDIFF - 1 - JT_UNTAG_SHIFT)
#define BPDELTA_MAX             (((ptrdiff_t)1 << BITS_PER_BPDELTA) - 1)
#define BPDELTA_TO_JT(bp)       ((JumpTarget *)((bp) << JT_UNTAG_SHIFT))
#define JT_TO_BPDELTA(jt)       ((ptrdiff_t)((jsword)(jt) >> JT_UNTAG_SHIFT))

#define SD_SET_TARGET(sd,jt)    ((sd)->target = JT_SET_TAG(jt))
#define SD_GET_TARGET(sd)       (JS_ASSERT(JT_HAS_TAG((sd)->target)),         \
                                 JT_CLR_TAG((sd)->target))
#define SD_SET_BPDELTA(sd,bp)   ((sd)->target = BPDELTA_TO_JT(bp))
#define SD_GET_BPDELTA(sd)      (JS_ASSERT(!JT_HAS_TAG((sd)->target)),        \
                                 JT_TO_BPDELTA((sd)->target))


#define SD_SPAN(sd,pivot)       (SD_GET_TARGET(sd)                            \
                                 ? JT_CLR_TAG((sd)->target)->offset - (pivot) \
                                 : 0)

struct TryNode {
    JSTryNote       note;
    TryNode       *prev;
};

struct CGObjectList {
    uint32_t            length;     
    ObjectBox           *lastbox;   

    CGObjectList() : length(0), lastbox(NULL) {}

    uintN index(ObjectBox *objbox);
    void finish(JSObjectArray *array);
};

class GCConstList {
    Vector<Value> list;
  public:
    GCConstList(JSContext *cx) : list(cx) {}
    bool append(Value v) { return list.append(v); }
    size_t length() const { return list.length(); }
    void finish(JSConstArray *array);
};

struct GlobalScope {
    GlobalScope(JSContext *cx, JSObject *globalObj, BytecodeEmitter *bce)
      : globalObj(globalObj), bce(bce), defs(cx), names(cx)
    { }

    struct GlobalDef {
        JSAtom        *atom;        
        FunctionBox   *funbox;      
                                    
        uint32_t      knownSlot;    

        GlobalDef() { }
        GlobalDef(uint32_t knownSlot) : atom(NULL), knownSlot(knownSlot) { }
        GlobalDef(JSAtom *atom, FunctionBox *box) : atom(atom), funbox(box) { }
    };

    JSObject        *globalObj;
    BytecodeEmitter *bce;

    







    Vector<GlobalDef, 16> defs;
    AtomIndexMap      names;
};

struct BytecodeEmitter : public TreeContext
{
    struct {
        jsbytecode  *base;          
        jsbytecode  *limit;         
        jsbytecode  *next;          
        jssrcnote   *notes;         
        uintN       noteCount;      
        uintN       noteLimit;      
        ptrdiff_t   lastNoteOffset; 
        uintN       currentLine;    
    } prolog, main, *current;

    OwnedAtomIndexMapPtr atomIndices; 
    AtomDefnMapPtr  roLexdeps;
    uintN           firstLine;      

    intN            stackDepth;     
    uintN           maxStackDepth;  

    uintN           ntrynotes;      
    TryNode         *lastTryNode;   

    SpanDep         *spanDeps;      
    JumpTarget      *jumpTargets;   
    JumpTarget      *jtFreeList;    
    uintN           numSpanDeps;    
    uintN           numJumpTargets; 
    ptrdiff_t       spanDepTodo;    


    uintN           arrayCompDepth; 

    uintN           emitLevel;      

    typedef HashMap<JSAtom *, Value> ConstMap;
    ConstMap        constMap;       

    GCConstList     constList;      

    CGObjectList    objectList;     
    CGObjectList    regexpList;     


    OwnedAtomIndexMapPtr upvarIndices; 

    UpvarCookies    upvarMap;       

    GlobalScope     *globalScope;   

    typedef Vector<GlobalSlotArray::Entry, 16> GlobalUseVector;

    GlobalUseVector globalUses;     
    OwnedAtomIndexMapPtr globalMap; 

    
    typedef Vector<uint32_t, 8> SlotVector;
    SlotVector      closedArgs;
    SlotVector      closedVars;

    uint16_t        typesetCount;   

    BytecodeEmitter(Parser *parser, uintN lineno);
    bool init(JSContext *cx, TreeContext::InitBehavior ib = USED_AS_CODE_GENERATOR);

    JSContext *context() {
        return parser->context;
    }

    





    ~BytecodeEmitter();

    













    bool addGlobalUse(JSAtom *atom, uint32_t slot, UpvarCookie *cookie);

    bool hasUpvarIndices() const {
        return upvarIndices.hasMap() && !upvarIndices->empty();
    }

    bool hasSharps() const {
        bool rv = !!(flags & TCF_HAS_SHARPS);
        JS_ASSERT((sharpSlotBase >= 0) == rv);
        return rv;
    }

    uintN sharpSlots() const {
        return hasSharps() ? SHARP_NSLOTS : 0;
    }

    bool compilingForEval() const { return !!(flags & TCF_COMPILE_FOR_EVAL); }
    JSVersion version() const { return parser->versionWithFlags(); }

    bool shouldNoteClosedName(ParseNode *pn);

    JS_ALWAYS_INLINE
    bool makeAtomIndex(JSAtom *atom, jsatomid *indexp) {
        AtomIndexAddPtr p = atomIndices->lookupForAdd(atom);
        if (p) {
            *indexp = p.value();
            return true;
        }

        jsatomid index = atomIndices->count();
        if (!atomIndices->add(p, atom, index))
            return false;

        *indexp = index;
        return true;
    }

    bool checkSingletonContext() {
        if (!compileAndGo() || inFunction())
            return false;
        for (StmtInfo *stmt = topStmt; stmt; stmt = stmt->down) {
            if (STMT_IS_LOOP(stmt))
                return false;
        }
        flags |= TCF_HAS_SINGLETONS;
        return true;
    }

    bool needsImplicitThis();

    TokenStream *tokenStream() { return &parser->tokenStream; }

    jsbytecode *base() const { return current->base; }
    jsbytecode *limit() const { return current->limit; }
    jsbytecode *next() const { return current->next; }
    jsbytecode *code(ptrdiff_t offset) const { return base() + offset; }
    ptrdiff_t offset() const { return next() - base(); }
    jsbytecode *prologBase() const { return prolog.base; }
    ptrdiff_t prologOffset() const { return prolog.next - prolog.base; }
    void switchToMain() { current = &main; }
    void switchToProlog() { current = &prolog; }

    jssrcnote *notes() const { return current->notes; }
    uintN noteCount() const { return current->noteCount; }
    uintN noteLimit() const { return current->noteLimit; }
    ptrdiff_t lastNoteOffset() const { return current->lastNoteOffset; }
    uintN currentLine() const { return current->currentLine; }

    inline ptrdiff_t countFinalSourceNotes();
};

inline BytecodeEmitter *
TreeContext::asBytecodeEmitter()
{
    JS_ASSERT(compiling());
    return static_cast<BytecodeEmitter *>(this);
}

namespace frontend {




ptrdiff_t
Emit1(JSContext *cx, BytecodeEmitter *bce, JSOp op);




ptrdiff_t
Emit2(JSContext *cx, BytecodeEmitter *bce, JSOp op, jsbytecode op1);




ptrdiff_t
Emit3(JSContext *cx, BytecodeEmitter *bce, JSOp op, jsbytecode op1, jsbytecode op2);




ptrdiff_t
EmitN(JSContext *cx, BytecodeEmitter *bce, JSOp op, size_t extra);




#define CHECK_AND_SET_JUMP_OFFSET_CUSTOM(cx,bce,pc,off,BAD_EXIT)              \
    JS_BEGIN_MACRO                                                            \
        if (!SetJumpOffset(cx, bce, pc, off)) {                               \
            BAD_EXIT;                                                         \
        }                                                                     \
    JS_END_MACRO

#define CHECK_AND_SET_JUMP_OFFSET(cx,bce,pc,off)                              \
    CHECK_AND_SET_JUMP_OFFSET_CUSTOM(cx,bce,pc,off,return JS_FALSE)

#define CHECK_AND_SET_JUMP_OFFSET_AT_CUSTOM(cx,bce,off,BAD_EXIT)              \
    CHECK_AND_SET_JUMP_OFFSET_CUSTOM(cx, bce, (bce)->code(off),               \
                                     bce->offset() - (off), BAD_EXIT)

#define CHECK_AND_SET_JUMP_OFFSET_AT(cx,bce,off)                              \
    CHECK_AND_SET_JUMP_OFFSET_AT_CUSTOM(cx, bce, off, return JS_FALSE)

JSBool
SetJumpOffset(JSContext *cx, BytecodeEmitter *bce, jsbytecode *pc, ptrdiff_t off);




void
PushStatement(TreeContext *tc, StmtInfo *stmt, StmtType type, ptrdiff_t top);






void
PushBlockScope(TreeContext *tc, StmtInfo *stmt, StaticBlockObject &blockObj, ptrdiff_t top);





void
PopStatementTC(TreeContext *tc);






JSBool
PopStatementBCE(JSContext *cx, BytecodeEmitter *bce);













JSBool
DefineCompileTimeConstant(JSContext *cx, BytecodeEmitter *bce, JSAtom *atom, ParseNode *pn);















StmtInfo *
LexicalLookup(TreeContext *tc, JSAtom *atom, jsint *slotp, StmtInfo *stmt = NULL);




JSBool
EmitTree(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn);




JSBool
EmitFunctionScript(JSContext *cx, BytecodeEmitter *bce, ParseNode *body);

} 





























enum SrcNoteType {
    SRC_NULL        = 0,        
    SRC_IF          = 1,        
    SRC_BREAK       = 1,        
    SRC_INITPROP    = 1,        


    SRC_GENEXP      = 1,        
    SRC_IF_ELSE     = 2,        
    SRC_FOR_IN      = 2,        

    SRC_FOR         = 3,        
    SRC_WHILE       = 4,        


    SRC_CONTINUE    = 5,        




    SRC_DECL        = 6,        
    SRC_DESTRUCT    = 6,        

    SRC_PCDELTA     = 7,        


    SRC_GROUPASSIGN = 7,        
    SRC_DESTRUCTLET = 7,        

    SRC_ASSIGNOP    = 8,        
    SRC_COND        = 9,        
    SRC_BRACE       = 10,       

    SRC_HIDDEN      = 11,       
    SRC_PCBASE      = 12,       


    SRC_LABEL       = 13,       
    SRC_LABELBRACE  = 14,       
    SRC_ENDBRACE    = 15,       
    SRC_BREAK2LABEL = 16,       
    SRC_CONT2LABEL  = 17,       
    SRC_SWITCH      = 18,       

    SRC_SWITCHBREAK = 18,       
    SRC_FUNCDEF     = 19,       
    SRC_CATCH       = 20,       
                                
    SRC_NEWLINE     = 22,       
    SRC_SETLINE     = 23,       
    SRC_XDELTA      = 24        
};











#define SRC_DECL_VAR            0
#define SRC_DECL_CONST          1
#define SRC_DECL_LET            2
#define SRC_DECL_NONE           3

#define SN_TYPE_BITS            5
#define SN_DELTA_BITS           3
#define SN_XDELTA_BITS          6
#define SN_TYPE_MASK            (JS_BITMASK(SN_TYPE_BITS) << SN_DELTA_BITS)
#define SN_DELTA_MASK           ((ptrdiff_t)JS_BITMASK(SN_DELTA_BITS))
#define SN_XDELTA_MASK          ((ptrdiff_t)JS_BITMASK(SN_XDELTA_BITS))

#define SN_MAKE_NOTE(sn,t,d)    (*(sn) = (jssrcnote)                          \
                                          (((t) << SN_DELTA_BITS)             \
                                           | ((d) & SN_DELTA_MASK)))
#define SN_MAKE_XDELTA(sn,d)    (*(sn) = (jssrcnote)                          \
                                          ((SRC_XDELTA << SN_DELTA_BITS)      \
                                           | ((d) & SN_XDELTA_MASK)))

#define SN_IS_XDELTA(sn)        ((*(sn) >> SN_DELTA_BITS) >= SRC_XDELTA)
#define SN_TYPE(sn)             ((js::SrcNoteType)(SN_IS_XDELTA(sn)           \
                                                   ? SRC_XDELTA               \
                                                   : *(sn) >> SN_DELTA_BITS))
#define SN_SET_TYPE(sn,type)    SN_MAKE_NOTE(sn, type, SN_DELTA(sn))
#define SN_IS_GETTABLE(sn)      (SN_TYPE(sn) < SRC_NEWLINE)

#define SN_DELTA(sn)            ((ptrdiff_t)(SN_IS_XDELTA(sn)                 \
                                             ? *(sn) & SN_XDELTA_MASK         \
                                             : *(sn) & SN_DELTA_MASK))
#define SN_SET_DELTA(sn,delta)  (SN_IS_XDELTA(sn)                             \
                                 ? SN_MAKE_XDELTA(sn, delta)                  \
                                 : SN_MAKE_NOTE(sn, SN_TYPE(sn), delta))

#define SN_DELTA_LIMIT          ((ptrdiff_t)JS_BIT(SN_DELTA_BITS))
#define SN_XDELTA_LIMIT         ((ptrdiff_t)JS_BIT(SN_XDELTA_BITS))






#define SN_3BYTE_OFFSET_FLAG    0x80
#define SN_3BYTE_OFFSET_MASK    0x7f

#define SN_MAX_OFFSET ((size_t)((ptrdiff_t)SN_3BYTE_OFFSET_FLAG << 16) - 1)

#define SN_LENGTH(sn)           ((js_SrcNoteSpec[SN_TYPE(sn)].arity == 0) ? 1 \
                                 : js_SrcNoteLength(sn))
#define SN_NEXT(sn)             ((sn) + SN_LENGTH(sn))


#define SN_MAKE_TERMINATOR(sn)  (*(sn) = SRC_NULL)
#define SN_IS_TERMINATOR(sn)    (*(sn) == SRC_NULL)

namespace frontend {







intN
NewSrcNote(JSContext *cx, BytecodeEmitter *bce, SrcNoteType type);

intN
NewSrcNote2(JSContext *cx, BytecodeEmitter *bce, SrcNoteType type, ptrdiff_t offset);

intN
NewSrcNote3(JSContext *cx, BytecodeEmitter *bce, SrcNoteType type, ptrdiff_t offset1,
               ptrdiff_t offset2);




jssrcnote *
AddToSrcNoteDelta(JSContext *cx, BytecodeEmitter *bce, jssrcnote *sn, ptrdiff_t delta);

JSBool
FinishTakingSrcNotes(JSContext *cx, BytecodeEmitter *bce, jssrcnote *notes);

void
FinishTakingTryNotes(BytecodeEmitter *bce, JSTryNoteArray *array);

} 











inline ptrdiff_t
BytecodeEmitter::countFinalSourceNotes()
{
    ptrdiff_t diff = prologOffset() - prolog.lastNoteOffset;
    ptrdiff_t cnt = prolog.noteCount + main.noteCount + 1;
    if (prolog.noteCount && prolog.currentLine != firstLine) {
        if (diff > SN_DELTA_MASK)
            cnt += JS_HOWMANY(diff - SN_DELTA_MASK, SN_XDELTA_MASK);
        cnt += 2 + ((firstLine > SN_3BYTE_OFFSET_MASK) << 1);
    } else if (diff > 0) {
        if (main.noteCount) {
            jssrcnote *sn = main.notes;
            diff -= SN_IS_XDELTA(sn)
                    ? SN_XDELTA_MASK - (*sn & SN_XDELTA_MASK)
                    : SN_DELTA_MASK - (*sn & SN_DELTA_MASK);
        }
        if (diff > 0)
            cnt += JS_HOWMANY(diff, SN_XDELTA_MASK);
    }
    return cnt;
}







inline ptrdiff_t PackLetData(size_t offset, bool groupAssign)
{
    JS_ASSERT(offset <= (size_t(-1) >> 1));
    return ptrdiff_t(offset << 1) | ptrdiff_t(groupAssign);
}

inline size_t LetDataToOffset(ptrdiff_t w)
{
    return size_t(w) >> 1;
}

inline bool LetDataToGroupAssign(ptrdiff_t w)
{
    return size_t(w) & 1;
}

} 

struct JSSrcNoteSpec {
    const char      *name;      
    int8_t          arity;      
    uint8_t         offsetBias; 
    int8_t          isSpanDep;  

};

extern JS_FRIEND_DATA(JSSrcNoteSpec)  js_SrcNoteSpec[];
extern JS_FRIEND_API(uintN)         js_SrcNoteLength(jssrcnote *sn);




extern JS_FRIEND_API(ptrdiff_t)
js_GetSrcNoteOffset(jssrcnote *sn, uintN which);

#endif 
