




































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



#define BUFFERING_SECONDS_LOW_WATER_MARK 1


#define RIFF_CHUNK_MAGIC 0x52494646
#define WAVE_CHUNK_MAGIC 0x57415645
#define FRMT_CHUNK_MAGIC 0x666d7420
#define DATA_CHUNK_MAGIC 0x64617461


#define RIFF_CHUNK_HEADER_SIZE 8


#define RIFF_INITIAL_SIZE (RIFF_CHUNK_HEADER_SIZE + 4)



#define WAVE_FORMAT_CHUNK_SIZE 16


#define WAVE_FORMAT_SIZE (RIFF_CHUNK_HEADER_SIZE + WAVE_FORMAT_CHUNK_SIZE)



#define WAVE_FORMAT_ENCODING_PCM 1


















class nsWaveStateMachine : public nsRunnable
{
public:
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

  nsWaveStateMachine(nsWaveDecoder* aDecoder, nsMediaStream* aStream,
                     PRUint32 aBufferWaitTime, float aInitialVolume);
  ~nsWaveStateMachine();

  
  
  float GetVolume();

  
  
  void SetVolume(float aVolume);

  



  void Play();
  void Pause();
  void Seek(float aTime);
  void Shutdown();

  
  
  
  float GetDuration();

  
  
  float GetCurrentTime();

  
  PRBool IsSeeking();

  
  PRBool IsEnded();

  
  void StreamEnded();

  
  NS_IMETHOD Run();

  
  
  
  void UpdateTimeOffset(float aTime);

private:
  
  
  
  void ChangeState(State aState);

  
  void OpenAudioStream();

  
  void CloseAudioStream();

  
  
  PRBool LoadRIFFChunk();

  
  
  
  PRBool LoadFormatChunk();

  
  
  
  PRBool FindDataOffset();

  
  
  
  float BytesToTime(PRUint32 aBytes) const
  {
    NS_ABORT_IF_FALSE(mMetadataValid, "Requires valid metadata");
    return float(aBytes) / mSampleRate / mSampleSize;
  }

  
  
  
  PRUint32 TimeToBytes(float aTime) const
  {
    NS_ABORT_IF_FALSE(mMetadataValid, "Requires valid metadata");
    return PRUint32(aTime * mSampleRate * mSampleSize);
  }

  
  
  PRUint32 RoundDownToSample(PRUint32 aBytes) const
  {
    NS_ABORT_IF_FALSE(mMetadataValid, "Requires valid metadata");
    return aBytes - (aBytes % mSampleSize);
  }

  
  
  
  
  
  nsWaveDecoder* mDecoder;

  
  
  
  
  
  nsMediaStream* mStream;

  
  
  
  nsAutoPtr<nsAudioStream> mAudioStream;

  
  PRUint32 mBufferingWait;

  
  PRUint32 mBufferingBytes;

  
  
  PRIntervalTime mBufferingStart;

  
  
  PRUint32 mAudioBufferSize;

  




  
  PRUint32 mSampleRate;

  
  PRUint32 mChannels;

  
  
  PRUint32 mSampleSize;

  
  nsAudioStream::SampleFormat mSampleFormat;

  
  
  PRUint32 mWaveLength;

  
  
  PRUint32 mWavePCMOffset;

  



  PRMonitor* mMonitor;

  
  State mState;

  
  
  
  
  State mNextState;

  
  float mInitialVolume;

  
  
  
  float mTimeOffset;

  
  
  
  PRPackedBool mExpectMoreData;

  
  float mSeekTime;

  
  
  
  PRPackedBool mMetadataValid;
};

nsWaveStateMachine::nsWaveStateMachine(nsWaveDecoder* aDecoder, nsMediaStream* aStream,
                                       PRUint32 aBufferWaitTime, float aInitialVolume)
  : mDecoder(aDecoder),
    mStream(aStream),
    mBufferingWait(aBufferWaitTime),
    mBufferingBytes(0),
    mBufferingStart(0),
    mAudioBufferSize(0),
    mSampleRate(0),
    mChannels(0),
    mSampleSize(0),
    mSampleFormat(nsAudioStream::FORMAT_S16_LE),
    mWaveLength(0),
    mWavePCMOffset(0),
    mMonitor(nsnull),
    mState(STATE_LOADING_METADATA),
    mNextState(STATE_PAUSED),
    mInitialVolume(aInitialVolume),
    mTimeOffset(0.0),
    mExpectMoreData(PR_TRUE),
    mSeekTime(0.0),
    mMetadataValid(PR_FALSE)
{
  mMonitor = nsAutoMonitor::NewMonitor("nsWaveStateMachine");
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
  if (mState == STATE_LOADING_METADATA || mState == STATE_SEEKING) {
    mNextState = STATE_PLAYING;
  } else {
    ChangeState(STATE_PLAYING);
  }
}

