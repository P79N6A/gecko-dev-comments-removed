




































#ifndef GMOCK_INCLUDE_GMOCK_GMOCK_MATCHERS_H_
#define GMOCK_INCLUDE_GMOCK_GMOCK_MATCHERS_H_

#include <algorithm>
#include <limits>
#include <ostream>  
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "gmock/internal/gmock-internal-utils.h"
#include "gmock/internal/gmock-port.h"
#include "gtest/gtest.h"

namespace testing {




















class MatchResultListener {
 public:
  
  
  explicit MatchResultListener(::std::ostream* os) : stream_(os) {}
  virtual ~MatchResultListener() = 0;  

  
  
  template <typename T>
  MatchResultListener& operator<<(const T& x) {
    if (stream_ != NULL)
      *stream_ << x;
    return *this;
  }

  
  ::std::ostream* stream() { return stream_; }

  
  
  
  
  bool IsInterested() const { return stream_ != NULL; }

 private:
  ::std::ostream* const stream_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(MatchResultListener);
};

inline MatchResultListener::~MatchResultListener() {
}


template <typename T>
class MatcherInterface {
 public:
  virtual ~MatcherInterface() {}

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual bool MatchAndExplain(T x, MatchResultListener* listener) const = 0;

  
  
  
  
  
  virtual void DescribeTo(::std::ostream* os) const = 0;

  
  
  
  
  
  
  virtual void DescribeNegationTo(::std::ostream* os) const {
    *os << "not (";
    DescribeTo(os);
    *os << ")";
  }
};

namespace internal {


class DummyMatchResultListener : public MatchResultListener {
 public:
  DummyMatchResultListener() : MatchResultListener(NULL) {}

 private:
  GTEST_DISALLOW_COPY_AND_ASSIGN_(DummyMatchResultListener);
};




class StreamMatchResultListener : public MatchResultListener {
 public:
  explicit StreamMatchResultListener(::std::ostream* os)
      : MatchResultListener(os) {}

 private:
  GTEST_DISALLOW_COPY_AND_ASSIGN_(StreamMatchResultListener);
};


class StringMatchResultListener : public MatchResultListener {
 public:
  StringMatchResultListener() : MatchResultListener(&ss_) {}

  
  internal::string str() const { return ss_.str(); }

 private:
  ::std::stringstream ss_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(StringMatchResultListener);
};




template <typename T>
class MatcherBase {
 public:
  
  
  bool MatchAndExplain(T x, MatchResultListener* listener) const {
    return impl_->MatchAndExplain(x, listener);
  }

  
  bool Matches(T x) const {
    DummyMatchResultListener dummy;
    return MatchAndExplain(x, &dummy);
  }

  
  void DescribeTo(::std::ostream* os) const { impl_->DescribeTo(os); }

  
  void DescribeNegationTo(::std::ostream* os) const {
    impl_->DescribeNegationTo(os);
  }

  
  void ExplainMatchResultTo(T x, ::std::ostream* os) const {
    StreamMatchResultListener listener(os);
    MatchAndExplain(x, &listener);
  }

 protected:
  MatcherBase() {}

  
  explicit MatcherBase(const MatcherInterface<T>* impl)
      : impl_(impl) {}

  virtual ~MatcherBase() {}

 private:
  
  
  
  
  
  
  
  
  
  
  
  ::testing::internal::linked_ptr<const MatcherInterface<T> > impl_;
};

}  






template <typename T>
class Matcher : public internal::MatcherBase<T> {
 public:
  
  
  
  Matcher() {}

  
  explicit Matcher(const MatcherInterface<T>* impl)
      : internal::MatcherBase<T>(impl) {}

  
  
  Matcher(T value);  
};




template <>
class Matcher<const internal::string&>
    : public internal::MatcherBase<const internal::string&> {
 public:
  Matcher() {}

  explicit Matcher(const MatcherInterface<const internal::string&>* impl)
      : internal::MatcherBase<const internal::string&>(impl) {}

  
  
  Matcher(const internal::string& s);  

  
  Matcher(const char* s);  
};

template <>
class Matcher<internal::string>
    : public internal::MatcherBase<internal::string> {
 public:
  Matcher() {}

  explicit Matcher(const MatcherInterface<internal::string>* impl)
      : internal::MatcherBase<internal::string>(impl) {}

  
  
  Matcher(const internal::string& s);  

  
  Matcher(const char* s);  
};













template <class Impl>
class PolymorphicMatcher {
 public:
  explicit PolymorphicMatcher(const Impl& an_impl) : impl_(an_impl) {}

  
  
  Impl& mutable_impl() { return impl_; }

  
  
  const Impl& impl() const { return impl_; }

  template <typename T>
  operator Matcher<T>() const {
    return Matcher<T>(new MonomorphicImpl<T>(impl_));
  }

 private:
  template <typename T>
  class MonomorphicImpl : public MatcherInterface<T> {
   public:
    explicit MonomorphicImpl(const Impl& impl) : impl_(impl) {}

    virtual void DescribeTo(::std::ostream* os) const {
      impl_.DescribeTo(os);
    }

    virtual void DescribeNegationTo(::std::ostream* os) const {
      impl_.DescribeNegationTo(os);
    }

    virtual bool MatchAndExplain(T x, MatchResultListener* listener) const {
      return impl_.MatchAndExplain(x, listener);
    }

   private:
    const Impl impl_;

    GTEST_DISALLOW_ASSIGN_(MonomorphicImpl);
  };

  Impl impl_;

  GTEST_DISALLOW_ASSIGN_(PolymorphicMatcher);
};








template <typename T>
inline Matcher<T> MakeMatcher(const MatcherInterface<T>* impl) {
  return Matcher<T>(impl);
};








template <class Impl>
inline PolymorphicMatcher<Impl> MakePolymorphicMatcher(const Impl& impl) {
  return PolymorphicMatcher<Impl>(impl);
}





template <typename T, typename M>
Matcher<T> MatcherCast(M m);








template <typename T>
class SafeMatcherCastImpl {
 public:
  
  
  template <typename M>
  static inline Matcher<T> Cast(M polymorphic_matcher) {
    return Matcher<T>(polymorphic_matcher);
  }

  
  
  
  
  
  
  
  
  
  template <typename U>
  static inline Matcher<T> Cast(const Matcher<U>& matcher) {
    
    GTEST_COMPILE_ASSERT_((internal::ImplicitlyConvertible<T, U>::value),
                          T_must_be_implicitly_convertible_to_U);
    
    
    GTEST_COMPILE_ASSERT_(
        internal::is_reference<T>::value || !internal::is_reference<U>::value,
        cannot_convert_non_referentce_arg_to_reference);
    
    
    typedef GTEST_REMOVE_REFERENCE_AND_CONST_(T) RawT;
    typedef GTEST_REMOVE_REFERENCE_AND_CONST_(U) RawU;
    const bool kTIsOther = GMOCK_KIND_OF_(RawT) == internal::kOther;
    const bool kUIsOther = GMOCK_KIND_OF_(RawU) == internal::kOther;
    GTEST_COMPILE_ASSERT_(
        kTIsOther || kUIsOther ||
        (internal::LosslessArithmeticConvertible<RawT, RawU>::value),
        conversion_of_arithmetic_types_must_be_lossless);
    return MatcherCast<T>(matcher);
  }
};

template <typename T, typename M>
inline Matcher<T> SafeMatcherCast(const M& polymorphic_matcher) {
  return SafeMatcherCastImpl<T>::Cast(polymorphic_matcher);
}


template <typename T>
Matcher<T> A();



namespace internal {


inline void PrintIfNotEmpty(const internal::string& explanation,
                            std::ostream* os) {
  if (explanation != "" && os != NULL) {
    *os << ", " << explanation;
  }
}




inline bool IsReadableTypeName(const string& type_name) {
  
  
  return (type_name.length() <= 20 ||
          type_name.find_first_of("<(") == string::npos);
}






template <typename Value, typename T>
bool MatchPrintAndExplain(Value& value, const Matcher<T>& matcher,
                          MatchResultListener* listener) {
  if (!listener->IsInterested()) {
    
    
    return matcher.Matches(value);
  }

  StringMatchResultListener inner_listener;
  const bool match = matcher.MatchAndExplain(value, &inner_listener);

  UniversalPrint(value, listener->stream());
#if GTEST_HAS_RTTI
  const string& type_name = GetTypeName<Value>();
  if (IsReadableTypeName(type_name))
    *listener->stream() << " (of type " << type_name << ")";
#endif
  PrintIfNotEmpty(inner_listener.str(), listener->stream());

  return match;
}



template <size_t N>
class TuplePrefix {
 public:
  
  
  
  template <typename MatcherTuple, typename ValueTuple>
  static bool Matches(const MatcherTuple& matcher_tuple,
                      const ValueTuple& value_tuple) {
    using ::std::tr1::get;
    return TuplePrefix<N - 1>::Matches(matcher_tuple, value_tuple)
        && get<N - 1>(matcher_tuple).Matches(get<N - 1>(value_tuple));
  }

  
  
  
  
