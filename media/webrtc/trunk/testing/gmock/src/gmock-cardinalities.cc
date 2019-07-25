


































#include "gmock/gmock-cardinalities.h"

#include <limits.h>
#include <ostream>  
#include <sstream>
#include <string>
#include "gmock/internal/gmock-internal-utils.h"
#include "gtest/gtest.h"

namespace testing {

namespace {


class BetweenCardinalityImpl : public CardinalityInterface {
 public:
  BetweenCardinalityImpl(int min, int max)
      : min_(min >= 0 ? min : 0),
        max_(max >= min_ ? max : min_) {
    std::stringstream ss;
    if (min < 0) {
      ss << "The invocation lower bound must be >= 0, "
         << "but is actually " << min << ".";
      internal::Expect(false, __FILE__, __LINE__, ss.str());
    } else if (max < 0) {
      ss << "The invocation upper bound must be >= 0, "
         << "but is actually " << max << ".";
      internal::Expect(false, __FILE__, __LINE__, ss.str());
    } else if (min > max) {
      ss << "The invocation upper bound (" << max
         << ") must be >= the invocation lower bound (" << min
         << ").";
      internal::Expect(false, __FILE__, __LINE__, ss.str());
    }
  }

  
  
  virtual int ConservativeLowerBound() const { return min_; }
  virtual int ConservativeUpperBound() const { return max_; }

  virtual bool IsSatisfiedByCallCount(int call_count) const {
    return min_ <= call_count && call_count <= max_ ;
  }

  virtual bool IsSaturatedByCallCount(int call_count) const {
    return call_count >= max_;
  }

  virtual void DescribeTo(::std::ostream* os) const;
 private:
  const int min_;
  const int max_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(BetweenCardinalityImpl);
};


inline internal::string FormatTimes(int n) {
  if (n == 1) {
    return "once";
  } else if (n == 2) {
    return "twice";
  } else {
    std::stringstream ss;
    ss << n << " times";
    return ss.str();
  }
}


void BetweenCardinalityImpl::DescribeTo(::std::ostream* os) const {
  if (min_ == 0) {
    if (max_ == 0) {
      *os << "never called";
    } else if (max_ == INT_MAX) {
      *os << "called any number of times";
    } else {
      *os << "called at most " << FormatTimes(max_);
    }
  } else if (min_ == max_) {
    *os << "called " << FormatTimes(min_);
  } else if (max_ == INT_MAX) {
    *os << "called at least " << FormatTimes(min_);
  } else {
    
    *os << "called between " << min_ << " and " << max_ << " times";
  }
}

}  


void Cardinality::DescribeActualCallCountTo(int actual_call_count,
                                            ::std::ostream* os) {
  if (actual_call_count > 0) {
    *os << "called " << FormatTimes(actual_call_count);
  } else {
    *os << "never called";
  }
}


Cardinality AtLeast(int n) { return Between(n, INT_MAX); }


Cardinality AtMost(int n) { return Between(0, n); }


Cardinality AnyNumber() { return AtLeast(0); }


Cardinality Between(int min, int max) {
  return Cardinality(new BetweenCardinalityImpl(min, max));
}


Cardinality Exactly(int n) { return Between(n, n); }

}  
