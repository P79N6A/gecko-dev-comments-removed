





#include "CSFLog.h"

#include "MediaPipeline.h"

#include <math.h>

#include "nspr.h"
#include <prlog.h>
#include "srtp.h"

#ifdef MOZILLA_INTERNAL_API
#include "VideoSegment.h"
#include "Layers.h"
#include "ImageTypes.h"
#include "ImageContainer.h"
#include "VideoUtils.h"
#endif

#include "logging.h"
#include "nsError.h"
#include "AudioSegment.h"
#include "MediaSegment.h"
#include "databuffer.h"
#include "transportflow.h"
#include "transportlayer.h"
#include "transportlayerdtls.h"
#include "transportlayerice.h"

#include "runnable_utils.h"

using namespace mozilla;

#ifdef DEBUG

#define MP_LOG_INFO PR_LOG_WARN
#else
#define MP_LOG_INFO PR_LOG_DEBUG
#endif



MOZ_MTLOG_MODULE("mediapipeline")

namespace mozilla {

static char kDTLSExporterLabel[] = "EXTRACTOR-dtls_srtp";

nsresult MediaPipeline::Init() {
  ASSERT_ON_THREAD(main_thread_);

  
  nsresult ret;
  RUN_ON_THREAD(sts_thread_,
		WrapRunnableRet(this, &MediaPipeline::Init_s, &ret),
		NS_DISPATCH_SYNC);
  return ret;
}

nsresult MediaPipeline::Init_s() {
  ASSERT_ON_THREAD(sts_thread_);
  conduit_->AttachTransport(transport_);

  MOZ_ASSERT(rtp_transport_);

  nsresult res;

  
  rtp_transport_->SignalStateChange.connect(this,
                                            &MediaPipeline::StateChange);

  if (rtp_transport_->state() == TransportLayer::TS_OPEN) {
    res = TransportReady(rtp_transport_);
    if (NS_FAILED(res)) {
      MOZ_MTLOG(PR_LOG_ERROR, "Error calling TransportReady()");
      return res;
    }
  } else {
    if (!muxed_) {
      rtcp_transport_->SignalStateChange.connect(this,
                                                 &MediaPipeline::StateChange);

      if (rtcp_transport_->state() == TransportLayer::TS_OPEN) {
        res = TransportReady(rtcp_transport_);
        if (NS_FAILED(res)) {
          MOZ_MTLOG(PR_LOG_ERROR, "Error calling TransportReady()");
          return res;
        }
      }
    }
  }
  return NS_OK;
}




void MediaPipeline::DetachTransport_s() {
  ASSERT_ON_THREAD(sts_thread_);

  disconnect_all();
  transport_->Detach();
  rtp_transport_ = NULL;
  rtcp_transport_ = NULL;
}

void MediaPipeline::DetachTransport() {
  RUN_ON_THREAD(sts_thread_,
                WrapRunnable(this, &MediaPipeline::DetachTransport_s),
                NS_DISPATCH_SYNC);
}

void MediaPipeline::StateChange(TransportFlow *flow, TransportLayer::State state) {
  if (state == TransportLayer::TS_OPEN) {
    MOZ_MTLOG(MP_LOG_INFO, "Flow is ready");
    TransportReady(flow);
  } else if (state == TransportLayer::TS_CLOSED ||
             state == TransportLayer::TS_ERROR) {
    TransportFailed(flow);
  }
}

nsresult MediaPipeline::TransportReady(TransportFlow *flow) {
  nsresult rv;
  nsresult res;

  rv = RUN_ON_THREAD(sts_thread_,
    WrapRunnableRet(this, &MediaPipeline::TransportReadyInt, flow, &res),
    NS_DISPATCH_SYNC);

  
  if (NS_FAILED(rv))
    return rv;

  return res;
}

nsresult MediaPipeline::TransportReadyInt(TransportFlow *flow) {
  MOZ_ASSERT(!description_.empty());
  bool rtcp = !(flow == rtp_transport_);
  State *state = rtcp ? &rtcp_state_ : &rtp_state_;

  if (*state != MP_CONNECTING) {
    MOZ_MTLOG(PR_LOG_ERROR, "Transport ready for flow in wrong state:" <<
	      description_ << ": " << (rtcp ? "rtcp" : "rtp"));
    return NS_ERROR_FAILURE;
  }

  nsresult res;

  MOZ_MTLOG(MP_LOG_INFO, "Transport ready for pipeline " <<
	    static_cast<void *>(this) << " flow " << description_ << ": " <<
	    (rtcp ? "rtcp" : "rtp"));

  
  TransportLayerDtls *dtls = static_cast<TransportLayerDtls *>(
      flow->GetLayer(TransportLayerDtls::ID()));
  MOZ_ASSERT(dtls);  

  uint16_t cipher_suite;
  res = dtls->GetSrtpCipher(&cipher_suite);
  if (NS_FAILED(res)) {
    MOZ_MTLOG(PR_LOG_ERROR, "Failed to negotiate DTLS-SRTP. This is an error");
    return res;
  }

  
  unsigned char srtp_block[SRTP_TOTAL_KEY_LENGTH * 2];
  res = dtls->ExportKeyingMaterial(kDTLSExporterLabel, false, "",
                                   srtp_block, sizeof(srtp_block));
  if (NS_FAILED(res)) {
    MOZ_MTLOG(PR_LOG_ERROR, "Failed to compute DTLS-SRTP keys. This is an error");
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
      MOZ_MTLOG(PR_LOG_ERROR, "Couldn't create SRTP flow for RTCP");
      *state = MP_CLOSED;
      return NS_ERROR_FAILURE;
    }

    
    if (muxed_) {
      MOZ_ASSERT(!rtcp_send_srtp_ && !rtcp_recv_srtp_);
      rtcp_send_srtp_ = rtp_send_srtp_;
      rtcp_recv_srtp_ = rtp_recv_srtp_;

      MOZ_MTLOG(MP_LOG_INFO, "Listening for packets received on " <<
                static_cast<void *>(dtls->downward()));

      dtls->downward()->SignalPacketReceived.connect(this,
                                                     &MediaPipeline::
                                                     PacketReceived);
    } else {
      MOZ_MTLOG(MP_LOG_INFO, "Listening for RTP packets received on " <<
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
      MOZ_MTLOG(PR_LOG_ERROR, "Couldn't create SRTCP flow for RTCP");
      *state = MP_CLOSED;
      return NS_ERROR_FAILURE;
    }

    MOZ_MTLOG(MP_LOG_INFO, "Listening for RTCP packets received on " <<
      static_cast<void *>(dtls->downward()));

    
    dtls->downward()->SignalPacketReceived.connect(this,
                                                  &MediaPipeline::
                                                  RtcpPacketReceived);
  }

  *state = MP_OPEN;
  return NS_OK;
}

nsresult MediaPipeline::TransportFailed(TransportFlow *flow) {
  bool rtcp = !(flow == rtp_transport_);

  State *state = rtcp ? &rtcp_state_ : &rtp_state_;

  *state = MP_CLOSED;

  MOZ_MTLOG(MP_LOG_INFO, "Transport closed for flow " << (rtcp ? "rtcp" : "rtp"));

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

    MOZ_MTLOG(PR_LOG_ERROR, "Failed write on stream");
    return NS_BASE_STREAM_CLOSED;
  }