  template <typename MatcherTuple, typename ValueTuple>
  static void ExplainMatchFailuresTo(const MatcherTuple& matchers,
                                     const ValueTuple& values,
                                     ::std::ostream* os) {
    using ::std::tr1::tuple_element;
    using ::std::tr1::get;

    
    TuplePrefix<N - 1>::ExplainMatchFailuresTo(matchers, values, os);

    
    
    typename tuple_element<N - 1, MatcherTuple>::type matcher =
        get<N - 1>(matchers);
    typedef typename tuple_element<N - 1, ValueTuple>::type Value;
    Value value = get<N - 1>(values);
    StringMatchResultListener listener;
    if (!matcher.MatchAndExplain(value, &listener)) {
      
      
      *os << "  Expected arg #" << N - 1 << ": ";
      get<N - 1>(matchers).DescribeTo(os);
      *os << "\n           Actual: ";
      
      
      
      
      
      internal::UniversalPrint(value, os);
      PrintIfNotEmpty(listener.str(), os);
      *os << "\n";
    }
  }
};


template <>
class TuplePrefix<0> {
 public:
  template <typename MatcherTuple, typename ValueTuple>
  static bool Matches(const MatcherTuple& ,
                      const ValueTuple& ) {
    return true;
  }

  template <typename MatcherTuple, typename ValueTuple>
  static void ExplainMatchFailuresTo(const MatcherTuple& ,
                                     const ValueTuple& ,
                                     ::std::ostream* ) {}
};






template <typename MatcherTuple, typename ValueTuple>
bool TupleMatches(const MatcherTuple& matcher_tuple,
                  const ValueTuple& value_tuple) {
  using ::std::tr1::tuple_size;
  
  
  GTEST_COMPILE_ASSERT_(tuple_size<MatcherTuple>::value ==
                        tuple_size<ValueTuple>::value,
                        matcher_and_value_have_different_numbers_of_fields);
  return TuplePrefix<tuple_size<ValueTuple>::value>::
      Matches(matcher_tuple, value_tuple);
}



template <typename MatcherTuple, typename ValueTuple>
void ExplainMatchFailureTupleTo(const MatcherTuple& matchers,
                                const ValueTuple& values,
                                ::std::ostream* os) {
  using ::std::tr1::tuple_size;
  TuplePrefix<tuple_size<MatcherTuple>::value>::ExplainMatchFailuresTo(
      matchers, values, os);
}










template <typename T, typename M>
class MatcherCastImpl {
 public:
  static Matcher<T> Cast(M polymorphic_matcher) {
    return Matcher<T>(polymorphic_matcher);
  }
};




template <typename T, typename U>
class MatcherCastImpl<T, Matcher<U> > {
 public:
  static Matcher<T> Cast(const Matcher<U>& source_matcher) {
    return Matcher<T>(new Impl(source_matcher));
  }

 private:
  class Impl : public MatcherInterface<T> {
   public:
    explicit Impl(const Matcher<U>& source_matcher)
        : source_matcher_(source_matcher) {}

    
    virtual bool MatchAndExplain(T x, MatchResultListener* listener) const {
      return source_matcher_.MatchAndExplain(static_cast<U>(x), listener);
    }

    virtual void DescribeTo(::std::ostream* os) const {
      source_matcher_.DescribeTo(os);
    }

    virtual void DescribeNegationTo(::std::ostream* os) const {
      source_matcher_.DescribeNegationTo(os);
    }

   private:
    const Matcher<U> source_matcher_;

    GTEST_DISALLOW_ASSIGN_(Impl);
  };
};



template <typename T>
class MatcherCastImpl<T, Matcher<T> > {
 public:
  static Matcher<T> Cast(const Matcher<T>& matcher) { return matcher; }
};


template <typename T>
class AnyMatcherImpl : public MatcherInterface<T> {
 public:
  virtual bool MatchAndExplain(
      T , MatchResultListener* ) const { return true; }
  virtual void DescribeTo(::std::ostream* os) const { *os << "is anything"; }
  virtual void DescribeNegationTo(::std::ostream* os) const {
    
    
    
    *os << "never matches";
  }
};





class AnythingMatcher {
 public:
  template <typename T>
  operator Matcher<T>() const { return A<T>(); }
};














#define GMOCK_IMPLEMENT_COMPARISON_MATCHER_( \
    name, op, relation, negated_relation) \
  template <typename Rhs> class name##Matcher { \
   public: \
    explicit name##Matcher(const Rhs& rhs) : rhs_(rhs) {} \
    template <typename Lhs> \
    operator Matcher<Lhs>() const { \
      return MakeMatcher(new Impl<Lhs>(rhs_)); \
    } \
   private: \
    template <typename Lhs> \
    class Impl : public MatcherInterface<Lhs> { \
     public: \
      explicit Impl(const Rhs& rhs) : rhs_(rhs) {} \
      virtual bool MatchAndExplain(\
          Lhs lhs, MatchResultListener* /* listener */) const { \
        return lhs op rhs_; \
      } \
      virtual void DescribeTo(::std::ostream* os) const { \
        *os << relation  " "; \
        UniversalPrint(rhs_, os); \
      } \
      virtual void DescribeNegationTo(::std::ostream* os) const { \
        *os << negated_relation  " "; \
        UniversalPrint(rhs_, os); \
      } \
     private: \
      Rhs rhs_; \
      GTEST_DISALLOW_ASSIGN_(Impl); \
    }; \
    Rhs rhs_; \
    GTEST_DISALLOW_ASSIGN_(name##Matcher); \
  }



GMOCK_IMPLEMENT_COMPARISON_MATCHER_(Eq, ==, "is equal to", "isn't equal to");
GMOCK_IMPLEMENT_COMPARISON_MATCHER_(Ge, >=, "is >=", "isn't >=");
GMOCK_IMPLEMENT_COMPARISON_MATCHER_(Gt, >, "is >", "isn't >");
GMOCK_IMPLEMENT_COMPARISON_MATCHER_(Le, <=, "is <=", "isn't <=");
GMOCK_IMPLEMENT_COMPARISON_MATCHER_(Lt, <, "is <", "isn't <");
GMOCK_IMPLEMENT_COMPARISON_MATCHER_(Ne, !=, "isn't equal to", "is equal to");

#undef GMOCK_IMPLEMENT_COMPARISON_MATCHER_



class IsNullMatcher {
 public:
  template <typename Pointer>
  bool MatchAndExplain(const Pointer& p,
                       MatchResultListener* ) const {
    return GetRawPointer(p) == NULL;
  }

  void DescribeTo(::std::ostream* os) const { *os << "is NULL"; }
  void DescribeNegationTo(::std::ostream* os) const {
    *os << "isn't NULL";
  }
};



class NotNullMatcher {
 public:
  template <typename Pointer>
  bool MatchAndExplain(const Pointer& p,
                       MatchResultListener* ) const {
    return GetRawPointer(p) != NULL;
  }

  void DescribeTo(::std::ostream* os) const { *os << "isn't NULL"; }
  void DescribeNegationTo(::std::ostream* os) const {
    *os << "is NULL";
  }
};














template <typename T>
class RefMatcher;

template <typename T>
class RefMatcher<T&> {
  
  
  
  
  
 public:
  
  
  
  explicit RefMatcher(T& x) : object_(x) {}  

  template <typename Super>
  operator Matcher<Super&>() const {
    
    
    
    
    
    return MakeMatcher(new Impl<Super>(object_));
  }

 private:
  template <typename Super>
  class Impl : public MatcherInterface<Super&> {
   public:
    explicit Impl(Super& x) : object_(x) {}  

    
    
    virtual bool MatchAndExplain(
        Super& x, MatchResultListener* listener) const {
      *listener << "which is located @" << static_cast<const void*>(&x);
      return &x == &object_;
    }

    virtual void DescribeTo(::std::ostream* os) const {
      *os << "references the variable ";
      UniversalPrinter<Super&>::Print(object_, os);
    }

    virtual void DescribeNegationTo(::std::ostream* os) const {
      *os << "does not reference the variable ";
      UniversalPrinter<Super&>::Print(object_, os);
    }

   private:
    const Super& object_;

    GTEST_DISALLOW_ASSIGN_(Impl);
  };

  T& object_;

  GTEST_DISALLOW_ASSIGN_(RefMatcher);
};


inline bool CaseInsensitiveCStringEquals(const char* lhs, const char* rhs) {
  return String::CaseInsensitiveCStringEquals(lhs, rhs);
}

inline bool CaseInsensitiveCStringEquals(const wchar_t* lhs,
                                         const wchar_t* rhs) {
  return String::CaseInsensitiveWideCStringEquals(lhs, rhs);
}



template <typename StringType>
bool CaseInsensitiveStringEquals(const StringType& s1,
                                 const StringType& s2) {
  
  if (!CaseInsensitiveCStringEquals(s1.c_str(), s2.c_str())) {
    return false;
  }

  
  const typename StringType::value_type nul = 0;
  const size_t i1 = s1.find(nul), i2 = s2.find(nul);

  
  if (i1 == StringType::npos || i2 == StringType::npos) {
    return i1 == i2;
  }

  
  return CaseInsensitiveStringEquals(s1.substr(i1 + 1), s2.substr(i2 + 1));
}




template <typename StringType>
class StrEqualityMatcher {
 public:
  typedef typename StringType::const_pointer ConstCharPointer;

  StrEqualityMatcher(const StringType& str, bool expect_eq,
                     bool case_sensitive)
      : string_(str), expect_eq_(expect_eq), case_sensitive_(case_sensitive) {}

  
  
  bool MatchAndExplain(ConstCharPointer s,
                       MatchResultListener* listener) const {
    if (s == NULL) {
      return !expect_eq_;
    }
    return MatchAndExplain(StringType(s), listener);
  }

