



#include "MediaEngineWebRTC.h"
#include <stdio.h>
#include <algorithm>
#include "mozilla/Assertions.h"
#include "MediaTrackConstraints.h"
#include "mtransport/runnable_utils.h"


#ifdef FF
#undef FF
#endif
#include "webrtc/modules/audio_device/opensl/single_rw_fifo.h"

#define CHANNELS 1
#define ENCODING "L16"
#define DEFAULT_PORT 5555

#define SAMPLE_RATE 256000
#define SAMPLE_FREQUENCY 16000
#define SAMPLE_LENGTH ((SAMPLE_FREQUENCY*10)/1000)


#define MAX_CHANNELS 2
#define MAX_SAMPLING_FREQ 48000 // Hz - multiple of 100

#define MAX_AEC_FIFO_DEPTH 200 // ms - multiple of 10
static_assert(!(MAX_AEC_FIFO_DEPTH % 10), "Invalid MAX_AEC_FIFO_DEPTH");

namespace mozilla {

#ifdef LOG
#undef LOG
#endif

#ifdef PR_LOGGING
extern PRLogModuleInfo* GetMediaManagerLog();
#define LOG(msg) PR_LOG(GetMediaManagerLog(), PR_LOG_DEBUG, msg)
#define LOG_FRAMES(msg) PR_LOG(GetMediaManagerLog(), 6, msg)
#else
#define LOG(msg)
#define LOG_FRAMES(msg)
#endif




NS_IMPL_ISUPPORTS0(MediaEngineWebRTCAudioSource)


StaticRefPtr<AudioOutputObserver> gFarendObserver;

AudioOutputObserver::AudioOutputObserver()
  : mPlayoutFreq(0)
  , mPlayoutChannels(0)
  , mChunkSize(0)
  , mSaved(nullptr)
  , mSamplesSaved(0)
{
  
  mPlayoutFifo = new webrtc::SingleRwFifo(MAX_AEC_FIFO_DEPTH/10);
}

AudioOutputObserver::~AudioOutputObserver()
{
  Clear();
  moz_free(mSaved);
  mSaved = nullptr;
}

void
AudioOutputObserver::Clear()
{
  while (mPlayoutFifo->size() > 0) {
    moz_free(mPlayoutFifo->Pop());
  }
  
}

FarEndAudioChunk *
AudioOutputObserver::Pop()
{
  return (FarEndAudioChunk *) mPlayoutFifo->Pop();
}

uint32_t
AudioOutputObserver::Size()
{
  return mPlayoutFifo->size();
}

void
AudioOutputObserver::MixerCallback(AudioDataValue* aMixedBuffer,
                                   AudioSampleFormat aFormat,
                                   uint32_t aChannels,
                                   uint32_t aFrames,
                                   uint32_t aSampleRate)
{
  if (gFarendObserver) {
    gFarendObserver->InsertFarEnd(aMixedBuffer, aFrames, false,
                                  aSampleRate, aChannels, aFormat);
  }
}


void
AudioOutputObserver::InsertFarEnd(const AudioDataValue *aBuffer, uint32_t aFrames, bool aOverran,
                                  int aFreq, int aChannels, AudioSampleFormat aFormat)
{
  if (mPlayoutChannels != 0) {
    if (mPlayoutChannels != static_cast<uint32_t>(aChannels)) {
      MOZ_CRASH();
    }
  } else {
    MOZ_ASSERT(aChannels <= MAX_CHANNELS);
    mPlayoutChannels = static_cast<uint32_t>(aChannels);
  }
  if (mPlayoutFreq != 0) {
    if (mPlayoutFreq != static_cast<uint32_t>(aFreq)) {
      MOZ_CRASH();
    }
  } else {
    MOZ_ASSERT(aFreq <= MAX_SAMPLING_FREQ);
    MOZ_ASSERT(!(aFreq % 100), "Sampling rate for far end data should be multiple of 100.");
    mPlayoutFreq = aFreq;
    mChunkSize = aFreq/100; 
  }

#ifdef LOG_FAREND_INSERTION
  static FILE *fp = fopen("insertfarend.pcm","wb");
#endif

  if (mSaved) {
    
    mSaved->mOverrun = aOverran;
    aOverran = false;
  }
  
  
  
  while (aFrames) {
    if (!mSaved) {
      mSaved = (FarEndAudioChunk *) moz_xmalloc(sizeof(FarEndAudioChunk) +
                                                (mChunkSize * aChannels - 1)*sizeof(int16_t));
      mSaved->mSamples = mChunkSize;
      mSaved->mOverrun = aOverran;
      aOverran = false;
    }
    uint32_t to_copy = mChunkSize - mSamplesSaved;
    if (to_copy > aFrames) {
      to_copy = aFrames;
    }

    int16_t *dest = &(mSaved->mData[mSamplesSaved * aChannels]);
    ConvertAudioSamples(aBuffer, dest, to_copy * aChannels);

#ifdef LOG_FAREND_INSERTION
    if (fp) {
      fwrite(&(mSaved->mData[mSamplesSaved * aChannels]), to_copy * aChannels, sizeof(int16_t), fp);
    }
#endif
    aFrames -= to_copy;
    mSamplesSaved += to_copy;
    aBuffer += to_copy * aChannels;

    if (mSamplesSaved >= mChunkSize) {
      int free_slots = mPlayoutFifo->capacity() - mPlayoutFifo->size();
      if (free_slots <= 0) {
        
        
        break;
      } else {
        mPlayoutFifo->Push((int8_t *) mSaved); 
        mSaved = nullptr;
        mSamplesSaved = 0;
      }
    }
  }
}

void
MediaEngineWebRTCAudioSource::GetName(nsAString& aName)
{
  if (mInitDone) {
    aName.Assign(mDeviceName);
  }

  return;
}

void
MediaEngineWebRTCAudioSource::GetUUID(nsAString& aUUID)
{
  if (mInitDone) {
    aUUID.Assign(mDeviceUUID);
  }

  return;
}

nsresult
MediaEngineWebRTCAudioSource::Config(bool aEchoOn, uint32_t aEcho,
                                     bool aAgcOn, uint32_t aAGC,
                                     bool aNoiseOn, uint32_t aNoise,
                                     int32_t aPlayoutDelay)
{
  LOG(("Audio config: aec: %d, agc: %d, noise: %d",
       aEchoOn ? aEcho : -1,
       aAgcOn ? aAGC : -1,
       aNoiseOn ? aNoise : -1));

  bool update_echo = (mEchoOn != aEchoOn);
  bool update_agc = (mAgcOn != aAgcOn);
  bool update_noise = (mNoiseOn != aNoiseOn);
  mEchoOn = aEchoOn;
  mAgcOn = aAgcOn;
  mNoiseOn = aNoiseOn;

  if ((webrtc::EcModes) aEcho != webrtc::kEcUnchanged) {
    if (mEchoCancel != (webrtc::EcModes) aEcho) {
      update_echo = true;
      mEchoCancel = (webrtc::EcModes) aEcho;
    }
  }
  if ((webrtc::AgcModes) aAGC != webrtc::kAgcUnchanged) {
    if (mAGC != (webrtc::AgcModes) aAGC) {
      update_agc = true;
      mAGC = (webrtc::AgcModes) aAGC;
    }
  }
  if ((webrtc::NsModes) aNoise != webrtc::kNsUnchanged) {
    if (mNoiseSuppress != (webrtc::NsModes) aNoise) {
      update_noise = true;
      mNoiseSuppress = (webrtc::NsModes) aNoise;
    }
  }
  mPlayoutDelay = aPlayoutDelay;

  if (mInitDone) {
    int error;

    if (update_echo &&
      0 != (error = mVoEProcessing->SetEcStatus(mEchoOn, (webrtc::EcModes) aEcho))) {
      LOG(("%s Error setting Echo Status: %d ",__FUNCTION__, error));
      
      if (mEchoOn) {
        if (0 != (error = mVoEProcessing->SetEcMetricsStatus(true))) {
          LOG(("%s Error setting Echo Metrics: %d ",__FUNCTION__, error));
        }
      }
    }
    if (update_agc &&
      0 != (error = mVoEProcessing->SetAgcStatus(mAgcOn, (webrtc::AgcModes) aAGC))) {
      LOG(("%s Error setting AGC Status: %d ",__FUNCTION__, error));
    }
    if (update_noise &&
      0 != (error = mVoEProcessing->SetNsStatus(mNoiseOn, (webrtc::NsModes) aNoise))) {
      LOG(("%s Error setting NoiseSuppression Status: %d ",__FUNCTION__, error));
    }
  }
  return NS_OK;
}

nsresult
MediaEngineWebRTCAudioSource::Allocate(const AudioTrackConstraintsN &aConstraints,
                                       const MediaEnginePrefs &aPrefs)
{
  if (mState == kReleased) {
    if (mInitDone) {
      ScopedCustomReleasePtr<webrtc::VoEHardware> ptrVoEHw(webrtc::VoEHardware::GetInterface(mVoiceEngine));
      if (!ptrVoEHw || ptrVoEHw->SetRecordingDevice(mCapIndex)) {
        return NS_ERROR_FAILURE;
      }
      mState = kAllocated;
      LOG(("Audio device %d allocated", mCapIndex));
    } else {
      LOG(("Audio device is not initalized"));
      return NS_ERROR_FAILURE;
    }
  } else if (mSources.IsEmpty()) {
    LOG(("Audio device %d reallocated", mCapIndex));
  } else {
    LOG(("Audio device %d allocated shared", mCapIndex));
  }
  return NS_OK;
}

nsresult
MediaEngineWebRTCAudioSource::Deallocate()
{
  if (mSources.IsEmpty()) {
    if (mState != kStopped && mState != kAllocated) {
      return NS_ERROR_FAILURE;
    }

    mState = kReleased;
    LOG(("Audio device %d deallocated", mCapIndex));
  } else {
    LOG(("Audio device %d deallocated but still in use", mCapIndex));
  }
  return NS_OK;
}

nsresult
MediaEngineWebRTCAudioSource::Start(SourceMediaStream* aStream, TrackID aID)
{
  if (!mInitDone || !aStream) {
    return NS_ERROR_FAILURE;
  }

  {
    MonitorAutoLock lock(mMonitor);
    mSources.AppendElement(aStream);
  }

  AudioSegment* segment = new AudioSegment();
  aStream->AddAudioTrack(aID, SAMPLE_FREQUENCY, 0, segment);
  aStream->AdvanceKnownTracksTime(STREAM_TIME_MAX);
  
  aStream->RegisterForAudioMixing();
  LOG(("Start audio for stream %p", aStream));

  if (mState == kStarted) {
    MOZ_ASSERT(aID == mTrackID);
    return NS_OK;
  }
  mState = kStarted;
  mTrackID = aID;

  
  AsyncLatencyLogger::Get(true);

  
  
  MOZ_ASSERT(gFarendObserver);
  gFarendObserver->Clear();

  
  Config(mEchoOn, webrtc::kEcUnchanged,
         mAgcOn, webrtc::kAgcUnchanged,
         mNoiseOn, webrtc::kNsUnchanged,
         mPlayoutDelay);

  if (mVoEBase->StartReceive(mChannel)) {
    return NS_ERROR_FAILURE;
  }
  if (mVoEBase->StartSend(mChannel)) {
    return NS_ERROR_FAILURE;
  }

  
  mVoERender->RegisterExternalMediaProcessing(mChannel, webrtc::kRecordingPerChannel, *this);

  return NS_OK;
}

nsresult
MediaEngineWebRTCAudioSource::Stop(SourceMediaStream *aSource, TrackID aID)
{
  {
    MonitorAutoLock lock(mMonitor);

    if (!mSources.RemoveElement(aSource)) {
      
      return NS_OK;
    }

    aSource->EndTrack(aID);

    if (!mSources.IsEmpty()) {
      return NS_OK;
    }
    if (mState != kStarted) {
      return NS_ERROR_FAILURE;
    }
    if (!mVoEBase) {
      return NS_ERROR_FAILURE;
    }

    mState = kStopped;
  }

  mVoERender->DeRegisterExternalMediaProcessing(mChannel, webrtc::kRecordingPerChannel);

  if (mVoEBase->StopSend(mChannel)) {
    return NS_ERROR_FAILURE;
  }
  if (mVoEBase->StopReceive(mChannel)) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

void
MediaEngineWebRTCAudioSource::NotifyPull(MediaStreamGraph* aGraph,
                                         SourceMediaStream *aSource,
                                         TrackID aID,
                                         StreamTime aDesiredTime,
                                         StreamTime &aLastEndTime)
{
  
#ifdef DEBUG
  StreamTime delta = aDesiredTime - aLastEndTime;
  LOG(("Audio: NotifyPull: aDesiredTime %ld, delta %ld",(int64_t) aDesiredTime,
       (int64_t) delta));
  aLastEndTime = aDesiredTime;
#endif
}

void
MediaEngineWebRTCAudioSource::Init()
{
  mVoEBase = webrtc::VoEBase::GetInterface(mVoiceEngine);

  mVoEBase->Init();

  mVoERender = webrtc::VoEExternalMedia::GetInterface(mVoiceEngine);
  if (!mVoERender) {
    return;
  }
  mVoENetwork = webrtc::VoENetwork::GetInterface(mVoiceEngine);
  if (!mVoENetwork) {
    return;
  }

  mVoEProcessing = webrtc::VoEAudioProcessing::GetInterface(mVoiceEngine);
  if (!mVoEProcessing) {
    return;
  }

  mVoECallReport = webrtc::VoECallReport::GetInterface(mVoiceEngine);
  if (!mVoECallReport) {
    return;
  }

  mChannel = mVoEBase->CreateChannel();
  if (mChannel < 0) {
    return;
  }
  mNullTransport = new NullTransport();
  if (mVoENetwork->RegisterExternalTransport(mChannel, *mNullTransport)) {
    return;
  }

  
  ScopedCustomReleasePtr<webrtc::VoEHardware> ptrVoEHw(webrtc::VoEHardware::GetInterface(mVoiceEngine));
  if (!ptrVoEHw || ptrVoEHw->SetRecordingDevice(mCapIndex)) {
    return;
  }

#ifndef MOZ_B2G
  
  
  bool avail = false;
  ptrVoEHw->GetRecordingDeviceStatus(avail);
  if (!avail) {
    return;
  }
#endif 

  
  ScopedCustomReleasePtr<webrtc::VoECodec> ptrVoECodec(webrtc::VoECodec::GetInterface(mVoiceEngine));
  if (!ptrVoECodec) {
    return;
  }

  webrtc::CodecInst codec;
  strcpy(codec.plname, ENCODING);
  codec.channels = CHANNELS;
  codec.rate = SAMPLE_RATE;
  codec.plfreq = SAMPLE_FREQUENCY;
  codec.pacsize = SAMPLE_LENGTH;
  codec.pltype = 0; 

  if (!ptrVoECodec->SetSendCodec(mChannel, codec)) {
    mInitDone = true;
  }
}

void
MediaEngineWebRTCAudioSource::Shutdown()
{
  if (!mInitDone) {
    
    if (mChannel != -1) {
      mVoENetwork->DeRegisterExternalTransport(mChannel);
    }

    delete mNullTransport;
    return;
  }

  if (mState == kStarted) {
    while (!mSources.IsEmpty()) {
      Stop(mSources[0], kAudioTrack); 
    }
    MOZ_ASSERT(mState == kStopped);
  }

  if (mState == kAllocated || mState == kStopped) {
    Deallocate();
  }

  mVoEBase->Terminate();
  if (mChannel != -1) {
    mVoENetwork->DeRegisterExternalTransport(mChannel);
  }

  delete mNullTransport;

  mVoEProcessing = nullptr;
  mVoENetwork = nullptr;
  mVoERender = nullptr;
  mVoEBase = nullptr;

  mState = kReleased;
  mInitDone = false;
}

typedef int16_t sample;

void
MediaEngineWebRTCAudioSource::Process(int channel,
  webrtc::ProcessingTypes type, sample* audio10ms,
  int length, int samplingFreq, bool isStereo)
{
  
  
  
  if (!mStarted) {
    mStarted  = true;
    while (gFarendObserver->Size() > 1) {
      moz_free(gFarendObserver->Pop()); 
    }
  }

  while (gFarendObserver->Size() > 0) {
    FarEndAudioChunk *buffer = gFarendObserver->Pop(); 
    if (buffer) {
      int length = buffer->mSamples;
      int res = mVoERender->ExternalPlayoutData(buffer->mData,
                                                gFarendObserver->PlayoutFrequency(),
                                                gFarendObserver->PlayoutChannels(),
                                                mPlayoutDelay,
                                                length);
      moz_free(buffer);
      if (res == -1) {
        return;
      }
    }
  }

#ifdef PR_LOGGING
  mSamples += length;
  if (mSamples > samplingFreq) {
    mSamples %= samplingFreq; 
    if (PR_LOG_TEST(GetMediaManagerLog(), PR_LOG_DEBUG)) {
      webrtc::EchoStatistics echo;

      mVoECallReport->GetEchoMetricSummary(echo);
#define DUMP_STATVAL(x) (x).min, (x).max, (x).average
      LOG(("Echo: ERL: %d/%d/%d, ERLE: %d/%d/%d, RERL: %d/%d/%d, NLP: %d/%d/%d",
           DUMP_STATVAL(echo.erl),
           DUMP_STATVAL(echo.erle),
           DUMP_STATVAL(echo.rerl),
           DUMP_STATVAL(echo.a_nlp)));
    }
  }
#endif

  MonitorAutoLock lock(mMonitor);
  if (mState != kStarted)
    return;

  uint32_t len = mSources.Length();
  for (uint32_t i = 0; i < len; i++) {
    nsRefPtr<SharedBuffer> buffer = SharedBuffer::Create(length * sizeof(sample));

    sample* dest = static_cast<sample*>(buffer->Data());
    memcpy(dest, audio10ms, length * sizeof(sample));

    nsAutoPtr<AudioSegment> segment(new AudioSegment());
    nsAutoTArray<const sample*,1> channels;
    channels.AppendElement(dest);
    segment->AppendFrames(buffer.forget(), channels, length);
    TimeStamp insertTime;
    segment->GetStartTime(insertTime);

    if (mSources[i]) {
      
      
      LogTime(AsyncLatencyLogger::AudioTrackInsertion, LATENCY_STREAM_ID(mSources[i], mTrackID),
              (i+1 < len) ? 0 : 1, insertTime);

      
      
      
      
      RUN_ON_THREAD(mThread, WrapRunnable(mSources[i], &SourceMediaStream::AppendToTrack,
                                          mTrackID, segment, (AudioSegment *) nullptr),
                    NS_DISPATCH_NORMAL);
    }
  }

  return;
}

}