  return NS_OK;
}

void MediaPipeline::increment_rtp_packets_sent() {
  ++rtp_packets_sent_;

  if (!(rtp_packets_sent_ % 100)) {
    MOZ_MTLOG(MP_LOG_INFO, "RTP sent packet count for " << description_
              << " Pipeline " << static_cast<void *>(this)
              << " Flow : " << static_cast<void *>(rtp_transport_)
              << ": " << rtp_packets_sent_);
  }
}

void MediaPipeline::increment_rtcp_packets_sent() {
  ++rtcp_packets_sent_;
  if (!(rtcp_packets_sent_ % 100)) {
    MOZ_MTLOG(MP_LOG_INFO, "RTCP sent packet count for " << description_
              << " Pipeline " << static_cast<void *>(this)
              << " Flow : " << static_cast<void *>(rtcp_transport_)
              << ": " << rtcp_packets_sent_);
  }
}

void MediaPipeline::increment_rtp_packets_received() {
  ++rtp_packets_received_;
  if (!(rtp_packets_received_ % 100)) {
    MOZ_MTLOG(MP_LOG_INFO, "RTP received packet count for " << description_
              << " Pipeline " << static_cast<void *>(this)
              << " Flow : " << static_cast<void *>(rtp_transport_)
              << ": " << rtp_packets_received_);
  }
}

void MediaPipeline::increment_rtcp_packets_received() {
  ++rtcp_packets_received_;
  if (!(rtcp_packets_received_ % 100)) {
    MOZ_MTLOG(MP_LOG_INFO, "RTCP received packet count for " << description_
              << " Pipeline " << static_cast<void *>(this)
              << " Flow : " << static_cast<void *>(rtcp_transport_)
              << ": " << rtcp_packets_received_);
  }
}

