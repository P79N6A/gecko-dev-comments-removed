


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

  nsresult CreateMediaPipeline(const JsepTrackPair& aTrackPair,
                               const JsepTrack& aTrack);

private:
  nsresult CreateMediaPipelineReceiving(
      RefPtr<TransportFlow> aRtpFlow,
      RefPtr<TransportFlow> aRtcpFlow,
      RefPtr<TransportFlow> aBundleRtpFlow,
      RefPtr<TransportFlow> aBundleRtcpFlow,
      const JsepTrackPair& aTrackPair,
      const JsepTrack& aTrack,
      const RefPtr<MediaSessionConduit>& aConduit);

  nsresult CreateMediaPipelineSending(
      RefPtr<TransportFlow> aRtpFlow,
      RefPtr<TransportFlow> aRtcpFlow,
      RefPtr<TransportFlow> aBundleRtpFlow,
      RefPtr<TransportFlow> aBundleRtcpFlow,
      const JsepTrackPair& aTrackPair,
      const JsepTrack& aTrack,
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

private:
  
  
  PeerConnectionMedia* mPCMedia;
  PeerConnectionImpl* mPC;
};

} 

#endif
