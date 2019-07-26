














































































































#include <limits>
#include <prdtoa.h>
#include "nsIURI.h"
#include "nsIFileURL.h"
#include "nsNetUtil.h"
#include "VideoUtils.h"
#include "nsThreadUtils.h"
#include "nsContentUtils.h"
#include "nsIContentPolicy.h"
#include "nsIContentSecurityPolicy.h"
#include "nsICachingChannel.h"
#include "MediaDecoderStateMachine.h"
#include "WebMDecoder.h"
#include "WebMReader.h"
#include "DASHReader.h"
#include "nsDASHMPDParser.h"
#include "DASHRepDecoder.h"
#include "DASHDecoder.h"
#include <algorithm>

namespace mozilla {

#ifdef PR_LOGGING
extern PRLogModuleInfo* gMediaDecoderLog;
#define LOG(msg, ...) PR_LOG(gMediaDecoderLog, PR_LOG_DEBUG, \
                             ("%p [DASHDecoder] " msg, this, __VA_ARGS__))
#define LOG1(msg) PR_LOG(gMediaDecoderLog, PR_LOG_DEBUG, \
                         ("%p [DASHDecoder] " msg, this))
#else
#define LOG(msg, ...)
#define LOG1(msg)
#endif

DASHDecoder::DASHDecoder() :
  MediaDecoder(),
  mNotifiedLoadAborted(false),
  mBuffer(nullptr),
  mBufferLength(0),
  mMPDReaderThread(nullptr),
  mPrincipal(nullptr),
  mDASHReader(nullptr),
  mVideoAdaptSetIdx(-1),
  mAudioRepDecoderIdx(-1),
  mVideoRepDecoderIdx(-1),
  mAudioSubsegmentIdx(0),
  mVideoSubsegmentIdx(0),
  mAudioMetadataReadCount(0),
  mVideoMetadataReadCount(0),
  mSeeking(false),
  mStatisticsLock("DASHDecoder.mStatisticsLock")
{
  MOZ_COUNT_CTOR(DASHDecoder);
  mAudioStatistics = new MediaChannelStatistics();
  mVideoStatistics = new MediaChannelStatistics();
}

DASHDecoder::~DASHDecoder()
{
  MOZ_COUNT_DTOR(DASHDecoder);
}

MediaDecoderStateMachine*
DASHDecoder::CreateStateMachine()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  return new MediaDecoderStateMachine(this, mDASHReader);
}

void
DASHDecoder::ReleaseStateMachine()
{
  NS_ASSERTION(NS_IsMainThread(), "Must be on main thread.");

  
  mDASHReader = nullptr;

  MediaDecoder::ReleaseStateMachine();
  for (uint i = 0; i < mAudioRepDecoders.Length(); i++) {
    mAudioRepDecoders[i]->ReleaseStateMachine();
  }
  for (uint i = 0; i < mVideoRepDecoders.Length(); i++) {
    mVideoRepDecoders[i]->ReleaseStateMachine();
  }
}

nsresult
DASHDecoder::Load(MediaResource* aResource,
                  nsIStreamListener** aStreamListener,
                  MediaDecoder* aCloneDonor)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  mDASHReader = new DASHReader(this);

  nsresult rv = OpenResource(aResource, aStreamListener);
  NS_ENSURE_SUCCESS(rv, rv);

  mDecoderStateMachine = CreateStateMachine();
  if (!mDecoderStateMachine) {
    LOG1("Failed to create state machine!");
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

void
DASHDecoder::NotifyDownloadEnded(nsresult aStatus)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  
  if (mMPDManager) {
    LOG("Network Error! Repeated MPD download notification but MPD Manager "
        "[%p] already exists!", mMPDManager.get());
    NetworkError();
    return;
  }

  if (NS_SUCCEEDED(aStatus)) {
    LOG1("MPD downloaded.");

    
    mPrincipal = GetCurrentPrincipal();

    
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(this, &DASHDecoder::ReadMPDBuffer);
    NS_ENSURE_TRUE_VOID(event);

    nsresult rv = NS_NewNamedThread("DASH MPD Reader",
                                    getter_AddRefs(mMPDReaderThread),
                                    event,
                                    MEDIA_THREAD_STACK_SIZE);
    if (NS_FAILED(rv) || !mMPDReaderThread) {
      LOG("Error creating MPD reader thread: rv[%x] thread [%p].",
          rv, mMPDReaderThread.get());
      DecodeError();
      return;
    }
  } else if (aStatus == NS_BINDING_ABORTED) {
    LOG("MPD download has been cancelled by the user: aStatus [%x].", aStatus);
    if (mOwner) {
      mOwner->LoadAborted();
    }
    return;
  } else if (aStatus != NS_BASE_STREAM_CLOSED) {
    LOG("Network error trying to download MPD: aStatus [%x].", aStatus);
    NetworkError();
  }
}

