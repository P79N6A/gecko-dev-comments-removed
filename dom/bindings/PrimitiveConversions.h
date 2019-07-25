









#ifndef mozilla_dom_PrimitiveConversions_h
#define mozilla_dom_PrimitiveConversions_h

#include "xpcpublic.h"

namespace mozilla {
namespace dom {

template<typename T>
struct PrimitiveConversionTraits {
};

struct PrimitiveConversionTraits_smallInt {
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef int32_t jstype;
  typedef int32_t intermediateType;
  static inline bool converter(JSContext* cx, JS::Value v, jstype* retval) {
    return JS::ToInt32(cx, v, retval);
  }
};
template<>
struct PrimitiveConversionTraits<int8_t> : PrimitiveConversionTraits_smallInt {
  typedef uint8_t intermediateType;
};
template<>
struct PrimitiveConversionTraits<uint8_t> : PrimitiveConversionTraits_smallInt {
};
template<>
struct PrimitiveConversionTraits<int16_t> : PrimitiveConversionTraits_smallInt {
  typedef uint16_t intermediateType;
};
template<>
struct PrimitiveConversionTraits<uint16_t> : PrimitiveConversionTraits_smallInt {
};
template<>
struct PrimitiveConversionTraits<int32_t> : PrimitiveConversionTraits_smallInt {
};
template<>
struct PrimitiveConversionTraits<uint32_t> : PrimitiveConversionTraits_smallInt {
};

template<>
struct PrimitiveConversionTraits<int64_t> {
  typedef int64_t jstype;
  typedef int64_t intermediateType;
  static inline bool converter(JSContext* cx, JS::Value v, jstype* retval) {
    return JS::ToInt64(cx, v, retval);
  }
};

template<>
struct PrimitiveConversionTraits<uint64_t> {
  typedef uint64_t jstype;
  typedef uint64_t intermediateType;
  static inline bool converter(JSContext* cx, JS::Value v, jstype* retval) {
    return JS::ToUint64(cx, v, retval);
  }
};

template<>
struct PrimitiveConversionTraits<bool> {
  typedef JSBool jstype;
  typedef bool intermediateType;
  static inline bool converter(JSContext* , JS::Value v, jstype* retval) {
    *retval = JS::ToBoolean(v);
    return true;
  }
};

struct PrimitiveConversionTraits_float {
  typedef double jstype;
  typedef double intermediateType;
  static inline bool converter(JSContext* cx, JS::Value v, jstype* retval) {
    return JS::ToNumber(cx, v, retval);
  }
};
template<>
struct PrimitiveConversionTraits<float> : PrimitiveConversionTraits_float {
};
template<>
struct PrimitiveConversionTraits<double> : PrimitiveConversionTraits_float {
};

template<typename T>
bool ValueToPrimitive(JSContext* cx, JS::Value v, T* retval)
{
  typename PrimitiveConversionTraits<T>::jstype t;
  if (!PrimitiveConversionTraits<T>::converter(cx, v, &t))
    return false;

  *retval =
    static_cast<typename PrimitiveConversionTraits<T>::intermediateType>(t);
  return true;
}

} 
} 

#endif 
