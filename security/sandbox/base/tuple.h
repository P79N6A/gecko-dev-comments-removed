



























#ifndef BASE_TUPLE_H__
#define BASE_TUPLE_H__

#include "base/bind_helpers.h"









template <class P>
struct TupleTraits {
  typedef P ValueType;
  typedef P& RefType;
  typedef const P& ParamType;
};

template <class P>
struct TupleTraits<P&> {
  typedef P ValueType;
  typedef P& RefType;
  typedef P& ParamType;
};

template <class P>
struct TupleTypes { };















struct Tuple0 {
  typedef Tuple0 ValueTuple;
  typedef Tuple0 RefTuple;
  typedef Tuple0 ParamTuple;
};

template <class A>
struct Tuple1 {
 public:
  typedef A TypeA;

  Tuple1() {}
  explicit Tuple1(typename TupleTraits<A>::ParamType a) : a(a) {}

  A a;
};

template <class A, class B>
struct Tuple2 {
 public:
  typedef A TypeA;
  typedef B TypeB;

  Tuple2() {}
  Tuple2(typename TupleTraits<A>::ParamType a,
         typename TupleTraits<B>::ParamType b)
      : a(a), b(b) {
  }

  A a;
  B b;
};

template <class A, class B, class C>
struct Tuple3 {
 public:
  typedef A TypeA;
  typedef B TypeB;
  typedef C TypeC;

  Tuple3() {}
  Tuple3(typename TupleTraits<A>::ParamType a,
         typename TupleTraits<B>::ParamType b,
         typename TupleTraits<C>::ParamType c)
      : a(a), b(b), c(c){
  }

  A a;
  B b;
  C c;
};

template <class A, class B, class C, class D>
struct Tuple4 {
 public:
  typedef A TypeA;
  typedef B TypeB;
  typedef C TypeC;
  typedef D TypeD;

  Tuple4() {}
  Tuple4(typename TupleTraits<A>::ParamType a,
         typename TupleTraits<B>::ParamType b,
         typename TupleTraits<C>::ParamType c,
         typename TupleTraits<D>::ParamType d)
      : a(a), b(b), c(c), d(d) {
  }

  A a;
  B b;
  C c;
  D d;
};

template <class A, class B, class C, class D, class E>
struct Tuple5 {
 public:
  typedef A TypeA;
  typedef B TypeB;
  typedef C TypeC;
  typedef D TypeD;
  typedef E TypeE;

  Tuple5() {}
  Tuple5(typename TupleTraits<A>::ParamType a,
    typename TupleTraits<B>::ParamType b,
    typename TupleTraits<C>::ParamType c,
    typename TupleTraits<D>::ParamType d,
    typename TupleTraits<E>::ParamType e)
    : a(a), b(b), c(c), d(d), e(e) {
  }

  A a;
  B b;
  C c;
  D d;
  E e;
};

template <class A, class B, class C, class D, class E, class F>
struct Tuple6 {
 public:
  typedef A TypeA;
  typedef B TypeB;
  typedef C TypeC;
  typedef D TypeD;
  typedef E TypeE;
  typedef F TypeF;

  Tuple6() {}
  Tuple6(typename TupleTraits<A>::ParamType a,
    typename TupleTraits<B>::ParamType b,
    typename TupleTraits<C>::ParamType c,
    typename TupleTraits<D>::ParamType d,
    typename TupleTraits<E>::ParamType e,
    typename TupleTraits<F>::ParamType f)
    : a(a), b(b), c(c), d(d), e(e), f(f) {
  }

  A a;
  B b;
  C c;
  D d;
  E e;
  F f;
};

template <class A, class B, class C, class D, class E, class F, class G>
struct Tuple7 {
 public:
  typedef A TypeA;
  typedef B TypeB;
  typedef C TypeC;
  typedef D TypeD;
  typedef E TypeE;
  typedef F TypeF;
  typedef G TypeG;

  Tuple7() {}
  Tuple7(typename TupleTraits<A>::ParamType a,
    typename TupleTraits<B>::ParamType b,
    typename TupleTraits<C>::ParamType c,
    typename TupleTraits<D>::ParamType d,
    typename TupleTraits<E>::ParamType e,
    typename TupleTraits<F>::ParamType f,
    typename TupleTraits<G>::ParamType g)
    : a(a), b(b), c(c), d(d), e(e), f(f), g(g) {
  }

  A a;
  B b;
  C c;
  D d;
  E e;
  F f;
  G g;
};

template <class A, class B, class C, class D, class E, class F, class G,
          class H>
struct Tuple8 {
 public:
  typedef A TypeA;
  typedef B TypeB;
  typedef C TypeC;
  typedef D TypeD;
  typedef E TypeE;
  typedef F TypeF;
  typedef G TypeG;
  typedef H TypeH;

  Tuple8() {}
  Tuple8(typename TupleTraits<A>::ParamType a,
    typename TupleTraits<B>::ParamType b,
    typename TupleTraits<C>::ParamType c,
    typename TupleTraits<D>::ParamType d,
    typename TupleTraits<E>::ParamType e,
    typename TupleTraits<F>::ParamType f,
    typename TupleTraits<G>::ParamType g,
    typename TupleTraits<H>::ParamType h)
    : a(a), b(b), c(c), d(d), e(e), f(f), g(g), h(h) {
  }

  A a;
  B b;
  C c;
  D d;
  E e;
  F f;
  G g;
  H h;
};






template <>
struct TupleTypes< Tuple0 > {
  typedef Tuple0 ValueTuple;
  typedef Tuple0 RefTuple;
  typedef Tuple0 ParamTuple;
};

template <class A>
struct TupleTypes< Tuple1<A> > {
  typedef Tuple1<typename TupleTraits<A>::ValueType> ValueTuple;
  typedef Tuple1<typename TupleTraits<A>::RefType> RefTuple;
  typedef Tuple1<typename TupleTraits<A>::ParamType> ParamTuple;
};

