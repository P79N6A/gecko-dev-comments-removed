




































#ifndef GMOCK_INCLUDE_GMOCK_GMOCK_GENERATED_MATCHERS_H_
#define GMOCK_INCLUDE_GMOCK_GMOCK_GENERATED_MATCHERS_H_

#include <sstream>
#include <string>
#include <vector>
#include "gmock/gmock-matchers.h"

namespace testing {
namespace internal {


#define GMOCK_FIELD_TYPE_(Tuple, i) \
    typename ::std::tr1::tuple_element<i, Tuple>::type












template <class Tuple, int k0 = -1, int k1 = -1, int k2 = -1, int k3 = -1,
    int k4 = -1, int k5 = -1, int k6 = -1, int k7 = -1, int k8 = -1,
    int k9 = -1>
class TupleFields;


template <class Tuple, int k0, int k1, int k2, int k3, int k4, int k5, int k6,
    int k7, int k8, int k9>
class TupleFields {
 public:
  typedef ::std::tr1::tuple<GMOCK_FIELD_TYPE_(Tuple, k0),
      GMOCK_FIELD_TYPE_(Tuple, k1), GMOCK_FIELD_TYPE_(Tuple, k2),
      GMOCK_FIELD_TYPE_(Tuple, k3), GMOCK_FIELD_TYPE_(Tuple, k4),
      GMOCK_FIELD_TYPE_(Tuple, k5), GMOCK_FIELD_TYPE_(Tuple, k6),
      GMOCK_FIELD_TYPE_(Tuple, k7), GMOCK_FIELD_TYPE_(Tuple, k8),
      GMOCK_FIELD_TYPE_(Tuple, k9)> type;
  static type GetSelectedFields(const Tuple& t) {
    using ::std::tr1::get;
    return type(get<k0>(t), get<k1>(t), get<k2>(t), get<k3>(t), get<k4>(t),
        get<k5>(t), get<k6>(t), get<k7>(t), get<k8>(t), get<k9>(t));
  }
};



template <class Tuple>
class TupleFields<Tuple, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1> {
 public:
  typedef ::std::tr1::tuple<> type;
  static type GetSelectedFields(const Tuple& ) {
    using ::std::tr1::get;
    return type();
  }
};

template <class Tuple, int k0>
class TupleFields<Tuple, k0, -1, -1, -1, -1, -1, -1, -1, -1, -1> {
 public:
  typedef ::std::tr1::tuple<GMOCK_FIELD_TYPE_(Tuple, k0)> type;
  static type GetSelectedFields(const Tuple& t) {
    using ::std::tr1::get;
    return type(get<k0>(t));
  }
};

template <class Tuple, int k0, int k1>
class TupleFields<Tuple, k0, k1, -1, -1, -1, -1, -1, -1, -1, -1> {
 public:
  typedef ::std::tr1::tuple<GMOCK_FIELD_TYPE_(Tuple, k0),
      GMOCK_FIELD_TYPE_(Tuple, k1)> type;
  static type GetSelectedFields(const Tuple& t) {
    using ::std::tr1::get;
    return type(get<k0>(t), get<k1>(t));
  }
};

template <class Tuple, int k0, int k1, int k2>
class TupleFields<Tuple, k0, k1, k2, -1, -1, -1, -1, -1, -1, -1> {
 public:
  typedef ::std::tr1::tuple<GMOCK_FIELD_TYPE_(Tuple, k0),
      GMOCK_FIELD_TYPE_(Tuple, k1), GMOCK_FIELD_TYPE_(Tuple, k2)> type;
  static type GetSelectedFields(const Tuple& t) {
    using ::std::tr1::get;
    return type(get<k0>(t), get<k1>(t), get<k2>(t));
  }
};

template <class Tuple, int k0, int k1, int k2, int k3>
class TupleFields<Tuple, k0, k1, k2, k3, -1, -1, -1, -1, -1, -1> {
 public:
  typedef ::std::tr1::tuple<GMOCK_FIELD_TYPE_(Tuple, k0),
      GMOCK_FIELD_TYPE_(Tuple, k1), GMOCK_FIELD_TYPE_(Tuple, k2),
      GMOCK_FIELD_TYPE_(Tuple, k3)> type;
  static type GetSelectedFields(const Tuple& t) {
    using ::std::tr1::get;
    return type(get<k0>(t), get<k1>(t), get<k2>(t), get<k3>(t));
  }
};

template <class Tuple, int k0, int k1, int k2, int k3, int k4>
class TupleFields<Tuple, k0, k1, k2, k3, k4, -1, -1, -1, -1, -1> {
 public:
  typedef ::std::tr1::tuple<GMOCK_FIELD_TYPE_(Tuple, k0),
      GMOCK_FIELD_TYPE_(Tuple, k1), GMOCK_FIELD_TYPE_(Tuple, k2),
      GMOCK_FIELD_TYPE_(Tuple, k3), GMOCK_FIELD_TYPE_(Tuple, k4)> type;
  static type GetSelectedFields(const Tuple& t) {
    using ::std::tr1::get;
    return type(get<k0>(t), get<k1>(t), get<k2>(t), get<k3>(t), get<k4>(t));
  }
};

template <class Tuple, int k0, int k1, int k2, int k3, int k4, int k5>
class TupleFields<Tuple, k0, k1, k2, k3, k4, k5, -1, -1, -1, -1> {
 public:
  typedef ::std::tr1::tuple<GMOCK_FIELD_TYPE_(Tuple, k0),
      GMOCK_FIELD_TYPE_(Tuple, k1), GMOCK_FIELD_TYPE_(Tuple, k2),
      GMOCK_FIELD_TYPE_(Tuple, k3), GMOCK_FIELD_TYPE_(Tuple, k4),
      GMOCK_FIELD_TYPE_(Tuple, k5)> type;
  static type GetSelectedFields(const Tuple& t) {
    using ::std::tr1::get;
    return type(get<k0>(t), get<k1>(t), get<k2>(t), get<k3>(t), get<k4>(t),
        get<k5>(t));
  }
};

template <class Tuple, int k0, int k1, int k2, int k3, int k4, int k5, int k6>
class TupleFields<Tuple, k0, k1, k2, k3, k4, k5, k6, -1, -1, -1> {
 public:
  typedef ::std::tr1::tuple<GMOCK_FIELD_TYPE_(Tuple, k0),
      GMOCK_FIELD_TYPE_(Tuple, k1), GMOCK_FIELD_TYPE_(Tuple, k2),
      GMOCK_FIELD_TYPE_(Tuple, k3), GMOCK_FIELD_TYPE_(Tuple, k4),
      GMOCK_FIELD_TYPE_(Tuple, k5), GMOCK_FIELD_TYPE_(Tuple, k6)> type;
  static type GetSelectedFields(const Tuple& t) {
    using ::std::tr1::get;
    return type(get<k0>(t), get<k1>(t), get<k2>(t), get<k3>(t), get<k4>(t),
        get<k5>(t), get<k6>(t));
  }
};

template <class Tuple, int k0, int k1, int k2, int k3, int k4, int k5, int k6,
    int k7>
class TupleFields<Tuple, k0, k1, k2, k3, k4, k5, k6, k7, -1, -1> {
 public:
  typedef ::std::tr1::tuple<GMOCK_FIELD_TYPE_(Tuple, k0),
      GMOCK_FIELD_TYPE_(Tuple, k1), GMOCK_FIELD_TYPE_(Tuple, k2),
      GMOCK_FIELD_TYPE_(Tuple, k3), GMOCK_FIELD_TYPE_(Tuple, k4),
      GMOCK_FIELD_TYPE_(Tuple, k5), GMOCK_FIELD_TYPE_(Tuple, k6),
      GMOCK_FIELD_TYPE_(Tuple, k7)> type;
  static type GetSelectedFields(const Tuple& t) {
    using ::std::tr1::get;
    return type(get<k0>(t), get<k1>(t), get<k2>(t), get<k3>(t), get<k4>(t),
        get<k5>(t), get<k6>(t), get<k7>(t));
  }
};

template <class Tuple, int k0, int k1, int k2, int k3, int k4, int k5, int k6,
    int k7, int k8>
class TupleFields<Tuple, k0, k1, k2, k3, k4, k5, k6, k7, k8, -1> {
 public:
  typedef ::std::tr1::tuple<GMOCK_FIELD_TYPE_(Tuple, k0),
      GMOCK_FIELD_TYPE_(Tuple, k1), GMOCK_FIELD_TYPE_(Tuple, k2),
      GMOCK_FIELD_TYPE_(Tuple, k3), GMOCK_FIELD_TYPE_(Tuple, k4),
      GMOCK_FIELD_TYPE_(Tuple, k5), GMOCK_FIELD_TYPE_(Tuple, k6),
      GMOCK_FIELD_TYPE_(Tuple, k7), GMOCK_FIELD_TYPE_(Tuple, k8)> type;
  static type GetSelectedFields(const Tuple& t) {
    using ::std::tr1::get;
    return type(get<k0>(t), get<k1>(t), get<k2>(t), get<k3>(t), get<k4>(t),
        get<k5>(t), get<k6>(t), get<k7>(t), get<k8>(t));
  }
};

#undef GMOCK_FIELD_TYPE_


template <class ArgsTuple, int k0 = -1, int k1 = -1, int k2 = -1, int k3 = -1,
    int k4 = -1, int k5 = -1, int k6 = -1, int k7 = -1, int k8 = -1,
    int k9 = -1>
class ArgsMatcherImpl : public MatcherInterface<ArgsTuple> {
 public:
  
  typedef GTEST_REMOVE_REFERENCE_AND_CONST_(ArgsTuple) RawArgsTuple;
  typedef typename internal::TupleFields<RawArgsTuple, k0, k1, k2, k3, k4, k5,
      k6, k7, k8, k9>::type SelectedArgs;
  typedef Matcher<const SelectedArgs&> MonomorphicInnerMatcher;

  template <typename InnerMatcher>
  explicit ArgsMatcherImpl(const InnerMatcher& inner_matcher)
      : inner_matcher_(SafeMatcherCast<const SelectedArgs&>(inner_matcher)) {}

  virtual bool MatchAndExplain(ArgsTuple args,
                               MatchResultListener* listener) const {
    const SelectedArgs& selected_args = GetSelectedArgs(args);
    if (!listener->IsInterested())
      return inner_matcher_.Matches(selected_args);

    PrintIndices(listener->stream());
    *listener << "are " << PrintToString(selected_args);

    StringMatchResultListener inner_listener;
    const bool match = inner_matcher_.MatchAndExplain(selected_args,
                                                      &inner_listener);
    PrintIfNotEmpty(inner_listener.str(), listener->stream());
    return match;
  }