  bool MatchAndExplain(const StringType& s,
                       MatchResultListener* ) const {
    const bool eq = case_sensitive_ ? s == string_ :
        CaseInsensitiveStringEquals(s, string_);
    return expect_eq_ == eq;
  }

  void DescribeTo(::std::ostream* os) const {
    DescribeToHelper(expect_eq_, os);
  }

  void DescribeNegationTo(::std::ostream* os) const {
    DescribeToHelper(!expect_eq_, os);
  }

 private:
  void DescribeToHelper(bool expect_eq, ::std::ostream* os) const {
    *os << (expect_eq ? "is " : "isn't ");
    *os << "equal to ";
    if (!case_sensitive_) {
      *os << "(ignoring case) ";
    }
    UniversalPrint(string_, os);
  }

  const StringType string_;
  const bool expect_eq_;
  const bool case_sensitive_;

  GTEST_DISALLOW_ASSIGN_(StrEqualityMatcher);
};




template <typename StringType>
class HasSubstrMatcher {
 public:
  typedef typename StringType::const_pointer ConstCharPointer;

  explicit HasSubstrMatcher(const StringType& substring)
      : substring_(substring) {}

  
  
  
  bool MatchAndExplain(ConstCharPointer s,
                       MatchResultListener* listener) const {
    return s != NULL && MatchAndExplain(StringType(s), listener);
  }

  bool MatchAndExplain(const StringType& s,
                       MatchResultListener* ) const {
    return s.find(substring_) != StringType::npos;
  }

  
  void DescribeTo(::std::ostream* os) const {
    *os << "has substring ";
    UniversalPrint(substring_, os);
  }

  void DescribeNegationTo(::std::ostream* os) const {
    *os << "has no substring ";
    UniversalPrint(substring_, os);
  }

 private:
  const StringType substring_;

  GTEST_DISALLOW_ASSIGN_(HasSubstrMatcher);
};




template <typename StringType>
class StartsWithMatcher {
 public:
  typedef typename StringType::const_pointer ConstCharPointer;

  explicit StartsWithMatcher(const StringType& prefix) : prefix_(prefix) {
  }

  
  
  
  bool MatchAndExplain(ConstCharPointer s,
                       MatchResultListener* listener) const {
    return s != NULL && MatchAndExplain(StringType(s), listener);
  }

  bool MatchAndExplain(const StringType& s,
                       MatchResultListener* ) const {
    return s.length() >= prefix_.length() &&
        s.substr(0, prefix_.length()) == prefix_;
  }

  void DescribeTo(::std::ostream* os) const {
    *os << "starts with ";
    UniversalPrint(prefix_, os);
  }

  void DescribeNegationTo(::std::ostream* os) const {
    *os << "doesn't start with ";
    UniversalPrint(prefix_, os);
  }

 private:
  const StringType prefix_;

  GTEST_DISALLOW_ASSIGN_(StartsWithMatcher);
};




template <typename StringType>
class EndsWithMatcher {
 public:
  typedef typename StringType::const_pointer ConstCharPointer;

  explicit EndsWithMatcher(const StringType& suffix) : suffix_(suffix) {}

  
  
  
  bool MatchAndExplain(ConstCharPointer s,
                       MatchResultListener* listener) const {
    return s != NULL && MatchAndExplain(StringType(s), listener);
  }

  bool MatchAndExplain(const StringType& s,
                       MatchResultListener* ) const {
    return s.length() >= suffix_.length() &&
        s.substr(s.length() - suffix_.length()) == suffix_;
  }

  void DescribeTo(::std::ostream* os) const {
    *os << "ends with ";
    UniversalPrint(suffix_, os);
  }

  void DescribeNegationTo(::std::ostream* os) const {
    *os << "doesn't end with ";
    UniversalPrint(suffix_, os);
  }

 private:
  const StringType suffix_;

  GTEST_DISALLOW_ASSIGN_(EndsWithMatcher);
};




class MatchesRegexMatcher {
 public:
  MatchesRegexMatcher(const RE* regex, bool full_match)
      : regex_(regex), full_match_(full_match) {}

  
  
  
  
  bool MatchAndExplain(const char* s,
                       MatchResultListener* listener) const {
    return s != NULL && MatchAndExplain(internal::string(s), listener);
  }

  bool MatchAndExplain(const internal::string& s,
                       MatchResultListener* ) const {
    return full_match_ ? RE::FullMatch(s, *regex_) :
        RE::PartialMatch(s, *regex_);
  }

  void DescribeTo(::std::ostream* os) const {
    *os << (full_match_ ? "matches" : "contains")
        << " regular expression ";
    UniversalPrinter<internal::string>::Print(regex_->pattern(), os);
  }

  void DescribeNegationTo(::std::ostream* os) const {
    *os << "doesn't " << (full_match_ ? "match" : "contain")
        << " regular expression ";
    UniversalPrinter<internal::string>::Print(regex_->pattern(), os);
  }

 private:
  const internal::linked_ptr<const RE> regex_;
  const bool full_match_;

  GTEST_DISALLOW_ASSIGN_(MatchesRegexMatcher);
};












#define GMOCK_IMPLEMENT_COMPARISON2_MATCHER_(name, op, relation) \
  class name##2Matcher { \
   public: \
    template <typename T1, typename T2> \
    operator Matcher< ::std::tr1::tuple<T1, T2> >() const { \
      return MakeMatcher(new Impl< ::std::tr1::tuple<T1, T2> >); \
    } \
    template <typename T1, typename T2> \
    operator Matcher<const ::std::tr1::tuple<T1, T2>&>() const { \
      return MakeMatcher(new Impl<const ::std::tr1::tuple<T1, T2>&>); \
    } \
   private: \
    template <typename Tuple> \
    class Impl : public MatcherInterface<Tuple> { \
     public: \
      virtual bool MatchAndExplain( \
          Tuple args, \
          MatchResultListener* /* listener */) const { \
        return ::std::tr1::get<0>(args) op ::std::tr1::get<1>(args); \
      } \
      virtual void DescribeTo(::std::ostream* os) const { \
        *os << "are " relation;                                 \
      } \
      virtual void DescribeNegationTo(::std::ostream* os) const { \
        *os << "aren't " relation; \
      } \
    }; \
  }


GMOCK_IMPLEMENT_COMPARISON2_MATCHER_(Eq, ==, "an equal pair");
GMOCK_IMPLEMENT_COMPARISON2_MATCHER_(
    Ge, >=, "a pair where the first >= the second");
GMOCK_IMPLEMENT_COMPARISON2_MATCHER_(
    Gt, >, "a pair where the first > the second");
GMOCK_IMPLEMENT_COMPARISON2_MATCHER_(
    Le, <=, "a pair where the first <= the second");
GMOCK_IMPLEMENT_COMPARISON2_MATCHER_(
    Lt, <, "a pair where the first < the second");
GMOCK_IMPLEMENT_COMPARISON2_MATCHER_(Ne, !=, "an unequal pair");

#undef GMOCK_IMPLEMENT_COMPARISON2_MATCHER_





template <typename T>
class NotMatcherImpl : public MatcherInterface<T> {
 public:
  explicit NotMatcherImpl(const Matcher<T>& matcher)
      : matcher_(matcher) {}

  virtual bool MatchAndExplain(T x, MatchResultListener* listener) const {
    return !matcher_.MatchAndExplain(x, listener);
  }

  virtual void DescribeTo(::std::ostream* os) const {
    matcher_.DescribeNegationTo(os);
  }

  virtual void DescribeNegationTo(::std::ostream* os) const {
    matcher_.DescribeTo(os);
  }

 private:
  const Matcher<T> matcher_;

  GTEST_DISALLOW_ASSIGN_(NotMatcherImpl);
};



template <typename InnerMatcher>
class NotMatcher {
 public:
  explicit NotMatcher(InnerMatcher matcher) : matcher_(matcher) {}

  
  
  template <typename T>
  operator Matcher<T>() const {
    return Matcher<T>(new NotMatcherImpl<T>(SafeMatcherCast<T>(matcher_)));
  }

 private:
  InnerMatcher matcher_;

  GTEST_DISALLOW_ASSIGN_(NotMatcher);
};





template <typename T>
class BothOfMatcherImpl : public MatcherInterface<T> {
 public:
  BothOfMatcherImpl(const Matcher<T>& matcher1, const Matcher<T>& matcher2)
      : matcher1_(matcher1), matcher2_(matcher2) {}

  virtual void DescribeTo(::std::ostream* os) const {
    *os << "(";
    matcher1_.DescribeTo(os);
    *os << ") and (";
    matcher2_.DescribeTo(os);
    *os << ")";
  }

  virtual void DescribeNegationTo(::std::ostream* os) const {
    *os << "(";
    matcher1_.DescribeNegationTo(os);
    *os << ") or (";
    matcher2_.DescribeNegationTo(os);
    *os << ")";
  }

  virtual bool MatchAndExplain(T x, MatchResultListener* listener) const {
    
    
    StringMatchResultListener listener1;
    if (!matcher1_.MatchAndExplain(x, &listener1)) {
      *listener << listener1.str();
      return false;
    }

    StringMatchResultListener listener2;
    if (!matcher2_.MatchAndExplain(x, &listener2)) {
      *listener << listener2.str();
      return false;
    }

    
    const internal::string s1 = listener1.str();
    const internal::string s2 = listener2.str();

    if (s1 == "") {
      *listener << s2;
    } else {
      *listener << s1;
      if (s2 != "") {
        *listener << ", and " << s2;
      }
    }
    return true;
  }