template <class A, class B>
struct TupleTypes< Tuple2<A, B> > {
  typedef Tuple2<typename TupleTraits<A>::ValueType,
                 typename TupleTraits<B>::ValueType> ValueTuple;
typedef Tuple2<typename TupleTraits<A>::RefType,
               typename TupleTraits<B>::RefType> RefTuple;
  typedef Tuple2<typename TupleTraits<A>::ParamType,
                 typename TupleTraits<B>::ParamType> ParamTuple;
};

template <class A, class B, class C>
struct TupleTypes< Tuple3<A, B, C> > {
  typedef Tuple3<typename TupleTraits<A>::ValueType,
                 typename TupleTraits<B>::ValueType,
                 typename TupleTraits<C>::ValueType> ValueTuple;
typedef Tuple3<typename TupleTraits<A>::RefType,
               typename TupleTraits<B>::RefType,
               typename TupleTraits<C>::RefType> RefTuple;
  typedef Tuple3<typename TupleTraits<A>::ParamType,
                 typename TupleTraits<B>::ParamType,
                 typename TupleTraits<C>::ParamType> ParamTuple;
};

template <class A, class B, class C, class D>
struct TupleTypes< Tuple4<A, B, C, D> > {
  typedef Tuple4<typename TupleTraits<A>::ValueType,
                 typename TupleTraits<B>::ValueType,
                 typename TupleTraits<C>::ValueType,
                 typename TupleTraits<D>::ValueType> ValueTuple;
typedef Tuple4<typename TupleTraits<A>::RefType,
               typename TupleTraits<B>::RefType,
               typename TupleTraits<C>::RefType,
               typename TupleTraits<D>::RefType> RefTuple;
  typedef Tuple4<typename TupleTraits<A>::ParamType,
                 typename TupleTraits<B>::ParamType,
                 typename TupleTraits<C>::ParamType,
                 typename TupleTraits<D>::ParamType> ParamTuple;
};

template <class A, class B, class C, class D, class E>
struct TupleTypes< Tuple5<A, B, C, D, E> > {
  typedef Tuple5<typename TupleTraits<A>::ValueType,
                 typename TupleTraits<B>::ValueType,
                 typename TupleTraits<C>::ValueType,
                 typename TupleTraits<D>::ValueType,
                 typename TupleTraits<E>::ValueType> ValueTuple;
typedef Tuple5<typename TupleTraits<A>::RefType,
               typename TupleTraits<B>::RefType,
               typename TupleTraits<C>::RefType,
               typename TupleTraits<D>::RefType,
               typename TupleTraits<E>::RefType> RefTuple;
  typedef Tuple5<typename TupleTraits<A>::ParamType,
                 typename TupleTraits<B>::ParamType,
                 typename TupleTraits<C>::ParamType,
                 typename TupleTraits<D>::ParamType,
                 typename TupleTraits<E>::ParamType> ParamTuple;
};

template <class A, class B, class C, class D, class E, class F>
struct TupleTypes< Tuple6<A, B, C, D, E, F> > {
  typedef Tuple6<typename TupleTraits<A>::ValueType,
                 typename TupleTraits<B>::ValueType,
                 typename TupleTraits<C>::ValueType,
                 typename TupleTraits<D>::ValueType,
                 typename TupleTraits<E>::ValueType,
                 typename TupleTraits<F>::ValueType> ValueTuple;
typedef Tuple6<typename TupleTraits<A>::RefType,
               typename TupleTraits<B>::RefType,
               typename TupleTraits<C>::RefType,
               typename TupleTraits<D>::RefType,
               typename TupleTraits<E>::RefType,
               typename TupleTraits<F>::RefType> RefTuple;
  typedef Tuple6<typename TupleTraits<A>::ParamType,
                 typename TupleTraits<B>::ParamType,
                 typename TupleTraits<C>::ParamType,
                 typename TupleTraits<D>::ParamType,
                 typename TupleTraits<E>::ParamType,
                 typename TupleTraits<F>::ParamType> ParamTuple;
};

template <class A, class B, class C, class D, class E, class F, class G>
struct TupleTypes< Tuple7<A, B, C, D, E, F, G> > {
  typedef Tuple7<typename TupleTraits<A>::ValueType,
                 typename TupleTraits<B>::ValueType,
                 typename TupleTraits<C>::ValueType,
                 typename TupleTraits<D>::ValueType,
                 typename TupleTraits<E>::ValueType,
                 typename TupleTraits<F>::ValueType,
                 typename TupleTraits<G>::ValueType> ValueTuple;
typedef Tuple7<typename TupleTraits<A>::RefType,
               typename TupleTraits<B>::RefType,
               typename TupleTraits<C>::RefType,
               typename TupleTraits<D>::RefType,
               typename TupleTraits<E>::RefType,
               typename TupleTraits<F>::RefType,
               typename TupleTraits<G>::RefType> RefTuple;
  typedef Tuple7<typename TupleTraits<A>::ParamType,
                 typename TupleTraits<B>::ParamType,
                 typename TupleTraits<C>::ParamType,
                 typename TupleTraits<D>::ParamType,
                 typename TupleTraits<E>::ParamType,
                 typename TupleTraits<F>::ParamType,
                 typename TupleTraits<G>::ParamType> ParamTuple;
};

template <class A, class B, class C, class D, class E, class F, class G,
          class H>
struct TupleTypes< Tuple8<A, B, C, D, E, F, G, H> > {
  typedef Tuple8<typename TupleTraits<A>::ValueType,
                 typename TupleTraits<B>::ValueType,
                 typename TupleTraits<C>::ValueType,
                 typename TupleTraits<D>::ValueType,
                 typename TupleTraits<E>::ValueType,
                 typename TupleTraits<F>::ValueType,
                 typename TupleTraits<G>::ValueType,
                 typename TupleTraits<H>::ValueType> ValueTuple;
typedef Tuple8<typename TupleTraits<A>::RefType,
               typename TupleTraits<B>::RefType,
               typename TupleTraits<C>::RefType,
               typename TupleTraits<D>::RefType,
               typename TupleTraits<E>::RefType,
               typename TupleTraits<F>::RefType,
               typename TupleTraits<G>::RefType,
               typename TupleTraits<H>::RefType> RefTuple;
  typedef Tuple8<typename TupleTraits<A>::ParamType,
                 typename TupleTraits<B>::ParamType,
                 typename TupleTraits<C>::ParamType,
                 typename TupleTraits<D>::ParamType,
                 typename TupleTraits<E>::ParamType,
                 typename TupleTraits<F>::ParamType,
                 typename TupleTraits<G>::ParamType,
                 typename TupleTraits<H>::ParamType> ParamTuple;
};






