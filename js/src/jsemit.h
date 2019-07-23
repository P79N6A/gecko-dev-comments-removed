







































#ifndef jsemit_h___
#define jsemit_h___




#include "jsstddef.h"
#include "jstypes.h"
#include "jsatom.h"
#include "jsopcode.h"
#include "jsscript.h"
#include "jsprvtd.h"
#include "jspubtd.h"

JS_BEGIN_EXTERN_C









typedef enum JSStmtType {
    STMT_LABEL,                 
    STMT_IF,                    
    STMT_ELSE,                  
    STMT_BODY,                  

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
    STMT_WHILE_LOOP             
} JSStmtType;

#define STMT_TYPE_IN_RANGE(t,b,e) ((uint)((t) - (b)) <= (uintN)((e) - (b)))




















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

typedef struct JSStmtInfo JSStmtInfo;

struct JSStmtInfo {
    uint16          type;           
    uint16          flags;          
    ptrdiff_t       update;         
    ptrdiff_t       breaks;         
    ptrdiff_t       continues;      
    JSAtom          *atom;          
    JSStmtInfo      *down;          
    JSStmtInfo      *downScope;     
};

#define SIF_SCOPE        0x0001     /* statement has its own lexical scope */
#define SIF_BODY_BLOCK   0x0002     /* STMT_BLOCK type is a function body */









#define CATCHNOTE(stmt)  ((stmt).update)
#define GOSUBS(stmt)     ((stmt).breaks)
#define GUARDJUMP(stmt)  ((stmt).continues)

#define AT_TOP_LEVEL(tc)                                                      \
    (!(tc)->topStmt || ((tc)->topStmt->flags & SIF_BODY_BLOCK))

#define SET_STATEMENT_TOP(stmt, top)                                          \
    ((stmt)->update = (top), (stmt)->breaks = (stmt)->continues = (-1))

struct JSTreeContext {              
    uint16          flags;          
    uint16          numGlobalVars;  
    uint32          globalUses;     
    uint32          loopyGlobalUses;
    JSStmtInfo      *topStmt;       
    JSStmtInfo      *topScopeStmt;  
    JSObject        *blockChain;    


    JSParseNode     *blockNode;     

    JSAtomList      decls;          
    JSParseNode     *nodeList;      
};

#define TCF_COMPILING          0x01 /* generating bytecode; this tc is a cg */
#define TCF_IN_FUNCTION        0x02 /* parsing inside function body */
#define TCF_RETURN_EXPR        0x04 /* function has 'return expr;' */
#define TCF_RETURN_VOID        0x08 /* function has 'return;' */
#define TCF_RETURN_FLAGS       0x0C /* propagate these out of blocks */
#define TCF_IN_FOR_INIT        0x10 /* parsing init expr of for; exclude 'in' */
#define TCF_FUN_CLOSURE_VS_VAR 0x20 /* function and var with same name */
#define TCF_FUN_USES_NONLOCALS 0x40 /* function refers to non-local names */
#define TCF_FUN_HEAVYWEIGHT    0x80 /* function needs Call object per call */
#define TCF_FUN_IS_GENERATOR  0x100 /* parsed yield statement in function */
#define TCF_FUN_FLAGS         0x1E0 /* flags to propagate from FunctionBody */
#define TCF_HAS_DEFXMLNS      0x200 /* default xml namespace = ...; parsed */
#define TCF_HAS_FUNCTION_STMT 0x400 /* block contains a function statement */
#define TCF_GENEXP_LAMBDA     0x800 /* flag lambda from generator expression */

#define TREE_CONTEXT_INIT(tc)                                                 \
    ((tc)->flags = (tc)->numGlobalVars = 0,                                   \
     (tc)->globalUses = (tc)->loopyGlobalUses = 0,                            \
     (tc)->topStmt = (tc)->topScopeStmt = NULL,                               \
     (tc)->blockChain = NULL,                                                 \
     ATOM_LIST_INIT(&(tc)->decls),                                            \
     (tc)->nodeList = NULL, (tc)->blockNode = NULL)

#define TREE_CONTEXT_FINISH(tc)                                               \
    ((void)0)