 private:
  const Matcher<T> matcher1_;
  const Matcher<T> matcher2_;

  GTEST_DISALLOW_ASSIGN_(BothOfMatcherImpl);
};



template <typename Matcher1, typename Matcher2>
class BothOfMatcher {
 public:
  BothOfMatcher(Matcher1 matcher1, Matcher2 matcher2)
      : matcher1_(matcher1), matcher2_(matcher2) {}

  
  
  
  template <typename T>
  operator Matcher<T>() const {
    return Matcher<T>(new BothOfMatcherImpl<T>(SafeMatcherCast<T>(matcher1_),
                                               SafeMatcherCast<T>(matcher2_)));
  }

 private:
  Matcher1 matcher1_;
  Matcher2 matcher2_;

  GTEST_DISALLOW_ASSIGN_(BothOfMatcher);
};





template <typename T>
class EitherOfMatcherImpl : public MatcherInterface<T> {
 public:
  EitherOfMatcherImpl(const Matcher<T>& matcher1, const Matcher<T>& matcher2)
      : matcher1_(matcher1), matcher2_(matcher2) {}

  virtual void DescribeTo(::std::ostream* os) const {
    *os << "(";
    matcher1_.DescribeTo(os);
    *os << ") or (";
    matcher2_.DescribeTo(os);
    *os << ")";
  }

  virtual void DescribeNegationTo(::std::ostream* os) const {
    *os << "(";
    matcher1_.DescribeNegationTo(os);
    *os << ") and (";
    matcher2_.DescribeNegationTo(os);
    *os << ")";
  }

  virtual bool MatchAndExplain(T x, MatchResultListener* listener) const {
    
    
    StringMatchResultListener listener1;
    if (matcher1_.MatchAndExplain(x, &listener1)) {
      *listener << listener1.str();
      return true;
    }

    StringMatchResultListener listener2;
    if (matcher2_.MatchAndExplain(x, &listener2)) {
      *listener << listener2.str();
      return true;
    }

    
    const internal::string s1 = listener1.str();
    const internal::string s2 = listener2.str();

    if (s1 == "") {
      *listener << s2;
    } else {
      *listener << s1;
      if (s2 != "") {
        *listener << ", and " << s2;
      }
    }
    return false;
  }

 private:
  const Matcher<T> matcher1_;
  const Matcher<T> matcher2_;

  GTEST_DISALLOW_ASSIGN_(EitherOfMatcherImpl);
};




template <typename Matcher1, typename Matcher2>
class EitherOfMatcher {
 public:
  EitherOfMatcher(Matcher1 matcher1, Matcher2 matcher2)
      : matcher1_(matcher1), matcher2_(matcher2) {}

  
  
  
  template <typename T>
  operator Matcher<T>() const {
    return Matcher<T>(new EitherOfMatcherImpl<T>(
        SafeMatcherCast<T>(matcher1_), SafeMatcherCast<T>(matcher2_)));
  }

 private:
  Matcher1 matcher1_;
  Matcher2 matcher2_;

  GTEST_DISALLOW_ASSIGN_(EitherOfMatcher);
};



template <typename Predicate>
class TrulyMatcher {
 public:
  explicit TrulyMatcher(Predicate pred) : predicate_(pred) {}

  
  
  
  
  template <typename T>
  bool MatchAndExplain(T& x,  
                       MatchResultListener* ) const {
    
    
    
    
    
    
    if (predicate_(x))
      return true;
    return false;
  }

  void DescribeTo(::std::ostream* os) const {
    *os << "satisfies the given predicate";
  }

  void DescribeNegationTo(::std::ostream* os) const {
    *os << "doesn't satisfy the given predicate";
  }

 private:
  Predicate predicate_;

  GTEST_DISALLOW_ASSIGN_(TrulyMatcher);
};



template <typename M>
class MatcherAsPredicate {
 public:
  explicit MatcherAsPredicate(M matcher) : matcher_(matcher) {}

  
  
  
  
  
  
  template <typename T>
  bool operator()(const T& x) const {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    return MatcherCast<const T&>(matcher_).Matches(x);
  }

 private:
  M matcher_;

  GTEST_DISALLOW_ASSIGN_(MatcherAsPredicate);
};



template <typename M>
class PredicateFormatterFromMatcher {
 public:
  explicit PredicateFormatterFromMatcher(const M& m) : matcher_(m) {}

  
  
  
  template <typename T>
  AssertionResult operator()(const char* value_text, const T& x) const {
    
    
    
    
    
    
    
    
    
    const Matcher<const T&> matcher = MatcherCast<const T&>(matcher_);
    StringMatchResultListener listener;
    if (MatchPrintAndExplain(x, matcher, &listener))
      return AssertionSuccess();

    ::std::stringstream ss;
    ss << "Value of: " << value_text << "\n"
       << "Expected: ";
    matcher.DescribeTo(&ss);
    ss << "\n  Actual: " << listener.str();
    return AssertionFailure() << ss.str();
  }

 private:
  const M matcher_;

  GTEST_DISALLOW_ASSIGN_(PredicateFormatterFromMatcher);
};




template <typename M>
inline PredicateFormatterFromMatcher<M>
MakePredicateFormatterFromMatcher(const M& matcher) {
  return PredicateFormatterFromMatcher<M>(matcher);
}





template <typename FloatType>
class FloatingEqMatcher {
 public:
  
  
  
  
  FloatingEqMatcher(FloatType rhs, bool nan_eq_nan) :
    rhs_(rhs), nan_eq_nan_(nan_eq_nan) {}

  
  template <typename T>
  class Impl : public MatcherInterface<T> {
   public:
    Impl(FloatType rhs, bool nan_eq_nan) :
      rhs_(rhs), nan_eq_nan_(nan_eq_nan) {}

    virtual bool MatchAndExplain(T value,
                                 MatchResultListener* ) const {
      const FloatingPoint<FloatType> lhs(value), rhs(rhs_);

      
      if (nan_eq_nan_ && lhs.is_nan()) {
        return rhs.is_nan();
      }

      return lhs.AlmostEquals(rhs);
    }

    virtual void DescribeTo(::std::ostream* os) const {
      
      
      
      const ::std::streamsize old_precision = os->precision(
          ::std::numeric_limits<FloatType>::digits10 + 2);
      if (FloatingPoint<FloatType>(rhs_).is_nan()) {
        if (nan_eq_nan_) {
          *os << "is NaN";
        } else {
          *os << "never matches";
        }
      } else {
        *os << "is approximately " << rhs_;
      }
      os->precision(old_precision);
    }

    virtual void DescribeNegationTo(::std::ostream* os) const {
      
      const ::std::streamsize old_precision = os->precision(
          ::std::numeric_limits<FloatType>::digits10 + 2);
      if (FloatingPoint<FloatType>(rhs_).is_nan()) {
        if (nan_eq_nan_) {
          *os << "isn't NaN";
        } else {
          *os << "is anything";
        }
      } else {
        *os << "isn't approximately " << rhs_;
      }
      
      os->precision(old_precision);
    }

   private:
    const FloatType rhs_;
    const bool nan_eq_nan_;

    GTEST_DISALLOW_ASSIGN_(Impl);
  };

  
  
  
  
  
  
  operator Matcher<FloatType>() const {
    return MakeMatcher(new Impl<FloatType>(rhs_, nan_eq_nan_));
  }

  operator Matcher<const FloatType&>() const {
    return MakeMatcher(new Impl<const FloatType&>(rhs_, nan_eq_nan_));
  }

  operator Matcher<FloatType&>() const {
    return MakeMatcher(new Impl<FloatType&>(rhs_, nan_eq_nan_));
  }
 private:
  const FloatType rhs_;
  const bool nan_eq_nan_;

  GTEST_DISALLOW_ASSIGN_(FloatingEqMatcher);
};



template <typename InnerMatcher>
class PointeeMatcher {
 public:
  explicit PointeeMatcher(const InnerMatcher& matcher) : matcher_(matcher) {}

  
  
  
  
  
  
  
  
  template <typename Pointer>
  operator Matcher<Pointer>() const {
    return MakeMatcher(new Impl<Pointer>(matcher_));
  }

 private:
  
  template <typename Pointer>
  class Impl : public MatcherInterface<Pointer> {
   public:
    typedef typename PointeeOf<GTEST_REMOVE_CONST_(  
        GTEST_REMOVE_REFERENCE_(Pointer))>::type Pointee;

    explicit Impl(const InnerMatcher& matcher)
        : matcher_(MatcherCast<const Pointee&>(matcher)) {}

    virtual void DescribeTo(::std::ostream* os) const {
      *os << "points to a value that ";
      matcher_.DescribeTo(os);
    }

    virtual void DescribeNegationTo(::std::ostream* os) const {
      *os << "does not point to a value that ";
      matcher_.DescribeTo(os);
    }

    virtual bool MatchAndExplain(Pointer pointer,
                                 MatchResultListener* listener) const {
      if (GetRawPointer(pointer) == NULL)
        return false;

      *listener << "which points to ";
      return MatchPrintAndExplain(*pointer, matcher_, listener);
    }

   private:
    const Matcher<const Pointee&> matcher_;

    GTEST_DISALLOW_ASSIGN_(Impl);
  };

