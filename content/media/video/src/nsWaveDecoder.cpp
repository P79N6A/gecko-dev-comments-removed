




































#include "limits"
#include "prlog.h"
#include "prmem.h"
#include "nsIDOMHTMLMediaElement.h"
#include "nsIDocument.h"
#include "nsIFrame.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsISeekableStream.h"
#include "nsAudioStream.h"
#include "nsAutoLock.h"
#include "nsHTMLMediaElement.h"
#include "nsNetUtil.h"
#include "nsThreadUtils.h"
#include "nsWaveDecoder.h"


#define BUFFERING_TIMEOUT 3






#define AUDIO_BUFFER_WAKEUP 100
#define AUDIO_BUFFER_LENGTH (2 * AUDIO_BUFFER_WAKEUP)


#define RIFF_CHUNK_MAGIC 0x52494646
#define WAVE_CHUNK_MAGIC 0x57415645
#define FRMT_CHUNK_MAGIC 0x666d7420
#define DATA_CHUNK_MAGIC 0x64617461


#define RIFF_CHUNK_HEADER_SIZE 8


#define RIFF_INITIAL_SIZE (RIFF_CHUNK_HEADER_SIZE + 4)



#define WAVE_FORMAT_CHUNK_SIZE 16



#define WAVE_FORMAT_ENCODING_PCM 1

enum State {
  STATE_LOADING_METADATA,
  STATE_BUFFERING,
  STATE_PLAYING,
  STATE_SEEKING,
  STATE_PAUSED,
  STATE_ENDED,
  STATE_ERROR,
  STATE_SHUTDOWN
};


















class nsWaveStateMachine : public nsRunnable
{
public:
  nsWaveStateMachine(nsWaveDecoder* aDecoder, nsMediaStream* aStream,
                     PRUint32 aBufferWaitTime, float aInitialVolume);
  ~nsWaveStateMachine();

  
  
  void SetVolume(float aVolume);

  



  void Play();
  void Pause();
  void Seek(float aTime);
  void Shutdown();

  
  
  
  float GetDuration();

  
  
  float GetCurrentTime();

  
  PRBool IsSeeking();

  
  PRBool IsEnded();

  
  NS_IMETHOD Run();

  
  nsMediaDecoder::Statistics GetStatistics();

  
  void SetTotalBytes(PRInt64 aBytes);
  
  void NotifyBytesDownloaded(PRInt64 aBytes);
  
  void NotifyDownloadSeeked(PRInt64 aOffset);
  
  void NotifyDownloadEnded(nsresult aStatus);
  
  void NotifyBytesConsumed(PRInt64 aBytes);

  
  nsHTMLMediaElement::NextFrameStatus GetNextFrameStatus();

  
  
  
  float GetTimeForPositionChange();

private:
  
  PRBool IsShutdown();

  
  
  
  PRBool ReadAll(char* aBuf, PRInt64 aSize, PRInt64* aBytesRead);

  
  
  
  void ChangeState(State aState);

  
  void OpenAudioStream();

  
  void CloseAudioStream();

  
  
  PRBool LoadRIFFChunk();

  
  
  
  PRBool ScanForwardUntil(PRUint32 aWantedChunk, PRUint32* aChunkSize);

  
  
  
  PRBool LoadFormatChunk();

  
  
  
  PRBool FindDataOffset();

  
  PRInt64 GetDataLength();

  
  
  
  void FirePositionChanged(PRBool aCoalesce);

  
  
  
  float BytesToTime(PRInt64 aBytes) const
  {
    NS_ABORT_IF_FALSE(mMetadataValid, "Requires valid metadata");
    NS_ABORT_IF_FALSE(aBytes >= 0, "Must be >= 0");
    return float(aBytes) / mSampleRate / mSampleSize;
  }

  
  
  
  PRInt64 TimeToBytes(float aTime) const
  {
    NS_ABORT_IF_FALSE(mMetadataValid, "Requires valid metadata");
    NS_ABORT_IF_FALSE(aTime >= 0.0, "Must be >= 0");
    return RoundDownToSample(PRInt64(aTime * mSampleRate * mSampleSize));
  }

  
  
  PRInt64 RoundDownToSample(PRInt64 aBytes) const
  {
    NS_ABORT_IF_FALSE(mMetadataValid, "Requires valid metadata");
    NS_ABORT_IF_FALSE(aBytes >= 0, "Must be >= 0");
    return aBytes - (aBytes % mSampleSize);
  }

  
  
  
  
  
  nsWaveDecoder* mDecoder;

  
  
  
  
  
  nsMediaStream* mStream;

  
  
  
  nsAutoPtr<nsAudioStream> mAudioStream;

  
  PRUint32 mBufferingWait;

  
  
  PRIntervalTime mBufferingStart;

  
  
  PRInt64 mBufferingEndOffset;

  




  
  PRUint32 mSampleRate;

  
  PRUint32 mChannels;

  
  
  PRUint32 mSampleSize;

  
  nsAudioStream::SampleFormat mSampleFormat;

  
  
  PRInt64 mWaveLength;

  
  
  PRInt64 mWavePCMOffset;

  



  PRMonitor* mMonitor;

  
  State mState;

  
  
  
  
  State mNextState;

  
  PRInt64 mTotalBytes;
  
  
  
  
  PRInt64 mDownloadPosition;
  
  PRInt64 mPlaybackPosition;
  
  
  nsMediaDecoder::ChannelStatistics mDownloadStatistics;

  
  float mInitialVolume;

  
  
