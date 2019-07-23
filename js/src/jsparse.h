







































#ifndef jsparse_h___
#define jsparse_h___



#include "jsconfig.h"
#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsscan.h"

JS_BEGIN_EXTERN_C



















































































































































































































typedef enum JSParseNodeArity {
    PN_FUNC     = -3,
    PN_LIST     = -2,
    PN_TERNARY  =  3,
    PN_BINARY   =  2,
    PN_UNARY    =  1,
    PN_NAME     = -1,
    PN_NULLARY  =  0
} JSParseNodeArity;

struct JSParseNode {
    uint16              pn_type;
    uint8               pn_op;
    int8                pn_arity;
    JSTokenPos          pn_pos;
    ptrdiff_t           pn_offset;      
    union {
        struct {                        
            JSParsedObjectBox *funpob;  
            JSParseNode *body;          
            uint16      flags;          
            uint16      sclen;          
        } func;
        struct {                        
            JSParseNode *head;          
            JSParseNode **tail;         
            uint32      count;          
            uint32      extra;          
        } list;
        struct {                        
            JSParseNode *kid1;          
            JSParseNode *kid2;          
            JSParseNode *kid3;          
        } ternary;
        struct {                        
            JSParseNode *left;
            JSParseNode *right;
            jsval       val;            
        } binary;
        struct {                        
            JSParseNode *kid;
            jsint       num;            
            JSBool      hidden;         
        } unary;
        struct {                        
            JSAtom      *atom;          
            JSParseNode *expr;          
            jsint       slot;           
            JSBool      constslot;      
        } name;
        struct {                        
            JSParsedObjectBox *pob;     
            JSParseNode *expr;          
            jsint       slot;           
        } lexical;
        struct {
            JSAtom      *atom;          
            JSAtom      *atom2;         
        } apair;
        struct {                        
            JSParsedObjectBox *pob;
        } object;
        jsdouble        dval;           
    } pn_u;
    JSParseNode         *pn_next;       
};

#define pn_funpob       pn_u.func.funpob
#define pn_body         pn_u.func.body
#define pn_flags        pn_u.func.flags
#define pn_sclen        pn_u.func.sclen
#define pn_head         pn_u.list.head
#define pn_tail         pn_u.list.tail
#define pn_count        pn_u.list.count
#define pn_extra        pn_u.list.extra
#define pn_kid1         pn_u.ternary.kid1
#define pn_kid2         pn_u.ternary.kid2
#define pn_kid3         pn_u.ternary.kid3
#define pn_left         pn_u.binary.left
#define pn_right        pn_u.binary.right
#define pn_val          pn_u.binary.val
#define pn_kid          pn_u.unary.kid
#define pn_num          pn_u.unary.num
#define pn_hidden       pn_u.unary.hidden
#define pn_atom         pn_u.name.atom
#define pn_expr         pn_u.name.expr
#define pn_slot         pn_u.name.slot
#define pn_const        pn_u.name.constslot
#define pn_dval         pn_u.dval
#define pn_atom2        pn_u.apair.atom2
#define pn_pob          pn_u.object.pob


#define PNX_STRCAT      0x01            /* TOK_PLUS list has string term */
#define PNX_CANTFOLD    0x02            /* TOK_PLUS list has unfoldable term */
#define PNX_POPVAR      0x04            /* TOK_VAR last result needs popping */
#define PNX_FORINVAR    0x08            /* TOK_VAR is left kid of TOK_IN node,
                                           which is left kid of TOK_FOR */
#define PNX_ENDCOMMA    0x10            /* array literal has comma at end */
#define PNX_XMLROOT     0x20            /* top-most node in XML literal tree */
#define PNX_GROUPINIT   0x40            /* var [a, b] = [c, d]; unit list */
#define PNX_NEEDBRACES  0x80            /* braces necessary due to closure */





#define PN_MOVE_NODE(pn, pn2)                                                 \
    JS_BEGIN_MACRO                                                            \
        (pn)->pn_type = (pn2)->pn_type;                                       \
        (pn)->pn_op = (pn2)->pn_op;                                           \
        (pn)->pn_arity = (pn2)->pn_arity;                                     \
        (pn)->pn_u = (pn2)->pn_u;                                             \
        PN_CLEAR_NODE(pn2);                                                   \
    JS_END_MACRO