  const InnerMatcher matcher_;

  GTEST_DISALLOW_ASSIGN_(PointeeMatcher);
};



template <typename Class, typename FieldType>
class FieldMatcher {
 public:
  FieldMatcher(FieldType Class::*field,
               const Matcher<const FieldType&>& matcher)
      : field_(field), matcher_(matcher) {}

  void DescribeTo(::std::ostream* os) const {
    *os << "is an object whose given field ";
    matcher_.DescribeTo(os);
  }

  void DescribeNegationTo(::std::ostream* os) const {
    *os << "is an object whose given field ";
    matcher_.DescribeNegationTo(os);
  }

  template <typename T>
  bool MatchAndExplain(const T& value, MatchResultListener* listener) const {
    return MatchAndExplainImpl(
        typename ::testing::internal::
            is_pointer<GTEST_REMOVE_CONST_(T)>::type(),
        value, listener);
  }

 private:
  
  
  
  bool MatchAndExplainImpl(false_type , const Class& obj,
                           MatchResultListener* listener) const {
    *listener << "whose given field is ";
    return MatchPrintAndExplain(obj.*field_, matcher_, listener);
  }

  bool MatchAndExplainImpl(true_type , const Class* p,
                           MatchResultListener* listener) const {
    if (p == NULL)
      return false;

    *listener << "which points to an object ";
    
    
    
    return MatchAndExplainImpl(false_type(), *p, listener);
  }

  const FieldType Class::*field_;
  const Matcher<const FieldType&> matcher_;

  GTEST_DISALLOW_ASSIGN_(FieldMatcher);
};



template <typename Class, typename PropertyType>
class PropertyMatcher {
 public:
  
  
  
  
  typedef GTEST_REFERENCE_TO_CONST_(PropertyType) RefToConstProperty;

  PropertyMatcher(PropertyType (Class::*property)() const,
                  const Matcher<RefToConstProperty>& matcher)
      : property_(property), matcher_(matcher) {}

  void DescribeTo(::std::ostream* os) const {
    *os << "is an object whose given property ";
    matcher_.DescribeTo(os);
  }

  void DescribeNegationTo(::std::ostream* os) const {
    *os << "is an object whose given property ";
    matcher_.DescribeNegationTo(os);
  }

  template <typename T>
  bool MatchAndExplain(const T&value, MatchResultListener* listener) const {
    return MatchAndExplainImpl(
        typename ::testing::internal::
            is_pointer<GTEST_REMOVE_CONST_(T)>::type(),
        value, listener);
  }

 private:
  
  
  
  bool MatchAndExplainImpl(false_type , const Class& obj,
                           MatchResultListener* listener) const {
    *listener << "whose given property is ";
    
    
    RefToConstProperty result = (obj.*property_)();
    return MatchPrintAndExplain(result, matcher_, listener);
  }

  bool MatchAndExplainImpl(true_type , const Class* p,
                           MatchResultListener* listener) const {
    if (p == NULL)
      return false;

    *listener << "which points to an object ";
    
    
    
    return MatchAndExplainImpl(false_type(), *p, listener);
  }

  PropertyType (Class::*property_)() const;
  const Matcher<RefToConstProperty> matcher_;

  GTEST_DISALLOW_ASSIGN_(PropertyMatcher);
};





template <typename Functor>
struct CallableTraits {
  typedef typename Functor::result_type ResultType;
  typedef Functor StorageType;

  static void CheckIsValid(Functor ) {}
  template <typename T>
  static ResultType Invoke(Functor f, T arg) { return f(arg); }
};


template <typename ArgType, typename ResType>
struct CallableTraits<ResType(*)(ArgType)> {
  typedef ResType ResultType;
  typedef ResType(*StorageType)(ArgType);

  static void CheckIsValid(ResType(*f)(ArgType)) {
    GTEST_CHECK_(f != NULL)
        << "NULL function pointer is passed into ResultOf().";
  }
  template <typename T>
  static ResType Invoke(ResType(*f)(ArgType), T arg) {
    return (*f)(arg);
  }
};



template <typename Callable>
class ResultOfMatcher {
 public:
  typedef typename CallableTraits<Callable>::ResultType ResultType;

  ResultOfMatcher(Callable callable, const Matcher<ResultType>& matcher)
      : callable_(callable), matcher_(matcher) {
    CallableTraits<Callable>::CheckIsValid(callable_);
  }

  template <typename T>
  operator Matcher<T>() const {
    return Matcher<T>(new Impl<T>(callable_, matcher_));
  }

 private:
  typedef typename CallableTraits<Callable>::StorageType CallableStorageType;

  template <typename T>
  class Impl : public MatcherInterface<T> {
   public:
    Impl(CallableStorageType callable, const Matcher<ResultType>& matcher)
        : callable_(callable), matcher_(matcher) {}

    virtual void DescribeTo(::std::ostream* os) const {
      *os << "is mapped by the given callable to a value that ";
      matcher_.DescribeTo(os);
    }

    virtual void DescribeNegationTo(::std::ostream* os) const {
      *os << "is mapped by the given callable to a value that ";
      matcher_.DescribeNegationTo(os);
    }

    virtual bool MatchAndExplain(T obj, MatchResultListener* listener) const {
      *listener << "which is mapped by the given callable to ";
      
      
      ResultType result =
          CallableTraits<Callable>::template Invoke<T>(callable_, obj);
      return MatchPrintAndExplain(result, matcher_, listener);
    }

   private:
    
    
    
    
    
    mutable CallableStorageType callable_;
    const Matcher<ResultType> matcher_;

    GTEST_DISALLOW_ASSIGN_(Impl);
  };  

  const CallableStorageType callable_;
  const Matcher<ResultType> matcher_;

  GTEST_DISALLOW_ASSIGN_(ResultOfMatcher);
};











template <typename Container>
class ContainerEqMatcher {
 public:
  typedef internal::StlContainerView<Container> View;
  typedef typename View::type StlContainer;
  typedef typename View::const_reference StlContainerReference;

  
  
  explicit ContainerEqMatcher(const Container& rhs) : rhs_(View::Copy(rhs)) {
    
    
    (void)testing::StaticAssertTypeEq<Container,
        GTEST_REMOVE_REFERENCE_AND_CONST_(Container)>();
  }

  void DescribeTo(::std::ostream* os) const {
    *os << "equals ";
    UniversalPrint(rhs_, os);
  }
  void DescribeNegationTo(::std::ostream* os) const {
    *os << "does not equal ";
    UniversalPrint(rhs_, os);
  }

  template <typename LhsContainer>
  bool MatchAndExplain(const LhsContainer& lhs,
                       MatchResultListener* listener) const {
    
    
    typedef internal::StlContainerView<GTEST_REMOVE_CONST_(LhsContainer)>
        LhsView;
    typedef typename LhsView::type LhsStlContainer;
    StlContainerReference lhs_stl_container = LhsView::ConstReference(lhs);
    if (lhs_stl_container == rhs_)
      return true;

    ::std::ostream* const os = listener->stream();
    if (os != NULL) {
      
      bool printed_header = false;
      for (typename LhsStlContainer::const_iterator it =
               lhs_stl_container.begin();
           it != lhs_stl_container.end(); ++it) {
        if (internal::ArrayAwareFind(rhs_.begin(), rhs_.end(), *it) ==
            rhs_.end()) {
          if (printed_header) {
            *os << ", ";
          } else {
            *os << "which has these unexpected elements: ";
            printed_header = true;
          }
          UniversalPrint(*it, os);
        }
      }

      
      bool printed_header2 = false;
      for (typename StlContainer::const_iterator it = rhs_.begin();
           it != rhs_.end(); ++it) {
        if (internal::ArrayAwareFind(
                lhs_stl_container.begin(), lhs_stl_container.end(), *it) ==
            lhs_stl_container.end()) {
          if (printed_header2) {
            *os << ", ";
          } else {
            *os << (printed_header ? ",\nand" : "which")
                << " doesn't have these expected elements: ";
            printed_header2 = true;
          }
          UniversalPrint(*it, os);
        }
      }
    }

    return false;
  }

 private:
  const StlContainer rhs_;

  GTEST_DISALLOW_ASSIGN_(ContainerEqMatcher);
};





template <typename TupleMatcher, typename RhsContainer>
class PointwiseMatcher {
 public:
  typedef internal::StlContainerView<RhsContainer> RhsView;
  typedef typename RhsView::type RhsStlContainer;
  typedef typename RhsStlContainer::value_type RhsValue;

  
  
  PointwiseMatcher(const TupleMatcher& tuple_matcher, const RhsContainer& rhs)
      : tuple_matcher_(tuple_matcher), rhs_(RhsView::Copy(rhs)) {
    
    
    (void)testing::StaticAssertTypeEq<RhsContainer,
        GTEST_REMOVE_REFERENCE_AND_CONST_(RhsContainer)>();
  }

  template <typename LhsContainer>
  operator Matcher<LhsContainer>() const {
    return MakeMatcher(new Impl<LhsContainer>(tuple_matcher_, rhs_));
  }

  template <typename LhsContainer>
  class Impl : public MatcherInterface<LhsContainer> {
   public:
    typedef internal::StlContainerView<
         GTEST_REMOVE_REFERENCE_AND_CONST_(LhsContainer)> LhsView;
    typedef typename LhsView::type LhsStlContainer;
    typedef typename LhsView::const_reference LhsStlContainerReference;
    typedef typename LhsStlContainer::value_type LhsValue;
    
    
    
    
    typedef std::tr1::tuple<const LhsValue&, const RhsValue&> InnerMatcherArg;