  PRInt64 mTimeOffset;

  
  float mSeekTime;

  
  
  
  PRPackedBool mMetadataValid;

  
  
  
  
  PRPackedBool mPositionChangeQueued;

  
  PRPackedBool mPaused;
};

nsWaveStateMachine::nsWaveStateMachine(nsWaveDecoder* aDecoder, nsMediaStream* aStream,
                                       PRUint32 aBufferWaitTime, float aInitialVolume)
  : mDecoder(aDecoder),
    mStream(aStream),
    mBufferingWait(aBufferWaitTime),
    mBufferingStart(0),
    mBufferingEndOffset(0),
    mSampleRate(0),
    mChannels(0),
    mSampleSize(0),
    mSampleFormat(nsAudioStream::FORMAT_S16_LE),
    mWaveLength(0),
    mWavePCMOffset(0),
    mMonitor(nsnull),
    mState(STATE_LOADING_METADATA),
    mNextState(STATE_PAUSED),
    mTotalBytes(-1),
    mDownloadPosition(0),
    mPlaybackPosition(0),
    mInitialVolume(aInitialVolume),
    mTimeOffset(0),
    mSeekTime(0.0),
    mMetadataValid(PR_FALSE),
    mPositionChangeQueued(PR_FALSE),
    mPaused(mNextState == STATE_PAUSED)
{
  mMonitor = nsAutoMonitor::NewMonitor("nsWaveStateMachine");
  mDownloadStatistics.Start(PR_IntervalNow());
}

nsWaveStateMachine::~nsWaveStateMachine()
{
  nsAutoMonitor::DestroyMonitor(mMonitor);
}

void
nsWaveStateMachine::Shutdown()
{
  ChangeState(STATE_SHUTDOWN);
}

void
nsWaveStateMachine::Play()
{
  nsAutoMonitor monitor(mMonitor);
  mPaused = PR_FALSE;
  if (mState == STATE_LOADING_METADATA || mState == STATE_SEEKING) {
    mNextState = STATE_PLAYING;
  } else {
    ChangeState(STATE_PLAYING);
  }
}

void
nsWaveStateMachine::SetVolume(float aVolume)
{
  nsAutoMonitor monitor(mMonitor);
  mInitialVolume = aVolume;
  if (mAudioStream) {
    mAudioStream->SetVolume(aVolume);
  }
}

void
nsWaveStateMachine::Pause()
{
  nsAutoMonitor monitor(mMonitor);
  mPaused = PR_TRUE;
  if (mState == STATE_LOADING_METADATA || mState == STATE_SEEKING) {
    mNextState = STATE_PAUSED;
  } else {
    ChangeState(STATE_PAUSED);
  }
}

void
nsWaveStateMachine::Seek(float aTime)
{
  nsAutoMonitor monitor(mMonitor);
  mSeekTime = aTime;
  if (mSeekTime < 0.0) {
    mSeekTime = 0.0;
  }
  if (mState == STATE_LOADING_METADATA) {
    mNextState = STATE_SEEKING;
  } else if (mState != STATE_SEEKING) {
    mNextState = mState;
    ChangeState(STATE_SEEKING);
  }
}

float
nsWaveStateMachine::GetDuration()
{
  nsAutoMonitor monitor(mMonitor);
  if (mMetadataValid) {
    return BytesToTime(GetDataLength());
  }
  return std::numeric_limits<float>::quiet_NaN();
}

float
nsWaveStateMachine::GetCurrentTime()
{
  nsAutoMonitor monitor(mMonitor);
  if (mMetadataValid) {
    return BytesToTime(mTimeOffset);
  }
  return std::numeric_limits<float>::quiet_NaN();
}

PRBool
nsWaveStateMachine::IsSeeking()
{
  nsAutoMonitor monitor(mMonitor);
  return mState == STATE_SEEKING || mNextState == STATE_SEEKING;
}

PRBool
nsWaveStateMachine::IsEnded()
{
  nsAutoMonitor monitor(mMonitor);
  return mState == STATE_ENDED || mState == STATE_SHUTDOWN;
}

nsHTMLMediaElement::NextFrameStatus
nsWaveStateMachine::GetNextFrameStatus()
{
  nsAutoMonitor monitor(mMonitor);
  if (mState == STATE_BUFFERING)
    return nsHTMLMediaElement::NEXT_FRAME_UNAVAILABLE_BUFFERING;
  if (mPlaybackPosition < mDownloadPosition)
    return nsHTMLMediaElement::NEXT_FRAME_AVAILABLE;
  return nsHTMLMediaElement::NEXT_FRAME_UNAVAILABLE;
}

float
nsWaveStateMachine::GetTimeForPositionChange()
{
  nsAutoMonitor monitor(mMonitor);
  mPositionChangeQueued = PR_FALSE;
  return GetCurrentTime();
}

