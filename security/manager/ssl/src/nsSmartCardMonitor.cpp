


































#include "nspr.h"

#include "pk11func.h"
#include "nsNSSComponent.h"
#include "nsSmartCardMonitor.h"
#include "nsSmartCardEvent.h"

















static NS_DEFINE_CID(kNSSComponentCID, NS_NSSCOMPONENT_CID);

#include <assert.h>



class SmartCardThreadEntry {
public:
 SmartCardThreadEntry *next;
 SmartCardThreadEntry *prev;
 SmartCardThreadEntry **head;
 SmartCardMonitoringThread *thread;
 SmartCardThreadEntry(SmartCardMonitoringThread *thread_,
   SmartCardThreadEntry *next_, SmartCardThreadEntry *prev_,
   SmartCardThreadEntry **head_) : 
   next(next_), prev(prev_), head(head_), thread(thread_) { 
    if (prev) { prev->next = this; } else { *head = this; }
    if (next) { next->prev = this; }
  }
  ~SmartCardThreadEntry() {
    if (prev) { prev->next = next; } else { *head = next; }
    if (next) { next->prev = prev; }
    
    delete thread;
  }
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
  SmartCardThreadEntry *current;
  for (current = head; current; current=current->next) {
    if (current->thread->GetModule() == aModule) {
      
      delete current;
      return;
    }
  }
}


nsresult
SmartCardThreadList::Add(SmartCardMonitoringThread *thread)
{
  SmartCardThreadEntry *current = new SmartCardThreadEntry(thread, head, nsnull,
                                                           &head);
  if (current) {  
     
    return thread->Start();
  }
  return NS_ERROR_OUT_OF_MEMORY;
}



static PR_CALLBACK PLHashNumber
unity(const void *key) { return PLHashNumber(NS_PTR_TO_INT32(key)); }

SmartCardMonitoringThread::SmartCardMonitoringThread(SECMODModule *module_)
  : mThread(nsnull)
{
  mModule = SECMOD_ReferenceModule(module_);
  
  
  mHash = PL_NewHashTable(10, unity, PL_CompareValues, 
                           PL_CompareStrings, nsnull, 0);
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
                              PR_PRIORITY_NORMAL, PR_LOCAL_THREAD,
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
                                       const char *tokenName, PRUint32 series)
{
  if (mHash) {
    if (tokenName) {
      int len = strlen(tokenName) + 1;
      

      char *entry = (char *)PR_Malloc(len+sizeof(PRUint32));
     
      if (entry) {  
        memcpy(entry,&series,sizeof(PRUint32));
        memcpy(&entry[sizeof(PRUint32)],tokenName,len);

        PL_HashTableAdd(mHash,(void *)slotid, entry); 
        return;
      }
    } 
    else {
      
      PL_HashTableRemove(mHash,(void *)slotid);
    }
  }
}


const char *
SmartCardMonitoringThread::GetTokenName(CK_SLOT_ID slotid)
{
  const char *tokenName = nsnull;
  const char *entry;

  if (mHash) {
    entry = (const char *)PL_HashTableLookupConst(mHash,(void *)slotid);
    if (entry) {
      tokenName = &entry[sizeof(PRUint32)];
    }
  }
  return tokenName;
}


PRUint32
SmartCardMonitoringThread::GetTokenSeries(CK_SLOT_ID slotid)
{
  PRUint32 series = 0;
  const char *entry;

  if (mHash) {
    entry = (const char *)PL_HashTableLookupConst(mHash,(void *)slotid);
    if (entry) {
      memcpy(&series,entry,sizeof(PRUint32));
    }
  }
  return series;
}




nsresult
SmartCardMonitoringThread::SendEvent(const nsAString &eventType,
                                     const char *tokenName)
{
  nsresult rv;
  nsCOMPtr<nsINSSComponent> 
                    nssComponent(do_GetService(kNSSComponentCID, &rv));
  if (NS_FAILED(rv))
    return rv;

  
  nssComponent->PostEvent(eventType, NS_ConvertUTF8toUTF16(tokenName));
  return NS_OK;
}




void SmartCardMonitoringThread::Execute()
{
  PK11SlotInfo *slot;
  const char *tokenName = nsnull;

  
  
  
  PK11SlotList *sl =
            PK11_FindSlotsByNames(mModule->dllName, nsnull, nsnull, PR_TRUE);
  PK11SlotListElement *sle;
 
  if (sl) {
    for (sle=PK11_GetFirstSafe(sl); sle; 
                                      sle=PK11_GetNextSafe(sl,sle,PR_FALSE)) {
      SetTokenName(PK11_GetSlotID(sle->slot), 
                  PK11_GetTokenName(sle->slot), PK11_GetSlotSeries(sle->slot));
    }
    PK11_FreeSlotList(sl);
  }

  
  do {
    slot = SECMOD_WaitForAnyTokenEvent(mModule, 0, PR_SecondsToInterval(1)  );
    if (slot == nsnull) {
      break;
    }

    
    
    if (PK11_IsPresent(slot)) {
      
      CK_SLOT_ID slotID = PK11_GetSlotID(slot);
      PRUint32 series = PK11_GetSlotSeries(slot);

      
      if (series != GetTokenSeries(slotID)) {
        
        
        tokenName = GetTokenName(slotID);
        if (tokenName) {
          SendEvent(NS_LITERAL_STRING(SMARTCARDEVENT_REMOVE), tokenName);
        }
        tokenName = PK11_GetTokenName(slot);
        
        SetTokenName(slotID, tokenName, series);
        SendEvent(NS_LITERAL_STRING(SMARTCARDEVENT_INSERT), tokenName);
      }
    } else {
      
      CK_SLOT_ID slotID = PK11_GetSlotID(slot);
      tokenName = GetTokenName(slotID);
      
      
      if (tokenName) {
        SendEvent(NS_LITERAL_STRING(SMARTCARDEVENT_REMOVE), tokenName);
        
        SetTokenName(slotID, nsnull, 0);
      }
    }
    PK11_FreeSlot(slot);

  } while (1);
}


const SECMODModule * SmartCardMonitoringThread::GetModule() 
{
  return mModule;
}


void SmartCardMonitoringThread::LaunchExecute(void *arg)
{
  ((SmartCardMonitoringThread*)arg)->Execute();
}

