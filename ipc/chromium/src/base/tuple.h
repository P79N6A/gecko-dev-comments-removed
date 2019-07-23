



























#ifndef BASE_TUPLE_H__
#define BASE_TUPLE_H__









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















struct Tuple0 {
  typedef Tuple0 ValueTuple;
  typedef Tuple0 RefTuple;
};

template <class A>
struct Tuple1 {
 public:
  typedef A TypeA;
  typedef Tuple1<typename TupleTraits<A>::ValueType> ValueTuple;
  typedef Tuple1<typename TupleTraits<A>::RefType> RefTuple;

  Tuple1() {}
  explicit Tuple1(typename TupleTraits<A>::ParamType a) : a(a) {}

  A a;
};

template <class A, class B>
struct Tuple2 {
 public:
  typedef A TypeA;
  typedef B TypeB;
  typedef Tuple2<typename TupleTraits<A>::ValueType,
                 typename TupleTraits<B>::ValueType> ValueTuple;
  typedef Tuple2<typename TupleTraits<A>::RefType,
                 typename TupleTraits<B>::RefType> RefTuple;

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
  typedef Tuple3<typename TupleTraits<A>::ValueType,
                 typename TupleTraits<B>::ValueType,
                 typename TupleTraits<C>::ValueType> ValueTuple;
  typedef Tuple3<typename TupleTraits<A>::RefType,
                 typename TupleTraits<B>::RefType,
                 typename TupleTraits<C>::RefType> RefTuple;

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
  typedef Tuple4<typename TupleTraits<A>::ValueType,
                 typename TupleTraits<B>::ValueType,
                 typename TupleTraits<C>::ValueType,
                 typename TupleTraits<D>::ValueType> ValueTuple;
  typedef Tuple4<typename TupleTraits<A>::RefType,
                 typename TupleTraits<B>::RefType,
                 typename TupleTraits<C>::RefType,
                 typename TupleTraits<D>::RefType> RefTuple;

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












template <class ObjT, class Method>
inline void DispatchToMethod(ObjT* obj, Method method, const Tuple0& arg) {
  (obj->*method)();
}

template <class ObjT, class Method, class A>
inline void DispatchToMethod(ObjT* obj, Method method, const A& arg) {
  (obj->*method)(arg);
}

template <class ObjT, class Method, class A>
inline void DispatchToMethod(ObjT* obj, Method method, const Tuple1<A>& arg) {
  (obj->*method)(arg.a);
}

template<class ObjT, class Method, class A, class B>
inline void DispatchToMethod(ObjT* obj,
                             Method method,
                             const Tuple2<A, B>& arg) {
  (obj->*method)(arg.a, arg.b);
}

template<class ObjT, class Method, class A, class B, class C>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple3<A, B, C>& arg) {
  (obj->*method)(arg.a, arg.b, arg.c);
}

template<class ObjT, class Method, class A, class B, class C, class D>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple4<A, B, C, D>& arg) {
  (obj->*method)(arg.a, arg.b, arg.c, arg.d);
}

template<class ObjT, class Method, class A, class B, class C, class D, class E>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple5<A, B, C, D, E>& arg) {
  (obj->*method)(arg.a, arg.b, arg.c, arg.d, arg.e);
}

template<class ObjT, class Method, class A, class B, class C, class D, class E,
         class F>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple6<A, B, C, D, E, F>& arg) {
  (obj->*method)(arg.a, arg.b, arg.c, arg.d, arg.e, arg.f);
}

template<class ObjT, class Method, class A, class B, class C, class D, class E,
         class F, class G>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple7<A, B, C, D, E, F, G>& arg) {
  (obj->*method)(arg.a, arg.b, arg.c, arg.d, arg.e, arg.f, arg.g);
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
  (*function)(arg.a);
}

template<class Function, class A, class B>
inline void DispatchToFunction(Function function, const Tuple2<A, B>& arg) {
  (*function)(arg.a, arg.b);
}

