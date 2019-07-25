






#ifndef TESTING_GMOCK_MUTANT_H_
#define TESTING_GMOCK_MUTANT_H_
































































































#include "base/memory/linked_ptr.h"
#include "base/tuple.h"  

namespace testing {


template <typename R, typename T, typename Method>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple0& p,
                          const Tuple0& c) {
  return (obj->*method)();
}
template <typename R, typename Function>
inline R DispatchToFunction(Function function,
                            const Tuple0& p,
                            const Tuple0& c) {
  return (*function)();
}


template <typename R, typename T, typename Method, typename C1>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple0& p,
                          const Tuple1<C1>& c) {
  return (obj->*method)(c.a);
}
template <typename R, typename Function, typename C1>
inline R DispatchToFunction(Function function,
                            const Tuple0& p,
                            const Tuple1<C1>& c) {
  return (*function)(c.a);
}


template <typename R, typename T, typename Method, typename C1, typename C2>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple0& p,
                          const Tuple2<C1, C2>& c) {
  return (obj->*method)(c.a, c.b);
}
template <typename R, typename Function, typename C1, typename C2>
inline R DispatchToFunction(Function function,
                            const Tuple0& p,
                            const Tuple2<C1, C2>& c) {
  return (*function)(c.a, c.b);
}


template <typename R, typename T, typename Method, typename C1, typename C2,
          typename C3>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple0& p,
                          const Tuple3<C1, C2, C3>& c) {
  return (obj->*method)(c.a, c.b, c.c);
}
template <typename R, typename Function, typename C1, typename C2, typename C3>
inline R DispatchToFunction(Function function,
                            const Tuple0& p,
                            const Tuple3<C1, C2, C3>& c) {
  return (*function)(c.a, c.b, c.c);
}


template <typename R, typename T, typename Method, typename C1, typename C2,
          typename C3, typename C4>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple0& p,
                          const Tuple4<C1, C2, C3, C4>& c) {
  return (obj->*method)(c.a, c.b, c.c, c.d);
}
template <typename R, typename Function, typename C1, typename C2, typename C3,
          typename C4>
inline R DispatchToFunction(Function function,
                            const Tuple0& p,
                            const Tuple4<C1, C2, C3, C4>& c) {
  return (*function)(c.a, c.b, c.c, c.d);
}


template <typename R, typename T, typename Method, typename C1, typename C2,
          typename C3, typename C4, typename C5>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple0& p,
                          const Tuple5<C1, C2, C3, C4, C5>& c) {
  return (obj->*method)(c.a, c.b, c.c, c.d, c.e);
}
template <typename R, typename Function, typename C1, typename C2, typename C3,
          typename C4, typename C5>
inline R DispatchToFunction(Function function,
                            const Tuple0& p,
                            const Tuple5<C1, C2, C3, C4, C5>& c) {
  return (*function)(c.a, c.b, c.c, c.d, c.e);
}


template <typename R, typename T, typename Method, typename C1, typename C2,
          typename C3, typename C4, typename C5, typename C6>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple0& p,
                          const Tuple6<C1, C2, C3, C4, C5, C6>& c) {
  return (obj->*method)(c.a, c.b, c.c, c.d, c.e, c.f);
}
template <typename R, typename Function, typename C1, typename C2, typename C3,
          typename C4, typename C5, typename C6>
inline R DispatchToFunction(Function function,
                            const Tuple0& p,
                            const Tuple6<C1, C2, C3, C4, C5, C6>& c) {
  return (*function)(c.a, c.b, c.c, c.d, c.e, c.f);
}


template <typename R, typename T, typename Method, typename P1>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple1<P1>& p,
                          const Tuple0& c) {
  return (obj->*method)(p.a);
}
template <typename R, typename Function, typename P1>
inline R DispatchToFunction(Function function,
                            const Tuple1<P1>& p,
                            const Tuple0& c) {
  return (*function)(p.a);
}


template <typename R, typename T, typename Method, typename P1, typename C1>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple1<P1>& p,
                          const Tuple1<C1>& c) {
  return (obj->*method)(p.a, c.a);
}
template <typename R, typename Function, typename P1, typename C1>
inline R DispatchToFunction(Function function,
                            const Tuple1<P1>& p,
                            const Tuple1<C1>& c) {
  return (*function)(p.a, c.a);
}


template <typename R, typename T, typename Method, typename P1, typename C1,
          typename C2>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple1<P1>& p,
                          const Tuple2<C1, C2>& c) {
  return (obj->*method)(p.a, c.a, c.b);
}
template <typename R, typename Function, typename P1, typename C1, typename C2>
inline R DispatchToFunction(Function function,
                            const Tuple1<P1>& p,
                            const Tuple2<C1, C2>& c) {
  return (*function)(p.a, c.a, c.b);
}


template <typename R, typename T, typename Method, typename P1, typename C1,
          typename C2, typename C3>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple1<P1>& p,
                          const Tuple3<C1, C2, C3>& c) {
  return (obj->*method)(p.a, c.a, c.b, c.c);
}
template <typename R, typename Function, typename P1, typename C1, typename C2,
          typename C3>
inline R DispatchToFunction(Function function,
                            const Tuple1<P1>& p,
                            const Tuple3<C1, C2, C3>& c) {
  return (*function)(p.a, c.a, c.b, c.c);
}


template <typename R, typename T, typename Method, typename P1, typename C1,
          typename C2, typename C3, typename C4>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple1<P1>& p,
                          const Tuple4<C1, C2, C3, C4>& c) {
  return (obj->*method)(p.a, c.a, c.b, c.c, c.d);
}
template <typename R, typename Function, typename P1, typename C1, typename C2,
          typename C3, typename C4>
inline R DispatchToFunction(Function function,
                            const Tuple1<P1>& p,
                            const Tuple4<C1, C2, C3, C4>& c) {
  return (*function)(p.a, c.a, c.b, c.c, c.d);
}


template <typename R, typename T, typename Method, typename P1, typename C1,
          typename C2, typename C3, typename C4, typename C5>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple1<P1>& p,
                          const Tuple5<C1, C2, C3, C4, C5>& c) {
  return (obj->*method)(p.a, c.a, c.b, c.c, c.d, c.e);
}
template <typename R, typename Function, typename P1, typename C1, typename C2,
          typename C3, typename C4, typename C5>
inline R DispatchToFunction(Function function,
                            const Tuple1<P1>& p,
                            const Tuple5<C1, C2, C3, C4, C5>& c) {
  return (*function)(p.a, c.a, c.b, c.c, c.d, c.e);
}


template <typename R, typename T, typename Method, typename P1, typename C1,
          typename C2, typename C3, typename C4, typename C5, typename C6>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple1<P1>& p,
                          const Tuple6<C1, C2, C3, C4, C5, C6>& c) {
  return (obj->*method)(p.a, c.a, c.b, c.c, c.d, c.e, c.f);
}
template <typename R, typename Function, typename P1, typename C1, typename C2,
          typename C3, typename C4, typename C5, typename C6>
inline R DispatchToFunction(Function function,
                            const Tuple1<P1>& p,
                            const Tuple6<C1, C2, C3, C4, C5, C6>& c) {
  return (*function)(p.a, c.a, c.b, c.c, c.d, c.e, c.f);
}


template <typename R, typename T, typename Method, typename P1, typename P2>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple2<P1, P2>& p,
                          const Tuple0& c) {
  return (obj->*method)(p.a, p.b);
}
template <typename R, typename Function, typename P1, typename P2>
inline R DispatchToFunction(Function function,
                            const Tuple2<P1, P2>& p,
                            const Tuple0& c) {
  return (*function)(p.a, p.b);
}


template <typename R, typename T, typename Method, typename P1, typename P2,
          typename C1>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple2<P1, P2>& p,
                          const Tuple1<C1>& c) {
  return (obj->*method)(p.a, p.b, c.a);
}
template <typename R, typename Function, typename P1, typename P2, typename C1>
inline R DispatchToFunction(Function function,
                            const Tuple2<P1, P2>& p,
                            const Tuple1<C1>& c) {
  return (*function)(p.a, p.b, c.a);
}


template <typename R, typename T, typename Method, typename P1, typename P2,
          typename C1, typename C2>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple2<P1, P2>& p,
                          const Tuple2<C1, C2>& c) {
  return (obj->*method)(p.a, p.b, c.a, c.b);
}
template <typename R, typename Function, typename P1, typename P2, typename C1,
          typename C2>
inline R DispatchToFunction(Function function,
                            const Tuple2<P1, P2>& p,
                            const Tuple2<C1, C2>& c) {
  return (*function)(p.a, p.b, c.a, c.b);
}


template <typename R, typename T, typename Method, typename P1, typename P2,
          typename C1, typename C2, typename C3>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple2<P1, P2>& p,
                          const Tuple3<C1, C2, C3>& c) {
  return (obj->*method)(p.a, p.b, c.a, c.b, c.c);
}
template <typename R, typename Function, typename P1, typename P2, typename C1,
          typename C2, typename C3>
inline R DispatchToFunction(Function function,
                            const Tuple2<P1, P2>& p,
                            const Tuple3<C1, C2, C3>& c) {
  return (*function)(p.a, p.b, c.a, c.b, c.c);
}


template <typename R, typename T, typename Method, typename P1, typename P2,
          typename C1, typename C2, typename C3, typename C4>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple2<P1, P2>& p,
                          const Tuple4<C1, C2, C3, C4>& c) {
  return (obj->*method)(p.a, p.b, c.a, c.b, c.c, c.d);
}
template <typename R, typename Function, typename P1, typename P2, typename C1,
          typename C2, typename C3, typename C4>
inline R DispatchToFunction(Function function,
                            const Tuple2<P1, P2>& p,
                            const Tuple4<C1, C2, C3, C4>& c) {
  return (*function)(p.a, p.b, c.a, c.b, c.c, c.d);
}


template <typename R, typename T, typename Method, typename P1, typename P2,
          typename C1, typename C2, typename C3, typename C4, typename C5>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple2<P1, P2>& p,
                          const Tuple5<C1, C2, C3, C4, C5>& c) {
  return (obj->*method)(p.a, p.b, c.a, c.b, c.c, c.d, c.e);
}
template <typename R, typename Function, typename P1, typename P2, typename C1,
          typename C2, typename C3, typename C4, typename C5>
inline R DispatchToFunction(Function function,
                            const Tuple2<P1, P2>& p,
                            const Tuple5<C1, C2, C3, C4, C5>& c) {
  return (*function)(p.a, p.b, c.a, c.b, c.c, c.d, c.e);
}


template <typename R, typename T, typename Method, typename P1, typename P2,
          typename C1, typename C2, typename C3, typename C4, typename C5,
          typename C6>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple2<P1, P2>& p,
                          const Tuple6<C1, C2, C3, C4, C5, C6>& c) {
  return (obj->*method)(p.a, p.b, c.a, c.b, c.c, c.d, c.e, c.f);
}
template <typename R, typename Function, typename P1, typename P2, typename C1,
          typename C2, typename C3, typename C4, typename C5, typename C6>
inline R DispatchToFunction(Function function,
                            const Tuple2<P1, P2>& p,
                            const Tuple6<C1, C2, C3, C4, C5, C6>& c) {
  return (*function)(p.a, p.b, c.a, c.b, c.c, c.d, c.e, c.f);
}


template <typename R, typename T, typename Method, typename P1, typename P2,
          typename P3>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple3<P1, P2, P3>& p,
                          const Tuple0& c) {
  return (obj->*method)(p.a, p.b, p.c);
}
template <typename R, typename Function, typename P1, typename P2, typename P3>
inline R DispatchToFunction(Function function,
                            const Tuple3<P1, P2, P3>& p,
                            const Tuple0& c) {
  return (*function)(p.a, p.b, p.c);
}


template <typename R, typename T, typename Method, typename P1, typename P2,
          typename P3, typename C1>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple3<P1, P2, P3>& p,
                          const Tuple1<C1>& c) {
  return (obj->*method)(p.a, p.b, p.c, c.a);
}
template <typename R, typename Function, typename P1, typename P2, typename P3,
          typename C1>
inline R DispatchToFunction(Function function,
                            const Tuple3<P1, P2, P3>& p,
                            const Tuple1<C1>& c) {
  return (*function)(p.a, p.b, p.c, c.a);
}


template <typename R, typename T, typename Method, typename P1, typename P2,
          typename P3, typename C1, typename C2>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple3<P1, P2, P3>& p,
                          const Tuple2<C1, C2>& c) {
  return (obj->*method)(p.a, p.b, p.c, c.a, c.b);
}
template <typename R, typename Function, typename P1, typename P2, typename P3,
          typename C1, typename C2>
inline R DispatchToFunction(Function function,
                            const Tuple3<P1, P2, P3>& p,
                            const Tuple2<C1, C2>& c) {
  return (*function)(p.a, p.b, p.c, c.a, c.b);
}


template <typename R, typename T, typename Method, typename P1, typename P2,
          typename P3, typename C1, typename C2, typename C3>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple3<P1, P2, P3>& p,
                          const Tuple3<C1, C2, C3>& c) {
  return (obj->*method)(p.a, p.b, p.c, c.a, c.b, c.c);
}
template <typename R, typename Function, typename P1, typename P2, typename P3,
          typename C1, typename C2, typename C3>
inline R DispatchToFunction(Function function,
                            const Tuple3<P1, P2, P3>& p,
                            const Tuple3<C1, C2, C3>& c) {
  return (*function)(p.a, p.b, p.c, c.a, c.b, c.c);
}


template <typename R, typename T, typename Method, typename P1, typename P2,
          typename P3, typename C1, typename C2, typename C3, typename C4>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple3<P1, P2, P3>& p,
                          const Tuple4<C1, C2, C3, C4>& c) {
  return (obj->*method)(p.a, p.b, p.c, c.a, c.b, c.c, c.d);
}
template <typename R, typename Function, typename P1, typename P2, typename P3,
          typename C1, typename C2, typename C3, typename C4>
inline R DispatchToFunction(Function function,
                            const Tuple3<P1, P2, P3>& p,
                            const Tuple4<C1, C2, C3, C4>& c) {
  return (*function)(p.a, p.b, p.c, c.a, c.b, c.c, c.d);
}


template <typename R, typename T, typename Method, typename P1, typename P2,
          typename P3, typename C1, typename C2, typename C3, typename C4,
          typename C5>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple3<P1, P2, P3>& p,
                          const Tuple5<C1, C2, C3, C4, C5>& c) {
  return (obj->*method)(p.a, p.b, p.c, c.a, c.b, c.c, c.d, c.e);
}
template <typename R, typename Function, typename P1, typename P2, typename P3,
          typename C1, typename C2, typename C3, typename C4, typename C5>
inline R DispatchToFunction(Function function,
                            const Tuple3<P1, P2, P3>& p,
                            const Tuple5<C1, C2, C3, C4, C5>& c) {
  return (*function)(p.a, p.b, p.c, c.a, c.b, c.c, c.d, c.e);
}


template <typename R, typename T, typename Method, typename P1, typename P2,
          typename P3, typename C1, typename C2, typename C3, typename C4,
          typename C5, typename C6>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple3<P1, P2, P3>& p,
                          const Tuple6<C1, C2, C3, C4, C5, C6>& c) {
  return (obj->*method)(p.a, p.b, p.c, c.a, c.b, c.c, c.d, c.e, c.f);
}
template <typename R, typename Function, typename P1, typename P2, typename P3,
          typename C1, typename C2, typename C3, typename C4, typename C5,
          typename C6>
inline R DispatchToFunction(Function function,
                            const Tuple3<P1, P2, P3>& p,
                            const Tuple6<C1, C2, C3, C4, C5, C6>& c) {
  return (*function)(p.a, p.b, p.c, c.a, c.b, c.c, c.d, c.e, c.f);
}


template <typename R, typename T, typename Method, typename P1, typename P2,
          typename P3, typename P4>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple4<P1, P2, P3, P4>& p,
                          const Tuple0& c) {
  return (obj->*method)(p.a, p.b, p.c, p.d);
}
template <typename R, typename Function, typename P1, typename P2, typename P3,
          typename P4>
inline R DispatchToFunction(Function function,
                            const Tuple4<P1, P2, P3, P4>& p,
                            const Tuple0& c) {
  return (*function)(p.a, p.b, p.c, p.d);
}


template <typename R, typename T, typename Method, typename P1, typename P2,
          typename P3, typename P4, typename C1>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple4<P1, P2, P3, P4>& p,
                          const Tuple1<C1>& c) {
  return (obj->*method)(p.a, p.b, p.c, p.d, c.a);
}
template <typename R, typename Function, typename P1, typename P2, typename P3,
          typename P4, typename C1>
inline R DispatchToFunction(Function function,
                            const Tuple4<P1, P2, P3, P4>& p,
                            const Tuple1<C1>& c) {
  return (*function)(p.a, p.b, p.c, p.d, c.a);
}


template <typename R, typename T, typename Method, typename P1, typename P2,
          typename P3, typename P4, typename C1, typename C2>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple4<P1, P2, P3, P4>& p,
                          const Tuple2<C1, C2>& c) {
  return (obj->*method)(p.a, p.b, p.c, p.d, c.a, c.b);
}
template <typename R, typename Function, typename P1, typename P2, typename P3,
          typename P4, typename C1, typename C2>
inline R DispatchToFunction(Function function,
                            const Tuple4<P1, P2, P3, P4>& p,
                            const Tuple2<C1, C2>& c) {
  return (*function)(p.a, p.b, p.c, p.d, c.a, c.b);
}


template <typename R, typename T, typename Method, typename P1, typename P2,
          typename P3, typename P4, typename C1, typename C2, typename C3>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple4<P1, P2, P3, P4>& p,
                          const Tuple3<C1, C2, C3>& c) {
  return (obj->*method)(p.a, p.b, p.c, p.d, c.a, c.b, c.c);
}
template <typename R, typename Function, typename P1, typename P2, typename P3,
          typename P4, typename C1, typename C2, typename C3>
inline R DispatchToFunction(Function function,
                            const Tuple4<P1, P2, P3, P4>& p,
                            const Tuple3<C1, C2, C3>& c) {
  return (*function)(p.a, p.b, p.c, p.d, c.a, c.b, c.c);
}


template <typename R, typename T, typename Method, typename P1, typename P2,
          typename P3, typename P4, typename C1, typename C2, typename C3,
          typename C4>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple4<P1, P2, P3, P4>& p,
                          const Tuple4<C1, C2, C3, C4>& c) {
  return (obj->*method)(p.a, p.b, p.c, p.d, c.a, c.b, c.c, c.d);
}
template <typename R, typename Function, typename P1, typename P2, typename P3,
          typename P4, typename C1, typename C2, typename C3, typename C4>
