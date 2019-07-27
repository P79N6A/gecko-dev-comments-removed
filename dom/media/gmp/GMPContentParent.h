




#ifndef GMPContentParent_h_
#define GMPContentParent_h_

#include "mozilla/gmp/PGMPContentParent.h"
#include "GMPSharedMemManager.h"
#include "nsISupportsImpl.h"

namespace mozilla {
namespace gmp {

class GMPAudioDecoderParent;
class GMPDecryptorParent;
class GMPParent;
class GMPVideoDecoderParent;
class GMPVideoEncoderParent;

class GMPContentParent final : public PGMPContentParent,
                               public GMPSharedMem
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(GMPContentParent)

  explicit GMPContentParent(GMPParent* aParent = nullptr);

  nsresult GetGMPVideoDecoder(GMPVideoDecoderParent** aGMPVD);
  void VideoDecoderDestroyed(GMPVideoDecoderParent* aDecoder);

  nsresult GetGMPVideoEncoder(GMPVideoEncoderParent** aGMPVE);
  void VideoEncoderDestroyed(GMPVideoEncoderParent* aEncoder);

  nsresult GetGMPDecryptor(GMPDecryptorParent** aGMPKS);
  void DecryptorDestroyed(GMPDecryptorParent* aSession);

  nsresult GetGMPAudioDecoder(GMPAudioDecoderParent** aGMPAD);
  void AudioDecoderDestroyed(GMPAudioDecoderParent* aDecoder);

  nsIThread* GMPThread();

  
  virtual void CheckThread() override;

  void SetDisplayName(const nsCString& aDisplayName)
  {
    mDisplayName = aDisplayName;
  }
  const nsCString& GetDisplayName()
  {
    return mDisplayName;
  }
  void SetPluginId(const uint32_t aPluginId)
  {
    mPluginId = aPluginId;
  }
  const uint32_t GetPluginId()
  {
    return mPluginId;
  }

private:
  ~GMPContentParent();

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

  virtual PGMPVideoDecoderParent* AllocPGMPVideoDecoderParent() override;
  virtual bool DeallocPGMPVideoDecoderParent(PGMPVideoDecoderParent* aActor) override;

  virtual PGMPVideoEncoderParent* AllocPGMPVideoEncoderParent() override;
  virtual bool DeallocPGMPVideoEncoderParent(PGMPVideoEncoderParent* aActor) override;

  virtual PGMPDecryptorParent* AllocPGMPDecryptorParent() override;
  virtual bool DeallocPGMPDecryptorParent(PGMPDecryptorParent* aActor) override;

  virtual PGMPAudioDecoderParent* AllocPGMPAudioDecoderParent() override;
  virtual bool DeallocPGMPAudioDecoderParent(PGMPAudioDecoderParent* aActor) override;

  void CloseIfUnused();
  
  
  void Close()
  {
    PGMPContentParent::Close();
  }

  nsTArray<nsRefPtr<GMPVideoDecoderParent>> mVideoDecoders;
  nsTArray<nsRefPtr<GMPVideoEncoderParent>> mVideoEncoders;
  nsTArray<nsRefPtr<GMPDecryptorParent>> mDecryptors;
  nsTArray<nsRefPtr<GMPAudioDecoderParent>> mAudioDecoders;
  nsCOMPtr<nsIThread> mGMPThread;
  nsRefPtr<GMPParent> mParent;
  nsCString mDisplayName;
  uint32_t mPluginId;
};

} 
} 

#endif 
