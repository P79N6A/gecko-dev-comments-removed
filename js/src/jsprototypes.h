





#ifndef jsprototypes_h
#define jsprototypes_h


























#define CLASP(name)                 (&name##Class)
#define OCLASP(name)                (&name##Object::class_)
#define TYPED_ARRAY_CLASP(type)     (&TypedArrayObject::classes[Scalar::type])

#ifdef ENABLE_PARALLEL_JS
#define IF_PJS(real,imaginary) real
#else
#define IF_PJS(real,imaginary) imaginary
#endif

#ifdef EXPOSE_INTL_API
#define IF_INTL(real,imaginary) real
#else
#define IF_INTL(real,imaginary) imaginary
#endif

#ifdef ENABLE_BINARYDATA
#define IF_BDATA(real,imaginary) real
#else
#define IF_BDATA(real,imaginary) imaginary
#endif

#ifdef ENABLE_SHARED_ARRAY_BUFFER
#define IF_SAB(real,imaginary) real
#else
#define IF_SAB(real,imaginary) imaginary
#endif

#ifdef JS_HAS_SYMBOLS
#define IF_SYMBOLS(real,imaginary) real
#else
#define IF_SYMBOLS(real,imaginary) imaginary
#endif

#define JS_FOR_PROTOTYPES(real,imaginary) \
    imaginary(Null,              0,     js_InitNullClass,          dummy) \
    real(Object,                 1,     js_InitViaClassSpec,       &JSObject::class_) \
    real(Function,               2,     js_InitViaClassSpec,       &JSFunction::class_) \
    real(Array,                  3,     js_InitViaClassSpec,       OCLASP(Array)) \
    real(Boolean,                4,     js_InitBooleanClass,       OCLASP(Boolean)) \
    real(JSON,                   5,     js_InitJSONClass,          CLASP(JSON)) \
    real(Date,                   6,     js_InitViaClassSpec,       OCLASP(Date)) \
    real(Math,                   7,     js_InitMathClass,          CLASP(Math)) \
    real(Number,                 8,     js_InitNumberClass,        OCLASP(Number)) \
    real(String,                 9,     js_InitStringClass,        OCLASP(String)) \
    real(RegExp,                10,     js_InitRegExpClass,        OCLASP(RegExp)) \
    real(Error,                 11,     js_InitViaClassSpec,       OCLASP(Error)) \
    real(InternalError,         12,     js_InitViaClassSpec,       OCLASP(Error)) \
    real(EvalError,             13,     js_InitViaClassSpec,       OCLASP(Error)) \
    real(RangeError,            14,     js_InitViaClassSpec,       OCLASP(Error)) \
    real(ReferenceError,        15,     js_InitViaClassSpec,       OCLASP(Error)) \
    real(SyntaxError,           16,     js_InitViaClassSpec,       OCLASP(Error)) \
    real(TypeError,             17,     js_InitViaClassSpec,       OCLASP(Error)) \
    real(URIError,              18,     js_InitViaClassSpec,       OCLASP(Error)) \
    real(Iterator,              19,     js_InitIteratorClasses,    OCLASP(PropertyIterator)) \
    real(StopIteration,         20,     js_InitIteratorClasses,    OCLASP(StopIteration)) \
    real(ArrayBuffer,           21,     js_InitArrayBufferClass,   &js::ArrayBufferObject::protoClass) \
    real(Int8Array,             22,     js_InitViaClassSpec,       TYPED_ARRAY_CLASP(Int8)) \
    real(Uint8Array,            23,     js_InitViaClassSpec,       TYPED_ARRAY_CLASP(Uint8)) \
    real(Int16Array,            24,     js_InitViaClassSpec,       TYPED_ARRAY_CLASP(Int16)) \
    real(Uint16Array,           25,     js_InitViaClassSpec,       TYPED_ARRAY_CLASP(Uint16)) \
    real(Int32Array,            26,     js_InitViaClassSpec,       TYPED_ARRAY_CLASP(Int32)) \
    real(Uint32Array,           27,     js_InitViaClassSpec,       TYPED_ARRAY_CLASP(Uint32)) \
    real(Float32Array,          28,     js_InitViaClassSpec,       TYPED_ARRAY_CLASP(Float32)) \
    real(Float64Array,          29,     js_InitViaClassSpec,       TYPED_ARRAY_CLASP(Float64)) \
    real(Uint8ClampedArray,     30,     js_InitViaClassSpec,       TYPED_ARRAY_CLASP(Uint8Clamped)) \
    real(Proxy,                 31,     js_InitProxyClass,         OCLASP(Proxy)) \
    real(WeakMap,               32,     js_InitWeakMapClass,       OCLASP(WeakMap)) \
    real(Map,                   33,     js_InitMapClass,           OCLASP(Map)) \
    real(Set,                   34,     js_InitSetClass,           OCLASP(Set)) \
    real(DataView,              35,     js_InitDataViewClass,      OCLASP(DataView)) \
IF_SYMBOLS(real,imaginary)(Symbol,              36,     js_InitSymbolClass,        &js::SymbolObject::class_) \
IF_SAB(real,imaginary)(SharedArrayBuffer,       37,     js_InitSharedArrayBufferClass, &js::SharedArrayBufferObject::protoClass) \
IF_INTL(real,imaginary) (Intl,                  38,     js_InitIntlClass,          CLASP(Intl)) \
IF_BDATA(real,imaginary)(TypedObject,           39,     js_InitTypedObjectModuleObject,   OCLASP(TypedObjectModule)) \
    imaginary(GeneratorFunction,     40,     js_InitIteratorClasses, dummy) \
IF_BDATA(real,imaginary)(SIMD,                  41,     js_InitSIMDClass, OCLASP(SIMD)) \
    real(WeakSet,               42,     js_InitWeakSetClass,       OCLASP(WeakSet)) \

#define JS_FOR_EACH_PROTOTYPE(macro) JS_FOR_PROTOTYPES(macro,macro)

#endif 
