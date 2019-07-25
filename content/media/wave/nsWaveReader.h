




































#if !defined(nsWaveReader_h_)
#define nsWaveReader_h_

#include "nsBuiltinDecoderReader.h"

class nsMediaDecoder;

class nsWaveReader : public nsBuiltinDecoderReader
{
public:
  nsWaveReader(nsBuiltinDecoder* aDecoder);
  ~nsWaveReader();

  virtual nsresult Init(nsBuiltinDecoderReader* aCloneDonor);
  virtual PRBool DecodeAudioData();
  virtual PRBool DecodeVideoFrame(PRBool &aKeyframeSkip,
                                  PRInt64 aTimeThreshold);

  virtual PRBool HasAudio()
  {
    return PR_TRUE;
  }

  virtual PRBool HasVideo()
  {
    return PR_FALSE;
  }

  virtual nsresult ReadMetadata(nsVideoInfo* aInfo);
  virtual nsresult Seek(PRInt64 aTime, PRInt64 aStartTime, PRInt64 aEndTime, PRInt64 aCurrentTime);
  virtual nsresult GetBuffered(nsTimeRanges* aBuffered, PRInt64 aStartTime);

private:
  PRBool ReadAll(char* aBuf, PRInt64 aSize, PRInt64* aBytesRead = nsnull);
  PRBool LoadRIFFChunk();
  PRBool ScanForwardUntil(PRUint32 aWantedChunk, PRUint32* aChunkSize);
  PRBool LoadFormatChunk();
  PRBool FindDataOffset();

  
  
  
  double BytesToTime(PRInt64 aBytes) const;

  
  
  
  PRInt64 TimeToBytes(double aTime) const;

  
  
  PRInt64 RoundDownToSample(PRInt64 aBytes) const;
  PRInt64 GetDataLength();
  PRInt64 GetPosition();

  




  
  PRUint32 mSampleRate;

  
  PRUint32 mChannels;

  
  
  PRUint32 mSampleSize;

  
  nsAudioStream::SampleFormat mSampleFormat;

  
  
  PRInt64 mWaveLength;

  
  
  PRInt64 mWavePCMOffset;
};

#endif
