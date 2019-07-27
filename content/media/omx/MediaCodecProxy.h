





#ifndef MEDIA_CODEC_PROXY_H
#define MEDIA_CODEC_PROXY_H

#include <nsString.h>

#include <stagefright/MediaCodec.h>
#include <utils/threads.h>

#include "MediaResourceHandler.h"

namespace android {

class MediaCodecProxy : public MediaResourceHandler::ResourceListener
{
public:
  


  struct CodecResourceListener : public virtual RefBase {
    


    virtual void codecReserved() = 0;
    



    virtual void codecCanceled() = 0;
  };

  
  bool allocated() const;

  
  
  static sp<MediaCodecProxy> CreateByType(sp<ALooper> aLooper,
                                          const char *aMime,
                                          bool aEncoder,
                                          bool aAsync=false,
                                          wp<CodecResourceListener> aListener=nullptr);

  
  status_t configure(const sp<AMessage> &aFormat,
                     const sp<Surface> &aNativeWindow,
                     const sp<ICrypto> &aCrypto,
                     uint32_t aFlags);

  status_t start();

  status_t stop();

  status_t release();

  status_t flush();

  status_t queueInputBuffer(size_t aIndex,
                            size_t aOffset,
                            size_t aSize,
                            int64_t aPresentationTimeUs,
                            uint32_t aFlags,
                            AString *aErrorDetailMessage=nullptr);

  status_t queueSecureInputBuffer(size_t aIndex,
                                  size_t aOffset,
                                  const CryptoPlugin::SubSample *aSubSamples,
                                  size_t aNumSubSamples,
                                  const uint8_t aKey[16],
                                  const uint8_t aIV[16],
                                  CryptoPlugin::Mode aMode,
                                  int64_t aPresentationTimeUs,
                                  uint32_t aFlags,
                                  AString *aErrorDetailMessage=nullptr);

  status_t dequeueInputBuffer(size_t *aIndex,
                              int64_t aTimeoutUs=INT64_C(0));

  status_t dequeueOutputBuffer(size_t *aIndex,
                               size_t *aOffset,
                               size_t *aSize,
                               int64_t *aPresentationTimeUs,
                               uint32_t *aFlags,
                               int64_t aTimeoutUs=INT64_C(0));

  status_t renderOutputBufferAndRelease(size_t aIndex);

  status_t releaseOutputBuffer(size_t aIndex);

  status_t signalEndOfInputStream();

  status_t getOutputFormat(sp<AMessage> *aFormat) const;

  status_t getInputBuffers(Vector<sp<ABuffer>> *aBuffers) const;

  status_t getOutputBuffers(Vector<sp<ABuffer>> *aBuffers) const;

  
  
  
  void requestActivityNotification(const sp<AMessage> &aNotify);

protected:
  virtual ~MediaCodecProxy();

  
  virtual void resourceReserved();
  
  virtual void resourceCanceled();

private:
  
  MediaCodecProxy() MOZ_DELETE;
  MediaCodecProxy(const MediaCodecProxy &) MOZ_DELETE;
  const MediaCodecProxy &operator=(const MediaCodecProxy &) MOZ_DELETE;

  
  MediaCodecProxy(sp<ALooper> aLooper,
                  const char *aMime,
                  bool aEncoder,
                  bool aAsync,
                  wp<CodecResourceListener> aListener);

  
  bool requestResource();
  
  void cancelResource();

  
  bool allocateCodec();
  
  void releaseCodec();

  
  sp<ALooper> mCodecLooper;
  nsCString mCodecMime;
  bool mCodecEncoder;

  
  wp<CodecResourceListener> mListener;

  
  sp<MediaResourceHandler> mResourceHandler;

  
  mutable RWLock mCodecLock;
  sp<MediaCodec> mCodec;
};

} 

#endif 