float
nsWaveStateMachine::GetVolume()
{
  float volume = mInitialVolume;
  if (mAudioStream) {
    volume = mAudioStream->GetVolume();
  }
  return volume;
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
    PRUint32 length = mWaveLength;
    PRInt64 contentLength = mDecoder->GetTotalBytes();
    
    
    
    if (contentLength >= 0 && contentLength - mWavePCMOffset < length) {
      length = contentLength - mWavePCMOffset;
    }
    return BytesToTime(length);
  }
  return std::numeric_limits<float>::quiet_NaN();
}

float
nsWaveStateMachine::GetCurrentTime()
{
  nsAutoMonitor monitor(mMonitor);
  double time = 0.0;
  if (mAudioStream) {
    time = mAudioStream->GetTime();
  }
  return float(time + mTimeOffset);
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

void
nsWaveStateMachine::StreamEnded()
{
  nsAutoMonitor monitor(mMonitor);
  mExpectMoreData = PR_FALSE;
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

    case STATE_BUFFERING:
      if ((PR_IntervalToMilliseconds(PR_IntervalNow() - mBufferingStart) < mBufferingWait) &&
          mStream->DownloadRate() >= 0 &&
          mStream->Available() < mBufferingBytes) {
        LOG(PR_LOG_DEBUG, ("Buffering data until %d bytes or %d milliseconds (rate %f)\n",
                           mBufferingBytes - mStream->Available(),
                           mBufferingWait - (PR_IntervalToMilliseconds(PR_IntervalNow() - mBufferingStart)),
                           mStream->DownloadRate()));
        monitor.Wait(PR_MillisecondsToInterval(1000));
      } else {
        ChangeState(mNextState);
        nsCOMPtr<nsIRunnable> event =
          NS_NEW_RUNNABLE_METHOD(nsWaveDecoder, mDecoder, BufferingStopped);
        NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
      }

      break;

    case STATE_PLAYING:
      if (!mAudioStream) {
        OpenAudioStream();
      } else {
        mAudioStream->Resume();
      }

      if (mStream->DownloadRate() >= 0 &&
          mStream->Available() < mStream->PlaybackRate() * BUFFERING_SECONDS_LOW_WATER_MARK) {
        nsCOMPtr<nsIRunnable> event =
          NS_NEW_RUNNABLE_METHOD(nsWaveDecoder, mDecoder, BufferingStarted);
        NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);

        
        mBufferingBytes = TimeToBytes(float(mBufferingWait) / 1000.0);
        mBufferingStart = PR_IntervalNow();
        ChangeState(STATE_BUFFERING);
      }

      if (!mExpectMoreData && mStream->Available() < mSampleSize) {
        
        
        ChangeState(STATE_ENDED);
      } else {
        
        
        
        
        PRUint32 sampleSize = mSampleFormat == nsAudioStream::FORMAT_U8 ? 1 : 2;
        PRUint32 len = RoundDownToSample(NS_MIN(mStream->Available(),
                                                PRUint32(mAudioStream->Available() * sampleSize)));
        if (len) {
          nsAutoArrayPtr<char> buf(new char[len]);
          PRUint32 got = 0;

          monitor.Exit();
          if (NS_FAILED(mStream->Read(buf.get(), len, &got))) {
            NS_WARNING("Stream read failed");
          }

          if (got == 0) {
            ChangeState(STATE_ENDED);
          }

          
          
          
          
          len = RoundDownToSample(got);

          
          
          PRInt64 endDelta = mWavePCMOffset + mWaveLength - mStream->Tell();
          if (endDelta < 0) {
            
            
            len -= -endDelta;
            if (RoundDownToSample(len) != len) {
              NS_WARNING("PCM data does not end with complete sample");
              len = RoundDownToSample(len);
            }
            ChangeState(STATE_ENDED);
          }

          PRUint32 lengthInSamples = len;
          if (mSampleFormat == nsAudioStream::FORMAT_S16_LE) {
            lengthInSamples /= sizeof(short);
          }
          mAudioStream->Write(buf.get(), lengthInSamples);
          monitor.Enter();
        }

        
        
        
        
        
        
        
        
        float nextWakeup = BytesToTime(mAudioBufferSize - mAudioStream->Available() * sizeof(short)) * 1000.0 / 2.0;
        monitor.Wait(PR_MillisecondsToInterval(PRUint32(nextWakeup)));
      }
      break;

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
        NS_ABORT_IF_FALSE(position >= 0 && position <= mWaveLength, "Invalid seek position");

        
        
        
        
        
        
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

        monitor.Exit();
        nsCOMPtr<nsIRunnable> stopEvent =
          NS_NEW_RUNNABLE_METHOD(nsWaveDecoder, mDecoder, SeekingStopped);
        NS_DispatchToMainThread(stopEvent, NS_DISPATCH_SYNC);
        monitor.Enter();

        if (mState == STATE_SEEKING && mSeekTime == seekTime) {
          
          
          
          
          
          State nextState = mNextState;
          if (nextState == STATE_SEEKING) {
            nextState = STATE_PAUSED;
          }
          ChangeState(nextState);
        }
      }
      break;

    case STATE_PAUSED:
      if (mAudioStream) {
        mAudioStream->Pause();
      }
      monitor.Wait();
      break;

    case STATE_ENDED:
      if (mAudioStream) {
        monitor.Exit();
        mAudioStream->Drain();
        monitor.Enter();
        mTimeOffset += mAudioStream->GetTime();
      }

      
      
      
      CloseAudioStream();

      if (mState != STATE_SHUTDOWN) {
        nsCOMPtr<nsIRunnable> event =
          NS_NEW_RUNNABLE_METHOD(nsWaveDecoder, mDecoder, PlaybackEnded);
        NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
      }

      while (mState != STATE_SHUTDOWN) {
        monitor.Wait();
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
      if (mAudioStream) {
        mTimeOffset += mAudioStream->GetTime();
      }
      CloseAudioStream();
      return NS_OK;
    }
  }

  return NS_OK;
}

