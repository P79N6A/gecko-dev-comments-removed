





#include "nsIMemoryReporter.h"
#include "mozilla/Mutex.h"
#include "nsTHashtable.h"
#include "nsHashKeys.h"

using mozilla::Mutex;

class nsMemoryReporterManager : public nsIMemoryReporterManager
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIMEMORYREPORTERMANAGER

  nsMemoryReporterManager();
  virtual ~nsMemoryReporterManager();

private:
  nsresult RegisterReporterHelper(nsIMemoryReporter *reporter, bool aForce);

  nsTHashtable<nsISupportsHashKey> mReporters;
  Mutex mMutex;
  bool mIsRegistrationBlocked;
};

#define NS_MEMORY_REPORTER_MANAGER_CID \
{ 0xfb97e4f5, 0x32dd, 0x497a, \
{ 0xba, 0xa2, 0x7d, 0x1e, 0x55, 0x7, 0x99, 0x10 } }