typedef struct JSSpanDep    JSSpanDep;
typedef struct JSJumpTarget JSJumpTarget;

struct JSSpanDep {
    ptrdiff_t       top;        
    ptrdiff_t       offset;     
    ptrdiff_t       before;     
    JSJumpTarget    *target;    
};







struct JSJumpTarget {
    ptrdiff_t       offset;     
    int             balance;    
    JSJumpTarget    *kids[2];   
};

#define JT_LEFT                 0
#define JT_RIGHT                1
#define JT_OTHER_DIR(dir)       (1 - (dir))
#define JT_IMBALANCE(dir)       (((dir) << 1) - 1)
#define JT_DIR(imbalance)       (((imbalance) + 1) >> 1)






#define JT_TAG_BIT              ((jsword) 1)
#define JT_UNTAG_SHIFT          1
#define JT_SET_TAG(jt)          ((JSJumpTarget *)((jsword)(jt) | JT_TAG_BIT))
#define JT_CLR_TAG(jt)          ((JSJumpTarget *)((jsword)(jt) & ~JT_TAG_BIT))
#define JT_HAS_TAG(jt)          ((jsword)(jt) & JT_TAG_BIT)

#define BITS_PER_PTRDIFF        (sizeof(ptrdiff_t) * JS_BITS_PER_BYTE)
#define BITS_PER_BPDELTA        (BITS_PER_PTRDIFF - 1 - JT_UNTAG_SHIFT)
#define BPDELTA_MAX             (((ptrdiff_t)1 << BITS_PER_BPDELTA) - 1)
#define BPDELTA_TO_JT(bp)       ((JSJumpTarget *)((bp) << JT_UNTAG_SHIFT))
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

typedef struct JSTryNode JSTryNode;

struct JSTryNode {
    JSTryNote       note;
    JSTryNode       *prev;
};

struct JSCodeGenerator {
    JSTreeContext   treeContext;    

    JSArenaPool     *codePool;      
    JSArenaPool     *notePool;      
    void            *codeMark;      
    void            *noteMark;      
    void            *tempMark;      

    struct {
        jsbytecode  *base;          
        jsbytecode  *limit;         
        jsbytecode  *next;          
        jssrcnote   *notes;         
        uintN       noteCount;      
        uintN       noteMask;       
        ptrdiff_t   lastNoteOffset; 
        uintN       currentLine;    
    } prolog, main, *current;

    const char      *filename;      
    uintN           firstLine;      
    JSPrincipals    *principals;    
    JSAtomList      atomList;       

    intN            stackDepth;     
    uintN           maxStackDepth;  

    uintN           ntrynotes;      
    JSTryNode       *lastTryNode;   

    JSSpanDep       *spanDeps;      
    JSJumpTarget    *jumpTargets;   
    JSJumpTarget    *jtFreeList;    
    uintN           numSpanDeps;    
    uintN           numJumpTargets; 
    ptrdiff_t       spanDepTodo;    


    uintN           arrayCompSlot;  

    uintN           emitLevel;      
    JSAtomList      constList;      
    JSCodeGenerator *parent;        
};

#define CG_BASE(cg)             ((cg)->current->base)
#define CG_LIMIT(cg)            ((cg)->current->limit)
#define CG_NEXT(cg)             ((cg)->current->next)
#define CG_CODE(cg,offset)      (CG_BASE(cg) + (offset))
#define CG_OFFSET(cg)           PTRDIFF(CG_NEXT(cg), CG_BASE(cg), jsbytecode)

#define CG_NOTES(cg)            ((cg)->current->notes)
#define CG_NOTE_COUNT(cg)       ((cg)->current->noteCount)
#define CG_NOTE_MASK(cg)        ((cg)->current->noteMask)
#define CG_LAST_NOTE_OFFSET(cg) ((cg)->current->lastNoteOffset)
#define CG_CURRENT_LINE(cg)     ((cg)->current->currentLine)

#define CG_PROLOG_BASE(cg)      ((cg)->prolog.base)
#define CG_PROLOG_LIMIT(cg)     ((cg)->prolog.limit)
#define CG_PROLOG_NEXT(cg)      ((cg)->prolog.next)
#define CG_PROLOG_CODE(cg,poff) (CG_PROLOG_BASE(cg) + (poff))
#define CG_PROLOG_OFFSET(cg)    PTRDIFF(CG_PROLOG_NEXT(cg), CG_PROLOG_BASE(cg),\
                                        jsbytecode)

