




#ifndef GMPParent_h_
#define GMPParent_h_

#include "GMPProcessParent.h"
#include "GMPServiceParent.h"
#include "GMPAudioDecoderParent.h"
#include "GMPDecryptorParent.h"
#include "GMPVideoDecoderParent.h"
#include "GMPVideoEncoderParent.h"
#include "GMPTimerParent.h"
#include "GMPStorageParent.h"
#include "mozilla/gmp/PGMPParent.h"
#include "nsCOMPtr.h"
#include "nscore.h"
#include "nsISupports.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsIFile.h"
#include "ThreadSafeRefcountingWithMainThreadDestruction.h"

class nsILineInputStream;
class nsIThread;

#ifdef MOZ_CRASHREPORTER
#include "nsExceptionHandler.h"

namespace mozilla {
namespace dom {
class PCrashReporterParent;
class CrashReporterParent;
}
}
#endif

namespace mozilla {
namespace gmp {

class GMPCapability
{
public:
  nsCString mAPIName;
  nsTArray<nsCString> mAPITags;
};

enum GMPState {
  GMPStateNotLoaded,
  GMPStateLoaded,
  GMPStateUnloading,
  GMPStateClosing
};

class GMPContentParent;

class GetGMPContentParentCallback
{
public:
  GetGMPContentParentCallback()
  {
    MOZ_COUNT_CTOR(GetGMPContentParentCallback);
  };
  virtual ~GetGMPContentParentCallback()
  {
    MOZ_COUNT_DTOR(GetGMPContentParentCallback);
  };
  virtual void Done(GMPContentParent* aGMPContentParent) = 0;
};

class GMPParent final : public PGMPParent
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING_WITH_MAIN_THREAD_DESTRUCTION(GMPParent)

  GMPParent();

  nsresult Init(GeckoMediaPluginServiceParent* aService, nsIFile* aPluginDir);
  nsresult CloneFrom(const GMPParent* aOther);

  void Crash();

  nsresult LoadProcess();

  
  void CloseIfUnused();

  
  
  void CloseActive(bool aDieWhenUnloaded);

  
  void MarkForDeletion();
  bool IsMarkedForDeletion();

  
  void Shutdown();

  
  void DeleteProcess();

  bool SupportsAPI(const nsCString& aAPI, const nsCString& aTag);

  GMPState State() const;
  nsIThread* GMPThread();

  
  
  
  
  
  
  
  
  
  
  

  
  void SetNodeId(const nsACString& aNodeId);
  const nsACString& GetNodeId() const { return mNodeId; }

  const nsCString& GetDisplayName() const;
  const nsCString& GetVersion() const;
  const nsCString& GetPluginId() const;

  
  bool CanBeSharedCrossNodeIds() const;

  
  
  
  bool CanBeUsedFrom(const nsACString& aNodeId) const;

  already_AddRefed<nsIFile> GetDirectory() {
    return nsCOMPtr<nsIFile>(mDirectory).forget();
  }

  void AbortAsyncShutdown();

  
  void ChildTerminated();

  bool GetGMPContentParent(UniquePtr<GetGMPContentParentCallback>&& aCallback);
  already_AddRefed<GMPContentParent> ForgetGMPContentParent();

  bool EnsureProcessLoaded(base::ProcessId* aID);

  bool Bridge(GMPServiceParent* aGMPServiceParent);

private:
  ~GMPParent();
  nsRefPtr<GeckoMediaPluginServiceParent> mService;
  bool EnsureProcessLoaded();
  nsresult ReadGMPMetaData();
#ifdef MOZ_CRASHREPORTER
  void WriteExtraDataForMinidump(CrashReporter::AnnotationTable& notes);
  void GetCrashID(nsString& aResult);
#endif
  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

  virtual PCrashReporterParent* AllocPCrashReporterParent(const NativeThreadId& aThread) override;
  virtual bool DeallocPCrashReporterParent(PCrashReporterParent* aCrashReporter) override;

  virtual bool RecvPGMPStorageConstructor(PGMPStorageParent* actor) override;
  virtual PGMPStorageParent* AllocPGMPStorageParent() override;
  virtual bool DeallocPGMPStorageParent(PGMPStorageParent* aActor) override;

  virtual PGMPContentParent* AllocPGMPContentParent(Transport* aTransport,
                                                    ProcessId aOtherPid) override;

  virtual bool RecvPGMPTimerConstructor(PGMPTimerParent* actor) override;
  virtual PGMPTimerParent* AllocPGMPTimerParent() override;
  virtual bool DeallocPGMPTimerParent(PGMPTimerParent* aActor) override;

  virtual bool RecvAsyncShutdownComplete() override;
  virtual bool RecvAsyncShutdownRequired() override;

  virtual bool RecvPGMPContentChildDestroyed() override;
  bool IsUsed()
  {
    return mGMPContentChildCount > 0;
  }


  nsresult EnsureAsyncShutdownTimeoutSet();

  GMPState mState;
  nsCOMPtr<nsIFile> mDirectory; 
  nsString mName; 
  nsCString mDisplayName; 
  nsCString mDescription; 
  nsCString mVersion;
  nsCString mPluginId;
  nsTArray<nsAutoPtr<GMPCapability>> mCapabilities;
  GMPProcessParent* mProcess;
  bool mDeleteProcessOnlyOnUnload;
  bool mAbnormalShutdownInProgress;
  bool mIsBlockingDeletion;

  nsTArray<nsRefPtr<GMPTimerParent>> mTimers;
  nsTArray<nsRefPtr<GMPStorageParent>> mStorage;
  nsCOMPtr<nsIThread> mGMPThread;
  nsCOMPtr<nsITimer> mAsyncShutdownTimeout; 
  
  
  nsAutoCString mNodeId;
  
  
  nsRefPtr<GMPContentParent> mGMPContentParent;
  nsTArray<UniquePtr<GetGMPContentParentCallback>> mCallbacks;
  uint32_t mGMPContentChildCount;

  bool mAsyncShutdownRequired;
  bool mAsyncShutdownInProgress;

#ifdef PR_LOGGING
  int mChildPid;
#endif
};

} 
} 

#endif 
