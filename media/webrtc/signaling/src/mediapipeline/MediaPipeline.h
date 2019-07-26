





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
#include "databuffer.h"
#include "runnable_utils.h"
#include "transportflow.h"

#ifdef MOZILLA_INTERNAL_API
#include "VideoSegment.h"
#endif

namespace mozilla {































class MediaPipeline : public sigslot::has_slots<> {
 public:
  enum Direction { TRANSMIT, RECEIVE };
  enum State { MP_CONNECTING, MP_OPEN, MP_CLOSED };
  MediaPipeline(const std::string& pc,
                Direction direction,
                nsCOMPtr<nsIEventTarget> main_thread,
                nsCOMPtr<nsIEventTarget> sts_thread,
                MediaStream *stream,
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
        muxed_((rtcp_transport_ == NULL) || (rtp_transport_ == rtcp_transport_)),
        pc_(pc),
        description_() {
  }

  virtual ~MediaPipeline() {
    MOZ_ASSERT(!stream_);  
  }

  void Shutdown() {
    ASSERT_ON_THREAD(main_thread_);
    
    
    
    
    DetachTransport();
    if (stream_) {
      DetachMediaStream();
    }
  }

  virtual nsresult Init();

  virtual Direction direction() const { return direction_; }

  int rtp_packets_sent() const { return rtp_packets_sent_; }
  int rtcp_packets_sent() const { return rtp_packets_sent_; }
  int rtp_packets_received() const { return rtp_packets_received_; }
  int rtcp_packets_received() const { return rtp_packets_received_; }

  
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MediaPipeline)

 protected:
  virtual void DetachMediaStream() {}

  
  class PipelineTransport : public TransportInterface {
   public:
    
    PipelineTransport(MediaPipeline *pipeline)
        : pipeline_(pipeline),
	  sts_thread_(pipeline->sts_thread_) {}

    void Detach() { pipeline_ = NULL; }
    MediaPipeline *pipeline() const { return pipeline_; }

    virtual nsresult SendRtpPacket(const void* data, int len);
    virtual nsresult SendRtcpPacket(const void* data, int len);

   private:
    virtual nsresult SendRtpPacket_s(nsAutoPtr<DataBuffer> data);
    virtual nsresult SendRtcpPacket_s(nsAutoPtr<DataBuffer> data);

    MediaPipeline *pipeline_;  
    nsCOMPtr<nsIEventTarget> sts_thread_;
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
  RefPtr<MediaStream> stream_;  
  		      		
  		      		
  RefPtr<MediaSessionConduit> conduit_;  
  			      		 

  
  RefPtr<TransportFlow> rtp_transport_;
  State rtp_state_;
  RefPtr<TransportFlow> rtcp_transport_;
  State rtcp_state_;

  
  
  nsCOMPtr<nsIEventTarget> main_thread_;
  nsCOMPtr<nsIEventTarget> sts_thread_;

  
  
  RefPtr<PipelineTransport> transport_;

  
  RefPtr<SrtpFlow> rtp_send_srtp_;
  RefPtr<SrtpFlow> rtcp_send_srtp_;
  RefPtr<SrtpFlow> rtp_recv_srtp_;
  RefPtr<SrtpFlow> rtcp_recv_srtp_;

  
  
  
  int rtp_packets_sent_;
  int rtcp_packets_sent_;
  int rtp_packets_received_;
  int rtcp_packets_received_;

  
  bool muxed_;
  std::string pc_;
  std::string description_;

 private:
  nsresult Init_s();
  void DetachTransport();
  void DetachTransport_s();

  nsresult TransportReadyInt(TransportFlow *flow);

  bool IsRtp(const unsigned char *data, size_t len);
};




class MediaPipelineTransmit : public MediaPipeline {
 public:
  MediaPipelineTransmit(const std::string& pc,
                        nsCOMPtr<nsIEventTarget> main_thread,
                        nsCOMPtr<nsIEventTarget> sts_thread,
                        MediaStream *stream,
                        RefPtr<MediaSessionConduit> conduit,
                        RefPtr<TransportFlow> rtp_transport,
                        RefPtr<TransportFlow> rtcp_transport) :
      MediaPipeline(pc, TRANSMIT, main_thread, sts_thread,
                    stream, conduit, rtp_transport,
                    rtcp_transport),
      listener_(new PipelineListener(conduit)) {}

  
  virtual nsresult Init();

  
  virtual void DetachMediaStream() {
    ASSERT_ON_THREAD(main_thread_);
    stream_->RemoveListener(listener_);
    
    
    listener_ = nullptr;
    stream_ = nullptr;
  }

  
  virtual nsresult TransportReady(TransportFlow *flow);

  
  class PipelineListener : public MediaStreamListener {
   public:
    PipelineListener(const RefPtr<MediaSessionConduit>& conduit)
      : conduit_(conduit), active_(false), samples_10ms_buffer_(nullptr),  
        buffer_current_(0), samplenum_10ms_(0){}

    
    
    
    void SetActive(bool active) { active_ = active; }

    
    virtual void NotifyQueuedTrackChanges(MediaStreamGraph* graph, TrackID tid,
                                          TrackRate rate,
                                          TrackTicks offset,
                                          uint32_t events,
                                          const MediaSegment& queued_media);
    virtual void NotifyPull(MediaStreamGraph* aGraph, StreamTime aDesiredTime) {}

