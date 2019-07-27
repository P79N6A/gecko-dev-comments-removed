




#ifndef GMPParent_h_
#define GMPParent_h_

#include "GMPProcessParent.h"
#include "GMPService.h"
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

class GMPParent MOZ_FINAL : public PGMPParent,
                            public GMPSharedMem
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING_WITH_MAIN_THREAD_DESTRUCTION(GMPParent)

  GMPParent();

  nsresult Init(GeckoMediaPluginService *aService, nsIFile* aPluginDir);
  nsresult CloneFrom(const GMPParent* aOther);

  void Crash();

  nsresult LoadProcess();

  
  void CloseIfUnused();

  
  
  void CloseActive(bool aDieWhenUnloaded);

  
  void Shutdown();

  
  void DeleteProcess();

  bool SupportsAPI(const nsCString& aAPI, const nsCString& aTag);

  nsresult GetGMPVideoDecoder(GMPVideoDecoderParent** aGMPVD);
  void VideoDecoderDestroyed(GMPVideoDecoderParent* aDecoder);

  nsresult GetGMPVideoEncoder(GMPVideoEncoderParent** aGMPVE);
  void VideoEncoderDestroyed(GMPVideoEncoderParent* aEncoder);

  nsresult GetGMPDecryptor(GMPDecryptorParent** aGMPKS);
  void DecryptorDestroyed(GMPDecryptorParent* aSession);

  nsresult GetGMPAudioDecoder(GMPAudioDecoderParent** aGMPAD);
  void AudioDecoderDestroyed(GMPAudioDecoderParent* aDecoder);

  GMPState State() const;
  nsIThread* GMPThread();

  
  
  
  
  
  
  
  
  
  
  

  
  void SetNodeId(const nsACString& aNodeId);

  
  bool CanBeSharedCrossNodeIds() const;

  
  
  
  bool CanBeUsedFrom(const nsACString& aNodeId) const;

  already_AddRefed<nsIFile> GetDirectory() {
    return nsCOMPtr<nsIFile>(mDirectory).forget();
  }

  
  virtual void CheckThread() MOZ_OVERRIDE;

  void AbortAsyncShutdown();

private:
  ~GMPParent();
  nsRefPtr<GeckoMediaPluginService> mService;
  bool EnsureProcessLoaded();
  nsresult ReadGMPMetaData();
#ifdef MOZ_CRASHREPORTER
  void WriteExtraDataForMinidump(CrashReporter::AnnotationTable& notes);
  void GetCrashID(nsString& aResult);
#endif
  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  virtual PCrashReporterParent* AllocPCrashReporterParent(const NativeThreadId& aThread) MOZ_OVERRIDE;
  virtual bool DeallocPCrashReporterParent(PCrashReporterParent* aCrashReporter) MOZ_OVERRIDE;

  virtual PGMPVideoDecoderParent* AllocPGMPVideoDecoderParent() MOZ_OVERRIDE;
  virtual bool DeallocPGMPVideoDecoderParent(PGMPVideoDecoderParent* aActor) MOZ_OVERRIDE;

  virtual PGMPVideoEncoderParent* AllocPGMPVideoEncoderParent() MOZ_OVERRIDE;
  virtual bool DeallocPGMPVideoEncoderParent(PGMPVideoEncoderParent* aActor) MOZ_OVERRIDE;

  virtual PGMPDecryptorParent* AllocPGMPDecryptorParent() MOZ_OVERRIDE;
  virtual bool DeallocPGMPDecryptorParent(PGMPDecryptorParent* aActor) MOZ_OVERRIDE;

  virtual PGMPAudioDecoderParent* AllocPGMPAudioDecoderParent() MOZ_OVERRIDE;
  virtual bool DeallocPGMPAudioDecoderParent(PGMPAudioDecoderParent* aActor) MOZ_OVERRIDE;

  virtual bool RecvPGMPStorageConstructor(PGMPStorageParent* actor) MOZ_OVERRIDE;
  virtual PGMPStorageParent* AllocPGMPStorageParent() MOZ_OVERRIDE;
  virtual bool DeallocPGMPStorageParent(PGMPStorageParent* aActor) MOZ_OVERRIDE;

  virtual bool RecvPGMPTimerConstructor(PGMPTimerParent* actor) MOZ_OVERRIDE;
  virtual PGMPTimerParent* AllocPGMPTimerParent() MOZ_OVERRIDE;
  virtual bool DeallocPGMPTimerParent(PGMPTimerParent* aActor) MOZ_OVERRIDE;

  virtual bool RecvAsyncShutdownComplete() MOZ_OVERRIDE;
  virtual bool RecvAsyncShutdownRequired() MOZ_OVERRIDE;

  GMPState mState;
  nsCOMPtr<nsIFile> mDirectory; 
  nsString mName; 
  nsCString mDisplayName; 
  nsCString mDescription; 
  nsCString mVersion;
  nsTArray<nsAutoPtr<GMPCapability>> mCapabilities;
  GMPProcessParent* mProcess;
  bool mDeleteProcessOnlyOnUnload;
  bool mAbnormalShutdownInProgress;

  nsTArray<nsRefPtr<GMPVideoDecoderParent>> mVideoDecoders;
  nsTArray<nsRefPtr<GMPVideoEncoderParent>> mVideoEncoders;
  nsTArray<nsRefPtr<GMPDecryptorParent>> mDecryptors;
  nsTArray<nsRefPtr<GMPAudioDecoderParent>> mAudioDecoders;
  nsTArray<nsRefPtr<GMPTimerParent>> mTimers;
  nsTArray<nsRefPtr<GMPStorageParent>> mStorage;
  nsCOMPtr<nsIThread> mGMPThread;
  
  
  nsAutoCString mNodeId;

  bool mAsyncShutdownRequired;
  bool mAsyncShutdownInProgress;
};

} 
} 

#endif 