void
DASHDecoder::ReadMPDBuffer()
{
  NS_ASSERTION(!NS_IsMainThread(), "Should not be on main thread.");

  LOG1("Started reading from the MPD buffer.");

  int64_t length = mResource->GetLength();
  if (length <= 0 || length > DASH_MAX_MPD_SIZE) {
    LOG("MPD is larger than [%d]MB.", DASH_MAX_MPD_SIZE/(1024*1024));
    DecodeError();
    return;
  }

  mBuffer = new char[length];

  uint32_t count = 0;
  nsresult rv = mResource->Read(mBuffer, length, &count);
  
  if (NS_FAILED(rv) || count != length) {
    LOG("Error reading MPD buffer: rv [%x] count [%d] length [%d].",
        rv, count, length);
    DecodeError();
    return;
  }
  
  mBufferLength = static_cast<uint32_t>(length);

  LOG1("Finished reading MPD buffer; back to main thread for parsing.");

  
  nsCOMPtr<nsIRunnable> event =
    NS_NewRunnableMethod(this, &DASHDecoder::OnReadMPDBufferCompleted);
  rv = NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
  if (NS_FAILED(rv)) {
    LOG("Error dispatching parse event to main thread: rv[%x]", rv);
    DecodeError();
    return;
  }
}

void
DASHDecoder::OnReadMPDBufferCompleted()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  if (mShuttingDown) {
    LOG1("Shutting down! Ignoring OnReadMPDBufferCompleted().");
    return;
  }

  
  if (!mMPDReaderThread) {
    LOG1("Error: MPD reader thread does not exist!");
    DecodeError();
    return;
  }
  nsresult rv = mMPDReaderThread->Shutdown();
  if (NS_FAILED(rv)) {
    LOG("MPD reader thread did not shutdown correctly! rv [%x]", rv);
    DecodeError();
    return;
  }
  mMPDReaderThread = nullptr;

  
  rv = ParseMPDBuffer();
  if (NS_FAILED(rv)) {
    LOG("Error parsing MPD buffer! rv [%x]", rv);
    DecodeError();
    return;
  }
  rv = CreateRepDecoders();
  if (NS_FAILED(rv)) {
    LOG("Error creating decoders for Representations! rv [%x]", rv);
    DecodeError();
    return;
  }

  rv = LoadRepresentations();
  if (NS_FAILED(rv)) {
    LOG("Error loading Representations! rv [%x]", rv);
    NetworkError();
    return;
  }

  
  
  mDASHReader->ReadyToReadMetadata();
}

nsresult
DASHDecoder::ParseMPDBuffer()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  NS_ENSURE_TRUE(mBuffer, NS_ERROR_NULL_POINTER);

  LOG1("Started parsing the MPD buffer.");

  
  nsAutoPtr<nsDASHMPDParser> parser;
  parser = new nsDASHMPDParser(mBuffer.forget(), mBufferLength, mPrincipal,
                               mResource->URI());
  mozilla::net::DASHMPDProfile profile;
  parser->Parse(getter_Transfers(mMPDManager), &profile);
  mBuffer = nullptr;
  NS_ENSURE_TRUE(mMPDManager, NS_ERROR_NULL_POINTER);

  LOG("Finished parsing the MPD buffer. Profile is [%d].", profile);

  return NS_OK;
}

