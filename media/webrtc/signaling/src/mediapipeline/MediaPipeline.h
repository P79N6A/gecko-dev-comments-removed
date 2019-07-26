





#ifndef mediapipeline_h__
#define mediapipeline_h__

#include "sigslot.h"

#ifdef USE_FAKE_MEDIA_STREAMS
#include "FakeMediaStreams.h"
#else
#include "DOMMediaStream.h"
#include "MediaStreamGraph.h"
#include "VideoUtils.h"
#endif
#include "MediaConduitInterface.h"
#include "AudioSegment.h"
#include "mozilla/ReentrantMonitor.h"
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
                TrackID track_id,
                RefPtr<MediaSessionConduit> conduit,
                RefPtr<TransportFlow> rtp_transport,
                RefPtr<TransportFlow> rtcp_transport)
      : direction_(direction),
        stream_(stream),
        track_id_(track_id),
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



  
  
  void ShutdownTransport_s();

  
  void ShutdownMedia_m() {
    ASSERT_ON_THREAD(main_thread_);

    MOZ_ASSERT(!rtp_transport_);
    MOZ_ASSERT(!rtcp_transport_);

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

  MediaSessionConduit *Conduit() { return conduit_; }

  
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

  virtual nsresult TransportFailed_s(TransportFlow *flow);  
  virtual nsresult TransportReady_s(TransportFlow *flow);   

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
  		      		
  		      		
  TrackID track_id_;            
                                
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

  bool IsRtp(const unsigned char *data, size_t len);
};

class GenericReceiveListener : public MediaStreamListener
{
 public:
  GenericReceiveListener(SourceMediaStream *source, TrackID track_id,
                         TrackRate track_rate)
    : source_(source),
      track_id_(track_id),
      track_rate_(track_rate),
      played_ticks_(0) {}

  virtual ~GenericReceiveListener() {}

  void AddSelf(MediaSegment* segment);

  void SetPlayedTicks(TrackTicks time) {
    played_ticks_ = time;
  }

  void EndTrack() {
    source_->EndTrack(track_id_);
  }

 protected:
  SourceMediaStream *source_;
  TrackID track_id_;
  TrackRate track_rate_;
  TrackTicks played_ticks_;
};

class TrackAddedCallback {
 public:
  virtual void TrackAdded(TrackTicks current_ticks) = 0;

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(TrackAddedCallback);

 protected:
  virtual ~TrackAddedCallback() {}
};

class GenericReceiveListener;

class GenericReceiveCallback : public TrackAddedCallback
{
 public:
  GenericReceiveCallback(GenericReceiveListener* listener)
    : listener_(listener) {}

  void TrackAdded(TrackTicks time) {
    listener_->SetPlayedTicks(time);
  }

 private:
  RefPtr<GenericReceiveListener> listener_;
};

class ConduitDeleteEvent: public nsRunnable
{
public:
  ConduitDeleteEvent(TemporaryRef<MediaSessionConduit> aConduit) :
    mConduit(aConduit) {}

  
  NS_IMETHOD Run() { return NS_OK; }
private:
  RefPtr<MediaSessionConduit> mConduit;
};



class MediaPipelineTransmit : public MediaPipeline {
 public:
  MediaPipelineTransmit(const std::string& pc,
                        nsCOMPtr<nsIEventTarget> main_thread,
                        nsCOMPtr<nsIEventTarget> sts_thread,
                        MediaStream *stream,
                        TrackID track_id,
                        RefPtr<MediaSessionConduit> conduit,
                        RefPtr<TransportFlow> rtp_transport,
                        RefPtr<TransportFlow> rtcp_transport) :
      MediaPipeline(pc, TRANSMIT, main_thread, sts_thread,
                    stream, track_id, conduit, rtp_transport,
                    rtcp_transport),
      listener_(new PipelineListener(conduit)) {}

  
  virtual nsresult Init();

  
  virtual void DetachMediaStream() {
    ASSERT_ON_THREAD(main_thread_);
    stream_->RemoveListener(listener_);
    
    
    listener_ = nullptr;
    stream_ = nullptr;
  }

  
  virtual nsresult TransportReady_s(TransportFlow *flow);

  
  class PipelineListener : public MediaStreamListener {
   public:
    PipelineListener(const RefPtr<MediaSessionConduit>& conduit)
      : conduit_(conduit), active_(false), samples_10ms_buffer_(nullptr),
        buffer_current_(0), samplenum_10ms_(0){}

    ~PipelineListener()
    {
      
      NS_DispatchToMainThread(new ConduitDeleteEvent(conduit_.forget()), NS_DISPATCH_NORMAL);
    }


    
    
    
    void SetActive(bool active) { active_ = active; }

    
    virtual void NotifyQueuedTrackChanges(MediaStreamGraph* graph, TrackID tid,
                                          TrackRate rate,
                                          TrackTicks offset,
                                          uint32_t events,
                                          const MediaSegment& queued_media) MOZ_OVERRIDE;
    virtual void NotifyPull(MediaStreamGraph* aGraph, StreamTime aDesiredTime) MOZ_OVERRIDE {}

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
                       TrackID track_id,
                       RefPtr<MediaSessionConduit> conduit,
                       RefPtr<TransportFlow> rtp_transport,
                       RefPtr<TransportFlow> rtcp_transport) :
      MediaPipeline(pc, RECEIVE, main_thread, sts_thread,
                    stream, track_id, conduit, rtp_transport,
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
                            TrackID track_id,
                            RefPtr<AudioSessionConduit> conduit,
                            RefPtr<TransportFlow> rtp_transport,
                            RefPtr<TransportFlow> rtcp_transport) :
      MediaPipelineReceive(pc, main_thread, sts_thread,
                           stream, track_id, conduit, rtp_transport,
                           rtcp_transport),
      listener_(new PipelineListener(stream->AsSourceStream(),
                                     track_id, conduit)) {
  }

