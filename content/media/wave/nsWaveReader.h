




































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
                                  PRInt64 aTimeThreshold);

  virtual bool HasAudio()
  {
    return true;
  }

  virtual bool HasVideo()
  {
    return false;
  }

  virtual nsresult ReadMetadata(nsVideoInfo* aInfo);
  virtual nsresult Seek(PRInt64 aTime, PRInt64 aStartTime, PRInt64 aEndTime, PRInt64 aCurrentTime);
  virtual nsresult GetBuffered(nsTimeRanges* aBuffered, PRInt64 aStartTime);

  
  virtual bool IsSeekableInBufferedRanges() {
    return true;
  }

private:
  bool ReadAll(char* aBuf, PRInt64 aSize, PRInt64* aBytesRead = nsnull);
  bool LoadRIFFChunk();
  bool ScanForwardUntil(PRUint32 aWantedChunk, PRUint32* aChunkSize);
  bool LoadFormatChunk();
  bool FindDataOffset();

  
  
  
  double BytesToTime(PRInt64 aBytes) const;

  
  
  
  PRInt64 TimeToBytes(double aTime) const;

  
  
  PRInt64 RoundDownToFrame(PRInt64 aBytes) const;
  PRInt64 GetDataLength();
  PRInt64 GetPosition();

  




  
  PRUint32 mSampleRate;

  
  PRUint32 mChannels;

  
  
  PRUint32 mFrameSize;

  
  nsAudioStream::SampleFormat mSampleFormat;

  
  
  PRInt64 mWaveLength;

  
  
  PRInt64 mWavePCMOffset;
};

#endif
