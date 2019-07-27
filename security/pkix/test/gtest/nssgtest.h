























#ifndef mozilla_pkix__nssgtest_h
#define mozilla_pkix__nssgtest_h

#include "stdint.h"
#include "gtest/gtest.h"
#include "pkix/pkixtypes.h"
#include "pkixtestutil.h"
#include "prerror.h"
#include "prtime.h"
#include "seccomon.h"

namespace mozilla { namespace pkix { namespace test {

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
  static mozilla::pkix::Time now;
  static PRTime pr_now;
  static PRTime pr_oneDayBeforeNow;
  static PRTime pr_oneDayAfterNow;
};

} } } 

#endif 