   private:
    virtual void ProcessAudioChunk(AudioSessionConduit *conduit,
				   TrackRate rate, AudioChunk& chunk);
#ifdef MOZILLA_INTERNAL_API
    virtual void ProcessVideoChunk(VideoSessionConduit *conduit,
				   TrackRate rate, VideoChunk& chunk);
#endif
    RefPtr<MediaSessionConduit> conduit_;
    volatile bool active_;

    
    
    
    nsAutoArrayPtr<int16_t> samples_10ms_buffer_;
    
    int64_t buffer_current_;
    
    int64_t samplenum_10ms_;
  };

 private:
RefPtr<PipelineListener> listener_;

};




class MediaPipelineReceive : public MediaPipeline {
 public:
  MediaPipelineReceive(const std::string& pc,
                       nsCOMPtr<nsIEventTarget> main_thread,
                       nsCOMPtr<nsIEventTarget> sts_thread,
                       MediaStream *stream,
                       RefPtr<MediaSessionConduit> conduit,
                       RefPtr<TransportFlow> rtp_transport,
                       RefPtr<TransportFlow> rtcp_transport) :
      MediaPipeline(pc, RECEIVE, main_thread, sts_thread,
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
  MediaPipelineReceiveAudio(const std::string& pc,
                            nsCOMPtr<nsIEventTarget> main_thread,
                            nsCOMPtr<nsIEventTarget> sts_thread,
                            MediaStream *stream,
                            RefPtr<AudioSessionConduit> conduit,
                            RefPtr<TransportFlow> rtp_transport,
                            RefPtr<TransportFlow> rtcp_transport) :
      MediaPipelineReceive(pc, main_thread, sts_thread,
                           stream, conduit, rtp_transport,
                           rtcp_transport),
      listener_(new PipelineListener(stream->AsSourceStream(),
                                     conduit)) {
  }

  virtual void DetachMediaStream() {
    ASSERT_ON_THREAD(main_thread_);
    stream_->RemoveListener(listener_);
    
    
    listener_ = nullptr;
    stream_ = nullptr;
  }

  virtual nsresult Init();

 private:
  
  class PipelineListener : public MediaStreamListener {
   public:
    PipelineListener(SourceMediaStream * source,
                     const RefPtr<MediaSessionConduit>& conduit)
        : source_(source),
          conduit_(conduit),
          played_(0) {}

    
    virtual void NotifyQueuedTrackChanges(MediaStreamGraph* graph, TrackID tid,
                                          TrackRate rate,
                                          TrackTicks offset,
                                          uint32_t events,
                                          const MediaSegment& queued_media) {}
    virtual void NotifyPull(MediaStreamGraph* aGraph, StreamTime aDesiredTime);

   private:
    SourceMediaStream *source_;
    RefPtr<MediaSessionConduit> conduit_;
    uint64_t played_;  
  };

  RefPtr<PipelineListener> listener_;
};




class MediaPipelineReceiveVideo : public MediaPipelineReceive {
 public:
  MediaPipelineReceiveVideo(const std::string& pc,
                            nsCOMPtr<nsIEventTarget> main_thread,
                            nsCOMPtr<nsIEventTarget> sts_thread,
                            MediaStream *stream,
                            RefPtr<VideoSessionConduit> conduit,
                            RefPtr<TransportFlow> rtp_transport,
                            RefPtr<TransportFlow> rtcp_transport) :
      MediaPipelineReceive(pc, main_thread, sts_thread,
                           stream, conduit, rtp_transport,
                           rtcp_transport),
      renderer_(new PipelineRenderer(this)) {
  }

  
  virtual void DetachMediaStream() {
    ASSERT_ON_THREAD(main_thread_);
    conduit_ = nullptr;  
                         
    stream_ = nullptr;
  }

  virtual nsresult Init();

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

  RefPtr<PipelineRenderer> renderer_;
};


}  
#endif
