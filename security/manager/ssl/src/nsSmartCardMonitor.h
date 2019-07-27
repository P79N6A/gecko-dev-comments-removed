



#ifndef nsSmartCardMonitor_h
#define nsSmartCardMonitor_h

#include "prthread.h"
#include "secmod.h"
#include "plhash.h"
#include "pkcs11t.h"

class SmartCardThreadEntry;
class SmartCardMonitoringThread;




class SmartCardThreadList {
public:
  SmartCardThreadList();
  ~SmartCardThreadList();
  void Remove(SECMODModule* module);
  nsresult Add(SmartCardMonitoringThread* thread);

private:
  SmartCardThreadEntry* head;
};







class SmartCardMonitoringThread
{
 public:
  explicit SmartCardMonitoringThread(SECMODModule* module);
  ~SmartCardMonitoringThread();
  
  nsresult Start();
  void Stop();
  
  void Execute();
  void Interrupt();
  
  const SECMODModule* GetModule();

 private:
  static void LaunchExecute(void* arg);
  void SetTokenName(CK_SLOT_ID slotid, const char* tokenName, uint32_t series);
  const char* GetTokenName(CK_SLOT_ID slotid);
  uint32_t GetTokenSeries(CK_SLOT_ID slotid);
  void SendEvent(const nsAString& type, const char* tokenName);
  
  SECMODModule* mModule;
  PLHashTable* mHash;
  PRThread* mThread;
};

#endif 
