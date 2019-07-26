



#ifndef _PEER_CONNECTION_MEDIA_H_
#define _PEER_CONNECTION_MEDIA_H_

#include <string>
#include <iostream>
#include <vector>
#include <map>

#include "nspr.h"
#include "prlock.h"

#include "mozilla/RefPtr.h"
#include "nsComponentManagerUtils.h"

#ifdef USE_FAKE_MEDIA_STREAMS
#include "FakeMediaStreams.h"
#else
#include "nsDOMMediaStream.h"
#include "MediaSegment.h"
#endif

#include "AudioSegment.h"

#ifdef MOZILLA_INTERNAL_API
#include "Layers.h"
#include "VideoUtils.h"
#include "ImageLayers.h"
#include "VideoSegment.h"
#else
namespace mozilla {
  class DataChannel;
}
#endif

#include "nricectx.h"
#include "nricemediastream.h"
#include "MediaPipeline.h"

namespace sipcc {

class PeerConnectionImpl;


class Fake_AudioGenerator {
 public:
Fake_AudioGenerator(nsDOMMediaStream* aStream) : mStream(aStream), mCount(0) {
    mTimer = do_CreateInstance("@mozilla.org/timer;1");
    PR_ASSERT(mTimer);

    
    mozilla::AudioSegment *segment = new mozilla::AudioSegment();
    segment->Init(1); 
    mStream->GetStream()->AsSourceStream()->AddTrack(1, 16000, 0, segment);

    
    mTimer->InitWithFuncCallback(Callback, this, 100, nsITimer::TYPE_REPEATING_PRECISE);
  }

  static void Callback(nsITimer* timer, void *arg) {
    Fake_AudioGenerator* gen = static_cast<Fake_AudioGenerator*>(arg);

    nsRefPtr<mozilla::SharedBuffer> samples = mozilla::SharedBuffer::Create(1600 * 2 * sizeof(int16_t));
    for (int i=0; i<1600*2; i++) {
      reinterpret_cast<int16_t *>(samples->Data())[i] = ((gen->mCount % 8) * 4000) - (7*4000)/2;
      ++gen->mCount;
    }

    mozilla::AudioSegment segment;
    segment.Init(1);
    segment.AppendFrames(samples.forget(), 1600,
                         0, 1600, mozilla::AUDIO_FORMAT_S16);

    gen->mStream->GetStream()->AsSourceStream()->AppendToTrack(1, &segment);
  }

 private:
  nsCOMPtr<nsITimer> mTimer;
  nsRefPtr<nsDOMMediaStream> mStream;
  int mCount;
};


#ifdef MOZILLA_INTERNAL_API
class Fake_VideoGenerator {
 public:
  Fake_VideoGenerator(nsDOMMediaStream* aStream) {
    mStream = aStream;
    mCount = 0;
    mTimer = do_CreateInstance("@mozilla.org/timer;1");
    PR_ASSERT(mTimer);

    
    mozilla::VideoSegment *segment = new mozilla::VideoSegment();
    mStream->GetStream()->AsSourceStream()->AddTrack(1, USECS_PER_S, 0, segment);
    mStream->GetStream()->AsSourceStream()->AdvanceKnownTracksTime(mozilla::STREAM_TIME_MAX);

    
    mTimer->InitWithFuncCallback(Callback, this, 100, nsITimer::TYPE_REPEATING_SLACK);
  }

  static void Callback(nsITimer* timer, void *arg) {
    Fake_VideoGenerator* gen = static_cast<Fake_VideoGenerator*>(arg);

    const uint32_t WIDTH = 640;
    const uint32_t HEIGHT = 480;

    
    mozilla::ImageFormat format = mozilla::PLANAR_YCBCR;
    nsRefPtr<mozilla::layers::ImageContainer> container =
      mozilla::layers::LayerManager::CreateImageContainer();

    nsRefPtr<mozilla::layers::Image> image = container->CreateImage(&format, 1);

    int len = ((WIDTH * HEIGHT) * 3 / 2);
    mozilla::layers::PlanarYCbCrImage* planar =
      static_cast<mozilla::layers::PlanarYCbCrImage*>(image.get());
    uint8_t* frame = (uint8_t*) PR_Malloc(len);
    ++gen->mCount;
    memset(frame, (gen->mCount / 8) & 0xff, len); 

    const uint8_t lumaBpp = 8;
    const uint8_t chromaBpp = 4;

    mozilla::layers::PlanarYCbCrImage::Data data;
    data.mYChannel = frame;
    data.mYSize = gfxIntSize(WIDTH, HEIGHT);
    data.mYStride = (int32_t) (WIDTH * lumaBpp / 8.0);
    data.mCbCrStride = (int32_t) (WIDTH * chromaBpp / 8.0);
    data.mCbChannel = frame + HEIGHT * data.mYStride;
    data.mCrChannel = data.mCbChannel + HEIGHT * data.mCbCrStride / 2;
    data.mCbCrSize = gfxIntSize(WIDTH / 2, HEIGHT / 2);
    data.mPicX = 0;
    data.mPicY = 0;
    data.mPicSize = gfxIntSize(WIDTH, HEIGHT);
    data.mStereoMode = mozilla::STEREO_MODE_MONO;

    
    planar->SetData(data);
    PR_Free(frame);

    
    mozilla::VideoSegment *segment = new mozilla::VideoSegment();
    
    segment->AppendFrame(image.forget(), USECS_PER_S / 10, gfxIntSize(WIDTH, HEIGHT));

    gen->mStream->GetStream()->AsSourceStream()->AppendToTrack(1, segment);
  }

