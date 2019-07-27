























#include "pkix/Result.h"
#include "pkixutil.h"

namespace mozilla { namespace pkix {

const char*
MapResultToName(Result result)
{
  switch (result)
  {
#define MOZILLA_PKIX_MAP(mozilla_pkix_result, value, nss_result) \
    case Result::mozilla_pkix_result: return "Result::" #mozilla_pkix_result;

    MOZILLA_PKIX_MAP_LIST

#undef MOZILLA_PKIX_MAP

    MOZILLA_PKIX_UNREACHABLE_DEFAULT_ENUM
  }
}

} } 