NS_IMETHODIMP
nsWaveStateMachine::Run()
{
  
  
  
  nsAutoMonitor monitor(mMonitor);

  for (;;) {
    switch (mState) {
    case STATE_LOADING_METADATA:
      {
        monitor.Exit();
        PRBool loaded = LoadRIFFChunk() && LoadFormatChunk() && FindDataOffset();
        monitor.Enter();

        if (mState == STATE_LOADING_METADATA) {
          nsCOMPtr<nsIRunnable> event;
          State newState;

          if (loaded) {
            mMetadataValid = PR_TRUE;
            if (mNextState != STATE_SEEKING) {
              event = NS_NEW_RUNNABLE_METHOD(nsWaveDecoder, mDecoder, MetadataLoaded);
            }
            newState = mNextState;
          } else {
            event = NS_NEW_RUNNABLE_METHOD(nsWaveDecoder, mDecoder, MediaErrorDecode);
            newState = STATE_ERROR;
          }

          if (event) {
            NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
          }
          ChangeState(newState);
        }
      }
      break;

    case STATE_BUFFERING: {
      PRIntervalTime now = PR_IntervalNow();
      if ((PR_IntervalToMilliseconds(now - mBufferingStart) < mBufferingWait) &&
          mDownloadPosition < mBufferingEndOffset &&
          (mTotalBytes < 0 || mDownloadPosition < mTotalBytes)) {
        LOG(PR_LOG_DEBUG,
            ("In buffering: buffering data until %d bytes available or %d milliseconds\n",
             PRUint32(mBufferingEndOffset - mDownloadPosition),
             mBufferingWait - (PR_IntervalToMilliseconds(now - mBufferingStart))));
        monitor.Wait(PR_MillisecondsToInterval(1000));
      } else {
        ChangeState(mNextState);
        nsCOMPtr<nsIRunnable> event =
          NS_NEW_RUNNABLE_METHOD(nsWaveDecoder, mDecoder, UpdateReadyStateForData);
        NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
      }

      break;
    }

    case STATE_PLAYING: {
      if (!mAudioStream) {
        OpenAudioStream();
        if (!mAudioStream) {
          ChangeState(STATE_ERROR);
          break;
        }
      }

      PRUint32 startTime = PR_IntervalToMilliseconds(PR_IntervalNow());
      startTime -= AUDIO_BUFFER_LENGTH;
      PRIntervalTime lastWakeup = PR_MillisecondsToInterval(startTime);

      do {
        PRIntervalTime now = PR_IntervalNow();
        PRInt32 sleepTime = PR_IntervalToMilliseconds(now - lastWakeup);
        lastWakeup = now;

        
        
        
        
        
        
        PRInt32 targetTime = AUDIO_BUFFER_LENGTH;
        if (sleepTime < targetTime) {
          targetTime = sleepTime;
        }

        PRInt64 len = TimeToBytes(float(targetTime) / 1000.0f);

        PRInt64 leftToPlay = GetDataLength() - mTimeOffset;
        if (leftToPlay <= len) {
          len = leftToPlay;
          ChangeState(STATE_ENDED);
        }

        PRInt64 available = mDownloadPosition - mPlaybackPosition;

        
        if (mState != STATE_ENDED && available < len) {
            mBufferingStart = PR_IntervalNow();
            mBufferingEndOffset = mDownloadPosition + TimeToBytes(float(mBufferingWait) / 1000.0f);
            mNextState = mState;
            ChangeState(STATE_BUFFERING);

            nsCOMPtr<nsIRunnable> event =
              NS_NEW_RUNNABLE_METHOD(nsWaveDecoder, mDecoder, UpdateReadyStateForData);
            NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
            break;
        }

        if (len > 0) {
          nsAutoArrayPtr<char> buf(new char[size_t(len)]);
          PRInt64 got = 0;

          monitor.Exit();
          PRBool ok = ReadAll(buf.get(), len, &got);
          PRInt64 streamPos = mStream->Tell();
          monitor.Enter();

          
          if (!ok) {
            ChangeState(STATE_ENDED);
            if (got == 0) {
              break;
            }
          }

          
          
          PRInt64 endDelta = mWavePCMOffset + mWaveLength - streamPos;
          if (endDelta < 0) {
            
            
            got -= -endDelta;
            ChangeState(STATE_ENDED);
          }

          if (mState == STATE_ENDED) {
            got = RoundDownToSample(got);
          }

          PRUint32 sampleSize = mSampleFormat == nsAudioStream::FORMAT_U8 ? 1 : 2;
          NS_ABORT_IF_FALSE(got % sampleSize == 0, "Must write complete samples");
          PRUint32 lengthInSamples = got / sampleSize;

          monitor.Exit();
          mAudioStream->Write(buf.get(), lengthInSamples);
          monitor.Enter();

          mTimeOffset += got;
          FirePositionChanged(PR_FALSE);
        }

        if (mState == STATE_PLAYING) {
          monitor.Wait(PR_MillisecondsToInterval(AUDIO_BUFFER_WAKEUP));
        }
      } while (mState == STATE_PLAYING);
      break;
    }

    case STATE_SEEKING:
      {
        CloseAudioStream();

        mSeekTime = NS_MIN(mSeekTime, GetDuration());
        float seekTime = mSeekTime;

        monitor.Exit();
        nsCOMPtr<nsIRunnable> startEvent =
          NS_NEW_RUNNABLE_METHOD(nsWaveDecoder, mDecoder, SeekingStarted);
        NS_DispatchToMainThread(startEvent, NS_DISPATCH_SYNC);
        monitor.Enter();

        if (mState == STATE_SHUTDOWN) {
          break;
        }

        
        PRInt64 position = RoundDownToSample(TimeToBytes(seekTime));
        NS_ABORT_IF_FALSE(position >= 0 && position <= GetDataLength(), "Invalid seek position");

        mTimeOffset = position;

        
        
        
        
        
        
        PRBool seekToZeroFirst = position == 0 &&
                                 (mWavePCMOffset < SEEK_VS_READ_THRESHOLD);

        
        position += mWavePCMOffset;

        monitor.Exit();
        nsresult rv;
        if (seekToZeroFirst) {
          rv = mStream->Seek(nsISeekableStream::NS_SEEK_SET, 0);
          if (NS_FAILED(rv)) {
            NS_WARNING("Seek to zero failed");
          }
        }
        rv = mStream->Seek(nsISeekableStream::NS_SEEK_SET, position);
        if (NS_FAILED(rv)) {
          NS_WARNING("Seek failed");
        }
        monitor.Enter();

        if (mState == STATE_SHUTDOWN) {
          break;
        }

        if (mState == STATE_SEEKING && mSeekTime == seekTime) {
          
          
          
          
          
          
          
          
          
          
          State nextState = mNextState;
          if (nextState == STATE_SEEKING) {
            nextState = STATE_PAUSED;
          } else if (nextState == STATE_ENDED) {
            nextState = mPaused ? STATE_PAUSED : STATE_PLAYING;
          }
          ChangeState(nextState);
        }

        FirePositionChanged(PR_TRUE);

        monitor.Exit();
        nsCOMPtr<nsIRunnable> stopEvent =
          NS_NEW_RUNNABLE_METHOD(nsWaveDecoder, mDecoder, SeekingStopped);
        NS_DispatchToMainThread(stopEvent, NS_DISPATCH_SYNC);
        monitor.Enter();
      }
      break;

    case STATE_PAUSED:
      monitor.Wait();
      break;

    case STATE_ENDED:
      FirePositionChanged(PR_TRUE);

      if (mAudioStream) {
        monitor.Exit();
        mAudioStream->Drain();
        monitor.Enter();
      }

      if (mState == STATE_ENDED) {
        nsCOMPtr<nsIRunnable> event =
          NS_NEW_RUNNABLE_METHOD(nsWaveDecoder, mDecoder, PlaybackEnded);
        NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);

        do {
          monitor.Wait();
        } while (mState == STATE_ENDED);
      }
      break;

    case STATE_ERROR:
      monitor.Wait();
      if (mState != STATE_SHUTDOWN) {
        NS_WARNING("Invalid state transition");
        ChangeState(STATE_ERROR);
      }
      break;

    case STATE_SHUTDOWN:
      CloseAudioStream();
      return NS_OK;
    }
  }

  return NS_OK;
}

