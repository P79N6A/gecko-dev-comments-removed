









#ifndef WEBRTC_TEST_TESTSUPPORT_GTEST_PROD_UTIL_H_
#define WEBRTC_TEST_TESTSUPPORT_GTEST_PROD_UTIL_H_
#pragma once

#include "gtest/gtest_prod.h"














#define FRIEND_TEST_ALL_PREFIXES(test_case_name, test_name) \
  FRIEND_TEST(test_case_name, test_name); \
  FRIEND_TEST(test_case_name, DISABLED_##test_name); \
  FRIEND_TEST(test_case_name, FLAKY_##test_name); \
  FRIEND_TEST(test_case_name, FAILS_##test_name)

#endif  
