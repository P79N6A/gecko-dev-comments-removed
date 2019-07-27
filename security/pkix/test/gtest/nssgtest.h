























#ifndef mozilla_pkix__nssgtest_h
#define mozilla_pkix__nssgtest_h

#include "gtest/gtest.h"
#include "pkix/pkixtypes.h"
#include "pkixtestutil.h"

namespace mozilla { namespace pkix { namespace test {

extern const std::time_t now;
extern const std::time_t oneDayBeforeNow;
extern const std::time_t oneDayAfterNow;

inline void
PORT_FreeArena_false(PLArenaPool* arena)
{
  
  
  return PORT_FreeArena(arena, PR_FALSE);
}

typedef ScopedPtr<PLArenaPool, PORT_FreeArena_false> ScopedPLArenaPool;

class NSSTest : public ::testing::Test
{
public:
  static void SetUpTestCase();

protected:
  NSSTest();

  ScopedPLArenaPool arena;
};

} } } 

#endif 