#if defined(DEBUG)
static PRBool
IsValidStateTransition(State aStartState, State aEndState)
{
  if (aEndState == STATE_SHUTDOWN) {
    return PR_TRUE;
  }

  if (aStartState == aEndState) {
    LOG(PR_LOG_WARNING, ("Transition to current state requested"));
    return PR_TRUE;
  }

  switch (aStartState) {
  case STATE_LOADING_METADATA:
    if (aEndState == STATE_PLAYING || aEndState == STATE_SEEKING ||
        aEndState == STATE_PAUSED || aEndState == STATE_ERROR)
      return PR_TRUE;
    break;
  case STATE_BUFFERING:
    if (aEndState == STATE_PLAYING || aEndState == STATE_PAUSED ||
        aEndState == STATE_SEEKING)
      return PR_TRUE;
    break;
  case STATE_PLAYING:
    if (aEndState == STATE_BUFFERING || aEndState == STATE_SEEKING ||
        aEndState == STATE_ENDED || aEndState == STATE_PAUSED)
      return PR_TRUE;
    break;
  case STATE_SEEKING:
    if (aEndState == STATE_PLAYING || aEndState == STATE_PAUSED)
      return PR_TRUE;
    break;
  case STATE_PAUSED:
    if (aEndState == STATE_PLAYING || aEndState == STATE_SEEKING)
      return PR_TRUE;
    break;
  case STATE_ENDED:
    if (aEndState == STATE_SEEKING)
      return PR_TRUE;
    
  case STATE_ERROR:
  case STATE_SHUTDOWN:
    break;
  }

  LOG(PR_LOG_ERROR, ("Invalid state transition from %d to %d", aStartState, aEndState));
  return PR_FALSE;
}
#endif

void
nsWaveStateMachine::ChangeState(State aState)
{
  nsAutoMonitor monitor(mMonitor);
  if (mState == STATE_SHUTDOWN) {
    LOG(PR_LOG_WARNING, ("In shutdown, state transition ignored"));
    return;
  }
#if defined(DEBUG)
  NS_ABORT_IF_FALSE(IsValidStateTransition(mState, aState), "Invalid state transition");
#endif
  mState = aState;
  monitor.NotifyAll();
}

void
nsWaveStateMachine::OpenAudioStream()
{
  mAudioStream = new nsAudioStream();
  if (!mAudioStream) {
    LOG(PR_LOG_ERROR, ("Could not create audio stream"));
  } else {
    NS_ABORT_IF_FALSE(mMetadataValid,
                      "Attempting to initialize audio stream with invalid metadata");
    mAudioStream->Init(mChannels, mSampleRate, mSampleFormat);
    mAudioStream->SetVolume(mInitialVolume);
  }
}

void
nsWaveStateMachine::CloseAudioStream()
{
  if (mAudioStream) {
    mAudioStream->Shutdown();
    mAudioStream = nsnull;
  }
}

