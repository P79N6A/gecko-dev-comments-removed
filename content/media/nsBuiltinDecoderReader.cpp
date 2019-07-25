






































#include "nsISeekableStream.h"
#include "nsClassHashtable.h"
#include "nsTArray.h"
#include "nsBuiltinDecoder.h"
#include "nsBuiltinDecoderReader.h"
#include "nsBuiltinDecoderStateMachine.h"
#include "mozilla/mozalloc.h"
#include "VideoUtils.h"

using namespace mozilla;




#ifdef PR_LOGGING
extern PRLogModuleInfo* gBuiltinDecoderLog;
#define LOG(type, msg) PR_LOG(gBuiltinDecoderLog, type, msg)
#ifdef SEEK_LOGGING
#define SEEK_LOG(type, msg) PR_LOG(gBuiltinDecoderLog, type, msg)
#else
#define SEEK_LOG(type, msg)
#endif
#else
#define LOG(type, msg)
#define SEEK_LOG(type, msg)
#endif




PRBool MulOverflow32(PRUint32 a, PRUint32 b, PRUint32& aResult) {
  PRUint64 a64 = a;
  PRUint64 b64 = b;
  PRUint64 r64 = a64 * b64;
  if (r64 > PR_UINT32_MAX)
    return PR_FALSE;
  aResult = static_cast<PRUint32>(r64);
  return PR_TRUE;
}

VideoData* VideoData::Create(PRInt64 aOffset,
                             PRInt64 aTime,
                             const YCbCrBuffer &aBuffer,
                             PRBool aKeyframe,
                             PRInt64 aTimecode)
{
  nsAutoPtr<VideoData> v(new VideoData(aOffset, aTime, aKeyframe, aTimecode));
  for (PRUint32 i=0; i < 3; ++i) {
    PRUint32 size = 0;
    if (!MulOverflow32(PR_ABS(aBuffer.mPlanes[i].mHeight),
                       PR_ABS(aBuffer.mPlanes[i].mStride),
                       size))
    {
      
      
      continue;
    }
    unsigned char* p = static_cast<unsigned char*>(moz_xmalloc(size));
    if (!p) {
      NS_WARNING("Failed to allocate memory for video frame");
      return nsnull;
    }
    v->mBuffer.mPlanes[i].mData = p;
    v->mBuffer.mPlanes[i].mWidth = aBuffer.mPlanes[i].mWidth;
    v->mBuffer.mPlanes[i].mHeight = aBuffer.mPlanes[i].mHeight;
    v->mBuffer.mPlanes[i].mStride = aBuffer.mPlanes[i].mStride;
    memcpy(v->mBuffer.mPlanes[i].mData, aBuffer.mPlanes[i].mData, size);
  }
  return v.forget();
}

nsBuiltinDecoderReader::nsBuiltinDecoderReader(nsBuiltinDecoder* aDecoder)
  : mMonitor("media.decoderreader"),
    mDecoder(aDecoder),
    mDataOffset(0)
{
  MOZ_COUNT_CTOR(nsBuiltinDecoderReader);
}

nsBuiltinDecoderReader::~nsBuiltinDecoderReader()
{
  ResetDecode();
  MOZ_COUNT_DTOR(nsBuiltinDecoderReader);
}

nsresult nsBuiltinDecoderReader::ResetDecode()
{
  nsresult res = NS_OK;

  mVideoQueue.Reset();
  mAudioQueue.Reset();

  return res;
}