void MediaPipeline::RtpPacketReceived(TransportLayer *layer,
                                      const unsigned char *data,
                                      size_t len) {
  if (!transport_->pipeline()) {
    MOZ_MTLOG(PR_LOG_DEBUG, "Discarding incoming packet; transport disconnected");
    return;
  }

  if (!conduit_) {
    MOZ_MTLOG(PR_LOG_DEBUG, "Discarding incoming packet; media disconnected");
    return;
  }

  MOZ_ASSERT(rtp_recv_srtp_);  

  if (direction_ == TRANSMIT) {
    
    
    return;
  }

  
  
  increment_rtp_packets_received();

  
  ScopedDeletePtr<unsigned char> inner_data(
      new unsigned char[len]);
  memcpy(inner_data, data, len);
  int out_len;
  nsresult res = rtp_recv_srtp_->UnprotectRtp(inner_data,
                                              len, len, &out_len);
  if (!NS_SUCCEEDED(res))
    return;

  (void)conduit_->ReceivedRTPPacket(inner_data, out_len);  
}

void MediaPipeline::RtcpPacketReceived(TransportLayer *layer,
                                       const unsigned char *data,
                                       size_t len) {
  if (!transport_->pipeline()) {
    MOZ_MTLOG(PR_LOG_DEBUG, "Discarding incoming packet; transport disconnected");
    return;
  }

  if (!conduit_) {
    MOZ_MTLOG(PR_LOG_DEBUG, "Discarding incoming packet; media disconnected");
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
    MOZ_MTLOG(PR_LOG_DEBUG, "Discarding incoming packet; transport disconnected");
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

  
  MOZ_MTLOG(PR_LOG_DEBUG, "Attaching pipeline to stream "
            << static_cast<void *>(stream_) <<
            " conduit type=" <<
            (conduit_->type() == MediaSessionConduit::AUDIO ?
             "audio" : "video"));

  stream_->AddListener(listener_);

  return MediaPipeline::Init();
}

nsresult MediaPipelineTransmit::TransportReady(TransportFlow *flow) {
  
  MediaPipeline::TransportReady(flow);

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
    MOZ_MTLOG(PR_LOG_DEBUG, "Couldn't write RTP packet; SRTP not set up yet");
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

  pipeline_->increment_rtp_packets_sent();
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
    MOZ_MTLOG(PR_LOG_DEBUG, "Couldn't write RTCP packet; SRTCP not set up yet");
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
NotifyQueuedTrackChanges(MediaStreamGraph* graph, TrackID tid,
                         TrackRate rate,
                         TrackTicks offset,
                         uint32_t events,
                         const MediaSegment& queued_media) {
  MOZ_MTLOG(PR_LOG_DEBUG, "MediaPipeline::NotifyQueuedTrackChanges()");

  if (!active_) {
    MOZ_MTLOG(PR_LOG_DEBUG, "Discarding packets because transport not ready");
    return;
  }

  
  
  
  if (queued_media.GetType() == MediaSegment::AUDIO) {
    if (conduit_->type() != MediaSessionConduit::AUDIO) {
      
      return;
    }
    AudioSegment* audio = const_cast<AudioSegment *>(
        static_cast<const AudioSegment *>(&queued_media));

    AudioSegment::ChunkIterator iter(*audio);
    while(!iter.IsEnded()) {
      ProcessAudioChunk(static_cast<AudioSessionConduit*>(conduit_.get()),
                        rate, *iter);
      iter.Next();
    }
  } else if (queued_media.GetType() == MediaSegment::VIDEO) {
#ifdef MOZILLA_INTERNAL_API
    if (conduit_->type() != MediaSessionConduit::VIDEO) {
      
      return;
    }
    VideoSegment* video = const_cast<VideoSegment *>(
        static_cast<const VideoSegment *>(&queued_media));

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
        MOZ_MTLOG(PR_LOG_ERROR, "Can't process audio except in 16-bit PCM yet");
        MOZ_ASSERT(PR_FALSE);
        return;
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

  ImageFormat format = img->GetFormat();

  if (format != PLANAR_YCBCR) {
    MOZ_MTLOG(PR_LOG_ERROR, "Can't process non-YCBCR video");
    MOZ_ASSERT(PR_FALSE);
    return;
  }

  
  layers::PlanarYCbCrImage* yuv =
    const_cast<layers::PlanarYCbCrImage *>(
      static_cast<const layers::PlanarYCbCrImage *>(img));

  
  
  const layers::PlanarYCbCrImage::Data *data = yuv->GetData();

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
  
  
  
  
  
  

  
  MOZ_MTLOG(PR_LOG_DEBUG, "Sending a video frame");
  
  conduit->SendVideoFrame(y, length, width, height, mozilla::kVideoI420, 0);
}
#endif

nsresult MediaPipelineReceiveAudio::Init() {
  char track_id_string[11];
  ASSERT_ON_THREAD(main_thread_);
  MOZ_MTLOG(PR_LOG_DEBUG, __FUNCTION__);

  
  PR_snprintf(track_id_string, sizeof(track_id_string), "%d", track_id_);

  description_ = pc_ + "| Receive audio[";
  description_ += track_id_string;
  description_ += "]";

  stream_->AddListener(listener_);

  return MediaPipelineReceive::Init();
}


MediaPipelineReceiveAudio::PipelineListener::PipelineListener(
    SourceMediaStream * source, TrackID track_id,
    const RefPtr<MediaSessionConduit>& conduit)
    : source_(source),
      track_id_(track_id),
      conduit_(conduit),
      played_(0) {
  mozilla::AudioSegment *segment = new mozilla::AudioSegment();
  source_->AddTrack(track_id_, 16000, 0, segment);
  source_->AdvanceKnownTracksTime(STREAM_TIME_MAX);
}

void MediaPipelineReceiveAudio::PipelineListener::
NotifyPull(MediaStreamGraph* graph, StreamTime desired_time) {
  MOZ_ASSERT(source_);
  if (!source_) {
    MOZ_MTLOG(PR_LOG_ERROR, "NotifyPull() called from a non-SourceMediaStream");
    return;
  }

  
  while (MillisecondsToMediaTime(played_) < desired_time) {
    
    nsRefPtr<SharedBuffer> samples = SharedBuffer::Create(1000);
    int16_t *samples_data = static_cast<int16_t *>(samples->Data());
    int samples_length;

    MediaConduitErrorCode err =
        static_cast<AudioSessionConduit*>(conduit_.get())->GetAudioFrame(
            samples_data,
            16000,  
            0,  
            samples_length);

    if (err != kMediaConduitNoError)
      return;

    MOZ_MTLOG(PR_LOG_DEBUG, "Audio conduit returned buffer of length " << samples_length);

    AudioSegment segment;
    nsAutoTArray<const int16_t*,1> channels;
    channels.AppendElement(samples_data);
    segment.AppendFrames(samples.forget(), channels, samples_length);

    source_->AppendToTrack(track_id_, &segment);

    played_ += 10;
  }
}

nsresult MediaPipelineReceiveVideo::Init() {
  char track_id_string[11];
  ASSERT_ON_THREAD(main_thread_);
  MOZ_MTLOG(PR_LOG_DEBUG, __FUNCTION__);

  
  PR_snprintf(track_id_string, sizeof(track_id_string), "%d", track_id_);

  description_ = pc_ + "| Receive video[";
  description_ += track_id_string;
  description_ += "]";

  stream_->AddListener(listener_);

  static_cast<VideoSessionConduit *>(conduit_.get())->
      AttachRenderer(renderer_);

  return MediaPipelineReceive::Init();
}

MediaPipelineReceiveVideo::PipelineListener::PipelineListener(
  SourceMediaStream* source, TrackID track_id)
    : source_(source),
      track_id_(track_id),
      played_(0),
      width_(640),
      height_(480),
#ifdef MOZILLA_INTERNAL_API
      image_container_(),
      image_(),
#endif
      monitor_("Video PipelineListener") {
#ifdef MOZILLA_INTERNAL_API
  image_container_ = layers::LayerManager::CreateImageContainer();
  source_->AddTrack(track_id_, USECS_PER_S, 0, new VideoSegment());
  source_->AdvanceKnownTracksTime(STREAM_TIME_MAX);
#endif
}

void MediaPipelineReceiveVideo::PipelineListener::RenderVideoFrame(
    const unsigned char* buffer,
    unsigned int buffer_size,
    uint32_t time_stamp,
    int64_t render_time) {
#ifdef MOZILLA_INTERNAL_API
  ReentrantMonitorAutoEnter enter(monitor_);

  
  ImageFormat format = PLANAR_YCBCR;
  nsRefPtr<layers::Image> image = image_container_->CreateImage(&format, 1);

  layers::PlanarYCbCrImage* videoImage = static_cast<layers::PlanarYCbCrImage*>(image.get());
  uint8_t* frame = const_cast<uint8_t*>(static_cast<const uint8_t*> (buffer));
  const uint8_t lumaBpp = 8;
  const uint8_t chromaBpp = 4;

  layers::PlanarYCbCrImage::Data data;
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
  TrackTicks delta = target - played_;

  
  
  
  if (delta > 0) {
    VideoSegment segment;
    segment.AppendFrame(image ? image.forget() : nullptr, delta,
                        gfxIntSize(width_, height_));
    source_->AppendToTrack(track_id_, &(segment));

    played_ = target;
  }
#endif
}


}  