nsresult
DASHDecoder::CreateRepDecoders()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  NS_ENSURE_TRUE(mMPDManager, NS_ERROR_NULL_POINTER);

  
  int64_t startTime = mMPDManager->GetStartTime();
  SetDuration(mMPDManager->GetDuration());
  NS_ENSURE_TRUE(startTime >= 0 && mDuration > 0, NS_ERROR_ILLEGAL_VALUE);

  

  for (uint32_t i = 0; i < mMPDManager->GetNumAdaptationSets(); i++) {
    IMPDManager::AdaptationSetType asType = mMPDManager->GetAdaptationSetType(i);
    if (asType == IMPDManager::DASH_VIDEO_STREAM) {
      mVideoAdaptSetIdx = i;
    }
    for (uint32_t j = 0; j < mMPDManager->GetNumRepresentations(i); j++) {
      
      nsAutoString segmentUrl;
      nsresult rv = mMPDManager->GetFirstSegmentUrl(i, j, segmentUrl);
      NS_ENSURE_SUCCESS(rv, rv);

      
      nsCOMPtr<nsIURI> url;
      rv = NS_NewURI(getter_AddRefs(url), segmentUrl, nullptr, mResource->URI());
      NS_ENSURE_SUCCESS(rv, rv);
#ifdef PR_LOGGING
      nsAutoCString newUrl;
      rv = url->GetSpec(newUrl);
      NS_ENSURE_SUCCESS(rv, rv);
      LOG("Using URL=\"%s\" for AdaptationSet [%d] Representation [%d]",
          newUrl.get(), i, j);
#endif

      
      nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(url);
      NS_ENSURE_FALSE(fileURL, NS_ERROR_ILLEGAL_VALUE);

      
      if (asType == IMPDManager::DASH_VIDEO_STREAM) {
        Representation const * rep = mMPDManager->GetRepresentation(i, j);
        NS_ENSURE_TRUE(rep, NS_ERROR_NULL_POINTER);
        rv = CreateVideoRepDecoder(url, rep);
        NS_ENSURE_SUCCESS(rv, rv);
      } else if (asType == IMPDManager::DASH_AUDIO_STREAM) {
        Representation const * rep = mMPDManager->GetRepresentation(i, j);
        NS_ENSURE_TRUE(rep, NS_ERROR_NULL_POINTER);
        rv = CreateAudioRepDecoder(url, rep);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }

  NS_ENSURE_TRUE(VideoRepDecoder(), NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(AudioRepDecoder(), NS_ERROR_NOT_INITIALIZED);

  return NS_OK;
}

nsresult
DASHDecoder::CreateAudioRepDecoder(nsIURI* aUrl,
                                   mozilla::net::Representation const * aRep)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  NS_ENSURE_ARG(aUrl);
  NS_ENSURE_ARG(aRep);
  NS_ENSURE_TRUE(mOwner, NS_ERROR_NOT_INITIALIZED);

  
  DASHRepDecoder* audioDecoder = new DASHRepDecoder(this);
  NS_ENSURE_TRUE(audioDecoder->Init(mOwner), NS_ERROR_NOT_INITIALIZED);

  
  if (mAudioRepDecoderIdx == -1) {
    mAudioRepDecoderIdx = 0;
  }
  mAudioRepDecoders.AppendElement(audioDecoder);

  
  WebMReader* audioReader = new WebMReader(audioDecoder);
  if (mDASHReader) {
    audioReader->SetMainReader(mDASHReader);
    mDASHReader->AddAudioReader(audioReader);
  }
  audioDecoder->SetReader(audioReader);

  
  MediaResource* audioResource
    = CreateAudioSubResource(aUrl, static_cast<MediaDecoder*>(audioDecoder));
  NS_ENSURE_TRUE(audioResource, NS_ERROR_NOT_INITIALIZED);

  audioDecoder->SetResource(audioResource);
  audioDecoder->SetMPDRepresentation(aRep);

  LOG("Created audio DASHRepDecoder [%p]", audioDecoder);

  return NS_OK;
}

nsresult
DASHDecoder::CreateVideoRepDecoder(nsIURI* aUrl,
                                   mozilla::net::Representation const * aRep)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  NS_ENSURE_ARG(aUrl);
  NS_ENSURE_ARG(aRep);
  NS_ENSURE_TRUE(mOwner, NS_ERROR_NOT_INITIALIZED);

  
  DASHRepDecoder* videoDecoder = new DASHRepDecoder(this);
  NS_ENSURE_TRUE(videoDecoder->Init(mOwner), NS_ERROR_NOT_INITIALIZED);

  
  if (mVideoRepDecoderIdx == -1) {
    mVideoRepDecoderIdx = 0;
  }
  mVideoRepDecoders.AppendElement(videoDecoder);

  
  WebMReader* videoReader = new WebMReader(videoDecoder);
  if (mDASHReader) {
    videoReader->SetMainReader(mDASHReader);
    mDASHReader->AddVideoReader(videoReader);
  }
  videoDecoder->SetReader(videoReader);

  
  MediaResource* videoResource
    = CreateVideoSubResource(aUrl, static_cast<MediaDecoder*>(videoDecoder));
  NS_ENSURE_TRUE(videoResource, NS_ERROR_NOT_INITIALIZED);

  videoDecoder->SetResource(videoResource);
  videoDecoder->SetMPDRepresentation(aRep);

  LOG("Created video DASHRepDecoder [%p]", videoDecoder);

  return NS_OK;
}

MediaResource*
DASHDecoder::CreateAudioSubResource(nsIURI* aUrl,
                                    MediaDecoder* aAudioDecoder)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  NS_ENSURE_TRUE(aUrl, nullptr);
  NS_ENSURE_TRUE(aAudioDecoder, nullptr);

  
  nsCOMPtr<nsIChannel> channel;
  nsresult rv = CreateSubChannel(aUrl, getter_AddRefs(channel));
  NS_ENSURE_SUCCESS(rv, nullptr);

  
  MediaResource* audioResource
    = MediaResource::Create(aAudioDecoder, channel);
  NS_ENSURE_TRUE(audioResource, nullptr);

  audioResource->RecordStatisticsTo(mAudioStatistics);
  return audioResource;
}

MediaResource*
DASHDecoder::CreateVideoSubResource(nsIURI* aUrl,
                                    MediaDecoder* aVideoDecoder)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  NS_ENSURE_TRUE(aUrl, nullptr);
  NS_ENSURE_TRUE(aVideoDecoder, nullptr);

  
  nsCOMPtr<nsIChannel> channel;
  nsresult rv = CreateSubChannel(aUrl, getter_AddRefs(channel));
  NS_ENSURE_SUCCESS(rv, nullptr);

  
  MediaResource* videoResource
    = MediaResource::Create(aVideoDecoder, channel);
  NS_ENSURE_TRUE(videoResource, nullptr);

  videoResource->RecordStatisticsTo(mVideoStatistics);
  return videoResource;
}

nsresult
DASHDecoder::CreateSubChannel(nsIURI* aUrl, nsIChannel** aChannel)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  NS_ENSURE_ARG(aUrl);

  NS_ENSURE_TRUE(mOwner, NS_ERROR_NULL_POINTER);
  HTMLMediaElement* element = mOwner->GetMediaElement();
  NS_ENSURE_TRUE(element, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsILoadGroup> loadGroup =
    element->GetDocumentLoadGroup();
  NS_ENSURE_TRUE(loadGroup, NS_ERROR_NULL_POINTER);

  
  
  nsCOMPtr<nsIChannelPolicy> channelPolicy;
  nsCOMPtr<nsIContentSecurityPolicy> csp;
  nsresult rv = element->NodePrincipal()->GetCsp(getter_AddRefs(csp));
  NS_ENSURE_SUCCESS(rv,rv);
  if (csp) {
    channelPolicy = do_CreateInstance("@mozilla.org/nschannelpolicy;1");
    channelPolicy->SetContentSecurityPolicy(csp);
    channelPolicy->SetLoadType(nsIContentPolicy::TYPE_MEDIA);
  }
  nsCOMPtr<nsIChannel> channel;
  rv = NS_NewChannel(getter_AddRefs(channel),
                     aUrl,
                     nullptr,
                     loadGroup,
                     nullptr,
                     nsICachingChannel::LOAD_BYPASS_LOCAL_CACHE_IF_BUSY,
                     channelPolicy);
  NS_ENSURE_SUCCESS(rv,rv);
  NS_ENSURE_TRUE(channel, NS_ERROR_NULL_POINTER);

  NS_ADDREF(*aChannel = channel);
  return NS_OK;
}

