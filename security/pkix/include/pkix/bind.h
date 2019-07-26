



























#ifndef mozilla_pkix__bind_h
#define mozilla_pkix__bind_h

#ifdef _MSC_VER
#pragma warning(disable:4275) //Suppress spurious MSVC warning
#endif
#include <functional>
#ifdef _MSC_VER
#pragma warning(default:4275)
#endif

namespace mozilla { namespace pkix {

#ifdef _MSC_VER

using std::bind;
using std::ref;
using std::cref;
using std::placeholders::_1;

#else

class Placeholder1 { };
extern Placeholder1 _1;

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
bind(R (*f)(P1&, B1&, B2&), Placeholder1 &, B1 & b1, B2 & b2)
{
  return internal::Bind2<R, P1, B1, B2>(f, b1, b2);
}

#endif 

} } 

#endif 
