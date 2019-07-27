









#ifndef WEBRTC_BASE_GUNIT_PROD_H_
#define WEBRTC_BASE_GUNIT_PROD_H_

#if defined(WEBRTC_ANDROID)


#define NO_GTEST
#elif defined (GTEST_RELATIVE_PATH)
#include "gtest/gtest_prod.h"
#else
#include "testing/base/gunit_prod.h"
#endif

#endif  
