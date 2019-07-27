




#ifndef GMPChild_h_
#define GMPChild_h_

#include "mozilla/gmp/PGMPChild.h"
#include "GMPSharedMemManager.h"
#include "GMPTimerChild.h"
#include "GMPStorageChild.h"
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

#if defined(XP_MACOSX) && defined(MOZ_GMP_SANDBOX)
  void OnChannelConnected(int32_t aPid);
#endif

  bool Init(const std::string& aPluginPath,
            base::ProcessHandle aParentProcessHandle,
            MessageLoop* aIOLoop,
            IPC::Channel* aChannel);
  bool LoadPluginLibrary(const std::string& aPluginPath);
#ifdef XP_WIN
  bool PreLoadLibraries(const std::string& aPluginPath);
#endif
  MessageLoop* GMPMessageLoop();

  
  GMPTimerChild* GetGMPTimers();
  GMPStorageChild* GetGMPStorage();

  
  virtual void CheckThread() MOZ_OVERRIDE;

  
  void ShutdownComplete() MOZ_OVERRIDE;

private:
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
  virtual void ProcessingError(Result aWhat) MOZ_OVERRIDE;

  GMPAsyncShutdown* mAsyncShutdown;
  nsRefPtr<GMPTimerChild> mTimerChild;
  nsRefPtr<GMPStorageChild> mStorage;

  PRLibrary* mLib;
  GMPGetAPIFunc mGetAPIFunc;
  MessageLoop* mGMPMessageLoop;
#if defined(XP_MACOSX) && defined(MOZ_GMP_SANDBOX)
  std::string mPluginPath;
  nsCString mPluginBinaryPath;
#endif
};

} 
} 

#endif 
