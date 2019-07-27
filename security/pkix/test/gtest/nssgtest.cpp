























#include "nssgtest.h"
#include "nss.h"
#include "pkixtestutil.h"
#include "prinit.h"

using namespace std;
using namespace testing;

namespace mozilla { namespace pkix { namespace test {

 void
NSSTest::SetUpTestCase()
{
  if (NSS_NoDB_Init(nullptr) != SECSuccess) {
    abort();
  }

  now = Now();
  pr_now = PR_Now();
  pr_oneDayBeforeNow = pr_now - ONE_DAY;
  pr_oneDayAfterNow = pr_now + ONE_DAY;
}

NSSTest::NSSTest()
  : arena(PORT_NewArena(DER_DEFAULT_CHUNKSIZE))
{
  if (!arena) {
    abort();
  }
}

 mozilla::pkix::Time NSSTest::now(Now());
 PRTime NSSTest::pr_now;
 PRTime NSSTest::pr_oneDayBeforeNow;
 PRTime NSSTest::pr_oneDayAfterNow;

} } } 