nsresult
DASHDecoder::LoadRepresentations()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  nsresult rv;
  {
    
    
    
    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());

    
    
    
    if (AudioRepDecoder()) {
      rv = AudioRepDecoder()->Load();
      NS_ENSURE_SUCCESS(rv, rv);
      mAudioMetadataReadCount++;
    }
    
    for (uint32_t i = 0; i < mVideoRepDecoders.Length(); i++) {
      rv = mVideoRepDecoders[i]->Load();
      NS_ENSURE_SUCCESS(rv, rv);
      mVideoMetadataReadCount++;
    }
    if (AudioRepDecoder()) {
      AudioRepDecoder()->SetStateMachine(mDecoderStateMachine);
    }
    for (uint32_t i = 0; i < mVideoRepDecoders.Length(); i++) {
      mVideoRepDecoders[i]->SetStateMachine(mDecoderStateMachine);
    }
  }

  
  if (mPlayState == PLAY_STATE_PLAYING) {
    mNextState = PLAY_STATE_PLAYING;
  }

  
  return InitializeStateMachine(nullptr);
}

void
DASHDecoder::NotifyDownloadEnded(DASHRepDecoder* aRepDecoder,
                                 nsresult aStatus,
                                 int32_t const aSubsegmentIdx)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  if (mShuttingDown) {
    LOG1("Shutting down! Ignoring NotifyDownloadEnded().");
    return;
  }

  
  if (!mMPDManager) {
    LOG1("Network Error! MPD Manager must exist, indicating MPD has been "
        "downloaded and parsed");
    NetworkError();
    return;
  }

  
  if (!aRepDecoder) {
    LOG1("Decoder for Representation is reported as null.");
    DecodeError();
    return;
  }

  if (NS_SUCCEEDED(aStatus)) {
    LOG("Byte range downloaded: decoder [%p] subsegmentIdx [%d]",
        aRepDecoder, aSubsegmentIdx);

    if (aSubsegmentIdx < 0) {
      LOG("Last subsegment for decoder [%p] was downloaded",
          aRepDecoder);
      return;
    }

    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
    nsRefPtr<DASHRepDecoder> decoder = aRepDecoder;
    {
      if (!IsDecoderAllowedToDownloadSubsegment(aRepDecoder,
                                                aSubsegmentIdx)) {
        NS_WARNING("Decoder downloaded subsegment but it is not allowed!");
        LOG("Error! Decoder [%p] downloaded subsegment [%d] but it is not "
            "allowed!", aRepDecoder, aSubsegmentIdx);
        return;
      }

      if (aRepDecoder == VideoRepDecoder() &&
          mVideoSubsegmentIdx == aSubsegmentIdx) {
        IncrementSubsegmentIndex(aRepDecoder);
      } else if (aRepDecoder == AudioRepDecoder() &&
          mAudioSubsegmentIdx == aSubsegmentIdx) {
        IncrementSubsegmentIndex(aRepDecoder);
      } else {
        return;
      }

      
      
      if (aRepDecoder == VideoRepDecoder() &&
          (uint32_t)mVideoSubsegmentIdx < VideoRepDecoder()->GetNumDataByteRanges()) {
        nsresult rv = PossiblySwitchDecoder(aRepDecoder);
        if (NS_FAILED(rv)) {
          LOG("Failed possibly switching decoder rv[0x%x]", rv);
          DecodeError();
          return;
        }
        decoder = VideoRepDecoder();
      }
    }

    
    if (!decoder || (decoder != AudioRepDecoder() &&
                     decoder != VideoRepDecoder())) {
      LOG("Invalid decoder [%p]: video idx [%d] audio idx [%d]",
          decoder.get(), AudioRepDecoder(), VideoRepDecoder());
      DecodeError();
      return;
    }

    
    
    if (decoder == VideoRepDecoder()) {
      if (mVideoSubsegmentLoads.IsEmpty() ||
          (uint32_t)mVideoSubsegmentIdx >= mVideoSubsegmentLoads.Length()) {
        LOG("Appending decoder [%d] [%p] to mVideoSubsegmentLoads at index "
            "[%d] before load; mVideoSubsegmentIdx[%d].",
            mVideoRepDecoderIdx, VideoRepDecoder(),
            mVideoSubsegmentLoads.Length(), mVideoSubsegmentIdx);
        mVideoSubsegmentLoads.AppendElement(mVideoRepDecoderIdx);
      } else {
        
        
        LOG("Setting decoder [%d] [%p] in mVideoSubsegmentLoads at index "
            "[%d] before load; mVideoSubsegmentIdx[%d].",
            mVideoRepDecoderIdx, VideoRepDecoder(),
            mVideoSubsegmentIdx, mVideoSubsegmentIdx);
        mVideoSubsegmentLoads[mVideoSubsegmentIdx] = mVideoRepDecoderIdx;
      }
      LOG("Notifying switch decided for video subsegment [%d]",
          mVideoSubsegmentIdx);
      mon.NotifyAll();
    }

    
    
    
    bool resourceLoaded = false;
    if (decoder.get() == AudioRepDecoder()) {
      LOG("Requesting load for audio decoder [%p] subsegment [%d].",
        decoder.get(), mAudioSubsegmentIdx);
      if (mAudioSubsegmentIdx >= decoder->GetNumDataByteRanges()) {
        resourceLoaded = true;
      }
    } else if (decoder.get() == VideoRepDecoder()) {
      LOG("Requesting load for video decoder [%p] subsegment [%d].",
        decoder.get(), mVideoSubsegmentIdx);
      if (mVideoSubsegmentIdx >= decoder->GetNumDataByteRanges()) {
        resourceLoaded = true;
      }
    }
    if (resourceLoaded) {
      ResourceLoaded();
      return;
    }
    decoder->LoadNextByteRange();
  } else if (aStatus == NS_BINDING_ABORTED) {
    LOG("Media download has been cancelled by the user: aStatus[%x]", aStatus);
    if (mOwner) {
      mOwner->LoadAborted();
    }
    return;
  } else if (aStatus != NS_BASE_STREAM_CLOSED) {
    LOG("Network error trying to download MPD: aStatus [%x].", aStatus);
    NetworkError();
  }
}

