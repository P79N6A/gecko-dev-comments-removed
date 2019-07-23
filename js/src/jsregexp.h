






































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

extern JS_FRIEND_API(void)
js_SaveAndClearRegExpStatics(JSContext *cx, JSRegExpStatics *statics,
                             JSTempValueRooter *tvr);

extern JS_FRIEND_API(void)
js_RestoreRegExpStatics(JSContext *cx, JSRegExpStatics *statics,
                        JSTempValueRooter *tvr);









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
js_NewRegExpOpt(JSContext *cx, JSString *str, JSString *opt, JSBool flat);

#define HOLD_REGEXP(cx, re) JS_ATOMIC_INCREMENT(&(re)->nrefs)
#define DROP_REGEXP(cx, re) js_DestroyRegExp(cx, re)

extern void
js_DestroyRegExp(JSContext *cx, JSRegExp *re);






extern JSBool
js_ExecuteRegExp(JSContext *cx, JSRegExp *re, JSString *str, size_t *indexp,
                 JSBool test, jsval *rval);

extern void
js_InitRegExpStatics(JSContext *cx);

extern void
js_TraceRegExpStatics(JSTracer *trc, JSContext *acx);

extern void
js_FreeRegExpStatics(JSContext *cx);

#define VALUE_IS_REGEXP(cx, v)                                                \
    (JSVAL_IS_OBJECT(v) && JSVAL_TO_OBJECT(v) &&                              \
     OBJ_GET_CLASS(cx, JSVAL_TO_OBJECT(v)) == &js_RegExpClass)

extern JSClass js_RegExpClass;

enum regexp_tinyid {
    REGEXP_SOURCE       = -1,
    REGEXP_GLOBAL       = -2,
    REGEXP_IGNORE_CASE  = -3,
    REGEXP_LAST_INDEX   = -4,
    REGEXP_MULTILINE    = -5,
    REGEXP_STICKY       = -6
};

extern JSObject *
js_InitRegExpClass(JSContext *cx, JSObject *obj);




extern JSBool
js_regexp_toString(JSContext *cx, JSObject *obj, jsval *vp);




extern JSObject *
js_NewRegExpObject(JSContext *cx, JSTokenStream *ts,
                   jschar *chars, size_t length, uintN flags);

extern JSBool
js_XDRRegExpObject(JSXDRState *xdr, JSObject **objp);

extern JSObject *
js_CloneRegExpObject(JSContext *cx, JSObject *obj, JSObject *parent);

const uint32 JSSLOT_REGEXP_LAST_INDEX = JSSLOT_PRIVATE + 1;
const uint32 REGEXP_CLASS_FIXED_RESERVED_SLOTS = 1;

static inline void
js_ClearRegExpLastIndex(JSObject *obj)
{
    JS_ASSERT(obj->getClass() == &js_RegExpClass);
    obj->fslots[JSSLOT_REGEXP_LAST_INDEX] = JSVAL_ZERO;
}


extern bool
js_ContainsRegExpMetaChars(const jschar *chars, size_t length);

JS_END_EXTERN_C

#endif 
