




#if !defined(AndroidMediaReader_h_)
#define AndroidMediaReader_h_

#include "mozilla/Attributes.h"
#include "MediaResource.h"
#include "MediaDecoderReader.h"
#include "ImageContainer.h"
#include "nsAutoPtr.h"
#include "mozilla/layers/SharedRGBImage.h"
 
#include "MPAPI.h"

class nsACString;

namespace mozilla {

class AbstractMediaDecoder;

namespace layers {
class ImageContainer;
}

namespace dom {
class TimeRanges;
}
 
class AndroidMediaReader : public MediaDecoderReader
{
  nsCString mType;
  MPAPI::Decoder *mPlugin;
  bool mHasAudio;
  bool mHasVideo;
  nsIntRect mPicture;
  nsIntSize mInitialFrame;
  int64_t mVideoSeekTimeUs;
  int64_t mAudioSeekTimeUs;
  nsRefPtr<VideoData> mLastVideoFrame;
public:
  AndroidMediaReader(AbstractMediaDecoder* aDecoder,
                     const nsACString& aContentType);

  virtual nsresult Init(MediaDecoderReader* aCloneDonor);
  virtual nsresult ResetDecode();

  virtual bool DecodeAudioData();
  virtual bool DecodeVideoFrame(bool &aKeyframeSkip,
                                int64_t aTimeThreshold);

  virtual bool HasAudio()
  {
    return mHasAudio;
  }

  virtual bool HasVideo()
  {
    return mHasVideo;
  }

  virtual bool IsMediaSeekable()
  {
    
    return true;
  }

  virtual nsresult ReadMetadata(MediaInfo* aInfo,
                                MetadataTags** aTags);
  virtual void Seek(int64_t aTime, int64_t aStartTime, int64_t aEndTime, int64_t aCurrentTime);

  virtual void Shutdown() MOZ_OVERRIDE;

  class ImageBufferCallback : public MPAPI::BufferCallback {
    typedef mozilla::layers::Image Image;

  public:
    ImageBufferCallback(mozilla::layers::ImageContainer *aImageContainer);
    void *operator()(size_t aWidth, size_t aHeight,
                     MPAPI::ColorFormat aColorFormat) MOZ_OVERRIDE;
    already_AddRefed<Image> GetImage();

  private:
    uint8_t *CreateI420Image(size_t aWidth, size_t aHeight);

    mozilla::layers::ImageContainer *mImageContainer;
    nsRefPtr<Image> mImage;
  };

};

} 

#endif
