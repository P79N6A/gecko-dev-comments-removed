





























#ifndef GMPVideoEncodedFrameImpl_h_
#define GMPVideoEncodedFrameImpl_h_

#include "gmp-errors.h"
#include "gmp-video-frame.h"
#include "gmp-video-frame-encoded.h"
#include "gmp-decryption.h"
#include "mozilla/ipc/Shmem.h"

namespace mozilla {
class CryptoSample;

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

  void InitCrypto(const CryptoSample& aCrypto);

  
  
  void DoneWithAPI();
  
  void ActorDestroyed();

  bool RelinquishFrameData(GMPVideoEncodedFrameData& aFrameData);

  
  virtual GMPVideoFrameFormat GetFrameFormat() override;
  virtual void Destroy() override;

  
  virtual GMPErr   CreateEmptyFrame(uint32_t aSize) override;
  virtual GMPErr   CopyFrame(const GMPVideoEncodedFrame& aFrame) override;
  virtual void     SetEncodedWidth(uint32_t aEncodedWidth) override;
  virtual uint32_t EncodedWidth() override;
  virtual void     SetEncodedHeight(uint32_t aEncodedHeight) override;
  virtual uint32_t EncodedHeight() override;
  
  virtual void     SetTimeStamp(uint64_t aTimeStamp) override;
  virtual uint64_t TimeStamp() override;
  
  
  
  
  virtual void     SetDuration(uint64_t aDuration) override;
  virtual uint64_t Duration() const override;
  virtual void     SetFrameType(GMPVideoFrameType aFrameType) override;
  virtual GMPVideoFrameType FrameType() override;
  virtual void     SetAllocatedSize(uint32_t aNewSize) override;
  virtual uint32_t AllocatedSize() override;
  virtual void     SetSize(uint32_t aSize) override;
  virtual uint32_t Size() override;
  virtual void     SetCompleteFrame(bool aCompleteFrame) override;
  virtual bool     CompleteFrame() override;
  virtual const uint8_t* Buffer() const override;
  virtual uint8_t* Buffer() override;
  virtual GMPBufferType BufferType() const override;
  virtual void     SetBufferType(GMPBufferType aBufferType) override;
  virtual const    GMPEncryptedBufferMetadata* GetDecryptionData() const override;

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

} 

#endif 
