






































#include "nsBuiltinDecoderStateMachine.h"
#include "nsBuiltinDecoder.h"
#include "nsRawReader.h"
#include "nsRawDecoder.h"
#include "VideoUtils.h"

#define RAW_ID 0x595556

nsRawReader::nsRawReader(nsBuiltinDecoder* aDecoder)
  : nsBuiltinDecoderReader(aDecoder),
    mCurrentFrame(0), mFrameSize(0)
{
  MOZ_COUNT_CTOR(nsRawReader);
}

nsRawReader::~nsRawReader()
{
  MOZ_COUNT_DTOR(nsRawReader);
}

nsresult nsRawReader::Init(nsBuiltinDecoderReader* aCloneDonor)
{
  return NS_OK;
}

nsresult nsRawReader::ResetDecode()
{
  mCurrentFrame = 0;
  return nsBuiltinDecoderReader::ResetDecode();
}

nsresult nsRawReader::ReadMetadata(nsVideoInfo* aInfo)
{
  NS_ASSERTION(mDecoder->OnStateMachineThread(),
               "Should be on state machine thread.");
  mozilla::MonitorAutoEnter autoEnter(mMonitor);

  nsMediaStream* stream = mDecoder->GetCurrentStream();
  NS_ASSERTION(stream, "Decoder has no media stream");

  if (!ReadFromStream(stream, reinterpret_cast<PRUint8*>(&mMetadata),
                      sizeof(mMetadata)))
    return NS_ERROR_FAILURE;

  
  if (!(mMetadata.headerPacketID == 0  &&
        mMetadata.codecID == RAW_ID  &&
        mMetadata.majorVersion == 0 &&
        mMetadata.minorVersion == 1))
    return NS_ERROR_FAILURE;

  PRUint32 dummy;
  if (!MulOverflow32(mMetadata.frameWidth, mMetadata.frameHeight, dummy))
    return NS_ERROR_FAILURE;

  mInfo.mHasVideo = PR_TRUE;
  mInfo.mPicture.x = 0;
  mInfo.mPicture.y = 0;
  mInfo.mPicture.width = mMetadata.frameWidth;
  mInfo.mPicture.height = mMetadata.frameHeight;
  mInfo.mFrame.width = mMetadata.frameWidth;
  mInfo.mFrame.height = mMetadata.frameHeight;
  if (mMetadata.aspectDenominator == 0 ||
      mMetadata.framerateDenominator == 0)
    return NS_ERROR_FAILURE; 
  mInfo.mPixelAspectRatio = static_cast<float>(mMetadata.aspectNumerator) / 
                            mMetadata.aspectDenominator;
  mInfo.mHasAudio = PR_FALSE;

  mFrameRate = static_cast<float>(mMetadata.framerateNumerator) /
               mMetadata.framerateDenominator;

  
  if (mFrameRate > 45 ||
      mFrameRate == 0 ||
      mInfo.mPixelAspectRatio == 0 ||
      mMetadata.frameWidth > 2000 ||
      mMetadata.frameHeight > 2000 ||
      mMetadata.chromaChannelBpp != 4 ||
      mMetadata.lumaChannelBpp != 8 ||
      mMetadata.colorspace != 1 )
    return NS_ERROR_FAILURE;

  mFrameSize = mMetadata.frameWidth * mMetadata.frameHeight *
    (mMetadata.lumaChannelBpp + mMetadata.chromaChannelBpp) / 8.0 +
    sizeof(nsRawPacketHeader);

  PRInt64 length = stream->GetLength();
  if (length != -1) {
    mozilla::MonitorAutoExit autoExitMonitor(mMonitor);
    mozilla::MonitorAutoEnter autoMonitor(mDecoder->GetMonitor());
    mDecoder->GetStateMachine()->SetDuration(USECS_PER_S *
                                           (length - sizeof(nsRawVideoHeader)) /
                                           (mFrameSize * mFrameRate));
  }

  *aInfo = mInfo;

  return NS_OK;
}

 PRBool nsRawReader::DecodeAudioData()
{
  NS_ASSERTION(mDecoder->OnStateMachineThread() || mDecoder->OnDecodeThread(),
               "Should be on state machine thread or decode thread.");
  return PR_FALSE;
}



PRBool nsRawReader::ReadFromStream(nsMediaStream *aStream, PRUint8* aBuf,
                                   PRUint32 aLength)
{
  while (aLength > 0) {
    PRUint32 bytesRead = 0;
    nsresult rv;

    rv = aStream->Read(reinterpret_cast<char*>(aBuf), aLength, &bytesRead);
    NS_ENSURE_SUCCESS(rv, PR_FALSE);

    if (bytesRead == 0) {
      return PR_FALSE;
    }

    aLength -= bytesRead;
    aBuf += bytesRead;
  }

  return PR_TRUE;
}

