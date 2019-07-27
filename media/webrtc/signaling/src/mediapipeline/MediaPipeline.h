






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
#include "MediaPipelineFilter.h"
#include "AudioSegment.h"
#include "mozilla/ReentrantMonitor.h"
#include "mozilla/Atomics.h"
#include "SrtpFlow.h"
#include "databuffer.h"
#include "runnable_utils.h"
#include "transportflow.h"

#ifdef MOZILLA_INTERNAL_API
#include "VideoSegment.h"
#endif

#include "webrtc/modules/rtp_rtcp/interface/rtp_header_parser.h"

namespace mozilla {

class PeerIdentity;
































class MediaPipeline : public sigslot::has_slots<> {
 public:
  enum Direction { TRANSMIT, RECEIVE };
  enum State { MP_CONNECTING, MP_OPEN, MP_CLOSED };
  MediaPipeline(const std::string& pc,
                Direction direction,
                nsCOMPtr<nsIEventTarget> main_thread,
                nsCOMPtr<nsIEventTarget> sts_thread,
                MediaStream *stream,
                const std::string& track_id,
                int level,
                RefPtr<MediaSessionConduit> conduit,
                RefPtr<TransportFlow> rtp_transport,
                RefPtr<TransportFlow> rtcp_transport,
                nsAutoPtr<MediaPipelineFilter> filter)
      : direction_(direction),
        stream_(stream),
        track_id_(track_id),
        level_(level),
        conduit_(conduit),
        rtp_(rtp_transport, rtcp_transport ? RTP : MUX),
        rtcp_(rtcp_transport ? rtcp_transport : rtp_transport,
              rtcp_transport ? RTCP : MUX),
        main_thread_(main_thread),
        sts_thread_(sts_thread),
        rtp_packets_sent_(0),
        rtcp_packets_sent_(0),
        rtp_packets_received_(0),
        rtcp_packets_received_(0),
        rtp_bytes_sent_(0),
        rtp_bytes_received_(0),
        pc_(pc),
        description_(),
        filter_(filter),
        rtp_parser_(webrtc::RtpHeaderParser::Create()) {
      
      
      
      MOZ_ASSERT(rtp_transport != rtcp_transport);

      
      transport_ = new PipelineTransport(this);
    }

  
  void ShutdownTransport_s();

  
  void ShutdownMedia_m() {
    ASSERT_ON_THREAD(main_thread_);

    if (direction_ == RECEIVE) {
      conduit_->StopReceiving();
    } else {
      conduit_->StopTransmitting();
    }

    if (stream_) {
      DetachMediaStream();
    }
  }

  virtual nsresult Init();

  void UpdateTransport_m(int level,
                         RefPtr<TransportFlow> rtp_transport,
                         RefPtr<TransportFlow> rtcp_transport,
                         nsAutoPtr<MediaPipelineFilter> filter);

  void UpdateTransport_s(int level,
                         RefPtr<TransportFlow> rtp_transport,
                         RefPtr<TransportFlow> rtcp_transport,
                         nsAutoPtr<MediaPipelineFilter> filter);

  virtual Direction direction() const { return direction_; }
  virtual const std::string& trackid() const { return track_id_; }
  virtual int level() const { return level_; }
  virtual bool IsVideo() const = 0;

  bool IsDoingRtcpMux() const {
    return (rtp_.type_ == MUX);
  }

  int32_t rtp_packets_sent() const { return rtp_packets_sent_; }
  int64_t rtp_bytes_sent() const { return rtp_bytes_sent_; }
  int32_t rtcp_packets_sent() const { return rtcp_packets_sent_; }
  int32_t rtp_packets_received() const { return rtp_packets_received_; }
  int64_t rtp_bytes_received() const { return rtp_bytes_received_; }
  int32_t rtcp_packets_received() const { return rtcp_packets_received_; }

  MediaSessionConduit *Conduit() const { return conduit_; }

  
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MediaPipeline)

  typedef enum {
    RTP,
    RTCP,
    MUX,
    MAX_RTP_TYPE
  } RtpType;

 protected:
  virtual ~MediaPipeline();
  virtual void DetachMediaStream() {}
  nsresult AttachTransport_s();
  void DetachTransport_s();

  
  class PipelineTransport : public TransportInterface {
   public:
    
    explicit PipelineTransport(MediaPipeline *pipeline)
        : pipeline_(pipeline),
          sts_thread_(pipeline->sts_thread_) {}