inline R DispatchToFunction(Function function,
                            const Tuple4<P1, P2, P3, P4>& p,
                            const Tuple4<C1, C2, C3, C4>& c) {
  return (*function)(p.a, p.b, p.c, p.d, c.a, c.b, c.c, c.d);
}


template <typename R, typename T, typename Method, typename P1, typename P2,
          typename P3, typename P4, typename C1, typename C2, typename C3,
          typename C4, typename C5>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple4<P1, P2, P3, P4>& p,
                          const Tuple5<C1, C2, C3, C4, C5>& c) {
  return (obj->*method)(p.a, p.b, p.c, p.d, c.a, c.b, c.c, c.d, c.e);
}
template <typename R, typename Function, typename P1, typename P2, typename P3,
          typename P4, typename C1, typename C2, typename C3, typename C4,
          typename C5>
inline R DispatchToFunction(Function function,
                            const Tuple4<P1, P2, P3, P4>& p,
                            const Tuple5<C1, C2, C3, C4, C5>& c) {
  return (*function)(p.a, p.b, p.c, p.d, c.a, c.b, c.c, c.d, c.e);
}


template <typename R, typename T, typename Method, typename P1, typename P2,
          typename P3, typename P4, typename C1, typename C2, typename C3,
          typename C4, typename C5, typename C6>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple4<P1, P2, P3, P4>& p,
                          const Tuple6<C1, C2, C3, C4, C5, C6>& c) {
  return (obj->*method)(p.a, p.b, p.c, p.d, c.a, c.b, c.c, c.d, c.e, c.f);
}
template <typename R, typename Function, typename P1, typename P2, typename P3,
          typename P4, typename C1, typename C2, typename C3, typename C4,
          typename C5, typename C6>
inline R DispatchToFunction(Function function,
                            const Tuple4<P1, P2, P3, P4>& p,
                            const Tuple6<C1, C2, C3, C4, C5, C6>& c) {
  return (*function)(p.a, p.b, p.c, p.d, c.a, c.b, c.c, c.d, c.e, c.f);
}


template <typename R, typename T, typename Method, typename P1, typename P2,
          typename P3, typename P4, typename P5>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple5<P1, P2, P3, P4, P5>& p,
                          const Tuple0& c) {
  return (obj->*method)(p.a, p.b, p.c, p.d, p.e);
}
template <typename R, typename Function, typename P1, typename P2, typename P3,
          typename P4, typename P5>
inline R DispatchToFunction(Function function,
                            const Tuple5<P1, P2, P3, P4, P5>& p,
                            const Tuple0& c) {
  return (*function)(p.a, p.b, p.c, p.d, p.e);
}


template <typename R, typename T, typename Method, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename C1>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple5<P1, P2, P3, P4, P5>& p,
                          const Tuple1<C1>& c) {
  return (obj->*method)(p.a, p.b, p.c, p.d, p.e, c.a);
}
template <typename R, typename Function, typename P1, typename P2, typename P3,
          typename P4, typename P5, typename C1>
inline R DispatchToFunction(Function function,
                            const Tuple5<P1, P2, P3, P4, P5>& p,
                            const Tuple1<C1>& c) {
  return (*function)(p.a, p.b, p.c, p.d, p.e, c.a);
}


template <typename R, typename T, typename Method, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename C1, typename C2>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple5<P1, P2, P3, P4, P5>& p,
                          const Tuple2<C1, C2>& c) {
  return (obj->*method)(p.a, p.b, p.c, p.d, p.e, c.a, c.b);
}
template <typename R, typename Function, typename P1, typename P2, typename P3,
          typename P4, typename P5, typename C1, typename C2>
inline R DispatchToFunction(Function function,
                            const Tuple5<P1, P2, P3, P4, P5>& p,
                            const Tuple2<C1, C2>& c) {
  return (*function)(p.a, p.b, p.c, p.d, p.e, c.a, c.b);
}


template <typename R, typename T, typename Method, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename C1, typename C2,
          typename C3>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple5<P1, P2, P3, P4, P5>& p,
                          const Tuple3<C1, C2, C3>& c) {
  return (obj->*method)(p.a, p.b, p.c, p.d, p.e, c.a, c.b, c.c);
}
template <typename R, typename Function, typename P1, typename P2, typename P3,
          typename P4, typename P5, typename C1, typename C2, typename C3>
inline R DispatchToFunction(Function function,
                            const Tuple5<P1, P2, P3, P4, P5>& p,
                            const Tuple3<C1, C2, C3>& c) {
  return (*function)(p.a, p.b, p.c, p.d, p.e, c.a, c.b, c.c);
}


template <typename R, typename T, typename Method, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename C1, typename C2,
          typename C3, typename C4>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple5<P1, P2, P3, P4, P5>& p,
                          const Tuple4<C1, C2, C3, C4>& c) {
  return (obj->*method)(p.a, p.b, p.c, p.d, p.e, c.a, c.b, c.c, c.d);
}
template <typename R, typename Function, typename P1, typename P2, typename P3,
          typename P4, typename P5, typename C1, typename C2, typename C3,
          typename C4>
inline R DispatchToFunction(Function function,
                            const Tuple5<P1, P2, P3, P4, P5>& p,
                            const Tuple4<C1, C2, C3, C4>& c) {
  return (*function)(p.a, p.b, p.c, p.d, p.e, c.a, c.b, c.c, c.d);
}


template <typename R, typename T, typename Method, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename C1, typename C2,
          typename C3, typename C4, typename C5>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple5<P1, P2, P3, P4, P5>& p,
                          const Tuple5<C1, C2, C3, C4, C5>& c) {
  return (obj->*method)(p.a, p.b, p.c, p.d, p.e, c.a, c.b, c.c, c.d, c.e);
}
template <typename R, typename Function, typename P1, typename P2, typename P3,
          typename P4, typename P5, typename C1, typename C2, typename C3,
          typename C4, typename C5>
inline R DispatchToFunction(Function function,
                            const Tuple5<P1, P2, P3, P4, P5>& p,
                            const Tuple5<C1, C2, C3, C4, C5>& c) {
  return (*function)(p.a, p.b, p.c, p.d, p.e, c.a, c.b, c.c, c.d, c.e);
}


template <typename R, typename T, typename Method, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename C1, typename C2,
          typename C3, typename C4, typename C5, typename C6>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple5<P1, P2, P3, P4, P5>& p,
                          const Tuple6<C1, C2, C3, C4, C5, C6>& c) {
  return (obj->*method)(p.a, p.b, p.c, p.d, p.e, c.a, c.b, c.c, c.d, c.e, c.f);
}
template <typename R, typename Function, typename P1, typename P2, typename P3,
          typename P4, typename P5, typename C1, typename C2, typename C3,
          typename C4, typename C5, typename C6>
inline R DispatchToFunction(Function function,
                            const Tuple5<P1, P2, P3, P4, P5>& p,
                            const Tuple6<C1, C2, C3, C4, C5, C6>& c) {
  return (*function)(p.a, p.b, p.c, p.d, p.e, c.a, c.b, c.c, c.d, c.e, c.f);
}


template <typename R, typename T, typename Method, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename P6>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple6<P1, P2, P3, P4, P5, P6>& p,
                          const Tuple0& c) {
  return (obj->*method)(p.a, p.b, p.c, p.d, p.e, p.f);
}
template <typename R, typename Function, typename P1, typename P2, typename P3,
          typename P4, typename P5, typename P6>
inline R DispatchToFunction(Function function,
                            const Tuple6<P1, P2, P3, P4, P5, P6>& p,
                            const Tuple0& c) {
  return (*function)(p.a, p.b, p.c, p.d, p.e, p.f);
}


template <typename R, typename T, typename Method, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename P6, typename C1>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple6<P1, P2, P3, P4, P5, P6>& p,
                          const Tuple1<C1>& c) {
  return (obj->*method)(p.a, p.b, p.c, p.d, p.e, p.f, c.a);
}
template <typename R, typename Function, typename P1, typename P2, typename P3,
          typename P4, typename P5, typename P6, typename C1>
inline R DispatchToFunction(Function function,
                            const Tuple6<P1, P2, P3, P4, P5, P6>& p,
                            const Tuple1<C1>& c) {
  return (*function)(p.a, p.b, p.c, p.d, p.e, p.f, c.a);
}


template <typename R, typename T, typename Method, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename P6, typename C1,
          typename C2>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple6<P1, P2, P3, P4, P5, P6>& p,
                          const Tuple2<C1, C2>& c) {
  return (obj->*method)(p.a, p.b, p.c, p.d, p.e, p.f, c.a, c.b);
}
template <typename R, typename Function, typename P1, typename P2, typename P3,
          typename P4, typename P5, typename P6, typename C1, typename C2>
inline R DispatchToFunction(Function function,
                            const Tuple6<P1, P2, P3, P4, P5, P6>& p,
                            const Tuple2<C1, C2>& c) {
  return (*function)(p.a, p.b, p.c, p.d, p.e, p.f, c.a, c.b);
}


template <typename R, typename T, typename Method, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename P6, typename C1,
          typename C2, typename C3>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple6<P1, P2, P3, P4, P5, P6>& p,
                          const Tuple3<C1, C2, C3>& c) {
  return (obj->*method)(p.a, p.b, p.c, p.d, p.e, p.f, c.a, c.b, c.c);
}
template <typename R, typename Function, typename P1, typename P2, typename P3,
          typename P4, typename P5, typename P6, typename C1, typename C2,
          typename C3>
inline R DispatchToFunction(Function function,
                            const Tuple6<P1, P2, P3, P4, P5, P6>& p,
                            const Tuple3<C1, C2, C3>& c) {
  return (*function)(p.a, p.b, p.c, p.d, p.e, p.f, c.a, c.b, c.c);
}


template <typename R, typename T, typename Method, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename P6, typename C1,
          typename C2, typename C3, typename C4>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple6<P1, P2, P3, P4, P5, P6>& p,
                          const Tuple4<C1, C2, C3, C4>& c) {
  return (obj->*method)(p.a, p.b, p.c, p.d, p.e, p.f, c.a, c.b, c.c, c.d);
}
template <typename R, typename Function, typename P1, typename P2, typename P3,
          typename P4, typename P5, typename P6, typename C1, typename C2,
          typename C3, typename C4>
inline R DispatchToFunction(Function function,
                            const Tuple6<P1, P2, P3, P4, P5, P6>& p,
                            const Tuple4<C1, C2, C3, C4>& c) {
  return (*function)(p.a, p.b, p.c, p.d, p.e, p.f, c.a, c.b, c.c, c.d);
}


template <typename R, typename T, typename Method, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename P6, typename C1,
          typename C2, typename C3, typename C4, typename C5>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple6<P1, P2, P3, P4, P5, P6>& p,
                          const Tuple5<C1, C2, C3, C4, C5>& c) {
  return (obj->*method)(p.a, p.b, p.c, p.d, p.e, p.f, c.a, c.b, c.c, c.d, c.e);
}
template <typename R, typename Function, typename P1, typename P2, typename P3,
          typename P4, typename P5, typename P6, typename C1, typename C2,
          typename C3, typename C4, typename C5>
inline R DispatchToFunction(Function function,
                            const Tuple6<P1, P2, P3, P4, P5, P6>& p,
                            const Tuple5<C1, C2, C3, C4, C5>& c) {
  return (*function)(p.a, p.b, p.c, p.d, p.e, p.f, c.a, c.b, c.c, c.d, c.e);
}


template <typename R, typename T, typename Method, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename P6, typename C1,
          typename C2, typename C3, typename C4, typename C5, typename C6>
inline R DispatchToMethod(T* obj, Method method,
                          const Tuple6<P1, P2, P3, P4, P5, P6>& p,
                          const Tuple6<C1, C2, C3, C4, C5, C6>& c) {
  return (obj->*method)(p.a, p.b, p.c, p.d, p.e, p.f, c.a, c.b, c.c, c.d, c.e, c.f);
}
template <typename R, typename Function, typename P1, typename P2, typename P3,
          typename P4, typename P5, typename P6, typename C1, typename C2,
          typename C3, typename C4, typename C5, typename C6>
inline R DispatchToFunction(Function function,
                            const Tuple6<P1, P2, P3, P4, P5, P6>& p,
                            const Tuple6<C1, C2, C3, C4, C5, C6>& c) {
  return (*function)(p.a, p.b, p.c, p.d, p.e, p.f, c.a, c.b, c.c, c.d, c.e, c.f);
}



template <typename R, typename Params>
class MutantRunner {
 public:
  virtual R RunWithParams(const Params& params) = 0;
  virtual ~MutantRunner() {}
};




template <typename R, typename T, typename Method,
          typename PreBound, typename Params>
class Mutant : public MutantRunner<R, Params> {
 public:
  Mutant(T* obj, Method method, const PreBound& pb)
      : obj_(obj), method_(method), pb_(pb) {
  }

  
  virtual R RunWithParams(const Params& params) {
    return DispatchToMethod<R>(this->obj_, this->method_, pb_, params);
  }

  T* obj_;
  Method method_;
  PreBound pb_;
};

template <typename R, typename Function, typename PreBound, typename Params>
class MutantFunction : public MutantRunner<R, Params> {
 public:
  MutantFunction(Function function, const PreBound& pb)
      : function_(function), pb_(pb) {
  }

  
  virtual R RunWithParams(const Params& params) {
    return DispatchToFunction<R>(function_, pb_, params);
  }

  Function function_;
  PreBound pb_;
};

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING



template <typename R, typename T, typename Method,
          typename PreBound, typename Params>
class MutantLateObjectBind : public MutantRunner<R, Params> {
 public:
  MutantLateObjectBind(T** obj, Method method, const PreBound& pb)
      : obj_(obj), method_(method), pb_(pb) {
  }

  
  virtual R RunWithParams(const Params& params) {
    EXPECT_THAT(*this->obj_, testing::NotNull());
    if (NULL == *this->obj_)
      return R();
    return DispatchToMethod<R>( *this->obj_, this->method_, pb_, params);
  }

  T** obj_;
  Method method_;
  PreBound pb_;
};
#endif



template <typename R, typename Params>
struct MutantFunctor {
  explicit MutantFunctor(MutantRunner<R, Params>*  cb) : impl_(cb) {
  }

  ~MutantFunctor() {
  }

  inline R operator()() {
    return impl_->RunWithParams(Tuple0());
  }

  template <typename Arg1>
  inline R operator()(const Arg1& a) {
    return impl_->RunWithParams(Params(a));
  }

  template <typename Arg1, typename Arg2>
  inline R operator()(const Arg1& a, const Arg2& b) {
    return impl_->RunWithParams(Params(a, b));
  }

  template <typename Arg1, typename Arg2, typename Arg3>
  inline R operator()(const Arg1& a, const Arg2& b, const Arg3& c) {
    return impl_->RunWithParams(Params(a, b, c));
  }

  template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
  inline R operator()(const Arg1& a, const Arg2& b, const Arg3& c,
                         const Arg4& d) {
    return impl_->RunWithParams(Params(a, b, c, d));
  }

 private:
  
  
  MutantFunctor();
  linked_ptr<MutantRunner<R, Params> > impl_;
};


template <typename R, typename T, typename U>
inline MutantFunctor<R, Tuple0>
CreateFunctor(T* obj, R (U::*method)()) {
  MutantRunner<R, Tuple0>* t =
      new Mutant<R, T, R (U::*)(),
                 Tuple0, Tuple0>
          (obj, method, MakeTuple());
  return MutantFunctor<R, Tuple0>(t);
}