    Impl(const TupleMatcher& tuple_matcher, const RhsStlContainer& rhs)
        
        : mono_tuple_matcher_(SafeMatcherCast<InnerMatcherArg>(tuple_matcher)),
          rhs_(rhs) {}

    virtual void DescribeTo(::std::ostream* os) const {
      *os << "contains " << rhs_.size()
          << " values, where each value and its corresponding value in ";
      UniversalPrinter<RhsStlContainer>::Print(rhs_, os);
      *os << " ";
      mono_tuple_matcher_.DescribeTo(os);
    }
    virtual void DescribeNegationTo(::std::ostream* os) const {
      *os << "doesn't contain exactly " << rhs_.size()
          << " values, or contains a value x at some index i"
          << " where x and the i-th value of ";
      UniversalPrint(rhs_, os);
      *os << " ";
      mono_tuple_matcher_.DescribeNegationTo(os);
    }

    virtual bool MatchAndExplain(LhsContainer lhs,
                                 MatchResultListener* listener) const {
      LhsStlContainerReference lhs_stl_container = LhsView::ConstReference(lhs);
      const size_t actual_size = lhs_stl_container.size();
      if (actual_size != rhs_.size()) {
        *listener << "which contains " << actual_size << " values";
        return false;
      }

      typename LhsStlContainer::const_iterator left = lhs_stl_container.begin();
      typename RhsStlContainer::const_iterator right = rhs_.begin();
      for (size_t i = 0; i != actual_size; ++i, ++left, ++right) {
        const InnerMatcherArg value_pair(*left, *right);

        if (listener->IsInterested()) {
          StringMatchResultListener inner_listener;
          if (!mono_tuple_matcher_.MatchAndExplain(
                  value_pair, &inner_listener)) {
            *listener << "where the value pair (";
            UniversalPrint(*left, listener->stream());
            *listener << ", ";
            UniversalPrint(*right, listener->stream());
            *listener << ") at index #" << i << " don't match";
            PrintIfNotEmpty(inner_listener.str(), listener->stream());
            return false;
          }
        } else {
          if (!mono_tuple_matcher_.Matches(value_pair))
            return false;
        }
      }

      return true;
    }

   private:
    const Matcher<InnerMatcherArg> mono_tuple_matcher_;
    const RhsStlContainer rhs_;

    GTEST_DISALLOW_ASSIGN_(Impl);
  };

 private:
  const TupleMatcher tuple_matcher_;
  const RhsStlContainer rhs_;

  GTEST_DISALLOW_ASSIGN_(PointwiseMatcher);
};


template <typename Container>
class QuantifierMatcherImpl : public MatcherInterface<Container> {
 public:
  typedef GTEST_REMOVE_REFERENCE_AND_CONST_(Container) RawContainer;
  typedef StlContainerView<RawContainer> View;
  typedef typename View::type StlContainer;
  typedef typename View::const_reference StlContainerReference;
  typedef typename StlContainer::value_type Element;

  template <typename InnerMatcher>
  explicit QuantifierMatcherImpl(InnerMatcher inner_matcher)
      : inner_matcher_(
           testing::SafeMatcherCast<const Element&>(inner_matcher)) {}

  
  
  
  bool MatchAndExplainImpl(bool all_elements_should_match,
                           Container container,
                           MatchResultListener* listener) const {
    StlContainerReference stl_container = View::ConstReference(container);
    size_t i = 0;
    for (typename StlContainer::const_iterator it = stl_container.begin();
         it != stl_container.end(); ++it, ++i) {
      StringMatchResultListener inner_listener;
      const bool matches = inner_matcher_.MatchAndExplain(*it, &inner_listener);

      if (matches != all_elements_should_match) {
        *listener << "whose element #" << i
                  << (matches ? " matches" : " doesn't match");
        PrintIfNotEmpty(inner_listener.str(), listener->stream());
        return !all_elements_should_match;
      }
    }
    return all_elements_should_match;
  }

 protected:
  const Matcher<const Element&> inner_matcher_;

  GTEST_DISALLOW_ASSIGN_(QuantifierMatcherImpl);
};



template <typename Container>
class ContainsMatcherImpl : public QuantifierMatcherImpl<Container> {
 public:
  template <typename InnerMatcher>
  explicit ContainsMatcherImpl(InnerMatcher inner_matcher)
      : QuantifierMatcherImpl<Container>(inner_matcher) {}

  
  virtual void DescribeTo(::std::ostream* os) const {
    *os << "contains at least one element that ";
    this->inner_matcher_.DescribeTo(os);
  }

  virtual void DescribeNegationTo(::std::ostream* os) const {
    *os << "doesn't contain any element that ";
    this->inner_matcher_.DescribeTo(os);
  }

  virtual bool MatchAndExplain(Container container,
                               MatchResultListener* listener) const {
    return this->MatchAndExplainImpl(false, container, listener);
  }

 private:
  GTEST_DISALLOW_ASSIGN_(ContainsMatcherImpl);
};



template <typename Container>
class EachMatcherImpl : public QuantifierMatcherImpl<Container> {
 public:
  template <typename InnerMatcher>
  explicit EachMatcherImpl(InnerMatcher inner_matcher)
      : QuantifierMatcherImpl<Container>(inner_matcher) {}

  
  virtual void DescribeTo(::std::ostream* os) const {
    *os << "only contains elements that ";
    this->inner_matcher_.DescribeTo(os);
  }

  virtual void DescribeNegationTo(::std::ostream* os) const {
    *os << "contains some element that ";
    this->inner_matcher_.DescribeNegationTo(os);
  }

  virtual bool MatchAndExplain(Container container,
                               MatchResultListener* listener) const {
    return this->MatchAndExplainImpl(true, container, listener);
  }

 private:
  GTEST_DISALLOW_ASSIGN_(EachMatcherImpl);
};


template <typename M>
class ContainsMatcher {
 public:
  explicit ContainsMatcher(M m) : inner_matcher_(m) {}

  template <typename Container>
  operator Matcher<Container>() const {
    return MakeMatcher(new ContainsMatcherImpl<Container>(inner_matcher_));
  }

 private:
  const M inner_matcher_;

  GTEST_DISALLOW_ASSIGN_(ContainsMatcher);
};


template <typename M>
class EachMatcher {
 public:
  explicit EachMatcher(M m) : inner_matcher_(m) {}

  template <typename Container>
  operator Matcher<Container>() const {
    return MakeMatcher(new EachMatcherImpl<Container>(inner_matcher_));
  }

 private:
  const M inner_matcher_;

  GTEST_DISALLOW_ASSIGN_(EachMatcher);
};





template <typename PairType>
class KeyMatcherImpl : public MatcherInterface<PairType> {
 public:
  typedef GTEST_REMOVE_REFERENCE_AND_CONST_(PairType) RawPairType;
  typedef typename RawPairType::first_type KeyType;

  template <typename InnerMatcher>
  explicit KeyMatcherImpl(InnerMatcher inner_matcher)
      : inner_matcher_(
          testing::SafeMatcherCast<const KeyType&>(inner_matcher)) {
  }

  
  virtual bool MatchAndExplain(PairType key_value,
                               MatchResultListener* listener) const {
    StringMatchResultListener inner_listener;
    const bool match = inner_matcher_.MatchAndExplain(key_value.first,
                                                      &inner_listener);
    const internal::string explanation = inner_listener.str();
    if (explanation != "") {
      *listener << "whose first field is a value " << explanation;
    }
    return match;
  }

  
  virtual void DescribeTo(::std::ostream* os) const {
    *os << "has a key that ";
    inner_matcher_.DescribeTo(os);
  }

  
  virtual void DescribeNegationTo(::std::ostream* os) const {
    *os << "doesn't have a key that ";
    inner_matcher_.DescribeTo(os);
  }

 private:
  const Matcher<const KeyType&> inner_matcher_;

  GTEST_DISALLOW_ASSIGN_(KeyMatcherImpl);
};


template <typename M>
class KeyMatcher {
 public:
  explicit KeyMatcher(M m) : matcher_for_key_(m) {}

  template <typename PairType>
  operator Matcher<PairType>() const {
    return MakeMatcher(new KeyMatcherImpl<PairType>(matcher_for_key_));
  }

 private:
  const M matcher_for_key_;

  GTEST_DISALLOW_ASSIGN_(KeyMatcher);
};



template <typename PairType>
class PairMatcherImpl : public MatcherInterface<PairType> {
 public:
  typedef GTEST_REMOVE_REFERENCE_AND_CONST_(PairType) RawPairType;
  typedef typename RawPairType::first_type FirstType;
  typedef typename RawPairType::second_type SecondType;

  template <typename FirstMatcher, typename SecondMatcher>
  PairMatcherImpl(FirstMatcher first_matcher, SecondMatcher second_matcher)
      : first_matcher_(
            testing::SafeMatcherCast<const FirstType&>(first_matcher)),
        second_matcher_(
            testing::SafeMatcherCast<const SecondType&>(second_matcher)) {
  }

  
  virtual void DescribeTo(::std::ostream* os) const {
    *os << "has a first field that ";
    first_matcher_.DescribeTo(os);
    *os << ", and has a second field that ";
    second_matcher_.DescribeTo(os);
  }

  
  virtual void DescribeNegationTo(::std::ostream* os) const {
    *os << "has a first field that ";
    first_matcher_.DescribeNegationTo(os);
    *os << ", or has a second field that ";
    second_matcher_.DescribeNegationTo(os);
  }

  
  