inline Tuple0 MakeTuple() {
  return Tuple0();
}

template <class A>
inline Tuple1<A> MakeTuple(const A& a) {
  return Tuple1<A>(a);
}

template <class A, class B>
inline Tuple2<A, B> MakeTuple(const A& a, const B& b) {
  return Tuple2<A, B>(a, b);
}

template <class A, class B, class C>
inline Tuple3<A, B, C> MakeTuple(const A& a, const B& b, const C& c) {
  return Tuple3<A, B, C>(a, b, c);
}

template <class A, class B, class C, class D>
inline Tuple4<A, B, C, D> MakeTuple(const A& a, const B& b, const C& c,
                                    const D& d) {
  return Tuple4<A, B, C, D>(a, b, c, d);
}

template <class A, class B, class C, class D, class E>
inline Tuple5<A, B, C, D, E> MakeTuple(const A& a, const B& b, const C& c,
                                       const D& d, const E& e) {
  return Tuple5<A, B, C, D, E>(a, b, c, d, e);
}

template <class A, class B, class C, class D, class E, class F>
inline Tuple6<A, B, C, D, E, F> MakeTuple(const A& a, const B& b, const C& c,
                                          const D& d, const E& e, const F& f) {
  return Tuple6<A, B, C, D, E, F>(a, b, c, d, e, f);
}

template <class A, class B, class C, class D, class E, class F, class G>
inline Tuple7<A, B, C, D, E, F, G> MakeTuple(const A& a, const B& b, const C& c,
                                             const D& d, const E& e, const F& f,
                                             const G& g) {
  return Tuple7<A, B, C, D, E, F, G>(a, b, c, d, e, f, g);
}

template <class A, class B, class C, class D, class E, class F, class G,
          class H>
inline Tuple8<A, B, C, D, E, F, G, H> MakeTuple(const A& a, const B& b,
                                                const C& c, const D& d,
                                                const E& e, const F& f,
                                                const G& g, const H& h) {
  return Tuple8<A, B, C, D, E, F, G, H>(a, b, c, d, e, f, g, h);
}




template <class A>
inline Tuple1<A&> MakeRefTuple(A& a) {
  return Tuple1<A&>(a);
}

template <class A, class B>
inline Tuple2<A&, B&> MakeRefTuple(A& a, B& b) {
  return Tuple2<A&, B&>(a, b);
}

template <class A, class B, class C>
inline Tuple3<A&, B&, C&> MakeRefTuple(A& a, B& b, C& c) {
  return Tuple3<A&, B&, C&>(a, b, c);
}

template <class A, class B, class C, class D>
inline Tuple4<A&, B&, C&, D&> MakeRefTuple(A& a, B& b, C& c, D& d) {
  return Tuple4<A&, B&, C&, D&>(a, b, c, d);
}

template <class A, class B, class C, class D, class E>
inline Tuple5<A&, B&, C&, D&, E&> MakeRefTuple(A& a, B& b, C& c, D& d, E& e) {
  return Tuple5<A&, B&, C&, D&, E&>(a, b, c, d, e);
}

template <class A, class B, class C, class D, class E, class F>
inline Tuple6<A&, B&, C&, D&, E&, F&> MakeRefTuple(A& a, B& b, C& c, D& d, E& e,
                                                   F& f) {
  return Tuple6<A&, B&, C&, D&, E&, F&>(a, b, c, d, e, f);
}

template <class A, class B, class C, class D, class E, class F, class G>
inline Tuple7<A&, B&, C&, D&, E&, F&, G&> MakeRefTuple(A& a, B& b, C& c, D& d,
                                                       E& e, F& f, G& g) {
  return Tuple7<A&, B&, C&, D&, E&, F&, G&>(a, b, c, d, e, f, g);
}

template <class A, class B, class C, class D, class E, class F, class G,
          class H>
inline Tuple8<A&, B&, C&, D&, E&, F&, G&, H&> MakeRefTuple(A& a, B& b, C& c,
                                                           D& d, E& e, F& f,
                                                           G& g, H& h) {
  return Tuple8<A&, B&, C&, D&, E&, F&, G&, H&>(a, b, c, d, e, f, g, h);
}












template <class ObjT, class Method>
inline void DispatchToMethod(ObjT* obj, Method method, const Tuple0& arg) {
  (obj->*method)();
}

template <class ObjT, class Method, class A>
inline void DispatchToMethod(ObjT* obj, Method method, const A& arg) {
  (obj->*method)(base::internal::UnwrapTraits<A>::Unwrap(arg));
}

template <class ObjT, class Method, class A>
inline void DispatchToMethod(ObjT* obj, Method method, const Tuple1<A>& arg) {
  (obj->*method)(base::internal::UnwrapTraits<A>::Unwrap(arg.a));
}

template<class ObjT, class Method, class A, class B>
inline void DispatchToMethod(ObjT* obj,
                             Method method,
                             const Tuple2<A, B>& arg) {
  (obj->*method)(base::internal::UnwrapTraits<A>::Unwrap(arg.a),
                 base::internal::UnwrapTraits<B>::Unwrap(arg.b));
}

template<class ObjT, class Method, class A, class B, class C>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple3<A, B, C>& arg) {
  (obj->*method)(base::internal::UnwrapTraits<A>::Unwrap(arg.a),
                 base::internal::UnwrapTraits<B>::Unwrap(arg.b),
                 base::internal::UnwrapTraits<C>::Unwrap(arg.c));
}

