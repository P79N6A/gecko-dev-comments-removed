





#ifndef mozilla_dom_workers_ServiceWorkerRegistrar_h
#define mozilla_dom_workers_ServiceWorkerRegistrar_h

#include "mozilla/Monitor.h"
#include "mozilla/Telemetry.h"
#include "nsClassHashtable.h"
#include "nsIObserver.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsTArray.h"

#define SERVICEWORKERREGISTRAR_FILE "serviceworker.txt"
#define SERVICEWORKERREGISTRAR_VERSION "1"
#define SERVICEWORKERREGISTRAR_TERMINATOR "#"
#define SERVICEWORKERREGISTRAR_TRUE "true"
#define SERVICEWORKERREGISTRAR_FALSE "false"

class nsIFile;

namespace mozilla {

namespace ipc {
class PrincipalInfo;
} 

namespace dom {

class ServiceWorkerRegistrationData;

class ServiceWorkerRegistrar : public nsIObserver
{
  friend class ServiceWorkerRegistrarSaveDataRunnable;

public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIOBSERVER

  static void Initialize();

  void Shutdown();

  void DataSaved();

  static already_AddRefed<ServiceWorkerRegistrar> Get();

  void GetRegistrations(nsTArray<ServiceWorkerRegistrationData>& aValues);

  void RegisterServiceWorker(const ServiceWorkerRegistrationData& aData);
  void UnregisterServiceWorker(const mozilla::ipc::PrincipalInfo& aPrincipalInfo,
                               const nsACString& aScope);
  void RemoveAll();

protected:
  
  
  void LoadData();
  void SaveData();

  nsresult ReadData();
  nsresult WriteData();
  void DeleteData();

  ServiceWorkerRegistrar();
  virtual ~ServiceWorkerRegistrar();

private:
  void ProfileStarted();
  void ProfileStopped();

  void ScheduleSaveData();
  void ShutdownCompleted();
  void MaybeScheduleShutdownCompleted();

  mozilla::Monitor mMonitor;

protected:
  
  nsTArray<ServiceWorkerRegistrationData> mData;
  bool mDataLoaded;

  bool mShuttingDown;
  bool* mShutdownCompleteFlag;
  uint32_t mRunnableCounter;

  nsCOMPtr<nsIFile> mProfileDir;
};

} 
} 

#endif 
