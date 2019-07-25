





































#ifndef GMOCK_INCLUDE_GMOCK_INTERNAL_GMOCK_GENERATED_INTERNAL_UTILS_H_
#define GMOCK_INCLUDE_GMOCK_INTERNAL_GMOCK_GENERATED_INTERNAL_UTILS_H_

#include "gmock/internal/gmock-port.h"

namespace testing {

template <typename T>
class Matcher;

namespace internal {



class IgnoredValue {
 public:
  
  
  
  
  
  template <typename T>
  IgnoredValue(const T&) {}
};



template <typename Tuple>
struct MatcherTuple;

template <>
struct MatcherTuple< ::std::tr1::tuple<> > {
  typedef ::std::tr1::tuple< > type;
};

template <typename A1>
struct MatcherTuple< ::std::tr1::tuple<A1> > {
  typedef ::std::tr1::tuple<Matcher<A1> > type;
};

template <typename A1, typename A2>
struct MatcherTuple< ::std::tr1::tuple<A1, A2> > {
  typedef ::std::tr1::tuple<Matcher<A1>, Matcher<A2> > type;
};

template <typename A1, typename A2, typename A3>
struct MatcherTuple< ::std::tr1::tuple<A1, A2, A3> > {
  typedef ::std::tr1::tuple<Matcher<A1>, Matcher<A2>, Matcher<A3> > type;
};

template <typename A1, typename A2, typename A3, typename A4>
struct MatcherTuple< ::std::tr1::tuple<A1, A2, A3, A4> > {
  typedef ::std::tr1::tuple<Matcher<A1>, Matcher<A2>, Matcher<A3>,
      Matcher<A4> > type;
};

template <typename A1, typename A2, typename A3, typename A4, typename A5>
struct MatcherTuple< ::std::tr1::tuple<A1, A2, A3, A4, A5> > {
  typedef ::std::tr1::tuple<Matcher<A1>, Matcher<A2>, Matcher<A3>, Matcher<A4>,
      Matcher<A5> > type;
};

template <typename A1, typename A2, typename A3, typename A4, typename A5,
    typename A6>
struct MatcherTuple< ::std::tr1::tuple<A1, A2, A3, A4, A5, A6> > {
  typedef ::std::tr1::tuple<Matcher<A1>, Matcher<A2>, Matcher<A3>, Matcher<A4>,
      Matcher<A5>, Matcher<A6> > type;
};

template <typename A1, typename A2, typename A3, typename A4, typename A5,
    typename A6, typename A7>
struct MatcherTuple< ::std::tr1::tuple<A1, A2, A3, A4, A5, A6, A7> > {
  typedef ::std::tr1::tuple<Matcher<A1>, Matcher<A2>, Matcher<A3>, Matcher<A4>,
      Matcher<A5>, Matcher<A6>, Matcher<A7> > type;
};

template <typename A1, typename A2, typename A3, typename A4, typename A5,
    typename A6, typename A7, typename A8>
struct MatcherTuple< ::std::tr1::tuple<A1, A2, A3, A4, A5, A6, A7, A8> > {
  typedef ::std::tr1::tuple<Matcher<A1>, Matcher<A2>, Matcher<A3>, Matcher<A4>,
      Matcher<A5>, Matcher<A6>, Matcher<A7>, Matcher<A8> > type;
};

template <typename A1, typename A2, typename A3, typename A4, typename A5,
    typename A6, typename A7, typename A8, typename A9>
struct MatcherTuple< ::std::tr1::tuple<A1, A2, A3, A4, A5, A6, A7, A8, A9> > {
  typedef ::std::tr1::tuple<Matcher<A1>, Matcher<A2>, Matcher<A3>, Matcher<A4>,
      Matcher<A5>, Matcher<A6>, Matcher<A7>, Matcher<A8>, Matcher<A9> > type;
};

template <typename A1, typename A2, typename A3, typename A4, typename A5,
    typename A6, typename A7, typename A8, typename A9, typename A10>
struct MatcherTuple< ::std::tr1::tuple<A1, A2, A3, A4, A5, A6, A7, A8, A9,
    A10> > {
  typedef ::std::tr1::tuple<Matcher<A1>, Matcher<A2>, Matcher<A3>, Matcher<A4>,
      Matcher<A5>, Matcher<A6>, Matcher<A7>, Matcher<A8>, Matcher<A9>,
      Matcher<A10> > type;
};














template <typename F>
struct Function;

template <typename R>
struct Function<R()> {
  typedef R Result;
  typedef ::std::tr1::tuple<> ArgumentTuple;
  typedef typename MatcherTuple<ArgumentTuple>::type ArgumentMatcherTuple;
  typedef void MakeResultVoid();
  typedef IgnoredValue MakeResultIgnoredValue();
};

template <typename R, typename A1>
struct Function<R(A1)>
    : Function<R()> {
  typedef A1 Argument1;
  typedef ::std::tr1::tuple<A1> ArgumentTuple;
  typedef typename MatcherTuple<ArgumentTuple>::type ArgumentMatcherTuple;
  typedef void MakeResultVoid(A1);
  typedef IgnoredValue MakeResultIgnoredValue(A1);
};

template <typename R, typename A1, typename A2>
struct Function<R(A1, A2)>
    : Function<R(A1)> {
  typedef A2 Argument2;
  typedef ::std::tr1::tuple<A1, A2> ArgumentTuple;
  typedef typename MatcherTuple<ArgumentTuple>::type ArgumentMatcherTuple;
  typedef void MakeResultVoid(A1, A2);
  typedef IgnoredValue MakeResultIgnoredValue(A1, A2);
};

template <typename R, typename A1, typename A2, typename A3>
struct Function<R(A1, A2, A3)>
    : Function<R(A1, A2)> {
  typedef A3 Argument3;
  typedef ::std::tr1::tuple<A1, A2, A3> ArgumentTuple;
  typedef typename MatcherTuple<ArgumentTuple>::type ArgumentMatcherTuple;
  typedef void MakeResultVoid(A1, A2, A3);
  typedef IgnoredValue MakeResultIgnoredValue(A1, A2, A3);
};

template <typename R, typename A1, typename A2, typename A3, typename A4>
struct Function<R(A1, A2, A3, A4)>
    : Function<R(A1, A2, A3)> {
  typedef A4 Argument4;
  typedef ::std::tr1::tuple<A1, A2, A3, A4> ArgumentTuple;
  typedef typename MatcherTuple<ArgumentTuple>::type ArgumentMatcherTuple;
  typedef void MakeResultVoid(A1, A2, A3, A4);
  typedef IgnoredValue MakeResultIgnoredValue(A1, A2, A3, A4);
};

template <typename R, typename A1, typename A2, typename A3, typename A4,
    typename A5>
struct Function<R(A1, A2, A3, A4, A5)>
    : Function<R(A1, A2, A3, A4)> {
  typedef A5 Argument5;
  typedef ::std::tr1::tuple<A1, A2, A3, A4, A5> ArgumentTuple;
  typedef typename MatcherTuple<ArgumentTuple>::type ArgumentMatcherTuple;
  typedef void MakeResultVoid(A1, A2, A3, A4, A5);
  typedef IgnoredValue MakeResultIgnoredValue(A1, A2, A3, A4, A5);
};

template <typename R, typename A1, typename A2, typename A3, typename A4,
    typename A5, typename A6>
struct Function<R(A1, A2, A3, A4, A5, A6)>
    : Function<R(A1, A2, A3, A4, A5)> {
  typedef A6 Argument6;
  typedef ::std::tr1::tuple<A1, A2, A3, A4, A5, A6> ArgumentTuple;
  typedef typename MatcherTuple<ArgumentTuple>::type ArgumentMatcherTuple;
  typedef void MakeResultVoid(A1, A2, A3, A4, A5, A6);
  typedef IgnoredValue MakeResultIgnoredValue(A1, A2, A3, A4, A5, A6);
};

template <typename R, typename A1, typename A2, typename A3, typename A4,
    typename A5, typename A6, typename A7>
struct Function<R(A1, A2, A3, A4, A5, A6, A7)>
    : Function<R(A1, A2, A3, A4, A5, A6)> {
  typedef A7 Argument7;
  typedef ::std::tr1::tuple<A1, A2, A3, A4, A5, A6, A7> ArgumentTuple;
  typedef typename MatcherTuple<ArgumentTuple>::type ArgumentMatcherTuple;
  typedef void MakeResultVoid(A1, A2, A3, A4, A5, A6, A7);
  typedef IgnoredValue MakeResultIgnoredValue(A1, A2, A3, A4, A5, A6, A7);
};

template <typename R, typename A1, typename A2, typename A3, typename A4,
    typename A5, typename A6, typename A7, typename A8>
struct Function<R(A1, A2, A3, A4, A5, A6, A7, A8)>
    : Function<R(A1, A2, A3, A4, A5, A6, A7)> {
  typedef A8 Argument8;
  typedef ::std::tr1::tuple<A1, A2, A3, A4, A5, A6, A7, A8> ArgumentTuple;
  typedef typename MatcherTuple<ArgumentTuple>::type ArgumentMatcherTuple;
  typedef void MakeResultVoid(A1, A2, A3, A4, A5, A6, A7, A8);
  typedef IgnoredValue MakeResultIgnoredValue(A1, A2, A3, A4, A5, A6, A7, A8);
};

template <typename R, typename A1, typename A2, typename A3, typename A4,
    typename A5, typename A6, typename A7, typename A8, typename A9>
struct Function<R(A1, A2, A3, A4, A5, A6, A7, A8, A9)>
    : Function<R(A1, A2, A3, A4, A5, A6, A7, A8)> {
  typedef A9 Argument9;
  typedef ::std::tr1::tuple<A1, A2, A3, A4, A5, A6, A7, A8, A9> ArgumentTuple;
  typedef typename MatcherTuple<ArgumentTuple>::type ArgumentMatcherTuple;
  typedef void MakeResultVoid(A1, A2, A3, A4, A5, A6, A7, A8, A9);
  typedef IgnoredValue MakeResultIgnoredValue(A1, A2, A3, A4, A5, A6, A7, A8,
      A9);
};

template <typename R, typename A1, typename A2, typename A3, typename A4,
    typename A5, typename A6, typename A7, typename A8, typename A9,
    typename A10>
struct Function<R(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10)>
    : Function<R(A1, A2, A3, A4, A5, A6, A7, A8, A9)> {
  typedef A10 Argument10;
  typedef ::std::tr1::tuple<A1, A2, A3, A4, A5, A6, A7, A8, A9,
      A10> ArgumentTuple;
  typedef typename MatcherTuple<ArgumentTuple>::type ArgumentMatcherTuple;
  typedef void MakeResultVoid(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10);
  typedef IgnoredValue MakeResultIgnoredValue(A1, A2, A3, A4, A5, A6, A7, A8,
      A9, A10);
};

}  

}  

#endif  
