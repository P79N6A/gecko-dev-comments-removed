




#ifndef GMPChild_h_
#define GMPChild_h_

#include "mozilla/gmp/PGMPChild.h"
#include "GMPSharedMemManager.h"
#include "GMPTimerChild.h"
#include "GMPStorageChild.h"
#include "GMPLoader.h"
#include "gmp-async-shutdown.h"
#include "gmp-entrypoints.h"
#include "prlink.h"

namespace mozilla {
namespace gmp {

class GMPChild : public PGMPChild
               , public GMPSharedMem
               , public GMPAsyncShutdownHost
{
public:
  GMPChild();
  virtual ~GMPChild();

  bool Init(const std::string& aPluginPath,
            const std::string& aVoucherPath,
            base::ProcessHandle aParentProcessHandle,
            MessageLoop* aIOLoop,
            IPC::Channel* aChannel);
#ifdef XP_WIN
  bool PreLoadLibraries(const std::string& aPluginPath);
#endif
  MessageLoop* GMPMessageLoop();

  
  GMPTimerChild* GetGMPTimers();
  GMPStorageChild* GetGMPStorage();

  
  virtual void CheckThread() MOZ_OVERRIDE;

  
  void ShutdownComplete() MOZ_OVERRIDE;

#if defined(XP_MACOSX) && defined(MOZ_GMP_SANDBOX)
  void StartMacSandbox();
#endif

private:

  bool PreLoadPluginVoucher(const std::string& aPluginPath);
  void PreLoadSandboxVoucher();

  bool GetLibPath(nsACString& aOutLibPath);

  virtual bool RecvSetNodeId(const nsCString& aNodeId) MOZ_OVERRIDE;
  virtual bool RecvStartPlugin() MOZ_OVERRIDE;

  virtual PCrashReporterChild* AllocPCrashReporterChild(const NativeThreadId& aThread) MOZ_OVERRIDE;
  virtual bool DeallocPCrashReporterChild(PCrashReporterChild*) MOZ_OVERRIDE;

  virtual PGMPVideoDecoderChild* AllocPGMPVideoDecoderChild() MOZ_OVERRIDE;
  virtual bool DeallocPGMPVideoDecoderChild(PGMPVideoDecoderChild* aActor) MOZ_OVERRIDE;
  virtual bool RecvPGMPVideoDecoderConstructor(PGMPVideoDecoderChild* aActor) MOZ_OVERRIDE;

  virtual PGMPVideoEncoderChild* AllocPGMPVideoEncoderChild() MOZ_OVERRIDE;
  virtual bool DeallocPGMPVideoEncoderChild(PGMPVideoEncoderChild* aActor) MOZ_OVERRIDE;
  virtual bool RecvPGMPVideoEncoderConstructor(PGMPVideoEncoderChild* aActor) MOZ_OVERRIDE;

  virtual PGMPDecryptorChild* AllocPGMPDecryptorChild() MOZ_OVERRIDE;
  virtual bool DeallocPGMPDecryptorChild(PGMPDecryptorChild* aActor) MOZ_OVERRIDE;
  virtual bool RecvPGMPDecryptorConstructor(PGMPDecryptorChild* aActor) MOZ_OVERRIDE;

  virtual PGMPAudioDecoderChild* AllocPGMPAudioDecoderChild() MOZ_OVERRIDE;
  virtual bool DeallocPGMPAudioDecoderChild(PGMPAudioDecoderChild* aActor) MOZ_OVERRIDE;
  virtual bool RecvPGMPAudioDecoderConstructor(PGMPAudioDecoderChild* aActor) MOZ_OVERRIDE;

  virtual PGMPTimerChild* AllocPGMPTimerChild() MOZ_OVERRIDE;
  virtual bool DeallocPGMPTimerChild(PGMPTimerChild* aActor) MOZ_OVERRIDE;

  virtual PGMPStorageChild* AllocPGMPStorageChild() MOZ_OVERRIDE;
  virtual bool DeallocPGMPStorageChild(PGMPStorageChild* aActor) MOZ_OVERRIDE;

  virtual bool RecvCrashPluginNow() MOZ_OVERRIDE;
  virtual bool RecvBeginAsyncShutdown() MOZ_OVERRIDE;

  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;
  virtual void ProcessingError(Result aCode, const char* aReason) MOZ_OVERRIDE;

  GMPErr GetAPI(const char* aAPIName, void* aHostAPI, void** aPluginAPI);

  GMPAsyncShutdown* mAsyncShutdown;
  nsRefPtr<GMPTimerChild> mTimerChild;
  nsRefPtr<GMPStorageChild> mStorage;

  MessageLoop* mGMPMessageLoop;
  std::string mPluginPath;
  std::string mVoucherPath;
#if defined(XP_MACOSX) && defined(MOZ_GMP_SANDBOX)
  nsCString mPluginBinaryPath;
#endif
  std::string mNodeId;
  GMPLoader* mGMPLoader;
  nsTArray<uint8_t> mPluginVoucher;
  nsTArray<uint8_t> mSandboxVoucher;
};

} 
} 

#endif 