#define CG_SWITCH_TO_MAIN(cg)   ((cg)->current = &(cg)->main)
#define CG_SWITCH_TO_PROLOG(cg) ((cg)->current = &(cg)->prolog)







extern JS_FRIEND_API(JSBool)
js_InitCodeGenerator(JSContext *cx, JSCodeGenerator *cg,
                     JSArenaPool *codePool, JSArenaPool *notePool,
                     const char *filename, uintN lineno,
                     JSPrincipals *principals);








extern JS_FRIEND_API(void)
js_FinishCodeGenerator(JSContext *cx, JSCodeGenerator *cg);




extern ptrdiff_t
js_Emit1(JSContext *cx, JSCodeGenerator *cg, JSOp op);




extern ptrdiff_t
js_Emit2(JSContext *cx, JSCodeGenerator *cg, JSOp op, jsbytecode op1);




extern ptrdiff_t
js_Emit3(JSContext *cx, JSCodeGenerator *cg, JSOp op, jsbytecode op1,
         jsbytecode op2);




extern ptrdiff_t
js_EmitN(JSContext *cx, JSCodeGenerator *cg, JSOp op, size_t extra);




#define CHECK_AND_SET_JUMP_OFFSET(cx,cg,pc,off)                               \
    JS_BEGIN_MACRO                                                            \
        if (!js_SetJumpOffset(cx, cg, pc, off))                               \
            return JS_FALSE;                                                  \
    JS_END_MACRO

#define CHECK_AND_SET_JUMP_OFFSET_AT(cx,cg,off)                               \
    CHECK_AND_SET_JUMP_OFFSET(cx, cg, CG_CODE(cg,off), CG_OFFSET(cg) - (off))

extern JSBool
js_SetJumpOffset(JSContext *cx, JSCodeGenerator *cg, jsbytecode *pc,
                 ptrdiff_t off);


extern JSBool
js_InStatement(JSTreeContext *tc, JSStmtType type);


#define js_InWithStatement(tc)      js_InStatement(tc, STMT_WITH)






extern JSBool
js_IsGlobalReference(JSTreeContext *tc, JSAtom *atom, JSBool *loopyp);




extern void
js_PushStatement(JSTreeContext *tc, JSStmtInfo *stmt, JSStmtType type,
                 ptrdiff_t top);






extern void
js_PushBlockScope(JSTreeContext *tc, JSStmtInfo *stmt, JSAtom *blockAtom,
                  ptrdiff_t top);





extern void
js_PopStatement(JSTreeContext *tc);






extern JSBool
js_PopStatementCG(JSContext *cx, JSCodeGenerator *cg);













extern JSBool
js_DefineCompileTimeConstant(JSContext *cx, JSCodeGenerator *cg, JSAtom *atom,
                             JSParseNode *pn);

extern JSBool
js_LookupCompileTimeConstant(JSContext *cx, JSCodeGenerator *cg, JSAtom *atom,
                             jsval *vp);















extern JSStmtInfo *
js_LexicalLookup(JSTreeContext *tc, JSAtom *atom, jsint *slotp,
                 uintN decltype);




extern JSBool
js_EmitTree(JSContext *cx, JSCodeGenerator *cg, JSParseNode *pn);




extern JSBool
js_EmitFunctionBytecode(JSContext *cx, JSCodeGenerator *cg, JSParseNode *body);





extern JSBool
js_EmitFunctionBody(JSContext *cx, JSCodeGenerator *cg, JSParseNode *body,
                    JSFunction *fun);































