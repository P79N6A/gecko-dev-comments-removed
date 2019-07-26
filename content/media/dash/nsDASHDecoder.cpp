














































































































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
#include "nsBuiltinDecoderStateMachine.h"
#include "nsWebMDecoder.h"
#include "nsWebMReader.h"
#include "nsDASHReader.h"
#include "nsDASHMPDParser.h"
#include "nsDASHRepDecoder.h"
#include "nsDASHDecoder.h"

#ifdef PR_LOGGING
extern PRLogModuleInfo* gBuiltinDecoderLog;
#define LOG(msg, ...) PR_LOG(gBuiltinDecoderLog, PR_LOG_DEBUG, \
                             ("%p [nsDASHDecoder] " msg, this, __VA_ARGS__))
#define LOG1(msg) PR_LOG(gBuiltinDecoderLog, PR_LOG_DEBUG, \
                         ("%p [nsDASHDecoder] " msg, this))
#else
#define LOG(msg, ...)
#define LOG1(msg)
#endif

nsDASHDecoder::nsDASHDecoder() :
  nsBuiltinDecoder(),
  mNotifiedLoadAborted(false),
  mBuffer(nullptr),
  mBufferLength(0),
  mMPDReaderThread(nullptr),
  mPrincipal(nullptr),
  mDASHReader(nullptr),
  mAudioRepDecoder(nullptr),
  mVideoRepDecoder(nullptr)
{
  MOZ_COUNT_CTOR(nsDASHDecoder);
}

nsDASHDecoder::~nsDASHDecoder()
{
  MOZ_COUNT_DTOR(nsDASHDecoder);
}

nsDecoderStateMachine*
nsDASHDecoder::CreateStateMachine()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  return new nsBuiltinDecoderStateMachine(this, mDASHReader);
}

void
nsDASHDecoder::ReleaseStateMachine()
{
  NS_ASSERTION(NS_IsMainThread(), "Must be on main thread.");

  
  mDASHReader = nullptr;

  nsBuiltinDecoder::ReleaseStateMachine();
  for (uint i = 0; i < mAudioRepDecoders.Length(); i++) {
    mAudioRepDecoders[i]->ReleaseStateMachine();
  }
  for (uint i = 0; i < mVideoRepDecoders.Length(); i++) {
    mVideoRepDecoders[i]->ReleaseStateMachine();
  }
}

nsresult
nsDASHDecoder::Load(MediaResource* aResource,
                    nsIStreamListener** aStreamListener,
                    nsMediaDecoder* aCloneDonor)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  mDASHReader = new nsDASHReader(this);

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
nsDASHDecoder::NotifyDownloadEnded(nsresult aStatus)
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
      NS_NewRunnableMethod(this, &nsDASHDecoder::ReadMPDBuffer);
    NS_ENSURE_TRUE(event, );

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
    if (mElement) {
      mElement->LoadAborted();
    }
    return;
  } else if (aStatus != NS_BASE_STREAM_CLOSED) {
    LOG("Network error trying to download MPD: aStatus [%x].", aStatus);
    NetworkError();
  }
}

void
nsDASHDecoder::ReadMPDBuffer()
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
    NS_NewRunnableMethod(this, &nsDASHDecoder::OnReadMPDBufferCompleted);
  rv = NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
  if (NS_FAILED(rv)) {
    LOG("Error dispatching parse event to main thread: rv[%x]", rv);
    DecodeError();
    return;
  }
}

void
nsDASHDecoder::OnReadMPDBufferCompleted()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  if (mShuttingDown) {
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

  
  rv = mResource ? mResource->Close() : NS_ERROR_NULL_POINTER;
  if (NS_FAILED(rv)) {
    LOG("Media Resource did not close correctly! rv [%x]", rv);
    NetworkError();
    return;
  }

  
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
nsDASHDecoder::ParseMPDBuffer()
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
nsDASHDecoder::CreateRepDecoders()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  NS_ENSURE_TRUE(mMPDManager, NS_ERROR_NULL_POINTER);

  
  int64_t startTime = mMPDManager->GetStartTime();
  mDuration = mMPDManager->GetDuration();
  NS_ENSURE_TRUE(startTime >= 0 && mDuration > 0, NS_ERROR_ILLEGAL_VALUE);

  

  for (int i = 0; i < mMPDManager->GetNumAdaptationSets(); i++) {
    IMPDManager::AdaptationSetType asType = mMPDManager->GetAdaptationSetType(i);
    for (int j = 0; j < mMPDManager->GetNumRepresentations(i); j++) {
      
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

  NS_ENSURE_TRUE(mVideoRepDecoder, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mAudioRepDecoder, NS_ERROR_NOT_INITIALIZED);

  return NS_OK;
}

nsresult
nsDASHDecoder::CreateAudioRepDecoder(nsIURI* aUrl,
                                     mozilla::net::Representation const * aRep)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  NS_ENSURE_ARG(aUrl);
  NS_ENSURE_ARG(aRep);
  NS_ENSURE_TRUE(mElement, NS_ERROR_NOT_INITIALIZED);

  
  nsDASHRepDecoder* audioDecoder = new nsDASHRepDecoder(this);
  NS_ENSURE_TRUE(audioDecoder->Init(mElement), NS_ERROR_NOT_INITIALIZED);

  if (!mAudioRepDecoder) {
    mAudioRepDecoder = audioDecoder;
  }
  mAudioRepDecoders.AppendElement(audioDecoder);

  
  nsWebMReader* audioReader = new nsWebMReader(audioDecoder);
  if (mDASHReader) {
    mDASHReader->AddAudioReader(audioReader);
  }
  audioDecoder->SetReader(audioReader);

  
  MediaResource* audioResource
    = CreateAudioSubResource(aUrl, static_cast<nsMediaDecoder*>(audioDecoder));
  NS_ENSURE_TRUE(audioResource, NS_ERROR_NOT_INITIALIZED);

  audioDecoder->SetResource(audioResource);
  audioDecoder->SetMPDRepresentation(aRep);

  return NS_OK;
}

nsresult
nsDASHDecoder::CreateVideoRepDecoder(nsIURI* aUrl,
                                     mozilla::net::Representation const * aRep)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  NS_ENSURE_ARG(aUrl);
  NS_ENSURE_ARG(aRep);
  NS_ENSURE_TRUE(mElement, NS_ERROR_NOT_INITIALIZED);

  
  nsDASHRepDecoder* videoDecoder = new nsDASHRepDecoder(this);
  NS_ENSURE_TRUE(videoDecoder->Init(mElement), NS_ERROR_NOT_INITIALIZED);

  if (!mVideoRepDecoder) {
    mVideoRepDecoder = videoDecoder;
  }
  mVideoRepDecoders.AppendElement(videoDecoder);

  
  nsWebMReader* videoReader = new nsWebMReader(videoDecoder);
  if (mDASHReader) {
    mDASHReader->AddVideoReader(videoReader);
  }
  videoDecoder->SetReader(videoReader);

  
  MediaResource* videoResource
    = CreateVideoSubResource(aUrl, static_cast<nsMediaDecoder*>(videoDecoder));
  NS_ENSURE_TRUE(videoResource, NS_ERROR_NOT_INITIALIZED);

  videoDecoder->SetResource(videoResource);
  videoDecoder->SetMPDRepresentation(aRep);

  return NS_OK;
}

