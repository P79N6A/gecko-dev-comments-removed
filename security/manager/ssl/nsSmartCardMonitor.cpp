


#include "nspr.h"

#include "mozilla/Services.h"
#include "mozilla/unused.h"
#include "nsIObserverService.h"
#include "nsServiceManagerUtils.h"
#include "nsSmartCardMonitor.h"
#include "nsThreadUtils.h"
#include "pk11func.h"

using namespace mozilla;


















class nsTokenEventRunnable : public nsIRunnable {
public:
  nsTokenEventRunnable(const nsAString& aType, const nsAString& aTokenName)
    : mType(aType)
    , mTokenName(aTokenName)
  {
  }

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIRUNNABLE

private:
  virtual ~nsTokenEventRunnable() {}

  nsString mType;
  nsString mTokenName;
};

NS_IMPL_ISUPPORTS(nsTokenEventRunnable, nsIRunnable)

NS_IMETHODIMP
nsTokenEventRunnable::Run()
{
  MOZ_ASSERT(NS_IsMainThread());

  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (!observerService) {
    return NS_ERROR_FAILURE;
  }
  
  
  NS_ConvertUTF16toUTF8 eventTypeUTF8(mType);
  return observerService->NotifyObservers(nullptr, eventTypeUTF8.get(),
                                          mTokenName.get());
}



class SmartCardThreadEntry
{
public:
  friend class SmartCardThreadList;
  SmartCardThreadEntry(SmartCardMonitoringThread *thread,
                       SmartCardThreadEntry *next,
                       SmartCardThreadEntry *prev,
                       SmartCardThreadEntry **head)
    : next(next)
    , prev(prev)
    , head(head)
    , thread(thread)
  {
    if (prev) {
      prev->next = this;
    } else {
      *head = this;
    }
    if (next) {
      next->prev = this;
    }
  }

  ~SmartCardThreadEntry()
  {
    if (prev) {
      prev->next = next;
    } else {
      *head = next;
    }
    if (next) {
      next->prev = prev;
    }
    
    delete thread;
  }

private:
  SmartCardThreadEntry *next;
  SmartCardThreadEntry *prev;
  SmartCardThreadEntry **head;
  SmartCardMonitoringThread *thread;
};






SmartCardThreadList::SmartCardThreadList() : head(0)
{
}

SmartCardThreadList::~SmartCardThreadList()
{
  
  
  
  while (head) {
    delete head;
  }
}

void
SmartCardThreadList::Remove(SECMODModule *aModule)
{
  for (SmartCardThreadEntry* current = head; current;
       current = current->next) {
    if (current->thread->GetModule() == aModule) {
      
      delete current;
      return;
    }
  }
}


nsresult
SmartCardThreadList::Add(SmartCardMonitoringThread* thread)
{
  SmartCardThreadEntry* current = new SmartCardThreadEntry(thread, head,
                                                           nullptr, &head);
  
  unused << current;

  return thread->Start();
}



static PLHashNumber
unity(const void* key) { return PLHashNumber(NS_PTR_TO_INT32(key)); }

SmartCardMonitoringThread::SmartCardMonitoringThread(SECMODModule* module_)
  : mThread(nullptr)
{
  mModule = SECMOD_ReferenceModule(module_);
  
  
  mHash = PL_NewHashTable(10, unity, PL_CompareValues, PL_CompareStrings,
                          nullptr, 0);
}





SmartCardMonitoringThread::~SmartCardMonitoringThread()
{
  Stop();
  SECMOD_DestroyModule(mModule);
  if (mHash) {
    PL_HashTableDestroy(mHash);
  }
}

nsresult
SmartCardMonitoringThread::Start()
{
  if (!mThread) {
    mThread = PR_CreateThread(PR_SYSTEM_THREAD, LaunchExecute, this,
                              PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD,
                              PR_JOINABLE_THREAD, 0);
  }
  return mThread ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}







void SmartCardMonitoringThread::Stop()
{
  SECStatus rv;

  rv = SECMOD_CancelWait(mModule);
  if (rv != SECSuccess) {
    
    
    return;
  }

  
  
  
  
  
  
  if (mThread) {
    PR_JoinThread(mThread);
    mThread = 0;
  }
}









