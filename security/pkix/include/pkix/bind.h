








































#ifndef mozilla_pkix__bind_h
#define mozilla_pkix__bind_h

#ifdef MOZILLA_PKIX_USE_REAL_FUNCTIONAL
#include <functional>
#endif

namespace mozilla { namespace pkix {

#ifdef MOZILLA_PKIX_USE_REAL_FUNCTIONAL

using std::bind;
using std::cref;
using std::ref;
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using std::placeholders::_4;

#else

extern class Placeholder1 { } _1;
extern class Placeholder2 { } _2;
extern class Placeholder3 { } _3;
extern class Placeholder4 { } _4;

template <typename V>       V&  ref(V& v)       { return v; }
template <typename V> const V& cref(const V& v) { return v; }

namespace internal {

template <typename R, typename P1, typename B1>
class Bind1
{
public:
  typedef R (*F)(P1 &, B1 &);
  Bind1(F f, B1 & b1) : f(f), b1(b1) { }
  R operator()(P1 & p1) const { return f(p1, b1); }
private:
  const F f;
  B1& b1;
  void operator=(const Bind1&) ;
};

template <typename R, typename P1, typename B1, typename B2>
class Bind2
{
public:
  typedef R (*F)(P1&, B1&, B2&);
  Bind2(F f, B1& b1, B2& b2) : f(f), b1(b1), b2(b2) { }
  R operator()(P1& p1) const { return f(p1, b1, b2); }
private:
  const F f;
  B1& b1;
  B2& b2;
  void operator=(const Bind2&) ;
};

template <typename R, typename P1, typename B1, typename B2, typename B3>
class Bind3
{
public:
  typedef R (*F)(P1&, B1, B2, B3&);
  Bind3(F f, B1& b1, B2& b2, B3& b3)
    : f(f), b1(b1), b2(b2), b3(b3) { }
  R operator()(P1& p1) const { return f(p1, b1, b2, b3); }
private:
  const F f;
  B1& b1;
  B2& b2;
  B3& b3;
  void operator=(const Bind3&) ;
};

template <typename R, typename P1, typename B1, typename B2, typename B3,
          typename B4>
class Bind4
{
public:
  typedef R (*F)(P1&, B1, B2, B3&, B4&);
  Bind4(F f, B1& b1, B2& b2, B3& b3, B4& b4)
    : f(f), b1(b1), b2(b2), b3(b3), b4(b4) { }
  R operator()(P1& p1) const { return f(p1, b1, b2, b3, b4); }
private:
  const F f;
  B1& b1;
  B2& b2;
  B3& b3;
  B4& b4;
  void operator=(const Bind4&) ;
};

template <typename R, typename C1, typename P1, typename P2, typename P3,
          typename P4>
class BindToMemberFunction4
{
public:
  typedef R (C1::*F)(P1&, P2&, P3, P4&);
  BindToMemberFunction4(F f, C1* that) : f(f), that(that) { }
  R operator()(P1& p1, P2& p2, P3 p3, P4& p4) const { return (that->*f)(p1, p2, p3, p4); }
private:
  const F f;
  C1* const that;
  void operator=(const BindToMemberFunction4&) ;
};

template <typename R, typename P1, typename B1, typename B2, typename B3,
          typename B4, typename B5>
class Bind5
{
public:
  typedef R (*F)(P1&, B1, B2, B3, B4, B5);
  Bind5(F f, B1 b1, B2 b2, B3 b3, B4 b4, B5 b5)
    : f(f), b1(b1), b2(b2), b3(b3), b4(b4), b5(b5) { }
  R operator()(P1& p1) const { return f(p1, b1, b2, b3, b4, b5); }
private:
  const F f;
  B1 b1;
  B2 b2;
  B3 b3;
  B4 b4;
  B5 b5;
  void operator=(const Bind5&) ;
};

} 

template <typename R, typename P1, typename B1>
inline internal::Bind1<R, P1, B1>
bind(R (*f)(P1&, B1&), Placeholder1&, B1& b1)
{
  return internal::Bind1<R, P1, B1>(f, b1);
}

template <typename R, typename P1, typename B1, typename B2>
inline internal::Bind2<R, P1, B1, B2>
bind(R (*f)(P1&, B1&, B2&), Placeholder1&, B1& b1, B2& b2)
{
  return internal::Bind2<R, P1, B1, B2>(f, b1, b2);
}

template <typename R, typename P1, typename B1, typename B2, typename B3>
inline internal::Bind3<R, P1, const B1, const B2, B3>
bind(R (*f)(P1&, B1, B2, B3&), Placeholder1&, const B1& b1, const B2& b2,
     B3& b3)
{
  return internal::Bind3<R, P1, const B1, const B2, B3>(f, b1, b2, b3);
}

template <typename R, typename P1, typename B1, typename B2, typename B3,
          typename B4>
inline internal::Bind4<R, P1, const B1, const B2, B3, B4>
bind(R (*f)(P1&, B1, B2, B3&, B4&), Placeholder1&, const B1& b1, const B2& b2,
     B3& b3, B4& b4)
{
  return internal::Bind4<R, P1, const B1, const B2, B3, B4>(f, b1, b2, b3, b4);
}

template <typename R, typename C1, typename P1, typename P2, typename P3,
          typename P4>
inline internal::BindToMemberFunction4<R, C1, P1, P2, P3, P4>
bind(R (C1::*f)(P1&, P2&, P3, P4&), C1* that, Placeholder1&, Placeholder2&,
     Placeholder3, Placeholder4&)
{
  return internal::BindToMemberFunction4<R, C1, P1, P2, P3, P4>(f, that);
}

template <typename R, typename P1, typename B1, typename B2, typename B3,
          typename B4, typename B5>
inline internal::Bind5<R, P1, B1, B2, B3, B4, B5&>
bind(R (*f)(P1&, B1, B2, B3, B4, B5&), Placeholder1&, B1 b1, B2 b2, B3 b3,
     B4 b4, B5& b5)
{
  return internal::Bind5<R, P1, B1, B2, B3, B4, B5&>(f, b1, b2, b3, b4, b5);
}

#endif

} } 

#endif 
