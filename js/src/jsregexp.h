






































#ifndef jsregexp_h___
#define jsregexp_h___



#include <stddef.h>
#include "jspubtd.h"
#include "jsstr.h"

#ifdef JS_THREADSAFE
#include "jsdhash.h"
#endif

JS_BEGIN_EXTERN_C

struct JSRegExpStatics {
    JSString    *input;         
    JSBool      multiline;      
    uint16      parenCount;     
    uint16      moreLength;     
    JSSubString parens[9];      
    JSSubString *moreParens;    
    JSSubString lastMatch;      
    JSSubString lastParen;      
    JSSubString leftContext;    
    JSSubString rightContext;   
};









typedef struct RECharSet {
    JSPackedBool    converted;
    JSPackedBool    sense;
    uint16          length;
    union {
        uint8       *bits;
        struct {
            size_t  startIndex;
            size_t  length;
        } src;
    } u;
} RECharSet;





#define REGEXP_PAREN_SUBSTRING(res, num)                                      \
    (((jsuint)(num) < (jsuint)(res)->parenCount)                              \
     ? ((jsuint)(num) < 9)                                                    \
       ? &(res)->parens[num]                                                  \
       : &(res)->moreParens[(num) - 9]                                        \
     : &js_EmptySubString)

typedef struct RENode RENode;

struct JSRegExp {
    jsrefcount   nrefs;         
    uint16       flags;         
    size_t       parenCount;    
    size_t       classCount;    
    RECharSet    *classList;    
    JSString     *source;       
    jsbytecode   program[1];    
};

extern JSRegExp *
js_NewRegExp(JSContext *cx, JSTokenStream *ts,
             JSString *str, uintN flags, JSBool flat);

extern JSRegExp *
js_NewRegExpOpt(JSContext *cx, JSTokenStream *ts,
                JSString *str, JSString *opt, JSBool flat);

#define HOLD_REGEXP(cx, re) JS_ATOMIC_INCREMENT(&(re)->nrefs)
#define DROP_REGEXP(cx, re) js_DestroyRegExp(cx, re)

extern void
js_DestroyRegExp(JSContext *cx, JSRegExp *re);






extern JSBool
js_ExecuteRegExp(JSContext *cx, JSRegExp *re, JSString *str, size_t *indexp,
                 JSBool test, jsval *rval);





extern JSBool
js_InitRegExpStatics(JSContext *cx, JSRegExpStatics *res);

extern void
js_FreeRegExpStatics(JSContext *cx, JSRegExpStatics *res);

#define JSVAL_IS_REGEXP(cx, v)                                                \
    (JSVAL_IS_OBJECT(v) && JSVAL_TO_OBJECT(v) &&                              \
     OBJ_GET_CLASS(cx, JSVAL_TO_OBJECT(v)) == &js_RegExpClass)

extern JSClass js_RegExpClass;

extern JSObject *
js_InitRegExpClass(JSContext *cx, JSObject *obj);




extern JSBool
js_regexp_toString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                   jsval *rval);




extern JSObject *
js_NewRegExpObject(JSContext *cx, JSTokenStream *ts,
                   jschar *chars, size_t length, uintN flags);

extern JSBool
js_XDRRegExp(JSXDRState *xdr, JSObject **objp);

extern JSObject *
js_CloneRegExpObject(JSContext *cx, JSObject *obj, JSObject *parent);




extern JSBool
js_GetLastIndex(JSContext *cx, JSObject *obj, jsdouble *lastIndex);

extern JSBool
js_SetLastIndex(JSContext *cx, JSObject *obj, jsdouble lastIndex);

JS_END_EXTERN_C

#endif 