void
nsWaveStateMachine::UpdateTimeOffset(float aTime)
{
  nsAutoMonitor monitor(mMonitor);
  mTimeOffset = NS_MIN(aTime, GetDuration());
  if (mTimeOffset < 0.0) {
    mTimeOffset = 0.0;
  }
}

void
nsWaveStateMachine::ChangeState(State aState)
{
  nsAutoMonitor monitor(mMonitor);
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
    mAudioBufferSize = mAudioStream->Available() * sizeof(short);
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

static PRBool
ReadAll(nsMediaStream* aStream, char* aBuf, PRUint32 aSize)
{
  PRUint32 got = 0;
  do {
    PRUint32 read = 0;
    if (NS_FAILED(aStream->Read(aBuf + got, aSize - got, &read))) {
      NS_WARNING("Stream read failed");
      return PR_FALSE;
    }
    got += read;
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

  if (!ReadAll(mStream, riffHeader, sizeof(riffHeader))) {
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
nsWaveStateMachine::LoadFormatChunk()
{
  PRUint32 rate, channels, sampleSize, sampleFormat;
  char waveFormat[WAVE_FORMAT_SIZE];
  const char* p = waveFormat;

  
  NS_ABORT_IF_FALSE(mStream->Tell() % 2 == 0,
                    "LoadFormatChunk called with unaligned stream");

  if (!ReadAll(mStream, waveFormat, sizeof(waveFormat))) {
    return PR_FALSE;
  }

  if (ReadUint32BE(&p) != FRMT_CHUNK_MAGIC) {
    NS_WARNING("Expected format chunk");
    return PR_FALSE;
  }

  PRUint32 fmtsize = ReadUint32LE(&p);

  if (ReadUint16LE(&p) != WAVE_FORMAT_ENCODING_PCM) {
    NS_WARNING("WAVE is not uncompressed PCM, compressed encodings are not supported");
    return PR_FALSE;
  }

  channels = ReadUint16LE(&p);
  rate = ReadUint32LE(&p);

  
  p += 4;

  sampleSize = ReadUint16LE(&p);

  sampleFormat = ReadUint16LE(&p);

  
  
  
  
  
  if (fmtsize > WAVE_FORMAT_CHUNK_SIZE) {
    char extLength[2];
    const char* p = extLength;

    if (!ReadAll(mStream, extLength, sizeof(extLength))) {
      return PR_FALSE;
    }

    PRUint16 extra = ReadUint16LE(&p);
    if (fmtsize - (WAVE_FORMAT_CHUNK_SIZE + 2) != extra) {
      NS_WARNING("Invalid extended format chunk size");
      return PR_FALSE;
    }
    extra += extra % 2;

    if (extra > 0) {
      nsAutoArrayPtr<char> chunkExtension(new char[extra]);
      if (!ReadAll(mStream, chunkExtension.get(), extra)) {
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

  
  
  for (;;) {
    char chunkHeader[8];
    const char* p = chunkHeader;

    if (!ReadAll(mStream, chunkHeader, sizeof(chunkHeader))) {
      return PR_FALSE;
    }

    PRUint32 magic = ReadUint32BE(&p);

    if (magic == DATA_CHUNK_MAGIC) {
      length = ReadUint32LE(&p);
      break;
    }

    if (magic == FRMT_CHUNK_MAGIC) {
      LOG(PR_LOG_ERROR, ("Invalid WAVE: expected \"data\" chunk but found \"format\" chunk"));
      return PR_FALSE;
    }

    PRUint32 size = ReadUint32LE(&p);
    size += size % 2;

    nsAutoArrayPtr<char> chunk(new char[size]);
    if (!ReadAll(mStream, chunk.get(), size)) {
      return PR_FALSE;
    }
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

NS_IMPL_THREADSAFE_ISUPPORTS1(nsWaveDecoder, nsIObserver)

nsWaveDecoder::nsWaveDecoder()
  : mBytesDownloaded(0),
    mInitialVolume(1.0),
    mStream(nsnull),
    mTimeOffset(0.0),
    mEndedCurrentTime(0.0),
    mEndedDuration(std::numeric_limits<float>::quiet_NaN()),
    mEnded(PR_FALSE),
    mNotifyOnShutdown(PR_FALSE),
    mSeekable(PR_TRUE),
    mResourceLoaded(PR_FALSE)
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
  if (mPlaybackStateMachine) {
    return mPlaybackStateMachine->GetCurrentTime();
  }
  return mEndedCurrentTime;
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

float
nsWaveDecoder::GetVolume()
{
  if (!mPlaybackStateMachine) {
    return mInitialVolume;
  }
  return mPlaybackStateMachine->GetVolume();
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

  
  mBytesDownloaded = 0;
  mResourceLoaded = PR_FALSE;

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

  nsresult rv = mStream->Open(this, mURI, aChannel, aStreamListener);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = NS_NewThread(getter_AddRefs(mPlaybackThread));
  NS_ENSURE_SUCCESS(rv, rv);

  mPlaybackStateMachine = new nsWaveStateMachine(this, mStream.get(),
                                                 BUFFERING_TIMEOUT * 1000,
                                                 mInitialVolume);
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

  if (!mResourceLoaded) {
    StartProgress();
  }
  else if (mElement)
  {
    
    
    mElement->DispatchAsyncProgressEvent(NS_LITERAL_STRING("progress"));
  }
}

void
nsWaveDecoder::PlaybackEnded()
{
  if (mShuttingDown) {
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

  
  
  if (mContentLength >= 0) {
    mBytesDownloaded = mContentLength;
  }

  mResourceLoaded = PR_TRUE;

  if (mElement) {
    mElement->ResourceLoaded();
  }
  if (mPlaybackStateMachine) {
    mPlaybackStateMachine->StreamEnded();
  }

  StopProgress();

  
  if (mElement) {
    mElement->DispatchAsyncProgressEvent(NS_LITERAL_STRING("progress"));
  }
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
  if (mPlaybackStateMachine) {
    mPlaybackStateMachine->StreamEnded();
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

PRUint64
nsWaveDecoder::GetBytesLoaded()
{
  return mBytesDownloaded;
}

PRInt64
nsWaveDecoder::GetTotalBytes()
{
  return mContentLength;
}

void
nsWaveDecoder::SetTotalBytes(PRInt64 aBytes)
{
  mContentLength = aBytes;
}

void
nsWaveDecoder::UpdateBytesDownloaded(PRUint64 aBytes)
{
  mBytesDownloaded = aBytes;
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
nsWaveDecoder::BufferingStarted()
{
  if (mShuttingDown) {
    return;
  }

  if (mElement) {
    mElement->ChangeReadyState(nsIDOMHTMLMediaElement::HAVE_CURRENT_DATA);
  }
}

void
nsWaveDecoder::BufferingStopped()
{
  if (mShuttingDown) {
    return;
  }

  if (mElement) {
    mElement->ChangeReadyState(nsIDOMHTMLMediaElement::HAVE_FUTURE_DATA);
  }
}

void
nsWaveDecoder::SeekingStarted()
{
  if (mShuttingDown) {
    return;
  }

  if (mPlaybackStateMachine) {
    mPlaybackStateMachine->UpdateTimeOffset(mTimeOffset);
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
nsWaveDecoder::SetSeekable(PRBool aSeekable)
{
  mSeekable = aSeekable;
}

PRBool
nsWaveDecoder::GetSeekable()
{
  return mSeekable;
}
