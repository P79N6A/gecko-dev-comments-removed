









#ifndef WEBRTC_TEST_TESTSUPPORT_GTEST_PROD_UTIL_H_
#define WEBRTC_TEST_TESTSUPPORT_GTEST_PROD_UTIL_H_
#pragma once



#define FRIEND_TEST_WEBRTC(test_case_name, test_name)\
friend class test_case_name##_##test_name##_Test














#define FRIEND_TEST_ALL_PREFIXES(test_case_name, test_name) \
  FRIEND_TEST_WEBRTC(test_case_name, test_name); \
  FRIEND_TEST_WEBRTC(test_case_name, DISABLED_##test_name); \
  FRIEND_TEST_WEBRTC(test_case_name, FLAKY_##test_name); \
  FRIEND_TEST_WEBRTC(test_case_name, FAILS_##test_name)

#endif  
