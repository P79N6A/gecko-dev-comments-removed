






































#ifndef jsmath_h___
#define jsmath_h___




extern js::Class js_MathClass;

extern JSObject *
js_InitMathClass(JSContext *cx, JSObject *obj);

extern void
js_InitRandom(JSContext *cx);

extern JSBool
js_math_ceil(JSContext *cx, uintN argc, js::Value *vp);

extern JSBool
js_math_floor(JSContext *cx, uintN argc, js::Value *vp);

extern JSBool
js_math_max(JSContext *cx, uintN argc, js::Value *vp);

extern JSBool
js_math_min(JSContext *cx, uintN argc, js::Value *vp);

extern JSBool
js_math_round(JSContext *cx, uintN argc, js::Value *vp);

#endif 