template<class ObjT, class Method, class A, class B, class C, class D>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple4<A, B, C, D>& arg) {
  (obj->*method)(base::internal::UnwrapTraits<A>::Unwrap(arg.a),
                 base::internal::UnwrapTraits<B>::Unwrap(arg.b),
                 base::internal::UnwrapTraits<C>::Unwrap(arg.c),
                 base::internal::UnwrapTraits<D>::Unwrap(arg.d));
}

template<class ObjT, class Method, class A, class B, class C, class D, class E>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple5<A, B, C, D, E>& arg) {
  (obj->*method)(base::internal::UnwrapTraits<A>::Unwrap(arg.a),
                 base::internal::UnwrapTraits<B>::Unwrap(arg.b),
                 base::internal::UnwrapTraits<C>::Unwrap(arg.c),
                 base::internal::UnwrapTraits<D>::Unwrap(arg.d),
                 base::internal::UnwrapTraits<E>::Unwrap(arg.e));
}

template<class ObjT, class Method, class A, class B, class C, class D, class E,
         class F>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple6<A, B, C, D, E, F>& arg) {
  (obj->*method)(base::internal::UnwrapTraits<A>::Unwrap(arg.a),
                 base::internal::UnwrapTraits<B>::Unwrap(arg.b),
                 base::internal::UnwrapTraits<C>::Unwrap(arg.c),
                 base::internal::UnwrapTraits<D>::Unwrap(arg.d),
                 base::internal::UnwrapTraits<E>::Unwrap(arg.e),
                 base::internal::UnwrapTraits<F>::Unwrap(arg.f));
}

template<class ObjT, class Method, class A, class B, class C, class D, class E,
         class F, class G>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple7<A, B, C, D, E, F, G>& arg) {
  (obj->*method)(base::internal::UnwrapTraits<A>::Unwrap(arg.a),
                 base::internal::UnwrapTraits<B>::Unwrap(arg.b),
                 base::internal::UnwrapTraits<C>::Unwrap(arg.c),
                 base::internal::UnwrapTraits<D>::Unwrap(arg.d),
                 base::internal::UnwrapTraits<E>::Unwrap(arg.e),
                 base::internal::UnwrapTraits<F>::Unwrap(arg.f),
                 base::internal::UnwrapTraits<G>::Unwrap(arg.g));
}

template<class ObjT, class Method, class A, class B, class C, class D, class E,
         class F, class G, class H>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple8<A, B, C, D, E, F, G, H>& arg) {
  (obj->*method)(base::internal::UnwrapTraits<A>::Unwrap(arg.a),
                 base::internal::UnwrapTraits<B>::Unwrap(arg.b),
                 base::internal::UnwrapTraits<C>::Unwrap(arg.c),
                 base::internal::UnwrapTraits<D>::Unwrap(arg.d),
                 base::internal::UnwrapTraits<E>::Unwrap(arg.e),
                 base::internal::UnwrapTraits<F>::Unwrap(arg.f),
                 base::internal::UnwrapTraits<G>::Unwrap(arg.g),
                 base::internal::UnwrapTraits<H>::Unwrap(arg.h));
}



template <class Function>
inline void DispatchToFunction(Function function, const Tuple0& arg) {
  (*function)();
}

template <class Function, class A>
inline void DispatchToFunction(Function function, const A& arg) {
  (*function)(arg);
}

template <class Function, class A>
inline void DispatchToFunction(Function function, const Tuple1<A>& arg) {
  (*function)(base::internal::UnwrapTraits<A>::Unwrap(arg.a));
}

template<class Function, class A, class B>
inline void DispatchToFunction(Function function, const Tuple2<A, B>& arg) {
  (*function)(base::internal::UnwrapTraits<A>::Unwrap(arg.a),
              base::internal::UnwrapTraits<B>::Unwrap(arg.b));
}

template<class Function, class A, class B, class C>
inline void DispatchToFunction(Function function, const Tuple3<A, B, C>& arg) {
  (*function)(base::internal::UnwrapTraits<A>::Unwrap(arg.a),
              base::internal::UnwrapTraits<B>::Unwrap(arg.b),
              base::internal::UnwrapTraits<C>::Unwrap(arg.c));
}

template<class Function, class A, class B, class C, class D>
inline void DispatchToFunction(Function function,
                               const Tuple4<A, B, C, D>& arg) {
  (*function)(base::internal::UnwrapTraits<A>::Unwrap(arg.a),
              base::internal::UnwrapTraits<B>::Unwrap(arg.b),
              base::internal::UnwrapTraits<C>::Unwrap(arg.c),
              base::internal::UnwrapTraits<D>::Unwrap(arg.d));
}

template<class Function, class A, class B, class C, class D, class E>
inline void DispatchToFunction(Function function,
                               const Tuple5<A, B, C, D, E>& arg) {
  (*function)(base::internal::UnwrapTraits<A>::Unwrap(arg.a),
              base::internal::UnwrapTraits<B>::Unwrap(arg.b),
              base::internal::UnwrapTraits<C>::Unwrap(arg.c),
              base::internal::UnwrapTraits<D>::Unwrap(arg.d),
              base::internal::UnwrapTraits<E>::Unwrap(arg.e));
}

template<class Function, class A, class B, class C, class D, class E, class F>
inline void DispatchToFunction(Function function,
                               const Tuple6<A, B, C, D, E, F>& arg) {
  (*function)(base::internal::UnwrapTraits<A>::Unwrap(arg.a),
              base::internal::UnwrapTraits<B>::Unwrap(arg.b),
              base::internal::UnwrapTraits<C>::Unwrap(arg.c),
              base::internal::UnwrapTraits<D>::Unwrap(arg.d),
              base::internal::UnwrapTraits<E>::Unwrap(arg.e),
              base::internal::UnwrapTraits<F>::Unwrap(arg.f));
}

template<class Function, class A, class B, class C, class D, class E, class F,
         class G>
