






























#include "gtest/gtest-typed-test.h"
#include "gtest/gtest.h"

namespace testing {
namespace internal {

#if GTEST_HAS_TYPED_TEST_P



static const char* SkipSpaces(const char* str) {
  while (IsSpace(*str))
    str++;
  return str;
}




const char* TypedTestCasePState::VerifyRegisteredTestNames(
    const char* file, int line, const char* registered_tests) {
  typedef ::std::set<const char*>::const_iterator DefinedTestIter;
  registered_ = true;

  
  
  registered_tests = SkipSpaces(registered_tests);

  Message errors;
  ::std::set<std::string> tests;
  for (const char* names = registered_tests; names != NULL;
       names = SkipComma(names)) {
    const std::string name = GetPrefixUntilComma(names);
    if (tests.count(name) != 0) {
      errors << "Test " << name << " is listed more than once.\n";
      continue;
    }

    bool found = false;
    for (DefinedTestIter it = defined_test_names_.begin();
         it != defined_test_names_.end();
         ++it) {
      if (name == *it) {
        found = true;
        break;
      }
    }

    if (found) {
      tests.insert(name);
    } else {
      errors << "No test named " << name
             << " can be found in this test case.\n";
    }
  }

  for (DefinedTestIter it = defined_test_names_.begin();
       it != defined_test_names_.end();
       ++it) {
    if (tests.count(*it) == 0) {
      errors << "You forgot to list test " << *it << ".\n";
    }
  }

  const std::string& errors_str = errors.GetString();
  if (errors_str != "") {
    fprintf(stderr, "%s %s", FormatFileLocation(file, line).c_str(),
            errors_str.c_str());
    fflush(stderr);
    posix::Abort();
  }

  return registered_tests;
}

#endif  

}  
}  