 private:
  nsCOMPtr<nsITimer> mTimer;
  nsRefPtr<nsDOMMediaStream> mStream;
  int mCount;
};
#endif

class LocalSourceStreamInfo {
public:
  LocalSourceStreamInfo(nsDOMMediaStream* aMediaStream)
    : mMediaStream(aMediaStream) {}
  ~LocalSourceStreamInfo() {
    mMediaStream = NULL;
  }

  nsDOMMediaStream* GetMediaStream() {
    return mMediaStream;
  }
  void StorePipeline(int aTrack, mozilla::RefPtr<mozilla::MediaPipeline> aPipeline);

  void ExpectAudio(const mozilla::TrackID);
  void ExpectVideo(const mozilla::TrackID);
  unsigned AudioTrackCount();
  unsigned VideoTrackCount();

  void Detach() {
    
    for (std::map<int, mozilla::RefPtr<mozilla::MediaPipeline> >::iterator it =
           mPipelines.begin(); it != mPipelines.end();
         ++it) {
      it->second->Shutdown();
    }
    mMediaStream = NULL;
  }

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(LocalSourceStreamInfo)
private:
  std::map<int, mozilla::RefPtr<mozilla::MediaPipeline> > mPipelines;
  nsRefPtr<nsDOMMediaStream> mMediaStream;
  nsTArray<mozilla::TrackID> mAudioTracks;
  nsTArray<mozilla::TrackID> mVideoTracks;
};

class RemoteSourceStreamInfo {
 public:
  RemoteSourceStreamInfo(nsDOMMediaStream* aMediaStream) :
    mMediaStream(already_AddRefed<nsDOMMediaStream>(aMediaStream)),
    mPipelines() {}

  nsDOMMediaStream* GetMediaStream() {
    return mMediaStream;
  }
  void StorePipeline(int aTrack, mozilla::RefPtr<mozilla::MediaPipeline> aPipeline);

  void Detach() {
    
    for (std::map<int, mozilla::RefPtr<mozilla::MediaPipeline> >::iterator it =
           mPipelines.begin(); it != mPipelines.end();
         ++it) {
      it->second->Shutdown();
    }
    mMediaStream = NULL;
  }

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(RemoteSourceStreamInfo)
 private:
  nsRefPtr<nsDOMMediaStream> mMediaStream;
  std::map<int, mozilla::RefPtr<mozilla::MediaPipeline> > mPipelines;
};

class PeerConnectionMedia : public sigslot::has_slots<> {
 public:
  PeerConnectionMedia(PeerConnectionImpl *parent)
      : mParent(parent),
      mLocalSourceStreamsLock(PR_NewLock()),
      mIceCtx(NULL) {}

  ~PeerConnectionMedia() {
    PR_DestroyLock(mLocalSourceStreamsLock);
  }

  nsresult Init();

  
  void SelfDestruct();

  mozilla::RefPtr<mozilla::NrIceCtx> ice_ctx() const { return mIceCtx; }

  mozilla::RefPtr<mozilla::NrIceMediaStream> ice_media_stream(size_t i) const {
    
    
    if (i >= mIceStreams.size()) {
      return NULL;
    }
    return mIceStreams[i];
  }

  
  nsresult AddStream(nsIDOMMediaStream* aMediaStream, uint32_t *stream_id);

  
  nsresult RemoveStream(nsIDOMMediaStream* aMediaStream, uint32_t *stream_id);

  
  uint32_t LocalStreamsLength()
  {
    return mLocalSourceStreams.Length();
  }
  LocalSourceStreamInfo* GetLocalStream(int index);

  
  uint32_t RemoteStreamsLength()
  {
    return mRemoteSourceStreams.Length();
  }
  RemoteSourceStreamInfo* GetRemoteStream(int index);

  
  nsresult AddRemoteStream(nsRefPtr<RemoteSourceStreamInfo> aInfo, int *aIndex);


  
  
  mozilla::RefPtr<mozilla::TransportFlow> GetTransportFlow(int aStreamIndex,
                                                           bool aIsRtcp) {
    int index_inner = aStreamIndex * 2 + (aIsRtcp ? 1 : 0);

    if (mTransportFlows.find(index_inner) == mTransportFlows.end())
      return NULL;

    return mTransportFlows[index_inner];
  }

  
  void AddTransportFlow(int aIndex, bool aRtcp,
                        mozilla::RefPtr<mozilla::TransportFlow> aFlow) {
    int index_inner = aIndex * 2 + (aRtcp ? 1 : 0);

    mTransportFlows[index_inner] = aFlow;
  }

  
  sigslot::signal1<mozilla::NrIceCtx *> SignalIceGatheringCompleted;  
  sigslot::signal1<mozilla::NrIceCtx *> SignalIceCompleted;  

 private:
  
  
  void DisconnectMediaStreams();

  
  void ShutdownMediaTransport();

  
  void IceGatheringCompleted(mozilla::NrIceCtx *aCtx);
  void IceCompleted(mozilla::NrIceCtx *aCtx);
  void IceStreamReady(mozilla::NrIceMediaStream *aStream);

  
  PeerConnectionImpl *mParent;

  
  PRLock *mLocalSourceStreamsLock;
  nsTArray<nsRefPtr<LocalSourceStreamInfo> > mLocalSourceStreams;

  
  PRLock *mRemoteSourceStreamsLock;
  nsTArray<nsRefPtr<RemoteSourceStreamInfo> > mRemoteSourceStreams;

  
  mozilla::RefPtr<mozilla::NrIceCtx> mIceCtx;
  std::vector<mozilla::RefPtr<mozilla::NrIceMediaStream> > mIceStreams;

  
  std::map<int, mozilla::RefPtr<mozilla::TransportFlow> > mTransportFlows;

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(PeerConnectionMedia)
};

}  
#endif
