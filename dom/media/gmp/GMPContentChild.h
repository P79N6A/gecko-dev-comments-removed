




#ifndef GMPContentChild_h_
#define GMPContentChild_h_

#include "mozilla/gmp/PGMPContentChild.h"
#include "GMPSharedMemManager.h"

namespace mozilla {
namespace gmp {

class GMPChild;

class GMPContentChild : public PGMPContentChild
                      , public GMPSharedMem
{
public:
  explicit GMPContentChild(GMPChild* aChild);
  virtual ~GMPContentChild();

  MessageLoop* GMPMessageLoop();

  virtual bool RecvPGMPAudioDecoderConstructor(PGMPAudioDecoderChild* aActor) override;
  virtual bool RecvPGMPDecryptorConstructor(PGMPDecryptorChild* aActor) override;
  virtual bool RecvPGMPVideoDecoderConstructor(PGMPVideoDecoderChild* aActor) override;
  virtual bool RecvPGMPVideoEncoderConstructor(PGMPVideoEncoderChild* aActor) override;

  virtual PGMPAudioDecoderChild* AllocPGMPAudioDecoderChild() override;
  virtual bool DeallocPGMPAudioDecoderChild(PGMPAudioDecoderChild* aActor) override;

  virtual PGMPDecryptorChild* AllocPGMPDecryptorChild() override;
  virtual bool DeallocPGMPDecryptorChild(PGMPDecryptorChild* aActor) override;

  virtual PGMPVideoDecoderChild* AllocPGMPVideoDecoderChild() override;
  virtual bool DeallocPGMPVideoDecoderChild(PGMPVideoDecoderChild* aActor) override;

  virtual PGMPVideoEncoderChild* AllocPGMPVideoEncoderChild() override;
  virtual bool DeallocPGMPVideoEncoderChild(PGMPVideoEncoderChild* aActor) override;

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;
  virtual void ProcessingError(Result aCode, const char* aReason) override;

  
  virtual void CheckThread() override;

  void CloseActive();
  bool IsUsed();

  GMPChild* mGMPChild;
};

} 
} 

#endif 
