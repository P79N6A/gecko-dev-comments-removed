




#if !defined(nsWaveReader_h_)
#define nsWaveReader_h_

#include "nsBuiltinDecoderReader.h"

class nsBuiltinDecoder;
class nsTimeRanges;

class nsWaveReader : public nsBuiltinDecoderReader
{
public:
  nsWaveReader(nsBuiltinDecoder* aDecoder);
  ~nsWaveReader();

  virtual nsresult Init(nsBuiltinDecoderReader* aCloneDonor);
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

  virtual nsresult ReadMetadata(nsVideoInfo* aInfo,
                                nsHTMLMediaElement::MetadataTags** aTags);
  virtual nsresult Seek(int64_t aTime, int64_t aStartTime, int64_t aEndTime, int64_t aCurrentTime);
  virtual nsresult GetBuffered(nsTimeRanges* aBuffered, int64_t aStartTime);

  
  virtual bool IsSeekableInBufferedRanges() {
    return true;
  }

private:
  bool ReadAll(char* aBuf, int64_t aSize, int64_t* aBytesRead = nullptr);
  bool LoadRIFFChunk();
  bool ScanForwardUntil(uint32_t aWantedChunk, uint32_t* aChunkSize);
  bool LoadFormatChunk();
  bool FindDataOffset();

  
  
  
  double BytesToTime(int64_t aBytes) const;

  
  
  
  int64_t TimeToBytes(double aTime) const;

  
  
  int64_t RoundDownToFrame(int64_t aBytes) const;
  int64_t GetDataLength();
  int64_t GetPosition();

  




  
  uint32_t mSampleRate;

  
  uint32_t mChannels;

  
  
  uint32_t mFrameSize;

  
  nsAudioStream::SampleFormat mSampleFormat;

  
  
  int64_t mWaveLength;

  
  
  int64_t mWavePCMOffset;
};

#endif
