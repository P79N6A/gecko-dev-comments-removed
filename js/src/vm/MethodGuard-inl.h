






































#ifndef MethodGuard_inl_h___
#define MethodGuard_inl_h___

#include "jsobj.h"

#include "MethodGuard.h"

#include "jsobjinlines.h"

#include "BooleanObject-inl.h"
#include "NumberObject-inl.h"
#include "StringObject-inl.h"

namespace js {

namespace detail {

template<typename T> class PrimitiveBehavior { };

template<>
class PrimitiveBehavior<bool> {
  public:
    static inline bool isType(const Value &v) { return v.isBoolean(); }
    static inline bool extract(const Value &v) { return v.toBoolean(); }
    static inline bool extract(JSObject &obj) { return obj.asBoolean().unbox(); }
    static inline Class *getClass() { return &BooleanClass; }
};

template<>
class PrimitiveBehavior<double> {
  public:
    static inline bool isType(const Value &v) { return v.isNumber(); }
    static inline double extract(const Value &v) { return v.toNumber(); }
    static inline double extract(JSObject &obj) { return obj.asNumber().unbox(); }
    static inline Class *getClass() { return &NumberClass; }
};

template<>
class PrimitiveBehavior<JSString *> {
  public:
    static inline bool isType(const Value &v) { return v.isString(); }
    static inline JSString *extract(const Value &v) { return v.toString(); }
    static inline JSString *extract(JSObject &obj) { return obj.asString().unbox(); }
    static inline Class *getClass() { return &StringClass; }
};

} 

inline JSObject *
NonGenericMethodGuard(JSContext *cx, CallArgs args, Native native, Class *clasp, bool *ok)
{
    const Value &thisv = args.thisv();
    if (thisv.isObject()) {
        JSObject &obj = thisv.toObject();
        if (obj.getClass() == clasp) {
            *ok = true;  
            return &obj;
        }
    }

    *ok = HandleNonGenericMethodClassMismatch(cx, args, native, clasp);
    return NULL;
}

template <typename T>
inline bool
BoxedPrimitiveMethodGuard(JSContext *cx, CallArgs args, Native native, T *v, bool *ok)
{
    typedef detail::PrimitiveBehavior<T> Behavior;

    const Value &thisv = args.thisv();
    if (Behavior::isType(thisv)) {
        *v = Behavior::extract(thisv);
        return true;
    }

    if (!NonGenericMethodGuard(cx, args, native, Behavior::getClass(), ok))
        return false;

    *v = Behavior::extract(thisv.toObject());
    return true;
}

} 

#endif 