inline void DispatchToFunction(Function function,
                               const Tuple7<A, B, C, D, E, F, G>& arg) {
  (*function)(base::internal::UnwrapTraits<A>::Unwrap(arg.a),
              base::internal::UnwrapTraits<B>::Unwrap(arg.b),
              base::internal::UnwrapTraits<C>::Unwrap(arg.c),
              base::internal::UnwrapTraits<D>::Unwrap(arg.d),
              base::internal::UnwrapTraits<E>::Unwrap(arg.e),
              base::internal::UnwrapTraits<F>::Unwrap(arg.f),
              base::internal::UnwrapTraits<G>::Unwrap(arg.g));
}

template<class Function, class A, class B, class C, class D, class E, class F,
         class G, class H>
inline void DispatchToFunction(Function function,
                               const Tuple8<A, B, C, D, E, F, G, H>& arg) {
  (*function)(base::internal::UnwrapTraits<A>::Unwrap(arg.a),
              base::internal::UnwrapTraits<B>::Unwrap(arg.b),
              base::internal::UnwrapTraits<C>::Unwrap(arg.c),
              base::internal::UnwrapTraits<D>::Unwrap(arg.d),
              base::internal::UnwrapTraits<E>::Unwrap(arg.e),
              base::internal::UnwrapTraits<F>::Unwrap(arg.f),
              base::internal::UnwrapTraits<G>::Unwrap(arg.g),
              base::internal::UnwrapTraits<H>::Unwrap(arg.h));
}



template <class ObjT, class Method>
inline void DispatchToMethod(ObjT* obj,
                             Method method,
                             const Tuple0& arg, Tuple0*) {
  (obj->*method)();
}

template <class ObjT, class Method, class A>
inline void DispatchToMethod(ObjT* obj, Method method, const A& arg, Tuple0*) {
  (obj->*method)(base::internal::UnwrapTraits<A>::Unwrap(arg));
}

template <class ObjT, class Method, class A>
inline void DispatchToMethod(ObjT* obj,
                             Method method,
                             const Tuple1<A>& arg, Tuple0*) {
  (obj->*method)(base::internal::UnwrapTraits<A>::Unwrap(arg.a));
}

template<class ObjT, class Method, class A, class B>
inline void DispatchToMethod(ObjT* obj,
                             Method method,
                             const Tuple2<A, B>& arg, Tuple0*) {
  (obj->*method)(base::internal::UnwrapTraits<A>::Unwrap(arg.a),
                 base::internal::UnwrapTraits<B>::Unwrap(arg.b));
}

template<class ObjT, class Method, class A, class B, class C>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple3<A, B, C>& arg, Tuple0*) {
  (obj->*method)(base::internal::UnwrapTraits<A>::Unwrap(arg.a),
                 base::internal::UnwrapTraits<B>::Unwrap(arg.b),
                 base::internal::UnwrapTraits<C>::Unwrap(arg.c));
}

template<class ObjT, class Method, class A, class B, class C, class D>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple4<A, B, C, D>& arg, Tuple0*) {
  (obj->*method)(base::internal::UnwrapTraits<A>::Unwrap(arg.a),
                 base::internal::UnwrapTraits<B>::Unwrap(arg.b),
                 base::internal::UnwrapTraits<C>::Unwrap(arg.c),
                 base::internal::UnwrapTraits<D>::Unwrap(arg.d));
}

template<class ObjT, class Method, class A, class B, class C, class D, class E>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple5<A, B, C, D, E>& arg, Tuple0*) {
  (obj->*method)(base::internal::UnwrapTraits<A>::Unwrap(arg.a),
                 base::internal::UnwrapTraits<B>::Unwrap(arg.b),
                 base::internal::UnwrapTraits<C>::Unwrap(arg.c),
                 base::internal::UnwrapTraits<D>::Unwrap(arg.d),
                 base::internal::UnwrapTraits<E>::Unwrap(arg.e));
}

template<class ObjT, class Method, class A, class B, class C, class D, class E,
         class F>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple6<A, B, C, D, E, F>& arg, Tuple0*) {
  (obj->*method)(base::internal::UnwrapTraits<A>::Unwrap(arg.a),
                 base::internal::UnwrapTraits<B>::Unwrap(arg.b),
                 base::internal::UnwrapTraits<C>::Unwrap(arg.c),
                 base::internal::UnwrapTraits<D>::Unwrap(arg.d),
                 base::internal::UnwrapTraits<E>::Unwrap(arg.e),
                 base::internal::UnwrapTraits<F>::Unwrap(arg.f));
}



template<class ObjT, class Method,
         class OutA>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple0& in,
                             Tuple1<OutA>* out) {
  (obj->*method)(&out->a);
}

template<class ObjT, class Method, class InA,
         class OutA>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const InA& in,
                             Tuple1<OutA>* out) {
  (obj->*method)(in, &out->a);
}

template<class ObjT, class Method, class InA,
         class OutA>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple1<InA>& in,
                             Tuple1<OutA>* out) {
  (obj->*method)(base::internal::UnwrapTraits<InA>::Unwrap(in.a), &out->a);
}

template<class ObjT, class Method, class InA, class InB,
         class OutA>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple2<InA, InB>& in,
                             Tuple1<OutA>* out) {
  (obj->*method)(base::internal::UnwrapTraits<InA>::Unwrap(in.a),
                 base::internal::UnwrapTraits<InB>::Unwrap(in.b),
                 &out->a);
}

template<class ObjT, class Method, class InA, class InB, class InC,
         class OutA>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple3<InA, InB, InC>& in,
                             Tuple1<OutA>* out) {
  (obj->*method)(base::internal::UnwrapTraits<InA>::Unwrap(in.a),
                 base::internal::UnwrapTraits<InB>::Unwrap(in.b),
                 base::internal::UnwrapTraits<InC>::Unwrap(in.c),
                 &out->a);
}

template<class ObjT, class Method, class InA, class InB, class InC, class InD,
         class OutA>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple4<InA, InB, InC, InD>& in,
                             Tuple1<OutA>* out) {
  (obj->*method)(base::internal::UnwrapTraits<InA>::Unwrap(in.a),
                 base::internal::UnwrapTraits<InB>::Unwrap(in.b),
                 base::internal::UnwrapTraits<InC>::Unwrap(in.c),
                 base::internal::UnwrapTraits<InD>::Unwrap(in.d),
                 &out->a);
}