  virtual bool MatchAndExplain(PairType a_pair,
                               MatchResultListener* listener) const {
    if (!listener->IsInterested()) {
      
      
      return first_matcher_.Matches(a_pair.first) &&
             second_matcher_.Matches(a_pair.second);
    }
    StringMatchResultListener first_inner_listener;
    if (!first_matcher_.MatchAndExplain(a_pair.first,
                                        &first_inner_listener)) {
      *listener << "whose first field does not match";
      PrintIfNotEmpty(first_inner_listener.str(), listener->stream());
      return false;
    }
    StringMatchResultListener second_inner_listener;
    if (!second_matcher_.MatchAndExplain(a_pair.second,
                                         &second_inner_listener)) {
      *listener << "whose second field does not match";
      PrintIfNotEmpty(second_inner_listener.str(), listener->stream());
      return false;
    }
    ExplainSuccess(first_inner_listener.str(), second_inner_listener.str(),
                   listener);
    return true;
  }

 private:
  void ExplainSuccess(const internal::string& first_explanation,
                      const internal::string& second_explanation,
                      MatchResultListener* listener) const {
    *listener << "whose both fields match";
    if (first_explanation != "") {
      *listener << ", where the first field is a value " << first_explanation;
    }
    if (second_explanation != "") {
      *listener << ", ";
      if (first_explanation != "") {
        *listener << "and ";
      } else {
        *listener << "where ";
      }
      *listener << "the second field is a value " << second_explanation;
    }
  }

  const Matcher<const FirstType&> first_matcher_;
  const Matcher<const SecondType&> second_matcher_;

  GTEST_DISALLOW_ASSIGN_(PairMatcherImpl);
};


template <typename FirstMatcher, typename SecondMatcher>
class PairMatcher {
 public:
  PairMatcher(FirstMatcher first_matcher, SecondMatcher second_matcher)
      : first_matcher_(first_matcher), second_matcher_(second_matcher) {}

  template <typename PairType>
  operator Matcher<PairType> () const {
    return MakeMatcher(
        new PairMatcherImpl<PairType>(
            first_matcher_, second_matcher_));
  }

 private:
  const FirstMatcher first_matcher_;
  const SecondMatcher second_matcher_;

  GTEST_DISALLOW_ASSIGN_(PairMatcher);
};


template <typename Container>
class ElementsAreMatcherImpl : public MatcherInterface<Container> {
 public:
  typedef GTEST_REMOVE_REFERENCE_AND_CONST_(Container) RawContainer;
  typedef internal::StlContainerView<RawContainer> View;
  typedef typename View::type StlContainer;
  typedef typename View::const_reference StlContainerReference;
  typedef typename StlContainer::value_type Element;

  
  
  template <typename InputIter>
  ElementsAreMatcherImpl(InputIter first, size_t a_count) {
    matchers_.reserve(a_count);
    InputIter it = first;
    for (size_t i = 0; i != a_count; ++i, ++it) {
      matchers_.push_back(MatcherCast<const Element&>(*it));
    }
  }

  
  virtual void DescribeTo(::std::ostream* os) const {
    if (count() == 0) {
      *os << "is empty";
    } else if (count() == 1) {
      *os << "has 1 element that ";
      matchers_[0].DescribeTo(os);
    } else {
      *os << "has " << Elements(count()) << " where\n";
      for (size_t i = 0; i != count(); ++i) {
        *os << "element #" << i << " ";
        matchers_[i].DescribeTo(os);
        if (i + 1 < count()) {
          *os << ",\n";
        }
      }
    }
  }

  
  virtual void DescribeNegationTo(::std::ostream* os) const {
    if (count() == 0) {
      *os << "isn't empty";
      return;
    }

    *os << "doesn't have " << Elements(count()) << ", or\n";
    for (size_t i = 0; i != count(); ++i) {
      *os << "element #" << i << " ";
      matchers_[i].DescribeNegationTo(os);
      if (i + 1 < count()) {
        *os << ", or\n";
      }
    }
  }

  virtual bool MatchAndExplain(Container container,
                               MatchResultListener* listener) const {
    StlContainerReference stl_container = View::ConstReference(container);
    const size_t actual_count = stl_container.size();
    if (actual_count != count()) {
      
      
      
      
      if (actual_count != 0) {
        *listener << "which has " << Elements(actual_count);
      }
      return false;
    }

    typename StlContainer::const_iterator it = stl_container.begin();
    
    std::vector<internal::string> explanations(count());
    for (size_t i = 0; i != count();  ++it, ++i) {
      StringMatchResultListener s;
      if (matchers_[i].MatchAndExplain(*it, &s)) {
        explanations[i] = s.str();
      } else {
        
        
        *listener << "whose element #" << i << " doesn't match";
        PrintIfNotEmpty(s.str(), listener->stream());
        return false;
      }
    }

    
    
    bool reason_printed = false;
    for (size_t i = 0; i != count(); ++i) {
      const internal::string& s = explanations[i];
      if (!s.empty()) {
        if (reason_printed) {
          *listener << ",\nand ";
        }
        *listener << "whose element #" << i << " matches, " << s;
        reason_printed = true;
      }
    }

    return true;
  }

 private:
  static Message Elements(size_t count) {
    return Message() << count << (count == 1 ? " element" : " elements");
  }

  size_t count() const { return matchers_.size(); }
  std::vector<Matcher<const Element&> > matchers_;

  GTEST_DISALLOW_ASSIGN_(ElementsAreMatcherImpl);
};


class ElementsAreMatcher0 {
 public:
  ElementsAreMatcher0() {}

  template <typename Container>
  operator Matcher<Container>() const {
    typedef GTEST_REMOVE_REFERENCE_AND_CONST_(Container) RawContainer;
    typedef typename internal::StlContainerView<RawContainer>::type::value_type
        Element;

    const Matcher<const Element&>* const matchers = NULL;
    return MakeMatcher(new ElementsAreMatcherImpl<Container>(matchers, 0));
  }
};


template <typename T>
class ElementsAreArrayMatcher {
 public:
  ElementsAreArrayMatcher(const T* first, size_t count) :
      first_(first), count_(count) {}

  template <typename Container>
  operator Matcher<Container>() const {
    typedef GTEST_REMOVE_REFERENCE_AND_CONST_(Container) RawContainer;
    typedef typename internal::StlContainerView<RawContainer>::type::value_type
        Element;

    return MakeMatcher(new ElementsAreMatcherImpl<Container>(first_, count_));
  }

 private:
  const T* const first_;
  const size_t count_;

  GTEST_DISALLOW_ASSIGN_(ElementsAreArrayMatcher);
};






string FormatMatcherDescription(bool negation, const char* matcher_name,
                                const Strings& param_values);

}  


template <typename T, typename M>
inline Matcher<T> MatcherCast(M matcher) {
  return internal::MatcherCastImpl<T, M>::Cast(matcher);
}










const internal::AnythingMatcher _ = {};

template <typename T>
inline Matcher<T> A() { return MakeMatcher(new internal::AnyMatcherImpl<T>()); }


template <typename T>
inline Matcher<T> An() { return A<T>(); }




template <typename T>
inline internal::EqMatcher<T> Eq(T x) { return internal::EqMatcher<T>(x); }



template <typename T>
Matcher<T>::Matcher(T value) { *this = Eq(value); }













template <typename Lhs, typename Rhs>
inline Matcher<Lhs> TypedEq(const Rhs& rhs) { return Eq(rhs); }


template <typename Rhs>
inline internal::GeMatcher<Rhs> Ge(Rhs x) {
  return internal::GeMatcher<Rhs>(x);
}


template <typename Rhs>
inline internal::GtMatcher<Rhs> Gt(Rhs x) {
  return internal::GtMatcher<Rhs>(x);
}


template <typename Rhs>
inline internal::LeMatcher<Rhs> Le(Rhs x) {
  return internal::LeMatcher<Rhs>(x);
}


template <typename Rhs>
inline internal::LtMatcher<Rhs> Lt(Rhs x) {
  return internal::LtMatcher<Rhs>(x);
}


template <typename Rhs>
inline internal::NeMatcher<Rhs> Ne(Rhs x) {
  return internal::NeMatcher<Rhs>(x);
}


inline PolymorphicMatcher<internal::IsNullMatcher > IsNull() {
  return MakePolymorphicMatcher(internal::IsNullMatcher());
}




inline PolymorphicMatcher<internal::NotNullMatcher > NotNull() {
  return MakePolymorphicMatcher(internal::NotNullMatcher());
}



template <typename T>
inline internal::RefMatcher<T&> Ref(T& x) {  
  return internal::RefMatcher<T&>(x);
}



inline internal::FloatingEqMatcher<double> DoubleEq(double rhs) {
  return internal::FloatingEqMatcher<double>(rhs, false);
}



inline internal::FloatingEqMatcher<double> NanSensitiveDoubleEq(double rhs) {
  return internal::FloatingEqMatcher<double>(rhs, true);
}



inline internal::FloatingEqMatcher<float> FloatEq(float rhs) {
  return internal::FloatingEqMatcher<float>(rhs, false);
}



