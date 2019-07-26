


#include <string>

#include "nspr.h"
#include "cc_constants.h"
#include "CSFLog.h"
#include "CSFLogStream.h"

#include "nricectx.h"
#include "nricemediastream.h"
#include "PeerConnectionImpl.h"
#include "PeerConnectionMedia.h"
#include "AudioConduit.h"
#include "VideoConduit.h"
#include "runnable_utils.h"

#ifdef MOZILLA_INTERNAL_API
#include "MediaStreamList.h"
#include "nsIScriptGlobalObject.h"
#include "jsapi.h"
#endif

using namespace mozilla;

namespace sipcc {

static const char* logTag = "PeerConnectionMedia";
static const mozilla::TrackID TRACK_AUDIO = 0;
static const mozilla::TrackID TRACK_VIDEO = 1;





void
LocalSourceStreamInfo::ExpectAudio(const mozilla::TrackID aID)
{
  mAudioTracks.AppendElement(aID);
}



void
LocalSourceStreamInfo::ExpectVideo(const mozilla::TrackID aID)
{
  mVideoTracks.AppendElement(aID);
}

unsigned
LocalSourceStreamInfo::AudioTrackCount()
{
  return mAudioTracks.Length();
}

unsigned
LocalSourceStreamInfo::VideoTrackCount()
{
  return mVideoTracks.Length();
}

PeerConnectionImpl* PeerConnectionImpl::CreatePeerConnection()
{
  PeerConnectionImpl *pc = new PeerConnectionImpl();

  CSFLogDebugS(logTag, "Created PeerConnection: " << static_cast<void*>(pc));

  return pc;
}


nsresult PeerConnectionMedia::Init(const std::vector<NrIceStunServer>& stun_servers)
{
  
  
  mIceCtx = NrIceCtx::Create("PC:" + mParent->GetHandle(), true);
  if(!mIceCtx) {
    CSFLogError(logTag, "%s: Failed to create Ice Context", __FUNCTION__);
    return NS_ERROR_FAILURE;
  }
  nsresult rv = mIceCtx->SetStunServers(stun_servers);
  if (NS_FAILED(rv)) {
    return rv;
  }
  mIceCtx->SignalGatheringCompleted.connect(this,
                                            &PeerConnectionMedia::IceGatheringCompleted);
  mIceCtx->SignalCompleted.connect(this,
                                   &PeerConnectionMedia::IceCompleted);

  
  
  
  RefPtr<NrIceMediaStream> audioStream = mIceCtx->CreateStream("stream1", 2);
  RefPtr<NrIceMediaStream> videoStream = mIceCtx->CreateStream("stream2", 2);
  RefPtr<NrIceMediaStream> dcStream = mIceCtx->CreateStream("stream3", 2);

  if (!audioStream) {
    CSFLogError(logTag, "%s: audio stream is NULL", __FUNCTION__);
    return NS_ERROR_FAILURE;
  } else {
    mIceStreams.push_back(audioStream);
  }

  if (!videoStream) {
    CSFLogError(logTag, "%s: video stream is NULL", __FUNCTION__);
    return NS_ERROR_FAILURE;
  } else {
    mIceStreams.push_back(videoStream);
  }

  if (!dcStream) {
    CSFLogError(logTag, "%s: datachannel stream is NULL", __FUNCTION__);
    return NS_ERROR_FAILURE;
  } else {
    mIceStreams.push_back(dcStream);
  }

  
  
  for (std::size_t i=0; i<mIceStreams.size(); i++) {
    mIceStreams[i]->SignalReady.connect(this, &PeerConnectionMedia::IceStreamReady);
  }

  
  nsresult res;
  mIceCtx->thread()->Dispatch(WrapRunnableRet(
    mIceCtx, &NrIceCtx::StartGathering, &res), NS_DISPATCH_SYNC
  );

  if (NS_FAILED(res)) {
    CSFLogErrorS(logTag, __FUNCTION__ << ": StartGathering failed: " <<
        static_cast<uint32_t>(res));
    return res;
  }

  return NS_OK;
}

nsresult
PeerConnectionMedia::AddStream(nsIDOMMediaStream* aMediaStream, uint32_t *stream_id)
{
  if (!aMediaStream) {
    CSFLogError(logTag, "%s - aMediaStream is NULL", __FUNCTION__);
    return NS_ERROR_FAILURE;
  }

  DOMMediaStream* stream = static_cast<DOMMediaStream*>(aMediaStream);

  CSFLogDebugS(logTag, __FUNCTION__ << ": MediaStream: " << static_cast<void*>(aMediaStream));

  
  uint32_t hints = stream->GetHintContents();

  if (!(hints & (DOMMediaStream::HINT_CONTENTS_AUDIO |
        DOMMediaStream::HINT_CONTENTS_VIDEO))) {
    CSFLogDebug(logTag, "Empty Stream !!");
    return NS_OK;
  }

  
  
  
  
  mozilla::MutexAutoLock lock(mLocalSourceStreamsLock);
  for (uint32_t u = 0; u < mLocalSourceStreams.Length(); u++) {
    nsRefPtr<LocalSourceStreamInfo> localSourceStream = mLocalSourceStreams[u];

    if (localSourceStream->GetMediaStream()->GetHintContents() & hints) {
      CSFLogError(logTag, "Only one stream of any given type allowed");
      return NS_ERROR_FAILURE;
    }
  }

  
  nsRefPtr<LocalSourceStreamInfo> localSourceStream =
    new LocalSourceStreamInfo(stream);
  *stream_id = mLocalSourceStreams.Length();

  if (hints & DOMMediaStream::HINT_CONTENTS_AUDIO) {
    localSourceStream->ExpectAudio(TRACK_AUDIO);
  }

  if (hints & DOMMediaStream::HINT_CONTENTS_VIDEO) {
    localSourceStream->ExpectVideo(TRACK_VIDEO);
  }

  mLocalSourceStreams.AppendElement(localSourceStream);

  return NS_OK;
}

nsresult
PeerConnectionMedia::RemoveStream(nsIDOMMediaStream* aMediaStream, uint32_t *stream_id)
{
  MOZ_ASSERT(aMediaStream);

  DOMMediaStream* stream = static_cast<DOMMediaStream*>(aMediaStream);

  CSFLogDebugS(logTag, __FUNCTION__ << ": MediaStream: " << static_cast<void*>(aMediaStream));

  mozilla::MutexAutoLock lock(mLocalSourceStreamsLock);
  for (uint32_t u = 0; u < mLocalSourceStreams.Length(); u++) {
    nsRefPtr<LocalSourceStreamInfo> localSourceStream = mLocalSourceStreams[u];
    if (localSourceStream->GetMediaStream() == stream) {
      *stream_id = u;
      return NS_OK;
    }
  }

  return NS_ERROR_ILLEGAL_VALUE;
}

void
PeerConnectionMedia::SelfDestruct()
{
   CSFLogDebug(logTag, "%s: Disconnecting media streams from PC", __FUNCTION__);

   DisconnectMediaStreams();

   CSFLogDebug(logTag, "%s: Disconnecting transport", __FUNCTION__);
   
   RUN_ON_THREAD(mParent->GetSTSThread(), WrapRunnable(
       this, &PeerConnectionMedia::ShutdownMediaTransport), NS_DISPATCH_SYNC);

  CSFLogDebug(logTag, "%s: Media shut down", __FUNCTION__);

  
  this->Release();
}

void
PeerConnectionMedia::DisconnectMediaStreams()
{
  for (uint32_t i=0; i < mLocalSourceStreams.Length(); ++i) {
    mLocalSourceStreams[i]->Detach();
  }

  for (uint32_t i=0; i < mRemoteSourceStreams.Length(); ++i) {
    mRemoteSourceStreams[i]->Detach();
  }

  mLocalSourceStreams.Clear();
  mRemoteSourceStreams.Clear();
}

void
PeerConnectionMedia::ShutdownMediaTransport()
{
  disconnect_all();
  mTransportFlows.clear();
  mIceStreams.clear();
  mIceCtx = NULL;
}

LocalSourceStreamInfo*
PeerConnectionMedia::GetLocalStream(int aIndex)
{
  if(aIndex < 0 || aIndex >= (int) mLocalSourceStreams.Length()) {
    return NULL;
  }

  MOZ_ASSERT(mLocalSourceStreams[aIndex]);
  return mLocalSourceStreams[aIndex];
}

RemoteSourceStreamInfo*
PeerConnectionMedia::GetRemoteStream(int aIndex)
{
  if(aIndex < 0 || aIndex >= (int) mRemoteSourceStreams.Length()) {
    return NULL;
  }

  MOZ_ASSERT(mRemoteSourceStreams[aIndex]);
  return mRemoteSourceStreams[aIndex];
}

nsresult
PeerConnectionMedia::AddRemoteStream(nsRefPtr<RemoteSourceStreamInfo> aInfo,
  int *aIndex)
{
  MOZ_ASSERT(aIndex);

  *aIndex = mRemoteSourceStreams.Length();

  mRemoteSourceStreams.AppendElement(aInfo);

  return NS_OK;
}


void
PeerConnectionMedia::IceGatheringCompleted(NrIceCtx *aCtx)
{
  MOZ_ASSERT(aCtx);
  SignalIceGatheringCompleted(aCtx);
}

void
PeerConnectionMedia::IceCompleted(NrIceCtx *aCtx)
{
  MOZ_ASSERT(aCtx);
  SignalIceCompleted(aCtx);
}

void
PeerConnectionMedia::IceStreamReady(NrIceMediaStream *aStream)
{
  MOZ_ASSERT(aStream);

  CSFLogDebugS(logTag, __FUNCTION__ << ": "  << aStream->name().c_str());
}


void
LocalSourceStreamInfo::StorePipeline(int aTrack,
  mozilla::RefPtr<mozilla::MediaPipeline> aPipeline)
{
  MOZ_ASSERT(mPipelines.find(aTrack) == mPipelines.end());
  if (mPipelines.find(aTrack) != mPipelines.end()) {
    CSFLogError(logTag, "%s: Storing duplicate track", __FUNCTION__);
    return;
  }
  
  
  mPipelines[aTrack] = aPipeline;
}

void
RemoteSourceStreamInfo::StorePipeline(int aTrack,
                                      bool aIsVideo,
                                      mozilla::RefPtr<mozilla::MediaPipeline> aPipeline)
{
  MOZ_ASSERT(mPipelines.find(aTrack) == mPipelines.end());
  if (mPipelines.find(aTrack) != mPipelines.end()) {
    CSFLogErrorS(logTag, __FUNCTION__ << ": Request to store duplicate track " << aTrack);
    return;
  }
  CSFLogDebug(logTag, "%s track %d %s = %p", __FUNCTION__, aTrack, aIsVideo ? "video" : "audio",
              aPipeline.get());
  
  
  for (std::map<int, bool>::iterator it = mTypes.begin(); it != mTypes.end(); ++it) {
    if (it->second != aIsVideo) {
      
      mozilla::WebrtcAudioConduit *audio_conduit = static_cast<mozilla::WebrtcAudioConduit*>
                                                   (aIsVideo ?
                                                    mPipelines[it->first]->Conduit() :
                                                    aPipeline->Conduit());
      mozilla::WebrtcVideoConduit *video_conduit = static_cast<mozilla::WebrtcVideoConduit*>
                                                   (aIsVideo ?
                                                    aPipeline->Conduit() :
                                                    mPipelines[it->first]->Conduit());
      video_conduit->SyncTo(audio_conduit);
      CSFLogDebug(logTag, "Syncing %p to %p, %d to %d", video_conduit, audio_conduit,
                  aTrack, it->first);
    }
  }
  
  
  mPipelines[aTrack] = aPipeline;
  
  mTypes[aTrack] = aIsVideo;
}


}  
