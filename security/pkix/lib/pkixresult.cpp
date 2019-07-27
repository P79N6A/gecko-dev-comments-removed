























#include "pkix/Result.h"

#include "pkix/nullptr.h"

namespace mozilla { namespace pkix {

const char*
MapResultToName(Result result)
{
  switch (result)
  {
#define MOZILLA_PKIX_MAP(mozilla_pkix_result, nss_result) \
    case mozilla_pkix_result: return #mozilla_pkix_result;

    MOZILLA_PKIX_MAP_LIST

#undef MOZILLA_PKIX_MAP

    default:
      assert(false);
      return nullptr;
  }
}

} } 