    void Detach() { pipeline_ = nullptr; }
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

  class TransportInfo {
    public:
      TransportInfo(RefPtr<TransportFlow> flow, RtpType type) :
        transport_(flow),
        state_(MP_CONNECTING),
        type_(type) {
        MOZ_ASSERT(flow);
      }

      void Detach()
      {
        transport_ = nullptr;
        send_srtp_ = nullptr;
        recv_srtp_ = nullptr;
      }

      RefPtr<TransportFlow> transport_;
      State state_;
      RefPtr<SrtpFlow> send_srtp_;
      RefPtr<SrtpFlow> recv_srtp_;
      RtpType type_;
  };

  
  virtual nsresult TransportFailed_s(TransportInfo &info);
  
  virtual nsresult TransportReady_s(TransportInfo &info);
  void UpdateRtcpMuxState(TransportInfo &info);

  
  void DisconnectTransport_s(TransportInfo &info);
  nsresult ConnectTransport_s(TransportInfo &info);

  TransportInfo* GetTransportInfo_s(TransportFlow *flow);

  void increment_rtp_packets_sent(int bytes);
  void increment_rtcp_packets_sent();
  void increment_rtp_packets_received(int bytes);
  void increment_rtcp_packets_received();

  virtual nsresult SendPacket(TransportFlow *flow, const void *data, int len);

  
  void StateChange(TransportFlow *flow, TransportLayer::State);
  void RtpPacketReceived(TransportLayer *layer, const unsigned char *data,
                         size_t len);
  void RtcpPacketReceived(TransportLayer *layer, const unsigned char *data,
                          size_t len);
  void PacketReceived(TransportLayer *layer, const unsigned char *data,
                      size_t len);

  Direction direction_;
  RefPtr<MediaStream> stream_;  
                                
                                
                                
  std::string track_id_;        
                                
                                
  
  
  
  Atomic<int> level_;
  RefPtr<MediaSessionConduit> conduit_;  
                                         

  
  TransportInfo rtp_;
  TransportInfo rtcp_;

  
  
  nsCOMPtr<nsIEventTarget> main_thread_;
  nsCOMPtr<nsIEventTarget> sts_thread_;

  
  
  RefPtr<PipelineTransport> transport_;

  
  
  int32_t rtp_packets_sent_;
  int32_t rtcp_packets_sent_;
  int32_t rtp_packets_received_;
  int32_t rtcp_packets_received_;
  int64_t rtp_bytes_sent_;
  int64_t rtp_bytes_received_;

  
  std::string pc_;
  std::string description_;

  
  nsAutoPtr<MediaPipelineFilter> filter_;
  nsAutoPtr<webrtc::RtpHeaderParser> rtp_parser_;

 private:
  nsresult Init_s();

  bool IsRtp(const unsigned char *data, size_t len);
};

class GenericReceiveListener : public MediaStreamListener
{
 public:
  GenericReceiveListener(SourceMediaStream *source, TrackID track_id,
                         TrackRate track_rate, bool queue_track)
    : source_(source),
      track_id_(track_id),
      track_rate_(track_rate),
      played_ticks_(0),
      queue_track_(queue_track) {}

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
  bool queue_track_;
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
  explicit GenericReceiveCallback(GenericReceiveListener* listener)
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
  explicit ConduitDeleteEvent(TemporaryRef<MediaSessionConduit> aConduit) :
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
                        DOMMediaStream *domstream,
                        const std::string& track_id,
                        int level,
                        bool is_video,
                        RefPtr<MediaSessionConduit> conduit,
                        RefPtr<TransportFlow> rtp_transport,
                        RefPtr<TransportFlow> rtcp_transport,
                        nsAutoPtr<MediaPipelineFilter> filter) :
      MediaPipeline(pc, TRANSMIT, main_thread, sts_thread,
                    domstream->GetStream(), track_id, level,
                    conduit, rtp_transport, rtcp_transport, filter),
      listener_(new PipelineListener(conduit)),
      domstream_(domstream),
      is_video_(is_video)
  {}

  
  virtual nsresult Init() override;

  virtual void AttachToTrack(const std::string& track_id);

  
  
  
  virtual TrackID const trackid_locked() { return listener_->trackid(); }
  
