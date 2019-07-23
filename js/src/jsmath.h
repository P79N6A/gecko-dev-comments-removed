






































#ifndef jsmath_h___
#define jsmath_h___




JS_BEGIN_EXTERN_C

extern JSClass js_MathClass;

extern JSObject *
js_InitMathClass(JSContext *cx, JSObject *obj);

extern void
js_InitRandom(JSThreadData *data);

extern JSBool
js_math_ceil(JSContext *cx, uintN argc, jsval *vp);

extern JSBool
js_math_floor(JSContext *cx, uintN argc, jsval *vp);

extern JSBool
js_math_max(JSContext *cx, uintN argc, jsval *vp);

extern JSBool
js_math_min(JSContext *cx, uintN argc, jsval *vp);

extern JSBool
js_math_round(JSContext *cx, uintN argc, jsval *vp);

JS_END_EXTERN_C

#endif 