void
SmartCardMonitoringThread::SetTokenName(CK_SLOT_ID slotid,
                                       const char* tokenName, uint32_t series)
{
  if (mHash) {
    if (tokenName) {
      int len = strlen(tokenName) + 1;
      

      char* entry = (char*)PR_Malloc(len + sizeof(uint32_t));

      if (entry) {
        memcpy(entry, &series, sizeof(uint32_t));
        memcpy(&entry[sizeof(uint32_t)], tokenName, len);

        PL_HashTableAdd(mHash, (void*)(uintptr_t)slotid, entry); 
        return;
      }
    } else {
      
      PL_HashTableRemove(mHash, (void*)(uintptr_t)slotid);
    }
  }
}


const char*
SmartCardMonitoringThread::GetTokenName(CK_SLOT_ID slotid)
{
  const char* tokenName = nullptr;
  const char* entry;

  if (mHash) {
    entry = (const char*)PL_HashTableLookupConst(mHash,
                                                 (void*)(uintptr_t)slotid);
    if (entry) {
      tokenName = &entry[sizeof(uint32_t)];
    }
  }
  return tokenName;
}


uint32_t
SmartCardMonitoringThread::GetTokenSeries(CK_SLOT_ID slotid)
{
  uint32_t series = 0;
  const char* entry;

  if (mHash) {
    entry = (const char*)PL_HashTableLookupConst(mHash,
                                                 (void*)(uintptr_t)slotid);
    if (entry) {
      memcpy(&series, entry, sizeof(uint32_t));
    }
  }
  return series;
}




void
SmartCardMonitoringThread::SendEvent(const nsAString& eventType,
                                     const char* tokenName)
{
  
  
  
  
  
  nsAutoString tokenNameUTF16(NS_LITERAL_STRING(""));
  if (IsUTF8(nsDependentCString(tokenName))) {
    tokenNameUTF16.Assign(NS_ConvertUTF8toUTF16(tokenName));
  }
  nsCOMPtr<nsIRunnable> runnable(new nsTokenEventRunnable(eventType,
                                                          tokenNameUTF16));
  NS_DispatchToMainThread(runnable);
}




void SmartCardMonitoringThread::Execute()
{
  PK11SlotInfo* slot;
  const char* tokenName;

  
  
  
  PK11SlotList* sl = PK11_FindSlotsByNames(mModule->dllName, nullptr, nullptr,
                                           true);

  PK11SlotListElement* sle;
  if (sl) {
    for (sle = PK11_GetFirstSafe(sl); sle;
         sle = PK11_GetNextSafe(sl, sle, false)) {
      SetTokenName(PK11_GetSlotID(sle->slot), PK11_GetTokenName(sle->slot),
                   PK11_GetSlotSeries(sle->slot));
    }
    PK11_FreeSlotList(sl);
  }

  
  do {
    slot = SECMOD_WaitForAnyTokenEvent(mModule, 0, PR_SecondsToInterval(1));
    if (!slot) {
      break;
    }

    
    
    if (PK11_IsPresent(slot)) {
      
      CK_SLOT_ID slotID = PK11_GetSlotID(slot);
      uint32_t series = PK11_GetSlotSeries(slot);

      
      if (series != GetTokenSeries(slotID)) {
        
        
        tokenName = GetTokenName(slotID);
        if (tokenName) {
          SendEvent(NS_LITERAL_STRING("smartcard-remove"), tokenName);
        }
        tokenName = PK11_GetTokenName(slot);
        
        SetTokenName(slotID, tokenName, series);
        SendEvent(NS_LITERAL_STRING("smartcard-insert"), tokenName);
      }
    } else {
      
      CK_SLOT_ID slotID = PK11_GetSlotID(slot);
      tokenName = GetTokenName(slotID);
      
      
      if (tokenName) {
        SendEvent(NS_LITERAL_STRING("smartcard-remove"), tokenName);
        
        SetTokenName(slotID, nullptr, 0);
      }
    }
    PK11_FreeSlot(slot);

  } while (1);
}


const SECMODModule* SmartCardMonitoringThread::GetModule()
{
  return mModule;
}


void SmartCardMonitoringThread::LaunchExecute(void* arg)
{
  PR_SetCurrentThreadName("SmartCard");

  ((SmartCardMonitoringThread*)arg)->Execute();
}