nsMediaDecoder::Statistics
nsWaveStateMachine::GetStatistics()
{
  nsMediaDecoder::Statistics result;
  nsAutoMonitor monitor(mMonitor);
  PRIntervalTime now = PR_IntervalNow();
  result.mDownloadRate = mDownloadStatistics.GetRate(now, &result.mDownloadRateReliable);
  result.mPlaybackRate = mSampleRate*mChannels*mSampleSize;
  result.mPlaybackRateReliable = PR_TRUE;
  result.mTotalBytes = mTotalBytes;
  result.mDownloadPosition = mDownloadPosition;
  result.mDecoderPosition = mPlaybackPosition;
  result.mPlaybackPosition = mPlaybackPosition;
  return result;
}

void
nsWaveStateMachine::SetTotalBytes(PRInt64 aBytes)
{
  nsAutoMonitor monitor(mMonitor);
  mTotalBytes = aBytes;
}

void
nsWaveStateMachine::NotifyBytesDownloaded(PRInt64 aBytes)
{
  nsAutoMonitor monitor(mMonitor);
  mDownloadStatistics.AddBytes(aBytes);
  mDownloadPosition += aBytes;
}

void
nsWaveStateMachine::NotifyDownloadSeeked(PRInt64 aOffset)
{
  nsAutoMonitor monitor(mMonitor);
  mDownloadPosition = mPlaybackPosition = aOffset;
}

void
nsWaveStateMachine::NotifyDownloadEnded(nsresult aStatus)
{
  if (aStatus == NS_BINDING_ABORTED)
    return;
  nsAutoMonitor monitor(mMonitor);
  mDownloadStatistics.Stop(PR_IntervalNow());
}

void
nsWaveStateMachine::NotifyBytesConsumed(PRInt64 aBytes)
{
  nsAutoMonitor monitor(mMonitor);
  mPlaybackPosition += aBytes;
}

static PRUint32
ReadUint32BE(const char** aBuffer)
{
  PRUint32 result =
    PRUint8((*aBuffer)[0]) << 24 |
    PRUint8((*aBuffer)[1]) << 16 |
    PRUint8((*aBuffer)[2]) << 8 |
    PRUint8((*aBuffer)[3]);
  *aBuffer += sizeof(PRUint32);
  return result;
}

static PRUint32
ReadUint32LE(const char** aBuffer)
{
  PRUint32 result =
    PRUint8((*aBuffer)[3]) << 24 |
    PRUint8((*aBuffer)[2]) << 16 |
    PRUint8((*aBuffer)[1]) << 8 |
    PRUint8((*aBuffer)[0]);
  *aBuffer += sizeof(PRUint32);
  return result;
}

static PRUint16
ReadUint16LE(const char** aBuffer)
{
  PRUint16 result =
    PRUint8((*aBuffer)[1]) << 8 |
    PRUint8((*aBuffer)[0]) << 0;
  *aBuffer += sizeof(PRUint16);
  return result;
}

PRBool
nsWaveStateMachine::IsShutdown()
{
  nsAutoMonitor monitor(mMonitor);
  return mState == STATE_SHUTDOWN;
}

PRBool
nsWaveStateMachine::ReadAll(char* aBuf, PRInt64 aSize, PRInt64* aBytesRead = nsnull)
{
  PRUint32 got = 0;
  if (aBytesRead) {
    *aBytesRead = 0;
  }
  do {
    PRUint32 read = 0;
    if (NS_FAILED(mStream->Read(aBuf + got, aSize - got, &read))) {
      NS_WARNING("Stream read failed");
      return PR_FALSE;
    }
    if (IsShutdown() || read == 0) {
      return PR_FALSE;
    }
    got += read;
    if (aBytesRead) {
      *aBytesRead = got;
    }
  } while (got != aSize);
  return PR_TRUE;
}

PRBool
nsWaveStateMachine::LoadRIFFChunk()
{
  char riffHeader[RIFF_INITIAL_SIZE];
  const char* p = riffHeader;

  NS_ABORT_IF_FALSE(mStream->Tell() == 0,
                    "LoadRIFFChunk called when stream in invalid state");

  if (!ReadAll(riffHeader, sizeof(riffHeader))) {
    return PR_FALSE;
  }

  if (ReadUint32BE(&p) != RIFF_CHUNK_MAGIC) {
    NS_WARNING("Stream data not in RIFF format");
    return PR_FALSE;
  }

  
  p += 4;

  if (ReadUint32BE(&p) != WAVE_CHUNK_MAGIC) {
    NS_WARNING("Expected WAVE chunk");
    return PR_FALSE;
  }

  return PR_TRUE;
}

PRBool
nsWaveStateMachine::ScanForwardUntil(PRUint32 aWantedChunk, PRUint32* aChunkSize)
{
  NS_ABORT_IF_FALSE(aChunkSize, "Require aChunkSize argument");
  *aChunkSize = 0;

  for (;;) {
    char chunkHeader[8];
    const char* p = chunkHeader;

    if (!ReadAll(chunkHeader, sizeof(chunkHeader))) {
      return PR_FALSE;
    }

    PRUint32 magic = ReadUint32BE(&p);
    PRUint32 size = ReadUint32LE(&p);

    if (magic == aWantedChunk) {
      *aChunkSize = size;
      return PR_TRUE;
    }

    
    size += size % 2;

    nsAutoArrayPtr<char> chunk(new char[size]);
    if (!ReadAll(chunk.get(), size)) {
      return PR_FALSE;
    }
  }
}