  virtual void DescribeTo(::std::ostream* os) const {
    *os << "are a tuple ";
    PrintIndices(os);
    inner_matcher_.DescribeTo(os);
  }

  virtual void DescribeNegationTo(::std::ostream* os) const {
    *os << "are a tuple ";
    PrintIndices(os);
    inner_matcher_.DescribeNegationTo(os);
  }

 private:
  static SelectedArgs GetSelectedArgs(ArgsTuple args) {
    return TupleFields<RawArgsTuple, k0, k1, k2, k3, k4, k5, k6, k7, k8,
        k9>::GetSelectedFields(args);
  }

  
  static void PrintIndices(::std::ostream* os) {
    *os << "whose fields (";
    const int indices[10] = { k0, k1, k2, k3, k4, k5, k6, k7, k8, k9 };
    for (int i = 0; i < 10; i++) {
      if (indices[i] < 0)
        break;

      if (i >= 1)
        *os << ", ";

      *os << "#" << indices[i];
    }
    *os << ") ";
  }

  const MonomorphicInnerMatcher inner_matcher_;

  GTEST_DISALLOW_ASSIGN_(ArgsMatcherImpl);
};

template <class InnerMatcher, int k0 = -1, int k1 = -1, int k2 = -1,
    int k3 = -1, int k4 = -1, int k5 = -1, int k6 = -1, int k7 = -1,
    int k8 = -1, int k9 = -1>
class ArgsMatcher {
 public:
  explicit ArgsMatcher(const InnerMatcher& inner_matcher)
      : inner_matcher_(inner_matcher) {}

  template <typename ArgsTuple>
  operator Matcher<ArgsTuple>() const {
    return MakeMatcher(new ArgsMatcherImpl<ArgsTuple, k0, k1, k2, k3, k4, k5,
        k6, k7, k8, k9>(inner_matcher_));
  }

 private:
  const InnerMatcher inner_matcher_;

  GTEST_DISALLOW_ASSIGN_(ArgsMatcher);
};



template <typename T1>
class ElementsAreMatcher1 {
 public:
  explicit ElementsAreMatcher1(const T1& e1) : e1_(e1) {}

  template <typename Container>
  operator Matcher<Container>() const {
    typedef GTEST_REMOVE_REFERENCE_AND_CONST_(Container) RawContainer;
    typedef typename internal::StlContainerView<RawContainer>::type::value_type
        Element;

    
    
    
    
    
    
    const Matcher<const Element&> matcher =
        MatcherCast<const Element&>(e1_);
    return MakeMatcher(new ElementsAreMatcherImpl<Container>(&matcher, 1));
  }

 private:
  const T1& e1_;

  GTEST_DISALLOW_ASSIGN_(ElementsAreMatcher1);
};

template <typename T1, typename T2>
class ElementsAreMatcher2 {
 public:
  ElementsAreMatcher2(const T1& e1, const T2& e2) : e1_(e1), e2_(e2) {}

  template <typename Container>
  operator Matcher<Container>() const {
    typedef GTEST_REMOVE_REFERENCE_AND_CONST_(Container) RawContainer;
    typedef typename internal::StlContainerView<RawContainer>::type::value_type
        Element;

    const Matcher<const Element&> matchers[] = {
      MatcherCast<const Element&>(e1_),
      MatcherCast<const Element&>(e2_),
    };

    return MakeMatcher(new ElementsAreMatcherImpl<Container>(matchers, 2));
  }

 private:
  const T1& e1_;
  const T2& e2_;

  GTEST_DISALLOW_ASSIGN_(ElementsAreMatcher2);
};

template <typename T1, typename T2, typename T3>
class ElementsAreMatcher3 {
 public:
  ElementsAreMatcher3(const T1& e1, const T2& e2, const T3& e3) : e1_(e1),
      e2_(e2), e3_(e3) {}

  template <typename Container>
  operator Matcher<Container>() const {
    typedef GTEST_REMOVE_REFERENCE_AND_CONST_(Container) RawContainer;
    typedef typename internal::StlContainerView<RawContainer>::type::value_type
        Element;

    const Matcher<const Element&> matchers[] = {
      MatcherCast<const Element&>(e1_),
      MatcherCast<const Element&>(e2_),
      MatcherCast<const Element&>(e3_),
    };

    return MakeMatcher(new ElementsAreMatcherImpl<Container>(matchers, 3));
  }

 private:
  const T1& e1_;
  const T2& e2_;
  const T3& e3_;

  GTEST_DISALLOW_ASSIGN_(ElementsAreMatcher3);
};

template <typename T1, typename T2, typename T3, typename T4>
class ElementsAreMatcher4 {
 public:
  ElementsAreMatcher4(const T1& e1, const T2& e2, const T3& e3,
      const T4& e4) : e1_(e1), e2_(e2), e3_(e3), e4_(e4) {}

  template <typename Container>
  operator Matcher<Container>() const {
    typedef GTEST_REMOVE_REFERENCE_AND_CONST_(Container) RawContainer;
    typedef typename internal::StlContainerView<RawContainer>::type::value_type
        Element;

    const Matcher<const Element&> matchers[] = {
      MatcherCast<const Element&>(e1_),
      MatcherCast<const Element&>(e2_),
      MatcherCast<const Element&>(e3_),
      MatcherCast<const Element&>(e4_),
    };

    return MakeMatcher(new ElementsAreMatcherImpl<Container>(matchers, 4));
  }

 private:
  const T1& e1_;
  const T2& e2_;
  const T3& e3_;
  const T4& e4_;

  GTEST_DISALLOW_ASSIGN_(ElementsAreMatcher4);
};

template <typename T1, typename T2, typename T3, typename T4, typename T5>
class ElementsAreMatcher5 {
 public:
  ElementsAreMatcher5(const T1& e1, const T2& e2, const T3& e3, const T4& e4,
      const T5& e5) : e1_(e1), e2_(e2), e3_(e3), e4_(e4), e5_(e5) {}

  template <typename Container>
  operator Matcher<Container>() const {
    typedef GTEST_REMOVE_REFERENCE_AND_CONST_(Container) RawContainer;
    typedef typename internal::StlContainerView<RawContainer>::type::value_type
        Element;

    const Matcher<const Element&> matchers[] = {
      MatcherCast<const Element&>(e1_),
      MatcherCast<const Element&>(e2_),
      MatcherCast<const Element&>(e3_),
      MatcherCast<const Element&>(e4_),
      MatcherCast<const Element&>(e5_),
    };

    return MakeMatcher(new ElementsAreMatcherImpl<Container>(matchers, 5));
  }

 private:
  const T1& e1_;
  const T2& e2_;
  const T3& e3_;
  const T4& e4_;
  const T5& e5_;

  GTEST_DISALLOW_ASSIGN_(ElementsAreMatcher5);
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6>
class ElementsAreMatcher6 {
 public:
  ElementsAreMatcher6(const T1& e1, const T2& e2, const T3& e3, const T4& e4,
      const T5& e5, const T6& e6) : e1_(e1), e2_(e2), e3_(e3), e4_(e4),
      e5_(e5), e6_(e6) {}

  template <typename Container>
  operator Matcher<Container>() const {
    typedef GTEST_REMOVE_REFERENCE_AND_CONST_(Container) RawContainer;
    typedef typename internal::StlContainerView<RawContainer>::type::value_type
        Element;

    const Matcher<const Element&> matchers[] = {
      MatcherCast<const Element&>(e1_),
      MatcherCast<const Element&>(e2_),
      MatcherCast<const Element&>(e3_),
      MatcherCast<const Element&>(e4_),
      MatcherCast<const Element&>(e5_),
      MatcherCast<const Element&>(e6_),
    };

    return MakeMatcher(new ElementsAreMatcherImpl<Container>(matchers, 6));
  }

 private:
  const T1& e1_;
  const T2& e2_;
  const T3& e3_;
  const T4& e4_;
  const T5& e5_;
  const T6& e6_;

  GTEST_DISALLOW_ASSIGN_(ElementsAreMatcher6);
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7>
class ElementsAreMatcher7 {
 public:
  ElementsAreMatcher7(const T1& e1, const T2& e2, const T3& e3, const T4& e4,
      const T5& e5, const T6& e6, const T7& e7) : e1_(e1), e2_(e2), e3_(e3),
      e4_(e4), e5_(e5), e6_(e6), e7_(e7) {}

  template <typename Container>
  operator Matcher<Container>() const {
    typedef GTEST_REMOVE_REFERENCE_AND_CONST_(Container) RawContainer;
    typedef typename internal::StlContainerView<RawContainer>::type::value_type
        Element;

    const Matcher<const Element&> matchers[] = {
      MatcherCast<const Element&>(e1_),
      MatcherCast<const Element&>(e2_),
      MatcherCast<const Element&>(e3_),
      MatcherCast<const Element&>(e4_),
      MatcherCast<const Element&>(e5_),
      MatcherCast<const Element&>(e6_),
      MatcherCast<const Element&>(e7_),
    };

    return MakeMatcher(new ElementsAreMatcherImpl<Container>(matchers, 7));
  }

 private:
  const T1& e1_;
  const T2& e2_;
  const T3& e3_;
  const T4& e4_;
  const T5& e5_;
  const T6& e6_;
  const T7& e7_;

  GTEST_DISALLOW_ASSIGN_(ElementsAreMatcher7);
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8>
class ElementsAreMatcher8 {
 public:
  ElementsAreMatcher8(const T1& e1, const T2& e2, const T3& e3, const T4& e4,
      const T5& e5, const T6& e6, const T7& e7, const T8& e8) : e1_(e1),
      e2_(e2), e3_(e3), e4_(e4), e5_(e5), e6_(e6), e7_(e7), e8_(e8) {}

  template <typename Container>
  operator Matcher<Container>() const {
    typedef GTEST_REMOVE_REFERENCE_AND_CONST_(Container) RawContainer;
    typedef typename internal::StlContainerView<RawContainer>::type::value_type
        Element;

    const Matcher<const Element&> matchers[] = {
      MatcherCast<const Element&>(e1_),
      MatcherCast<const Element&>(e2_),
      MatcherCast<const Element&>(e3_),
      MatcherCast<const Element&>(e4_),
      MatcherCast<const Element&>(e5_),
      MatcherCast<const Element&>(e6_),
      MatcherCast<const Element&>(e7_),
      MatcherCast<const Element&>(e8_),
    };

    return MakeMatcher(new ElementsAreMatcherImpl<Container>(matchers, 8));
  }