nsresult nsBuiltinDecoderReader::GetBufferedBytes(nsTArray<ByteRange>& aRanges)
{
  NS_ASSERTION(mDecoder->OnStateMachineThread(),
               "Should be on state machine thread.");
  mMonitor.AssertCurrentThreadIn();
  PRInt64 startOffset = mDataOffset;
  nsMediaStream* stream = mDecoder->GetCurrentStream();
  while (PR_TRUE) {
    PRInt64 endOffset = stream->GetCachedDataEnd(startOffset);
    if (endOffset == startOffset) {
      
      endOffset = stream->GetNextCachedData(startOffset);
      if (endOffset == -1) {
        
        
        break;
      }
    } else {
      
      PRInt64 startTime = -1;
      PRInt64 endTime = -1;
      if (NS_FAILED(ResetDecode())) {
        return NS_ERROR_FAILURE;
      }
      FindStartTime(startOffset, startTime);
      if (startTime != -1 &&
          (endTime = FindEndTime(endOffset) != -1))
      {
        aRanges.AppendElement(ByteRange(startOffset,
                                        endOffset,
                                        startTime,
                                        endTime));
      }
    }
    startOffset = endOffset;
  }
  if (NS_FAILED(ResetDecode())) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

ByteRange
nsBuiltinDecoderReader::GetSeekRange(const nsTArray<ByteRange>& ranges,
                                     PRInt64 aTarget,
                                     PRInt64 aStartTime,
                                     PRInt64 aEndTime,
                                     PRBool aExact)
{
  NS_ASSERTION(mDecoder->OnStateMachineThread(),
               "Should be on state machine thread.");
  PRInt64 so = mDataOffset;
  PRInt64 eo = mDecoder->GetCurrentStream()->GetLength();
  PRInt64 st = aStartTime;
  PRInt64 et = aEndTime;
  for (PRUint32 i = 0; i < ranges.Length(); i++) {
    const ByteRange &r = ranges[i];
    if (r.mTimeStart < aTarget) {
      so = r.mOffsetStart;
      st = r.mTimeStart;
    }
    if (r.mTimeEnd >= aTarget && r.mTimeEnd < et) {
      eo = r.mOffsetEnd;
      et = r.mTimeEnd;
    }

    if (r.mTimeStart < aTarget && aTarget <= r.mTimeEnd) {
      
      return ranges[i];
    }
  }
  return aExact ? ByteRange() : ByteRange(so, eo, st, et);
}

VideoData* nsBuiltinDecoderReader::FindStartTime(PRInt64 aOffset,
                                      PRInt64& aOutStartTime)
{
  NS_ASSERTION(mDecoder->OnStateMachineThread(), "Should be on state machine thread.");

  nsMediaStream* stream = mDecoder->GetCurrentStream();

  stream->Seek(nsISeekableStream::NS_SEEK_SET, aOffset);
  if (NS_FAILED(ResetDecode())) {
    return nsnull;
  }

  
  
  PRInt64 videoStartTime = PR_INT64_MAX;
  PRInt64 audioStartTime = PR_INT64_MAX;
  VideoData* videoData = nsnull;

  if (HasVideo()) {
    videoData = DecodeToFirstData(&nsBuiltinDecoderReader::DecodeVideoFrame,
                                  mVideoQueue);
    if (videoData) {
      videoStartTime = videoData->mTime;
    }
  }
  if (HasAudio()) {
    SoundData* soundData = DecodeToFirstData(&nsBuiltinDecoderReader::DecodeAudioData,
                                             mAudioQueue);
    if (soundData) {
      audioStartTime = soundData->mTime;
    }
  }

  PRInt64 startTime = PR_MIN(videoStartTime, audioStartTime);
  if (startTime != PR_INT64_MAX) {
    aOutStartTime = startTime;
  }

  return videoData;
}

template<class Data>
Data* nsBuiltinDecoderReader::DecodeToFirstData(DecodeFn aDecodeFn,
                                                MediaQueue<Data>& aQueue)
{
  PRBool eof = PR_FALSE;
  while (!eof && aQueue.GetSize() == 0) {
    {
      MonitorAutoEnter decoderMon(mDecoder->GetMonitor());
      if (mDecoder->GetDecodeState() == nsDecoderStateMachine::DECODER_STATE_SHUTDOWN) {
        return nsnull;
      }
    }
    eof = !(this->*aDecodeFn)();
  }
  Data* d = nsnull;
  return (d = aQueue.PeekFront()) ? d : nsnull;
}