PRBool
nsWaveStateMachine::LoadFormatChunk()
{
  PRUint32 fmtSize, rate, channels, sampleSize, sampleFormat;
  char waveFormat[WAVE_FORMAT_CHUNK_SIZE];
  const char* p = waveFormat;

  
  NS_ABORT_IF_FALSE(mStream->Tell() % 2 == 0,
                    "LoadFormatChunk called with unaligned stream");

  
  
  if (!ScanForwardUntil(FRMT_CHUNK_MAGIC, &fmtSize)) {
      return PR_FALSE;
  }

  if (!ReadAll(waveFormat, sizeof(waveFormat))) {
    return PR_FALSE;
  }

  if (ReadUint16LE(&p) != WAVE_FORMAT_ENCODING_PCM) {
    NS_WARNING("WAVE is not uncompressed PCM, compressed encodings are not supported");
    return PR_FALSE;
  }

  channels = ReadUint16LE(&p);
  rate = ReadUint32LE(&p);

  
  p += 4;

  sampleSize = ReadUint16LE(&p);

  sampleFormat = ReadUint16LE(&p);

  
  
  
  
  
  if (fmtSize > WAVE_FORMAT_CHUNK_SIZE) {
    char extLength[2];
    const char* p = extLength;

    if (!ReadAll(extLength, sizeof(extLength))) {
      return PR_FALSE;
    }

    PRUint16 extra = ReadUint16LE(&p);
    if (fmtSize - (WAVE_FORMAT_CHUNK_SIZE + 2) != extra) {
      NS_WARNING("Invalid extended format chunk size");
      return PR_FALSE;
    }
    extra += extra % 2;

    if (extra > 0) {
      nsAutoArrayPtr<char> chunkExtension(new char[extra]);
      if (!ReadAll(chunkExtension.get(), extra)) {
        return PR_FALSE;
      }
    }
  }

  
  NS_ABORT_IF_FALSE(mStream->Tell() % 2 == 0,
                    "LoadFormatChunk left stream unaligned");

  
  
  
  if (rate < 100 || rate > 96000 ||
      channels < 1 || channels > 2 ||
      (sampleSize != 1 && sampleSize != 2 && sampleSize != 4) ||
      (sampleFormat != 8 && sampleFormat != 16)) {
    NS_WARNING("Invalid WAVE metadata");
    return PR_FALSE;
  }

  nsAutoMonitor monitor(mMonitor);
  mSampleRate = rate;
  mChannels = channels;
  mSampleSize = sampleSize;
  if (sampleFormat == 8) {
    mSampleFormat = nsAudioStream::FORMAT_U8;
  } else {
    mSampleFormat = nsAudioStream::FORMAT_S16_LE;
  }
  return PR_TRUE;
}

PRBool
nsWaveStateMachine::FindDataOffset()
{
  PRUint32 length;
  PRInt64 offset;

  
  NS_ABORT_IF_FALSE(mStream->Tell() % 2 == 0,
                    "FindDataOffset called with unaligned stream");

  
  
  if (!ScanForwardUntil(DATA_CHUNK_MAGIC, &length)) {
    return PR_FALSE;
  }

  offset = mStream->Tell();
  if (!offset) {
    NS_WARNING("PCM data offset not found");
    return PR_FALSE;
  }

  if (offset < 0 || offset > PR_UINT32_MAX) {
    NS_WARNING("offset out of range");
    return PR_FALSE;
  }

  nsAutoMonitor monitor(mMonitor);
  mWaveLength = length;
  mWavePCMOffset = PRUint32(offset);
  return PR_TRUE;
}

PRInt64
nsWaveStateMachine::GetDataLength()
{
  NS_ABORT_IF_FALSE(mMetadataValid,
                    "Attempting to initialize audio stream with invalid metadata");

  PRInt64 length = mWaveLength;
  
  
  
  if (mTotalBytes >= 0 && mTotalBytes - mWavePCMOffset < length) {
    length = mTotalBytes - mWavePCMOffset;
  }
  return length;
}

void
nsWaveStateMachine::FirePositionChanged(PRBool aCoalesce)
{
  if (aCoalesce && mPositionChangeQueued) {
    return;
  }

  mPositionChangeQueued = PR_TRUE;
  nsCOMPtr<nsIRunnable> event = NS_NEW_RUNNABLE_METHOD(nsWaveDecoder, mDecoder, PlaybackPositionChanged);
  NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
}

NS_IMPL_THREADSAFE_ISUPPORTS1(nsWaveDecoder, nsIObserver)

nsWaveDecoder::nsWaveDecoder()
  : mInitialVolume(1.0),
    mStream(nsnull),
    mTimeOffset(0.0),
    mCurrentTime(0.0),
    mEndedCurrentTime(0.0),
    mEndedDuration(std::numeric_limits<float>::quiet_NaN()),
    mEnded(PR_FALSE),
    mNotifyOnShutdown(PR_FALSE),
    mSeekable(PR_TRUE),
    mResourceLoaded(PR_FALSE),
    mMetadataLoadedReported(PR_FALSE),
    mResourceLoadedReported(PR_FALSE)
{
  MOZ_COUNT_CTOR(nsWaveDecoder);
}

nsWaveDecoder::~nsWaveDecoder()
{
  MOZ_COUNT_DTOR(nsWaveDecoder);
}

void
nsWaveDecoder::GetCurrentURI(nsIURI** aURI)
{
  NS_IF_ADDREF(*aURI = mURI);
}

nsIPrincipal*
nsWaveDecoder::GetCurrentPrincipal()
{
  if (!mStream) {
    return nsnull;
  }
  return mStream->GetCurrentPrincipal();
}

float
nsWaveDecoder::GetCurrentTime()
{
  return mCurrentTime;
}

