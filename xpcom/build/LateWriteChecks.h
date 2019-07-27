





#ifndef mozilla_LateWriteChecks_h
#define mozilla_LateWriteChecks_h






namespace mozilla {


enum ShutdownChecksMode
{
  SCM_CRASH,      
  SCM_RECORD,     
  SCM_NOTHING     
};





extern ShutdownChecksMode gShutdownChecks;






void InitLateWriteChecks();












void BeginLateWriteChecks();






void StopLateWriteChecks();

} 

#endif 