template<class Function, class A, class B, class C>
inline void DispatchToFunction(Function function, const Tuple3<A, B, C>& arg) {
  (*function)(arg.a, arg.b, arg.c);
}

template<class Function, class A, class B, class C, class D>
inline void DispatchToFunction(Function function,
                               const Tuple4<A, B, C, D>& arg) {
  (*function)(arg.a, arg.b, arg.c, arg.d);
}

template<class Function, class A, class B, class C, class D, class E>
inline void DispatchToFunction(Function function,
                               const Tuple5<A, B, C, D, E>& arg) {
  (*function)(arg.a, arg.b, arg.c, arg.d, arg.e);
}

template<class Function, class A, class B, class C, class D, class E, class F>
inline void DispatchToFunction(Function function,
                               const Tuple6<A, B, C, D, E, F>& arg) {
  (*function)(arg.a, arg.b, arg.c, arg.d, arg.e, arg.f);
}



template <class ObjT, class Method>
inline void DispatchToMethod(ObjT* obj,
                             Method method,
                             const Tuple0& arg, Tuple0*) {
  (obj->*method)();
}

template <class ObjT, class Method, class A>
inline void DispatchToMethod(ObjT* obj, Method method, const A& arg, Tuple0*) {
  (obj->*method)(arg);
}

template <class ObjT, class Method, class A>
inline void DispatchToMethod(ObjT* obj,
                             Method method,
                             const Tuple1<A>& arg, Tuple0*) {
  (obj->*method)(arg.a);
}

template<class ObjT, class Method, class A, class B>
inline void DispatchToMethod(ObjT* obj,
                             Method method,
                             const Tuple2<A, B>& arg, Tuple0*) {
  (obj->*method)(arg.a, arg.b);
}

template<class ObjT, class Method, class A, class B, class C>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple3<A, B, C>& arg, Tuple0*) {
  (obj->*method)(arg.a, arg.b, arg.c);
}

template<class ObjT, class Method, class A, class B, class C, class D>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple4<A, B, C, D>& arg, Tuple0*) {
  (obj->*method)(arg.a, arg.b, arg.c, arg.d);
}

template<class ObjT, class Method, class A, class B, class C, class D, class E>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple5<A, B, C, D, E>& arg, Tuple0*) {
  (obj->*method)(arg.a, arg.b, arg.c, arg.d, arg.e);
}

template<class ObjT, class Method, class A, class B, class C, class D, class E,
         class F>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple6<A, B, C, D, E, F>& arg, Tuple0*) {
  (obj->*method)(arg.a, arg.b, arg.c, arg.d, arg.e, arg.f);
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
  (obj->*method)(in.a, &out->a);
}

template<class ObjT, class Method, class InA, class InB,
         class OutA>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple2<InA, InB>& in,
                             Tuple1<OutA>* out) {
  (obj->*method)(in.a, in.b, &out->a);
}

template<class ObjT, class Method, class InA, class InB, class InC,
         class OutA>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple3<InA, InB, InC>& in,
                             Tuple1<OutA>* out) {
  (obj->*method)(in.a, in.b, in.c, &out->a);
}

template<class ObjT, class Method, class InA, class InB, class InC, class InD,
         class OutA>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple4<InA, InB, InC, InD>& in,
                             Tuple1<OutA>* out) {
  (obj->*method)(in.a, in.b, in.c, in.d, &out->a);
}

template<class ObjT, class Method,
         class InA, class InB, class InC, class InD, class InE,
         class OutA>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple5<InA, InB, InC, InD, InE>& in,
                             Tuple1<OutA>* out) {
  (obj->*method)(in.a, in.b, in.c, in.d, in.e, &out->a);
}

template<class ObjT, class Method,
         class InA, class InB, class InC, class InD, class InE, class InF,
         class OutA>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple6<InA, InB, InC, InD, InE, InF>& in,
                             Tuple1<OutA>* out) {
  (obj->*method)(in.a, in.b, in.c, in.d, in.e, in.f, &out->a);
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
  (obj->*method)(in.a, &out->a, &out->b);
}

