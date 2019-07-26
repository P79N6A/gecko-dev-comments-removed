





#ifndef mediapipeline_h__
#define mediapipeline_h__

#include "sigslot.h"

#ifdef USE_FAKE_MEDIA_STREAMS
#include "FakeMediaStreams.h"
#else
#include "nsDOMMediaStream.h"
#endif
#include "MediaConduitInterface.h"
#include "AudioSegment.h"
#include "SrtpFlow.h"
#include "transportflow.h"

#ifdef MOZILLA_INTERNAL_API
#include "VideoSegment.h"
#endif

namespace mozilla {











class MediaPipeline : public sigslot::has_slots<> {
 public:
  enum Direction { TRANSMIT, RECEIVE };
  enum State { MP_CONNECTING, MP_OPEN, MP_CLOSED };
  MediaPipeline(Direction direction,
                nsCOMPtr<nsIEventTarget> main_thread,
                nsCOMPtr<nsIEventTarget> sts_thread,
                nsDOMMediaStream* stream,
                RefPtr<MediaSessionConduit> conduit,
                RefPtr<TransportFlow> rtp_transport,
                RefPtr<TransportFlow> rtcp_transport)
      : direction_(direction),
        stream_(stream),
        conduit_(conduit),
        rtp_transport_(rtp_transport),
        rtp_state_(MP_CONNECTING),
        rtcp_transport_(rtcp_transport),
        rtcp_state_(MP_CONNECTING),
        main_thread_(main_thread),
        sts_thread_(sts_thread),
        transport_(new PipelineTransport(this)),
        rtp_send_srtp_(),
        rtcp_send_srtp_(),
        rtp_recv_srtp_(),
        rtcp_recv_srtp_(),
        rtp_packets_sent_(0),
        rtcp_packets_sent_(0),
        rtp_packets_received_(0),
        rtcp_packets_received_(0),
        muxed_((rtcp_transport_ == NULL) || (rtp_transport_ == rtcp_transport_)) {
    Init();
  }

  virtual ~MediaPipeline() {
    DetachTransport();
  }

  virtual nsresult Init();

  virtual Direction direction() const { return direction_; }

  virtual void DetachMediaStream() {}
  virtual void DetachTransport();

  int rtp_packets_sent() const { return rtp_packets_sent_; }
  int rtcp_packets_sent() const { return rtp_packets_sent_; }
  int rtp_packets_received() const { return rtp_packets_received_; }
  int rtcp_packets_received() const { return rtp_packets_received_; }

  
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MediaPipeline)

  protected:
  
  class PipelineTransport : public TransportInterface {
   public:
    
    PipelineTransport(MediaPipeline *pipeline)
        : pipeline_(pipeline) {}
    void Detach() { pipeline_ = NULL; }
    MediaPipeline *pipeline() const { return pipeline_; }

    virtual nsresult SendRtpPacket(const void* data, int len);
    virtual nsresult SendRtcpPacket(const void* data, int len);

   private:
    MediaPipeline *pipeline_;  
  };
  friend class PipelineTransport;

  virtual nsresult TransportReady(TransportFlow *flow);  
  virtual nsresult TransportFailed(TransportFlow *flow);  

  void increment_rtp_packets_sent();
  void increment_rtcp_packets_sent();
  void increment_rtp_packets_received();
  void increment_rtcp_packets_received();

  virtual nsresult SendPacket(TransportFlow *flow, const void* data, int len);

  
  void StateChange(TransportFlow *flow, TransportLayer::State);
  void RtpPacketReceived(TransportLayer *layer, const unsigned char *data,
                         size_t len);
  void RtcpPacketReceived(TransportLayer *layer, const unsigned char *data,
                          size_t len);
  void PacketReceived(TransportLayer *layer, const unsigned char *data,
                      size_t len);


  Direction direction_;
  nsDOMMediaStream* stream_;
  RefPtr<MediaSessionConduit> conduit_;
  RefPtr<TransportFlow> rtp_transport_;
  State rtp_state_;
  RefPtr<TransportFlow> rtcp_transport_;
  State rtcp_state_;
  nsCOMPtr<nsIEventTarget> main_thread_;
  nsCOMPtr<nsIEventTarget> sts_thread_;
  RefPtr<PipelineTransport> transport_;
  bool transport_connected_;
  RefPtr<SrtpFlow> rtp_send_srtp_;
  RefPtr<SrtpFlow> rtcp_send_srtp_;
  RefPtr<SrtpFlow> rtp_recv_srtp_;
  RefPtr<SrtpFlow> rtcp_recv_srtp_;
  int rtp_packets_sent_;
  int rtcp_packets_sent_;
  int rtp_packets_received_;
  int rtcp_packets_received_;
  bool muxed_;

 private:
  virtual void DetachTransportInt();

  bool IsRtp(const unsigned char *data, size_t len);
};




class MediaPipelineTransmit : public MediaPipeline {
 public:
  MediaPipelineTransmit(nsCOMPtr<nsIEventTarget> main_thread,
                        nsCOMPtr<nsIEventTarget> sts_thread,
                        nsDOMMediaStream* stream,
                        RefPtr<MediaSessionConduit> conduit,
                        RefPtr<TransportFlow> rtp_transport,
                        RefPtr<TransportFlow> rtcp_transport) :
      MediaPipeline(TRANSMIT, main_thread, sts_thread,
                    stream, conduit, rtp_transport,
                    rtcp_transport),
      listener_(new PipelineListener(this)) {
    Init();  
  }

  
  nsresult Init();