 private:
  const T1& e1_;
  const T2& e2_;
  const T3& e3_;
  const T4& e4_;
  const T5& e5_;
  const T6& e6_;
  const T7& e7_;
  const T8& e8_;

  GTEST_DISALLOW_ASSIGN_(ElementsAreMatcher8);
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9>
class ElementsAreMatcher9 {
 public:
  ElementsAreMatcher9(const T1& e1, const T2& e2, const T3& e3, const T4& e4,
      const T5& e5, const T6& e6, const T7& e7, const T8& e8,
      const T9& e9) : e1_(e1), e2_(e2), e3_(e3), e4_(e4), e5_(e5), e6_(e6),
      e7_(e7), e8_(e8), e9_(e9) {}

  template <typename Container>
  operator Matcher<Container>() const {
    typedef GTEST_REMOVE_REFERENCE_AND_CONST_(Container) RawContainer;
    typedef typename internal::StlContainerView<RawContainer>::type::value_type
        Element;

    const Matcher<const Element&> matchers[] = {
      MatcherCast<const Element&>(e1_),
      MatcherCast<const Element&>(e2_),
      MatcherCast<const Element&>(e3_),
      MatcherCast<const Element&>(e4_),
      MatcherCast<const Element&>(e5_),
      MatcherCast<const Element&>(e6_),
      MatcherCast<const Element&>(e7_),
      MatcherCast<const Element&>(e8_),
      MatcherCast<const Element&>(e9_),
    };

    return MakeMatcher(new ElementsAreMatcherImpl<Container>(matchers, 9));
  }

 private:
  const T1& e1_;
  const T2& e2_;
  const T3& e3_;
  const T4& e4_;
  const T5& e5_;
  const T6& e6_;
  const T7& e7_;
  const T8& e8_;
  const T9& e9_;

  GTEST_DISALLOW_ASSIGN_(ElementsAreMatcher9);
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10>
class ElementsAreMatcher10 {
 public:
  ElementsAreMatcher10(const T1& e1, const T2& e2, const T3& e3, const T4& e4,
      const T5& e5, const T6& e6, const T7& e7, const T8& e8, const T9& e9,
      const T10& e10) : e1_(e1), e2_(e2), e3_(e3), e4_(e4), e5_(e5), e6_(e6),
      e7_(e7), e8_(e8), e9_(e9), e10_(e10) {}

  template <typename Container>
  operator Matcher<Container>() const {
    typedef GTEST_REMOVE_REFERENCE_AND_CONST_(Container) RawContainer;
    typedef typename internal::StlContainerView<RawContainer>::type::value_type
        Element;

    const Matcher<const Element&> matchers[] = {
      MatcherCast<const Element&>(e1_),
      MatcherCast<const Element&>(e2_),
      MatcherCast<const Element&>(e3_),
      MatcherCast<const Element&>(e4_),
      MatcherCast<const Element&>(e5_),
      MatcherCast<const Element&>(e6_),
      MatcherCast<const Element&>(e7_),
      MatcherCast<const Element&>(e8_),
      MatcherCast<const Element&>(e9_),
      MatcherCast<const Element&>(e10_),
    };

    return MakeMatcher(new ElementsAreMatcherImpl<Container>(matchers, 10));
  }

 private:
  const T1& e1_;
  const T2& e2_;
  const T3& e3_;
  const T4& e4_;
  const T5& e5_;
  const T6& e6_;
  const T7& e7_;
  const T8& e8_;
  const T9& e9_;
  const T10& e10_;