inline internal::FloatingEqMatcher<float> NanSensitiveFloatEq(float rhs) {
  return internal::FloatingEqMatcher<float>(rhs, true);
}



template <typename InnerMatcher>
inline internal::PointeeMatcher<InnerMatcher> Pointee(
    const InnerMatcher& inner_matcher) {
  return internal::PointeeMatcher<InnerMatcher>(inner_matcher);
}





template <typename Class, typename FieldType, typename FieldMatcher>
inline PolymorphicMatcher<
  internal::FieldMatcher<Class, FieldType> > Field(
    FieldType Class::*field, const FieldMatcher& matcher) {
  return MakePolymorphicMatcher(
      internal::FieldMatcher<Class, FieldType>(
          field, MatcherCast<const FieldType&>(matcher)));
  
  
  
  
}





template <typename Class, typename PropertyType, typename PropertyMatcher>
inline PolymorphicMatcher<
  internal::PropertyMatcher<Class, PropertyType> > Property(
    PropertyType (Class::*property)() const, const PropertyMatcher& matcher) {
  return MakePolymorphicMatcher(
      internal::PropertyMatcher<Class, PropertyType>(
          property,
          MatcherCast<GTEST_REFERENCE_TO_CONST_(PropertyType)>(matcher)));
  
  
  
  
}














template <typename Callable, typename ResultOfMatcher>
internal::ResultOfMatcher<Callable> ResultOf(
    Callable callable, const ResultOfMatcher& matcher) {
  return internal::ResultOfMatcher<Callable>(
          callable,
          MatcherCast<typename internal::CallableTraits<Callable>::ResultType>(
              matcher));
  
  
  
  
}




inline PolymorphicMatcher<internal::StrEqualityMatcher<internal::string> >
    StrEq(const internal::string& str) {
  return MakePolymorphicMatcher(internal::StrEqualityMatcher<internal::string>(
      str, true, true));
}


inline PolymorphicMatcher<internal::StrEqualityMatcher<internal::string> >
    StrNe(const internal::string& str) {
  return MakePolymorphicMatcher(internal::StrEqualityMatcher<internal::string>(
      str, false, true));
}


inline PolymorphicMatcher<internal::StrEqualityMatcher<internal::string> >
    StrCaseEq(const internal::string& str) {
  return MakePolymorphicMatcher(internal::StrEqualityMatcher<internal::string>(
      str, true, false));
}


inline PolymorphicMatcher<internal::StrEqualityMatcher<internal::string> >
    StrCaseNe(const internal::string& str) {
  return MakePolymorphicMatcher(internal::StrEqualityMatcher<internal::string>(
      str, false, false));
}



inline PolymorphicMatcher<internal::HasSubstrMatcher<internal::string> >
    HasSubstr(const internal::string& substring) {
  return MakePolymorphicMatcher(internal::HasSubstrMatcher<internal::string>(
      substring));
}


inline PolymorphicMatcher<internal::StartsWithMatcher<internal::string> >
    StartsWith(const internal::string& prefix) {
  return MakePolymorphicMatcher(internal::StartsWithMatcher<internal::string>(
      prefix));
}


inline PolymorphicMatcher<internal::EndsWithMatcher<internal::string> >
    EndsWith(const internal::string& suffix) {
  return MakePolymorphicMatcher(internal::EndsWithMatcher<internal::string>(
      suffix));
}



inline PolymorphicMatcher<internal::MatchesRegexMatcher> MatchesRegex(
    const internal::RE* regex) {
  return MakePolymorphicMatcher(internal::MatchesRegexMatcher(regex, true));
}
inline PolymorphicMatcher<internal::MatchesRegexMatcher> MatchesRegex(
    const internal::string& regex) {
  return MatchesRegex(new internal::RE(regex));
}



inline PolymorphicMatcher<internal::MatchesRegexMatcher> ContainsRegex(
    const internal::RE* regex) {
  return MakePolymorphicMatcher(internal::MatchesRegexMatcher(regex, false));
}
inline PolymorphicMatcher<internal::MatchesRegexMatcher> ContainsRegex(
    const internal::string& regex) {
  return ContainsRegex(new internal::RE(regex));
}

#if GTEST_HAS_GLOBAL_WSTRING || GTEST_HAS_STD_WSTRING



inline PolymorphicMatcher<internal::StrEqualityMatcher<internal::wstring> >
    StrEq(const internal::wstring& str) {
  return MakePolymorphicMatcher(internal::StrEqualityMatcher<internal::wstring>(
      str, true, true));
}


inline PolymorphicMatcher<internal::StrEqualityMatcher<internal::wstring> >
    StrNe(const internal::wstring& str) {
  return MakePolymorphicMatcher(internal::StrEqualityMatcher<internal::wstring>(
      str, false, true));
}


inline PolymorphicMatcher<internal::StrEqualityMatcher<internal::wstring> >
    StrCaseEq(const internal::wstring& str) {
  return MakePolymorphicMatcher(internal::StrEqualityMatcher<internal::wstring>(
      str, true, false));
}


inline PolymorphicMatcher<internal::StrEqualityMatcher<internal::wstring> >
    StrCaseNe(const internal::wstring& str) {
  return MakePolymorphicMatcher(internal::StrEqualityMatcher<internal::wstring>(
      str, false, false));
}



inline PolymorphicMatcher<internal::HasSubstrMatcher<internal::wstring> >
    HasSubstr(const internal::wstring& substring) {
  return MakePolymorphicMatcher(internal::HasSubstrMatcher<internal::wstring>(
      substring));
}


inline PolymorphicMatcher<internal::StartsWithMatcher<internal::wstring> >
    StartsWith(const internal::wstring& prefix) {
  return MakePolymorphicMatcher(internal::StartsWithMatcher<internal::wstring>(
      prefix));
}


inline PolymorphicMatcher<internal::EndsWithMatcher<internal::wstring> >
    EndsWith(const internal::wstring& suffix) {
  return MakePolymorphicMatcher(internal::EndsWithMatcher<internal::wstring>(
      suffix));
}

#endif  



inline internal::Eq2Matcher Eq() { return internal::Eq2Matcher(); }



inline internal::Ge2Matcher Ge() { return internal::Ge2Matcher(); }



inline internal::Gt2Matcher Gt() { return internal::Gt2Matcher(); }



inline internal::Le2Matcher Le() { return internal::Le2Matcher(); }



inline internal::Lt2Matcher Lt() { return internal::Lt2Matcher(); }



inline internal::Ne2Matcher Ne() { return internal::Ne2Matcher(); }



template <typename InnerMatcher>
inline internal::NotMatcher<InnerMatcher> Not(InnerMatcher m) {
  return internal::NotMatcher<InnerMatcher>(m);
}




template <typename Predicate>
inline PolymorphicMatcher<internal::TrulyMatcher<Predicate> >
Truly(Predicate pred) {
  return MakePolymorphicMatcher(internal::TrulyMatcher<Predicate>(pred));
}





template <typename Container>
inline PolymorphicMatcher<internal::ContainerEqMatcher<  
                            GTEST_REMOVE_CONST_(Container)> >
    ContainerEq(const Container& rhs) {
  
  
  typedef GTEST_REMOVE_CONST_(Container) RawContainer;
  return MakePolymorphicMatcher(
      internal::ContainerEqMatcher<RawContainer>(rhs));
}







template <typename TupleMatcher, typename Container>
inline internal::PointwiseMatcher<TupleMatcher,
                                  GTEST_REMOVE_CONST_(Container)>
Pointwise(const TupleMatcher& tuple_matcher, const Container& rhs) {
  
  
  typedef GTEST_REMOVE_CONST_(Container) RawContainer;
  return internal::PointwiseMatcher<TupleMatcher, RawContainer>(
      tuple_matcher, rhs);
}



















template <typename M>
inline internal::ContainsMatcher<M> Contains(M matcher) {
  return internal::ContainsMatcher<M>(matcher);
}




























template <typename M>
inline internal::EachMatcher<M> Each(M matcher) {
  return internal::EachMatcher<M>(matcher);
}




template <typename M>
inline internal::KeyMatcher<M> Key(M inner_matcher) {
  return internal::KeyMatcher<M>(inner_matcher);
}






template <typename FirstMatcher, typename SecondMatcher>
inline internal::PairMatcher<FirstMatcher, SecondMatcher>
Pair(FirstMatcher first_matcher, SecondMatcher second_matcher) {
  return internal::PairMatcher<FirstMatcher, SecondMatcher>(
      first_matcher, second_matcher);
}



template <typename M>
inline internal::MatcherAsPredicate<M> Matches(M matcher) {
  return internal::MatcherAsPredicate<M>(matcher);
}


template <typename T, typename M>
inline bool Value(const T& value, M matcher) {
  return testing::Matches(matcher)(value);
}



template <typename T, typename M>
inline bool ExplainMatchResult(
    M matcher, const T& value, MatchResultListener* listener) {
  return SafeMatcherCast<const T&>(matcher).MatchAndExplain(value, listener);
}








template <typename InnerMatcher>
inline InnerMatcher AllArgs(const InnerMatcher& matcher) { return matcher; }





#define ASSERT_THAT(value, matcher) ASSERT_PRED_FORMAT1(\
    ::testing::internal::MakePredicateFormatterFromMatcher(matcher), value)
#define EXPECT_THAT(value, matcher) EXPECT_PRED_FORMAT1(\
    ::testing::internal::MakePredicateFormatterFromMatcher(matcher), value)

}  

#endif  