template<class ObjT, class Method, class InA, class InB, class InC, class InD,
         class InE, class OutA>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple5<InA, InB, InC, InD, InE>& in,
                             Tuple1<OutA>* out) {
  (obj->*method)(base::internal::UnwrapTraits<InA>::Unwrap(in.a),
                 base::internal::UnwrapTraits<InB>::Unwrap(in.b),
                 base::internal::UnwrapTraits<InC>::Unwrap(in.c),
                 base::internal::UnwrapTraits<InD>::Unwrap(in.d),
                 base::internal::UnwrapTraits<InE>::Unwrap(in.e),
                 &out->a);
}

template<class ObjT, class Method,
         class InA, class InB, class InC, class InD, class InE, class InF,
         class OutA>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple6<InA, InB, InC, InD, InE, InF>& in,
                             Tuple1<OutA>* out) {
  (obj->*method)(base::internal::UnwrapTraits<InA>::Unwrap(in.a),
                 base::internal::UnwrapTraits<InB>::Unwrap(in.b),
                 base::internal::UnwrapTraits<InC>::Unwrap(in.c),
                 base::internal::UnwrapTraits<InD>::Unwrap(in.d),
                 base::internal::UnwrapTraits<InE>::Unwrap(in.e),
                 base::internal::UnwrapTraits<InF>::Unwrap(in.f),
                 &out->a);
}



template<class ObjT, class Method,
         class OutA, class OutB>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple0& in,
                             Tuple2<OutA, OutB>* out) {
  (obj->*method)(&out->a, &out->b);
}

template<class ObjT, class Method, class InA,
         class OutA, class OutB>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const InA& in,
                             Tuple2<OutA, OutB>* out) {
  (obj->*method)(in, &out->a, &out->b);
}

template<class ObjT, class Method, class InA,
         class OutA, class OutB>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple1<InA>& in,
                             Tuple2<OutA, OutB>* out) {
  (obj->*method)(
      base::internal::UnwrapTraits<InA>::Unwrap(in.a), &out->a, &out->b);
}

template<class ObjT, class Method, class InA, class InB,
         class OutA, class OutB>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple2<InA, InB>& in,
                             Tuple2<OutA, OutB>* out) {
  (obj->*method)(base::internal::UnwrapTraits<InA>::Unwrap(in.a),
                 base::internal::UnwrapTraits<InB>::Unwrap(in.b),
                 &out->a,
                 &out->b);
}

template<class ObjT, class Method, class InA, class InB, class InC,
         class OutA, class OutB>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple3<InA, InB, InC>& in,
                             Tuple2<OutA, OutB>* out) {
  (obj->*method)(base::internal::UnwrapTraits<InA>::Unwrap(in.a),
                 base::internal::UnwrapTraits<InB>::Unwrap(in.b),
                 base::internal::UnwrapTraits<InC>::Unwrap(in.c),
                 &out->a,
                 &out->b);
}

template<class ObjT, class Method, class InA, class InB, class InC, class InD,
         class OutA, class OutB>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple4<InA, InB, InC, InD>& in,
                             Tuple2<OutA, OutB>* out) {
  (obj->*method)(base::internal::UnwrapTraits<InA>::Unwrap(in.a),
                 base::internal::UnwrapTraits<InB>::Unwrap(in.b),
                 base::internal::UnwrapTraits<InC>::Unwrap(in.c),
                 base::internal::UnwrapTraits<InD>::Unwrap(in.d),
                 &out->a,
                 &out->b);
}

template<class ObjT, class Method,
         class InA, class InB, class InC, class InD, class InE,
         class OutA, class OutB>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple5<InA, InB, InC, InD, InE>& in,
                             Tuple2<OutA, OutB>* out) {
  (obj->*method)(base::internal::UnwrapTraits<InA>::Unwrap(in.a),
                 base::internal::UnwrapTraits<InB>::Unwrap(in.b),
                 base::internal::UnwrapTraits<InC>::Unwrap(in.c),
                 base::internal::UnwrapTraits<InD>::Unwrap(in.d),
                 base::internal::UnwrapTraits<InE>::Unwrap(in.e),
                 &out->a,
                 &out->b);
}

template<class ObjT, class Method,
         class InA, class InB, class InC, class InD, class InE, class InF,
         class OutA, class OutB>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple6<InA, InB, InC, InD, InE, InF>& in,
                             Tuple2<OutA, OutB>* out) {
  (obj->*method)(base::internal::UnwrapTraits<InA>::Unwrap(in.a),
                 base::internal::UnwrapTraits<InB>::Unwrap(in.b),
                 base::internal::UnwrapTraits<InC>::Unwrap(in.c),
                 base::internal::UnwrapTraits<InD>::Unwrap(in.d),
                 base::internal::UnwrapTraits<InE>::Unwrap(in.e),
                 base::internal::UnwrapTraits<InF>::Unwrap(in.f),
                 &out->a,
                 &out->b);
}



template<class ObjT, class Method,
         class OutA, class OutB, class OutC>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple0& in,
                             Tuple3<OutA, OutB, OutC>* out) {
  (obj->*method)(&out->a, &out->b, &out->c);
}

template<class ObjT, class Method, class InA,
         class OutA, class OutB, class OutC>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const InA& in,
                             Tuple3<OutA, OutB, OutC>* out) {
  (obj->*method)(in, &out->a, &out->b, &out->c);
}

template<class ObjT, class Method, class InA,
         class OutA, class OutB, class OutC>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple1<InA>& in,
                             Tuple3<OutA, OutB, OutC>* out) {
  (obj->*method)(base::internal::UnwrapTraits<InA>::Unwrap(in.a),
                 &out->a,
                 &out->b,
                 &out->c);
}

