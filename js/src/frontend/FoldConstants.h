







































#ifndef FoldConstants_h__
#define FoldConstants_h__

#include "jsprvtd.h"

JS_BEGIN_EXTERN_C

extern JSBool
js_FoldConstants(JSContext *cx, JSParseNode *pn, JSTreeContext *tc,
                 bool inCond = false);

JS_END_EXTERN_C

#endif 
