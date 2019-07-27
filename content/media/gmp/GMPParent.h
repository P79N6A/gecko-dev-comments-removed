




#ifndef GMPParent_h_
#define GMPParent_h_

#include "GMPProcessParent.h"
#include "GMPVideoDecoderParent.h"
#include "GMPVideoEncoderParent.h"
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
  GMPStateLoaded
};

class GMPParent MOZ_FINAL : public PGMPParent
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING_WITH_MAIN_THREAD_DESTRUCTION(GMPParent)

  GMPParent();

  nsresult Init(nsIFile* aPluginDir);
  nsresult LoadProcess();
  void MaybeUnloadProcess();
  void UnloadProcess();
  bool SupportsAPI(const nsCString& aAPI, const nsCString& aTag);
  nsresult GetGMPVideoDecoder(GMPVideoDecoderParent** aGMPVD);
  void VideoDecoderDestroyed(GMPVideoDecoderParent* aDecoder);
  nsresult GetGMPVideoEncoder(GMPVideoEncoderParent** aGMPVE);
  void VideoEncoderDestroyed(GMPVideoEncoderParent* aEncoder);
  GMPState State() const;
#ifdef DEBUG
  nsIThread* GMPThread();
#endif

  
  
  
  
  
  
  
  
  

  
  void SetOrigin(const nsAString& aOrigin);

  
  bool CanBeSharedCrossOrigin() const;

  
  
  
  bool CanBeUsedFrom(const nsAString& aOrigin) const;

  already_AddRefed<nsIFile> GetDirectory() {
    return nsCOMPtr<nsIFile>(mDirectory).forget();
  }

private:
  ~GMPParent();
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

  GMPState mState;
  nsCOMPtr<nsIFile> mDirectory; 
  nsString mName; 
  nsCString mDisplayName; 
  nsCString mDescription; 
  nsCString mVersion;
  nsTArray<nsAutoPtr<GMPCapability>> mCapabilities;
  GMPProcessParent* mProcess;

  nsTArray<nsRefPtr<GMPVideoDecoderParent>> mVideoDecoders;
  nsTArray<nsRefPtr<GMPVideoEncoderParent>> mVideoEncoders;
#ifdef DEBUG
  nsCOMPtr<nsIThread> mGMPThread;
#endif
  
  
  nsAutoString mOrigin;
};

} 
} 

#endif 