  virtual void DetachMediaStream() {
    ASSERT_ON_THREAD(main_thread_);
    listener_->EndTrack();
    stream_->RemoveListener(listener_);
    
    
    listener_ = nullptr;
    stream_ = nullptr;
  }

  virtual nsresult Init();

 private:
  
  class PipelineListener : public GenericReceiveListener {
   public:
    PipelineListener(SourceMediaStream * source, TrackID track_id,
                     const RefPtr<MediaSessionConduit>& conduit);

    ~PipelineListener()
    {
      
      NS_DispatchToMainThread(new ConduitDeleteEvent(conduit_.forget()), NS_DISPATCH_NORMAL);
    }

    
    virtual void NotifyQueuedTrackChanges(MediaStreamGraph* graph, TrackID tid,
                                          TrackRate rate,
                                          TrackTicks offset,
                                          uint32_t events,
                                          const MediaSegment& queued_media) MOZ_OVERRIDE {}
    virtual void NotifyPull(MediaStreamGraph* graph, StreamTime desired_time) MOZ_OVERRIDE;

   private:
    RefPtr<MediaSessionConduit> conduit_;
  };

  RefPtr<PipelineListener> listener_;
};




class MediaPipelineReceiveVideo : public MediaPipelineReceive {
 public:
  MediaPipelineReceiveVideo(const std::string& pc,
                            nsCOMPtr<nsIEventTarget> main_thread,
                            nsCOMPtr<nsIEventTarget> sts_thread,
                            MediaStream *stream,
                            TrackID track_id,
                            RefPtr<VideoSessionConduit> conduit,
                            RefPtr<TransportFlow> rtp_transport,
                            RefPtr<TransportFlow> rtcp_transport) :
      MediaPipelineReceive(pc, main_thread, sts_thread,
                           stream, track_id, conduit, rtp_transport,
                           rtcp_transport),
      renderer_(new PipelineRenderer(this)),
      listener_(new PipelineListener(stream->AsSourceStream(), track_id)) {
  }

  
  virtual void DetachMediaStream() {
    ASSERT_ON_THREAD(main_thread_);

    listener_->EndTrack();

    conduit_ = nullptr;  
                         

    stream_->RemoveListener(listener_);
    
    
    listener_ = nullptr;

    stream_ = nullptr;
  }

  virtual nsresult Init();

 private:
  class PipelineRenderer : public VideoRenderer {
   public:
    PipelineRenderer(MediaPipelineReceiveVideo *pipeline) :
      pipeline_(pipeline) {}

    void Detach() { pipeline_ = NULL; }

    
    virtual void FrameSizeChange(unsigned int width,
                                 unsigned int height,
                                 unsigned int number_of_streams) {
      pipeline_->listener_->FrameSizeChange(width, height, number_of_streams);
    }

    virtual void RenderVideoFrame(const unsigned char* buffer,
                                  unsigned int buffer_size,
                                  uint32_t time_stamp,
                                  int64_t render_time) {
      pipeline_->listener_->RenderVideoFrame(buffer, buffer_size, time_stamp,
                                            render_time);
    }

   private:
    MediaPipelineReceiveVideo *pipeline_;  
  };

  
  class PipelineListener : public GenericReceiveListener {
   public:
    PipelineListener(SourceMediaStream * source, TrackID track_id);

    
    virtual void NotifyQueuedTrackChanges(MediaStreamGraph* graph, TrackID tid,
                                          TrackRate rate,
                                          TrackTicks offset,
                                          uint32_t events,
                                          const MediaSegment& queued_media) MOZ_OVERRIDE {}
    virtual void NotifyPull(MediaStreamGraph* graph, StreamTime desired_time) MOZ_OVERRIDE;

    
    void FrameSizeChange(unsigned int width,
                         unsigned int height,
                         unsigned int number_of_streams) {
      ReentrantMonitorAutoEnter enter(monitor_);

      width_ = width;
      height_ = height;
    }

    void RenderVideoFrame(const unsigned char* buffer,
                          unsigned int buffer_size,
                          uint32_t time_stamp,
                          int64_t render_time);


   private:
    int width_;
    int height_;
#ifdef MOZILLA_INTERNAL_API
    nsRefPtr<layers::ImageContainer> image_container_;
    nsRefPtr<layers::Image> image_;
#endif
    mozilla::ReentrantMonitor monitor_; 
                                        
                                        
                                        
  };

  friend class PipelineRenderer;

  RefPtr<PipelineRenderer> renderer_;
  RefPtr<PipelineListener> listener_;
};


}  
#endif
