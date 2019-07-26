






#ifndef ParallelArray_h__
#define ParallelArray_h__

#include "jsapi.h"
#include "jscntxt.h"
#include "jsobj.h"

namespace js {

class ParallelArrayObject;
typedef Rooted<ParallelArrayObject *> RootedParallelArrayObject;
typedef Handle<ParallelArrayObject *> HandleParallelArrayObject;

































class ParallelArrayObject : public JSObject {
  public:
    typedef Vector<uint32_t, 4> IndexVector;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    struct IndexInfo {
        
        
        IndexVector indices;

        
        
        IndexVector dimensions;

        
        
        
        
        
        
        
        
        IndexVector partialProducts;

        IndexInfo(JSContext *cx)
            : indices(cx), dimensions(cx), partialProducts(cx)
        {}

        
        
        
        
        
        
        inline bool initialize(uint32_t space);

        
        inline bool initialize(JSContext *cx, HandleParallelArrayObject source,
                               uint32_t space);

        
        
        inline bool bump();

        
        
        inline uint32_t scalarLengthOfDimensions();

        
        inline uint32_t toScalar();

        
        inline bool fromScalar(uint32_t index);

        inline bool inBounds() const;
        bool isInitialized() const;
    };

    static JSObject *initClass(JSContext *cx, JSObject *obj);
    static Class class_;

    static inline bool is(const Value &v);
    static inline bool is(JSObject *obj);
    static inline ParallelArrayObject *as(JSObject *obj);

    inline JSObject *dimensionArray();
    inline JSObject *buffer();
    inline uint32_t bufferOffset();
    inline uint32_t outermostDimension();
    inline bool isOneDimensional();
    inline bool getDimensions(JSContext *cx, IndexVector &dims);

    
    bool getParallelArrayElement(JSContext *cx, IndexInfo &iv, MutableHandleValue vp);

    
    
    
    
    
    
    
    
    bool getParallelArrayElement(JSContext *cx, uint32_t index, IndexInfo *maybeIV,
                                 MutableHandleValue vp);

    
    
    
    bool getParallelArrayElement(JSContext *cx, uint32_t index, MutableHandleValue vp);

    bool toStringBuffer(JSContext *cx, bool useLocale, StringBuffer &sb);

    
    
    static bool enumerate(JSContext *cx, HandleObject obj, unsigned flags,
                          AutoIdVector *props);

  private:
    enum {
        
        
        
        SLOT_DIMENSIONS = 0,

        
        SLOT_BUFFER,

        
        SLOT_BUFFER_OFFSET,

        RESERVED_SLOTS
    };