void
DASHDecoder::LoadAborted()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  if (!mNotifiedLoadAborted && mOwner) {
    LOG1("Load Aborted! Notifying media element.");
    mOwner->LoadAborted();
    mNotifiedLoadAborted = true;
  }
}

void
DASHDecoder::Suspend()
{
  MOZ_ASSERT(NS_IsMainThread());
  
  if (!mMPDManager && mResource) {
    LOG1("Suspending MPD download.");
    mResource->Suspend(true);
    return;
  }

  
  if (AudioRepDecoder()) {
    LOG("Suspending download for audio decoder [%p].", AudioRepDecoder());
    AudioRepDecoder()->Suspend();
  }
  if (VideoRepDecoder()) {
    LOG("Suspending download for video decoder [%p].", VideoRepDecoder());
    VideoRepDecoder()->Suspend();
  }
}

void
DASHDecoder::Resume(bool aForceBuffering)
{
  MOZ_ASSERT(NS_IsMainThread());
  
  if (!mMPDManager) {
    if (mResource) {
      LOG1("Resuming MPD download.");
      mResource->Resume();
    }
    if (aForceBuffering) {
      ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
      if (mDecoderStateMachine) {
        mDecoderStateMachine->StartBuffering();
      }
    }
  }

  
  if (AudioRepDecoder()) {
    LOG("Resuming download for audio decoder [%p].", AudioRepDecoder());
    AudioRepDecoder()->Resume(aForceBuffering);
  }
  if (VideoRepDecoder()) {
    LOG("Resuming download for video decoder [%p].", VideoRepDecoder());
    VideoRepDecoder()->Resume(aForceBuffering);
  }
}

void
DASHDecoder::Shutdown()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  LOG1("Shutting down.");

  
  if (mDASHReader) {
    mDASHReader->NotifyDecoderShuttingDown();
  }

  
  MediaDecoder::Shutdown();
  NS_ENSURE_TRUE_VOID(mShuttingDown);

  
  if (mMPDReaderThread) {
    nsresult rv = mMPDReaderThread->Shutdown();
    NS_ENSURE_SUCCESS_VOID(rv);
    mMPDReaderThread = nullptr;
  }

  
  for (uint i = 0; i < mAudioRepDecoders.Length(); i++) {
    if (mAudioRepDecoders[i]) {
      mAudioRepDecoders[i]->Shutdown();
    }
  }
  for (uint i = 0; i < mVideoRepDecoders.Length(); i++) {
    if (mVideoRepDecoders[i]) {
      mVideoRepDecoders[i]->Shutdown();
    }
  }
}

void
DASHDecoder::DecodeError()
{
  if (NS_IsMainThread()) {
    MediaDecoder::DecodeError();
  } else {
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(this, &MediaDecoder::DecodeError);
    nsresult rv = NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
    if (NS_FAILED(rv)) {
      LOG("Error dispatching DecodeError event to main thread: rv[%x]", rv);
    }
  }
}

