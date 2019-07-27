
































#ifndef GTEST_TEST_PRODUCTION_H_
#define GTEST_TEST_PRODUCTION_H_

#include "gtest/gtest_prod.h"

class PrivateCode {
 public:
  
  FRIEND_TEST(PrivateCodeTest, CanAccessPrivateMembers);

  
  FRIEND_TEST(PrivateCodeFixtureTest, CanAccessPrivateMembers);

  PrivateCode();

  int x() const { return x_; }
 private:
  void set_x(int an_x) { x_ = an_x; }
  int x_;
};

#endif  
