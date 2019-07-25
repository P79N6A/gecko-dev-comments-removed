










































#ifndef mozilla_perfprobe_h
#define mozilla_perfprobe_h

#if !defined(XP_WIN)
#error "For the moment, perfprobe.h is defined only for Windows platforms"
#endif

#include "nsError.h"
#include "nsStringGlue.h"
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
  ~Probe() {};

protected:
  Probe(const nsCID &aGUID,
        const nsACString &aName,
         ProbeManager *aManager);
  friend ProbeManager;

protected:

  



  const GUID mGUID;

  



  const nsCString mName;

  




  class ProbeManager *mManager;
};









class ProbeManager
{
public:
  NS_INLINE_DECL_REFCOUNTING(ProbeManager)

  













  ProbeManager(const nsCID &applicationUID,
               const nsACString &applicationName);

  

















  already_AddRefed<Probe> GetProbe(const nsCID &eventUID,
                                   const nsACString &eventName);

  







  nsresult StartSession();
  nsresult StopSession();

  




  bool IsActive();

  ~ProbeManager();

protected:
  nsresult StartSession(nsTArray<nsRefPtr<Probe> > &probes);
  nsresult Init(const nsCID &applicationUID, const nsACString &applicationName);

protected:
  


  bool mIsActive;

  




  nsCID mApplicationUID;

  


  nsCString mApplicationName;

  


  nsTArray<nsRefPtr<Probe> > mAllProbes;

  


  TRACEHANDLE mSessionHandle;

  


  TRACEHANDLE mRegistrationHandle;

  


  bool mInitialized;

  friend Probe;
  friend ULONG WINAPI ControlCallback(
                                      __in  WMIDPREQUESTCODE RequestCode,
                                      __in  PVOID Context,
                                      __in  ULONG *Reserved,
                                      __in  PVOID Buffer
                                      );
};
}
};

#endif 