template<class ObjT, class Method, class InA, class InB,
         class OutA, class OutB, class OutC>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple2<InA, InB>& in,
                             Tuple3<OutA, OutB, OutC>* out) {
  (obj->*method)(base::internal::UnwrapTraits<InA>::Unwrap(in.a),
                 base::internal::UnwrapTraits<InB>::Unwrap(in.b),
                 &out->a,
                 &out->b,
                 &out->c);
}

template<class ObjT, class Method, class InA, class InB, class InC,
         class OutA, class OutB, class OutC>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple3<InA, InB, InC>& in,
                             Tuple3<OutA, OutB, OutC>* out) {
  (obj->*method)(base::internal::UnwrapTraits<InA>::Unwrap(in.a),
                 base::internal::UnwrapTraits<InB>::Unwrap(in.b),
                 base::internal::UnwrapTraits<InC>::Unwrap(in.c),
                 &out->a,
                 &out->b,
                 &out->c);
}

template<class ObjT, class Method, class InA, class InB, class InC, class InD,
         class OutA, class OutB, class OutC>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple4<InA, InB, InC, InD>& in,
                             Tuple3<OutA, OutB, OutC>* out) {
  (obj->*method)(base::internal::UnwrapTraits<InA>::Unwrap(in.a),
                 base::internal::UnwrapTraits<InB>::Unwrap(in.b),
                 base::internal::UnwrapTraits<InC>::Unwrap(in.c),
                 base::internal::UnwrapTraits<InD>::Unwrap(in.d),
                 &out->a,
                 &out->b,
                 &out->c);
}

template<class ObjT, class Method,
         class InA, class InB, class InC, class InD, class InE,
         class OutA, class OutB, class OutC>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple5<InA, InB, InC, InD, InE>& in,
                             Tuple3<OutA, OutB, OutC>* out) {
  (obj->*method)(base::internal::UnwrapTraits<InA>::Unwrap(in.a),
                 base::internal::UnwrapTraits<InB>::Unwrap(in.b),
                 base::internal::UnwrapTraits<InC>::Unwrap(in.c),
                 base::internal::UnwrapTraits<InD>::Unwrap(in.d),
                 base::internal::UnwrapTraits<InE>::Unwrap(in.e),
                 &out->a,
                 &out->b,
                 &out->c);
}

template<class ObjT, class Method,
         class InA, class InB, class InC, class InD, class InE, class InF,
         class OutA, class OutB, class OutC>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple6<InA, InB, InC, InD, InE, InF>& in,
                             Tuple3<OutA, OutB, OutC>* out) {
  (obj->*method)(base::internal::UnwrapTraits<InA>::Unwrap(in.a),
                 base::internal::UnwrapTraits<InB>::Unwrap(in.b),
                 base::internal::UnwrapTraits<InC>::Unwrap(in.c),
                 base::internal::UnwrapTraits<InD>::Unwrap(in.d),
                 base::internal::UnwrapTraits<InE>::Unwrap(in.e),
                 base::internal::UnwrapTraits<InF>::Unwrap(in.f),
                 &out->a,
                 &out->b,
                 &out->c);
}



template<class ObjT, class Method,
         class OutA, class OutB, class OutC, class OutD>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple0& in,
                             Tuple4<OutA, OutB, OutC, OutD>* out) {
  (obj->*method)(&out->a, &out->b, &out->c, &out->d);
}

template<class ObjT, class Method, class InA,
         class OutA, class OutB, class OutC, class OutD>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const InA& in,
                             Tuple4<OutA, OutB, OutC, OutD>* out) {
  (obj->*method)(base::internal::UnwrapTraits<InA>::Unwrap(in),
                 &out->a,
                 &out->b,
                 &out->c,
                 &out->d);
}

template<class ObjT, class Method, class InA,
         class OutA, class OutB, class OutC, class OutD>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple1<InA>& in,
                             Tuple4<OutA, OutB, OutC, OutD>* out) {
  (obj->*method)(base::internal::UnwrapTraits<InA>::Unwrap(in.a),
                 &out->a,
                 &out->b,
                 &out->c,
                 &out->d);
}

template<class ObjT, class Method, class InA, class InB,
         class OutA, class OutB, class OutC, class OutD>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple2<InA, InB>& in,
                             Tuple4<OutA, OutB, OutC, OutD>* out) {
  (obj->*method)(base::internal::UnwrapTraits<InA>::Unwrap(in.a),
                 base::internal::UnwrapTraits<InB>::Unwrap(in.b),
                 &out->a,
                 &out->b,
                 &out->c,
                 &out->d);
}

template<class ObjT, class Method, class InA, class InB, class InC,
         class OutA, class OutB, class OutC, class OutD>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple3<InA, InB, InC>& in,
                             Tuple4<OutA, OutB, OutC, OutD>* out) {
  (obj->*method)(base::internal::UnwrapTraits<InA>::Unwrap(in.a),
                 base::internal::UnwrapTraits<InB>::Unwrap(in.b),
                 base::internal::UnwrapTraits<InC>::Unwrap(in.c),
                 &out->a,
                 &out->b,
                 &out->c,
                 &out->d);
}

template<class ObjT, class Method, class InA, class InB, class InC, class InD,
         class OutA, class OutB, class OutC, class OutD>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple4<InA, InB, InC, InD>& in,
                             Tuple4<OutA, OutB, OutC, OutD>* out) {
  (obj->*method)(base::internal::UnwrapTraits<InA>::Unwrap(in.a),
                 base::internal::UnwrapTraits<InB>::Unwrap(in.b),
                 base::internal::UnwrapTraits<InC>::Unwrap(in.c),
                 base::internal::UnwrapTraits<InD>::Unwrap(in.d),
                 &out->a,
                 &out->b,
                 &out->c,
                 &out->d);
}