template<class ObjT, class Method, class InA, class InB,
         class OutA, class OutB>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple2<InA, InB>& in,
                             Tuple2<OutA, OutB>* out) {
  (obj->*method)(in.a, in.b, &out->a, &out->b);
}

template<class ObjT, class Method, class InA, class InB, class InC,
         class OutA, class OutB>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple3<InA, InB, InC>& in,
                             Tuple2<OutA, OutB>* out) {
  (obj->*method)(in.a, in.b, in.c, &out->a, &out->b);
}

template<class ObjT, class Method, class InA, class InB, class InC, class InD,
         class OutA, class OutB>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple4<InA, InB, InC, InD>& in,
                             Tuple2<OutA, OutB>* out) {
  (obj->*method)(in.a, in.b, in.c, in.d, &out->a, &out->b);
}

template<class ObjT, class Method,
         class InA, class InB, class InC, class InD, class InE,
         class OutA, class OutB>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple5<InA, InB, InC, InD, InE>& in,
                             Tuple2<OutA, OutB>* out) {
  (obj->*method)(in.a, in.b, in.c, in.d, in.e, &out->a, &out->b);
}

template<class ObjT, class Method,
         class InA, class InB, class InC, class InD, class InE, class InF,
         class OutA, class OutB>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple6<InA, InB, InC, InD, InE, InF>& in,
                             Tuple2<OutA, OutB>* out) {
  (obj->*method)(in.a, in.b, in.c, in.d, in.e, in.f, &out->a, &out->b);
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
  (obj->*method)(in.a, &out->a, &out->b, &out->c);
}

template<class ObjT, class Method, class InA, class InB,
         class OutA, class OutB, class OutC>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple2<InA, InB>& in,
                             Tuple3<OutA, OutB, OutC>* out) {
  (obj->*method)(in.a, in.b, &out->a, &out->b, &out->c);
}

template<class ObjT, class Method, class InA, class InB, class InC,
         class OutA, class OutB, class OutC>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple3<InA, InB, InC>& in,
                             Tuple3<OutA, OutB, OutC>* out) {
  (obj->*method)(in.a, in.b, in.c, &out->a, &out->b, &out->c);
}

template<class ObjT, class Method, class InA, class InB, class InC, class InD,
         class OutA, class OutB, class OutC>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple4<InA, InB, InC, InD>& in,
                             Tuple3<OutA, OutB, OutC>* out) {
  (obj->*method)(in.a, in.b, in.c, in.d, &out->a, &out->b, &out->c);
}

template<class ObjT, class Method,
         class InA, class InB, class InC, class InD, class InE,
         class OutA, class OutB, class OutC>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple5<InA, InB, InC, InD, InE>& in,
                             Tuple3<OutA, OutB, OutC>* out) {
  (obj->*method)(in.a, in.b, in.c, in.d, in.e, &out->a, &out->b, &out->c);
}

template<class ObjT, class Method,
         class InA, class InB, class InC, class InD, class InE, class InF,
         class OutA, class OutB, class OutC>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple6<InA, InB, InC, InD, InE, InF>& in,
                             Tuple3<OutA, OutB, OutC>* out) {
  (obj->*method)(in.a, in.b, in.c, in.d, in.e, in.f, &out->a, &out->b, &out->c);
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
  (obj->*method)(in, &out->a, &out->b, &out->c, &out->d);
}

template<class ObjT, class Method, class InA,
         class OutA, class OutB, class OutC, class OutD>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple1<InA>& in,
                             Tuple4<OutA, OutB, OutC, OutD>* out) {
  (obj->*method)(in.a, &out->a, &out->b, &out->c, &out->d);
}

template<class ObjT, class Method, class InA, class InB,
         class OutA, class OutB, class OutC, class OutD>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple2<InA, InB>& in,
                             Tuple4<OutA, OutB, OutC, OutD>* out) {
  (obj->*method)(in.a, in.b, &out->a, &out->b, &out->c, &out->d);
}