  virtual bool IsVideo() const override { return is_video_; }

#ifdef MOZILLA_INTERNAL_API
  
  
  virtual void UpdateSinkIdentity_m(nsIPrincipal* principal,
                                    const PeerIdentity* sinkIdentity);
#endif

  
  virtual void DetachMediaStream() override {
    ASSERT_ON_THREAD(main_thread_);
    domstream_->RemoveDirectListener(listener_);
    domstream_ = nullptr;
    stream_->RemoveListener(listener_);
    
    stream_ = nullptr;
  }

  
  virtual nsresult TransportReady_s(TransportInfo &info) override;

  
  
  
  
  virtual nsresult ReplaceTrack(DOMMediaStream *domstream,
                                const std::string& track_id);


  
  class PipelineListener : public MediaStreamDirectListener {
   friend class MediaPipelineTransmit;
   public:
    explicit PipelineListener(const RefPtr<MediaSessionConduit>& conduit)
      : conduit_(conduit),
        track_id_(TRACK_INVALID),
        mMutex("MediaPipelineTransmit::PipelineListener"),
        track_id_external_(TRACK_INVALID),
        active_(false),
        enabled_(false),
        direct_connect_(false),
        samples_10ms_buffer_(nullptr),
        buffer_current_(0),
        samplenum_10ms_(0)
#ifdef MOZILLA_INTERNAL_API
        , last_img_(-1)
#endif 
    {
    }

    ~PipelineListener()
    {
      
      nsresult rv = NS_DispatchToMainThread(new
        ConduitDeleteEvent(conduit_.forget()));
      MOZ_ASSERT(!NS_FAILED(rv),"Could not dispatch conduit shutdown to main");
      if (NS_FAILED(rv)) {
        MOZ_CRASH();
      }
    }

    void SetActive(bool active) { active_ = active; }
    void SetEnabled(bool enabled) { enabled_ = enabled; }
    TrackID trackid() {
      MutexAutoLock lock(mMutex);
      return track_id_external_;
    }

    
    virtual void NotifyQueuedTrackChanges(MediaStreamGraph* graph, TrackID tid,
                                          StreamTime offset,
                                          uint32_t events,
                                          const MediaSegment& queued_media) override;
    virtual void NotifyPull(MediaStreamGraph* aGraph, StreamTime aDesiredTime) override {}

    
    virtual void NotifyRealtimeData(MediaStreamGraph* graph, TrackID tid,
                                    StreamTime offset,
                                    uint32_t events,
                                    const MediaSegment& media) override;

   private:
    void NewData(MediaStreamGraph* graph, TrackID tid,
                 StreamTime offset,
                 uint32_t events,
                 const MediaSegment& media);

    virtual void ProcessAudioChunk(AudioSessionConduit *conduit,
                                   TrackRate rate, AudioChunk& chunk);
#ifdef MOZILLA_INTERNAL_API
    virtual void ProcessVideoChunk(VideoSessionConduit *conduit,
                                   VideoChunk& chunk);
#endif
    RefPtr<MediaSessionConduit> conduit_;

    
    TrackID track_id_; 
    Mutex mMutex;
    
    
    TrackID track_id_external_; 

    
    mozilla::Atomic<bool> active_;
    
    
    mozilla::Atomic<bool> enabled_;

    bool direct_connect_;


    
    
    
    nsAutoArrayPtr<int16_t> samples_10ms_buffer_;
    
    int64_t buffer_current_;
    
    int64_t samplenum_10ms_;

#ifdef MOZILLA_INTERNAL_API
    int32_t last_img_; 
#endif 
  };

 private:
  RefPtr<PipelineListener> listener_;
  DOMMediaStream *domstream_;
  bool is_video_;
};




class MediaPipelineReceive : public MediaPipeline {
 public:
  
  MediaPipelineReceive(const std::string& pc,
                       nsCOMPtr<nsIEventTarget> main_thread,
                       nsCOMPtr<nsIEventTarget> sts_thread,
                       MediaStream *stream,
                       const std::string& track_id,
                       int level,
                       RefPtr<MediaSessionConduit> conduit,
                       RefPtr<TransportFlow> rtp_transport,
                       RefPtr<TransportFlow> rtcp_transport,
                       nsAutoPtr<MediaPipelineFilter> filter) :
      MediaPipeline(pc, RECEIVE, main_thread, sts_thread,
                    stream, track_id, level, conduit, rtp_transport,
                    rtcp_transport, filter),
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
                            
                            
                            const std::string& media_stream_track_id,
                            
                            
                            