template<class ObjT, class Method,
         class InA, class InB, class InC, class InD, class InE,
         class OutA, class OutB, class OutC, class OutD>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple5<InA, InB, InC, InD, InE>& in,
                             Tuple4<OutA, OutB, OutC, OutD>* out) {
  (obj->*method)(base::internal::UnwrapTraits<InA>::Unwrap(in.a),
                 base::internal::UnwrapTraits<InB>::Unwrap(in.b),
                 base::internal::UnwrapTraits<InC>::Unwrap(in.c),
                 base::internal::UnwrapTraits<InD>::Unwrap(in.d),
                 base::internal::UnwrapTraits<InE>::Unwrap(in.e),
                 &out->a,
                 &out->b,
                 &out->c,
                 &out->d);
}

template<class ObjT, class Method,
         class InA, class InB, class InC, class InD, class InE, class InF,
         class OutA, class OutB, class OutC, class OutD>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple6<InA, InB, InC, InD, InE, InF>& in,
                             Tuple4<OutA, OutB, OutC, OutD>* out) {
  (obj->*method)(base::internal::UnwrapTraits<InA>::Unwrap(in.a),
                 base::internal::UnwrapTraits<InB>::Unwrap(in.b),
                 base::internal::UnwrapTraits<InC>::Unwrap(in.c),
                 base::internal::UnwrapTraits<InD>::Unwrap(in.d),
                 base::internal::UnwrapTraits<InE>::Unwrap(in.e),
                 base::internal::UnwrapTraits<InF>::Unwrap(in.f),
                 &out->a,
                 &out->b,
                 &out->c,
                 &out->d);
}



template<class ObjT, class Method,
         class OutA, class OutB, class OutC, class OutD, class OutE>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple0& in,
                             Tuple5<OutA, OutB, OutC, OutD, OutE>* out) {
  (obj->*method)(&out->a, &out->b, &out->c, &out->d, &out->e);
}

template<class ObjT, class Method, class InA,
         class OutA, class OutB, class OutC, class OutD, class OutE>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const InA& in,
                             Tuple5<OutA, OutB, OutC, OutD, OutE>* out) {
  (obj->*method)(base::internal::UnwrapTraits<InA>::Unwrap(in),
                 &out->a,
                 &out->b,
                 &out->c,
                 &out->d,
                 &out->e);
}

template<class ObjT, class Method, class InA,
         class OutA, class OutB, class OutC, class OutD, class OutE>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple1<InA>& in,
                             Tuple5<OutA, OutB, OutC, OutD, OutE>* out) {
  (obj->*method)(base::internal::UnwrapTraits<InA>::Unwrap(in.a),
                 &out->a,
                 &out->b,
                 &out->c,
                 &out->d,
                 &out->e);
}

template<class ObjT, class Method, class InA, class InB,
         class OutA, class OutB, class OutC, class OutD, class OutE>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple2<InA, InB>& in,
                             Tuple5<OutA, OutB, OutC, OutD, OutE>* out) {
  (obj->*method)(base::internal::UnwrapTraits<InA>::Unwrap(in.a),
                 base::internal::UnwrapTraits<InB>::Unwrap(in.b),
                 &out->a,
                 &out->b,
                 &out->c,
                 &out->d,
                 &out->e);
}

template<class ObjT, class Method, class InA, class InB, class InC,
         class OutA, class OutB, class OutC, class OutD, class OutE>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple3<InA, InB, InC>& in,
                             Tuple5<OutA, OutB, OutC, OutD, OutE>* out) {
  (obj->*method)(base::internal::UnwrapTraits<InA>::Unwrap(in.a),
                 base::internal::UnwrapTraits<InB>::Unwrap(in.b),
                 base::internal::UnwrapTraits<InC>::Unwrap(in.c),
                 &out->a,
                 &out->b,
                 &out->c,
                 &out->d,
                 &out->e);
}

template<class ObjT, class Method, class InA, class InB, class InC, class InD,
         class OutA, class OutB, class OutC, class OutD, class OutE>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple4<InA, InB, InC, InD>& in,
                             Tuple5<OutA, OutB, OutC, OutD, OutE>* out) {
  (obj->*method)(base::internal::UnwrapTraits<InA>::Unwrap(in.a),
                 base::internal::UnwrapTraits<InB>::Unwrap(in.b),
                 base::internal::UnwrapTraits<InC>::Unwrap(in.c),
                 base::internal::UnwrapTraits<InD>::Unwrap(in.d),
                 &out->a,
                 &out->b,
                 &out->c,
                 &out->d,
                 &out->e);
}

template<class ObjT, class Method,
         class InA, class InB, class InC, class InD, class InE,
         class OutA, class OutB, class OutC, class OutD, class OutE>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple5<InA, InB, InC, InD, InE>& in,
                             Tuple5<OutA, OutB, OutC, OutD, OutE>* out) {
  (obj->*method)(base::internal::UnwrapTraits<InA>::Unwrap(in.a),
                 base::internal::UnwrapTraits<InB>::Unwrap(in.b),
                 base::internal::UnwrapTraits<InC>::Unwrap(in.c),
                 base::internal::UnwrapTraits<InD>::Unwrap(in.d),
                 base::internal::UnwrapTraits<InE>::Unwrap(in.e),
                 &out->a,
                 &out->b,
                 &out->c,
                 &out->d,
                 &out->e);
}

template<class ObjT, class Method,
         class InA, class InB, class InC, class InD, class InE, class InF,
         class OutA, class OutB, class OutC, class OutD, class OutE>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple6<InA, InB, InC, InD, InE, InF>& in,
                             Tuple5<OutA, OutB, OutC, OutD, OutE>* out) {
  (obj->*method)(base::internal::UnwrapTraits<InA>::Unwrap(in.a),
                 base::internal::UnwrapTraits<InB>::Unwrap(in.b),
                 base::internal::UnwrapTraits<InC>::Unwrap(in.c),
                 base::internal::UnwrapTraits<InD>::Unwrap(in.d),
                 base::internal::UnwrapTraits<InE>::Unwrap(in.e),
                 base::internal::UnwrapTraits<InF>::Unwrap(in.f),
                 &out->a,
                 &out->b,
                 &out->c,
                 &out->d,
                 &out->e);
}

#endif  
