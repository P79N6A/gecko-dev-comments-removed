











































#include "TestHarness.h"
#include "jemalloc.h"

static inline bool
TestOne(size_t size)
{
    size_t req = size;
    size_t adv = je_malloc_usable_size_in_advance(req);
    char* p = (char*)malloc(req);
    size_t usable = moz_malloc_usable_size(p);
    if (adv != usable) {
      fail("je_malloc_usable_size_in_advance(%d) --> %d; "
           "malloc_usable_size(%d) --> %d",
           req, adv, req, usable);
      return false;
    }
    free(p);
    return true;
}

static inline bool
TestThree(size_t size)
{
    return TestOne(size - 1) && TestOne(size) && TestOne(size + 1);
}

static nsresult
TestJemallocUsableSizeInAdvance()
{
  #define K   * 1024
  #define M   * 1024 * 1024

  




  for (size_t n = 0; n < 16 K; n++)
    if (!TestOne(n))
      return NS_ERROR_UNEXPECTED;

  for (size_t n = 16 K; n < 1 M; n += 4 K)
    if (!TestThree(n))
      return NS_ERROR_UNEXPECTED;

  for (size_t n = 1 M; n < 8 M; n += 128 K)
    if (!TestThree(n))
      return NS_ERROR_UNEXPECTED;

  passed("je_malloc_usable_size_in_advance");

  return NS_OK;
}

int main(int argc, char** argv)
{
  int rv = 0;
  ScopedXPCOM xpcom("jemalloc");
  if (xpcom.failed())
      return 1;

  if (NS_FAILED(TestJemallocUsableSizeInAdvance()))
    rv = 1;

  return rv;
}

