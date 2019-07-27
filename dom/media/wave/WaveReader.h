




#if !defined(WaveReader_h_)
#define WaveReader_h_

#include "MediaDecoderReader.h"
#include "mozilla/dom/HTMLMediaElement.h"

namespace mozilla {
namespace dom {
class TimeRanges;
}
}

namespace mozilla {

class WaveReader : public MediaDecoderReader
{
public:
  explicit WaveReader(AbstractMediaDecoder* aDecoder);

protected:
  ~WaveReader();

public:
  virtual nsresult Init(MediaDecoderReader* aCloneDonor);
  virtual bool DecodeAudioData();
  virtual bool DecodeVideoFrame(bool &aKeyframeSkip,
                                  int64_t aTimeThreshold);

  virtual bool HasAudio()
  {
    return true;
  }

  virtual bool HasVideo()
  {
    return false;
  }

  virtual nsresult ReadMetadata(MediaInfo* aInfo,
                                MetadataTags** aTags);
  virtual nsresult Seek(int64_t aTime, int64_t aStartTime, int64_t aEndTime, int64_t aCurrentTime);
  virtual nsresult GetBuffered(dom::TimeRanges* aBuffered, int64_t aStartTime);

  
  virtual bool IsSeekableInBufferedRanges() {
    return true;
  }

  virtual bool IsMediaSeekable() MOZ_OVERRIDE;

private:
  bool ReadAll(char* aBuf, int64_t aSize, int64_t* aBytesRead = nullptr);
  bool LoadRIFFChunk();
  bool GetNextChunk(uint32_t* aChunk, uint32_t* aChunkSize);
  bool LoadFormatChunk(uint32_t aChunkSize);
  bool FindDataOffset(uint32_t aChunkSize);
  bool LoadListChunk(uint32_t aChunkSize, nsAutoPtr<dom::HTMLMediaElement::MetadataTags> &aTags);
  bool LoadAllChunks(nsAutoPtr<dom::HTMLMediaElement::MetadataTags> &aTags);

  
  
  
  double BytesToTime(int64_t aBytes) const;

  
  
  
  int64_t TimeToBytes(double aTime) const;

  
  
  int64_t RoundDownToFrame(int64_t aBytes) const;
  int64_t GetDataLength();
  int64_t GetPosition();

  




  
  uint32_t mSampleRate;

  
  uint32_t mChannels;

  
  
  uint32_t mFrameSize;

  
  
  enum {
    FORMAT_U8,
    FORMAT_S16
  } mSampleFormat;

  
  
  int64_t mWaveLength;

  
  
  int64_t mWavePCMOffset;
};

} 

#endif