template <typename R>
inline MutantFunctor<R, Tuple0>
CreateFunctor(R (*function)()) {
  MutantRunner<R, Tuple0>* t =
      new MutantFunction<R, R (*)(),
                         Tuple0, Tuple0>
          (function, MakeTuple());
  return MutantFunctor<R, Tuple0>(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U>
inline MutantFunctor<R, Tuple0>
CreateFunctor(T** obj, R (U::*method)()) {
  MutantRunner<R, Tuple0>* t =
      new MutantLateObjectBind<R, T, R (U::*)(),
                               Tuple0, Tuple0>
          (obj, method, MakeTuple());
  return MutantFunctor<R, Tuple0>(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U>
inline MutantFunctor<R, Tuple0>
CreateFunctor(T* obj, R (__stdcall U::*method)()) {
  MutantRunner<R, Tuple0>* t =
      new Mutant<R, T, R (__stdcall U::*)(),
                 Tuple0, Tuple0>
          (obj, method, MakeTuple());
  return MutantFunctor<R, Tuple0>(t);
}

template <typename R>
inline MutantFunctor<R, Tuple0>
CreateFunctor(R (__stdcall *function)()) {
  MutantRunner<R, Tuple0>* t =
      new MutantFunction<R, R (__stdcall *)(),
                         Tuple0, Tuple0>
          (function, MakeTuple());
  return MutantFunctor<R, Tuple0>(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U>
inline MutantFunctor<R, Tuple0>
CreateFunctor(T** obj, R (__stdcall U::*method)()) {
  MutantRunner<R, Tuple0>* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(),
                               Tuple0, Tuple0>
          (obj, method, MakeTuple());
  return MutantFunctor<R, Tuple0>(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename A1>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(T* obj, R (U::*method)(A1)) {
  MutantRunner<R, Tuple1<A1> >* t =
      new Mutant<R, T, R (U::*)(A1),
                 Tuple0, Tuple1<A1> >
          (obj, method, MakeTuple());
  return MutantFunctor<R, Tuple1<A1> >(t);
}

template <typename R, typename A1>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(R (*function)(A1)) {
  MutantRunner<R, Tuple1<A1> >* t =
      new MutantFunction<R, R (*)(A1),
                         Tuple0, Tuple1<A1> >
          (function, MakeTuple());
  return MutantFunctor<R, Tuple1<A1> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename A1>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(T** obj, R (U::*method)(A1)) {
  MutantRunner<R, Tuple1<A1> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(A1),
                               Tuple0, Tuple1<A1> >
          (obj, method, MakeTuple());
  return MutantFunctor<R, Tuple1<A1> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename A1>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(T* obj, R (__stdcall U::*method)(A1)) {
  MutantRunner<R, Tuple1<A1> >* t =
      new Mutant<R, T, R (__stdcall U::*)(A1),
                 Tuple0, Tuple1<A1> >
          (obj, method, MakeTuple());
  return MutantFunctor<R, Tuple1<A1> >(t);
}

template <typename R, typename A1>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(R (__stdcall *function)(A1)) {
  MutantRunner<R, Tuple1<A1> >* t =
      new MutantFunction<R, R (__stdcall *)(A1),
                         Tuple0, Tuple1<A1> >
          (function, MakeTuple());
  return MutantFunctor<R, Tuple1<A1> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename A1>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(T** obj, R (__stdcall U::*method)(A1)) {
  MutantRunner<R, Tuple1<A1> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(A1),
                               Tuple0, Tuple1<A1> >
          (obj, method, MakeTuple());
  return MutantFunctor<R, Tuple1<A1> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename A1, typename A2>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(T* obj, R (U::*method)(A1, A2)) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new Mutant<R, T, R (U::*)(A1, A2),
                 Tuple0, Tuple2<A1, A2> >
          (obj, method, MakeTuple());
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}

template <typename R, typename A1, typename A2>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(R (*function)(A1, A2)) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new MutantFunction<R, R (*)(A1, A2),
                         Tuple0, Tuple2<A1, A2> >
          (function, MakeTuple());
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename A1, typename A2>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(T** obj, R (U::*method)(A1, A2)) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(A1, A2),
                               Tuple0, Tuple2<A1, A2> >
          (obj, method, MakeTuple());
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename A1, typename A2>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(T* obj, R (__stdcall U::*method)(A1, A2)) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new Mutant<R, T, R (__stdcall U::*)(A1, A2),
                 Tuple0, Tuple2<A1, A2> >
          (obj, method, MakeTuple());
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}

template <typename R, typename A1, typename A2>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(R (__stdcall *function)(A1, A2)) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new MutantFunction<R, R (__stdcall *)(A1, A2),
                         Tuple0, Tuple2<A1, A2> >
          (function, MakeTuple());
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename A1, typename A2>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(T** obj, R (__stdcall U::*method)(A1, A2)) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(A1, A2),
                               Tuple0, Tuple2<A1, A2> >
          (obj, method, MakeTuple());
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename A1, typename A2,
          typename A3>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(T* obj, R (U::*method)(A1, A2, A3)) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new Mutant<R, T, R (U::*)(A1, A2, A3),
                 Tuple0, Tuple3<A1, A2, A3> >
          (obj, method, MakeTuple());
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}

template <typename R, typename A1, typename A2, typename A3>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(R (*function)(A1, A2, A3)) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new MutantFunction<R, R (*)(A1, A2, A3),
                         Tuple0, Tuple3<A1, A2, A3> >
          (function, MakeTuple());
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename A1, typename A2,
          typename A3>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(T** obj, R (U::*method)(A1, A2, A3)) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(A1, A2, A3),
                               Tuple0, Tuple3<A1, A2, A3> >
          (obj, method, MakeTuple());
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename A1, typename A2,
          typename A3>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(T* obj, R (__stdcall U::*method)(A1, A2, A3)) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new Mutant<R, T, R (__stdcall U::*)(A1, A2, A3),
                 Tuple0, Tuple3<A1, A2, A3> >
          (obj, method, MakeTuple());
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}

template <typename R, typename A1, typename A2, typename A3>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(R (__stdcall *function)(A1, A2, A3)) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new MutantFunction<R, R (__stdcall *)(A1, A2, A3),
                         Tuple0, Tuple3<A1, A2, A3> >
          (function, MakeTuple());
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename A1, typename A2,
          typename A3>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(T** obj, R (__stdcall U::*method)(A1, A2, A3)) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(A1, A2, A3),
                               Tuple0, Tuple3<A1, A2, A3> >
          (obj, method, MakeTuple());
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename A1, typename A2,
          typename A3, typename A4>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(T* obj, R (U::*method)(A1, A2, A3, A4)) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new Mutant<R, T, R (U::*)(A1, A2, A3, A4),
                 Tuple0, Tuple4<A1, A2, A3, A4> >
          (obj, method, MakeTuple());
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}

template <typename R, typename A1, typename A2, typename A3, typename A4>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(R (*function)(A1, A2, A3, A4)) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new MutantFunction<R, R (*)(A1, A2, A3, A4),
                         Tuple0, Tuple4<A1, A2, A3, A4> >
          (function, MakeTuple());
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename A1, typename A2,
          typename A3, typename A4>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(T** obj, R (U::*method)(A1, A2, A3, A4)) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(A1, A2, A3, A4),
                               Tuple0, Tuple4<A1, A2, A3, A4> >
          (obj, method, MakeTuple());
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename A1, typename A2,
          typename A3, typename A4>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(T* obj, R (__stdcall U::*method)(A1, A2, A3, A4)) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new Mutant<R, T, R (__stdcall U::*)(A1, A2, A3, A4),
                 Tuple0, Tuple4<A1, A2, A3, A4> >
          (obj, method, MakeTuple());
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}

template <typename R, typename A1, typename A2, typename A3, typename A4>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(R (__stdcall *function)(A1, A2, A3, A4)) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new MutantFunction<R, R (__stdcall *)(A1, A2, A3, A4),
                         Tuple0, Tuple4<A1, A2, A3, A4> >
          (function, MakeTuple());
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename A1, typename A2,
          typename A3, typename A4>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(T** obj, R (__stdcall U::*method)(A1, A2, A3, A4)) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(A1, A2, A3, A4),
                               Tuple0, Tuple4<A1, A2, A3, A4> >
          (obj, method, MakeTuple());
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename A1, typename A2,
          typename A3, typename A4, typename A5>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(T* obj, R (U::*method)(A1, A2, A3, A4, A5)) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new Mutant<R, T, R (U::*)(A1, A2, A3, A4, A5),
                 Tuple0, Tuple5<A1, A2, A3, A4, A5> >
          (obj, method, MakeTuple());
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}

template <typename R, typename A1, typename A2, typename A3, typename A4,
          typename A5>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(R (*function)(A1, A2, A3, A4, A5)) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new MutantFunction<R, R (*)(A1, A2, A3, A4, A5),
                         Tuple0, Tuple5<A1, A2, A3, A4, A5> >
          (function, MakeTuple());
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename A1, typename A2,
          typename A3, typename A4, typename A5>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(T** obj, R (U::*method)(A1, A2, A3, A4, A5)) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(A1, A2, A3, A4, A5),
                               Tuple0, Tuple5<A1, A2, A3, A4, A5> >
          (obj, method, MakeTuple());
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename A1, typename A2,
          typename A3, typename A4, typename A5>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(T* obj, R (__stdcall U::*method)(A1, A2, A3, A4, A5)) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new Mutant<R, T, R (__stdcall U::*)(A1, A2, A3, A4, A5),
                 Tuple0, Tuple5<A1, A2, A3, A4, A5> >
          (obj, method, MakeTuple());
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}

template <typename R, typename A1, typename A2, typename A3, typename A4,
          typename A5>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(R (__stdcall *function)(A1, A2, A3, A4, A5)) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new MutantFunction<R, R (__stdcall *)(A1, A2, A3, A4, A5),
                         Tuple0, Tuple5<A1, A2, A3, A4, A5> >
          (function, MakeTuple());
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename A1, typename A2,
          typename A3, typename A4, typename A5>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(T** obj, R (__stdcall U::*method)(A1, A2, A3, A4, A5)) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(A1, A2, A3, A4, A5),
                               Tuple0, Tuple5<A1, A2, A3, A4, A5> >
          (obj, method, MakeTuple());
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename A1, typename A2,
          typename A3, typename A4, typename A5, typename A6>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(T* obj, R (U::*method)(A1, A2, A3, A4, A5, A6)) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new Mutant<R, T, R (U::*)(A1, A2, A3, A4, A5, A6),
                 Tuple0, Tuple6<A1, A2, A3, A4, A5, A6> >
          (obj, method, MakeTuple());
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}

template <typename R, typename A1, typename A2, typename A3, typename A4,
          typename A5, typename A6>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(R (*function)(A1, A2, A3, A4, A5, A6)) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new MutantFunction<R, R (*)(A1, A2, A3, A4, A5, A6),
                         Tuple0, Tuple6<A1, A2, A3, A4, A5, A6> >
          (function, MakeTuple());
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename A1, typename A2,
          typename A3, typename A4, typename A5, typename A6>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(T** obj, R (U::*method)(A1, A2, A3, A4, A5, A6)) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(A1, A2, A3, A4, A5, A6),
                               Tuple0, Tuple6<A1, A2, A3, A4, A5, A6> >
          (obj, method, MakeTuple());
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename A1, typename A2,
          typename A3, typename A4, typename A5, typename A6>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(T* obj, R (__stdcall U::*method)(A1, A2, A3, A4, A5, A6)) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new Mutant<R, T, R (__stdcall U::*)(A1, A2, A3, A4, A5, A6),
                 Tuple0, Tuple6<A1, A2, A3, A4, A5, A6> >
          (obj, method, MakeTuple());
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}

template <typename R, typename A1, typename A2, typename A3, typename A4,
          typename A5, typename A6>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(R (__stdcall *function)(A1, A2, A3, A4, A5, A6)) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new MutantFunction<R, R (__stdcall *)(A1, A2, A3, A4, A5, A6),
                         Tuple0, Tuple6<A1, A2, A3, A4, A5, A6> >
          (function, MakeTuple());
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename A1, typename A2,
          typename A3, typename A4, typename A5, typename A6>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(T** obj, R (__stdcall U::*method)(A1, A2, A3, A4, A5, A6)) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(A1, A2, A3, A4, A5, A6),
                               Tuple0, Tuple6<A1, A2, A3, A4, A5, A6> >
          (obj, method, MakeTuple());
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename X1>
inline MutantFunctor<R, Tuple0>
CreateFunctor(T* obj, R (U::*method)(X1), const P1& p1) {
  MutantRunner<R, Tuple0>* t =
      new Mutant<R, T, R (U::*)(X1),
                 Tuple1<P1>, Tuple0>
          (obj, method, MakeTuple(p1));
  return MutantFunctor<R, Tuple0>(t);
}

template <typename R, typename P1, typename X1>
inline MutantFunctor<R, Tuple0>
CreateFunctor(R (*function)(X1), const P1& p1) {
  MutantRunner<R, Tuple0>* t =
      new MutantFunction<R, R (*)(X1),
                         Tuple1<P1>, Tuple0>
          (function, MakeTuple(p1));
  return MutantFunctor<R, Tuple0>(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename X1>
inline MutantFunctor<R, Tuple0>
CreateFunctor(T** obj, R (U::*method)(X1), const P1& p1) {
  MutantRunner<R, Tuple0>* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1),
                               Tuple1<P1>, Tuple0>
          (obj, method, MakeTuple(p1));
  return MutantFunctor<R, Tuple0>(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename X1>
inline MutantFunctor<R, Tuple0>
CreateFunctor(T* obj, R (__stdcall U::*method)(X1), const P1& p1) {
  MutantRunner<R, Tuple0>* t =
      new Mutant<R, T, R (__stdcall U::*)(X1),
                 Tuple1<P1>, Tuple0>
          (obj, method, MakeTuple(p1));
  return MutantFunctor<R, Tuple0>(t);
}

template <typename R, typename P1, typename X1>
inline MutantFunctor<R, Tuple0>
CreateFunctor(R (__stdcall *function)(X1), const P1& p1) {
  MutantRunner<R, Tuple0>* t =
      new MutantFunction<R, R (__stdcall *)(X1),
                         Tuple1<P1>, Tuple0>
          (function, MakeTuple(p1));
  return MutantFunctor<R, Tuple0>(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename X1>
inline MutantFunctor<R, Tuple0>
CreateFunctor(T** obj, R (__stdcall U::*method)(X1), const P1& p1) {
  MutantRunner<R, Tuple0>* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1),
                               Tuple1<P1>, Tuple0>
          (obj, method, MakeTuple(p1));
  return MutantFunctor<R, Tuple0>(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename A1,
          typename X1>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(T* obj, R (U::*method)(X1, A1), const P1& p1) {
  MutantRunner<R, Tuple1<A1> >* t =
      new Mutant<R, T, R (U::*)(X1, A1),
                 Tuple1<P1>, Tuple1<A1> >
          (obj, method, MakeTuple(p1));
  return MutantFunctor<R, Tuple1<A1> >(t);
}

template <typename R, typename P1, typename A1, typename X1>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(R (*function)(X1, A1), const P1& p1) {
  MutantRunner<R, Tuple1<A1> >* t =
      new MutantFunction<R, R (*)(X1, A1),
                         Tuple1<P1>, Tuple1<A1> >
          (function, MakeTuple(p1));
  return MutantFunctor<R, Tuple1<A1> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename A1,
          typename X1>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(T** obj, R (U::*method)(X1, A1), const P1& p1) {
  MutantRunner<R, Tuple1<A1> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, A1),
                               Tuple1<P1>, Tuple1<A1> >
          (obj, method, MakeTuple(p1));
  return MutantFunctor<R, Tuple1<A1> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename A1,
          typename X1>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, A1), const P1& p1) {
  MutantRunner<R, Tuple1<A1> >* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, A1),
                 Tuple1<P1>, Tuple1<A1> >
          (obj, method, MakeTuple(p1));
  return MutantFunctor<R, Tuple1<A1> >(t);
}

template <typename R, typename P1, typename A1, typename X1>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(R (__stdcall *function)(X1, A1), const P1& p1) {
  MutantRunner<R, Tuple1<A1> >* t =
      new MutantFunction<R, R (__stdcall *)(X1, A1),
                         Tuple1<P1>, Tuple1<A1> >
          (function, MakeTuple(p1));
  return MutantFunctor<R, Tuple1<A1> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename A1,
          typename X1>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, A1), const P1& p1) {
  MutantRunner<R, Tuple1<A1> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, A1),
                               Tuple1<P1>, Tuple1<A1> >
          (obj, method, MakeTuple(p1));
  return MutantFunctor<R, Tuple1<A1> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename A1,
          typename A2, typename X1>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(T* obj, R (U::*method)(X1, A1, A2), const P1& p1) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new Mutant<R, T, R (U::*)(X1, A1, A2),
                 Tuple1<P1>, Tuple2<A1, A2> >
          (obj, method, MakeTuple(p1));
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}

template <typename R, typename P1, typename A1, typename A2, typename X1>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(R (*function)(X1, A1, A2), const P1& p1) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new MutantFunction<R, R (*)(X1, A1, A2),
                         Tuple1<P1>, Tuple2<A1, A2> >
          (function, MakeTuple(p1));
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename A1,
          typename A2, typename X1>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(T** obj, R (U::*method)(X1, A1, A2), const P1& p1) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, A1, A2),
                               Tuple1<P1>, Tuple2<A1, A2> >
          (obj, method, MakeTuple(p1));
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename A1,
          typename A2, typename X1>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, A1, A2), const P1& p1) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, A1, A2),
                 Tuple1<P1>, Tuple2<A1, A2> >
          (obj, method, MakeTuple(p1));
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}

template <typename R, typename P1, typename A1, typename A2, typename X1>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(R (__stdcall *function)(X1, A1, A2), const P1& p1) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new MutantFunction<R, R (__stdcall *)(X1, A1, A2),
                         Tuple1<P1>, Tuple2<A1, A2> >
          (function, MakeTuple(p1));
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename A1,
          typename A2, typename X1>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, A1, A2), const P1& p1) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, A1, A2),
                               Tuple1<P1>, Tuple2<A1, A2> >
          (obj, method, MakeTuple(p1));
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename A1,
          typename A2, typename A3, typename X1>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(T* obj, R (U::*method)(X1, A1, A2, A3), const P1& p1) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new Mutant<R, T, R (U::*)(X1, A1, A2, A3),
                 Tuple1<P1>, Tuple3<A1, A2, A3> >
          (obj, method, MakeTuple(p1));
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}

template <typename R, typename P1, typename A1, typename A2, typename A3,
          typename X1>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(R (*function)(X1, A1, A2, A3), const P1& p1) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new MutantFunction<R, R (*)(X1, A1, A2, A3),
                         Tuple1<P1>, Tuple3<A1, A2, A3> >
          (function, MakeTuple(p1));
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename A1,
          typename A2, typename A3, typename X1>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(T** obj, R (U::*method)(X1, A1, A2, A3), const P1& p1) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, A1, A2, A3),
                               Tuple1<P1>, Tuple3<A1, A2, A3> >
          (obj, method, MakeTuple(p1));
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename A1,
          typename A2, typename A3, typename X1>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, A1, A2, A3), const P1& p1) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, A1, A2, A3),
                 Tuple1<P1>, Tuple3<A1, A2, A3> >
          (obj, method, MakeTuple(p1));
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}

template <typename R, typename P1, typename A1, typename A2, typename A3,
          typename X1>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(R (__stdcall *function)(X1, A1, A2, A3), const P1& p1) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new MutantFunction<R, R (__stdcall *)(X1, A1, A2, A3),
                         Tuple1<P1>, Tuple3<A1, A2, A3> >
          (function, MakeTuple(p1));
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename A1,
          typename A2, typename A3, typename X1>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, A1, A2, A3), const P1& p1) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, A1, A2, A3),
                               Tuple1<P1>, Tuple3<A1, A2, A3> >
          (obj, method, MakeTuple(p1));
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename A1,
          typename A2, typename A3, typename A4, typename X1>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(T* obj, R (U::*method)(X1, A1, A2, A3, A4), const P1& p1) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new Mutant<R, T, R (U::*)(X1, A1, A2, A3, A4),
                 Tuple1<P1>, Tuple4<A1, A2, A3, A4> >
          (obj, method, MakeTuple(p1));
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}

template <typename R, typename P1, typename A1, typename A2, typename A3,
          typename A4, typename X1>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(R (*function)(X1, A1, A2, A3, A4), const P1& p1) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new MutantFunction<R, R (*)(X1, A1, A2, A3, A4),
                         Tuple1<P1>, Tuple4<A1, A2, A3, A4> >
          (function, MakeTuple(p1));
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename A1,
          typename A2, typename A3, typename A4, typename X1>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(T** obj, R (U::*method)(X1, A1, A2, A3, A4), const P1& p1) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, A1, A2, A3, A4),
                               Tuple1<P1>, Tuple4<A1, A2, A3, A4> >
          (obj, method, MakeTuple(p1));
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename A1,
          typename A2, typename A3, typename A4, typename X1>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, A1, A2, A3, A4),
    const P1& p1) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, A1, A2, A3, A4),
                 Tuple1<P1>, Tuple4<A1, A2, A3, A4> >
          (obj, method, MakeTuple(p1));
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}

template <typename R, typename P1, typename A1, typename A2, typename A3,
          typename A4, typename X1>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(R (__stdcall *function)(X1, A1, A2, A3, A4), const P1& p1) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new MutantFunction<R, R (__stdcall *)(X1, A1, A2, A3, A4),
                         Tuple1<P1>, Tuple4<A1, A2, A3, A4> >
          (function, MakeTuple(p1));
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename A1,
          typename A2, typename A3, typename A4, typename X1>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, A1, A2, A3, A4),
    const P1& p1) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, A1, A2, A3, A4),
                               Tuple1<P1>, Tuple4<A1, A2, A3, A4> >
          (obj, method, MakeTuple(p1));
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename A1,
          typename A2, typename A3, typename A4, typename A5, typename X1>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(T* obj, R (U::*method)(X1, A1, A2, A3, A4, A5), const P1& p1) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new Mutant<R, T, R (U::*)(X1, A1, A2, A3, A4, A5),
                 Tuple1<P1>, Tuple5<A1, A2, A3, A4, A5> >
          (obj, method, MakeTuple(p1));
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}

