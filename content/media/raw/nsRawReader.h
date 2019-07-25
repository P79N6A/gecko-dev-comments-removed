






































#if !defined(nsRawReader_h_)
#define nsRawReader_h_

#include "nsBuiltinDecoderReader.h"

struct nsRawVideo_PRUint24 {
  operator PRUint32() const { return value[2] << 16 | value[1] << 8 | value[0]; }
private:
  PRUint8 value[3];
};

struct nsRawPacketHeader {
  typedef nsRawVideo_PRUint24 PRUint24;
  PRUint8 packetID;
  PRUint24 codecID;
};


struct nsRawVideoHeader {
  typedef nsRawVideo_PRUint24 PRUint24;
  PRUint8 headerPacketID;          
  PRUint24 codecID;                
  PRUint8 majorVersion;            
  PRUint8 minorVersion;            
  PRUint16 options;                
                                   
                                   
                                   
                                   
                                   
                                   
                                   
                                   
                                   
                                   

  PRUint8 alphaChannelBpp;
  PRUint8 lumaChannelBpp;
  PRUint8 chromaChannelBpp;
  PRUint8 colorspace;

  PRUint24 frameWidth;
  PRUint24 frameHeight;
  PRUint24 aspectNumerator;
  PRUint24 aspectDenominator;

  PRUint32 framerateNumerator;
  PRUint32 framerateDenominator;
};

class nsRawReader : public nsBuiltinDecoderReader
{
public:
  nsRawReader(nsBuiltinDecoder* aDecoder);
  ~nsRawReader();

  virtual nsresult Init(nsBuiltinDecoderReader* aCloneDonor);
  virtual nsresult ResetDecode();
  virtual PRBool DecodeAudioData();

  virtual PRBool DecodeVideoFrame(PRBool &aKeyframeSkip,
                                  PRInt64 aTimeThreshold);

  virtual PRBool HasAudio()
  {
    return PR_FALSE;
  }

  virtual PRBool HasVideo()
  {
    return PR_TRUE;
  }

  virtual nsresult ReadMetadata(nsVideoInfo* aInfo);
  virtual nsresult Seek(PRInt64 aTime, PRInt64 aStartTime, PRInt64 aEndTime, PRInt64 aCurrentTime);
  virtual nsresult GetBuffered(nsTimeRanges* aBuffered, PRInt64 aStartTime);

private:
  PRBool ReadFromStream(nsMediaStream *aStream, PRUint8 *aBuf,
                        PRUint32 aLength);

  nsRawVideoHeader mMetadata;
  PRUint32 mCurrentFrame;
  double mFrameRate;
  PRUint32 mFrameSize;
  nsIntRect mPicture;
};

#endif
