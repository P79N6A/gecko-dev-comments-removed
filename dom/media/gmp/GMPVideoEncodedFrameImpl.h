





























#ifndef GMPVideoEncodedFrameImpl_h_
#define GMPVideoEncodedFrameImpl_h_

#include "gmp-errors.h"
#include "gmp-video-frame.h"
#include "gmp-video-frame-encoded.h"
#include "gmp-decryption.h"
#include "mozilla/UniquePtr.h"
#include "mozilla/ipc/Shmem.h"
#include "mp4_demuxer/DecoderData.h"

namespace mozilla {
namespace gmp {

class GMPVideoHostImpl;
class GMPVideoEncodedFrameData;
class GMPEncryptedBufferDataImpl;

class GMPVideoEncodedFrameImpl: public GMPVideoEncodedFrame
{
  friend struct IPC::ParamTraits<mozilla::gmp::GMPVideoEncodedFrameImpl>;
public:
  explicit GMPVideoEncodedFrameImpl(GMPVideoHostImpl* aHost);
  GMPVideoEncodedFrameImpl(const GMPVideoEncodedFrameData& aFrameData, GMPVideoHostImpl* aHost);
  virtual ~GMPVideoEncodedFrameImpl();

  void InitCrypto(const mp4_demuxer::CryptoSample& aCrypto);

  
  
  void DoneWithAPI();
  
  void ActorDestroyed();

  bool RelinquishFrameData(GMPVideoEncodedFrameData& aFrameData);

  
  virtual GMPVideoFrameFormat GetFrameFormat() MOZ_OVERRIDE;
  virtual void Destroy() MOZ_OVERRIDE;

  
  virtual GMPErr   CreateEmptyFrame(uint32_t aSize) MOZ_OVERRIDE;
  virtual GMPErr   CopyFrame(const GMPVideoEncodedFrame& aFrame) MOZ_OVERRIDE;
  virtual void     SetEncodedWidth(uint32_t aEncodedWidth) MOZ_OVERRIDE;
  virtual uint32_t EncodedWidth() MOZ_OVERRIDE;
  virtual void     SetEncodedHeight(uint32_t aEncodedHeight) MOZ_OVERRIDE;
  virtual uint32_t EncodedHeight() MOZ_OVERRIDE;
  
  virtual void     SetTimeStamp(uint64_t aTimeStamp) MOZ_OVERRIDE;
  virtual uint64_t TimeStamp() MOZ_OVERRIDE;
  
  
  
  
  virtual void     SetDuration(uint64_t aDuration) MOZ_OVERRIDE;
  virtual uint64_t Duration() const MOZ_OVERRIDE;
  virtual void     SetFrameType(GMPVideoFrameType aFrameType) MOZ_OVERRIDE;
  virtual GMPVideoFrameType FrameType() MOZ_OVERRIDE;
  virtual void     SetAllocatedSize(uint32_t aNewSize) MOZ_OVERRIDE;
  virtual uint32_t AllocatedSize() MOZ_OVERRIDE;
  virtual void     SetSize(uint32_t aSize) MOZ_OVERRIDE;
  virtual uint32_t Size() MOZ_OVERRIDE;
  virtual void     SetCompleteFrame(bool aCompleteFrame) MOZ_OVERRIDE;
  virtual bool     CompleteFrame() MOZ_OVERRIDE;
  virtual const uint8_t* Buffer() const MOZ_OVERRIDE;
  virtual uint8_t* Buffer() MOZ_OVERRIDE;
  virtual GMPBufferType BufferType() const MOZ_OVERRIDE;
  virtual void     SetBufferType(GMPBufferType aBufferType) MOZ_OVERRIDE;
  virtual const    GMPEncryptedBufferMetadata* GetDecryptionData() const MOZ_OVERRIDE;

private:
  void DestroyBuffer();

  uint32_t mEncodedWidth;
  uint32_t mEncodedHeight;
  uint64_t mTimeStamp;
  uint64_t mDuration;
  GMPVideoFrameType mFrameType;
  uint32_t mSize;
  bool     mCompleteFrame;
  GMPVideoHostImpl* mHost;
  ipc::Shmem mBuffer;
  GMPBufferType mBufferType;
  nsAutoPtr<GMPEncryptedBufferDataImpl> mCrypto;
};

} 

template<>
struct DefaultDelete<mozilla::gmp::GMPVideoEncodedFrameImpl>
{
  void operator()(mozilla::gmp::GMPVideoEncodedFrameImpl* aFrame) const
  {
    aFrame->Destroy();
  }
};

} 

#endif 
