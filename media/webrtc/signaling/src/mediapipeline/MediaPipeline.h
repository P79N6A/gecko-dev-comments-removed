






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
                TrackID track_id,
                int level,
                RefPtr<MediaSessionConduit> conduit,
                RefPtr<TransportFlow> rtp_transport,
                RefPtr<TransportFlow> rtcp_transport)
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
        description_() {
      
      
      
      MOZ_ASSERT(rtp_transport != rtcp_transport);

      
      transport_ = new PipelineTransport(this);
  }

  
  void ShutdownTransport_s();

  
  void ShutdownMedia_m() {
    ASSERT_ON_THREAD(main_thread_);

    if (stream_) {
      DetachMediaStream();
    }
  }

  virtual nsresult Init();

  
  
  
  
  
  
  void SetUsingBundle_s(bool decision);
  MediaPipelineFilter* UpdateFilterFromRemoteDescription_s(
      nsAutoPtr<MediaPipelineFilter> filter);

  virtual Direction direction() const { return direction_; }
  virtual TrackID trackid() const { return track_id_; }
  virtual int level() const { return level_; }
  virtual bool IsVideo() const { return false; }

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
                                
                                
                                
  TrackID track_id_;            
                                
                                
  int level_; 
  RefPtr<MediaSessionConduit> conduit_;  
                                         

  
  TransportInfo rtp_;
  TransportInfo rtcp_;
  
  
  
  
  
  
  
  
  nsAutoPtr<TransportInfo> possible_bundle_rtp_;
  nsAutoPtr<TransportInfo> possible_bundle_rtcp_;

  
  
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
                        int pipeline_index, 
                        int level,
                        bool is_video,
                        RefPtr<MediaSessionConduit> conduit,
                        RefPtr<TransportFlow> rtp_transport,
                        RefPtr<TransportFlow> rtcp_transport) :
      MediaPipeline(pc, TRANSMIT, main_thread, sts_thread,
                    domstream->GetStream(), TRACK_INVALID, level,
                    conduit, rtp_transport, rtcp_transport),
      listener_(new PipelineListener(conduit)),
      domstream_(domstream),
      pipeline_index_(pipeline_index),
      is_video_(is_video)
  {}

  
  virtual nsresult Init() MOZ_OVERRIDE;

  virtual void AttachToTrack(TrackID track_id);

  
  virtual TrackID pipeline_index() const { return pipeline_index_; }
  
  
  virtual TrackID const trackid_locked() { return listener_->trackid(); }
  
  virtual bool IsVideo() const MOZ_OVERRIDE { return is_video_; }

#ifdef MOZILLA_INTERNAL_API
  
  
  virtual void UpdateSinkIdentity_m(nsIPrincipal* principal,
                                    const PeerIdentity* sinkIdentity);
#endif

  
  virtual void DetachMediaStream() {
    ASSERT_ON_THREAD(main_thread_);
    domstream_->RemoveDirectListener(listener_);
    domstream_ = nullptr;
    stream_->RemoveListener(listener_);
    
    stream_ = nullptr;
  }

  
  virtual nsresult TransportReady_s(TransportInfo &info);

  
  
  
  
  virtual nsresult ReplaceTrack(DOMMediaStream *domstream,
                                TrackID track_id);


  
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
                                          TrackRate rate,
                                          TrackTicks offset,
                                          uint32_t events,
                                          const MediaSegment& queued_media) MOZ_OVERRIDE;
    virtual void NotifyPull(MediaStreamGraph* aGraph, StreamTime aDesiredTime) MOZ_OVERRIDE {}

    
    virtual void NotifyRealtimeData(MediaStreamGraph* graph, TrackID tid,
                                    TrackRate rate,
                                    TrackTicks offset,
                                    uint32_t events,
                                    const MediaSegment& media) MOZ_OVERRIDE;

   private:
    void NewData(MediaStreamGraph* graph, TrackID tid,
                 TrackRate rate,
                 TrackTicks offset,
                 uint32_t events,
                 const MediaSegment& media);

    virtual void ProcessAudioChunk(AudioSessionConduit *conduit,
                                   TrackRate rate, AudioChunk& chunk);