template<class ObjT, class Method, class InA, class InB, class InC,
         class OutA, class OutB, class OutC, class OutD>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple3<InA, InB, InC>& in,
                             Tuple4<OutA, OutB, OutC, OutD>* out) {
  (obj->*method)(in.a, in.b, in.c, &out->a, &out->b, &out->c, &out->d);
}

template<class ObjT, class Method, class InA, class InB, class InC, class InD,
         class OutA, class OutB, class OutC, class OutD>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple4<InA, InB, InC, InD>& in,
                             Tuple4<OutA, OutB, OutC, OutD>* out) {
  (obj->*method)(in.a, in.b, in.c, in.d, &out->a, &out->b, &out->c, &out->d);
}

template<class ObjT, class Method,
         class InA, class InB, class InC, class InD, class InE,
         class OutA, class OutB, class OutC, class OutD>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple5<InA, InB, InC, InD, InE>& in,
                             Tuple4<OutA, OutB, OutC, OutD>* out) {
  (obj->*method)(in.a, in.b, in.c, in.d, in.e,
                 &out->a, &out->b, &out->c, &out->d);
}

template<class ObjT, class Method,
         class InA, class InB, class InC, class InD, class InE, class InF,
         class OutA, class OutB, class OutC, class OutD>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple6<InA, InB, InC, InD, InE, InF>& in,
                             Tuple4<OutA, OutB, OutC, OutD>* out) {
  (obj->*method)(in.a, in.b, in.c, in.d, in.e, in.f,
                 &out->a, &out->b, &out->c, &out->d);
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
  (obj->*method)(in, &out->a, &out->b, &out->c, &out->d, &out->e);
}

template<class ObjT, class Method, class InA,
         class OutA, class OutB, class OutC, class OutD, class OutE>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple1<InA>& in,
                             Tuple5<OutA, OutB, OutC, OutD, OutE>* out) {
  (obj->*method)(in.a, &out->a, &out->b, &out->c, &out->d, &out->e);
}

template<class ObjT, class Method, class InA, class InB,
         class OutA, class OutB, class OutC, class OutD, class OutE>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple2<InA, InB>& in,
                             Tuple5<OutA, OutB, OutC, OutD, OutE>* out) {
  (obj->*method)(in.a, in.b, &out->a, &out->b, &out->c, &out->d, &out->e);
}

template<class ObjT, class Method, class InA, class InB, class InC,
         class OutA, class OutB, class OutC, class OutD, class OutE>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple3<InA, InB, InC>& in,
                             Tuple5<OutA, OutB, OutC, OutD, OutE>* out) {
  (obj->*method)(in.a, in.b, in.c, &out->a, &out->b, &out->c, &out->d, &out->e);
}

template<class ObjT, class Method, class InA, class InB, class InC, class InD,
         class OutA, class OutB, class OutC, class OutD, class OutE>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple4<InA, InB, InC, InD>& in,
                             Tuple5<OutA, OutB, OutC, OutD, OutE>* out) {
  (obj->*method)(in.a, in.b, in.c, in.d, &out->a, &out->b, &out->c, &out->d,
                 &out->e);
}

template<class ObjT, class Method,
         class InA, class InB, class InC, class InD, class InE,
         class OutA, class OutB, class OutC, class OutD, class OutE>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple5<InA, InB, InC, InD, InE>& in,
                             Tuple5<OutA, OutB, OutC, OutD, OutE>* out) {
  (obj->*method)(in.a, in.b, in.c, in.d, in.e,
                 &out->a, &out->b, &out->c, &out->d, &out->e);
}

template<class ObjT, class Method,
         class InA, class InB, class InC, class InD, class InE, class InF,
         class OutA, class OutB, class OutC, class OutD, class OutE>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple6<InA, InB, InC, InD, InE, InF>& in,
                             Tuple5<OutA, OutB, OutC, OutD, OutE>* out) {
  (obj->*method)(in.a, in.b, in.c, in.d, in.e, in.f,
                 &out->a, &out->b, &out->c, &out->d, &out->e);
}

#endif  