typedef enum JSSrcNoteType {
    SRC_NULL        = 0,        
    SRC_IF          = 1,        
    SRC_INITPROP    = 1,        


    SRC_GENEXP      = 1,        
    SRC_IF_ELSE     = 2,        
    SRC_WHILE       = 3,        
    SRC_FOR         = 4,        
    SRC_CONTINUE    = 5,        


    SRC_DECL        = 6,        
    SRC_DESTRUCT    = 6,        

    SRC_PCDELTA     = 7,        


    SRC_GROUPASSIGN = 7,        
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

    SRC_FUNCDEF     = 19,       
    SRC_CATCH       = 20,       
    SRC_EXTENDED    = 21,       
    SRC_NEWLINE     = 22,       
    SRC_SETLINE     = 23,       
    SRC_XDELTA      = 24        
} JSSrcNoteType;











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
#define SN_TYPE(sn)             (SN_IS_XDELTA(sn) ? SRC_XDELTA                \
                                                  : *(sn) >> SN_DELTA_BITS)
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

typedef struct JSSrcNoteSpec {
    const char      *name;      
    uint8           arity;      
    uint8           offsetBias; 
    int8            isSpanDep;  

} JSSrcNoteSpec;

extern JS_FRIEND_DATA(JSSrcNoteSpec) js_SrcNoteSpec[];
extern JS_FRIEND_API(uintN)          js_SrcNoteLength(jssrcnote *sn);

#define SN_LENGTH(sn)           ((js_SrcNoteSpec[SN_TYPE(sn)].arity == 0) ? 1 \
                                 : js_SrcNoteLength(sn))
#define SN_NEXT(sn)             ((sn) + SN_LENGTH(sn))


#define SN_MAKE_TERMINATOR(sn)  (*(sn) = SRC_NULL)
#define SN_IS_TERMINATOR(sn)    (*(sn) == SRC_NULL)







extern intN
js_NewSrcNote(JSContext *cx, JSCodeGenerator *cg, JSSrcNoteType type);

extern intN
js_NewSrcNote2(JSContext *cx, JSCodeGenerator *cg, JSSrcNoteType type,
               ptrdiff_t offset);

extern intN
js_NewSrcNote3(JSContext *cx, JSCodeGenerator *cg, JSSrcNoteType type,
               ptrdiff_t offset1, ptrdiff_t offset2);




extern jssrcnote *
js_AddToSrcNoteDelta(JSContext *cx, JSCodeGenerator *cg, jssrcnote *sn,
                     ptrdiff_t delta);




extern JS_FRIEND_API(ptrdiff_t)
js_GetSrcNoteOffset(jssrcnote *sn, uintN which);

extern JSBool
js_SetSrcNoteOffset(JSContext *cx, JSCodeGenerator *cg, uintN index,
                    uintN which, ptrdiff_t offset);











#define CG_COUNT_FINAL_SRCNOTES(cg, cnt)                                      \
    JS_BEGIN_MACRO                                                            \
        ptrdiff_t diff_ = CG_PROLOG_OFFSET(cg) - (cg)->prolog.lastNoteOffset; \
        cnt = (cg)->prolog.noteCount + (cg)->main.noteCount + 1;              \
        if ((cg)->prolog.noteCount &&                                         \
            (cg)->prolog.currentLine != (cg)->firstLine) {                    \
            if (diff_ > SN_DELTA_MASK)                                        \
                cnt += JS_HOWMANY(diff_ - SN_DELTA_MASK, SN_XDELTA_MASK);     \
            cnt += 2 + (((cg)->firstLine > SN_3BYTE_OFFSET_MASK) << 1);       \
        } else if (diff_ > 0) {                                               \
            if (cg->main.noteCount) {                                         \
                jssrcnote *sn_ = (cg)->main.notes;                            \
                diff_ -= SN_IS_XDELTA(sn_)                                    \
                         ? SN_XDELTA_MASK - (*sn_ & SN_XDELTA_MASK)           \
                         : SN_DELTA_MASK - (*sn_ & SN_DELTA_MASK);            \
            }                                                                 \
            if (diff_ > 0)                                                    \
                cnt += JS_HOWMANY(diff_, SN_XDELTA_MASK);                     \
        }                                                                     \
    JS_END_MACRO

extern JSBool
js_FinishTakingSrcNotes(JSContext *cx, JSCodeGenerator *cg, jssrcnote *notes);

extern void
js_FinishTakingTryNotes(JSContext *cx, JSCodeGenerator *cg,
                        JSTryNoteArray *array);

JS_END_EXTERN_C

#endif 
