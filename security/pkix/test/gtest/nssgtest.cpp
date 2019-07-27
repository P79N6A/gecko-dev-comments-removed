























#include "nssgtest.h"
#include "nss.h"
#include "pkixtestutil.h"

using namespace std;
using namespace testing;

namespace mozilla { namespace pkix { namespace test {

 void
NSSTest::SetUpTestCase()
{
  if (NSS_NoDB_Init(nullptr) != SECSuccess) {
    abort();
  }
}

NSSTest::NSSTest()
  : arena(PORT_NewArena(DER_DEFAULT_CHUNKSIZE))
{
  if (!arena) {
    abort();
  }
}



const std::time_t now(time(nullptr));
const std::time_t oneDayBeforeNow(time(nullptr) - Time::ONE_DAY_IN_SECONDS);
const std::time_t oneDayAfterNow(time(nullptr) + Time::ONE_DAY_IN_SECONDS);

} } } 