  virtual ~MediaPipelineTransmit() {
    if (stream_ && listener_){
      stream_->GetStream()->RemoveListener(listener_);

      
      
      
      listener_->Detach();
    }
  }

  virtual void DetachMediaStream() {
    
    stream_->GetStream()->RemoveListener(listener_);
    stream_ = NULL;
    listener_->Detach();
  }

  
  class PipelineListener : public MediaStreamListener {
   public:
    PipelineListener(MediaPipelineTransmit *pipeline) :
    pipeline_(pipeline) {}
    void Detach() { pipeline_ = NULL; }


    
    virtual void NotifyQueuedTrackChanges(MediaStreamGraph* graph, TrackID tid,
                                          TrackRate rate,
                                          TrackTicks offset,
                                          PRUint32 events,
                                          const MediaSegment& queued_media);
    virtual void NotifyPull(MediaStreamGraph* aGraph, StreamTime aDesiredTime) {}

   private:
    MediaPipelineTransmit *pipeline_;  
  };
  friend class PipelineListener;

 private:
  virtual void ProcessAudioChunk(AudioSessionConduit *conduit,
                                 TrackRate rate, AudioChunk& chunk);
#ifdef MOZILLA_INTERNAL_API
  virtual void ProcessVideoChunk(VideoSessionConduit *conduit,
                                 TrackRate rate, VideoChunk& chunk);
#endif
  RefPtr<PipelineListener> listener_;
};




class MediaPipelineReceive : public MediaPipeline {
 public:
  MediaPipelineReceive(nsCOMPtr<nsIEventTarget> main_thread,
                       nsCOMPtr<nsIEventTarget> sts_thread,
                       nsDOMMediaStream* stream,
                       RefPtr<MediaSessionConduit> conduit,
                       RefPtr<TransportFlow> rtp_transport,
                       RefPtr<TransportFlow> rtcp_transport) :
      MediaPipeline(RECEIVE, main_thread, sts_thread,
                    stream, conduit, rtp_transport,
                    rtcp_transport),
      segments_added_(0) {
  }

  int segments_added() const { return segments_added_; }

 protected:
  int segments_added_;

 private:
};




class MediaPipelineReceiveAudio : public MediaPipelineReceive {
 public:
  MediaPipelineReceiveAudio(nsCOMPtr<nsIEventTarget> main_thread,
                            nsCOMPtr<nsIEventTarget> sts_thread,
                            nsDOMMediaStream* stream,
                            RefPtr<AudioSessionConduit> conduit,
                            RefPtr<TransportFlow> rtp_transport,
                            RefPtr<TransportFlow> rtcp_transport) :
      MediaPipelineReceive(main_thread, sts_thread,
                           stream, conduit, rtp_transport,
                           rtcp_transport),
      listener_(new PipelineListener(this)) {
    Init();
  }

  ~MediaPipelineReceiveAudio() {
    if (stream_ && listener_) {
      stream_->GetStream()->RemoveListener(listener_);
      listener_->Detach();
    }
  }

  virtual void DetachMediaStream() {
    
    stream_->GetStream()->RemoveListener(listener_);
    stream_ = NULL;
    listener_->Detach();
  }

 private:
  
  class PipelineListener : public MediaStreamListener {
   public:
    PipelineListener(MediaPipelineReceiveAudio *pipeline)
        : pipeline_(pipeline),
        played_(0) {}
    void Detach() { pipeline_ = NULL; }

    
    virtual void NotifyQueuedTrackChanges(MediaStreamGraph* graph, TrackID tid,
                                          TrackRate rate,
                                          TrackTicks offset,
                                          PRUint32 events,
                                          const MediaSegment& queued_media) {}
    virtual void NotifyPull(MediaStreamGraph* aGraph, StreamTime aDesiredTime);

   private:
    MediaPipelineReceiveAudio *pipeline_;  
    StreamTime played_;
  };
  friend class PipelineListener;

  nsresult Init();

  RefPtr<PipelineListener> listener_;
};




class MediaPipelineReceiveVideo : public MediaPipelineReceive {
 public:
  MediaPipelineReceiveVideo(nsCOMPtr<nsIEventTarget> main_thread,
                            nsCOMPtr<nsIEventTarget> sts_thread,
                            nsDOMMediaStream* stream,
                            RefPtr<VideoSessionConduit> conduit,
                            RefPtr<TransportFlow> rtp_transport,
                            RefPtr<TransportFlow> rtcp_transport) :
      MediaPipelineReceive(main_thread, sts_thread,
                           stream, conduit, rtp_transport,
                           rtcp_transport),
      renderer_(new PipelineRenderer(this)) {
    Init();
  }

  ~MediaPipelineReceiveVideo() {
  }

 private:
  class PipelineRenderer : public VideoRenderer {
   public:
    PipelineRenderer(MediaPipelineReceiveVideo *);
    void Detach() { pipeline_ = NULL; }

    
    virtual void FrameSizeChange(unsigned int width,
                                 unsigned int height,
                                 unsigned int number_of_streams) {
      width_ = width;
      height_ = height;
    }

    virtual void RenderVideoFrame(const unsigned char* buffer,
                                  unsigned int buffer_size,
                                  uint32_t time_stamp,
                                  int64_t render_time);


   private:
    MediaPipelineReceiveVideo *pipeline_;  
#ifdef MOZILLA_INTERNAL_API
    nsRefPtr<layers::ImageContainer> image_container_;
#endif
    int width_;
    int height_;
  };
  friend class PipelineRenderer;

  nsresult Init();
  RefPtr<PipelineRenderer> renderer_;
};


}  
#endif