template <typename R, typename P1, typename A1, typename A2, typename A3,
          typename A4, typename A5, typename X1>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(R (*function)(X1, A1, A2, A3, A4, A5), const P1& p1) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new MutantFunction<R, R (*)(X1, A1, A2, A3, A4, A5),
                         Tuple1<P1>, Tuple5<A1, A2, A3, A4, A5> >
          (function, MakeTuple(p1));
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename A1,
          typename A2, typename A3, typename A4, typename A5, typename X1>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(T** obj, R (U::*method)(X1, A1, A2, A3, A4, A5), const P1& p1) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, A1, A2, A3, A4, A5),
                               Tuple1<P1>, Tuple5<A1, A2, A3, A4, A5> >
          (obj, method, MakeTuple(p1));
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename A1,
          typename A2, typename A3, typename A4, typename A5, typename X1>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, A1, A2, A3, A4, A5),
    const P1& p1) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, A1, A2, A3, A4, A5),
                 Tuple1<P1>, Tuple5<A1, A2, A3, A4, A5> >
          (obj, method, MakeTuple(p1));
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}

template <typename R, typename P1, typename A1, typename A2, typename A3,
          typename A4, typename A5, typename X1>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(R (__stdcall *function)(X1, A1, A2, A3, A4, A5), const P1& p1) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new MutantFunction<R, R (__stdcall *)(X1, A1, A2, A3, A4, A5),
                         Tuple1<P1>, Tuple5<A1, A2, A3, A4, A5> >
          (function, MakeTuple(p1));
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename A1,
          typename A2, typename A3, typename A4, typename A5, typename X1>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, A1, A2, A3, A4, A5),
    const P1& p1) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, A1, A2, A3, A4, A5),
                               Tuple1<P1>, Tuple5<A1, A2, A3, A4, A5> >
          (obj, method, MakeTuple(p1));
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename A1,
          typename A2, typename A3, typename A4, typename A5, typename A6,
          typename X1>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(T* obj, R (U::*method)(X1, A1, A2, A3, A4, A5, A6),
    const P1& p1) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new Mutant<R, T, R (U::*)(X1, A1, A2, A3, A4, A5, A6),
                 Tuple1<P1>, Tuple6<A1, A2, A3, A4, A5, A6> >
          (obj, method, MakeTuple(p1));
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}

template <typename R, typename P1, typename A1, typename A2, typename A3,
          typename A4, typename A5, typename A6, typename X1>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(R (*function)(X1, A1, A2, A3, A4, A5, A6), const P1& p1) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new MutantFunction<R, R (*)(X1, A1, A2, A3, A4, A5, A6),
                         Tuple1<P1>, Tuple6<A1, A2, A3, A4, A5, A6> >
          (function, MakeTuple(p1));
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename A1,
          typename A2, typename A3, typename A4, typename A5, typename A6,
          typename X1>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(T** obj, R (U::*method)(X1, A1, A2, A3, A4, A5, A6),
    const P1& p1) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, A1, A2, A3, A4, A5, A6),
                               Tuple1<P1>, Tuple6<A1, A2, A3, A4, A5, A6> >
          (obj, method, MakeTuple(p1));
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename A1,
          typename A2, typename A3, typename A4, typename A5, typename A6,
          typename X1>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, A1, A2, A3, A4, A5, A6),
    const P1& p1) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, A1, A2, A3, A4, A5, A6),
                 Tuple1<P1>, Tuple6<A1, A2, A3, A4, A5, A6> >
          (obj, method, MakeTuple(p1));
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}

template <typename R, typename P1, typename A1, typename A2, typename A3,
          typename A4, typename A5, typename A6, typename X1>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(R (__stdcall *function)(X1, A1, A2, A3, A4, A5, A6),
    const P1& p1) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new MutantFunction<R, R (__stdcall *)(X1, A1, A2, A3, A4, A5, A6),
                         Tuple1<P1>, Tuple6<A1, A2, A3, A4, A5, A6> >
          (function, MakeTuple(p1));
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename A1,
          typename A2, typename A3, typename A4, typename A5, typename A6,
          typename X1>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, A1, A2, A3, A4, A5, A6),
    const P1& p1) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, A1, A2, A3, A4, A5, A6),
                               Tuple1<P1>, Tuple6<A1, A2, A3, A4, A5, A6> >
          (obj, method, MakeTuple(p1));
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename P2,
          typename X1, typename X2>
inline MutantFunctor<R, Tuple0>
CreateFunctor(T* obj, R (U::*method)(X1, X2), const P1& p1, const P2& p2) {
  MutantRunner<R, Tuple0>* t =
      new Mutant<R, T, R (U::*)(X1, X2),
                 Tuple2<P1, P2>, Tuple0>
          (obj, method, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple0>(t);
}

template <typename R, typename P1, typename P2, typename X1, typename X2>
inline MutantFunctor<R, Tuple0>
CreateFunctor(R (*function)(X1, X2), const P1& p1, const P2& p2) {
  MutantRunner<R, Tuple0>* t =
      new MutantFunction<R, R (*)(X1, X2),
                         Tuple2<P1, P2>, Tuple0>
          (function, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple0>(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename X1, typename X2>
inline MutantFunctor<R, Tuple0>
CreateFunctor(T** obj, R (U::*method)(X1, X2), const P1& p1, const P2& p2) {
  MutantRunner<R, Tuple0>* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, X2),
                               Tuple2<P1, P2>, Tuple0>
          (obj, method, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple0>(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename P2,
          typename X1, typename X2>
inline MutantFunctor<R, Tuple0>
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, X2), const P1& p1,
    const P2& p2) {
  MutantRunner<R, Tuple0>* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, X2),
                 Tuple2<P1, P2>, Tuple0>
          (obj, method, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple0>(t);
}

template <typename R, typename P1, typename P2, typename X1, typename X2>
inline MutantFunctor<R, Tuple0>
CreateFunctor(R (__stdcall *function)(X1, X2), const P1& p1, const P2& p2) {
  MutantRunner<R, Tuple0>* t =
      new MutantFunction<R, R (__stdcall *)(X1, X2),
                         Tuple2<P1, P2>, Tuple0>
          (function, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple0>(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename X1, typename X2>
inline MutantFunctor<R, Tuple0>
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, X2), const P1& p1,
    const P2& p2) {
  MutantRunner<R, Tuple0>* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, X2),
                               Tuple2<P1, P2>, Tuple0>
          (obj, method, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple0>(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename P2,
          typename A1, typename X1, typename X2>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(T* obj, R (U::*method)(X1, X2, A1), const P1& p1, const P2& p2) {
  MutantRunner<R, Tuple1<A1> >* t =
      new Mutant<R, T, R (U::*)(X1, X2, A1),
                 Tuple2<P1, P2>, Tuple1<A1> >
          (obj, method, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple1<A1> >(t);
}

template <typename R, typename P1, typename P2, typename A1, typename X1,
          typename X2>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(R (*function)(X1, X2, A1), const P1& p1, const P2& p2) {
  MutantRunner<R, Tuple1<A1> >* t =
      new MutantFunction<R, R (*)(X1, X2, A1),
                         Tuple2<P1, P2>, Tuple1<A1> >
          (function, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple1<A1> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename A1, typename X1, typename X2>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(T** obj, R (U::*method)(X1, X2, A1), const P1& p1, const P2& p2) {
  MutantRunner<R, Tuple1<A1> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, X2, A1),
                               Tuple2<P1, P2>, Tuple1<A1> >
          (obj, method, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple1<A1> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename P2,
          typename A1, typename X1, typename X2>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, X2, A1), const P1& p1,
    const P2& p2) {
  MutantRunner<R, Tuple1<A1> >* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, X2, A1),
                 Tuple2<P1, P2>, Tuple1<A1> >
          (obj, method, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple1<A1> >(t);
}

template <typename R, typename P1, typename P2, typename A1, typename X1,
          typename X2>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(R (__stdcall *function)(X1, X2, A1), const P1& p1,
    const P2& p2) {
  MutantRunner<R, Tuple1<A1> >* t =
      new MutantFunction<R, R (__stdcall *)(X1, X2, A1),
                         Tuple2<P1, P2>, Tuple1<A1> >
          (function, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple1<A1> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename A1, typename X1, typename X2>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, X2, A1), const P1& p1,
    const P2& p2) {
  MutantRunner<R, Tuple1<A1> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, X2, A1),
                               Tuple2<P1, P2>, Tuple1<A1> >
          (obj, method, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple1<A1> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename P2,
          typename A1, typename A2, typename X1, typename X2>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(T* obj, R (U::*method)(X1, X2, A1, A2), const P1& p1,
    const P2& p2) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new Mutant<R, T, R (U::*)(X1, X2, A1, A2),
                 Tuple2<P1, P2>, Tuple2<A1, A2> >
          (obj, method, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}

template <typename R, typename P1, typename P2, typename A1, typename A2,
          typename X1, typename X2>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(R (*function)(X1, X2, A1, A2), const P1& p1, const P2& p2) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new MutantFunction<R, R (*)(X1, X2, A1, A2),
                         Tuple2<P1, P2>, Tuple2<A1, A2> >
          (function, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename A1, typename A2, typename X1, typename X2>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(T** obj, R (U::*method)(X1, X2, A1, A2), const P1& p1,
    const P2& p2) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, X2, A1, A2),
                               Tuple2<P1, P2>, Tuple2<A1, A2> >
          (obj, method, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename P2,
          typename A1, typename A2, typename X1, typename X2>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, X2, A1, A2), const P1& p1,
    const P2& p2) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, X2, A1, A2),
                 Tuple2<P1, P2>, Tuple2<A1, A2> >
          (obj, method, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}

template <typename R, typename P1, typename P2, typename A1, typename A2,
          typename X1, typename X2>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(R (__stdcall *function)(X1, X2, A1, A2), const P1& p1,
    const P2& p2) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new MutantFunction<R, R (__stdcall *)(X1, X2, A1, A2),
                         Tuple2<P1, P2>, Tuple2<A1, A2> >
          (function, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename A1, typename A2, typename X1, typename X2>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, X2, A1, A2), const P1& p1,
    const P2& p2) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, X2, A1, A2),
                               Tuple2<P1, P2>, Tuple2<A1, A2> >
          (obj, method, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename P2,
          typename A1, typename A2, typename A3, typename X1, typename X2>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(T* obj, R (U::*method)(X1, X2, A1, A2, A3), const P1& p1,
    const P2& p2) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new Mutant<R, T, R (U::*)(X1, X2, A1, A2, A3),
                 Tuple2<P1, P2>, Tuple3<A1, A2, A3> >
          (obj, method, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}

template <typename R, typename P1, typename P2, typename A1, typename A2,
          typename A3, typename X1, typename X2>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(R (*function)(X1, X2, A1, A2, A3), const P1& p1, const P2& p2) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new MutantFunction<R, R (*)(X1, X2, A1, A2, A3),
                         Tuple2<P1, P2>, Tuple3<A1, A2, A3> >
          (function, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename A1, typename A2, typename A3, typename X1, typename X2>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(T** obj, R (U::*method)(X1, X2, A1, A2, A3), const P1& p1,
    const P2& p2) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, X2, A1, A2, A3),
                               Tuple2<P1, P2>, Tuple3<A1, A2, A3> >
          (obj, method, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename P2,
          typename A1, typename A2, typename A3, typename X1, typename X2>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, X2, A1, A2, A3),
    const P1& p1, const P2& p2) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, X2, A1, A2, A3),
                 Tuple2<P1, P2>, Tuple3<A1, A2, A3> >
          (obj, method, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}

template <typename R, typename P1, typename P2, typename A1, typename A2,
          typename A3, typename X1, typename X2>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(R (__stdcall *function)(X1, X2, A1, A2, A3), const P1& p1,
    const P2& p2) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new MutantFunction<R, R (__stdcall *)(X1, X2, A1, A2, A3),
                         Tuple2<P1, P2>, Tuple3<A1, A2, A3> >
          (function, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename A1, typename A2, typename A3, typename X1, typename X2>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, X2, A1, A2, A3),
    const P1& p1, const P2& p2) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, X2, A1, A2, A3),
                               Tuple2<P1, P2>, Tuple3<A1, A2, A3> >
          (obj, method, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename P2,
          typename A1, typename A2, typename A3, typename A4, typename X1,
          typename X2>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(T* obj, R (U::*method)(X1, X2, A1, A2, A3, A4), const P1& p1,
    const P2& p2) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new Mutant<R, T, R (U::*)(X1, X2, A1, A2, A3, A4),
                 Tuple2<P1, P2>, Tuple4<A1, A2, A3, A4> >
          (obj, method, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}

template <typename R, typename P1, typename P2, typename A1, typename A2,
          typename A3, typename A4, typename X1, typename X2>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(R (*function)(X1, X2, A1, A2, A3, A4), const P1& p1,
    const P2& p2) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new MutantFunction<R, R (*)(X1, X2, A1, A2, A3, A4),
                         Tuple2<P1, P2>, Tuple4<A1, A2, A3, A4> >
          (function, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename A1, typename A2, typename A3, typename A4, typename X1,
          typename X2>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(T** obj, R (U::*method)(X1, X2, A1, A2, A3, A4), const P1& p1,
    const P2& p2) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, X2, A1, A2, A3, A4),
                               Tuple2<P1, P2>, Tuple4<A1, A2, A3, A4> >
          (obj, method, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename P2,
          typename A1, typename A2, typename A3, typename A4, typename X1,
          typename X2>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, X2, A1, A2, A3, A4),
    const P1& p1, const P2& p2) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, X2, A1, A2, A3, A4),
                 Tuple2<P1, P2>, Tuple4<A1, A2, A3, A4> >
          (obj, method, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}

template <typename R, typename P1, typename P2, typename A1, typename A2,
          typename A3, typename A4, typename X1, typename X2>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(R (__stdcall *function)(X1, X2, A1, A2, A3, A4), const P1& p1,
    const P2& p2) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new MutantFunction<R, R (__stdcall *)(X1, X2, A1, A2, A3, A4),
                         Tuple2<P1, P2>, Tuple4<A1, A2, A3, A4> >
          (function, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename A1, typename A2, typename A3, typename A4, typename X1,
          typename X2>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, X2, A1, A2, A3, A4),
    const P1& p1, const P2& p2) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, X2, A1, A2, A3, A4),
                               Tuple2<P1, P2>, Tuple4<A1, A2, A3, A4> >
          (obj, method, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename P2,
          typename A1, typename A2, typename A3, typename A4, typename A5,
          typename X1, typename X2>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(T* obj, R (U::*method)(X1, X2, A1, A2, A3, A4, A5), const P1& p1,
    const P2& p2) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new Mutant<R, T, R (U::*)(X1, X2, A1, A2, A3, A4, A5),
                 Tuple2<P1, P2>, Tuple5<A1, A2, A3, A4, A5> >
          (obj, method, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}

template <typename R, typename P1, typename P2, typename A1, typename A2,
          typename A3, typename A4, typename A5, typename X1, typename X2>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(R (*function)(X1, X2, A1, A2, A3, A4, A5), const P1& p1,
    const P2& p2) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new MutantFunction<R, R (*)(X1, X2, A1, A2, A3, A4, A5),
                         Tuple2<P1, P2>, Tuple5<A1, A2, A3, A4, A5> >
          (function, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename A1, typename A2, typename A3, typename A4, typename A5,
          typename X1, typename X2>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(T** obj, R (U::*method)(X1, X2, A1, A2, A3, A4, A5), const P1& p1,
    const P2& p2) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, X2, A1, A2, A3, A4, A5),
                               Tuple2<P1, P2>, Tuple5<A1, A2, A3, A4, A5> >
          (obj, method, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename P2,
          typename A1, typename A2, typename A3, typename A4, typename A5,
          typename X1, typename X2>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, X2, A1, A2, A3, A4, A5),
    const P1& p1, const P2& p2) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, X2, A1, A2, A3, A4, A5),
                 Tuple2<P1, P2>, Tuple5<A1, A2, A3, A4, A5> >
          (obj, method, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}

template <typename R, typename P1, typename P2, typename A1, typename A2,
          typename A3, typename A4, typename A5, typename X1, typename X2>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(R (__stdcall *function)(X1, X2, A1, A2, A3, A4, A5), const P1& p1,
    const P2& p2) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new MutantFunction<R, R (__stdcall *)(X1, X2, A1, A2, A3, A4, A5),
                         Tuple2<P1, P2>, Tuple5<A1, A2, A3, A4, A5> >
          (function, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename A1, typename A2, typename A3, typename A4, typename A5,
          typename X1, typename X2>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, X2, A1, A2, A3, A4, A5),
    const P1& p1, const P2& p2) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, X2, A1, A2, A3, A4, A5),
                               Tuple2<P1, P2>, Tuple5<A1, A2, A3, A4, A5> >
          (obj, method, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename P2,
          typename A1, typename A2, typename A3, typename A4, typename A5,
          typename A6, typename X1, typename X2>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(T* obj, R (U::*method)(X1, X2, A1, A2, A3, A4, A5, A6),
    const P1& p1, const P2& p2) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new Mutant<R, T, R (U::*)(X1, X2, A1, A2, A3, A4, A5, A6),
                 Tuple2<P1, P2>, Tuple6<A1, A2, A3, A4, A5, A6> >
          (obj, method, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}

template <typename R, typename P1, typename P2, typename A1, typename A2,
          typename A3, typename A4, typename A5, typename A6, typename X1,
          typename X2>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(R (*function)(X1, X2, A1, A2, A3, A4, A5, A6), const P1& p1,
    const P2& p2) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new MutantFunction<R, R (*)(X1, X2, A1, A2, A3, A4, A5, A6),
                         Tuple2<P1, P2>, Tuple6<A1, A2, A3, A4, A5, A6> >
          (function, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename A1, typename A2, typename A3, typename A4, typename A5,
          typename A6, typename X1, typename X2>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(T** obj, R (U::*method)(X1, X2, A1, A2, A3, A4, A5, A6),
    const P1& p1, const P2& p2) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, X2, A1, A2, A3, A4, A5, A6),
                               Tuple2<P1, P2>, Tuple6<A1, A2, A3, A4, A5, A6> >
          (obj, method, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename P2,
          typename A1, typename A2, typename A3, typename A4, typename A5,
          typename A6, typename X1, typename X2>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, X2, A1, A2, A3, A4, A5, A6),
    const P1& p1, const P2& p2) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, X2, A1, A2, A3, A4, A5, A6),
                 Tuple2<P1, P2>, Tuple6<A1, A2, A3, A4, A5, A6> >
          (obj, method, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}