#ifdef MOZILLA_INTERNAL_API
    virtual void ProcessVideoChunk(VideoSessionConduit *conduit,
                                   TrackRate rate, VideoChunk& chunk);
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
  int pipeline_index_; 
  bool is_video_;
};




class MediaPipelineReceive : public MediaPipeline {
 public:
  
  MediaPipelineReceive(const std::string& pc,
                       nsCOMPtr<nsIEventTarget> main_thread,
                       nsCOMPtr<nsIEventTarget> sts_thread,
                       MediaStream *stream,
                       TrackID track_id,
                       int level,
                       RefPtr<MediaSessionConduit> conduit,
                       RefPtr<TransportFlow> rtp_transport,
                       RefPtr<TransportFlow> rtcp_transport,
                       RefPtr<TransportFlow> bundle_rtp_transport,
                       RefPtr<TransportFlow> bundle_rtcp_transport,
                       nsAutoPtr<MediaPipelineFilter> filter) :
      MediaPipeline(pc, RECEIVE, main_thread, sts_thread,
                    stream, track_id, level, conduit, rtp_transport,
                    rtcp_transport),
      segments_added_(0) {
    filter_ = filter;
    rtp_parser_ = webrtc::RtpHeaderParser::Create();
    if (bundle_rtp_transport) {
      if (bundle_rtcp_transport) {
        MOZ_ASSERT(bundle_rtp_transport != bundle_rtcp_transport);
        possible_bundle_rtp_ = new TransportInfo(bundle_rtp_transport, RTP);
        possible_bundle_rtcp_ = new TransportInfo(bundle_rtcp_transport, RTCP);
      } else {
        possible_bundle_rtp_ = new TransportInfo(bundle_rtp_transport, MUX);
        possible_bundle_rtcp_ = new TransportInfo(bundle_rtp_transport, MUX);
      }
    }
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
                            int level,
                            RefPtr<AudioSessionConduit> conduit,
                            RefPtr<TransportFlow> rtp_transport,
                            RefPtr<TransportFlow> rtcp_transport,
                            RefPtr<TransportFlow> bundle_rtp_transport,
                            RefPtr<TransportFlow> bundle_rtcp_transport,
                            nsAutoPtr<MediaPipelineFilter> filter) :
      MediaPipelineReceive(pc, main_thread, sts_thread,
                           stream, track_id, level, conduit, rtp_transport,
                           rtcp_transport, bundle_rtp_transport,
                           bundle_rtcp_transport, filter),
      listener_(new PipelineListener(stream->AsSourceStream(),
                                     track_id, conduit)) {
  }

  virtual void DetachMediaStream() {
    ASSERT_ON_THREAD(main_thread_);
    listener_->EndTrack();
    stream_->RemoveListener(listener_);
    stream_ = nullptr;
  }

  virtual nsresult Init() MOZ_OVERRIDE;

 private:
  
  class PipelineListener : public GenericReceiveListener {
   public:
    PipelineListener(SourceMediaStream * source, TrackID track_id,
                     const RefPtr<MediaSessionConduit>& conduit);

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
                            int level,
                            RefPtr<VideoSessionConduit> conduit,
                            RefPtr<TransportFlow> rtp_transport,
                            RefPtr<TransportFlow> rtcp_transport,
                            RefPtr<TransportFlow> bundle_rtp_transport,
                            RefPtr<TransportFlow> bundle_rtcp_transport,
                            nsAutoPtr<MediaPipelineFilter> filter) :
      MediaPipelineReceive(pc, main_thread, sts_thread,
                           stream, track_id, level, conduit, rtp_transport,
                           rtcp_transport, bundle_rtp_transport,
                           bundle_rtcp_transport, filter),
      renderer_(new PipelineRenderer(MOZ_THIS_IN_INITIALIZER_LIST())),
      listener_(new PipelineListener(stream->AsSourceStream(), track_id)) {
  }

  
  virtual void DetachMediaStream() {
    ASSERT_ON_THREAD(main_thread_);

    listener_->EndTrack();
    
    
    
    
    static_cast<VideoSessionConduit*>(conduit_.get())->DetachRenderer();
    stream_->RemoveListener(listener_);
    stream_ = nullptr;
  }

  virtual nsresult Init() MOZ_OVERRIDE;

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
