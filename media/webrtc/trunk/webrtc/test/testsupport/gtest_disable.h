








#ifndef TEST_TESTSUPPORT_INCLUDE_GTEST_DISABLE_H_
#define TEST_TESTSUPPORT_INCLUDE_GTEST_DISABLE_H_









#ifdef WEBRTC_LINUX
#define DISABLED_ON_LINUX(test) DISABLED_##test
#else
#define DISABLED_ON_LINUX(test) test
#endif

#ifdef WEBRTC_MAC
#define DISABLED_ON_MAC(test) DISABLED_##test
#else
#define DISABLED_ON_MAC(test) test
#endif

#ifdef _WIN32
#define DISABLED_ON_WIN(test) DISABLED_##test
#else
#define DISABLED_ON_WIN(test) test
#endif

#ifdef WEBRTC_ANDROID
#define DISABLED_ON_ANDROID(test) DISABLED_##test
#else
#define DISABLED_ON_ANDROID(test) test
#endif

#endif  
