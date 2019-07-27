








































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

#else

extern class Placeholder1 { } _1;
extern class Placeholder2 { } _2;
extern class Placeholder3 { } _3;

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

template <typename R, typename C1, typename P1, typename P2, typename P3>
class BindToMemberFunction3
{
public:
  typedef R (C1::*F)(P1&, P2&, P3&);
  BindToMemberFunction3(F f, C1* that) : f(f), that(that) { }
  R operator()(P1& p1, P2& p2, P3& p3) const { return (that->*f)(p1, p2, p3); }
private:
  const F f;
  C1* const that;
  void operator=(const BindToMemberFunction3&) ;
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

template <typename R, typename P1, typename B1, typename B2, typename B3,
          typename B4>
inline internal::Bind4<R, P1, const B1, const B2, B3, B4>
bind(R (*f)(P1&, B1, B2, B3&, B4&), Placeholder1&, const B1& b1, const B2& b2,
     B3& b3, B4& b4)
{
  return internal::Bind4<R, P1, const B1, const B2, B3, B4>(f, b1, b2, b3, b4);
}

template <typename R, typename C1, typename P1, typename P2, typename P3>
inline internal::BindToMemberFunction3<R, C1, P1, P2, P3>
bind(R (C1::*f)(P1&, P2&, P3&), C1* that, Placeholder1&, Placeholder2&,
     Placeholder3&)
{
  return internal::BindToMemberFunction3<R, C1, P1, P2, P3>(f, that);
}

#endif

} } 

#endif 