template <typename R, typename P1, typename P2, typename A1, typename A2,
          typename A3, typename A4, typename A5, typename A6, typename X1,
          typename X2>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(R (__stdcall *function)(X1, X2, A1, A2, A3, A4, A5, A6),
    const P1& p1, const P2& p2) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new MutantFunction<R, R (__stdcall *)(X1, X2, A1, A2, A3, A4, A5, A6),
                         Tuple2<P1, P2>, Tuple6<A1, A2, A3, A4, A5, A6> >
          (function, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename A1, typename A2, typename A3, typename A4, typename A5,
          typename A6, typename X1, typename X2>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, X2, A1, A2, A3, A4, A5, A6),
    const P1& p1, const P2& p2) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, X2, A1, A2, A3, A4, A5, A6),
                               Tuple2<P1, P2>, Tuple6<A1, A2, A3, A4, A5, A6> >
          (obj, method, MakeTuple(p1, p2));
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename X1, typename X2, typename X3>
inline MutantFunctor<R, Tuple0>
CreateFunctor(T* obj, R (U::*method)(X1, X2, X3), const P1& p1, const P2& p2,
    const P3& p3) {
  MutantRunner<R, Tuple0>* t =
      new Mutant<R, T, R (U::*)(X1, X2, X3),
                 Tuple3<P1, P2, P3>, Tuple0>
          (obj, method, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple0>(t);
}

template <typename R, typename P1, typename P2, typename P3, typename X1,
          typename X2, typename X3>
inline MutantFunctor<R, Tuple0>
CreateFunctor(R (*function)(X1, X2, X3), const P1& p1, const P2& p2,
    const P3& p3) {
  MutantRunner<R, Tuple0>* t =
      new MutantFunction<R, R (*)(X1, X2, X3),
                         Tuple3<P1, P2, P3>, Tuple0>
          (function, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple0>(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename X1, typename X2, typename X3>
inline MutantFunctor<R, Tuple0>
CreateFunctor(T** obj, R (U::*method)(X1, X2, X3), const P1& p1, const P2& p2,
    const P3& p3) {
  MutantRunner<R, Tuple0>* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, X2, X3),
                               Tuple3<P1, P2, P3>, Tuple0>
          (obj, method, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple0>(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename X1, typename X2, typename X3>
inline MutantFunctor<R, Tuple0>
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, X2, X3), const P1& p1,
    const P2& p2, const P3& p3) {
  MutantRunner<R, Tuple0>* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, X2, X3),
                 Tuple3<P1, P2, P3>, Tuple0>
          (obj, method, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple0>(t);
}

template <typename R, typename P1, typename P2, typename P3, typename X1,
          typename X2, typename X3>
inline MutantFunctor<R, Tuple0>
CreateFunctor(R (__stdcall *function)(X1, X2, X3), const P1& p1, const P2& p2,
    const P3& p3) {
  MutantRunner<R, Tuple0>* t =
      new MutantFunction<R, R (__stdcall *)(X1, X2, X3),
                         Tuple3<P1, P2, P3>, Tuple0>
          (function, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple0>(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename X1, typename X2, typename X3>
inline MutantFunctor<R, Tuple0>
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, X2, X3), const P1& p1,
    const P2& p2, const P3& p3) {
  MutantRunner<R, Tuple0>* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, X2, X3),
                               Tuple3<P1, P2, P3>, Tuple0>
          (obj, method, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple0>(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename A1, typename X1, typename X2, typename X3>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(T* obj, R (U::*method)(X1, X2, X3, A1), const P1& p1,
    const P2& p2, const P3& p3) {
  MutantRunner<R, Tuple1<A1> >* t =
      new Mutant<R, T, R (U::*)(X1, X2, X3, A1),
                 Tuple3<P1, P2, P3>, Tuple1<A1> >
          (obj, method, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple1<A1> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename A1,
          typename X1, typename X2, typename X3>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(R (*function)(X1, X2, X3, A1), const P1& p1, const P2& p2,
    const P3& p3) {
  MutantRunner<R, Tuple1<A1> >* t =
      new MutantFunction<R, R (*)(X1, X2, X3, A1),
                         Tuple3<P1, P2, P3>, Tuple1<A1> >
          (function, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple1<A1> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename A1, typename X1, typename X2, typename X3>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(T** obj, R (U::*method)(X1, X2, X3, A1), const P1& p1,
    const P2& p2, const P3& p3) {
  MutantRunner<R, Tuple1<A1> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, X2, X3, A1),
                               Tuple3<P1, P2, P3>, Tuple1<A1> >
          (obj, method, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple1<A1> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename A1, typename X1, typename X2, typename X3>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, X2, X3, A1), const P1& p1,
    const P2& p2, const P3& p3) {
  MutantRunner<R, Tuple1<A1> >* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, X2, X3, A1),
                 Tuple3<P1, P2, P3>, Tuple1<A1> >
          (obj, method, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple1<A1> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename A1,
          typename X1, typename X2, typename X3>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(R (__stdcall *function)(X1, X2, X3, A1), const P1& p1,
    const P2& p2, const P3& p3) {
  MutantRunner<R, Tuple1<A1> >* t =
      new MutantFunction<R, R (__stdcall *)(X1, X2, X3, A1),
                         Tuple3<P1, P2, P3>, Tuple1<A1> >
          (function, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple1<A1> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename A1, typename X1, typename X2, typename X3>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, X2, X3, A1), const P1& p1,
    const P2& p2, const P3& p3) {
  MutantRunner<R, Tuple1<A1> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, X2, X3, A1),
                               Tuple3<P1, P2, P3>, Tuple1<A1> >
          (obj, method, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple1<A1> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename A1, typename A2, typename X1, typename X2,
          typename X3>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(T* obj, R (U::*method)(X1, X2, X3, A1, A2), const P1& p1,
    const P2& p2, const P3& p3) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new Mutant<R, T, R (U::*)(X1, X2, X3, A1, A2),
                 Tuple3<P1, P2, P3>, Tuple2<A1, A2> >
          (obj, method, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename A1,
          typename A2, typename X1, typename X2, typename X3>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(R (*function)(X1, X2, X3, A1, A2), const P1& p1, const P2& p2,
    const P3& p3) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new MutantFunction<R, R (*)(X1, X2, X3, A1, A2),
                         Tuple3<P1, P2, P3>, Tuple2<A1, A2> >
          (function, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename A1, typename A2, typename X1, typename X2,
          typename X3>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(T** obj, R (U::*method)(X1, X2, X3, A1, A2), const P1& p1,
    const P2& p2, const P3& p3) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, X2, X3, A1, A2),
                               Tuple3<P1, P2, P3>, Tuple2<A1, A2> >
          (obj, method, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename A1, typename A2, typename X1, typename X2,
          typename X3>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, X2, X3, A1, A2),
    const P1& p1, const P2& p2, const P3& p3) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, X2, X3, A1, A2),
                 Tuple3<P1, P2, P3>, Tuple2<A1, A2> >
          (obj, method, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename A1,
          typename A2, typename X1, typename X2, typename X3>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(R (__stdcall *function)(X1, X2, X3, A1, A2), const P1& p1,
    const P2& p2, const P3& p3) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new MutantFunction<R, R (__stdcall *)(X1, X2, X3, A1, A2),
                         Tuple3<P1, P2, P3>, Tuple2<A1, A2> >
          (function, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename A1, typename A2, typename X1, typename X2,
          typename X3>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, X2, X3, A1, A2),
    const P1& p1, const P2& p2, const P3& p3) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, X2, X3, A1, A2),
                               Tuple3<P1, P2, P3>, Tuple2<A1, A2> >
          (obj, method, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename A1, typename A2, typename A3, typename X1,
          typename X2, typename X3>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(T* obj, R (U::*method)(X1, X2, X3, A1, A2, A3), const P1& p1,
    const P2& p2, const P3& p3) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new Mutant<R, T, R (U::*)(X1, X2, X3, A1, A2, A3),
                 Tuple3<P1, P2, P3>, Tuple3<A1, A2, A3> >
          (obj, method, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename A1,
          typename A2, typename A3, typename X1, typename X2, typename X3>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(R (*function)(X1, X2, X3, A1, A2, A3), const P1& p1, const P2& p2,
    const P3& p3) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new MutantFunction<R, R (*)(X1, X2, X3, A1, A2, A3),
                         Tuple3<P1, P2, P3>, Tuple3<A1, A2, A3> >
          (function, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename A1, typename A2, typename A3, typename X1,
          typename X2, typename X3>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(T** obj, R (U::*method)(X1, X2, X3, A1, A2, A3), const P1& p1,
    const P2& p2, const P3& p3) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, X2, X3, A1, A2, A3),
                               Tuple3<P1, P2, P3>, Tuple3<A1, A2, A3> >
          (obj, method, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename A1, typename A2, typename A3, typename X1,
          typename X2, typename X3>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, X2, X3, A1, A2, A3),
    const P1& p1, const P2& p2, const P3& p3) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, X2, X3, A1, A2, A3),
                 Tuple3<P1, P2, P3>, Tuple3<A1, A2, A3> >
          (obj, method, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename A1,
          typename A2, typename A3, typename X1, typename X2, typename X3>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(R (__stdcall *function)(X1, X2, X3, A1, A2, A3), const P1& p1,
    const P2& p2, const P3& p3) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new MutantFunction<R, R (__stdcall *)(X1, X2, X3, A1, A2, A3),
                         Tuple3<P1, P2, P3>, Tuple3<A1, A2, A3> >
          (function, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename A1, typename A2, typename A3, typename X1,
          typename X2, typename X3>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, X2, X3, A1, A2, A3),
    const P1& p1, const P2& p2, const P3& p3) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, X2, X3, A1, A2, A3),
                               Tuple3<P1, P2, P3>, Tuple3<A1, A2, A3> >
          (obj, method, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename A1, typename A2, typename A3, typename A4,
          typename X1, typename X2, typename X3>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(T* obj, R (U::*method)(X1, X2, X3, A1, A2, A3, A4), const P1& p1,
    const P2& p2, const P3& p3) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new Mutant<R, T, R (U::*)(X1, X2, X3, A1, A2, A3, A4),
                 Tuple3<P1, P2, P3>, Tuple4<A1, A2, A3, A4> >
          (obj, method, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename A1,
          typename A2, typename A3, typename A4, typename X1, typename X2,
          typename X3>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(R (*function)(X1, X2, X3, A1, A2, A3, A4), const P1& p1,
    const P2& p2, const P3& p3) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new MutantFunction<R, R (*)(X1, X2, X3, A1, A2, A3, A4),
                         Tuple3<P1, P2, P3>, Tuple4<A1, A2, A3, A4> >
          (function, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename A1, typename A2, typename A3, typename A4,
          typename X1, typename X2, typename X3>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(T** obj, R (U::*method)(X1, X2, X3, A1, A2, A3, A4), const P1& p1,
    const P2& p2, const P3& p3) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, X2, X3, A1, A2, A3, A4),
                               Tuple3<P1, P2, P3>, Tuple4<A1, A2, A3, A4> >
          (obj, method, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename A1, typename A2, typename A3, typename A4,
          typename X1, typename X2, typename X3>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, X2, X3, A1, A2, A3, A4),
    const P1& p1, const P2& p2, const P3& p3) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, X2, X3, A1, A2, A3, A4),
                 Tuple3<P1, P2, P3>, Tuple4<A1, A2, A3, A4> >
          (obj, method, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename A1,
          typename A2, typename A3, typename A4, typename X1, typename X2,
          typename X3>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(R (__stdcall *function)(X1, X2, X3, A1, A2, A3, A4), const P1& p1,
    const P2& p2, const P3& p3) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new MutantFunction<R, R (__stdcall *)(X1, X2, X3, A1, A2, A3, A4),
                         Tuple3<P1, P2, P3>, Tuple4<A1, A2, A3, A4> >
          (function, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename A1, typename A2, typename A3, typename A4,
          typename X1, typename X2, typename X3>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, X2, X3, A1, A2, A3, A4),
    const P1& p1, const P2& p2, const P3& p3) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, X2, X3, A1, A2, A3, A4),
                               Tuple3<P1, P2, P3>, Tuple4<A1, A2, A3, A4> >
          (obj, method, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename A1, typename A2, typename A3, typename A4,
          typename A5, typename X1, typename X2, typename X3>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(T* obj, R (U::*method)(X1, X2, X3, A1, A2, A3, A4, A5),
    const P1& p1, const P2& p2, const P3& p3) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new Mutant<R, T, R (U::*)(X1, X2, X3, A1, A2, A3, A4, A5),
                 Tuple3<P1, P2, P3>, Tuple5<A1, A2, A3, A4, A5> >
          (obj, method, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename A1,
          typename A2, typename A3, typename A4, typename A5, typename X1,
          typename X2, typename X3>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(R (*function)(X1, X2, X3, A1, A2, A3, A4, A5), const P1& p1,
    const P2& p2, const P3& p3) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new MutantFunction<R, R (*)(X1, X2, X3, A1, A2, A3, A4, A5),
                         Tuple3<P1, P2, P3>, Tuple5<A1, A2, A3, A4, A5> >
          (function, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename A1, typename A2, typename A3, typename A4,
          typename A5, typename X1, typename X2, typename X3>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(T** obj, R (U::*method)(X1, X2, X3, A1, A2, A3, A4, A5),
    const P1& p1, const P2& p2, const P3& p3) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, X2, X3, A1, A2, A3, A4, A5),
                               Tuple3<P1, P2, P3>, Tuple5<A1, A2, A3, A4, A5> >
          (obj, method, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename A1, typename A2, typename A3, typename A4,
          typename A5, typename X1, typename X2, typename X3>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, X2, X3, A1, A2, A3, A4, A5),
    const P1& p1, const P2& p2, const P3& p3) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, X2, X3, A1, A2, A3, A4, A5),
                 Tuple3<P1, P2, P3>, Tuple5<A1, A2, A3, A4, A5> >
          (obj, method, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename A1,
          typename A2, typename A3, typename A4, typename A5, typename X1,
          typename X2, typename X3>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(R (__stdcall *function)(X1, X2, X3, A1, A2, A3, A4, A5),
    const P1& p1, const P2& p2, const P3& p3) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new MutantFunction<R, R (__stdcall *)(X1, X2, X3, A1, A2, A3, A4, A5),
                         Tuple3<P1, P2, P3>, Tuple5<A1, A2, A3, A4, A5> >
          (function, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename A1, typename A2, typename A3, typename A4,
          typename A5, typename X1, typename X2, typename X3>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, X2, X3, A1, A2, A3, A4, A5),
    const P1& p1, const P2& p2, const P3& p3) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, X2, X3, A1, A2, A3, A4, A5),
                               Tuple3<P1, P2, P3>, Tuple5<A1, A2, A3, A4, A5> >
          (obj, method, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename A1, typename A2, typename A3, typename A4,
          typename A5, typename A6, typename X1, typename X2, typename X3>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(T* obj, R (U::*method)(X1, X2, X3, A1, A2, A3, A4, A5, A6),
    const P1& p1, const P2& p2, const P3& p3) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new Mutant<R, T, R (U::*)(X1, X2, X3, A1, A2, A3, A4, A5, A6),
                 Tuple3<P1, P2, P3>, Tuple6<A1, A2, A3, A4, A5, A6> >
          (obj, method, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename A1,
          typename A2, typename A3, typename A4, typename A5, typename A6,
          typename X1, typename X2, typename X3>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(R (*function)(X1, X2, X3, A1, A2, A3, A4, A5, A6), const P1& p1,
    const P2& p2, const P3& p3) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new MutantFunction<R, R (*)(X1, X2, X3, A1, A2, A3, A4, A5, A6),
                         Tuple3<P1, P2, P3>, Tuple6<A1, A2, A3, A4, A5, A6> >
          (function, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename A1, typename A2, typename A3, typename A4,
          typename A5, typename A6, typename X1, typename X2, typename X3>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(T** obj, R (U::*method)(X1, X2, X3, A1, A2, A3, A4, A5, A6),
    const P1& p1, const P2& p2, const P3& p3) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, X2, X3, A1, A2, A3, A4, A5, A6),
                               Tuple3<P1, P2, P3>, Tuple6<A1, A2, A3, A4, A5, A6> >
          (obj, method, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename A1, typename A2, typename A3, typename A4,
          typename A5, typename A6, typename X1, typename X2, typename X3>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, X2, X3, A1, A2, A3, A4, A5,
    A6), const P1& p1, const P2& p2, const P3& p3) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, X2, X3, A1, A2, A3, A4, A5, A6),
                 Tuple3<P1, P2, P3>, Tuple6<A1, A2, A3, A4, A5, A6> >
          (obj, method, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename A1,
          typename A2, typename A3, typename A4, typename A5, typename A6,
          typename X1, typename X2, typename X3>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(R (__stdcall *function)(X1, X2, X3, A1, A2, A3, A4, A5, A6),
    const P1& p1, const P2& p2, const P3& p3) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new MutantFunction<R, R (__stdcall *)(X1, X2, X3, A1, A2, A3, A4, A5, A6),
                         Tuple3<P1, P2, P3>, Tuple6<A1, A2, A3, A4, A5, A6> >
          (function, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename A1, typename A2, typename A3, typename A4,
          typename A5, typename A6, typename X1, typename X2, typename X3>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, X2, X3, A1, A2, A3, A4, A5,
    A6), const P1& p1, const P2& p2, const P3& p3) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, X2, X3, A1, A2, A3, A4, A5, A6),
                               Tuple3<P1, P2, P3>, Tuple6<A1, A2, A3, A4, A5, A6> >
          (obj, method, MakeTuple(p1, p2, p3));
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename X1, typename X2, typename X3,
          typename X4>
inline MutantFunctor<R, Tuple0>
CreateFunctor(T* obj, R (U::*method)(X1, X2, X3, X4), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple0>* t =
      new Mutant<R, T, R (U::*)(X1, X2, X3, X4),
                 Tuple4<P1, P2, P3, P4>, Tuple0>
          (obj, method, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple0>(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename X1, typename X2, typename X3, typename X4>
inline MutantFunctor<R, Tuple0>
CreateFunctor(R (*function)(X1, X2, X3, X4), const P1& p1, const P2& p2,
    const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple0>* t =
      new MutantFunction<R, R (*)(X1, X2, X3, X4),
                         Tuple4<P1, P2, P3, P4>, Tuple0>
          (function, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple0>(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename X1, typename X2, typename X3,
          typename X4>
inline MutantFunctor<R, Tuple0>
CreateFunctor(T** obj, R (U::*method)(X1, X2, X3, X4), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple0>* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, X2, X3, X4),
                               Tuple4<P1, P2, P3, P4>, Tuple0>
          (obj, method, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple0>(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename X1, typename X2, typename X3,
          typename X4>
inline MutantFunctor<R, Tuple0>
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, X2, X3, X4), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple0>* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, X2, X3, X4),
                 Tuple4<P1, P2, P3, P4>, Tuple0>
          (obj, method, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple0>(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename X1, typename X2, typename X3, typename X4>
inline MutantFunctor<R, Tuple0>
CreateFunctor(R (__stdcall *function)(X1, X2, X3, X4), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple0>* t =
      new MutantFunction<R, R (__stdcall *)(X1, X2, X3, X4),
                         Tuple4<P1, P2, P3, P4>, Tuple0>
          (function, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple0>(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename X1, typename X2, typename X3,
          typename X4>
inline MutantFunctor<R, Tuple0>
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, X2, X3, X4), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple0>* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, X2, X3, X4),
                               Tuple4<P1, P2, P3, P4>, Tuple0>
          (obj, method, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple0>(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename A1, typename X1, typename X2,
          typename X3, typename X4>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(T* obj, R (U::*method)(X1, X2, X3, X4, A1), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple1<A1> >* t =
      new Mutant<R, T, R (U::*)(X1, X2, X3, X4, A1),
                 Tuple4<P1, P2, P3, P4>, Tuple1<A1> >
          (obj, method, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple1<A1> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename A1, typename X1, typename X2, typename X3, typename X4>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(R (*function)(X1, X2, X3, X4, A1), const P1& p1, const P2& p2,
    const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple1<A1> >* t =
      new MutantFunction<R, R (*)(X1, X2, X3, X4, A1),
                         Tuple4<P1, P2, P3, P4>, Tuple1<A1> >
          (function, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple1<A1> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename A1, typename X1, typename X2,
          typename X3, typename X4>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(T** obj, R (U::*method)(X1, X2, X3, X4, A1), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple1<A1> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, X2, X3, X4, A1),
                               Tuple4<P1, P2, P3, P4>, Tuple1<A1> >
          (obj, method, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple1<A1> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename A1, typename X1, typename X2,
          typename X3, typename X4>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, X2, X3, X4, A1),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple1<A1> >* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, X2, X3, X4, A1),
                 Tuple4<P1, P2, P3, P4>, Tuple1<A1> >
          (obj, method, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple1<A1> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename A1, typename X1, typename X2, typename X3, typename X4>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(R (__stdcall *function)(X1, X2, X3, X4, A1), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple1<A1> >* t =
      new MutantFunction<R, R (__stdcall *)(X1, X2, X3, X4, A1),
                         Tuple4<P1, P2, P3, P4>, Tuple1<A1> >
          (function, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple1<A1> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename A1, typename X1, typename X2,
          typename X3, typename X4>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, X2, X3, X4, A1),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple1<A1> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, X2, X3, X4, A1),
                               Tuple4<P1, P2, P3, P4>, Tuple1<A1> >
          (obj, method, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple1<A1> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename A1, typename A2, typename X1,
          typename X2, typename X3, typename X4>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(T* obj, R (U::*method)(X1, X2, X3, X4, A1, A2), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new Mutant<R, T, R (U::*)(X1, X2, X3, X4, A1, A2),
                 Tuple4<P1, P2, P3, P4>, Tuple2<A1, A2> >
          (obj, method, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename A1, typename A2, typename X1, typename X2, typename X3,
          typename X4>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(R (*function)(X1, X2, X3, X4, A1, A2), const P1& p1, const P2& p2,
    const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new MutantFunction<R, R (*)(X1, X2, X3, X4, A1, A2),
                         Tuple4<P1, P2, P3, P4>, Tuple2<A1, A2> >
          (function, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename A1, typename A2, typename X1,
          typename X2, typename X3, typename X4>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(T** obj, R (U::*method)(X1, X2, X3, X4, A1, A2), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, X2, X3, X4, A1, A2),
                               Tuple4<P1, P2, P3, P4>, Tuple2<A1, A2> >
          (obj, method, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename A1, typename A2, typename X1,
          typename X2, typename X3, typename X4>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, X2, X3, X4, A1, A2),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, X2, X3, X4, A1, A2),
                 Tuple4<P1, P2, P3, P4>, Tuple2<A1, A2> >
          (obj, method, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename A1, typename A2, typename X1, typename X2, typename X3,
          typename X4>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(R (__stdcall *function)(X1, X2, X3, X4, A1, A2), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new MutantFunction<R, R (__stdcall *)(X1, X2, X3, X4, A1, A2),
                         Tuple4<P1, P2, P3, P4>, Tuple2<A1, A2> >
          (function, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename A1, typename A2, typename X1,
          typename X2, typename X3, typename X4>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, X2, X3, X4, A1, A2),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, X2, X3, X4, A1, A2),
                               Tuple4<P1, P2, P3, P4>, Tuple2<A1, A2> >
          (obj, method, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename A1, typename A2, typename A3,
          typename X1, typename X2, typename X3, typename X4>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(T* obj, R (U::*method)(X1, X2, X3, X4, A1, A2, A3), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new Mutant<R, T, R (U::*)(X1, X2, X3, X4, A1, A2, A3),
                 Tuple4<P1, P2, P3, P4>, Tuple3<A1, A2, A3> >
          (obj, method, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename A1, typename A2, typename A3, typename X1, typename X2,
          typename X3, typename X4>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(R (*function)(X1, X2, X3, X4, A1, A2, A3), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new MutantFunction<R, R (*)(X1, X2, X3, X4, A1, A2, A3),
                         Tuple4<P1, P2, P3, P4>, Tuple3<A1, A2, A3> >
          (function, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename A1, typename A2, typename A3,
          typename X1, typename X2, typename X3, typename X4>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(T** obj, R (U::*method)(X1, X2, X3, X4, A1, A2, A3), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, X2, X3, X4, A1, A2, A3),
                               Tuple4<P1, P2, P3, P4>, Tuple3<A1, A2, A3> >
          (obj, method, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename A1, typename A2, typename A3,
          typename X1, typename X2, typename X3, typename X4>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, X2, X3, X4, A1, A2, A3),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, X2, X3, X4, A1, A2, A3),
                 Tuple4<P1, P2, P3, P4>, Tuple3<A1, A2, A3> >
          (obj, method, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename A1, typename A2, typename A3, typename X1, typename X2,
          typename X3, typename X4>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(R (__stdcall *function)(X1, X2, X3, X4, A1, A2, A3), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new MutantFunction<R, R (__stdcall *)(X1, X2, X3, X4, A1, A2, A3),
                         Tuple4<P1, P2, P3, P4>, Tuple3<A1, A2, A3> >
          (function, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename A1, typename A2, typename A3,
          typename X1, typename X2, typename X3, typename X4>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, X2, X3, X4, A1, A2, A3),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, X2, X3, X4, A1, A2, A3),
                               Tuple4<P1, P2, P3, P4>, Tuple3<A1, A2, A3> >
          (obj, method, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename A1, typename A2, typename A3,
          typename A4, typename X1, typename X2, typename X3, typename X4>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(T* obj, R (U::*method)(X1, X2, X3, X4, A1, A2, A3, A4),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new Mutant<R, T, R (U::*)(X1, X2, X3, X4, A1, A2, A3, A4),
                 Tuple4<P1, P2, P3, P4>, Tuple4<A1, A2, A3, A4> >
          (obj, method, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename A1, typename A2, typename A3, typename A4, typename X1,
          typename X2, typename X3, typename X4>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(R (*function)(X1, X2, X3, X4, A1, A2, A3, A4), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new MutantFunction<R, R (*)(X1, X2, X3, X4, A1, A2, A3, A4),
                         Tuple4<P1, P2, P3, P4>, Tuple4<A1, A2, A3, A4> >
          (function, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename A1, typename A2, typename A3,
          typename A4, typename X1, typename X2, typename X3, typename X4>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(T** obj, R (U::*method)(X1, X2, X3, X4, A1, A2, A3, A4),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, X2, X3, X4, A1, A2, A3, A4),
                               Tuple4<P1, P2, P3, P4>, Tuple4<A1, A2, A3, A4> >
          (obj, method, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename A1, typename A2, typename A3,
          typename A4, typename X1, typename X2, typename X3, typename X4>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, X2, X3, X4, A1, A2, A3, A4),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, X2, X3, X4, A1, A2, A3, A4),
                 Tuple4<P1, P2, P3, P4>, Tuple4<A1, A2, A3, A4> >
          (obj, method, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename A1, typename A2, typename A3, typename A4, typename X1,
          typename X2, typename X3, typename X4>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(R (__stdcall *function)(X1, X2, X3, X4, A1, A2, A3, A4),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new MutantFunction<R, R (__stdcall *)(X1, X2, X3, X4, A1, A2, A3, A4),
                         Tuple4<P1, P2, P3, P4>, Tuple4<A1, A2, A3, A4> >
          (function, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename A1, typename A2, typename A3,
          typename A4, typename X1, typename X2, typename X3, typename X4>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, X2, X3, X4, A1, A2, A3, A4),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, X2, X3, X4, A1, A2, A3, A4),
                               Tuple4<P1, P2, P3, P4>, Tuple4<A1, A2, A3, A4> >
          (obj, method, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename A1, typename A2, typename A3,
          typename A4, typename A5, typename X1, typename X2, typename X3,
          typename X4>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(T* obj, R (U::*method)(X1, X2, X3, X4, A1, A2, A3, A4, A5),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new Mutant<R, T, R (U::*)(X1, X2, X3, X4, A1, A2, A3, A4, A5),
                 Tuple4<P1, P2, P3, P4>, Tuple5<A1, A2, A3, A4, A5> >
          (obj, method, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename A1, typename A2, typename A3, typename A4, typename A5,
          typename X1, typename X2, typename X3, typename X4>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(R (*function)(X1, X2, X3, X4, A1, A2, A3, A4, A5), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new MutantFunction<R, R (*)(X1, X2, X3, X4, A1, A2, A3, A4, A5),
                         Tuple4<P1, P2, P3, P4>, Tuple5<A1, A2, A3, A4, A5> >
          (function, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename A1, typename A2, typename A3,
          typename A4, typename A5, typename X1, typename X2, typename X3,
          typename X4>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(T** obj, R (U::*method)(X1, X2, X3, X4, A1, A2, A3, A4, A5),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, X2, X3, X4, A1, A2, A3, A4, A5),
                               Tuple4<P1, P2, P3, P4>, Tuple5<A1, A2, A3, A4, A5> >
          (obj, method, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename A1, typename A2, typename A3,
          typename A4, typename A5, typename X1, typename X2, typename X3,
          typename X4>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, X2, X3, X4, A1, A2, A3, A4,
    A5), const P1& p1, const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, X2, X3, X4, A1, A2, A3, A4, A5),
                 Tuple4<P1, P2, P3, P4>, Tuple5<A1, A2, A3, A4, A5> >
          (obj, method, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename A1, typename A2, typename A3, typename A4, typename A5,
          typename X1, typename X2, typename X3, typename X4>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(R (__stdcall *function)(X1, X2, X3, X4, A1, A2, A3, A4, A5),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new MutantFunction<R, R (__stdcall *)(X1, X2, X3, X4, A1, A2, A3, A4, A5),
                         Tuple4<P1, P2, P3, P4>, Tuple5<A1, A2, A3, A4, A5> >
          (function, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename A1, typename A2, typename A3,
          typename A4, typename A5, typename X1, typename X2, typename X3,
          typename X4>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, X2, X3, X4, A1, A2, A3, A4,
    A5), const P1& p1, const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, X2, X3, X4, A1, A2, A3, A4, A5),
                               Tuple4<P1, P2, P3, P4>, Tuple5<A1, A2, A3, A4, A5> >
          (obj, method, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename A1, typename A2, typename A3,
          typename A4, typename A5, typename A6, typename X1, typename X2,
          typename X3, typename X4>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(T* obj, R (U::*method)(X1, X2, X3, X4, A1, A2, A3, A4, A5, A6),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new Mutant<R, T, R (U::*)(X1, X2, X3, X4, A1, A2, A3, A4, A5, A6),
                 Tuple4<P1, P2, P3, P4>, Tuple6<A1, A2, A3, A4, A5, A6> >
          (obj, method, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename A1, typename A2, typename A3, typename A4, typename A5,
          typename A6, typename X1, typename X2, typename X3, typename X4>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(R (*function)(X1, X2, X3, X4, A1, A2, A3, A4, A5, A6),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new MutantFunction<R, R (*)(X1, X2, X3, X4, A1, A2, A3, A4, A5, A6),
                         Tuple4<P1, P2, P3, P4>, Tuple6<A1, A2, A3, A4, A5, A6> >
          (function, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename A1, typename A2, typename A3,
          typename A4, typename A5, typename A6, typename X1, typename X2,
          typename X3, typename X4>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(T** obj, R (U::*method)(X1, X2, X3, X4, A1, A2, A3, A4, A5, A6),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, X2, X3, X4, A1, A2, A3, A4, A5, A6),
                               Tuple4<P1, P2, P3, P4>, Tuple6<A1, A2, A3, A4, A5, A6> >
          (obj, method, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename A1, typename A2, typename A3,
          typename A4, typename A5, typename A6, typename X1, typename X2,
          typename X3, typename X4>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, X2, X3, X4, A1, A2, A3, A4,
    A5, A6), const P1& p1, const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, X2, X3, X4, A1, A2, A3, A4, A5, A6),
                 Tuple4<P1, P2, P3, P4>, Tuple6<A1, A2, A3, A4, A5, A6> >
          (obj, method, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename A1, typename A2, typename A3, typename A4, typename A5,
          typename A6, typename X1, typename X2, typename X3, typename X4>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(R (__stdcall *function)(X1, X2, X3, X4, A1, A2, A3, A4, A5, A6),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new MutantFunction<R, R (__stdcall *)(X1, X2, X3, X4, A1, A2, A3, A4, A5, A6),
                         Tuple4<P1, P2, P3, P4>, Tuple6<A1, A2, A3, A4, A5, A6> >
          (function, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename A1, typename A2, typename A3,
          typename A4, typename A5, typename A6, typename X1, typename X2,
          typename X3, typename X4>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, X2, X3, X4, A1, A2, A3, A4,
    A5, A6), const P1& p1, const P2& p2, const P3& p3, const P4& p4) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, X2, X3, X4, A1, A2, A3, A4, A5, A6),
                               Tuple4<P1, P2, P3, P4>, Tuple6<A1, A2, A3, A4, A5, A6> >
          (obj, method, MakeTuple(p1, p2, p3, p4));
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename X1, typename X2,
          typename X3, typename X4, typename X5>
inline MutantFunctor<R, Tuple0>
CreateFunctor(T* obj, R (U::*method)(X1, X2, X3, X4, X5), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4, const P5& p5) {
  MutantRunner<R, Tuple0>* t =
      new Mutant<R, T, R (U::*)(X1, X2, X3, X4, X5),
                 Tuple5<P1, P2, P3, P4, P5>, Tuple0>
          (obj, method, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple0>(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename P5, typename X1, typename X2, typename X3, typename X4,
          typename X5>
inline MutantFunctor<R, Tuple0>
CreateFunctor(R (*function)(X1, X2, X3, X4, X5), const P1& p1, const P2& p2,
    const P3& p3, const P4& p4, const P5& p5) {
  MutantRunner<R, Tuple0>* t =
      new MutantFunction<R, R (*)(X1, X2, X3, X4, X5),
                         Tuple5<P1, P2, P3, P4, P5>, Tuple0>
          (function, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple0>(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename X1, typename X2,
          typename X3, typename X4, typename X5>
inline MutantFunctor<R, Tuple0>
CreateFunctor(T** obj, R (U::*method)(X1, X2, X3, X4, X5), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4, const P5& p5) {
  MutantRunner<R, Tuple0>* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, X2, X3, X4, X5),
                               Tuple5<P1, P2, P3, P4, P5>, Tuple0>
          (obj, method, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple0>(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename X1, typename X2,
          typename X3, typename X4, typename X5>
inline MutantFunctor<R, Tuple0>
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, X2, X3, X4, X5),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5) {
  MutantRunner<R, Tuple0>* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, X2, X3, X4, X5),
                 Tuple5<P1, P2, P3, P4, P5>, Tuple0>
          (obj, method, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple0>(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename P5, typename X1, typename X2, typename X3, typename X4,
          typename X5>
inline MutantFunctor<R, Tuple0>
CreateFunctor(R (__stdcall *function)(X1, X2, X3, X4, X5), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4, const P5& p5) {
  MutantRunner<R, Tuple0>* t =
      new MutantFunction<R, R (__stdcall *)(X1, X2, X3, X4, X5),
                         Tuple5<P1, P2, P3, P4, P5>, Tuple0>
          (function, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple0>(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename X1, typename X2,
          typename X3, typename X4, typename X5>
inline MutantFunctor<R, Tuple0>
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, X2, X3, X4, X5),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5) {
  MutantRunner<R, Tuple0>* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, X2, X3, X4, X5),
                               Tuple5<P1, P2, P3, P4, P5>, Tuple0>
          (obj, method, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple0>(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename A1, typename X1,
          typename X2, typename X3, typename X4, typename X5>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(T* obj, R (U::*method)(X1, X2, X3, X4, X5, A1), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4, const P5& p5) {
  MutantRunner<R, Tuple1<A1> >* t =
      new Mutant<R, T, R (U::*)(X1, X2, X3, X4, X5, A1),
                 Tuple5<P1, P2, P3, P4, P5>, Tuple1<A1> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple1<A1> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename P5, typename A1, typename X1, typename X2, typename X3,
          typename X4, typename X5>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(R (*function)(X1, X2, X3, X4, X5, A1), const P1& p1, const P2& p2,
    const P3& p3, const P4& p4, const P5& p5) {
  MutantRunner<R, Tuple1<A1> >* t =
      new MutantFunction<R, R (*)(X1, X2, X3, X4, X5, A1),
                         Tuple5<P1, P2, P3, P4, P5>, Tuple1<A1> >
          (function, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple1<A1> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename A1, typename X1,
          typename X2, typename X3, typename X4, typename X5>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(T** obj, R (U::*method)(X1, X2, X3, X4, X5, A1), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4, const P5& p5) {
  MutantRunner<R, Tuple1<A1> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, X2, X3, X4, X5, A1),
                               Tuple5<P1, P2, P3, P4, P5>, Tuple1<A1> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple1<A1> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename A1, typename X1,
          typename X2, typename X3, typename X4, typename X5>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, X2, X3, X4, X5, A1),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5) {
  MutantRunner<R, Tuple1<A1> >* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, X2, X3, X4, X5, A1),
                 Tuple5<P1, P2, P3, P4, P5>, Tuple1<A1> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple1<A1> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename P5, typename A1, typename X1, typename X2, typename X3,
          typename X4, typename X5>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(R (__stdcall *function)(X1, X2, X3, X4, X5, A1), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4, const P5& p5) {
  MutantRunner<R, Tuple1<A1> >* t =
      new MutantFunction<R, R (__stdcall *)(X1, X2, X3, X4, X5, A1),
                         Tuple5<P1, P2, P3, P4, P5>, Tuple1<A1> >
          (function, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple1<A1> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename A1, typename X1,
          typename X2, typename X3, typename X4, typename X5>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, X2, X3, X4, X5, A1),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5) {
  MutantRunner<R, Tuple1<A1> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, X2, X3, X4, X5, A1),
                               Tuple5<P1, P2, P3, P4, P5>, Tuple1<A1> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple1<A1> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename A1, typename A2,
          typename X1, typename X2, typename X3, typename X4, typename X5>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(T* obj, R (U::*method)(X1, X2, X3, X4, X5, A1, A2), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4, const P5& p5) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new Mutant<R, T, R (U::*)(X1, X2, X3, X4, X5, A1, A2),
                 Tuple5<P1, P2, P3, P4, P5>, Tuple2<A1, A2> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename P5, typename A1, typename A2, typename X1, typename X2,
          typename X3, typename X4, typename X5>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(R (*function)(X1, X2, X3, X4, X5, A1, A2), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4, const P5& p5) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new MutantFunction<R, R (*)(X1, X2, X3, X4, X5, A1, A2),
                         Tuple5<P1, P2, P3, P4, P5>, Tuple2<A1, A2> >
          (function, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename A1, typename A2,
          typename X1, typename X2, typename X3, typename X4, typename X5>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(T** obj, R (U::*method)(X1, X2, X3, X4, X5, A1, A2), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4, const P5& p5) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, X2, X3, X4, X5, A1, A2),
                               Tuple5<P1, P2, P3, P4, P5>, Tuple2<A1, A2> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename A1, typename A2,
          typename X1, typename X2, typename X3, typename X4, typename X5>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, X2, X3, X4, X5, A1, A2),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, X2, X3, X4, X5, A1, A2),
                 Tuple5<P1, P2, P3, P4, P5>, Tuple2<A1, A2> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename P5, typename A1, typename A2, typename X1, typename X2,
          typename X3, typename X4, typename X5>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(R (__stdcall *function)(X1, X2, X3, X4, X5, A1, A2), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4, const P5& p5) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new MutantFunction<R, R (__stdcall *)(X1, X2, X3, X4, X5, A1, A2),
                         Tuple5<P1, P2, P3, P4, P5>, Tuple2<A1, A2> >
          (function, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename A1, typename A2,
          typename X1, typename X2, typename X3, typename X4, typename X5>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, X2, X3, X4, X5, A1, A2),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, X2, X3, X4, X5, A1, A2),
                               Tuple5<P1, P2, P3, P4, P5>, Tuple2<A1, A2> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename A1, typename A2,
          typename A3, typename X1, typename X2, typename X3, typename X4,
          typename X5>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(T* obj, R (U::*method)(X1, X2, X3, X4, X5, A1, A2, A3),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new Mutant<R, T, R (U::*)(X1, X2, X3, X4, X5, A1, A2, A3),
                 Tuple5<P1, P2, P3, P4, P5>, Tuple3<A1, A2, A3> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename P5, typename A1, typename A2, typename A3, typename X1,
          typename X2, typename X3, typename X4, typename X5>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(R (*function)(X1, X2, X3, X4, X5, A1, A2, A3), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4, const P5& p5) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new MutantFunction<R, R (*)(X1, X2, X3, X4, X5, A1, A2, A3),
                         Tuple5<P1, P2, P3, P4, P5>, Tuple3<A1, A2, A3> >
          (function, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename A1, typename A2,
          typename A3, typename X1, typename X2, typename X3, typename X4,
          typename X5>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(T** obj, R (U::*method)(X1, X2, X3, X4, X5, A1, A2, A3),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, X2, X3, X4, X5, A1, A2, A3),
                               Tuple5<P1, P2, P3, P4, P5>, Tuple3<A1, A2, A3> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename A1, typename A2,
          typename A3, typename X1, typename X2, typename X3, typename X4,
          typename X5>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, X2, X3, X4, X5, A1, A2, A3),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, X2, X3, X4, X5, A1, A2, A3),
                 Tuple5<P1, P2, P3, P4, P5>, Tuple3<A1, A2, A3> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename P5, typename A1, typename A2, typename A3, typename X1,
          typename X2, typename X3, typename X4, typename X5>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(R (__stdcall *function)(X1, X2, X3, X4, X5, A1, A2, A3),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new MutantFunction<R, R (__stdcall *)(X1, X2, X3, X4, X5, A1, A2, A3),
                         Tuple5<P1, P2, P3, P4, P5>, Tuple3<A1, A2, A3> >
          (function, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename A1, typename A2,
          typename A3, typename X1, typename X2, typename X3, typename X4,
          typename X5>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, X2, X3, X4, X5, A1, A2, A3),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, X2, X3, X4, X5, A1, A2, A3),
                               Tuple5<P1, P2, P3, P4, P5>, Tuple3<A1, A2, A3> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename A1, typename A2,
          typename A3, typename A4, typename X1, typename X2, typename X3,
          typename X4, typename X5>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(T* obj, R (U::*method)(X1, X2, X3, X4, X5, A1, A2, A3, A4),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new Mutant<R, T, R (U::*)(X1, X2, X3, X4, X5, A1, A2, A3, A4),
                 Tuple5<P1, P2, P3, P4, P5>, Tuple4<A1, A2, A3, A4> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename P5, typename A1, typename A2, typename A3, typename A4,
          typename X1, typename X2, typename X3, typename X4, typename X5>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(R (*function)(X1, X2, X3, X4, X5, A1, A2, A3, A4), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4, const P5& p5) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new MutantFunction<R, R (*)(X1, X2, X3, X4, X5, A1, A2, A3, A4),
                         Tuple5<P1, P2, P3, P4, P5>, Tuple4<A1, A2, A3, A4> >
          (function, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename A1, typename A2,
          typename A3, typename A4, typename X1, typename X2, typename X3,
          typename X4, typename X5>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(T** obj, R (U::*method)(X1, X2, X3, X4, X5, A1, A2, A3, A4),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, X2, X3, X4, X5, A1, A2, A3, A4),
                               Tuple5<P1, P2, P3, P4, P5>, Tuple4<A1, A2, A3, A4> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename A1, typename A2,
          typename A3, typename A4, typename X1, typename X2, typename X3,
          typename X4, typename X5>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, X2, X3, X4, X5, A1, A2, A3,
    A4), const P1& p1, const P2& p2, const P3& p3, const P4& p4,
    const P5& p5) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, X2, X3, X4, X5, A1, A2, A3, A4),
                 Tuple5<P1, P2, P3, P4, P5>, Tuple4<A1, A2, A3, A4> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename P5, typename A1, typename A2, typename A3, typename A4,
          typename X1, typename X2, typename X3, typename X4, typename X5>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(R (__stdcall *function)(X1, X2, X3, X4, X5, A1, A2, A3, A4),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new MutantFunction<R, R (__stdcall *)(X1, X2, X3, X4, X5, A1, A2, A3, A4),
                         Tuple5<P1, P2, P3, P4, P5>, Tuple4<A1, A2, A3, A4> >
          (function, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename A1, typename A2,
          typename A3, typename A4, typename X1, typename X2, typename X3,
          typename X4, typename X5>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, X2, X3, X4, X5, A1, A2, A3,
    A4), const P1& p1, const P2& p2, const P3& p3, const P4& p4,
    const P5& p5) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, X2, X3, X4, X5, A1, A2, A3, A4),
                               Tuple5<P1, P2, P3, P4, P5>, Tuple4<A1, A2, A3, A4> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename A1, typename A2,
          typename A3, typename A4, typename A5, typename X1, typename X2,
          typename X3, typename X4, typename X5>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(T* obj, R (U::*method)(X1, X2, X3, X4, X5, A1, A2, A3, A4, A5),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new Mutant<R, T, R (U::*)(X1, X2, X3, X4, X5, A1, A2, A3, A4, A5),
                 Tuple5<P1, P2, P3, P4, P5>, Tuple5<A1, A2, A3, A4, A5> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename P5, typename A1, typename A2, typename A3, typename A4,
          typename A5, typename X1, typename X2, typename X3, typename X4,
          typename X5>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(R (*function)(X1, X2, X3, X4, X5, A1, A2, A3, A4, A5),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new MutantFunction<R, R (*)(X1, X2, X3, X4, X5, A1, A2, A3, A4, A5),
                         Tuple5<P1, P2, P3, P4, P5>, Tuple5<A1, A2, A3, A4, A5> >
          (function, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename A1, typename A2,
          typename A3, typename A4, typename A5, typename X1, typename X2,
          typename X3, typename X4, typename X5>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(T** obj, R (U::*method)(X1, X2, X3, X4, X5, A1, A2, A3, A4, A5),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, X2, X3, X4, X5, A1, A2, A3, A4, A5),
                               Tuple5<P1, P2, P3, P4, P5>, Tuple5<A1, A2, A3, A4, A5> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename A1, typename A2,
          typename A3, typename A4, typename A5, typename X1, typename X2,
          typename X3, typename X4, typename X5>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, X2, X3, X4, X5, A1, A2, A3,
    A4, A5), const P1& p1, const P2& p2, const P3& p3, const P4& p4,
    const P5& p5) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, X2, X3, X4, X5, A1, A2, A3, A4, A5),
                 Tuple5<P1, P2, P3, P4, P5>, Tuple5<A1, A2, A3, A4, A5> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename P5, typename A1, typename A2, typename A3, typename A4,
          typename A5, typename X1, typename X2, typename X3, typename X4,
          typename X5>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(R (__stdcall *function)(X1, X2, X3, X4, X5, A1, A2, A3, A4, A5),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new MutantFunction<R, R (__stdcall *)(X1, X2, X3, X4, X5, A1, A2, A3, A4, A5),
                         Tuple5<P1, P2, P3, P4, P5>, Tuple5<A1, A2, A3, A4, A5> >
          (function, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename A1, typename A2,
          typename A3, typename A4, typename A5, typename X1, typename X2,
          typename X3, typename X4, typename X5>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, X2, X3, X4, X5, A1, A2, A3,
    A4, A5), const P1& p1, const P2& p2, const P3& p3, const P4& p4,
    const P5& p5) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, X2, X3, X4, X5, A1, A2, A3, A4, A5),
                               Tuple5<P1, P2, P3, P4, P5>, Tuple5<A1, A2, A3, A4, A5> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename A1, typename A2,
          typename A3, typename A4, typename A5, typename A6, typename X1,
          typename X2, typename X3, typename X4, typename X5>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(T* obj, R (U::*method)(X1, X2, X3, X4, X5, A1, A2, A3, A4, A5,
    A6), const P1& p1, const P2& p2, const P3& p3, const P4& p4,
    const P5& p5) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new Mutant<R, T, R (U::*)(X1, X2, X3, X4, X5, A1, A2, A3, A4, A5, A6),
                 Tuple5<P1, P2, P3, P4, P5>, Tuple6<A1, A2, A3, A4, A5, A6> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename P5, typename A1, typename A2, typename A3, typename A4,
          typename A5, typename A6, typename X1, typename X2, typename X3,
          typename X4, typename X5>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(R (*function)(X1, X2, X3, X4, X5, A1, A2, A3, A4, A5, A6),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new MutantFunction<R, R (*)(X1, X2, X3, X4, X5, A1, A2, A3, A4, A5, A6),
                         Tuple5<P1, P2, P3, P4, P5>, Tuple6<A1, A2, A3, A4, A5, A6> >
          (function, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename A1, typename A2,
          typename A3, typename A4, typename A5, typename A6, typename X1,
          typename X2, typename X3, typename X4, typename X5>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(T** obj, R (U::*method)(X1, X2, X3, X4, X5, A1, A2, A3, A4, A5,
    A6), const P1& p1, const P2& p2, const P3& p3, const P4& p4,
    const P5& p5) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, X2, X3, X4, X5, A1, A2, A3, A4, A5, A6),
                               Tuple5<P1, P2, P3, P4, P5>, Tuple6<A1, A2, A3, A4, A5, A6> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename A1, typename A2,
          typename A3, typename A4, typename A5, typename A6, typename X1,
          typename X2, typename X3, typename X4, typename X5>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, X2, X3, X4, X5, A1, A2, A3,
    A4, A5, A6), const P1& p1, const P2& p2, const P3& p3, const P4& p4,
    const P5& p5) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, X2, X3, X4, X5, A1, A2, A3, A4, A5, A6),
                 Tuple5<P1, P2, P3, P4, P5>, Tuple6<A1, A2, A3, A4, A5, A6> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename P5, typename A1, typename A2, typename A3, typename A4,
          typename A5, typename A6, typename X1, typename X2, typename X3,
          typename X4, typename X5>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(R (__stdcall *function)(X1, X2, X3, X4, X5, A1, A2, A3, A4, A5,
    A6), const P1& p1, const P2& p2, const P3& p3, const P4& p4,
    const P5& p5) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new MutantFunction<R, R (__stdcall *)(X1, X2, X3, X4, X5, A1, A2, A3, A4, A5, A6),
                         Tuple5<P1, P2, P3, P4, P5>, Tuple6<A1, A2, A3, A4, A5, A6> >
          (function, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename A1, typename A2,
          typename A3, typename A4, typename A5, typename A6, typename X1,
          typename X2, typename X3, typename X4, typename X5>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, X2, X3, X4, X5, A1, A2, A3,
    A4, A5, A6), const P1& p1, const P2& p2, const P3& p3, const P4& p4,
    const P5& p5) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, X2, X3, X4, X5, A1, A2, A3, A4, A5, A6),
                               Tuple5<P1, P2, P3, P4, P5>, Tuple6<A1, A2, A3, A4, A5, A6> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5));
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename P6, typename X1,
          typename X2, typename X3, typename X4, typename X5, typename X6>
inline MutantFunctor<R, Tuple0>
CreateFunctor(T* obj, R (U::*method)(X1, X2, X3, X4, X5, X6), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4, const P5& p5, const P6& p6) {
  MutantRunner<R, Tuple0>* t =
      new Mutant<R, T, R (U::*)(X1, X2, X3, X4, X5, X6),
                 Tuple6<P1, P2, P3, P4, P5, P6>, Tuple0>
          (obj, method, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple0>(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename P5, typename P6, typename X1, typename X2, typename X3,
          typename X4, typename X5, typename X6>
inline MutantFunctor<R, Tuple0>
CreateFunctor(R (*function)(X1, X2, X3, X4, X5, X6), const P1& p1, const P2& p2,
    const P3& p3, const P4& p4, const P5& p5, const P6& p6) {
  MutantRunner<R, Tuple0>* t =
      new MutantFunction<R, R (*)(X1, X2, X3, X4, X5, X6),
                         Tuple6<P1, P2, P3, P4, P5, P6>, Tuple0>
          (function, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple0>(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename P6, typename X1,
          typename X2, typename X3, typename X4, typename X5, typename X6>
inline MutantFunctor<R, Tuple0>
CreateFunctor(T** obj, R (U::*method)(X1, X2, X3, X4, X5, X6), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4, const P5& p5, const P6& p6) {
  MutantRunner<R, Tuple0>* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, X2, X3, X4, X5, X6),
                               Tuple6<P1, P2, P3, P4, P5, P6>, Tuple0>
          (obj, method, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple0>(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename P6, typename X1,
          typename X2, typename X3, typename X4, typename X5, typename X6>
inline MutantFunctor<R, Tuple0>
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, X2, X3, X4, X5, X6),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5,
    const P6& p6) {
  MutantRunner<R, Tuple0>* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, X2, X3, X4, X5, X6),
                 Tuple6<P1, P2, P3, P4, P5, P6>, Tuple0>
          (obj, method, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple0>(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename P5, typename P6, typename X1, typename X2, typename X3,
          typename X4, typename X5, typename X6>
inline MutantFunctor<R, Tuple0>
CreateFunctor(R (__stdcall *function)(X1, X2, X3, X4, X5, X6), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4, const P5& p5, const P6& p6) {
  MutantRunner<R, Tuple0>* t =
      new MutantFunction<R, R (__stdcall *)(X1, X2, X3, X4, X5, X6),
                         Tuple6<P1, P2, P3, P4, P5, P6>, Tuple0>
          (function, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple0>(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename P6, typename X1,
          typename X2, typename X3, typename X4, typename X5, typename X6>
inline MutantFunctor<R, Tuple0>
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, X2, X3, X4, X5, X6),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5,
    const P6& p6) {
  MutantRunner<R, Tuple0>* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, X2, X3, X4, X5, X6),
                               Tuple6<P1, P2, P3, P4, P5, P6>, Tuple0>
          (obj, method, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple0>(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename P6, typename A1,
          typename X1, typename X2, typename X3, typename X4, typename X5,
          typename X6>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(T* obj, R (U::*method)(X1, X2, X3, X4, X5, X6, A1), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4, const P5& p5, const P6& p6) {
  MutantRunner<R, Tuple1<A1> >* t =
      new Mutant<R, T, R (U::*)(X1, X2, X3, X4, X5, X6, A1),
                 Tuple6<P1, P2, P3, P4, P5, P6>, Tuple1<A1> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple1<A1> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename P5, typename P6, typename A1, typename X1, typename X2,
          typename X3, typename X4, typename X5, typename X6>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(R (*function)(X1, X2, X3, X4, X5, X6, A1), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4, const P5& p5, const P6& p6) {
  MutantRunner<R, Tuple1<A1> >* t =
      new MutantFunction<R, R (*)(X1, X2, X3, X4, X5, X6, A1),
                         Tuple6<P1, P2, P3, P4, P5, P6>, Tuple1<A1> >
          (function, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple1<A1> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename P6, typename A1,
          typename X1, typename X2, typename X3, typename X4, typename X5,
          typename X6>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(T** obj, R (U::*method)(X1, X2, X3, X4, X5, X6, A1), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4, const P5& p5, const P6& p6) {
  MutantRunner<R, Tuple1<A1> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, X2, X3, X4, X5, X6, A1),
                               Tuple6<P1, P2, P3, P4, P5, P6>, Tuple1<A1> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple1<A1> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename P6, typename A1,
          typename X1, typename X2, typename X3, typename X4, typename X5,
          typename X6>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, X2, X3, X4, X5, X6, A1),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5,
    const P6& p6) {
  MutantRunner<R, Tuple1<A1> >* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, X2, X3, X4, X5, X6, A1),
                 Tuple6<P1, P2, P3, P4, P5, P6>, Tuple1<A1> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple1<A1> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename P5, typename P6, typename A1, typename X1, typename X2,
          typename X3, typename X4, typename X5, typename X6>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(R (__stdcall *function)(X1, X2, X3, X4, X5, X6, A1), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4, const P5& p5, const P6& p6) {
  MutantRunner<R, Tuple1<A1> >* t =
      new MutantFunction<R, R (__stdcall *)(X1, X2, X3, X4, X5, X6, A1),
                         Tuple6<P1, P2, P3, P4, P5, P6>, Tuple1<A1> >
          (function, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple1<A1> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename P6, typename A1,
          typename X1, typename X2, typename X3, typename X4, typename X5,
          typename X6>
inline MutantFunctor<R, Tuple1<A1> >
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, X2, X3, X4, X5, X6, A1),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5,
    const P6& p6) {
  MutantRunner<R, Tuple1<A1> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, X2, X3, X4, X5, X6, A1),
                               Tuple6<P1, P2, P3, P4, P5, P6>, Tuple1<A1> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple1<A1> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename P6, typename A1,
          typename A2, typename X1, typename X2, typename X3, typename X4,
          typename X5, typename X6>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(T* obj, R (U::*method)(X1, X2, X3, X4, X5, X6, A1, A2),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5,
    const P6& p6) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new Mutant<R, T, R (U::*)(X1, X2, X3, X4, X5, X6, A1, A2),
                 Tuple6<P1, P2, P3, P4, P5, P6>, Tuple2<A1, A2> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename P5, typename P6, typename A1, typename A2, typename X1,
          typename X2, typename X3, typename X4, typename X5, typename X6>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(R (*function)(X1, X2, X3, X4, X5, X6, A1, A2), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4, const P5& p5, const P6& p6) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new MutantFunction<R, R (*)(X1, X2, X3, X4, X5, X6, A1, A2),
                         Tuple6<P1, P2, P3, P4, P5, P6>, Tuple2<A1, A2> >
          (function, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename P6, typename A1,
          typename A2, typename X1, typename X2, typename X3, typename X4,
          typename X5, typename X6>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(T** obj, R (U::*method)(X1, X2, X3, X4, X5, X6, A1, A2),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5,
    const P6& p6) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, X2, X3, X4, X5, X6, A1, A2),
                               Tuple6<P1, P2, P3, P4, P5, P6>, Tuple2<A1, A2> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename P6, typename A1,
          typename A2, typename X1, typename X2, typename X3, typename X4,
          typename X5, typename X6>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, X2, X3, X4, X5, X6, A1, A2),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5,
    const P6& p6) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, X2, X3, X4, X5, X6, A1, A2),
                 Tuple6<P1, P2, P3, P4, P5, P6>, Tuple2<A1, A2> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename P5, typename P6, typename A1, typename A2, typename X1,
          typename X2, typename X3, typename X4, typename X5, typename X6>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(R (__stdcall *function)(X1, X2, X3, X4, X5, X6, A1, A2),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5,
    const P6& p6) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new MutantFunction<R, R (__stdcall *)(X1, X2, X3, X4, X5, X6, A1, A2),
                         Tuple6<P1, P2, P3, P4, P5, P6>, Tuple2<A1, A2> >
          (function, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename P6, typename A1,
          typename A2, typename X1, typename X2, typename X3, typename X4,
          typename X5, typename X6>
inline MutantFunctor<R, Tuple2<A1, A2> >
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, X2, X3, X4, X5, X6, A1, A2),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5,
    const P6& p6) {
  MutantRunner<R, Tuple2<A1, A2> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, X2, X3, X4, X5, X6, A1, A2),
                               Tuple6<P1, P2, P3, P4, P5, P6>, Tuple2<A1, A2> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple2<A1, A2> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename P6, typename A1,
          typename A2, typename A3, typename X1, typename X2, typename X3,
          typename X4, typename X5, typename X6>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(T* obj, R (U::*method)(X1, X2, X3, X4, X5, X6, A1, A2, A3),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5,
    const P6& p6) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new Mutant<R, T, R (U::*)(X1, X2, X3, X4, X5, X6, A1, A2, A3),
                 Tuple6<P1, P2, P3, P4, P5, P6>, Tuple3<A1, A2, A3> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename P5, typename P6, typename A1, typename A2, typename A3,
          typename X1, typename X2, typename X3, typename X4, typename X5,
          typename X6>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(R (*function)(X1, X2, X3, X4, X5, X6, A1, A2, A3), const P1& p1,
    const P2& p2, const P3& p3, const P4& p4, const P5& p5, const P6& p6) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new MutantFunction<R, R (*)(X1, X2, X3, X4, X5, X6, A1, A2, A3),
                         Tuple6<P1, P2, P3, P4, P5, P6>, Tuple3<A1, A2, A3> >
          (function, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename P6, typename A1,
          typename A2, typename A3, typename X1, typename X2, typename X3,
          typename X4, typename X5, typename X6>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(T** obj, R (U::*method)(X1, X2, X3, X4, X5, X6, A1, A2, A3),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5,
    const P6& p6) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, X2, X3, X4, X5, X6, A1, A2, A3),
                               Tuple6<P1, P2, P3, P4, P5, P6>, Tuple3<A1, A2, A3> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename P6, typename A1,
          typename A2, typename A3, typename X1, typename X2, typename X3,
          typename X4, typename X5, typename X6>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, X2, X3, X4, X5, X6, A1, A2,
    A3), const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5,
    const P6& p6) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, X2, X3, X4, X5, X6, A1, A2, A3),
                 Tuple6<P1, P2, P3, P4, P5, P6>, Tuple3<A1, A2, A3> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename P5, typename P6, typename A1, typename A2, typename A3,
          typename X1, typename X2, typename X3, typename X4, typename X5,
          typename X6>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(R (__stdcall *function)(X1, X2, X3, X4, X5, X6, A1, A2, A3),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5,
    const P6& p6) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new MutantFunction<R, R (__stdcall *)(X1, X2, X3, X4, X5, X6, A1, A2, A3),
                         Tuple6<P1, P2, P3, P4, P5, P6>, Tuple3<A1, A2, A3> >
          (function, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename P6, typename A1,
          typename A2, typename A3, typename X1, typename X2, typename X3,
          typename X4, typename X5, typename X6>
inline MutantFunctor<R, Tuple3<A1, A2, A3> >
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, X2, X3, X4, X5, X6, A1, A2,
    A3), const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5,
    const P6& p6) {
  MutantRunner<R, Tuple3<A1, A2, A3> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, X2, X3, X4, X5, X6, A1, A2, A3),
                               Tuple6<P1, P2, P3, P4, P5, P6>, Tuple3<A1, A2, A3> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple3<A1, A2, A3> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename P6, typename A1,
          typename A2, typename A3, typename A4, typename X1, typename X2,
          typename X3, typename X4, typename X5, typename X6>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(T* obj, R (U::*method)(X1, X2, X3, X4, X5, X6, A1, A2, A3, A4),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5,
    const P6& p6) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new Mutant<R, T, R (U::*)(X1, X2, X3, X4, X5, X6, A1, A2, A3, A4),
                 Tuple6<P1, P2, P3, P4, P5, P6>, Tuple4<A1, A2, A3, A4> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename P5, typename P6, typename A1, typename A2, typename A3,
          typename A4, typename X1, typename X2, typename X3, typename X4,
          typename X5, typename X6>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(R (*function)(X1, X2, X3, X4, X5, X6, A1, A2, A3, A4),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5,
    const P6& p6) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new MutantFunction<R, R (*)(X1, X2, X3, X4, X5, X6, A1, A2, A3, A4),
                         Tuple6<P1, P2, P3, P4, P5, P6>, Tuple4<A1, A2, A3, A4> >
          (function, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename P6, typename A1,
          typename A2, typename A3, typename A4, typename X1, typename X2,
          typename X3, typename X4, typename X5, typename X6>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(T** obj, R (U::*method)(X1, X2, X3, X4, X5, X6, A1, A2, A3, A4),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5,
    const P6& p6) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, X2, X3, X4, X5, X6, A1, A2, A3, A4),
                               Tuple6<P1, P2, P3, P4, P5, P6>, Tuple4<A1, A2, A3, A4> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename P6, typename A1,
          typename A2, typename A3, typename A4, typename X1, typename X2,
          typename X3, typename X4, typename X5, typename X6>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, X2, X3, X4, X5, X6, A1, A2,
    A3, A4), const P1& p1, const P2& p2, const P3& p3, const P4& p4,
    const P5& p5, const P6& p6) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, X2, X3, X4, X5, X6, A1, A2, A3, A4),
                 Tuple6<P1, P2, P3, P4, P5, P6>, Tuple4<A1, A2, A3, A4> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename P5, typename P6, typename A1, typename A2, typename A3,
          typename A4, typename X1, typename X2, typename X3, typename X4,
          typename X5, typename X6>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(R (__stdcall *function)(X1, X2, X3, X4, X5, X6, A1, A2, A3, A4),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5,
    const P6& p6) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new MutantFunction<R, R (__stdcall *)(X1, X2, X3, X4, X5, X6, A1, A2, A3, A4),
                         Tuple6<P1, P2, P3, P4, P5, P6>, Tuple4<A1, A2, A3, A4> >
          (function, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename P6, typename A1,
          typename A2, typename A3, typename A4, typename X1, typename X2,
          typename X3, typename X4, typename X5, typename X6>
inline MutantFunctor<R, Tuple4<A1, A2, A3, A4> >
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, X2, X3, X4, X5, X6, A1, A2,
    A3, A4), const P1& p1, const P2& p2, const P3& p3, const P4& p4,
    const P5& p5, const P6& p6) {
  MutantRunner<R, Tuple4<A1, A2, A3, A4> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, X2, X3, X4, X5, X6, A1, A2, A3, A4),
                               Tuple6<P1, P2, P3, P4, P5, P6>, Tuple4<A1, A2, A3, A4> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple4<A1, A2, A3, A4> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename P6, typename A1,
          typename A2, typename A3, typename A4, typename A5, typename X1,
          typename X2, typename X3, typename X4, typename X5, typename X6>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(T* obj, R (U::*method)(X1, X2, X3, X4, X5, X6, A1, A2, A3, A4,
    A5), const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5,
    const P6& p6) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new Mutant<R, T, R (U::*)(X1, X2, X3, X4, X5, X6, A1, A2, A3, A4, A5),
                 Tuple6<P1, P2, P3, P4, P5, P6>, Tuple5<A1, A2, A3, A4, A5> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename P5, typename P6, typename A1, typename A2, typename A3,
          typename A4, typename A5, typename X1, typename X2, typename X3,
          typename X4, typename X5, typename X6>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(R (*function)(X1, X2, X3, X4, X5, X6, A1, A2, A3, A4, A5),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5,
    const P6& p6) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new MutantFunction<R, R (*)(X1, X2, X3, X4, X5, X6, A1, A2, A3, A4, A5),
                         Tuple6<P1, P2, P3, P4, P5, P6>, Tuple5<A1, A2, A3, A4, A5> >
          (function, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename P6, typename A1,
          typename A2, typename A3, typename A4, typename A5, typename X1,
          typename X2, typename X3, typename X4, typename X5, typename X6>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(T** obj, R (U::*method)(X1, X2, X3, X4, X5, X6, A1, A2, A3, A4,
    A5), const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5,
    const P6& p6) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, X2, X3, X4, X5, X6, A1, A2, A3, A4, A5),
                               Tuple6<P1, P2, P3, P4, P5, P6>, Tuple5<A1, A2, A3, A4, A5> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename P6, typename A1,
          typename A2, typename A3, typename A4, typename A5, typename X1,
          typename X2, typename X3, typename X4, typename X5, typename X6>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, X2, X3, X4, X5, X6, A1, A2,
    A3, A4, A5), const P1& p1, const P2& p2, const P3& p3, const P4& p4,
    const P5& p5, const P6& p6) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, X2, X3, X4, X5, X6, A1, A2, A3, A4, A5),
                 Tuple6<P1, P2, P3, P4, P5, P6>, Tuple5<A1, A2, A3, A4, A5> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename P5, typename P6, typename A1, typename A2, typename A3,
          typename A4, typename A5, typename X1, typename X2, typename X3,
          typename X4, typename X5, typename X6>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(R (__stdcall *function)(X1, X2, X3, X4, X5, X6, A1, A2, A3, A4,
    A5), const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5,
    const P6& p6) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new MutantFunction<R, R (__stdcall *)(X1, X2, X3, X4, X5, X6, A1, A2, A3, A4, A5),
                         Tuple6<P1, P2, P3, P4, P5, P6>, Tuple5<A1, A2, A3, A4, A5> >
          (function, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename P6, typename A1,
          typename A2, typename A3, typename A4, typename A5, typename X1,
          typename X2, typename X3, typename X4, typename X5, typename X6>
inline MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, X2, X3, X4, X5, X6, A1, A2,
    A3, A4, A5), const P1& p1, const P2& p2, const P3& p3, const P4& p4,
    const P5& p5, const P6& p6) {
  MutantRunner<R, Tuple5<A1, A2, A3, A4, A5> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, X2, X3, X4, X5, X6, A1, A2, A3, A4, A5),
                               Tuple6<P1, P2, P3, P4, P5, P6>, Tuple5<A1, A2, A3, A4, A5> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple5<A1, A2, A3, A4, A5> >(t);
}
#endif  
#endif  


template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename P6, typename A1,
          typename A2, typename A3, typename A4, typename A5, typename A6,
          typename X1, typename X2, typename X3, typename X4, typename X5,
          typename X6>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(T* obj, R (U::*method)(X1, X2, X3, X4, X5, X6, A1, A2, A3, A4, A5,
    A6), const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5,
    const P6& p6) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new Mutant<R, T, R (U::*)(X1, X2, X3, X4, X5, X6, A1, A2, A3, A4, A5, A6),
                 Tuple6<P1, P2, P3, P4, P5, P6>, Tuple6<A1, A2, A3, A4, A5, A6> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename P5, typename P6, typename A1, typename A2, typename A3,
          typename A4, typename A5, typename A6, typename X1, typename X2,
          typename X3, typename X4, typename X5, typename X6>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(R (*function)(X1, X2, X3, X4, X5, X6, A1, A2, A3, A4, A5, A6),
    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5,
    const P6& p6) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new MutantFunction<R, R (*)(X1, X2, X3, X4, X5, X6, A1, A2, A3, A4, A5, A6),
                         Tuple6<P1, P2, P3, P4, P5, P6>, Tuple6<A1, A2, A3, A4, A5, A6> >
          (function, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename P6, typename A1,
          typename A2, typename A3, typename A4, typename A5, typename A6,
          typename X1, typename X2, typename X3, typename X4, typename X5,
          typename X6>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(T** obj, R (U::*method)(X1, X2, X3, X4, X5, X6, A1, A2, A3, A4, A5,
    A6), const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5,
    const P6& p6) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new MutantLateObjectBind<R, T, R (U::*)(X1, X2, X3, X4, X5, X6, A1, A2, A3, A4, A5, A6),
                               Tuple6<P1, P2, P3, P4, P5, P6>, Tuple6<A1, A2, A3, A4, A5, A6> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}
#endif  

#if defined (OS_WIN)
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename P6, typename A1,
          typename A2, typename A3, typename A4, typename A5, typename A6,
          typename X1, typename X2, typename X3, typename X4, typename X5,
          typename X6>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(T* obj, R (__stdcall U::*method)(X1, X2, X3, X4, X5, X6, A1, A2,
    A3, A4, A5, A6), const P1& p1, const P2& p2, const P3& p3, const P4& p4,
    const P5& p5, const P6& p6) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new Mutant<R, T, R (__stdcall U::*)(X1, X2, X3, X4, X5, X6, A1, A2, A3, A4, A5, A6),
                 Tuple6<P1, P2, P3, P4, P5, P6>, Tuple6<A1, A2, A3, A4, A5, A6> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}

template <typename R, typename P1, typename P2, typename P3, typename P4,
          typename P5, typename P6, typename A1, typename A2, typename A3,
          typename A4, typename A5, typename A6, typename X1, typename X2,
          typename X3, typename X4, typename X5, typename X6>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(R (__stdcall *function)(X1, X2, X3, X4, X5, X6, A1, A2, A3, A4,
    A5, A6), const P1& p1, const P2& p2, const P3& p3, const P4& p4,
    const P5& p5, const P6& p6) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new MutantFunction<R, R (__stdcall *)(X1, X2, X3, X4, X5, X6, A1, A2, A3, A4, A5, A6),
                         Tuple6<P1, P2, P3, P4, P5, P6>, Tuple6<A1, A2, A3, A4, A5, A6> >
          (function, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}
#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
template <typename R, typename T, typename U, typename P1, typename P2,
          typename P3, typename P4, typename P5, typename P6, typename A1,
          typename A2, typename A3, typename A4, typename A5, typename A6,
          typename X1, typename X2, typename X3, typename X4, typename X5,
          typename X6>
inline MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >
CreateFunctor(T** obj, R (__stdcall U::*method)(X1, X2, X3, X4, X5, X6, A1, A2,
    A3, A4, A5, A6), const P1& p1, const P2& p2, const P3& p3, const P4& p4,
    const P5& p5, const P6& p6) {
  MutantRunner<R, Tuple6<A1, A2, A3, A4, A5, A6> >* t =
      new MutantLateObjectBind<R, T, R (__stdcall U::*)(X1, X2, X3, X4, X5, X6, A1, A2, A3, A4, A5, A6),
                               Tuple6<P1, P2, P3, P4, P5, P6>, Tuple6<A1, A2, A3, A4, A5, A6> >
          (obj, method, MakeTuple(p1, p2, p3, p4, p5, p6));
  return MutantFunctor<R, Tuple6<A1, A2, A3, A4, A5, A6> >(t);
}
#endif  
#endif  

}  

#endif  
