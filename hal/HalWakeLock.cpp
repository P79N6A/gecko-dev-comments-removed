




#include "Hal.h"
#include "mozilla/HalWakeLock.h"
#include "mozilla/Services.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/dom/ContentParent.h"
#include "nsClassHashtable.h"
#include "nsDataHashtable.h"
#include "nsHashKeys.h"
#include "nsIPropertyBag2.h"
#include "nsIObserverService.h"

using namespace mozilla;
using namespace mozilla::hal;

namespace {

struct LockCount {
  LockCount()
    : numLocks(0)
    , numHidden(0)
  {}
  uint32_t numLocks;
  uint32_t numHidden;
  nsTArray<uint64_t> processes;
};

typedef nsDataHashtable<nsUint64HashKey, LockCount> ProcessLockTable;
typedef nsClassHashtable<nsStringHashKey, ProcessLockTable> LockTable;

int sActiveListeners = 0;
StaticAutoPtr<LockTable> sLockTable;
bool sInitialized = false;
bool sIsShuttingDown = false;

WakeLockInformation
WakeLockInfoFromLockCount(const nsAString& aTopic, const LockCount& aLockCount)
{
  
  
  
  
  

  WakeLockInformation info;
  info.topic() = aTopic;
  info.numLocks() = aLockCount.numLocks;
  info.numHidden() = aLockCount.numHidden;
  info.lockingProcesses().AppendElements(aLockCount.processes);
  return info;
}

PLDHashOperator
CountWakeLocks(const uint64_t& aKey, LockCount aCount, void* aUserArg)
{
  MOZ_ASSERT(aUserArg);

  LockCount* totalCount = static_cast<LockCount*>(aUserArg);
  totalCount->numLocks += aCount.numLocks;
  totalCount->numHidden += aCount.numHidden;

  
  if (!totalCount->processes.Contains(aKey)) {
    totalCount->processes.AppendElement(aKey);
  }

  return PL_DHASH_NEXT;
}

static PLDHashOperator
RemoveChildFromList(const nsAString& aKey, nsAutoPtr<ProcessLockTable>& aTable,
                    void* aUserArg)
{
  MOZ_ASSERT(aUserArg);

  PLDHashOperator op = PL_DHASH_NEXT;
  uint64_t childID = *static_cast<uint64_t*>(aUserArg);
  if (aTable->Get(childID, nullptr)) {
    aTable->Remove(childID);

    LockCount totalCount;
    aTable->EnumerateRead(CountWakeLocks, &totalCount);
    if (!totalCount.numLocks) {
      op = PL_DHASH_REMOVE;
    }

    if (sActiveListeners) {
      NotifyWakeLockChange(WakeLockInfoFromLockCount(aKey, totalCount));
    }
  }

  return op;
}

class ClearHashtableOnShutdown final : public nsIObserver {
  ~ClearHashtableOnShutdown() {}
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
};

NS_IMPL_ISUPPORTS(ClearHashtableOnShutdown, nsIObserver)

NS_IMETHODIMP
ClearHashtableOnShutdown::Observe(nsISupports* aSubject, const char* aTopic, const char16_t* data)
{
  MOZ_ASSERT(!strcmp(aTopic, "xpcom-shutdown"));

  sIsShuttingDown = true;
  sLockTable = nullptr;

  return NS_OK;
}

class CleanupOnContentShutdown final : public nsIObserver {
  ~CleanupOnContentShutdown() {}
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
};

NS_IMPL_ISUPPORTS(CleanupOnContentShutdown, nsIObserver)

NS_IMETHODIMP
CleanupOnContentShutdown::Observe(nsISupports* aSubject, const char* aTopic, const char16_t* data)
{
  MOZ_ASSERT(!strcmp(aTopic, "ipc:content-shutdown"));

  if (sIsShuttingDown) {
    return NS_OK;
  }

  nsCOMPtr<nsIPropertyBag2> props = do_QueryInterface(aSubject);
  if (!props) {
    NS_WARNING("ipc:content-shutdown message without property bag as subject");
    return NS_OK;
  }

  uint64_t childID = 0;
  nsresult rv = props->GetPropertyAsUint64(NS_LITERAL_STRING("childID"),
                                           &childID);
  if (NS_SUCCEEDED(rv)) {
    sLockTable->Enumerate(RemoveChildFromList, &childID);
  } else {
    NS_WARNING("ipc:content-shutdown message without childID property");
  }
  return NS_OK;
}

void
Init()
{
  sLockTable = new LockTable();
  sInitialized = true;

  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (obs) {
    obs->AddObserver(new ClearHashtableOnShutdown(), "xpcom-shutdown", false);
    obs->AddObserver(new CleanupOnContentShutdown(), "ipc:content-shutdown", false);
  }
}

} 