PRBool nsRawReader::DecodeVideoFrame(PRBool &aKeyframeSkip,
                                     PRInt64 aTimeThreshold)
{
  mozilla::MonitorAutoEnter autoEnter(mMonitor);
  NS_ASSERTION(mDecoder->OnStateMachineThread() || mDecoder->OnDecodeThread(),
               "Should be on state machine thread or decode thread.");

  
  
  PRUint32 parsed = 0, decoded = 0;
  nsMediaDecoder::AutoNotifyDecoded autoNotify(mDecoder, parsed, decoded);

  if (!mFrameSize)
    return PR_FALSE; 

  PRInt64 currentFrameTime = USECS_PER_S * mCurrentFrame / mFrameRate;
  PRUint32 length = mFrameSize - sizeof(nsRawPacketHeader);

  nsAutoArrayPtr<PRUint8> buffer(new PRUint8[length]);
  nsMediaStream* stream = mDecoder->GetCurrentStream();
  NS_ASSERTION(stream, "Decoder has no media stream");

  
  while(true) {
    nsRawPacketHeader header;

    
    if (!(ReadFromStream(stream, reinterpret_cast<PRUint8*>(&header),
                         sizeof(header))) ||
        !(header.packetID == 0xFF && header.codecID == RAW_ID )) {
      return PR_FALSE;
    }

    if (!ReadFromStream(stream, buffer, length)) {
      return PR_FALSE;
    }

    parsed++;

    if (currentFrameTime >= aTimeThreshold)
      break;

    mCurrentFrame++;
    currentFrameTime += static_cast<double>(USECS_PER_S) / mFrameRate;
  }

  VideoData::YCbCrBuffer b;
  b.mPlanes[0].mData = buffer;
  b.mPlanes[0].mStride = mMetadata.frameWidth * mMetadata.lumaChannelBpp / 8.0;
  b.mPlanes[0].mHeight = mMetadata.frameHeight;
  b.mPlanes[0].mWidth = mMetadata.frameWidth;

  PRUint32 cbcrStride = mMetadata.frameWidth * mMetadata.chromaChannelBpp / 8.0;

  b.mPlanes[1].mData = buffer + mMetadata.frameHeight * b.mPlanes[0].mStride;
  b.mPlanes[1].mStride = cbcrStride;
  b.mPlanes[1].mHeight = mMetadata.frameHeight / 2;
  b.mPlanes[1].mWidth = mMetadata.frameWidth / 2;

  b.mPlanes[2].mData = b.mPlanes[1].mData + mMetadata.frameHeight * cbcrStride / 2;
  b.mPlanes[2].mStride = cbcrStride;
  b.mPlanes[2].mHeight = mMetadata.frameHeight / 2;
  b.mPlanes[2].mWidth = mMetadata.frameWidth / 2;

  VideoData *v = VideoData::Create(mInfo,
                                   mDecoder->GetImageContainer(),
                                   -1,
                                   currentFrameTime,
                                   currentFrameTime + (USECS_PER_S / mFrameRate),
                                   b,
                                   1, 
                                   -1);
  if (!v)
    return PR_FALSE;

  mVideoQueue.Push(v);
  mCurrentFrame++;
  decoded++;
  currentFrameTime += USECS_PER_S / mFrameRate;

  return PR_TRUE;
}

nsresult nsRawReader::Seek(PRInt64 aTime, PRInt64 aStartTime, PRInt64 aEndTime, PRInt64 aCurrentTime)
{
  mozilla::MonitorAutoEnter autoEnter(mMonitor);
  NS_ASSERTION(mDecoder->OnStateMachineThread(),
               "Should be on state machine thread.");

  nsMediaStream *stream = mDecoder->GetCurrentStream();
  NS_ASSERTION(stream, "Decoder has no media stream");

  PRUint32 frame = mCurrentFrame;
  if (aTime >= UINT_MAX)
    return NS_ERROR_FAILURE;
  mCurrentFrame = aTime * mFrameRate / USECS_PER_S;

  PRUint32 offset;
  if (!MulOverflow32(mCurrentFrame, mFrameSize, offset))
    return NS_ERROR_FAILURE;

  offset += sizeof(nsRawVideoHeader);

  nsresult rv = stream->Seek(nsISeekableStream::NS_SEEK_SET, offset);
  NS_ENSURE_SUCCESS(rv, rv);

  mVideoQueue.Erase();

  while(mVideoQueue.GetSize() == 0) {
    PRBool keyframeSkip = PR_FALSE;
    if (!DecodeVideoFrame(keyframeSkip, 0)) {
      mCurrentFrame = frame;
      return NS_ERROR_FAILURE;
    }

    {
      mozilla::MonitorAutoExit autoMonitorExit(mMonitor);
      mozilla::MonitorAutoEnter autoMonitor(mDecoder->GetMonitor());
      if (mDecoder->GetDecodeState() ==
          nsBuiltinDecoderStateMachine::DECODER_STATE_SHUTDOWN) {
        mCurrentFrame = frame;
        return NS_ERROR_FAILURE;
      }
    }

    nsAutoPtr<VideoData> video(mVideoQueue.PeekFront());
    if (video && video->mEndTime < aTime) {
      mVideoQueue.PopFront();
      video = nsnull;
    } else {
      video.forget();
    }
  }

  return NS_OK;
}

nsresult nsRawReader::GetBuffered(nsTimeRanges* aBuffered, PRInt64 aStartTime)
{
  return NS_OK;
}
