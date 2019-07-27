










#ifndef mozilla_perfprobe_h
#define mozilla_perfprobe_h

#if !defined(XP_WIN)
#error "For the moment, perfprobe.h is defined only for Windows platforms"
#endif

#include "nsError.h"
#include "nsString.h"
#include "prlog.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include <windows.h>
#undef GetStartupInfo //Prevent Windows from polluting global namespace
#include <wmistr.h>
#include <evntrace.h>

namespace mozilla {
namespace probes {

class ProbeManager;






class Probe
{
public:
  NS_INLINE_DECL_REFCOUNTING(Probe)

  




  nsresult Trigger();

protected:
  ~Probe() {};

  Probe(const nsCID& aGUID, const nsACString& aName, ProbeManager* aManager);
  friend class ProbeManager;

protected:

  



  const GUID mGUID;

  



  const nsCString mName;

  




  class ProbeManager* mManager;
};









class ProbeManager
{
public:
  NS_INLINE_DECL_REFCOUNTING(ProbeManager)

  














  ProbeManager(const nsCID& aApplicationUID,
               const nsACString& aApplicationName);

  

















  already_AddRefed<Probe> GetProbe(const nsCID& aEventUID,
                                   const nsACString& aEventName);

  







  nsresult StartSession();
  nsresult StopSession();

  




  bool IsActive();

protected:
  ~ProbeManager();

  nsresult StartSession(nsTArray<nsRefPtr<Probe>>& aProbes);
  nsresult Init(const nsCID& aApplicationUID,
                const nsACString& aApplicationName);

protected:
  


  bool mIsActive;

  




  nsCID mApplicationUID;

  


  nsCString mApplicationName;

  


  nsTArray<nsRefPtr<Probe>> mAllProbes;

  


  TRACEHANDLE mSessionHandle;

  


  TRACEHANDLE mRegistrationHandle;

  


  bool mInitialized;

  friend class Probe;  
  friend ULONG WINAPI ControlCallback(WMIDPREQUESTCODE aRequestCode,
                                      PVOID aContext,
                                      ULONG* aReserved,
                                      PVOID aBuffer);  
};

}  
}  

#endif 
