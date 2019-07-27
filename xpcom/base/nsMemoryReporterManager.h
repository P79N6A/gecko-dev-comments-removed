





#ifndef nsMemoryReporterManager_h__
#define nsMemoryReporterManager_h__

#include "mozilla/Mutex.h"
#include "nsHashKeys.h"
#include "nsIMemoryReporter.h"
#include "nsITimer.h"
#include "nsServiceManagerUtils.h"
#include "nsTHashtable.h"

namespace mozilla {
namespace dom {
class ContentParent;
class MemoryReport;
} 
} 

class nsITimer;

class nsMemoryReporterManager final : public nsIMemoryReporterManager
{
  virtual ~nsMemoryReporterManager();

public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIMEMORYREPORTERMANAGER

  nsMemoryReporterManager();

  
  static nsMemoryReporterManager* GetOrCreate()
  {
    nsCOMPtr<nsIMemoryReporterManager> imgr =
      do_GetService("@mozilla.org/memory-reporter-manager;1");
    return static_cast<nsMemoryReporterManager*>(imgr.get());
  }

  typedef nsTHashtable<nsRefPtrHashKey<nsIMemoryReporter>> StrongReportersTable;
  typedef nsTHashtable<nsPtrHashKey<nsIMemoryReporter>> WeakReportersTable;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  void HandleChildReport(uint32_t aGeneration,
                         const mozilla::dom::MemoryReport& aChildReport);
  void EndProcessReport(uint32_t aGeneration, bool aSuccess);

  
  
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

    AmountFns()
    {
      mozilla::PodZero(this);
    }
  };
  AmountFns mAmountFns;

  
  
  static int64_t ResidentFast();

  
  static int64_t ResidentPeak();

  
  
  static int64_t ResidentUnique();

  
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
  nsresult FinishReporting();

  static void TimeoutCallback(nsITimer* aTimer, void* aData);
  
  
  static const uint32_t kTimeoutLengthMS = 50000;

  mozilla::Mutex mMutex;
  bool mIsRegistrationBlocked;

  StrongReportersTable* mStrongReporters;
  WeakReportersTable* mWeakReporters;

  
  StrongReportersTable* mSavedStrongReporters;
  WeakReportersTable* mSavedWeakReporters;

  uint32_t mNextGeneration;

  struct GetReportsState
  {
    uint32_t                             mGeneration;
    bool                                 mAnonymize;
    bool                                 mMinimize;
    nsCOMPtr<nsITimer>                   mTimer;
    
    
    
    nsTArray<nsRefPtr<mozilla::dom::ContentParent>>* mChildrenPending;
    uint32_t                             mNumProcessesRunning;
    uint32_t                             mNumProcessesCompleted;
    uint32_t                             mConcurrencyLimit;
    nsCOMPtr<nsIHandleReportCallback>    mHandleReport;
    nsCOMPtr<nsISupports>                mHandleReportData;
    nsCOMPtr<nsIFinishReportingCallback> mFinishReporting;
    nsCOMPtr<nsISupports>                mFinishReportingData;
    nsString                             mDMDDumpIdent;

    GetReportsState(uint32_t aGeneration, bool aAnonymize, bool aMinimize,
                    uint32_t aConcurrencyLimit,
                    nsIHandleReportCallback* aHandleReport,
                    nsISupports* aHandleReportData,
                    nsIFinishReportingCallback* aFinishReporting,
                    nsISupports* aFinishReportingData,
                    const nsAString& aDMDDumpIdent)
      : mGeneration(aGeneration)
      , mAnonymize(aAnonymize)
      , mMinimize(aMinimize)
      , mChildrenPending(nullptr)
      , mNumProcessesRunning(1) 
      , mNumProcessesCompleted(0)
      , mConcurrencyLimit(aConcurrencyLimit)
      , mHandleReport(aHandleReport)
      , mHandleReportData(aHandleReportData)
      , mFinishReporting(aFinishReporting)
      , mFinishReportingData(aFinishReportingData)
      , mDMDDumpIdent(aDMDDumpIdent)
    {
    }

    ~GetReportsState();
  };

  
  
  
  GetReportsState* mGetReportsState;

  GetReportsState* GetStateForGeneration(uint32_t aGeneration);
  static bool StartChildReport(mozilla::dom::ContentParent* aChild,
                               const GetReportsState* aState);
};

#define NS_MEMORY_REPORTER_MANAGER_CID \
{ 0xfb97e4f5, 0x32dd, 0x497a, \
{ 0xba, 0xa2, 0x7d, 0x1e, 0x55, 0x7, 0x99, 0x10 } }

#endif 