  GTEST_DISALLOW_ASSIGN_(ElementsAreMatcher10);
};

}  




template <typename InnerMatcher>
inline internal::ArgsMatcher<InnerMatcher>
Args(const InnerMatcher& matcher) {
  return internal::ArgsMatcher<InnerMatcher>(matcher);
}

template <int k1, typename InnerMatcher>
inline internal::ArgsMatcher<InnerMatcher, k1>
Args(const InnerMatcher& matcher) {
  return internal::ArgsMatcher<InnerMatcher, k1>(matcher);
}

template <int k1, int k2, typename InnerMatcher>
inline internal::ArgsMatcher<InnerMatcher, k1, k2>
Args(const InnerMatcher& matcher) {
  return internal::ArgsMatcher<InnerMatcher, k1, k2>(matcher);
}

template <int k1, int k2, int k3, typename InnerMatcher>
inline internal::ArgsMatcher<InnerMatcher, k1, k2, k3>
Args(const InnerMatcher& matcher) {
  return internal::ArgsMatcher<InnerMatcher, k1, k2, k3>(matcher);
}

template <int k1, int k2, int k3, int k4, typename InnerMatcher>
inline internal::ArgsMatcher<InnerMatcher, k1, k2, k3, k4>
Args(const InnerMatcher& matcher) {
  return internal::ArgsMatcher<InnerMatcher, k1, k2, k3, k4>(matcher);
}

template <int k1, int k2, int k3, int k4, int k5, typename InnerMatcher>
inline internal::ArgsMatcher<InnerMatcher, k1, k2, k3, k4, k5>
Args(const InnerMatcher& matcher) {
  return internal::ArgsMatcher<InnerMatcher, k1, k2, k3, k4, k5>(matcher);
}

template <int k1, int k2, int k3, int k4, int k5, int k6, typename InnerMatcher>
inline internal::ArgsMatcher<InnerMatcher, k1, k2, k3, k4, k5, k6>
Args(const InnerMatcher& matcher) {
  return internal::ArgsMatcher<InnerMatcher, k1, k2, k3, k4, k5, k6>(matcher);
}

template <int k1, int k2, int k3, int k4, int k5, int k6, int k7,
    typename InnerMatcher>
inline internal::ArgsMatcher<InnerMatcher, k1, k2, k3, k4, k5, k6, k7>
Args(const InnerMatcher& matcher) {
  return internal::ArgsMatcher<InnerMatcher, k1, k2, k3, k4, k5, k6,
      k7>(matcher);
}

template <int k1, int k2, int k3, int k4, int k5, int k6, int k7, int k8,
    typename InnerMatcher>
inline internal::ArgsMatcher<InnerMatcher, k1, k2, k3, k4, k5, k6, k7, k8>
Args(const InnerMatcher& matcher) {
  return internal::ArgsMatcher<InnerMatcher, k1, k2, k3, k4, k5, k6, k7,
      k8>(matcher);
}

template <int k1, int k2, int k3, int k4, int k5, int k6, int k7, int k8,
    int k9, typename InnerMatcher>
inline internal::ArgsMatcher<InnerMatcher, k1, k2, k3, k4, k5, k6, k7, k8, k9>
Args(const InnerMatcher& matcher) {
  return internal::ArgsMatcher<InnerMatcher, k1, k2, k3, k4, k5, k6, k7, k8,
      k9>(matcher);
}

template <int k1, int k2, int k3, int k4, int k5, int k6, int k7, int k8,
    int k9, int k10, typename InnerMatcher>
inline internal::ArgsMatcher<InnerMatcher, k1, k2, k3, k4, k5, k6, k7, k8, k9,
    k10>
Args(const InnerMatcher& matcher) {
  return internal::ArgsMatcher<InnerMatcher, k1, k2, k3, k4, k5, k6, k7, k8,
      k9, k10>(matcher);
}











inline internal::ElementsAreMatcher0 ElementsAre() {
  return internal::ElementsAreMatcher0();
}

template <typename T1>
inline internal::ElementsAreMatcher1<T1> ElementsAre(const T1& e1) {
  return internal::ElementsAreMatcher1<T1>(e1);
}

template <typename T1, typename T2>
inline internal::ElementsAreMatcher2<T1, T2> ElementsAre(const T1& e1,
    const T2& e2) {
  return internal::ElementsAreMatcher2<T1, T2>(e1, e2);
}

template <typename T1, typename T2, typename T3>
inline internal::ElementsAreMatcher3<T1, T2, T3> ElementsAre(const T1& e1,
    const T2& e2, const T3& e3) {
  return internal::ElementsAreMatcher3<T1, T2, T3>(e1, e2, e3);
}

template <typename T1, typename T2, typename T3, typename T4>
inline internal::ElementsAreMatcher4<T1, T2, T3, T4> ElementsAre(const T1& e1,
    const T2& e2, const T3& e3, const T4& e4) {
  return internal::ElementsAreMatcher4<T1, T2, T3, T4>(e1, e2, e3, e4);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5>
inline internal::ElementsAreMatcher5<T1, T2, T3, T4,
    T5> ElementsAre(const T1& e1, const T2& e2, const T3& e3, const T4& e4,
    const T5& e5) {
  return internal::ElementsAreMatcher5<T1, T2, T3, T4, T5>(e1, e2, e3, e4, e5);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6>
inline internal::ElementsAreMatcher6<T1, T2, T3, T4, T5,
    T6> ElementsAre(const T1& e1, const T2& e2, const T3& e3, const T4& e4,
    const T5& e5, const T6& e6) {
  return internal::ElementsAreMatcher6<T1, T2, T3, T4, T5, T6>(e1, e2, e3, e4,
      e5, e6);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7>
inline internal::ElementsAreMatcher7<T1, T2, T3, T4, T5, T6,
    T7> ElementsAre(const T1& e1, const T2& e2, const T3& e3, const T4& e4,
    const T5& e5, const T6& e6, const T7& e7) {
  return internal::ElementsAreMatcher7<T1, T2, T3, T4, T5, T6, T7>(e1, e2, e3,
      e4, e5, e6, e7);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8>
inline internal::ElementsAreMatcher8<T1, T2, T3, T4, T5, T6, T7,
    T8> ElementsAre(const T1& e1, const T2& e2, const T3& e3, const T4& e4,
    const T5& e5, const T6& e6, const T7& e7, const T8& e8) {
  return internal::ElementsAreMatcher8<T1, T2, T3, T4, T5, T6, T7, T8>(e1, e2,
      e3, e4, e5, e6, e7, e8);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9>
inline internal::ElementsAreMatcher9<T1, T2, T3, T4, T5, T6, T7, T8,
    T9> ElementsAre(const T1& e1, const T2& e2, const T3& e3, const T4& e4,
    const T5& e5, const T6& e6, const T7& e7, const T8& e8, const T9& e9) {
  return internal::ElementsAreMatcher9<T1, T2, T3, T4, T5, T6, T7, T8, T9>(e1,
      e2, e3, e4, e5, e6, e7, e8, e9);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10>
inline internal::ElementsAreMatcher10<T1, T2, T3, T4, T5, T6, T7, T8, T9,
    T10> ElementsAre(const T1& e1, const T2& e2, const T3& e3, const T4& e4,
    const T5& e5, const T6& e6, const T7& e7, const T8& e8, const T9& e9,
    const T10& e10) {
  return internal::ElementsAreMatcher10<T1, T2, T3, T4, T5, T6, T7, T8, T9,
      T10>(e1, e2, e3, e4, e5, e6, e7, e8, e9, e10);
}







template <typename T>
inline internal::ElementsAreArrayMatcher<T> ElementsAreArray(
    const T* first, size_t count) {
  return internal::ElementsAreArrayMatcher<T>(first, count);
}

template <typename T, size_t N>
inline internal::ElementsAreArrayMatcher<T>
ElementsAreArray(const T (&array)[N]) {
  return internal::ElementsAreArrayMatcher<T>(array, N);
}




template <typename Matcher1, typename Matcher2>
inline internal::BothOfMatcher<Matcher1, Matcher2>
AllOf(Matcher1 m1, Matcher2 m2) {
  return internal::BothOfMatcher<Matcher1, Matcher2>(m1, m2);
}

template <typename Matcher1, typename Matcher2, typename Matcher3>
inline internal::BothOfMatcher<Matcher1, internal::BothOfMatcher<Matcher2,
    Matcher3> >
AllOf(Matcher1 m1, Matcher2 m2, Matcher3 m3) {
  return ::testing::AllOf(m1, ::testing::AllOf(m2, m3));
}

template <typename Matcher1, typename Matcher2, typename Matcher3,
    typename Matcher4>
inline internal::BothOfMatcher<Matcher1, internal::BothOfMatcher<Matcher2,
    internal::BothOfMatcher<Matcher3, Matcher4> > >
AllOf(Matcher1 m1, Matcher2 m2, Matcher3 m3, Matcher4 m4) {
  return ::testing::AllOf(m1, ::testing::AllOf(m2, m3, m4));
}

template <typename Matcher1, typename Matcher2, typename Matcher3,
    typename Matcher4, typename Matcher5>
inline internal::BothOfMatcher<Matcher1, internal::BothOfMatcher<Matcher2,
    internal::BothOfMatcher<Matcher3, internal::BothOfMatcher<Matcher4,
    Matcher5> > > >
AllOf(Matcher1 m1, Matcher2 m2, Matcher3 m3, Matcher4 m4, Matcher5 m5) {
  return ::testing::AllOf(m1, ::testing::AllOf(m2, m3, m4, m5));
}

template <typename Matcher1, typename Matcher2, typename Matcher3,
    typename Matcher4, typename Matcher5, typename Matcher6>
inline internal::BothOfMatcher<Matcher1, internal::BothOfMatcher<Matcher2,
    internal::BothOfMatcher<Matcher3, internal::BothOfMatcher<Matcher4,
    internal::BothOfMatcher<Matcher5, Matcher6> > > > >
AllOf(Matcher1 m1, Matcher2 m2, Matcher3 m3, Matcher4 m4, Matcher5 m5,
    Matcher6 m6) {
  return ::testing::AllOf(m1, ::testing::AllOf(m2, m3, m4, m5, m6));
}

template <typename Matcher1, typename Matcher2, typename Matcher3,
    typename Matcher4, typename Matcher5, typename Matcher6, typename Matcher7>
inline internal::BothOfMatcher<Matcher1, internal::BothOfMatcher<Matcher2,
    internal::BothOfMatcher<Matcher3, internal::BothOfMatcher<Matcher4,
    internal::BothOfMatcher<Matcher5, internal::BothOfMatcher<Matcher6,
    Matcher7> > > > > >
AllOf(Matcher1 m1, Matcher2 m2, Matcher3 m3, Matcher4 m4, Matcher5 m5,
    Matcher6 m6, Matcher7 m7) {
  return ::testing::AllOf(m1, ::testing::AllOf(m2, m3, m4, m5, m6, m7));
}

template <typename Matcher1, typename Matcher2, typename Matcher3,
    typename Matcher4, typename Matcher5, typename Matcher6, typename Matcher7,
    typename Matcher8>
inline internal::BothOfMatcher<Matcher1, internal::BothOfMatcher<Matcher2,
    internal::BothOfMatcher<Matcher3, internal::BothOfMatcher<Matcher4,
    internal::BothOfMatcher<Matcher5, internal::BothOfMatcher<Matcher6,
    internal::BothOfMatcher<Matcher7, Matcher8> > > > > > >
AllOf(Matcher1 m1, Matcher2 m2, Matcher3 m3, Matcher4 m4, Matcher5 m5,
    Matcher6 m6, Matcher7 m7, Matcher8 m8) {
  return ::testing::AllOf(m1, ::testing::AllOf(m2, m3, m4, m5, m6, m7, m8));
}

template <typename Matcher1, typename Matcher2, typename Matcher3,
    typename Matcher4, typename Matcher5, typename Matcher6, typename Matcher7,
    typename Matcher8, typename Matcher9>
inline internal::BothOfMatcher<Matcher1, internal::BothOfMatcher<Matcher2,
    internal::BothOfMatcher<Matcher3, internal::BothOfMatcher<Matcher4,
    internal::BothOfMatcher<Matcher5, internal::BothOfMatcher<Matcher6,
    internal::BothOfMatcher<Matcher7, internal::BothOfMatcher<Matcher8,
    Matcher9> > > > > > > >
AllOf(Matcher1 m1, Matcher2 m2, Matcher3 m3, Matcher4 m4, Matcher5 m5,
    Matcher6 m6, Matcher7 m7, Matcher8 m8, Matcher9 m9) {
  return ::testing::AllOf(m1, ::testing::AllOf(m2, m3, m4, m5, m6, m7, m8, m9));
}

template <typename Matcher1, typename Matcher2, typename Matcher3,
    typename Matcher4, typename Matcher5, typename Matcher6, typename Matcher7,
    typename Matcher8, typename Matcher9, typename Matcher10>
inline internal::BothOfMatcher<Matcher1, internal::BothOfMatcher<Matcher2,
    internal::BothOfMatcher<Matcher3, internal::BothOfMatcher<Matcher4,
    internal::BothOfMatcher<Matcher5, internal::BothOfMatcher<Matcher6,
    internal::BothOfMatcher<Matcher7, internal::BothOfMatcher<Matcher8,
    internal::BothOfMatcher<Matcher9, Matcher10> > > > > > > > >
AllOf(Matcher1 m1, Matcher2 m2, Matcher3 m3, Matcher4 m4, Matcher5 m5,
    Matcher6 m6, Matcher7 m7, Matcher8 m8, Matcher9 m9, Matcher10 m10) {
  return ::testing::AllOf(m1, ::testing::AllOf(m2, m3, m4, m5, m6, m7, m8, m9,
      m10));
}




template <typename Matcher1, typename Matcher2>
inline internal::EitherOfMatcher<Matcher1, Matcher2>
AnyOf(Matcher1 m1, Matcher2 m2) {
  return internal::EitherOfMatcher<Matcher1, Matcher2>(m1, m2);
}

template <typename Matcher1, typename Matcher2, typename Matcher3>
inline internal::EitherOfMatcher<Matcher1, internal::EitherOfMatcher<Matcher2,
    Matcher3> >
AnyOf(Matcher1 m1, Matcher2 m2, Matcher3 m3) {
  return ::testing::AnyOf(m1, ::testing::AnyOf(m2, m3));
}

template <typename Matcher1, typename Matcher2, typename Matcher3,
    typename Matcher4>
inline internal::EitherOfMatcher<Matcher1, internal::EitherOfMatcher<Matcher2,
    internal::EitherOfMatcher<Matcher3, Matcher4> > >
AnyOf(Matcher1 m1, Matcher2 m2, Matcher3 m3, Matcher4 m4) {
  return ::testing::AnyOf(m1, ::testing::AnyOf(m2, m3, m4));
}

template <typename Matcher1, typename Matcher2, typename Matcher3,
    typename Matcher4, typename Matcher5>
inline internal::EitherOfMatcher<Matcher1, internal::EitherOfMatcher<Matcher2,
    internal::EitherOfMatcher<Matcher3, internal::EitherOfMatcher<Matcher4,
    Matcher5> > > >
AnyOf(Matcher1 m1, Matcher2 m2, Matcher3 m3, Matcher4 m4, Matcher5 m5) {
  return ::testing::AnyOf(m1, ::testing::AnyOf(m2, m3, m4, m5));
}

template <typename Matcher1, typename Matcher2, typename Matcher3,
    typename Matcher4, typename Matcher5, typename Matcher6>
inline internal::EitherOfMatcher<Matcher1, internal::EitherOfMatcher<Matcher2,
    internal::EitherOfMatcher<Matcher3, internal::EitherOfMatcher<Matcher4,
    internal::EitherOfMatcher<Matcher5, Matcher6> > > > >
AnyOf(Matcher1 m1, Matcher2 m2, Matcher3 m3, Matcher4 m4, Matcher5 m5,
    Matcher6 m6) {
  return ::testing::AnyOf(m1, ::testing::AnyOf(m2, m3, m4, m5, m6));
}

template <typename Matcher1, typename Matcher2, typename Matcher3,
    typename Matcher4, typename Matcher5, typename Matcher6, typename Matcher7>
inline internal::EitherOfMatcher<Matcher1, internal::EitherOfMatcher<Matcher2,
    internal::EitherOfMatcher<Matcher3, internal::EitherOfMatcher<Matcher4,
    internal::EitherOfMatcher<Matcher5, internal::EitherOfMatcher<Matcher6,
    Matcher7> > > > > >
AnyOf(Matcher1 m1, Matcher2 m2, Matcher3 m3, Matcher4 m4, Matcher5 m5,
    Matcher6 m6, Matcher7 m7) {
  return ::testing::AnyOf(m1, ::testing::AnyOf(m2, m3, m4, m5, m6, m7));
}

template <typename Matcher1, typename Matcher2, typename Matcher3,
    typename Matcher4, typename Matcher5, typename Matcher6, typename Matcher7,
    typename Matcher8>
inline internal::EitherOfMatcher<Matcher1, internal::EitherOfMatcher<Matcher2,
    internal::EitherOfMatcher<Matcher3, internal::EitherOfMatcher<Matcher4,
    internal::EitherOfMatcher<Matcher5, internal::EitherOfMatcher<Matcher6,
    internal::EitherOfMatcher<Matcher7, Matcher8> > > > > > >
AnyOf(Matcher1 m1, Matcher2 m2, Matcher3 m3, Matcher4 m4, Matcher5 m5,
    Matcher6 m6, Matcher7 m7, Matcher8 m8) {
  return ::testing::AnyOf(m1, ::testing::AnyOf(m2, m3, m4, m5, m6, m7, m8));
}

template <typename Matcher1, typename Matcher2, typename Matcher3,
    typename Matcher4, typename Matcher5, typename Matcher6, typename Matcher7,
    typename Matcher8, typename Matcher9>
inline internal::EitherOfMatcher<Matcher1, internal::EitherOfMatcher<Matcher2,
    internal::EitherOfMatcher<Matcher3, internal::EitherOfMatcher<Matcher4,
    internal::EitherOfMatcher<Matcher5, internal::EitherOfMatcher<Matcher6,
    internal::EitherOfMatcher<Matcher7, internal::EitherOfMatcher<Matcher8,
    Matcher9> > > > > > > >
AnyOf(Matcher1 m1, Matcher2 m2, Matcher3 m3, Matcher4 m4, Matcher5 m5,
    Matcher6 m6, Matcher7 m7, Matcher8 m8, Matcher9 m9) {
  return ::testing::AnyOf(m1, ::testing::AnyOf(m2, m3, m4, m5, m6, m7, m8, m9));
}

template <typename Matcher1, typename Matcher2, typename Matcher3,
    typename Matcher4, typename Matcher5, typename Matcher6, typename Matcher7,
    typename Matcher8, typename Matcher9, typename Matcher10>
inline internal::EitherOfMatcher<Matcher1, internal::EitherOfMatcher<Matcher2,
    internal::EitherOfMatcher<Matcher3, internal::EitherOfMatcher<Matcher4,
    internal::EitherOfMatcher<Matcher5, internal::EitherOfMatcher<Matcher6,
    internal::EitherOfMatcher<Matcher7, internal::EitherOfMatcher<Matcher8,
    internal::EitherOfMatcher<Matcher9, Matcher10> > > > > > > > >
AnyOf(Matcher1 m1, Matcher2 m2, Matcher3 m3, Matcher4 m4, Matcher5 m5,
    Matcher6 m6, Matcher7 m7, Matcher8 m8, Matcher9 m9, Matcher10 m10) {
  return ::testing::AnyOf(m1, ::testing::AnyOf(m2, m3, m4, m5, m6, m7, m8, m9,
      m10));
}

}  

























































































































































































































#define MATCHER(name, description)\
  class name##Matcher {\
   public:\
    template <typename arg_type>\
    class gmock_Impl : public ::testing::MatcherInterface<arg_type> {\
     public:\
      gmock_Impl()\
           {}\
      virtual bool MatchAndExplain(\
          arg_type arg, ::testing::MatchResultListener* result_listener) const;\
      virtual void DescribeTo(::std::ostream* gmock_os) const {\
        *gmock_os << FormatDescription(false);\
      }\
      virtual void DescribeNegationTo(::std::ostream* gmock_os) const {\
        *gmock_os << FormatDescription(true);\
      }\
     private:\
      ::testing::internal::string FormatDescription(bool negation) const {\
        const ::testing::internal::string gmock_description = (description);\
        if (!gmock_description.empty())\
          return gmock_description;\
        return ::testing::internal::FormatMatcherDescription(\
            negation, #name,\
            ::testing::internal::UniversalTersePrintTupleFieldsToStrings(\
                ::std::tr1::tuple<>()));\
      }\
      GTEST_DISALLOW_ASSIGN_(gmock_Impl);\
    };\
    template <typename arg_type>\
    operator ::testing::Matcher<arg_type>() const {\
      return ::testing::Matcher<arg_type>(\
          new gmock_Impl<arg_type>());\
    }\
    name##Matcher() {\
    }\
   private:\
    GTEST_DISALLOW_ASSIGN_(name##Matcher);\
  };\
  inline name##Matcher name() {\
    return name##Matcher();\
  }\
  template <typename arg_type>\
  bool name##Matcher::gmock_Impl<arg_type>::MatchAndExplain(\
      arg_type arg,\
      ::testing::MatchResultListener* result_listener GTEST_ATTRIBUTE_UNUSED_)\
          const

#define MATCHER_P(name, p0, description)\
  template <typename p0##_type>\
  class name##MatcherP {\
   public:\
    template <typename arg_type>\
    class gmock_Impl : public ::testing::MatcherInterface<arg_type> {\
     public:\
      explicit gmock_Impl(p0##_type gmock_p0)\
           : p0(gmock_p0) {}\
      virtual bool MatchAndExplain(\
          arg_type arg, ::testing::MatchResultListener* result_listener) const;\
      virtual void DescribeTo(::std::ostream* gmock_os) const {\
        *gmock_os << FormatDescription(false);\
      }\
      virtual void DescribeNegationTo(::std::ostream* gmock_os) const {\
        *gmock_os << FormatDescription(true);\
      }\
      p0##_type p0;\
     private:\
      ::testing::internal::string FormatDescription(bool negation) const {\
        const ::testing::internal::string gmock_description = (description);\
        if (!gmock_description.empty())\
          return gmock_description;\
        return ::testing::internal::FormatMatcherDescription(\
            negation, #name,\
            ::testing::internal::UniversalTersePrintTupleFieldsToStrings(\
                ::std::tr1::tuple<p0##_type>(p0)));\
      }\
      GTEST_DISALLOW_ASSIGN_(gmock_Impl);\
    };\
    template <typename arg_type>\
    operator ::testing::Matcher<arg_type>() const {\
      return ::testing::Matcher<arg_type>(\
          new gmock_Impl<arg_type>(p0));\
    }\
    name##MatcherP(p0##_type gmock_p0) : p0(gmock_p0) {\
    }\
    p0##_type p0;\
   private:\
    GTEST_DISALLOW_ASSIGN_(name##MatcherP);\
  };\
  template <typename p0##_type>\
  inline name##MatcherP<p0##_type> name(p0##_type p0) {\
    return name##MatcherP<p0##_type>(p0);\
  }\
  template <typename p0##_type>\
  template <typename arg_type>\
  bool name##MatcherP<p0##_type>::gmock_Impl<arg_type>::MatchAndExplain(\
      arg_type arg,\
      ::testing::MatchResultListener* result_listener GTEST_ATTRIBUTE_UNUSED_)\
          const

#define MATCHER_P2(name, p0, p1, description)\
  template <typename p0##_type, typename p1##_type>\
  class name##MatcherP2 {\
   public:\
    template <typename arg_type>\
    class gmock_Impl : public ::testing::MatcherInterface<arg_type> {\
     public:\
      gmock_Impl(p0##_type gmock_p0, p1##_type gmock_p1)\
           : p0(gmock_p0), p1(gmock_p1) {}\
      virtual bool MatchAndExplain(\
          arg_type arg, ::testing::MatchResultListener* result_listener) const;\
      virtual void DescribeTo(::std::ostream* gmock_os) const {\
        *gmock_os << FormatDescription(false);\
      }\
      virtual void DescribeNegationTo(::std::ostream* gmock_os) const {\
        *gmock_os << FormatDescription(true);\
      }\
      p0##_type p0;\
      p1##_type p1;\
     private:\
      ::testing::internal::string FormatDescription(bool negation) const {\
        const ::testing::internal::string gmock_description = (description);\
        if (!gmock_description.empty())\
          return gmock_description;\
        return ::testing::internal::FormatMatcherDescription(\
            negation, #name,\
            ::testing::internal::UniversalTersePrintTupleFieldsToStrings(\
                ::std::tr1::tuple<p0##_type, p1##_type>(p0, p1)));\
      }\
      GTEST_DISALLOW_ASSIGN_(gmock_Impl);\
    };\
    template <typename arg_type>\
    operator ::testing::Matcher<arg_type>() const {\
      return ::testing::Matcher<arg_type>(\
          new gmock_Impl<arg_type>(p0, p1));\
    }\
    name##MatcherP2(p0##_type gmock_p0, p1##_type gmock_p1) : p0(gmock_p0), \
        p1(gmock_p1) {\
    }\
    p0##_type p0;\
    p1##_type p1;\
   private:\
    GTEST_DISALLOW_ASSIGN_(name##MatcherP2);\
  };\
  template <typename p0##_type, typename p1##_type>\
  inline name##MatcherP2<p0##_type, p1##_type> name(p0##_type p0, \
      p1##_type p1) {\
    return name##MatcherP2<p0##_type, p1##_type>(p0, p1);\
  }\
  template <typename p0##_type, typename p1##_type>\
  template <typename arg_type>\
  bool name##MatcherP2<p0##_type, \
      p1##_type>::gmock_Impl<arg_type>::MatchAndExplain(\
      arg_type arg,\
      ::testing::MatchResultListener* result_listener GTEST_ATTRIBUTE_UNUSED_)\
          const

#define MATCHER_P3(name, p0, p1, p2, description)\
  template <typename p0##_type, typename p1##_type, typename p2##_type>\
  class name##MatcherP3 {\
   public:\
    template <typename arg_type>\
    class gmock_Impl : public ::testing::MatcherInterface<arg_type> {\
     public:\
      gmock_Impl(p0##_type gmock_p0, p1##_type gmock_p1, p2##_type gmock_p2)\
           : p0(gmock_p0), p1(gmock_p1), p2(gmock_p2) {}\
      virtual bool MatchAndExplain(\
          arg_type arg, ::testing::MatchResultListener* result_listener) const;\
      virtual void DescribeTo(::std::ostream* gmock_os) const {\
        *gmock_os << FormatDescription(false);\
      }\
      virtual void DescribeNegationTo(::std::ostream* gmock_os) const {\
        *gmock_os << FormatDescription(true);\
      }\
      p0##_type p0;\
      p1##_type p1;\
      p2##_type p2;\
     private:\
      ::testing::internal::string FormatDescription(bool negation) const {\
        const ::testing::internal::string gmock_description = (description);\
        if (!gmock_description.empty())\
          return gmock_description;\
        return ::testing::internal::FormatMatcherDescription(\
            negation, #name,\
            ::testing::internal::UniversalTersePrintTupleFieldsToStrings(\
                ::std::tr1::tuple<p0##_type, p1##_type, p2##_type>(p0, p1, \
                    p2)));\
      }\
      GTEST_DISALLOW_ASSIGN_(gmock_Impl);\
    };\
    template <typename arg_type>\
    operator ::testing::Matcher<arg_type>() const {\
      return ::testing::Matcher<arg_type>(\
          new gmock_Impl<arg_type>(p0, p1, p2));\
    }\
    name##MatcherP3(p0##_type gmock_p0, p1##_type gmock_p1, \
        p2##_type gmock_p2) : p0(gmock_p0), p1(gmock_p1), p2(gmock_p2) {\
    }\
    p0##_type p0;\
    p1##_type p1;\
    p2##_type p2;\
   private:\
    GTEST_DISALLOW_ASSIGN_(name##MatcherP3);\
  };\
  template <typename p0##_type, typename p1##_type, typename p2##_type>\
  inline name##MatcherP3<p0##_type, p1##_type, p2##_type> name(p0##_type p0, \
      p1##_type p1, p2##_type p2) {\
    return name##MatcherP3<p0##_type, p1##_type, p2##_type>(p0, p1, p2);\
  }\
  template <typename p0##_type, typename p1##_type, typename p2##_type>\
  template <typename arg_type>\
  bool name##MatcherP3<p0##_type, p1##_type, \
      p2##_type>::gmock_Impl<arg_type>::MatchAndExplain(\
      arg_type arg,\
      ::testing::MatchResultListener* result_listener GTEST_ATTRIBUTE_UNUSED_)\
          const

#define MATCHER_P4(name, p0, p1, p2, p3, description)\
  template <typename p0##_type, typename p1##_type, typename p2##_type, \
      typename p3##_type>\
  class name##MatcherP4 {\
   public:\
    template <typename arg_type>\
    class gmock_Impl : public ::testing::MatcherInterface<arg_type> {\
     public:\
      gmock_Impl(p0##_type gmock_p0, p1##_type gmock_p1, p2##_type gmock_p2, \
          p3##_type gmock_p3)\
           : p0(gmock_p0), p1(gmock_p1), p2(gmock_p2), p3(gmock_p3) {}\
      virtual bool MatchAndExplain(\
          arg_type arg, ::testing::MatchResultListener* result_listener) const;\
      virtual void DescribeTo(::std::ostream* gmock_os) const {\
        *gmock_os << FormatDescription(false);\
      }\
      virtual void DescribeNegationTo(::std::ostream* gmock_os) const {\
        *gmock_os << FormatDescription(true);\
      }\
      p0##_type p0;\
      p1##_type p1;\
      p2##_type p2;\
      p3##_type p3;\
     private:\
      ::testing::internal::string FormatDescription(bool negation) const {\
        const ::testing::internal::string gmock_description = (description);\
        if (!gmock_description.empty())\
          return gmock_description;\
        return ::testing::internal::FormatMatcherDescription(\
            negation, #name,\
            ::testing::internal::UniversalTersePrintTupleFieldsToStrings(\
                ::std::tr1::tuple<p0##_type, p1##_type, p2##_type, \
                    p3##_type>(p0, p1, p2, p3)));\
      }\
      GTEST_DISALLOW_ASSIGN_(gmock_Impl);\
    };\
    template <typename arg_type>\
    operator ::testing::Matcher<arg_type>() const {\
      return ::testing::Matcher<arg_type>(\
          new gmock_Impl<arg_type>(p0, p1, p2, p3));\
    }\
    name##MatcherP4(p0##_type gmock_p0, p1##_type gmock_p1, \
        p2##_type gmock_p2, p3##_type gmock_p3) : p0(gmock_p0), p1(gmock_p1), \
        p2(gmock_p2), p3(gmock_p3) {\
    }\
    p0##_type p0;\
    p1##_type p1;\
    p2##_type p2;\
    p3##_type p3;\
   private:\
    GTEST_DISALLOW_ASSIGN_(name##MatcherP4);\
  };\
  template <typename p0##_type, typename p1##_type, typename p2##_type, \
      typename p3##_type>\
  inline name##MatcherP4<p0##_type, p1##_type, p2##_type, \
      p3##_type> name(p0##_type p0, p1##_type p1, p2##_type p2, \
      p3##_type p3) {\
    return name##MatcherP4<p0##_type, p1##_type, p2##_type, p3##_type>(p0, \
        p1, p2, p3);\
  }\
  template <typename p0##_type, typename p1##_type, typename p2##_type, \
      typename p3##_type>\
  template <typename arg_type>\
  bool name##MatcherP4<p0##_type, p1##_type, p2##_type, \
      p3##_type>::gmock_Impl<arg_type>::MatchAndExplain(\
      arg_type arg,\
      ::testing::MatchResultListener* result_listener GTEST_ATTRIBUTE_UNUSED_)\
          const

#define MATCHER_P5(name, p0, p1, p2, p3, p4, description)\
  template <typename p0##_type, typename p1##_type, typename p2##_type, \
      typename p3##_type, typename p4##_type>\
  class name##MatcherP5 {\
   public:\
    template <typename arg_type>\
    class gmock_Impl : public ::testing::MatcherInterface<arg_type> {\
     public:\
      gmock_Impl(p0##_type gmock_p0, p1##_type gmock_p1, p2##_type gmock_p2, \
          p3##_type gmock_p3, p4##_type gmock_p4)\
           : p0(gmock_p0), p1(gmock_p1), p2(gmock_p2), p3(gmock_p3), \
               p4(gmock_p4) {}\
      virtual bool MatchAndExplain(\
          arg_type arg, ::testing::MatchResultListener* result_listener) const;\
      virtual void DescribeTo(::std::ostream* gmock_os) const {\
        *gmock_os << FormatDescription(false);\
      }\
      virtual void DescribeNegationTo(::std::ostream* gmock_os) const {\
        *gmock_os << FormatDescription(true);\
      }\
      p0##_type p0;\
      p1##_type p1;\
      p2##_type p2;\
      p3##_type p3;\
      p4##_type p4;\
     private:\
      ::testing::internal::string FormatDescription(bool negation) const {\
        const ::testing::internal::string gmock_description = (description);\
        if (!gmock_description.empty())\
          return gmock_description;\
        return ::testing::internal::FormatMatcherDescription(\
            negation, #name,\
            ::testing::internal::UniversalTersePrintTupleFieldsToStrings(\
                ::std::tr1::tuple<p0##_type, p1##_type, p2##_type, p3##_type, \
                    p4##_type>(p0, p1, p2, p3, p4)));\
      }\
      GTEST_DISALLOW_ASSIGN_(gmock_Impl);\
    };\
    template <typename arg_type>\
    operator ::testing::Matcher<arg_type>() const {\
      return ::testing::Matcher<arg_type>(\
          new gmock_Impl<arg_type>(p0, p1, p2, p3, p4));\
    }\
    name##MatcherP5(p0##_type gmock_p0, p1##_type gmock_p1, \
        p2##_type gmock_p2, p3##_type gmock_p3, \
        p4##_type gmock_p4) : p0(gmock_p0), p1(gmock_p1), p2(gmock_p2), \
        p3(gmock_p3), p4(gmock_p4) {\
    }\
    p0##_type p0;\
    p1##_type p1;\
    p2##_type p2;\
    p3##_type p3;\
    p4##_type p4;\
   private:\
    GTEST_DISALLOW_ASSIGN_(name##MatcherP5);\
  };\
  template <typename p0##_type, typename p1##_type, typename p2##_type, \
      typename p3##_type, typename p4##_type>\
  inline name##MatcherP5<p0##_type, p1##_type, p2##_type, p3##_type, \
      p4##_type> name(p0##_type p0, p1##_type p1, p2##_type p2, p3##_type p3, \
      p4##_type p4) {\
    return name##MatcherP5<p0##_type, p1##_type, p2##_type, p3##_type, \
        p4##_type>(p0, p1, p2, p3, p4);\
  }\
  template <typename p0##_type, typename p1##_type, typename p2##_type, \
      typename p3##_type, typename p4##_type>\
  template <typename arg_type>\
  bool name##MatcherP5<p0##_type, p1##_type, p2##_type, p3##_type, \
      p4##_type>::gmock_Impl<arg_type>::MatchAndExplain(\
      arg_type arg,\
      ::testing::MatchResultListener* result_listener GTEST_ATTRIBUTE_UNUSED_)\
          const

#define MATCHER_P6(name, p0, p1, p2, p3, p4, p5, description)\
  template <typename p0##_type, typename p1##_type, typename p2##_type, \
      typename p3##_type, typename p4##_type, typename p5##_type>\
  class name##MatcherP6 {\
   public:\
    template <typename arg_type>\
    class gmock_Impl : public ::testing::MatcherInterface<arg_type> {\
     public:\
      gmock_Impl(p0##_type gmock_p0, p1##_type gmock_p1, p2##_type gmock_p2, \
          p3##_type gmock_p3, p4##_type gmock_p4, p5##_type gmock_p5)\
           : p0(gmock_p0), p1(gmock_p1), p2(gmock_p2), p3(gmock_p3), \
               p4(gmock_p4), p5(gmock_p5) {}\
      virtual bool MatchAndExplain(\
          arg_type arg, ::testing::MatchResultListener* result_listener) const;\
      virtual void DescribeTo(::std::ostream* gmock_os) const {\
        *gmock_os << FormatDescription(false);\
      }\
      virtual void DescribeNegationTo(::std::ostream* gmock_os) const {\
        *gmock_os << FormatDescription(true);\
      }\
      p0##_type p0;\
      p1##_type p1;\
      p2##_type p2;\
      p3##_type p3;\
      p4##_type p4;\
      p5##_type p5;\
     private:\
      ::testing::internal::string FormatDescription(bool negation) const {\
        const ::testing::internal::string gmock_description = (description);\
        if (!gmock_description.empty())\
          return gmock_description;\
        return ::testing::internal::FormatMatcherDescription(\
            negation, #name,\
            ::testing::internal::UniversalTersePrintTupleFieldsToStrings(\
                ::std::tr1::tuple<p0##_type, p1##_type, p2##_type, p3##_type, \
                    p4##_type, p5##_type>(p0, p1, p2, p3, p4, p5)));\
      }\
      GTEST_DISALLOW_ASSIGN_(gmock_Impl);\
    };\
    template <typename arg_type>\
    operator ::testing::Matcher<arg_type>() const {\
      return ::testing::Matcher<arg_type>(\
          new gmock_Impl<arg_type>(p0, p1, p2, p3, p4, p5));\
    }\
    name##MatcherP6(p0##_type gmock_p0, p1##_type gmock_p1, \
        p2##_type gmock_p2, p3##_type gmock_p3, p4##_type gmock_p4, \
        p5##_type gmock_p5) : p0(gmock_p0), p1(gmock_p1), p2(gmock_p2), \
        p3(gmock_p3), p4(gmock_p4), p5(gmock_p5) {\
    }\
    p0##_type p0;\
    p1##_type p1;\
    p2##_type p2;\
    p3##_type p3;\
    p4##_type p4;\
    p5##_type p5;\
   private:\
    GTEST_DISALLOW_ASSIGN_(name##MatcherP6);\
  };\
  template <typename p0##_type, typename p1##_type, typename p2##_type, \
      typename p3##_type, typename p4##_type, typename p5##_type>\
  inline name##MatcherP6<p0##_type, p1##_type, p2##_type, p3##_type, \
      p4##_type, p5##_type> name(p0##_type p0, p1##_type p1, p2##_type p2, \
      p3##_type p3, p4##_type p4, p5##_type p5) {\
    return name##MatcherP6<p0##_type, p1##_type, p2##_type, p3##_type, \
        p4##_type, p5##_type>(p0, p1, p2, p3, p4, p5);\
  }\
  template <typename p0##_type, typename p1##_type, typename p2##_type, \
      typename p3##_type, typename p4##_type, typename p5##_type>\
  template <typename arg_type>\
  bool name##MatcherP6<p0##_type, p1##_type, p2##_type, p3##_type, p4##_type, \
      p5##_type>::gmock_Impl<arg_type>::MatchAndExplain(\
      arg_type arg,\
      ::testing::MatchResultListener* result_listener GTEST_ATTRIBUTE_UNUSED_)\
          const

#define MATCHER_P7(name, p0, p1, p2, p3, p4, p5, p6, description)\
  template <typename p0##_type, typename p1##_type, typename p2##_type, \
      typename p3##_type, typename p4##_type, typename p5##_type, \
      typename p6##_type>\
  class name##MatcherP7 {\
   public:\
    template <typename arg_type>\
    class gmock_Impl : public ::testing::MatcherInterface<arg_type> {\
     public:\
      gmock_Impl(p0##_type gmock_p0, p1##_type gmock_p1, p2##_type gmock_p2, \
          p3##_type gmock_p3, p4##_type gmock_p4, p5##_type gmock_p5, \
          p6##_type gmock_p6)\
           : p0(gmock_p0), p1(gmock_p1), p2(gmock_p2), p3(gmock_p3), \
               p4(gmock_p4), p5(gmock_p5), p6(gmock_p6) {}\
      virtual bool MatchAndExplain(\
          arg_type arg, ::testing::MatchResultListener* result_listener) const;\
      virtual void DescribeTo(::std::ostream* gmock_os) const {\
        *gmock_os << FormatDescription(false);\
      }\
      virtual void DescribeNegationTo(::std::ostream* gmock_os) const {\
        *gmock_os << FormatDescription(true);\
      }\
      p0##_type p0;\
      p1##_type p1;\
      p2##_type p2;\
      p3##_type p3;\
      p4##_type p4;\
      p5##_type p5;\
      p6##_type p6;\
     private:\
      ::testing::internal::string FormatDescription(bool negation) const {\
        const ::testing::internal::string gmock_description = (description);\
        if (!gmock_description.empty())\
          return gmock_description;\
        return ::testing::internal::FormatMatcherDescription(\
            negation, #name,\
            ::testing::internal::UniversalTersePrintTupleFieldsToStrings(\
                ::std::tr1::tuple<p0##_type, p1##_type, p2##_type, p3##_type, \
                    p4##_type, p5##_type, p6##_type>(p0, p1, p2, p3, p4, p5, \
                    p6)));\
      }\
      GTEST_DISALLOW_ASSIGN_(gmock_Impl);\
    };\
    template <typename arg_type>\
    operator ::testing::Matcher<arg_type>() const {\
      return ::testing::Matcher<arg_type>(\
          new gmock_Impl<arg_type>(p0, p1, p2, p3, p4, p5, p6));\
    }\
    name##MatcherP7(p0##_type gmock_p0, p1##_type gmock_p1, \
        p2##_type gmock_p2, p3##_type gmock_p3, p4##_type gmock_p4, \
        p5##_type gmock_p5, p6##_type gmock_p6) : p0(gmock_p0), p1(gmock_p1), \
        p2(gmock_p2), p3(gmock_p3), p4(gmock_p4), p5(gmock_p5), \
        p6(gmock_p6) {\
    }\
    p0##_type p0;\
    p1##_type p1;\
    p2##_type p2;\
    p3##_type p3;\
    p4##_type p4;\
    p5##_type p5;\
    p6##_type p6;\
   private:\
    GTEST_DISALLOW_ASSIGN_(name##MatcherP7);\
  };\
  template <typename p0##_type, typename p1##_type, typename p2##_type, \
      typename p3##_type, typename p4##_type, typename p5##_type, \
      typename p6##_type>\
  inline name##MatcherP7<p0##_type, p1##_type, p2##_type, p3##_type, \
      p4##_type, p5##_type, p6##_type> name(p0##_type p0, p1##_type p1, \
      p2##_type p2, p3##_type p3, p4##_type p4, p5##_type p5, \
      p6##_type p6) {\
    return name##MatcherP7<p0##_type, p1##_type, p2##_type, p3##_type, \
        p4##_type, p5##_type, p6##_type>(p0, p1, p2, p3, p4, p5, p6);\
  }\
  template <typename p0##_type, typename p1##_type, typename p2##_type, \
      typename p3##_type, typename p4##_type, typename p5##_type, \
      typename p6##_type>\
  template <typename arg_type>\
  bool name##MatcherP7<p0##_type, p1##_type, p2##_type, p3##_type, p4##_type, \
      p5##_type, p6##_type>::gmock_Impl<arg_type>::MatchAndExplain(\
      arg_type arg,\
      ::testing::MatchResultListener* result_listener GTEST_ATTRIBUTE_UNUSED_)\
          const

#define MATCHER_P8(name, p0, p1, p2, p3, p4, p5, p6, p7, description)\
  template <typename p0##_type, typename p1##_type, typename p2##_type, \
      typename p3##_type, typename p4##_type, typename p5##_type, \
      typename p6##_type, typename p7##_type>\
  class name##MatcherP8 {\
   public:\
    template <typename arg_type>\
    class gmock_Impl : public ::testing::MatcherInterface<arg_type> {\
     public:\
      gmock_Impl(p0##_type gmock_p0, p1##_type gmock_p1, p2##_type gmock_p2, \
          p3##_type gmock_p3, p4##_type gmock_p4, p5##_type gmock_p5, \
          p6##_type gmock_p6, p7##_type gmock_p7)\
           : p0(gmock_p0), p1(gmock_p1), p2(gmock_p2), p3(gmock_p3), \
               p4(gmock_p4), p5(gmock_p5), p6(gmock_p6), p7(gmock_p7) {}\
      virtual bool MatchAndExplain(\
          arg_type arg, ::testing::MatchResultListener* result_listener) const;\
      virtual void DescribeTo(::std::ostream* gmock_os) const {\
        *gmock_os << FormatDescription(false);\
      }\
      virtual void DescribeNegationTo(::std::ostream* gmock_os) const {\
        *gmock_os << FormatDescription(true);\
      }\
      p0##_type p0;\
      p1##_type p1;\
      p2##_type p2;\
      p3##_type p3;\
      p4##_type p4;\
      p5##_type p5;\
      p6##_type p6;\
      p7##_type p7;\
     private:\
      ::testing::internal::string FormatDescription(bool negation) const {\
        const ::testing::internal::string gmock_description = (description);\
        if (!gmock_description.empty())\
          return gmock_description;\
        return ::testing::internal::FormatMatcherDescription(\
            negation, #name,\
            ::testing::internal::UniversalTersePrintTupleFieldsToStrings(\
                ::std::tr1::tuple<p0##_type, p1##_type, p2##_type, p3##_type, \
                    p4##_type, p5##_type, p6##_type, p7##_type>(p0, p1, p2, \
                    p3, p4, p5, p6, p7)));\
      }\
      GTEST_DISALLOW_ASSIGN_(gmock_Impl);\
    };\
    template <typename arg_type>\
    operator ::testing::Matcher<arg_type>() const {\
      return ::testing::Matcher<arg_type>(\
          new gmock_Impl<arg_type>(p0, p1, p2, p3, p4, p5, p6, p7));\
    }\
    name##MatcherP8(p0##_type gmock_p0, p1##_type gmock_p1, \
        p2##_type gmock_p2, p3##_type gmock_p3, p4##_type gmock_p4, \
        p5##_type gmock_p5, p6##_type gmock_p6, \
        p7##_type gmock_p7) : p0(gmock_p0), p1(gmock_p1), p2(gmock_p2), \
        p3(gmock_p3), p4(gmock_p4), p5(gmock_p5), p6(gmock_p6), \
        p7(gmock_p7) {\
    }\
    p0##_type p0;\
    p1##_type p1;\
    p2##_type p2;\
    p3##_type p3;\
    p4##_type p4;\
    p5##_type p5;\
    p6##_type p6;\
    p7##_type p7;\
   private:\
    GTEST_DISALLOW_ASSIGN_(name##MatcherP8);\
  };\
  template <typename p0##_type, typename p1##_type, typename p2##_type, \
      typename p3##_type, typename p4##_type, typename p5##_type, \
      typename p6##_type, typename p7##_type>\
  inline name##MatcherP8<p0##_type, p1##_type, p2##_type, p3##_type, \
      p4##_type, p5##_type, p6##_type, p7##_type> name(p0##_type p0, \
      p1##_type p1, p2##_type p2, p3##_type p3, p4##_type p4, p5##_type p5, \
      p6##_type p6, p7##_type p7) {\
    return name##MatcherP8<p0##_type, p1##_type, p2##_type, p3##_type, \
        p4##_type, p5##_type, p6##_type, p7##_type>(p0, p1, p2, p3, p4, p5, \
        p6, p7);\
  }\
  template <typename p0##_type, typename p1##_type, typename p2##_type, \
      typename p3##_type, typename p4##_type, typename p5##_type, \
      typename p6##_type, typename p7##_type>\
  template <typename arg_type>\
  bool name##MatcherP8<p0##_type, p1##_type, p2##_type, p3##_type, p4##_type, \
      p5##_type, p6##_type, \
      p7##_type>::gmock_Impl<arg_type>::MatchAndExplain(\
      arg_type arg,\
      ::testing::MatchResultListener* result_listener GTEST_ATTRIBUTE_UNUSED_)\
          const

#define MATCHER_P9(name, p0, p1, p2, p3, p4, p5, p6, p7, p8, description)\
  template <typename p0##_type, typename p1##_type, typename p2##_type, \
      typename p3##_type, typename p4##_type, typename p5##_type, \
      typename p6##_type, typename p7##_type, typename p8##_type>\
  class name##MatcherP9 {\
   public:\
    template <typename arg_type>\
    class gmock_Impl : public ::testing::MatcherInterface<arg_type> {\
     public:\
      gmock_Impl(p0##_type gmock_p0, p1##_type gmock_p1, p2##_type gmock_p2, \
          p3##_type gmock_p3, p4##_type gmock_p4, p5##_type gmock_p5, \
          p6##_type gmock_p6, p7##_type gmock_p7, p8##_type gmock_p8)\
           : p0(gmock_p0), p1(gmock_p1), p2(gmock_p2), p3(gmock_p3), \
               p4(gmock_p4), p5(gmock_p5), p6(gmock_p6), p7(gmock_p7), \
               p8(gmock_p8) {}\
      virtual bool MatchAndExplain(\
          arg_type arg, ::testing::MatchResultListener* result_listener) const;\
      virtual void DescribeTo(::std::ostream* gmock_os) const {\
        *gmock_os << FormatDescription(false);\
      }\
      virtual void DescribeNegationTo(::std::ostream* gmock_os) const {\
        *gmock_os << FormatDescription(true);\
      }\
      p0##_type p0;\
      p1##_type p1;\
      p2##_type p2;\
      p3##_type p3;\
      p4##_type p4;\
      p5##_type p5;\
      p6##_type p6;\
      p7##_type p7;\
      p8##_type p8;\
     private:\
      ::testing::internal::string FormatDescription(bool negation) const {\
        const ::testing::internal::string gmock_description = (description);\
        if (!gmock_description.empty())\
          return gmock_description;\
        return ::testing::internal::FormatMatcherDescription(\
            negation, #name,\
            ::testing::internal::UniversalTersePrintTupleFieldsToStrings(\
                ::std::tr1::tuple<p0##_type, p1##_type, p2##_type, p3##_type, \
                    p4##_type, p5##_type, p6##_type, p7##_type, \
                    p8##_type>(p0, p1, p2, p3, p4, p5, p6, p7, p8)));\
      }\
      GTEST_DISALLOW_ASSIGN_(gmock_Impl);\
    };\
    template <typename arg_type>\
    operator ::testing::Matcher<arg_type>() const {\
      return ::testing::Matcher<arg_type>(\
          new gmock_Impl<arg_type>(p0, p1, p2, p3, p4, p5, p6, p7, p8));\
    }\
    name##MatcherP9(p0##_type gmock_p0, p1##_type gmock_p1, \
        p2##_type gmock_p2, p3##_type gmock_p3, p4##_type gmock_p4, \
        p5##_type gmock_p5, p6##_type gmock_p6, p7##_type gmock_p7, \
        p8##_type gmock_p8) : p0(gmock_p0), p1(gmock_p1), p2(gmock_p2), \
        p3(gmock_p3), p4(gmock_p4), p5(gmock_p5), p6(gmock_p6), p7(gmock_p7), \
        p8(gmock_p8) {\
    }\
    p0##_type p0;\
    p1##_type p1;\
    p2##_type p2;\
    p3##_type p3;\
    p4##_type p4;\
    p5##_type p5;\
    p6##_type p6;\
    p7##_type p7;\
    p8##_type p8;\
   private:\
    GTEST_DISALLOW_ASSIGN_(name##MatcherP9);\
  };\
  template <typename p0##_type, typename p1##_type, typename p2##_type, \
      typename p3##_type, typename p4##_type, typename p5##_type, \
      typename p6##_type, typename p7##_type, typename p8##_type>\
  inline name##MatcherP9<p0##_type, p1##_type, p2##_type, p3##_type, \
      p4##_type, p5##_type, p6##_type, p7##_type, \
      p8##_type> name(p0##_type p0, p1##_type p1, p2##_type p2, p3##_type p3, \
      p4##_type p4, p5##_type p5, p6##_type p6, p7##_type p7, \
      p8##_type p8) {\
    return name##MatcherP9<p0##_type, p1##_type, p2##_type, p3##_type, \
        p4##_type, p5##_type, p6##_type, p7##_type, p8##_type>(p0, p1, p2, \
        p3, p4, p5, p6, p7, p8);\
  }\
  template <typename p0##_type, typename p1##_type, typename p2##_type, \
      typename p3##_type, typename p4##_type, typename p5##_type, \
      typename p6##_type, typename p7##_type, typename p8##_type>\
  template <typename arg_type>\
  bool name##MatcherP9<p0##_type, p1##_type, p2##_type, p3##_type, p4##_type, \
      p5##_type, p6##_type, p7##_type, \
      p8##_type>::gmock_Impl<arg_type>::MatchAndExplain(\
      arg_type arg,\
      ::testing::MatchResultListener* result_listener GTEST_ATTRIBUTE_UNUSED_)\
          const

#define MATCHER_P10(name, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, description)\
  template <typename p0##_type, typename p1##_type, typename p2##_type, \
      typename p3##_type, typename p4##_type, typename p5##_type, \
      typename p6##_type, typename p7##_type, typename p8##_type, \
      typename p9##_type>\
  class name##MatcherP10 {\
   public:\
    template <typename arg_type>\
    class gmock_Impl : public ::testing::MatcherInterface<arg_type> {\
     public:\
      gmock_Impl(p0##_type gmock_p0, p1##_type gmock_p1, p2##_type gmock_p2, \
          p3##_type gmock_p3, p4##_type gmock_p4, p5##_type gmock_p5, \
          p6##_type gmock_p6, p7##_type gmock_p7, p8##_type gmock_p8, \
          p9##_type gmock_p9)\
           : p0(gmock_p0), p1(gmock_p1), p2(gmock_p2), p3(gmock_p3), \
               p4(gmock_p4), p5(gmock_p5), p6(gmock_p6), p7(gmock_p7), \
               p8(gmock_p8), p9(gmock_p9) {}\
      virtual bool MatchAndExplain(\
          arg_type arg, ::testing::MatchResultListener* result_listener) const;\
      virtual void DescribeTo(::std::ostream* gmock_os) const {\
        *gmock_os << FormatDescription(false);\
      }\
      virtual void DescribeNegationTo(::std::ostream* gmock_os) const {\
        *gmock_os << FormatDescription(true);\
      }\
      p0##_type p0;\
      p1##_type p1;\
      p2##_type p2;\
      p3##_type p3;\
      p4##_type p4;\
      p5##_type p5;\
      p6##_type p6;\
      p7##_type p7;\
      p8##_type p8;\
      p9##_type p9;\
     private:\
      ::testing::internal::string FormatDescription(bool negation) const {\
        const ::testing::internal::string gmock_description = (description);\
        if (!gmock_description.empty())\
          return gmock_description;\
        return ::testing::internal::FormatMatcherDescription(\
            negation, #name,\
            ::testing::internal::UniversalTersePrintTupleFieldsToStrings(\
                ::std::tr1::tuple<p0##_type, p1##_type, p2##_type, p3##_type, \
                    p4##_type, p5##_type, p6##_type, p7##_type, p8##_type, \
                    p9##_type>(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9)));\
      }\
      GTEST_DISALLOW_ASSIGN_(gmock_Impl);\
    };\
    template <typename arg_type>\
    operator ::testing::Matcher<arg_type>() const {\
      return ::testing::Matcher<arg_type>(\
          new gmock_Impl<arg_type>(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9));\
    }\
    name##MatcherP10(p0##_type gmock_p0, p1##_type gmock_p1, \
        p2##_type gmock_p2, p3##_type gmock_p3, p4##_type gmock_p4, \
        p5##_type gmock_p5, p6##_type gmock_p6, p7##_type gmock_p7, \
        p8##_type gmock_p8, p9##_type gmock_p9) : p0(gmock_p0), p1(gmock_p1), \
        p2(gmock_p2), p3(gmock_p3), p4(gmock_p4), p5(gmock_p5), p6(gmock_p6), \
        p7(gmock_p7), p8(gmock_p8), p9(gmock_p9) {\
    }\
    p0##_type p0;\
    p1##_type p1;\
    p2##_type p2;\
    p3##_type p3;\
    p4##_type p4;\
    p5##_type p5;\
    p6##_type p6;\
    p7##_type p7;\
    p8##_type p8;\
    p9##_type p9;\
   private:\
    GTEST_DISALLOW_ASSIGN_(name##MatcherP10);\
  };\
  template <typename p0##_type, typename p1##_type, typename p2##_type, \
      typename p3##_type, typename p4##_type, typename p5##_type, \
      typename p6##_type, typename p7##_type, typename p8##_type, \
      typename p9##_type>\
  inline name##MatcherP10<p0##_type, p1##_type, p2##_type, p3##_type, \
      p4##_type, p5##_type, p6##_type, p7##_type, p8##_type, \
      p9##_type> name(p0##_type p0, p1##_type p1, p2##_type p2, p3##_type p3, \
      p4##_type p4, p5##_type p5, p6##_type p6, p7##_type p7, p8##_type p8, \
      p9##_type p9) {\
    return name##MatcherP10<p0##_type, p1##_type, p2##_type, p3##_type, \
        p4##_type, p5##_type, p6##_type, p7##_type, p8##_type, p9##_type>(p0, \
        p1, p2, p3, p4, p5, p6, p7, p8, p9);\
  }\
  template <typename p0##_type, typename p1##_type, typename p2##_type, \
      typename p3##_type, typename p4##_type, typename p5##_type, \
      typename p6##_type, typename p7##_type, typename p8##_type, \
      typename p9##_type>\
  template <typename arg_type>\
  bool name##MatcherP10<p0##_type, p1##_type, p2##_type, p3##_type, \
      p4##_type, p5##_type, p6##_type, p7##_type, p8##_type, \
      p9##_type>::gmock_Impl<arg_type>::MatchAndExplain(\
      arg_type arg,\
      ::testing::MatchResultListener* result_listener GTEST_ATTRIBUTE_UNUSED_)\
          const

#endif  