                            TrackID numeric_track_id,
                            int level,
                            RefPtr<AudioSessionConduit> conduit,
                            RefPtr<TransportFlow> rtp_transport,
                            RefPtr<TransportFlow> rtcp_transport,
                            nsAutoPtr<MediaPipelineFilter> filter,
                            bool queue_track) :
      MediaPipelineReceive(pc, main_thread, sts_thread,
                           stream, media_stream_track_id, level, conduit,
                           rtp_transport, rtcp_transport, filter),
      listener_(new PipelineListener(stream->AsSourceStream(),
                                     numeric_track_id, conduit, queue_track)) {
  }

  virtual void DetachMediaStream() override {
    ASSERT_ON_THREAD(main_thread_);
    listener_->EndTrack();
    stream_->RemoveListener(listener_);
    stream_ = nullptr;
  }

  virtual nsresult Init() override;
  virtual bool IsVideo() const override { return false; }

 private:
  
  class PipelineListener : public GenericReceiveListener {
   public:
    PipelineListener(SourceMediaStream * source, TrackID track_id,
                     const RefPtr<MediaSessionConduit>& conduit,
                     bool queue_track);

    ~PipelineListener()
    {
      
      nsresult rv = NS_DispatchToMainThread(new
        ConduitDeleteEvent(conduit_.forget()));
      MOZ_ASSERT(!NS_FAILED(rv),"Could not dispatch conduit shutdown to main");
      if (NS_FAILED(rv)) {
        MOZ_CRASH();
      }
    }

    
    virtual void NotifyQueuedTrackChanges(MediaStreamGraph* graph, TrackID tid,
                                          StreamTime offset,
                                          uint32_t events,
                                          const MediaSegment& queued_media) override {}
    virtual void NotifyPull(MediaStreamGraph* graph, StreamTime desired_time) override;

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
                            
                            
                            const std::string& media_stream_track_id,
                            
                            
                            
                            TrackID numeric_track_id,
                            int level,
                            RefPtr<VideoSessionConduit> conduit,
                            RefPtr<TransportFlow> rtp_transport,
                            RefPtr<TransportFlow> rtcp_transport,
                            nsAutoPtr<MediaPipelineFilter> filter,
                            bool queue_track) :
      MediaPipelineReceive(pc, main_thread, sts_thread,
                           stream, media_stream_track_id, level, conduit,
                           rtp_transport, rtcp_transport, filter),
      renderer_(new PipelineRenderer(this)),
      listener_(new PipelineListener(stream->AsSourceStream(),
                                     numeric_track_id, queue_track)) {
  }

  
  virtual void DetachMediaStream() override {
    ASSERT_ON_THREAD(main_thread_);

    listener_->EndTrack();
    
    
    
    
    static_cast<VideoSessionConduit*>(conduit_.get())->DetachRenderer();
    stream_->RemoveListener(listener_);
    stream_ = nullptr;
  }

  virtual nsresult Init() override;
  virtual bool IsVideo() const override { return true; }

 private:
  class PipelineRenderer : public VideoRenderer {
   public:
    explicit PipelineRenderer(MediaPipelineReceiveVideo *pipeline) :
      pipeline_(pipeline) {}

    void Detach() { pipeline_ = nullptr; }

    
    virtual void FrameSizeChange(unsigned int width,
                                 unsigned int height,
                                 unsigned int number_of_streams) {
      pipeline_->listener_->FrameSizeChange(width, height, number_of_streams);
    }

    virtual void RenderVideoFrame(const unsigned char* buffer,
                                  unsigned int buffer_size,
                                  uint32_t time_stamp,
                                  int64_t render_time,
                                  const ImageHandle& handle) {
      pipeline_->listener_->RenderVideoFrame(buffer, buffer_size, time_stamp,
                                             render_time,
                                             handle.GetImage());
    }

   private:
    MediaPipelineReceiveVideo *pipeline_;  
  };

  
  class PipelineListener : public GenericReceiveListener {
   public:
    PipelineListener(SourceMediaStream * source, TrackID track_id,
                     bool queue_track);

    
    virtual void NotifyQueuedTrackChanges(MediaStreamGraph* graph, TrackID tid,
                                          StreamTime offset,
                                          uint32_t events,
                                          const MediaSegment& queued_media) override {}
    virtual void NotifyPull(MediaStreamGraph* graph, StreamTime desired_time) override;

    
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
                          int64_t render_time,
                          const RefPtr<layers::Image>& video_image);

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
