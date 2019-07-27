


#ifndef _MEDIAPIPELINEFACTORY_H_
#define _MEDIAPIPELINEFACTORY_H_

#include "MediaConduitInterface.h"
#include "PeerConnectionMedia.h"
#include "transportflow.h"

#include "signaling/src/jsep/JsepTrack.h"
#include "mozilla/RefPtr.h"
#include "mozilla/UniquePtr.h"

namespace mozilla {

class MediaPipelineFactory
{
public:
  explicit MediaPipelineFactory(PeerConnectionMedia* aPCMedia)
      : mPCMedia(aPCMedia), mPC(aPCMedia->GetPC())
  {
  }

  nsresult CreateOrUpdateMediaPipeline(const JsepTrackPair& aTrackPair,
                                       const JsepTrack& aTrack);

private:
  nsresult CreateMediaPipelineReceiving(
      const JsepTrackPair& aTrackPair,
      const JsepTrack& aTrack,
      size_t level,
      RefPtr<TransportFlow> aRtpFlow,
      RefPtr<TransportFlow> aRtcpFlow,
      nsAutoPtr<MediaPipelineFilter> filter,
      const RefPtr<MediaSessionConduit>& aConduit);

  nsresult CreateMediaPipelineSending(
      const JsepTrackPair& aTrackPair,
      const JsepTrack& aTrack,
      size_t level,
      RefPtr<TransportFlow> aRtpFlow,
      RefPtr<TransportFlow> aRtcpFlow,
      nsAutoPtr<MediaPipelineFilter> filter,
      const RefPtr<MediaSessionConduit>& aConduit);

  nsresult CreateAudioConduit(const JsepTrackPair& aTrackPair,
                              const JsepTrack& aTrack,
                              RefPtr<MediaSessionConduit>* aConduitp);

  nsresult CreateVideoConduit(const JsepTrackPair& aTrackPair,
                              const JsepTrack& aTrack,
                              RefPtr<MediaSessionConduit>* aConduitp);

  MediaConduitErrorCode EnsureExternalCodec(VideoSessionConduit& aConduit,
                                            VideoCodecConfig* aConfig,
                                            bool aIsSend);

  nsresult CreateOrGetTransportFlow(size_t aLevel, bool aIsRtcp,
                                    const JsepTransport& transport,
                                    RefPtr<TransportFlow>* out);

  nsresult GetTransportParameters(const JsepTrackPair& aTrackPair,
                                  const JsepTrack& aTrack,
                                  size_t* aLevelOut,
                                  RefPtr<TransportFlow>* aRtpOut,
                                  RefPtr<TransportFlow>* aRtcpOut,
                                  nsAutoPtr<MediaPipelineFilter>* aFilterOut);

private:
  
  
  PeerConnectionMedia* mPCMedia;
  PeerConnectionImpl* mPC;
};

} 

#endif
