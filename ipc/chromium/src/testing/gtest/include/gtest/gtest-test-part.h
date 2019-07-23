































#ifndef GTEST_INCLUDE_GTEST_GTEST_TEST_PART_H_
#define GTEST_INCLUDE_GTEST_GTEST_TEST_PART_H_

#include <iosfwd>
#include <gtest/internal/gtest-internal.h>
#include <gtest/internal/gtest-string.h>

namespace testing {



enum TestPartResultType {
  TPRT_SUCCESS,           
  TPRT_NONFATAL_FAILURE,  
  TPRT_FATAL_FAILURE      
};





class TestPartResult {
 public:
  
  
  
  TestPartResult(TestPartResultType type,
                 const char* file_name,
                 int line_number,
                 const char* message)
      : type_(type),
        file_name_(file_name),
        line_number_(line_number),
        summary_(ExtractSummary(message)),
        message_(message) {
  }

  
  TestPartResultType type() const { return type_; }

  
  
  const char* file_name() const { return file_name_.c_str(); }

  
  
  int line_number() const { return line_number_; }

  
  const char* summary() const { return summary_.c_str(); }

  
  const char* message() const { return message_.c_str(); }

  
  bool passed() const { return type_ == TPRT_SUCCESS; }

  
  bool failed() const { return type_ != TPRT_SUCCESS; }

  
  bool nonfatally_failed() const { return type_ == TPRT_NONFATAL_FAILURE; }

  
  bool fatally_failed() const { return type_ == TPRT_FATAL_FAILURE; }
 private:
  TestPartResultType type_;

  
  
  static internal::String ExtractSummary(const char* message);

  
  
  internal::String file_name_;
  
  
  int line_number_;
  internal::String summary_;  
  internal::String message_;  
};


std::ostream& operator<<(std::ostream& os, const TestPartResult& result);








class TestPartResultArray {
 public:
  TestPartResultArray();
  ~TestPartResultArray();

  
  void Append(const TestPartResult& result);

  
  const TestPartResult& GetTestPartResult(int index) const;

  
  int size() const;
 private:
  
  
  internal::List<TestPartResult>* const list_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(TestPartResultArray);
};


class TestPartResultReporterInterface {
 public:
  virtual ~TestPartResultReporterInterface() {}

  virtual void ReportTestPartResult(const TestPartResult& result) = 0;
};

namespace internal {







class HasNewFatalFailureHelper : public TestPartResultReporterInterface {
 public:
  HasNewFatalFailureHelper();
  virtual ~HasNewFatalFailureHelper();
  virtual void ReportTestPartResult(const TestPartResult& result);
  bool has_new_fatal_failure() const { return has_new_fatal_failure_; }
 private:
  bool has_new_fatal_failure_;
  TestPartResultReporterInterface* original_reporter_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(HasNewFatalFailureHelper);
};

}  

}  

#endif  
