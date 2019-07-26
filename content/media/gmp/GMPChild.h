




#ifndef GMPChild_h_
#define GMPChild_h_

#include "mozilla/gmp/PGMPChild.h"
#include "gmp-entrypoints.h"
#include "prlink.h"

namespace mozilla {
namespace gmp {

class GMPChild : public PGMPChild
{
public:
  GMPChild();
  virtual ~GMPChild();

  bool Init(const std::string& aPluginPath,
            base::ProcessHandle aParentProcessHandle,
            MessageLoop* aIOLoop,
            IPC::Channel* aChannel);
  bool LoadPluginLibrary(const std::string& aPluginPath);
  MessageLoop* GMPMessageLoop();

private:
  virtual PGMPVideoDecoderChild* AllocPGMPVideoDecoderChild() MOZ_OVERRIDE;
  virtual bool DeallocPGMPVideoDecoderChild(PGMPVideoDecoderChild* aActor) MOZ_OVERRIDE;
  virtual PGMPVideoEncoderChild* AllocPGMPVideoEncoderChild() MOZ_OVERRIDE;
  virtual bool DeallocPGMPVideoEncoderChild(PGMPVideoEncoderChild* aActor) MOZ_OVERRIDE;
  virtual bool RecvPGMPVideoDecoderConstructor(PGMPVideoDecoderChild* aActor) MOZ_OVERRIDE;
  virtual bool RecvPGMPVideoEncoderConstructor(PGMPVideoEncoderChild* aActor) MOZ_OVERRIDE;
  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;
  virtual void ProcessingError(Result aWhat) MOZ_OVERRIDE;

  PRLibrary* mLib;
  GMPGetAPIFunc mGetAPIFunc;
  MessageLoop* mGMPMessageLoop;
};

} 
} 

#endif 
