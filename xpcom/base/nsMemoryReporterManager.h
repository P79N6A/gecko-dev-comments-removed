





#ifndef nsMemoryReporterManager_h__
#define nsMemoryReporterManager_h__

#include "nsIMemoryReporter.h"
#include "nsITimer.h"
#include "nsServiceManagerUtils.h"
#include "mozilla/Mutex.h"
#include "nsTHashtable.h"
#include "nsHashKeys.h"

class nsITimer;

namespace mozilla {
namespace dom {
class MemoryReport;
}
}

class nsMemoryReporterManager : public nsIMemoryReporterManager
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIMEMORYREPORTERMANAGER

  nsMemoryReporterManager();
  virtual ~nsMemoryReporterManager();

  
  static nsMemoryReporterManager* GetOrCreate()
  {
    nsCOMPtr<nsIMemoryReporterManager> imgr =
      do_GetService("@mozilla.org/memory-reporter-manager;1");
    return static_cast<nsMemoryReporterManager*>(imgr.get());
  }

  typedef nsTHashtable<nsRefPtrHashKey<nsIMemoryReporter> > StrongReportersTable;
  typedef nsTHashtable<nsPtrHashKey<nsIMemoryReporter> > WeakReportersTable;

  void IncrementNumChildProcesses();
  void DecrementNumChildProcesses();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  void HandleChildReports(
    const uint32_t& generation,
    const InfallibleTArray<mozilla::dom::MemoryReport>& aChildReports);
  nsresult FinishReporting();

  
  
  struct AmountFns
  {
    mozilla::InfallibleAmountFn mJSMainRuntimeGCHeap;
    mozilla::InfallibleAmountFn mJSMainRuntimeTemporaryPeak;
    mozilla::InfallibleAmountFn mJSMainRuntimeCompartmentsSystem;
    mozilla::InfallibleAmountFn mJSMainRuntimeCompartmentsUser;

    mozilla::InfallibleAmountFn mImagesContentUsedUncompressed;

    mozilla::InfallibleAmountFn mStorageSQLite;

    mozilla::InfallibleAmountFn mLowMemoryEventsVirtual;
    mozilla::InfallibleAmountFn mLowMemoryEventsPhysical;

    mozilla::InfallibleAmountFn mGhostWindows;

    AmountFns() { mozilla::PodZero(this); }
  };
  AmountFns mAmountFns;

  
  
  static int64_t ResidentFast();

  
  struct SizeOfTabFns
  {
    mozilla::JSSizeOfTabFn    mJS;
    mozilla::NonJSSizeOfTabFn mNonJS;

    SizeOfTabFns()
    {
      mozilla::PodZero(this);
    }
  };
  SizeOfTabFns mSizeOfTabFns;

private:
  nsresult RegisterReporterHelper(nsIMemoryReporter* aReporter,
                                  bool aForce, bool aStrongRef);
  nsresult StartGettingReports();

  static void TimeoutCallback(nsITimer* aTimer, void* aData);
  
  
  static const uint32_t kTimeoutLengthMS = 50000;

  mozilla::Mutex mMutex;
  bool mIsRegistrationBlocked;

  StrongReportersTable* mStrongReporters;
  WeakReportersTable* mWeakReporters;

  
  StrongReportersTable* mSavedStrongReporters;
  WeakReportersTable* mSavedWeakReporters;

  uint32_t mNumChildProcesses;
  uint32_t mNextGeneration;

  struct GetReportsState
  {
    uint32_t                             mGeneration;
    nsCOMPtr<nsITimer>                   mTimer;
    uint32_t                             mNumChildProcesses;
    uint32_t                             mNumChildProcessesCompleted;
    nsCOMPtr<nsIHandleReportCallback>    mHandleReport;
    nsCOMPtr<nsISupports>                mHandleReportData;
    nsCOMPtr<nsIFinishReportingCallback> mFinishReporting;
    nsCOMPtr<nsISupports>                mFinishReportingData;
    nsString                             mDMDDumpIdent;

    GetReportsState(uint32_t aGeneration, nsITimer* aTimer,
                    uint32_t aNumChildProcesses,
                    nsIHandleReportCallback* aHandleReport,
                    nsISupports* aHandleReportData,
                    nsIFinishReportingCallback* aFinishReporting,
                    nsISupports* aFinishReportingData,
                    const nsAString &aDMDDumpIdent)
      : mGeneration(aGeneration)
      , mTimer(aTimer)
      , mNumChildProcesses(aNumChildProcesses)
      , mNumChildProcessesCompleted(0)
      , mHandleReport(aHandleReport)
      , mHandleReportData(aHandleReportData)
      , mFinishReporting(aFinishReporting)
      , mFinishReportingData(aFinishReportingData)
      , mDMDDumpIdent(aDMDDumpIdent)
    {
    }
  };

  
  
  
  GetReportsState* mGetReportsState;
};

#define NS_MEMORY_REPORTER_MANAGER_CID \
{ 0xfb97e4f5, 0x32dd, 0x497a, \
{ 0xba, 0xa2, 0x7d, 0x1e, 0x55, 0x7, 0x99, 0x10 } }

#endif