mozilla::MediaResource*
nsDASHDecoder::CreateAudioSubResource(nsIURI* aUrl,
                                      nsMediaDecoder* aAudioDecoder)
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

  return audioResource;
}

mozilla::MediaResource*
nsDASHDecoder::CreateVideoSubResource(nsIURI* aUrl,
                                      nsMediaDecoder* aVideoDecoder)
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

  return videoResource;
}

nsresult
nsDASHDecoder::CreateSubChannel(nsIURI* aUrl, nsIChannel** aChannel)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  NS_ENSURE_ARG(aUrl);

  nsCOMPtr<nsILoadGroup> loadGroup = mElement->GetDocumentLoadGroup();
  NS_ENSURE_TRUE(loadGroup, NS_ERROR_NULL_POINTER);

  
  
  nsCOMPtr<nsIChannelPolicy> channelPolicy;
  nsCOMPtr<nsIContentSecurityPolicy> csp;
  nsresult rv = mElement->NodePrincipal()->GetCsp(getter_AddRefs(csp));
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
nsDASHDecoder::LoadRepresentations()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  nsresult rv;
  {
    
    
    
    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());

    
    if (mAudioRepDecoder) {
      rv = mAudioRepDecoder->Load();
      NS_ENSURE_SUCCESS(rv, rv);
    }
    if (mVideoRepDecoder) {
      rv = mVideoRepDecoder->Load();
      NS_ENSURE_SUCCESS(rv, rv);
    }
    if (NS_FAILED(rv)) {
      LOG("Failed to open stream! rv [%x].", rv);
      return rv;
    }
  }

  if (mAudioRepDecoder) {
    mAudioRepDecoder->SetStateMachine(mDecoderStateMachine);
  }
  if (mVideoRepDecoder) {
    mVideoRepDecoder->SetStateMachine(mDecoderStateMachine);
  }

  
  return InitializeStateMachine(nullptr);
}

void
nsDASHDecoder::NotifyDownloadEnded(nsDASHRepDecoder* aRepDecoder,
                                   nsresult aStatus,
                                   MediaByteRange &aRange)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  
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
    
    if (aRepDecoder != mAudioRepDecoder && aRepDecoder != mVideoRepDecoder) {
      LOG("Error! Decoder [%p] does not match current sub-decoders!",
          aRepDecoder);
      DecodeError();
      return;
    }
    LOG("Byte range downloaded: decoder [%p] range requested [%d - %d]",
        aRepDecoder, aRange.mStart, aRange.mEnd);

    
    
    
    aRepDecoder->LoadNextByteRange();
    return;
  } else if (aStatus == NS_BINDING_ABORTED) {
    LOG("MPD download has been cancelled by the user: aStatus [%x].", aStatus);
    if (mElement) {
      mElement->LoadAborted();
    }
    return;
  } else if (aStatus != NS_BASE_STREAM_CLOSED) {
    LOG("Network error trying to download MPD: aStatus [%x].", aStatus);
    NetworkError();
  }
}

void
nsDASHDecoder::LoadAborted()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  if (!mNotifiedLoadAborted && mElement) {
    mElement->LoadAborted();
    mNotifiedLoadAborted = true;
    LOG1("Load Aborted! Notifying media element.");
  }
}

void
nsDASHDecoder::Shutdown()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  
  if (mDASHReader) {
    mDASHReader->NotifyDecoderShuttingDown();
  }

  
  nsBuiltinDecoder::Shutdown();
  NS_ENSURE_TRUE(mShuttingDown, );

  
  if (mMPDReaderThread) {
    nsresult rv = mMPDReaderThread->Shutdown();
    NS_ENSURE_SUCCESS(rv, );
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
nsDASHDecoder::DecodeError()
{
  if (NS_IsMainThread()) {
    nsBuiltinDecoder::DecodeError();
  } else {
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(this, &nsBuiltinDecoder::DecodeError);
    nsresult rv = NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
    if (NS_FAILED(rv)) {
      LOG("Error dispatching DecodeError event to main thread: rv[%x]", rv);
    }
  }
}