void
DASHDecoder::OnReadMetadataCompleted(DASHRepDecoder* aRepDecoder)
{
  if (mShuttingDown) {
    LOG1("Shutting down! Ignoring OnReadMetadataCompleted().");
    return;
  }

  NS_ASSERTION(aRepDecoder, "aRepDecoder is null!");
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");

  LOG("Metadata loaded for decoder[%p]", aRepDecoder);

  
  nsRefPtr<DASHRepDecoder> activeDecoder;
  {
    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
    for (uint32_t i = 0; i < mAudioRepDecoders.Length(); i++) {
      if (aRepDecoder == mAudioRepDecoders[i]) {
        --mAudioMetadataReadCount;
        break;
      }
    }
    for (uint32_t i = 0; i < mVideoRepDecoders.Length(); i++) {
      if (aRepDecoder == mVideoRepDecoders[i]) {
        --mVideoMetadataReadCount;
        break;
      }
    }
  }

  
  
  if (mAudioMetadataReadCount == 0 && mVideoMetadataReadCount == 0) {
    if (AudioRepDecoder()) {
      LOG("Dispatching load event for audio decoder [%p]", AudioRepDecoder());
      nsCOMPtr<nsIRunnable> event =
        NS_NewRunnableMethod(AudioRepDecoder(), &DASHRepDecoder::LoadNextByteRange);
      nsresult rv = NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
      if (NS_FAILED(rv)) {
        LOG("Error dispatching audio decoder [%p] load event to main thread: "
            "rv[%x]", AudioRepDecoder(), rv);
        DecodeError();
        return;
      }
    }
    if (VideoRepDecoder()) {
      LOG("Dispatching load event for video decoder [%p]", VideoRepDecoder());
      
      NS_ASSERTION(mVideoSubsegmentLoads.IsEmpty(),
                   "No subsegment loads should be recorded at this stage!");
      NS_ASSERTION(mVideoSubsegmentIdx == 0,
                   "Current subsegment should be 0 at this stage!");
      LOG("Appending decoder [%d] [%p] to mVideoSubsegmentLoads at index "
          "[%d] before load; mVideoSubsegmentIdx[%d].",
          mVideoRepDecoderIdx, VideoRepDecoder(),
          (uint32_t)mVideoSubsegmentLoads.Length(), mVideoSubsegmentIdx);
      mVideoSubsegmentLoads.AppendElement(mVideoRepDecoderIdx);

      nsCOMPtr<nsIRunnable> event =
        NS_NewRunnableMethod(VideoRepDecoder(), &DASHRepDecoder::LoadNextByteRange);
      nsresult rv = NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
      if (NS_FAILED(rv)) {
        LOG("Error dispatching video decoder [%p] load event to main thread: "
            "rv[%x]", VideoRepDecoder(), rv);
        DecodeError();
        return;
      }
    }
  }
}

nsresult
DASHDecoder::PossiblySwitchDecoder(DASHRepDecoder* aRepDecoder)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  NS_ENSURE_FALSE(mShuttingDown, NS_ERROR_UNEXPECTED);
  NS_ENSURE_TRUE(aRepDecoder == VideoRepDecoder(), NS_ERROR_ILLEGAL_VALUE);
  NS_ASSERTION((uint32_t)mVideoRepDecoderIdx < mVideoRepDecoders.Length(),
               "Index for video decoder is out of bounds!");
  NS_ASSERTION((uint32_t)mVideoSubsegmentIdx < VideoRepDecoder()->GetNumDataByteRanges(),
               "Can't switch to a byte range out of bounds.");
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());

  
  
  
  
  NS_ASSERTION(VideoRepDecoder(), "Video decoder should not be null.");
  NS_ASSERTION(VideoRepDecoder()->GetResource(),
               "Video resource should not be null");
  bool reliable = false;
  double downloadRate = 0;
  {
    MutexAutoLock lock(mStatisticsLock);
    downloadRate = mVideoStatistics->GetRate(&reliable);
  }
  uint32_t bestRepIdx = UINT32_MAX;
  bool noRepAvailable = !mMPDManager->GetBestRepForBandwidth(mVideoAdaptSetIdx,
                                                             downloadRate,
                                                             bestRepIdx);
  LOG("downloadRate [%0.2f kbps] reliable [%s] bestRepIdx [%d] noRepAvailable [%s]",
      downloadRate/1000.0, (reliable ? "yes" : "no"), bestRepIdx,
      (noRepAvailable ? "yes" : "no"));

  
  
  
  uint32_t toDecoderIdx = mVideoRepDecoderIdx;
  if (bestRepIdx > toDecoderIdx) {
    toDecoderIdx = std::min(toDecoderIdx+1, mVideoRepDecoders.Length()-1);
  } else if (toDecoderIdx < bestRepIdx) {
    
    
    toDecoderIdx = bestRepIdx;
  }

  
  
  if (mVideoSubsegmentIdx < mVideoSubsegmentLoads.Length() &&
      toDecoderIdx < mVideoSubsegmentLoads[mVideoSubsegmentIdx]) {
    
    uint32_t betterRepIdx = mVideoSubsegmentLoads[mVideoSubsegmentIdx];
    if (mVideoRepDecoders[betterRepIdx]->IsSubsegmentCached(mVideoSubsegmentIdx)) {
      toDecoderIdx = betterRepIdx;
    }
  }

  NS_ENSURE_TRUE(toDecoderIdx < mVideoRepDecoders.Length(),
                 NS_ERROR_ILLEGAL_VALUE);

  
  if (toDecoderIdx != (uint32_t)mVideoRepDecoderIdx) {
    LOG("*** Switching video decoder from [%d] [%p] to [%d] [%p] at "
        "subsegment [%d]", mVideoRepDecoderIdx, VideoRepDecoder(),
        toDecoderIdx, mVideoRepDecoders[toDecoderIdx].get(),
        mVideoSubsegmentIdx);

    
    
    mDASHReader->RequestVideoReaderSwitch(mVideoRepDecoderIdx, toDecoderIdx,
                                          mVideoSubsegmentIdx);
    
    mVideoRepDecoders[mVideoRepDecoderIdx]->PrepareForSwitch();
    
    mVideoRepDecoderIdx = toDecoderIdx;
  }

  return NS_OK;
}