#define PN_CLEAR_NODE(pn)                                                     \
    JS_BEGIN_MACRO                                                            \
        (pn)->pn_type = TOK_EOF;                                              \
        (pn)->pn_op = JSOP_NOP;                                               \
        (pn)->pn_arity = PN_NULLARY;                                          \
    JS_END_MACRO


#define PN_IS_CONSTANT(pn)                                                    \
    ((pn)->pn_type == TOK_NUMBER ||                                           \
     (pn)->pn_type == TOK_STRING ||                                           \
     ((pn)->pn_type == TOK_PRIMARY && (pn)->pn_op != JSOP_THIS))

#define PN_OP(pn)    ((JSOp)(pn)->pn_op)
#define PN_TYPE(pn)  ((JSTokenType)(pn)->pn_type)





#define PN_LAST(list) \
    ((JSParseNode *)((char *)(list)->pn_tail - offsetof(JSParseNode, pn_next)))

#define PN_INIT_LIST(list)                                                    \
    JS_BEGIN_MACRO                                                            \
        (list)->pn_head = NULL;                                               \
        (list)->pn_tail = &(list)->pn_head;                                   \
        (list)->pn_count = (list)->pn_extra = 0;                              \
    JS_END_MACRO

#define PN_INIT_LIST_1(list, pn)                                              \
    JS_BEGIN_MACRO                                                            \
        (list)->pn_head = (pn);                                               \
        (list)->pn_tail = &(pn)->pn_next;                                     \
        (list)->pn_count = 1;                                                 \
        (list)->pn_extra = 0;                                                 \
    JS_END_MACRO

#define PN_APPEND(list, pn)                                                   \
    JS_BEGIN_MACRO                                                            \
        *(list)->pn_tail = (pn);                                              \
        (list)->pn_tail = &(pn)->pn_next;                                     \
        (list)->pn_count++;                                                   \
    JS_END_MACRO

struct JSParsedObjectBox {
    JSParsedObjectBox   *traceLink;
    JSParsedObjectBox   *emitLink;
    JSObject            *object;
};

struct JSParseContext {
    JSTokenStream       tokenStream;
    void                *tempPoolMark;  
    JSPrincipals        *principals;    
    JSParseNode         *nodeList;      

    JSParsedObjectBox   *traceListHead; 

    JSTempValueRooter   tempRoot;       
#ifdef DEBUG
    




    void                *lastAllocMark;
#endif
};




#define TS(pc) (&(pc)->tokenStream)




extern JSParseNode *
js_ParseScript(JSContext *cx, JSObject *chain, JSParseContext *pc);

extern JSScript *
js_CompileScript(JSContext *cx, JSObject *obj, JSPrincipals *principals,
                 uint32 tcflags, const jschar *chars, size_t length,
                 FILE *file, const char *filename, uintN lineno);

extern JSBool
js_CompileFunctionBody(JSContext *cx, JSFunction *fun, JSPrincipals *principals,
                       const jschar *chars, size_t length,
                       const char *filename, uintN lineno);

extern JSBool
js_FoldConstants(JSContext *cx, JSParseNode *pn, JSTreeContext *tc);

#if JS_HAS_XML_SUPPORT
JS_FRIEND_API(JSParseNode *)
js_ParseXMLText(JSContext *cx, JSObject *chain, JSParseContext *pc,
                JSBool allowList);
#endif









extern JSBool
js_InitParseContext(JSContext *cx, JSParseContext *pc, JSPrincipals *principals,
                    const jschar *base, size_t length, FILE *fp,
                    const char *filename, uintN lineno);

extern void
js_FinishParseContext(JSContext *cx, JSParseContext *pc);

extern void
js_InitCompilePrincipals(JSContext *cx, JSParseContext *pc,
                         JSPrincipals *principals);




extern JSParsedObjectBox *
js_NewParsedObjectBox(JSContext *cx, JSParseContext *pc, JSObject *obj);

extern void
js_TraceParseContext(JSTracer *trc, JSParseContext *pc);

JS_END_EXTERN_C

#endif 