nsresult
nsWaveDecoder::Seek(float aTime)
{
  mTimeOffset = aTime;

  if (!mPlaybackStateMachine) {
    Load(mURI, nsnull, nsnull);
  }

  if (mPlaybackStateMachine) {
    mPlaybackStateMachine->Seek(mTimeOffset);
    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}

nsresult
nsWaveDecoder::PlaybackRateChanged()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

float
nsWaveDecoder::GetDuration()
{
  if (mPlaybackStateMachine) {
    return mPlaybackStateMachine->GetDuration();
  }
  return mEndedDuration;
}

void
nsWaveDecoder::Pause()
{
  if (mPlaybackStateMachine) {
    mPlaybackStateMachine->Pause();
  }
}

void
nsWaveDecoder::SetVolume(float aVolume)
{
  mInitialVolume = aVolume;
  if (mPlaybackStateMachine) {
    mPlaybackStateMachine->SetVolume(aVolume);
  }
}

nsresult
nsWaveDecoder::Play()
{
  if (!mPlaybackStateMachine) {
    Load(mURI, nsnull, nsnull);
  }

  if (mPlaybackStateMachine) {
    mPlaybackStateMachine->Play();
    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}

void
nsWaveDecoder::Stop()
{
  if (mStopping) {
    return;
  }

  mStopping = PR_TRUE;

  StopProgress();

  if (mPlaybackStateMachine) {
    mPlaybackStateMachine->Shutdown();
  }

  if (mStream) {
    mStream->Cancel();
  }

  if (mPlaybackThread) {
    mPlaybackThread->Shutdown();
  }

  if (mPlaybackStateMachine) {
    mEndedCurrentTime = mPlaybackStateMachine->GetCurrentTime();
    mEndedDuration = mPlaybackStateMachine->GetDuration();
    mEnded = mPlaybackStateMachine->IsEnded();
  }

  mPlaybackThread = nsnull;
  mPlaybackStateMachine = nsnull;
  mStream = nsnull;

  UnregisterShutdownObserver();
}

nsresult
nsWaveDecoder::Load(nsIURI* aURI, nsIChannel* aChannel, nsIStreamListener** aStreamListener)
{
  mStopping = PR_FALSE;

  
  mResourceLoaded = PR_FALSE;
  mResourceLoadedReported = PR_FALSE;
  mMetadataLoadedReported = PR_FALSE;

  if (aStreamListener) {
    *aStreamListener = nsnull;
  }

  if (aURI) {
    NS_ASSERTION(!aStreamListener, "No listener should be requested here");
    mURI = aURI;
  } else {
    NS_ASSERTION(aChannel, "Either a URI or a channel is required");
    NS_ASSERTION(aStreamListener, "A listener should be requested here");

    nsresult rv = NS_GetFinalChannelURI(aChannel, getter_AddRefs(mURI));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  RegisterShutdownObserver();

  mStream = new nsMediaStream();
  NS_ENSURE_TRUE(mStream, NS_ERROR_OUT_OF_MEMORY);

  mPlaybackStateMachine = new nsWaveStateMachine(this, mStream.get(),
                                                 BUFFERING_TIMEOUT * 1000,
                                                 mInitialVolume);
  NS_ENSURE_TRUE(mPlaybackStateMachine, NS_ERROR_OUT_OF_MEMORY);

  
  
  nsresult rv = mStream->Open(this, mURI, aChannel, aStreamListener);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = NS_NewThread(getter_AddRefs(mPlaybackThread));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPlaybackThread->Dispatch(mPlaybackStateMachine, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

void
nsWaveDecoder::MetadataLoaded()
{
  if (mShuttingDown) {
    return;
  }

  if (mElement) {
    mElement->MetadataLoaded();
    mElement->FirstFrameLoaded();
  }

  mMetadataLoadedReported = PR_TRUE;

  if (mResourceLoaded) {
    ResourceLoaded();
  } else {
    StartProgress();
  }
}

void
nsWaveDecoder::PlaybackEnded()
{
  if (mShuttingDown) {
    return;
  }

  if (!mPlaybackStateMachine->IsEnded()) {
    return;
  }

  Stop();
  if (mElement) {
    mElement->PlaybackEnded();
  }
}

void
nsWaveDecoder::ResourceLoaded()
{
  if (mShuttingDown) {
    return;
  }

  mResourceLoaded = PR_TRUE;

  if (!mMetadataLoadedReported || mResourceLoadedReported)
    return;

  StopProgress();

  if (mElement) {
    
    mElement->DispatchAsyncProgressEvent(NS_LITERAL_STRING("progress"));
    mElement->ResourceLoaded();
  }

  mResourceLoadedReported = PR_TRUE;
}

void
nsWaveDecoder::NetworkError()
{
  if (mShuttingDown) {
    return;
  }
  if (mElement) {
    mElement->NetworkError();
  }
  Stop();
}

PRBool
nsWaveDecoder::IsSeeking() const
{
  if (mPlaybackStateMachine) {
    return mPlaybackStateMachine->IsSeeking();
  }
  return PR_FALSE;
}

PRBool
nsWaveDecoder::IsEnded() const
{
  if (mPlaybackStateMachine) {
    return mPlaybackStateMachine->IsEnded();
  }
  return mEnded;
}

nsMediaDecoder::Statistics
nsWaveDecoder::GetStatistics()
{
  if (!mPlaybackStateMachine)
    return Statistics();
  return mPlaybackStateMachine->GetStatistics();
}

void
nsWaveDecoder::NotifyBytesDownloaded(PRInt64 aBytes)
{
  if (mPlaybackStateMachine) {
    mPlaybackStateMachine->NotifyBytesDownloaded(aBytes);
  }
  UpdateReadyStateForData();
}

void
nsWaveDecoder::NotifyDownloadSeeked(PRInt64 aBytes)
{
  if (mPlaybackStateMachine) {
    mPlaybackStateMachine->NotifyDownloadSeeked(aBytes);
  }
}

void
nsWaveDecoder::NotifyDownloadEnded(nsresult aStatus)
{
  if (mPlaybackStateMachine) {
    mPlaybackStateMachine->NotifyDownloadEnded(aStatus);
  }
  if (aStatus != NS_BINDING_ABORTED) {
    if (NS_SUCCEEDED(aStatus)) {
      ResourceLoaded();
    } else if (aStatus != NS_BASE_STREAM_CLOSED) {
      NetworkError();
    }
  }
  UpdateReadyStateForData();
}

void
nsWaveDecoder::NotifyBytesConsumed(PRInt64 aBytes)
{
  if (mPlaybackStateMachine) {
    mPlaybackStateMachine->NotifyBytesConsumed(aBytes);
  }
}

void
nsWaveDecoder::SetTotalBytes(PRInt64 aBytes)
{
  if (mPlaybackStateMachine) {
    mPlaybackStateMachine->SetTotalBytes(aBytes);
  } else {
    NS_WARNING("Forgot total bytes since there is no state machine set up");
  }
}







class nsWaveDecoderShutdown : public nsRunnable
{
public:
  nsWaveDecoderShutdown(nsWaveDecoder* aDecoder)
    : mDecoder(aDecoder)
  {
  }

  NS_IMETHOD Run()
  {
    mDecoder->Stop();
    return NS_OK;
  }

private:
  nsRefPtr<nsWaveDecoder> mDecoder;
};

void
nsWaveDecoder::Shutdown()
{
  mShuttingDown = PR_TRUE;

  nsMediaDecoder::Shutdown();

  nsCOMPtr<nsIRunnable> event = new nsWaveDecoderShutdown(this);
  NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
}

nsresult
nsWaveDecoder::Observe(nsISupports* aSubject, const char* aTopic, const PRUnichar* aData)
{
  if (strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID) == 0) {
    Shutdown();
  }
  return NS_OK;
}

void
nsWaveDecoder::UpdateReadyStateForData()
{
  if (!mElement || mShuttingDown || !mPlaybackStateMachine)
    return;

  nsHTMLMediaElement::NextFrameStatus frameStatus =
    mPlaybackStateMachine->GetNextFrameStatus();
  if (frameStatus == nsHTMLMediaElement::NEXT_FRAME_AVAILABLE &&
      !mMetadataLoadedReported) {
    frameStatus = nsHTMLMediaElement::NEXT_FRAME_UNAVAILABLE;
  }
  mElement->UpdateReadyStateForData(frameStatus);
}

void
nsWaveDecoder::SeekingStarted()
{
  if (mShuttingDown) {
    return;
  }

  if (mElement) {
    mElement->SeekStarted();
  }
}

void
nsWaveDecoder::SeekingStopped()
{
  if (mShuttingDown) {
    return;
  }

  if (mElement) {
    mElement->SeekCompleted();
    UpdateReadyStateForData();
  }
}

void
nsWaveDecoder::RegisterShutdownObserver()
{
  if (!mNotifyOnShutdown) {
    nsCOMPtr<nsIObserverService> observerService =
      do_GetService("@mozilla.org/observer-service;1");
    if (observerService) {
      mNotifyOnShutdown =
        NS_SUCCEEDED(observerService->AddObserver(this,
                                                  NS_XPCOM_SHUTDOWN_OBSERVER_ID,
                                                  PR_FALSE));
    } else {
      NS_WARNING("Could not get an observer service. Audio playback may not shutdown cleanly.");
    }
  }
}

void
nsWaveDecoder::UnregisterShutdownObserver()
{
  if (mNotifyOnShutdown) {
    nsCOMPtr<nsIObserverService> observerService =
      do_GetService("@mozilla.org/observer-service;1");
    if (observerService) {
      mNotifyOnShutdown = PR_FALSE;
      observerService->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
    }
  }
}

void
nsWaveDecoder::MediaErrorDecode()
{
  if (mShuttingDown) {
    return;
  }
#if 0
  if (mElement) {
    mElement->MediaErrorDecode();
  }
#else
  NS_WARNING("MediaErrorDecode fired, but not implemented.");
#endif
}

void
nsWaveDecoder::PlaybackPositionChanged()
{
  if (mShuttingDown) {
    return;
  }

  float lastTime = mCurrentTime;

  if (mPlaybackStateMachine) {
    mCurrentTime = mPlaybackStateMachine->GetTimeForPositionChange();
  }

  if (mElement && lastTime != mCurrentTime) {
    mElement->DispatchSimpleEvent(NS_LITERAL_STRING("timeupdate"));
  }
}

void
nsWaveDecoder::SetDuration(PRInt64 )
{
  
  
}

void
nsWaveDecoder::SetSeekable(PRBool aSeekable)
{
  mSeekable = aSeekable;
}

PRBool
nsWaveDecoder::GetSeekable()
{
  return mSeekable;
}

void
nsWaveDecoder::Suspend()
{
  if (mStream) {
    mStream->Suspend();
  }
}

void
nsWaveDecoder::Resume()
{
  if (mStream) {
    mStream->Resume();
  }
}