nsresult
DASHDecoder::Seek(double aTime)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  NS_ENSURE_FALSE(mShuttingDown, NS_ERROR_UNEXPECTED);

  LOG("Seeking to [%.2fs]", aTime);

  {
    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
    
    
    mSeeking = true;
  }

  return MediaDecoder::Seek(aTime);
}

void
DASHDecoder::NotifySeekInVideoSubsegment(int32_t aRepDecoderIdx,
                                         int32_t aSubsegmentIdx)
{
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");

  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());

  NS_ASSERTION(0 <= aRepDecoderIdx &&
               aRepDecoderIdx < mVideoRepDecoders.Length(),
               "Video decoder index is out of bounds");

  
  mVideoSubsegmentIdx = aSubsegmentIdx;
  
  
  mVideoRepDecoderIdx = aRepDecoderIdx;

  mSeeking = false;

  LOG("Dispatching load for video decoder [%d] [%p]: seek in subsegment [%d]",
      mVideoRepDecoderIdx, VideoRepDecoder(), aSubsegmentIdx);

  nsCOMPtr<nsIRunnable> event =
    NS_NewRunnableMethod(VideoRepDecoder(),
                         &DASHRepDecoder::LoadNextByteRange);
  nsresult rv = NS_DispatchToMainThread(event);
  if (NS_FAILED(rv)) {
    LOG("Error dispatching video byte range load: rv[0x%x].",
        rv);
    NetworkError();
    return;
  }
}

void
DASHDecoder::NotifySeekInAudioSubsegment(int32_t aSubsegmentIdx)
{
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");

  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());

  
  mAudioSubsegmentIdx = aSubsegmentIdx;

  LOG("Dispatching seeking load for audio decoder [%d] [%p]: subsegment [%d]",
     mAudioRepDecoderIdx, AudioRepDecoder(), aSubsegmentIdx);

  nsCOMPtr<nsIRunnable> event =
    NS_NewRunnableMethod(AudioRepDecoder(),
                         &DASHRepDecoder::LoadNextByteRange);
  nsresult rv = NS_DispatchToMainThread(event);
  if (NS_FAILED(rv)) {
    LOG("Error dispatching audio byte range load: rv[0x%x].",
        rv);
    NetworkError();
    return;
  }
}

bool
DASHDecoder::IsDecoderAllowedToDownloadData(DASHRepDecoder* aRepDecoder)
{
  NS_ASSERTION(aRepDecoder, "DASHRepDecoder pointer is null.");

  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  LOG("Checking aRepDecoder [%p] with AudioRepDecoder [%p] metadataReadCount "
      "[%d] and VideoRepDecoder [%p] metadataReadCount [%d]",
      aRepDecoder, AudioRepDecoder(), mAudioMetadataReadCount,
      VideoRepDecoder(), mVideoMetadataReadCount);
  
  
  return ((aRepDecoder == AudioRepDecoder() && mAudioMetadataReadCount == 0) ||
          (aRepDecoder == VideoRepDecoder() && mVideoMetadataReadCount == 0));
}

bool
DASHDecoder::IsDecoderAllowedToDownloadSubsegment(DASHRepDecoder* aRepDecoder,
                                                  int32_t const aSubsegmentIdx)
{
  NS_ASSERTION(aRepDecoder, "DASHRepDecoder pointer is null.");

  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());

  
  if (mSeeking) {
    return false;
  }
  
  if (mAudioMetadataReadCount != 0 || mVideoMetadataReadCount != 0) {
    return false;
  }
  
  if (aRepDecoder == AudioRepDecoder()) {
    return true;
  }

  int32_t videoDecoderIdx = GetRepIdxForVideoSubsegmentLoad(aSubsegmentIdx);
  if (aRepDecoder == mVideoRepDecoders[videoDecoderIdx]) {
    return true;
  }
  return false;
}

void
DASHDecoder::SetSubsegmentIndex(DASHRepDecoder* aRepDecoder,
                                int32_t aSubsegmentIdx)
{
  NS_ASSERTION(0 <= aSubsegmentIdx,
               "Subsegment index should not be negative!");
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  if (aRepDecoder == AudioRepDecoder()) {
    mAudioSubsegmentIdx = aSubsegmentIdx;
  } else if (aRepDecoder == VideoRepDecoder()) {
    
    
    
    
    
    mVideoSubsegmentIdx = aSubsegmentIdx;
  }
}

double
DASHDecoder::ComputePlaybackRate(bool* aReliable)
{
  GetReentrantMonitor().AssertCurrentThreadIn();
  MOZ_ASSERT(NS_IsMainThread() || OnStateMachineThread());
  NS_ASSERTION(aReliable, "Bool pointer aRelible should not be null!");

  
  if (mResource && !mMPDManager) {
    return 0;
  }

  
  
  
  double videoRate = 0;
  if (VideoRepDecoder()) {
    videoRate = VideoRepDecoder()->ComputePlaybackRate(aReliable);
  }
  return videoRate;
}

void
DASHDecoder::UpdatePlaybackRate()
{
  MOZ_ASSERT(NS_IsMainThread() || OnStateMachineThread());
  GetReentrantMonitor().AssertCurrentThreadIn();
  
  
  if (mResource && !mMPDManager) {
    return;
  }
  
  
  if (AudioRepDecoder()) {
    AudioRepDecoder()->UpdatePlaybackRate();
  }
  if (VideoRepDecoder()) {
    VideoRepDecoder()->UpdatePlaybackRate();
  }
}

