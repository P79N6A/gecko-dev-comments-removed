






































#ifndef jsmath_h___
#define jsmath_h___




JS_BEGIN_EXTERN_C

extern JSClass js_MathClass;

extern JSObject *
js_InitMathClass(JSContext *cx, JSObject *obj);

extern void
js_random_init(JSRuntime *rt);

extern jsdouble
js_random_nextDouble(JSRuntime *rt);

JS_END_EXTERN_C

#endif 