namespace mozilla {

namespace hal {

WakeLockState
ComputeWakeLockState(int aNumLocks, int aNumHidden)
{
  if (aNumLocks == 0) {
    return WAKE_LOCK_STATE_UNLOCKED;
  } else if (aNumLocks == aNumHidden) {
    return WAKE_LOCK_STATE_HIDDEN;
  } else {
    return WAKE_LOCK_STATE_VISIBLE;
  }
}

} 

namespace hal_impl {

void
EnableWakeLockNotifications()
{
  sActiveListeners++;
}

void
DisableWakeLockNotifications()
{
  sActiveListeners--;
}

void
ModifyWakeLock(const nsAString& aTopic,
               hal::WakeLockControl aLockAdjust,
               hal::WakeLockControl aHiddenAdjust,
               uint64_t aProcessID)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aProcessID != CONTENT_PROCESS_ID_UNKNOWN);

  if (sIsShuttingDown) {
    return;
  }
  if (!sInitialized) {
    Init();
  }

  ProcessLockTable* table = sLockTable->Get(aTopic);
  LockCount processCount;
  LockCount totalCount;
  if (!table) {
    table = new ProcessLockTable();
    sLockTable->Put(aTopic, table);
  } else {
    table->Get(aProcessID, &processCount);
    table->EnumerateRead(CountWakeLocks, &totalCount);
  }

  MOZ_ASSERT(processCount.numLocks >= processCount.numHidden);
  MOZ_ASSERT(aLockAdjust >= 0 || processCount.numLocks > 0);
  MOZ_ASSERT(aHiddenAdjust >= 0 || processCount.numHidden > 0);
  MOZ_ASSERT(totalCount.numLocks >= totalCount.numHidden);
  MOZ_ASSERT(aLockAdjust >= 0 || totalCount.numLocks > 0);
  MOZ_ASSERT(aHiddenAdjust >= 0 || totalCount.numHidden > 0);

  WakeLockState oldState = ComputeWakeLockState(totalCount.numLocks, totalCount.numHidden);
  bool processWasLocked = processCount.numLocks > 0;

  processCount.numLocks += aLockAdjust;
  processCount.numHidden += aHiddenAdjust;

  totalCount.numLocks += aLockAdjust;
  totalCount.numHidden += aHiddenAdjust;

  if (processCount.numLocks) {
    table->Put(aProcessID, processCount);
  } else {
    table->Remove(aProcessID);
  }
  if (!totalCount.numLocks) {
    sLockTable->Remove(aTopic);
  }

  if (sActiveListeners &&
      (oldState != ComputeWakeLockState(totalCount.numLocks,
                                        totalCount.numHidden) ||
       processWasLocked != (processCount.numLocks > 0))) {

    WakeLockInformation info;
    hal::GetWakeLockInfo(aTopic, &info);
    NotifyWakeLockChange(info);
  }
}

void
GetWakeLockInfo(const nsAString& aTopic, WakeLockInformation* aWakeLockInfo)
{
  if (sIsShuttingDown) {
    NS_WARNING("You don't want to get wake lock information during xpcom-shutdown!");
    *aWakeLockInfo = WakeLockInformation();
    return;
  }
  if (!sInitialized) {
    Init();
  }

  ProcessLockTable* table = sLockTable->Get(aTopic);
  if (!table) {
    *aWakeLockInfo = WakeLockInfoFromLockCount(aTopic, LockCount());
    return;
  }
  LockCount totalCount;
  table->EnumerateRead(CountWakeLocks, &totalCount);
  *aWakeLockInfo = WakeLockInfoFromLockCount(aTopic, totalCount);
}

} 
} 