void
DASHDecoder::NotifyPlaybackStarted()
{
  GetReentrantMonitor().AssertCurrentThreadIn();
  
  
  if (mResource && !mMPDManager) {
    return;
  }
  
  
  if (AudioRepDecoder()) {
    AudioRepDecoder()->NotifyPlaybackStarted();
  }
  if (VideoRepDecoder()) {
    VideoRepDecoder()->NotifyPlaybackStarted();
  }
}

void
DASHDecoder::NotifyPlaybackStopped()
{
  GetReentrantMonitor().AssertCurrentThreadIn();
  
  
  if (mResource && !mMPDManager) {
    return;
  }
  
  
  if (AudioRepDecoder()) {
    AudioRepDecoder()->NotifyPlaybackStopped();
  }
  if (VideoRepDecoder()) {
    VideoRepDecoder()->NotifyPlaybackStopped();
  }
}

MediaDecoder::Statistics
DASHDecoder::GetStatistics()
{
  MOZ_ASSERT(NS_IsMainThread() || OnStateMachineThread());
  Statistics result;

  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  if (mResource && !mMPDManager) {
    return MediaDecoder::GetStatistics();
  }

  
  
  
  if (VideoRepDecoder() && VideoRepDecoder()->GetResource()) {
    MediaResource *resource = VideoRepDecoder()->GetResource();
    
    result.mDownloadRate =
      resource->GetDownloadRate(&result.mDownloadRateReliable);
    result.mDownloadPosition =
      resource->GetCachedDataEnd(VideoRepDecoder()->mDecoderPosition);
    result.mTotalBytes = resource->GetLength();
    result.mPlaybackRate = ComputePlaybackRate(&result.mPlaybackRateReliable);
    result.mDecoderPosition = VideoRepDecoder()->mDecoderPosition;
    result.mPlaybackPosition = VideoRepDecoder()->mPlaybackPosition;
  }
  else {
    result.mDownloadRate = 0;
    result.mDownloadRateReliable = true;
    result.mPlaybackRate = 0;
    result.mPlaybackRateReliable = true;
    result.mDecoderPosition = 0;
    result.mPlaybackPosition = 0;
    result.mDownloadPosition = 0;
    result.mTotalBytes = 0;
  }

  return result;
}

bool
DASHDecoder::IsDataCachedToEndOfResource()
{
  NS_ASSERTION(!mShuttingDown, "Don't call during shutdown!");
  GetReentrantMonitor().AssertCurrentThreadIn();

  if (!mMPDManager || !mResource) {
    return false;
  }

  bool resourceIsLoaded = false;
  if (VideoRepDecoder()) {
    resourceIsLoaded = VideoRepDecoder()->IsDataCachedToEndOfResource();
    LOG("IsDataCachedToEndOfResource for VideoRepDecoder %p = %s",
       VideoRepDecoder(), resourceIsLoaded ? "yes" : "no");
  }
  if (AudioRepDecoder()) {
    bool isAudioResourceLoaded =
      AudioRepDecoder()->IsDataCachedToEndOfResource();
    LOG("IsDataCachedToEndOfResource for AudioRepDecoder %p = %s",
       AudioRepDecoder(), isAudioResourceLoaded ? "yes" : "no");
    resourceIsLoaded = resourceIsLoaded && isAudioResourceLoaded;
  }

  return resourceIsLoaded;
}

void
DASHDecoder::StopProgressUpdates()
{
  MOZ_ASSERT(OnStateMachineThread() || OnDecodeThread());
  GetReentrantMonitor().AssertCurrentThreadIn();
  mIgnoreProgressData = true;
  for (uint32_t i = 0; i < mVideoRepDecoders.Length(); i++) {
    mVideoRepDecoders[i]->StopProgressUpdates();
  }
  for (uint32_t i = 0; i < mAudioRepDecoders.Length(); i++) {
    mAudioRepDecoders[i]->StopProgressUpdates();
  }
}

void
DASHDecoder::StartProgressUpdates()
{
  MOZ_ASSERT(OnStateMachineThread() || OnDecodeThread());
  GetReentrantMonitor().AssertCurrentThreadIn();
  mIgnoreProgressData = false;
  for (uint32_t i = 0; i < mVideoRepDecoders.Length(); i++) {
    mVideoRepDecoders[i]->StartProgressUpdates();
  }
  for (uint32_t i = 0; i < mAudioRepDecoders.Length(); i++) {
    mAudioRepDecoders[i]->StartProgressUpdates();
  }
}

int32_t
DASHDecoder::GetRepIdxForVideoSubsegmentLoadAfterSeek(int32_t aSubsegmentIndex)
{
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  
  
  if (aSubsegmentIndex < 1 ||
      aSubsegmentIndex >= VideoRepDecoder()->GetNumDataByteRanges()) {
    return -1;
  }
  
  
  
  
  while (mVideoSubsegmentIdx == aSubsegmentIndex-1) {
    LOG("Waiting for switching decision for video subsegment [%d].",
        aSubsegmentIndex);
    mon.Wait();
  }

  return mVideoSubsegmentLoads[aSubsegmentIndex];
}

} 
