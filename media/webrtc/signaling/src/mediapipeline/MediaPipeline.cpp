






#include "logging.h"
#include "MediaPipeline.h"

#ifndef USE_FAKE_MEDIA_STREAMS
#include "MediaStreamGraphImpl.h"
#endif

#include <math.h>

#include "nspr.h"
#include "srtp.h"

#ifdef MOZILLA_INTERNAL_API
#include "VideoSegment.h"
#include "Layers.h"
#include "ImageTypes.h"
#include "ImageContainer.h"
#include "VideoUtils.h"
#ifdef MOZ_WIDGET_GONK
#include "GrallocImages.h"
#endif
#endif

#include "nsError.h"
#include "AudioSegment.h"
#include "MediaSegment.h"
#include "databuffer.h"
#include "transportflow.h"
#include "transportlayer.h"
#include "transportlayerdtls.h"
#include "transportlayerice.h"
#include "runnable_utils.h"
#include "gfxImageSurface.h"
#include "libyuv/convert.h"

using namespace mozilla;


MOZ_MTLOG_MODULE("mediapipeline")

namespace mozilla {

static char kDTLSExporterLabel[] = "EXTRACTOR-dtls_srtp";

MediaPipeline::~MediaPipeline() {
  MOZ_ASSERT(!stream_);  
  MOZ_MTLOG(ML_INFO, "Destroying MediaPipeline: " << description_);
}

nsresult MediaPipeline::Init() {
  ASSERT_ON_THREAD(main_thread_);

  RUN_ON_THREAD(sts_thread_,
                WrapRunnable(
                    nsRefPtr<MediaPipeline>(this),
                    &MediaPipeline::Init_s),
                NS_DISPATCH_NORMAL);

  return NS_OK;
}

nsresult MediaPipeline::Init_s() {
  ASSERT_ON_THREAD(sts_thread_);
  conduit_->AttachTransport(transport_);

  nsresult res;
  MOZ_ASSERT(rtp_transport_);
  
  rtp_transport_->SignalStateChange.connect(this,
                                            &MediaPipeline::StateChange);

  if (rtp_transport_->state() == TransportLayer::TS_OPEN) {
    res = TransportReady_s(rtp_transport_);
    if (NS_FAILED(res)) {
      MOZ_MTLOG(ML_ERROR, "Error calling TransportReady(); res="
                << static_cast<uint32_t>(res) << " in " << __FUNCTION__);
      return res;
    }
  } else if (rtp_transport_->state() == TransportLayer::TS_ERROR) {
    MOZ_MTLOG(ML_ERROR, "RTP transport is already in error state");
    TransportFailed_s(rtp_transport_);
    return NS_ERROR_FAILURE;
  }

  
  
  if (rtcp_transport_ != rtp_transport_) {
    rtcp_transport_->SignalStateChange.connect(this,
                                               &MediaPipeline::StateChange);

    if (rtcp_transport_->state() == TransportLayer::TS_OPEN) {
      res = TransportReady_s(rtcp_transport_);
      if (NS_FAILED(res)) {
        MOZ_MTLOG(ML_ERROR, "Error calling TransportReady(); res="
                  << static_cast<uint32_t>(res) << " in " << __FUNCTION__);
        return res;
      }
    } else if (rtcp_transport_->state() == TransportLayer::TS_ERROR) {
      MOZ_MTLOG(ML_ERROR, "RTCP transport is already in error state");
      TransportFailed_s(rtcp_transport_);
      return NS_ERROR_FAILURE;
    }
  }

  return NS_OK;
}





void MediaPipeline::ShutdownTransport_s() {
  ASSERT_ON_THREAD(sts_thread_);
  MOZ_ASSERT(!stream_); 

  disconnect_all();
  transport_->Detach();
  rtp_transport_ = nullptr;
  rtcp_transport_ = nullptr;
}

void MediaPipeline::StateChange(TransportFlow *flow, TransportLayer::State state) {
  
  
  if (rtcp_transport_ == rtp_transport_) {
    MOZ_ASSERT(flow == rtp_transport_);
  }

  if (state == TransportLayer::TS_OPEN) {
    MOZ_MTLOG(ML_INFO, "Flow is ready");
    TransportReady_s(flow);
  } else if (state == TransportLayer::TS_CLOSED ||
             state == TransportLayer::TS_ERROR) {
    TransportFailed_s(flow);
  }
}

nsresult MediaPipeline::TransportReady_s(TransportFlow *flow) {
  MOZ_ASSERT(!description_.empty());
  bool rtcp = !(flow == rtp_transport_);
  State *state = rtcp ? &rtcp_state_ : &rtp_state_;

  
  
  if (*state != MP_CONNECTING) {
    MOZ_MTLOG(ML_ERROR, "Transport ready for flow in wrong state:" <<
              description_ << ": " << (rtcp ? "rtcp" : "rtp"));
    return NS_ERROR_FAILURE;
  }

  nsresult res;

  MOZ_MTLOG(ML_INFO, "Transport ready for pipeline " <<
            static_cast<void *>(this) << " flow " << description_ << ": " <<
            (rtcp ? "rtcp" : "rtp"));

  
  TransportLayerDtls *dtls = static_cast<TransportLayerDtls *>(
      flow->GetLayer(TransportLayerDtls::ID()));
  MOZ_ASSERT(dtls);  

  uint16_t cipher_suite;
  res = dtls->GetSrtpCipher(&cipher_suite);
  if (NS_FAILED(res)) {
    MOZ_MTLOG(ML_ERROR, "Failed to negotiate DTLS-SRTP. This is an error");
    *state = MP_CLOSED;
    return res;
  }

  
  unsigned char srtp_block[SRTP_TOTAL_KEY_LENGTH * 2];
  res = dtls->ExportKeyingMaterial(kDTLSExporterLabel, false, "",
                                   srtp_block, sizeof(srtp_block));
  if (NS_FAILED(res)) {
    MOZ_MTLOG(ML_ERROR, "Failed to compute DTLS-SRTP keys. This is an error");
    *state = MP_CLOSED;
    MOZ_CRASH();  
                  
                  
    return res;
  }

  
  unsigned char client_write_key[SRTP_TOTAL_KEY_LENGTH];
  unsigned char server_write_key[SRTP_TOTAL_KEY_LENGTH];
  int offset = 0;
  memcpy(client_write_key, srtp_block + offset, SRTP_MASTER_KEY_LENGTH);
  offset += SRTP_MASTER_KEY_LENGTH;
  memcpy(server_write_key, srtp_block + offset, SRTP_MASTER_KEY_LENGTH);
  offset += SRTP_MASTER_KEY_LENGTH;
  memcpy(client_write_key + SRTP_MASTER_KEY_LENGTH,
         srtp_block + offset, SRTP_MASTER_SALT_LENGTH);
  offset += SRTP_MASTER_SALT_LENGTH;
  memcpy(server_write_key + SRTP_MASTER_KEY_LENGTH,
         srtp_block + offset, SRTP_MASTER_SALT_LENGTH);
  offset += SRTP_MASTER_SALT_LENGTH;
  MOZ_ASSERT(offset == sizeof(srtp_block));

  unsigned char *write_key;
  unsigned char *read_key;

  if (dtls->role() == TransportLayerDtls::CLIENT) {
    write_key = client_write_key;
    read_key = server_write_key;
  } else {
    write_key = server_write_key;
    read_key = client_write_key;
  }

  if (!rtcp) {
    
    MOZ_ASSERT(!rtp_send_srtp_ && !rtp_recv_srtp_);
    rtp_send_srtp_ = SrtpFlow::Create(cipher_suite, false,
                                      write_key, SRTP_TOTAL_KEY_LENGTH);
    rtp_recv_srtp_ = SrtpFlow::Create(cipher_suite, true,
                                      read_key, SRTP_TOTAL_KEY_LENGTH);
    if (!rtp_send_srtp_ || !rtp_recv_srtp_) {
      MOZ_MTLOG(ML_ERROR, "Couldn't create SRTP flow for RTCP");
      *state = MP_CLOSED;
      return NS_ERROR_FAILURE;
    }

    
    
    if (rtcp_transport_ == rtp_transport_) {
      MOZ_ASSERT(!rtcp_send_srtp_ && !rtcp_recv_srtp_);
      rtcp_send_srtp_ = rtp_send_srtp_;
      rtcp_recv_srtp_ = rtp_recv_srtp_;

      MOZ_MTLOG(ML_INFO, "Listening for packets received on " <<
                static_cast<void *>(dtls->downward()));

      dtls->downward()->SignalPacketReceived.connect(this,
                                                     &MediaPipeline::
                                                     PacketReceived);
      rtcp_state_ = MP_OPEN;
    } else {
      MOZ_MTLOG(ML_INFO, "Listening for RTP packets received on " <<
                static_cast<void *>(dtls->downward()));

      dtls->downward()->SignalPacketReceived.connect(this,
                                                     &MediaPipeline::
                                                     RtpPacketReceived);
    }
  }
  else {
    MOZ_ASSERT(!rtcp_send_srtp_ && !rtcp_recv_srtp_);
    rtcp_send_srtp_ = SrtpFlow::Create(cipher_suite, false,
                                       write_key, SRTP_TOTAL_KEY_LENGTH);
    rtcp_recv_srtp_ = SrtpFlow::Create(cipher_suite, true,
                                       read_key, SRTP_TOTAL_KEY_LENGTH);
    if (!rtcp_send_srtp_ || !rtcp_recv_srtp_) {
      MOZ_MTLOG(ML_ERROR, "Couldn't create SRTCP flow for RTCP");
      *state = MP_CLOSED;
      return NS_ERROR_FAILURE;
    }

    MOZ_MTLOG(ML_DEBUG, "Listening for RTCP packets received on " <<
              static_cast<void *>(dtls->downward()));

    
    dtls->downward()->SignalPacketReceived.connect(this,
                                                  &MediaPipeline::
                                                  RtcpPacketReceived);
  }

  *state = MP_OPEN;
  return NS_OK;
}

nsresult MediaPipeline::TransportFailed_s(TransportFlow *flow) {
  ASSERT_ON_THREAD(sts_thread_);
  bool rtcp = !(flow == rtp_transport_);

  State *state = rtcp ? &rtcp_state_ : &rtp_state_;

  *state = MP_CLOSED;

  
  if(rtcp_transport_ == rtp_transport_) {
    MOZ_ASSERT(state != &rtcp_state_);
    rtcp_state_ = MP_CLOSED;
  }


  MOZ_MTLOG(ML_INFO, "Transport closed for flow " << (rtcp ? "rtcp" : "rtp"));

  NS_WARNING(
      "MediaPipeline Transport failed. This is not properly cleaned up yet");


  
  
  
  

  return NS_OK;
}


nsresult MediaPipeline::SendPacket(TransportFlow *flow, const void *data,
                                   int len) {
  ASSERT_ON_THREAD(sts_thread_);

  
  TransportLayerDtls *dtls = static_cast<TransportLayerDtls *>(
      flow->GetLayer(TransportLayerDtls::ID()));
  MOZ_ASSERT(dtls);

  TransportResult res = dtls->downward()->
      SendPacket(static_cast<const unsigned char *>(data), len);

  if (res != len) {
    
    if (res == TE_WOULDBLOCK)
      return NS_OK;

    MOZ_MTLOG(ML_ERROR, "Failed write on stream");
    return NS_BASE_STREAM_CLOSED;
  }

  return NS_OK;
}

void MediaPipeline::increment_rtp_packets_sent(int32_t bytes) {
  ++rtp_packets_sent_;
  rtp_bytes_sent_ += bytes;

  if (!(rtp_packets_sent_ % 100)) {
    MOZ_MTLOG(ML_INFO, "RTP sent packet count for " << description_
              << " Pipeline " << static_cast<void *>(this)
              << " Flow : " << static_cast<void *>(rtp_transport_)
              << ": " << rtp_packets_sent_
              << " (" << rtp_bytes_sent_ << " bytes)");
  }
}

void MediaPipeline::increment_rtcp_packets_sent() {
  ++rtcp_packets_sent_;
  if (!(rtcp_packets_sent_ % 100)) {
    MOZ_MTLOG(ML_INFO, "RTCP sent packet count for " << description_
              << " Pipeline " << static_cast<void *>(this)
              << " Flow : " << static_cast<void *>(rtcp_transport_)
              << ": " << rtcp_packets_sent_);
  }
}

void MediaPipeline::increment_rtp_packets_received(int32_t bytes) {
  ++rtp_packets_received_;
  rtp_bytes_received_ += bytes;
  if (!(rtp_packets_received_ % 100)) {
    MOZ_MTLOG(ML_INFO, "RTP received packet count for " << description_
              << " Pipeline " << static_cast<void *>(this)
              << " Flow : " << static_cast<void *>(rtp_transport_)
              << ": " << rtp_packets_received_
              << " (" << rtp_bytes_received_ << " bytes)");
  }
}

void MediaPipeline::increment_rtcp_packets_received() {
  ++rtcp_packets_received_;
  if (!(rtcp_packets_received_ % 100)) {
    MOZ_MTLOG(ML_INFO, "RTCP received packet count for " << description_
              << " Pipeline " << static_cast<void *>(this)
              << " Flow : " << static_cast<void *>(rtcp_transport_)
              << ": " << rtcp_packets_received_);
  }
}

void MediaPipeline::RtpPacketReceived(TransportLayer *layer,
                                      const unsigned char *data,
                                      size_t len) {
  if (!transport_->pipeline()) {
    MOZ_MTLOG(ML_ERROR, "Discarding incoming packet; transport disconnected");
    return;
  }

  if (!conduit_) {
    MOZ_MTLOG(ML_DEBUG, "Discarding incoming packet; media disconnected");
    return;
  }

  if (rtp_state_ != MP_OPEN) {
    MOZ_MTLOG(ML_ERROR, "Discarding incoming packet; pipeline not open");
    return;
  }

  if (rtp_transport_->state() != TransportLayer::TS_OPEN) {
    MOZ_MTLOG(ML_ERROR, "Discarding incoming packet; transport not open");
    return;
  }

  MOZ_ASSERT(rtp_recv_srtp_);  

  if (direction_ == TRANSMIT) {
    
    
    return;
  }

  
  

  
  ScopedDeletePtr<unsigned char> inner_data(
      new unsigned char[len]);
  memcpy(inner_data, data, len);
  int out_len = 0;
  nsresult res = rtp_recv_srtp_->UnprotectRtp(inner_data,
                                              len, len, &out_len);
  if (!NS_SUCCEEDED(res)) {
    char tmp[16];

    PR_snprintf(tmp, sizeof(tmp), "%.2x %.2x %.2x %.2x",
                inner_data[0],
                inner_data[1],
                inner_data[2],
                inner_data[3]);

    MOZ_MTLOG(ML_NOTICE, "Error unprotecting RTP in " << description_
              << "len= " << len << "[" << tmp << "...]");

    return;
  }
  increment_rtp_packets_received(out_len);

  (void)conduit_->ReceivedRTPPacket(inner_data, out_len);  
}

void MediaPipeline::RtcpPacketReceived(TransportLayer *layer,
                                       const unsigned char *data,
                                       size_t len) {
  if (!transport_->pipeline()) {
    MOZ_MTLOG(ML_DEBUG, "Discarding incoming packet; transport disconnected");
    return;
  }

  if (!conduit_) {
    MOZ_MTLOG(ML_DEBUG, "Discarding incoming packet; media disconnected");
    return;
  }

  if (rtcp_state_ != MP_OPEN) {
    MOZ_MTLOG(ML_DEBUG, "Discarding incoming packet; pipeline not open");
    return;
  }

  if (rtcp_transport_->state() != TransportLayer::TS_OPEN) {
    MOZ_MTLOG(ML_ERROR, "Discarding incoming packet; transport not open");
    return;
  }

  if (direction_ == RECEIVE) {
    
    
    return;
  }

  increment_rtcp_packets_received();

  MOZ_ASSERT(rtcp_recv_srtp_);  

  
  ScopedDeletePtr<unsigned char> inner_data(
      new unsigned char[len]);
  memcpy(inner_data, data, len);
  int out_len;

  nsresult res = rtcp_recv_srtp_->UnprotectRtcp(inner_data, len, len, &out_len);

  if (!NS_SUCCEEDED(res))
    return;

  (void)conduit_->ReceivedRTCPPacket(inner_data, out_len);  
}

bool MediaPipeline::IsRtp(const unsigned char *data, size_t len) {
  if (len < 2)
    return false;

  
  

  
  if ((data[1] < 192) || (data[1] > 207))
    return true;

  if (data[1] == 192)  
    return false;

  if (data[1] == 193)  
    return true;       

  if (data[1] == 194)
    return true;

  if (data[1] == 195)  
    return false;

  if ((data[1] > 195) && (data[1] < 200))  
    return true;

  if ((data[1] >= 200) && (data[1] <= 207))  
    return false;                            

  MOZ_ASSERT(false);  
  return true;
}

void MediaPipeline::PacketReceived(TransportLayer *layer,
                                   const unsigned char *data,
                                   size_t len) {
  if (!transport_->pipeline()) {
    MOZ_MTLOG(ML_DEBUG, "Discarding incoming packet; transport disconnected");
    return;
  }

  if (IsRtp(data, len)) {
    RtpPacketReceived(layer, data, len);
  } else {
    RtcpPacketReceived(layer, data, len);
  }
}

nsresult MediaPipelineTransmit::Init() {
  char track_id_string[11];
  ASSERT_ON_THREAD(main_thread_);

  
  PR_snprintf(track_id_string, sizeof(track_id_string), "%d", track_id_);

  description_ = pc_ + "| ";
  description_ += conduit_->type() == MediaSessionConduit::AUDIO ?
      "Transmit audio[" : "Transmit video[";
  description_ += track_id_string;
  description_ += "]";

  
  MOZ_MTLOG(ML_DEBUG, "Attaching pipeline to stream "
            << static_cast<void *>(stream_) << " conduit type=" <<
            (conduit_->type() == MediaSessionConduit::AUDIO ?"audio":"video"));

  stream_->AddListener(listener_);

  
  
  
  if (domstream_->AddDirectListener(listener_)) {
    listener_->direct_connect_ = true;
  }

  return MediaPipeline::Init();
}

nsresult MediaPipelineTransmit::TransportReady_s(TransportFlow *flow) {
  
  MediaPipeline::TransportReady_s(flow);

  if (flow == rtp_transport_) {
    
    listener_->SetActive(true);
  }

  return NS_OK;
}

nsresult MediaPipeline::PipelineTransport::SendRtpPacket(
    const void *data, int len) {

    nsAutoPtr<DataBuffer> buf(new DataBuffer(static_cast<const uint8_t *>(data),
                                             len));

    RUN_ON_THREAD(sts_thread_,
                  WrapRunnable(
                      RefPtr<MediaPipeline::PipelineTransport>(this),
                      &MediaPipeline::PipelineTransport::SendRtpPacket_s,
                      buf),
                  NS_DISPATCH_NORMAL);

    return NS_OK;
}

nsresult MediaPipeline::PipelineTransport::SendRtpPacket_s(
    nsAutoPtr<DataBuffer> data) {
  if (!pipeline_)
    return NS_OK;  

  if (!pipeline_->rtp_send_srtp_) {
    MOZ_MTLOG(ML_DEBUG, "Couldn't write RTP packet; SRTP not set up yet");
    return NS_OK;
  }

  MOZ_ASSERT(pipeline_->rtp_transport_);
  NS_ENSURE_TRUE(pipeline_->rtp_transport_, NS_ERROR_NULL_POINTER);

  
  
  
  
  int max_len = data->len() + SRTP_MAX_EXPANSION;
  ScopedDeletePtr<unsigned char> inner_data(
      new unsigned char[max_len]);
  memcpy(inner_data, data->data(), data->len());

  int out_len;
  nsresult res = pipeline_->rtp_send_srtp_->ProtectRtp(inner_data,
                                                       data->len(),
                                                       max_len,
                                                       &out_len);
  if (!NS_SUCCEEDED(res))
    return res;

  pipeline_->increment_rtp_packets_sent(out_len);
  return pipeline_->SendPacket(pipeline_->rtp_transport_, inner_data,
                               out_len);
}

nsresult MediaPipeline::PipelineTransport::SendRtcpPacket(
    const void *data, int len) {

    nsAutoPtr<DataBuffer> buf(new DataBuffer(static_cast<const uint8_t *>(data),
                                             len));

    RUN_ON_THREAD(sts_thread_,
                  WrapRunnable(
                      RefPtr<MediaPipeline::PipelineTransport>(this),
                      &MediaPipeline::PipelineTransport::SendRtcpPacket_s,
                      buf),
                  NS_DISPATCH_NORMAL);

    return NS_OK;
}

nsresult MediaPipeline::PipelineTransport::SendRtcpPacket_s(
    nsAutoPtr<DataBuffer> data) {
  if (!pipeline_)
    return NS_OK;  

  if (!pipeline_->rtcp_send_srtp_) {
    MOZ_MTLOG(ML_DEBUG, "Couldn't write RTCP packet; SRTCP not set up yet");
    return NS_OK;
  }

  MOZ_ASSERT(pipeline_->rtcp_transport_);
  NS_ENSURE_TRUE(pipeline_->rtcp_transport_, NS_ERROR_NULL_POINTER);

  
  
  
  
  int max_len = data->len() + SRTP_MAX_EXPANSION;
  ScopedDeletePtr<unsigned char> inner_data(
      new unsigned char[max_len]);
  memcpy(inner_data, data->data(), data->len());

  int out_len;
  nsresult res = pipeline_->rtcp_send_srtp_->ProtectRtcp(inner_data,
                                                         data->len(),
                                                         max_len,
                                                         &out_len);
  if (!NS_SUCCEEDED(res))
    return res;

  pipeline_->increment_rtcp_packets_sent();
  return pipeline_->SendPacket(pipeline_->rtcp_transport_, inner_data,
                               out_len);
}


void MediaPipelineTransmit::PipelineListener::
NotifyRealtimeData(MediaStreamGraph* graph, TrackID tid,
                   TrackRate rate,
                   TrackTicks offset,
                   uint32_t events,
                   const MediaSegment& media) {
  MOZ_MTLOG(ML_DEBUG, "MediaPipeline::NotifyRealtimeData()");

  NewData(graph, tid, rate, offset, events, media);
}

void MediaPipelineTransmit::PipelineListener::
NotifyQueuedTrackChanges(MediaStreamGraph* graph, TrackID tid,
                         TrackRate rate,
                         TrackTicks offset,
                         uint32_t events,
                         const MediaSegment& queued_media) {
  MOZ_MTLOG(ML_DEBUG, "MediaPipeline::NotifyQueuedTrackChanges()");

  
  if (!direct_connect_) {
    NewData(graph, tid, rate, offset, events, queued_media);
  }
}

void MediaPipelineTransmit::PipelineListener::
NewData(MediaStreamGraph* graph, TrackID tid,
        TrackRate rate,
        TrackTicks offset,
        uint32_t events,
        const MediaSegment& media) {
  if (!active_) {
    MOZ_MTLOG(ML_DEBUG, "Discarding packets because transport not ready");
    return;
  }

  
  
  
  if (media.GetType() == MediaSegment::AUDIO) {
    if (conduit_->type() != MediaSessionConduit::AUDIO) {
      
      return;
    }
    AudioSegment* audio = const_cast<AudioSegment *>(
        static_cast<const AudioSegment *>(&media));

    AudioSegment::ChunkIterator iter(*audio);
    while(!iter.IsEnded()) {
      ProcessAudioChunk(static_cast<AudioSessionConduit*>(conduit_.get()),
                        rate, *iter);
      iter.Next();
    }
  } else if (media.GetType() == MediaSegment::VIDEO) {
#ifdef MOZILLA_INTERNAL_API
    if (conduit_->type() != MediaSessionConduit::VIDEO) {
      
      return;
    }
    VideoSegment* video = const_cast<VideoSegment *>(
        static_cast<const VideoSegment *>(&media));

    VideoSegment::ChunkIterator iter(*video);
    while(!iter.IsEnded()) {
      ProcessVideoChunk(static_cast<VideoSessionConduit*>(conduit_.get()),
                        rate, *iter);
      iter.Next();
    }
#endif
  } else {
    
  }
}

void MediaPipelineTransmit::PipelineListener::ProcessAudioChunk(
    AudioSessionConduit *conduit,
    TrackRate rate,
    AudioChunk& chunk) {
  
  nsAutoArrayPtr<int16_t> samples(new int16_t[chunk.mDuration]);

  if (chunk.mBuffer) {
    switch (chunk.mBufferFormat) {
      case AUDIO_FORMAT_FLOAT32:
        {
          const float* buf = static_cast<const float *>(chunk.mChannelData[0]);
          ConvertAudioSamplesWithScale(buf, static_cast<int16_t*>(samples),
                                       chunk.mDuration, chunk.mVolume);
        }
        break;
      case AUDIO_FORMAT_S16:
        {
          const short* buf = static_cast<const short *>(chunk.mChannelData[0]);
          ConvertAudioSamplesWithScale(buf, samples, chunk.mDuration, chunk.mVolume);
        }
        break;
      default:
        MOZ_ASSERT(PR_FALSE);
        return;
        break;
    }
  } else {
    
    for (uint32_t i = 0; i < chunk.mDuration; ++i) {
      samples[i] = 0;
    }
  }

  MOZ_ASSERT(!(rate%100)); 

  
  
  
  

  if (samplenum_10ms_ !=  rate/100) {
    
    samplenum_10ms_ = rate/100;
    
    
    samples_10ms_buffer_ = new int16_t[samplenum_10ms_];
    buffer_current_ = 0;
  }

  
  
  
  int64_t chunk_remaining;
  int64_t tocpy;
  int16_t *samples_tmp = samples.get();

  chunk_remaining = chunk.mDuration;

  MOZ_ASSERT(chunk_remaining >= 0);

  if (buffer_current_) {
    tocpy = std::min(chunk_remaining, samplenum_10ms_ - buffer_current_);
    memcpy(&samples_10ms_buffer_[buffer_current_], samples_tmp, tocpy * sizeof(int16_t));
    buffer_current_ += tocpy;
    samples_tmp += tocpy;
    chunk_remaining -= tocpy;

    if (buffer_current_ == samplenum_10ms_) {
      
      conduit->SendAudioFrame(samples_10ms_buffer_, samplenum_10ms_, rate, 0);
      buffer_current_ = 0;
    } else {
      
      return;
    }
  }

  
  tocpy = (chunk_remaining / samplenum_10ms_) * samplenum_10ms_;
  if (tocpy > 0) {
    conduit->SendAudioFrame(samples_tmp, tocpy, rate, 0);
    samples_tmp += tocpy;
    chunk_remaining -= tocpy;
  }
  

  MOZ_ASSERT(chunk_remaining < samplenum_10ms_);

  if (chunk_remaining) {
    memcpy(samples_10ms_buffer_, samples_tmp, chunk_remaining * sizeof(int16_t));
    buffer_current_ = chunk_remaining;
  }

}

#ifdef MOZILLA_INTERNAL_API
void MediaPipelineTransmit::PipelineListener::ProcessVideoChunk(
    VideoSessionConduit* conduit,
    TrackRate rate,
    VideoChunk& chunk) {
  layers::Image *img = chunk.mFrame.GetImage();

  
  if (!img) {
    
    return;
  }

  gfxIntSize size = img->GetSize();
  if ((size.width & 1) != 0 || (size.height & 1) != 0) {
    MOZ_ASSERT(false, "Can't handle odd-sized images");
    return;
  }

  if (chunk.mFrame.GetForceBlack()) {
    uint32_t yPlaneLen = size.width*size.height;
    uint32_t cbcrPlaneLen = yPlaneLen/2;
    uint32_t length = yPlaneLen + cbcrPlaneLen;

    
    nsAutoArrayPtr<uint8_t> pixelData;
    static const fallible_t fallible = fallible_t();
    pixelData = new (fallible) uint8_t[length];
    if (pixelData) {
      memset(pixelData, 0x10, yPlaneLen);
      
      memset(pixelData + yPlaneLen, 0x80, cbcrPlaneLen);

      MOZ_MTLOG(ML_DEBUG, "Sending a black video frame");
      conduit->SendVideoFrame(pixelData, length, size.width, size.height,
                              mozilla::kVideoI420, 0);
    }
    return;
  }

  
  int32_t serial = img->GetSerial();
  if (serial == last_img_) {
    return;
  }
  last_img_ = serial;

  ImageFormat format = img->GetFormat();
#ifdef MOZ_WIDGET_GONK
  if (format == GRALLOC_PLANAR_YCBCR) {
    layers::GrallocImage *nativeImage = static_cast<layers::GrallocImage*>(img);
    layers::SurfaceDescriptor handle = nativeImage->GetSurfaceDescriptor();
    layers::SurfaceDescriptorGralloc grallocHandle = handle.get_SurfaceDescriptorGralloc();

    android::sp<android::GraphicBuffer> graphicBuffer = layers::GrallocBufferActor::GetFrom(grallocHandle);
    void *basePtr;
    graphicBuffer->lock(android::GraphicBuffer::USAGE_SW_READ_MASK, &basePtr);
    conduit->SendVideoFrame(static_cast<unsigned char*>(basePtr),
                            (graphicBuffer->getWidth() * graphicBuffer->getHeight() * 3) / 2,
                            graphicBuffer->getWidth(),
                            graphicBuffer->getHeight(),
                            mozilla::kVideoNV21, 0);
    graphicBuffer->unlock();
  } else
#endif
  if (format == PLANAR_YCBCR) {
    
    layers::PlanarYCbCrImage* yuv =
    const_cast<layers::PlanarYCbCrImage *>(
          static_cast<const layers::PlanarYCbCrImage *>(img));
    
    
    const layers::PlanarYCbCrData *data = yuv->GetData();

    uint8_t *y = data->mYChannel;
#ifdef DEBUG
    uint8_t *cb = data->mCbChannel;
    uint8_t *cr = data->mCrChannel;
#endif
    uint32_t width = yuv->GetSize().width;
    uint32_t height = yuv->GetSize().height;
    uint32_t length = yuv->GetDataSize();

    
    
    MOZ_ASSERT(cb == (y + width*height) &&
               cr == (cb + width*height/4));
    
    
    
    
    
    

    
    MOZ_MTLOG(ML_DEBUG, "Sending a video frame");
    
    conduit->SendVideoFrame(y, length, width, height, mozilla::kVideoI420, 0);
  } else if(format == CAIRO_SURFACE) {
    layers::CairoImage* rgb =
    const_cast<layers::CairoImage *>(
          static_cast<const layers::CairoImage *>(img));

    gfxIntSize size = rgb->GetSize();
    int half_width = (size.width + 1) >> 1;
    int half_height = (size.height + 1) >> 1;
    int c_size = half_width * half_height;
    int buffer_size = size.width * size.height + 2 * c_size;
    uint8* yuv = (uint8*) malloc(buffer_size);
    if (!yuv)
      return;

    int cb_offset = size.width * size.height;
    int cr_offset = cb_offset + c_size;
    nsRefPtr<gfxImageSurface> surf = rgb->mSurface->GetAsImageSurface();

    switch (surf->Format()) {
      case gfxImageFormatARGB32:
      case gfxImageFormatRGB24:
        libyuv::ARGBToI420(static_cast<uint8*>(surf->Data()), surf->Stride(),
                           yuv, size.width,
                           yuv + cb_offset, half_width,
                           yuv + cr_offset, half_width,
                           size.width, size.height);
        break;
      case gfxImageFormatRGB16_565:
        libyuv::RGB565ToI420(static_cast<uint8*>(surf->Data()), surf->Stride(),
                             yuv, size.width,
                             yuv + cb_offset, half_width,
                             yuv + cr_offset, half_width,
                             size.width, size.height);
        break;
      case gfxImageFormatA1:
      case gfxImageFormatA8:
      case gfxImageFormatUnknown:
      default:
        MOZ_MTLOG(ML_ERROR, "Unsupported RGB video format");
        MOZ_ASSERT(PR_FALSE);
    }
    conduit->SendVideoFrame(yuv, buffer_size, size.width, size.height, mozilla::kVideoI420, 0);
  } else {
    MOZ_MTLOG(ML_ERROR, "Unsupported video format");
    MOZ_ASSERT(PR_FALSE);
    return;
  }
}
#endif

nsresult MediaPipelineReceiveAudio::Init() {
  char track_id_string[11];
  ASSERT_ON_THREAD(main_thread_);
  MOZ_MTLOG(ML_DEBUG, __FUNCTION__);

  
  PR_snprintf(track_id_string, sizeof(track_id_string), "%d", track_id_);

  description_ = pc_ + "| Receive audio[";
  description_ += track_id_string;
  description_ += "]";

  listener_->AddSelf(new AudioSegment());

  return MediaPipelineReceive::Init();
}



static void AddTrackAndListener(MediaStream* source,
                                TrackID track_id, TrackRate track_rate,
                                MediaStreamListener* listener, MediaSegment* segment,
                                const RefPtr<TrackAddedCallback>& completed) {
  
#ifdef MOZILLA_INTERNAL_API
  class Message : public ControlMessage {
   public:
    Message(MediaStream* stream, TrackID track, TrackRate rate,
            MediaSegment* segment, MediaStreamListener* listener,
            const RefPtr<TrackAddedCallback>& completed)
      : ControlMessage(stream),
        track_id_(track),
        track_rate_(rate),
        segment_(segment),
        listener_(listener),
        completed_(completed) {}

    virtual void Run() MOZ_OVERRIDE {
      StreamTime current_end = mStream->GetBufferEnd();
      TrackTicks current_ticks = TimeToTicksRoundUp(track_rate_, current_end);

      mStream->AddListenerImpl(listener_.forget());

      
      

      if (current_end != 0L) {
        MOZ_MTLOG(ML_DEBUG, "added track @ " << current_end <<
                  " -> " << MediaTimeToSeconds(current_end));
      }

      
      
      segment_->AppendNullData(current_ticks);
      mStream->AsSourceStream()->AddTrack(track_id_, track_rate_,
                                          current_ticks, segment_);
      
      
      
      
      
      mStream->AsSourceStream()->AdvanceKnownTracksTime(STREAM_TIME_MAX);

      
      
      completed_->TrackAdded(current_ticks);
    }
   private:
    TrackID track_id_;
    TrackRate track_rate_;
    MediaSegment* segment_;
    nsRefPtr<MediaStreamListener> listener_;
    const RefPtr<TrackAddedCallback> completed_;
  };

  MOZ_ASSERT(listener);

  source->GraphImpl()->AppendMessage(new Message(source, track_id, track_rate, segment, listener, completed));
#else
  source->AddListener(listener);
  source->AsSourceStream()->AddTrack(track_id, track_rate, 0, segment);
#endif
}

void GenericReceiveListener::AddSelf(MediaSegment* segment) {
  RefPtr<TrackAddedCallback> callback = new GenericReceiveCallback(this);
  AddTrackAndListener(source_, track_id_, track_rate_, this, segment, callback);
}

MediaPipelineReceiveAudio::PipelineListener::PipelineListener(
    SourceMediaStream * source, TrackID track_id,
    const RefPtr<MediaSessionConduit>& conduit)
  : GenericReceiveListener(source, track_id, 16000), 
    conduit_(conduit)
{
  MOZ_ASSERT(track_rate_%100 == 0);
}

void MediaPipelineReceiveAudio::PipelineListener::
NotifyPull(MediaStreamGraph* graph, StreamTime desired_time) {
  MOZ_ASSERT(source_);
  if (!source_) {
    MOZ_MTLOG(ML_ERROR, "NotifyPull() called from a non-SourceMediaStream");
    return;
  }

  
  while (TicksToTimeRoundDown(track_rate_, played_ticks_) < desired_time) {
    
    
#define AUDIO_SAMPLE_BUFFER_MAX 1000
    MOZ_ASSERT((track_rate_/100)*sizeof(uint16_t) <= AUDIO_SAMPLE_BUFFER_MAX);

    nsRefPtr<SharedBuffer> samples = SharedBuffer::Create(AUDIO_SAMPLE_BUFFER_MAX);
    int16_t *samples_data = static_cast<int16_t *>(samples->Data());
    int samples_length;

    
    MediaConduitErrorCode err =
        static_cast<AudioSessionConduit*>(conduit_.get())->GetAudioFrame(
            samples_data,
            track_rate_,
            0,  
            samples_length);
    MOZ_ASSERT(samples_length < AUDIO_SAMPLE_BUFFER_MAX);

    if (err != kMediaConduitNoError) {
      
      MOZ_MTLOG(ML_ERROR, "Audio conduit failed (" << err
                << ") to return data @ " << played_ticks_
                << " (desired " << desired_time << " -> "
                << MediaTimeToSeconds(desired_time) << ")");
      MOZ_ASSERT(err == kMediaConduitNoError);
      samples_length = (track_rate_/100)*sizeof(uint16_t); 
      memset(samples_data, '\0', samples_length);
    }

    MOZ_MTLOG(ML_DEBUG, "Audio conduit returned buffer of length "
              << samples_length);

    AudioSegment segment;
    nsAutoTArray<const int16_t*,1> channels;
    channels.AppendElement(samples_data);
    segment.AppendFrames(samples.forget(), channels, samples_length);

    
    if (source_->AppendToTrack(track_id_, &segment)) {
      played_ticks_ += track_rate_/100; 
    } else {
      MOZ_MTLOG(ML_ERROR, "AppendToTrack failed");
      
      
      return;
    }
  }
}

nsresult MediaPipelineReceiveVideo::Init() {
  char track_id_string[11];
  ASSERT_ON_THREAD(main_thread_);
  MOZ_MTLOG(ML_DEBUG, __FUNCTION__);

  
  PR_snprintf(track_id_string, sizeof(track_id_string), "%d", track_id_);

  description_ = pc_ + "| Receive video[";
  description_ += track_id_string;
  description_ += "]";

#ifdef MOZILLA_INTERNAL_API
  listener_->AddSelf(new VideoSegment());
#endif

  
  static_cast<VideoSessionConduit *>(conduit_.get())->
      AttachRenderer(renderer_);

  return MediaPipelineReceive::Init();
}

MediaPipelineReceiveVideo::PipelineListener::PipelineListener(
  SourceMediaStream* source, TrackID track_id)
  : GenericReceiveListener(source, track_id, USECS_PER_S),
    width_(640),
    height_(480),
#ifdef MOZILLA_INTERNAL_API
    image_container_(),
    image_(),
#endif
    monitor_("Video PipelineListener") {
#ifdef MOZILLA_INTERNAL_API
  image_container_ = layers::LayerManager::CreateImageContainer();
#endif
}

void MediaPipelineReceiveVideo::PipelineListener::RenderVideoFrame(
    const unsigned char* buffer,
    unsigned int buffer_size,
    uint32_t time_stamp,
    int64_t render_time) {
#ifdef MOZILLA_INTERNAL_API
  ReentrantMonitorAutoEnter enter(monitor_);

  
#ifdef MOZ_WIDGET_GONK
  ImageFormat format = GRALLOC_PLANAR_YCBCR;
#else
  ImageFormat format = PLANAR_YCBCR;
#endif
  nsRefPtr<layers::Image> image = image_container_->CreateImage(&format, 1);

  layers::PlanarYCbCrImage* videoImage = static_cast<layers::PlanarYCbCrImage*>(image.get());
  uint8_t* frame = const_cast<uint8_t*>(static_cast<const uint8_t*> (buffer));
  const uint8_t lumaBpp = 8;
  const uint8_t chromaBpp = 4;

  layers::PlanarYCbCrData data;
  data.mYChannel = frame;
  data.mYSize = gfxIntSize(width_, height_);
  data.mYStride = width_ * lumaBpp/ 8;
  data.mCbCrStride = width_ * chromaBpp / 8;
  data.mCbChannel = frame + height_ * data.mYStride;
  data.mCrChannel = data.mCbChannel + height_ * data.mCbCrStride / 2;
  data.mCbCrSize = gfxIntSize(width_/ 2, height_/ 2);
  data.mPicX = 0;
  data.mPicY = 0;
  data.mPicSize = gfxIntSize(width_, height_);
  data.mStereoMode = STEREO_MODE_MONO;

  videoImage->SetData(data);

  image_ = image.forget();
#endif
}

void MediaPipelineReceiveVideo::PipelineListener::
NotifyPull(MediaStreamGraph* graph, StreamTime desired_time) {
  ReentrantMonitorAutoEnter enter(monitor_);

#ifdef MOZILLA_INTERNAL_API
  nsRefPtr<layers::Image> image = image_;
  TrackTicks target = TimeToTicksRoundUp(USECS_PER_S, desired_time);
  TrackTicks delta = target - played_ticks_;

  
  
  
  if (delta > 0) {
    VideoSegment segment;
    segment.AppendFrame(image ? image.forget() : nullptr, delta,
                        gfxIntSize(width_, height_));
    
    if (source_->AppendToTrack(track_id_, &segment)) {
      played_ticks_ = target;
    } else {
      MOZ_MTLOG(ML_ERROR, "AppendToTrack failed");
      return;
    }
  }
#endif
}


}  
