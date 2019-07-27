























#ifndef mozilla_pkix__nssgtest_h
#define mozilla_pkix__nssgtest_h

#include "gtest/gtest.h"
#include "pkix/pkixtypes.h"
#include "pkixtestutil.h"

namespace mozilla { namespace pkix { namespace test {

extern const std::time_t now;
extern const std::time_t oneDayBeforeNow;
extern const std::time_t oneDayAfterNow;

class NSSTest : public ::testing::Test
{
public:
  static void SetUpTestCase();
};

} } } 

#endif 