    enum ExecutionStatus {
        ExecutionFailed = 0,
        ExecutionCompiled,
        ExecutionSucceeded
    };

    
    
    
    
    
    
    

#define JS_PA_build_ARGS               \
    JSContext *cx,                     \
    IndexInfo &iv,                     \
    HandleObject elementalFun,         \
    HandleObject buffer

#define JS_PA_map_ARGS                 \
    JSContext *cx,                     \
    HandleParallelArrayObject source,  \
    HandleObject elementalFun,         \
    HandleObject buffer

#define JS_PA_reduce_ARGS              \
    JSContext *cx,                     \
    HandleParallelArrayObject source,  \
    HandleObject elementalFun,         \
    HandleObject buffer,               \
    MutableHandleValue vp

#define JS_PA_scatter_ARGS             \
    JSContext *cx,                     \
    HandleParallelArrayObject source,  \
    HandleObject targets,              \
    const Value &defaultValue,         \
    HandleObject conflictFun,          \
    HandleObject buffer

#define JS_PA_filter_ARGS              \
    JSContext *cx,                     \
    HandleParallelArrayObject source,  \
    HandleObject filters,              \
    HandleObject buffer

#define JS_PA_DECLARE_OP(NAME) \
    ExecutionStatus NAME(JS_PA_ ## NAME ## _ARGS)

#define JS_PA_DECLARE_ALL_OPS          \
    JS_PA_DECLARE_OP(build);           \
    JS_PA_DECLARE_OP(map);             \
    JS_PA_DECLARE_OP(reduce);          \
    JS_PA_DECLARE_OP(scatter);         \
    JS_PA_DECLARE_OP(filter);

    class ExecutionMode {
      public:
        
        
        virtual JS_PA_DECLARE_OP(build) = 0;

        
        virtual JS_PA_DECLARE_OP(map) = 0;

        
        
        virtual JS_PA_DECLARE_OP(reduce) = 0;

        
        virtual JS_PA_DECLARE_OP(scatter) = 0;

        
        virtual JS_PA_DECLARE_OP(filter) = 0;

        virtual const char *toString() = 0;
    };

    
    
    class FallbackMode : public ExecutionMode {
      public:
        JS_PA_DECLARE_ALL_OPS
        const char *toString() { return "fallback"; }
    };

    class ParallelMode : public ExecutionMode {
      public:
        JS_PA_DECLARE_ALL_OPS
        const char *toString() { return "parallel"; }
    };

    class SequentialMode : public ExecutionMode {
      public:
        JS_PA_DECLARE_ALL_OPS
        const char *toString() { return "sequential"; }
    };

    static SequentialMode sequential;
    static ParallelMode parallel;
    static FallbackMode fallback;

#undef JS_PA_build_ARGS
#undef JS_PA_map_ARGS
#undef JS_PA_reduce_ARGS
#undef JS_PA_scatter_ARGS
#undef JS_PA_filter_ARGS
#undef JS_PA_DECLARE_OP
#undef JS_PA_DECLARE_ALL_OPS

#ifdef DEBUG
    
    
    
    
    
    struct DebugOptions {
        ExecutionMode *mode;
        ExecutionStatus expect;
        bool init(JSContext *cx, const Value &v);
        bool check(JSContext *cx, ExecutionStatus actual);
    };

    static const char *ExecutionStatusToString(ExecutionStatus ss);
#endif

    static JSFunctionSpec methods[];
    static Class protoClass;

    static inline bool DenseArrayToIndexVector(JSContext *cx, HandleObject obj,
                                               IndexVector &indices);

    static bool create(JSContext *cx, MutableHandleValue vp);
    static bool create(JSContext *cx, HandleObject buffer, MutableHandleValue vp);
    static bool create(JSContext *cx, HandleObject buffer, uint32_t offset,
                       const IndexVector &dims, MutableHandleValue vp);
    static JSBool construct(JSContext *cx, unsigned argc, Value *vp);

    static bool map(JSContext *cx, CallArgs args);
    static bool reduce(JSContext *cx, CallArgs args);
    static bool scan(JSContext *cx, CallArgs args);
    static bool scatter(JSContext *cx, CallArgs args);
    static bool filter(JSContext *cx, CallArgs args);
    static bool flatten(JSContext *cx, CallArgs args);
    static bool partition(JSContext *cx, CallArgs args);
    static bool get(JSContext *cx, CallArgs args);
    static bool dimensionsGetter(JSContext *cx, CallArgs args);
    static bool lengthGetter(JSContext *cx, CallArgs args);
    static bool toString(JSContext *cx, CallArgs args);
    static bool toLocaleString(JSContext *cx, CallArgs args);
    static bool toSource(JSContext *cx, CallArgs args);

    static void mark(JSTracer *trc, RawObject obj);
    static JSBool lookupGeneric(JSContext *cx, HandleObject obj, HandleId id,
                                MutableHandleObject objp, MutableHandleShape propp);
    static JSBool lookupProperty(JSContext *cx, HandleObject obj, HandlePropertyName name,
                                 MutableHandleObject objp, MutableHandleShape propp);
    static JSBool lookupElement(JSContext *cx, HandleObject obj, uint32_t index,
                                MutableHandleObject objp, MutableHandleShape propp);
    static JSBool lookupSpecial(JSContext *cx, HandleObject obj, HandleSpecialId sid,
                                MutableHandleObject objp, MutableHandleShape propp);
    static JSBool defineGeneric(JSContext *cx, HandleObject obj, HandleId id, HandleValue value,
                                JSPropertyOp getter, StrictPropertyOp setter, unsigned attrs);
    static JSBool defineProperty(JSContext *cx, HandleObject obj,
                                 HandlePropertyName name, HandleValue value,
                                 JSPropertyOp getter, StrictPropertyOp setter, unsigned attrs);
    static JSBool defineElement(JSContext *cx, HandleObject obj,
                                uint32_t index, HandleValue value,
                                PropertyOp getter, StrictPropertyOp setter, unsigned attrs);
    static JSBool defineSpecial(JSContext *cx, HandleObject obj,
                                HandleSpecialId sid, HandleValue value,
                                PropertyOp getter, StrictPropertyOp setter, unsigned attrs);
    static JSBool getGeneric(JSContext *cx, HandleObject obj, HandleObject receiver,
                             HandleId id, MutableHandleValue vp);
    static JSBool getProperty(JSContext *cx, HandleObject obj, HandleObject receiver,
                              HandlePropertyName name, MutableHandleValue vp);
    static JSBool getElement(JSContext *cx, HandleObject obj, HandleObject receiver,
                             uint32_t index, MutableHandleValue vp);
    static JSBool getSpecial(JSContext *cx, HandleObject obj, HandleObject receiver,
                             HandleSpecialId sid, MutableHandleValue vp);
    static JSBool setGeneric(JSContext *cx, HandleObject obj, HandleId id,
                             MutableHandleValue vp, JSBool strict);
    static JSBool setProperty(JSContext *cx, HandleObject obj, HandlePropertyName name,
                              MutableHandleValue vp, JSBool strict);
    static JSBool setElement(JSContext *cx, HandleObject obj, uint32_t index,
                             MutableHandleValue vp, JSBool strict);
    static JSBool getElementIfPresent(JSContext *cx, HandleObject obj, HandleObject receiver,
                                      uint32_t index, MutableHandleValue vp, bool *present);
    static JSBool setSpecial(JSContext *cx, HandleObject obj, HandleSpecialId sid,
                             MutableHandleValue vp, JSBool strict);
    static JSBool getGenericAttributes(JSContext *cx, HandleObject obj, HandleId id,
                                       unsigned *attrsp);
    static JSBool getPropertyAttributes(JSContext *cx, HandleObject obj, HandlePropertyName name,
                                        unsigned *attrsp);
    static JSBool getElementAttributes(JSContext *cx, HandleObject obj, uint32_t index,
                                       unsigned *attrsp);
    static JSBool getSpecialAttributes(JSContext *cx, HandleObject obj, HandleSpecialId sid,
                                       unsigned *attrsp);
    static JSBool setGenericAttributes(JSContext *cx, HandleObject obj, HandleId id,
                                       unsigned *attrsp);
    static JSBool setPropertyAttributes(JSContext *cx, HandleObject obj, HandlePropertyName name,
                                        unsigned *attrsp);
    static JSBool setElementAttributes(JSContext *cx, HandleObject obj, uint32_t index,
                                       unsigned *attrsp);
    static JSBool setSpecialAttributes(JSContext *cx, HandleObject obj, HandleSpecialId sid,
                                       unsigned *attrsp);
    static JSBool deleteGeneric(JSContext *cx, HandleObject obj, HandleId id,
                                MutableHandleValue rval, JSBool strict);
    static JSBool deleteProperty(JSContext *cx, HandleObject obj, HandlePropertyName name,
                                 MutableHandleValue rval, JSBool strict);
    static JSBool deleteElement(JSContext *cx, HandleObject obj, uint32_t index,
                                MutableHandleValue rval, JSBool strict);
    static JSBool deleteSpecial(JSContext *cx, HandleObject obj, HandleSpecialId sid,
                                MutableHandleValue rval, JSBool strict);
};

} 

extern JSObject *
js_InitParallelArrayClass(JSContext *cx, js::HandleObject obj);

#endif
