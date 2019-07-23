



#ifndef BASE_NO_WINDOWS2000_UNITTEST_H_
#define BASE_NO_WINDOWS2000_UNITTEST_H_

#include "testing/gtest/include/gtest/gtest.h"
#include "base/win_util.h"



template<typename Parent>
class NoWindows2000Test : public Parent {
 public:
  static bool IsTestCaseDisabled() {
    return win_util::GetWinVersion() <= win_util::WINVERSION_2000;
  }
};

#endif  
