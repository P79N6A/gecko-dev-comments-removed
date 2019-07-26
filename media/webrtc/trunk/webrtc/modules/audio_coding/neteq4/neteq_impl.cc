









#include "webrtc/modules/audio_coding/neteq4/neteq_impl.h"

#include <assert.h>
#include <memory.h>  

#include <algorithm>

#include "webrtc/common_audio/signal_processing/include/signal_processing_library.h"
#include "webrtc/modules/audio_coding/neteq4/accelerate.h"
#include "webrtc/modules/audio_coding/neteq4/background_noise.h"
#include "webrtc/modules/audio_coding/neteq4/buffer_level_filter.h"
#include "webrtc/modules/audio_coding/neteq4/comfort_noise.h"
#include "webrtc/modules/audio_coding/neteq4/decision_logic.h"
#include "webrtc/modules/audio_coding/neteq4/decoder_database.h"
#include "webrtc/modules/audio_coding/neteq4/defines.h"
#include "webrtc/modules/audio_coding/neteq4/delay_manager.h"
#include "webrtc/modules/audio_coding/neteq4/delay_peak_detector.h"
#include "webrtc/modules/audio_coding/neteq4/dtmf_buffer.h"
#include "webrtc/modules/audio_coding/neteq4/dtmf_tone_generator.h"
#include "webrtc/modules/audio_coding/neteq4/expand.h"
#include "webrtc/modules/audio_coding/neteq4/interface/audio_decoder.h"
#include "webrtc/modules/audio_coding/neteq4/merge.h"
#include "webrtc/modules/audio_coding/neteq4/normal.h"
#include "webrtc/modules/audio_coding/neteq4/packet_buffer.h"
#include "webrtc/modules/audio_coding/neteq4/packet.h"
#include "webrtc/modules/audio_coding/neteq4/payload_splitter.h"
#include "webrtc/modules/audio_coding/neteq4/post_decode_vad.h"
#include "webrtc/modules/audio_coding/neteq4/preemptive_expand.h"
#include "webrtc/modules/audio_coding/neteq4/sync_buffer.h"
#include "webrtc/modules/audio_coding/neteq4/timestamp_scaler.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/logging.h"




#define LEGACY_BITEXACT

namespace webrtc {

NetEqImpl::NetEqImpl(int fs,
                     BufferLevelFilter* buffer_level_filter,
                     DecoderDatabase* decoder_database,
                     DelayManager* delay_manager,
                     DelayPeakDetector* delay_peak_detector,
                     DtmfBuffer* dtmf_buffer,
                     DtmfToneGenerator* dtmf_tone_generator,
                     PacketBuffer* packet_buffer,
                     PayloadSplitter* payload_splitter,
                     TimestampScaler* timestamp_scaler,
                     AccelerateFactory* accelerate_factory,
                     ExpandFactory* expand_factory,
                     PreemptiveExpandFactory* preemptive_expand_factory)
    : buffer_level_filter_(buffer_level_filter),
      decoder_database_(decoder_database),
      delay_manager_(delay_manager),
      delay_peak_detector_(delay_peak_detector),
      dtmf_buffer_(dtmf_buffer),
      dtmf_tone_generator_(dtmf_tone_generator),
      packet_buffer_(packet_buffer),
      payload_splitter_(payload_splitter),
      timestamp_scaler_(timestamp_scaler),
      vad_(new PostDecodeVad()),
      expand_factory_(expand_factory),
      accelerate_factory_(accelerate_factory),
      preemptive_expand_factory_(preemptive_expand_factory),
      last_mode_(kModeNormal),
      mute_factor_array_(NULL),
      decoded_buffer_length_(kMaxFrameSize),
      decoded_buffer_(new int16_t[decoded_buffer_length_]),
      playout_timestamp_(0),
      new_codec_(false),
      timestamp_(0),
      reset_decoder_(false),
      current_rtp_payload_type_(0xFF),  
      current_cng_rtp_payload_type_(0xFF),  
      ssrc_(0),
      first_packet_(true),
      error_code_(0),
      decoder_error_code_(0),
      crit_sect_(CriticalSectionWrapper::CreateCriticalSection()),
      decoded_packet_sequence_number_(-1),
      decoded_packet_timestamp_(0) {
  if (fs != 8000 && fs != 16000 && fs != 32000 && fs != 48000) {
    LOG(LS_ERROR) << "Sample rate " << fs << " Hz not supported. " <<
        "Changing to 8000 Hz.";
    fs = 8000;
  }
  LOG(LS_INFO) << "Create NetEqImpl object with fs = " << fs << ".";
  fs_hz_ = fs;
  fs_mult_ = fs / 8000;
  output_size_samples_ = kOutputSizeMs * 8 * fs_mult_;
  decoder_frame_length_ = 3 * output_size_samples_;
  WebRtcSpl_Init();
  decision_logic_.reset(DecisionLogic::Create(fs_hz_, output_size_samples_,
                                              kPlayoutOn,
                                              decoder_database_.get(),
                                              *packet_buffer_.get(),
                                              delay_manager_.get(),
                                              buffer_level_filter_.get()));
  SetSampleRateAndChannels(fs, 1);  
}

NetEqImpl::~NetEqImpl() {
  LOG(LS_INFO) << "Deleting NetEqImpl object.";
}

int NetEqImpl::InsertPacket(const WebRtcRTPHeader& rtp_header,
                            const uint8_t* payload,
                            int length_bytes,
                            uint32_t receive_timestamp) {
  CriticalSectionScoped lock(crit_sect_.get());
  LOG(LS_VERBOSE) << "InsertPacket: ts=" << rtp_header.header.timestamp <<
      ", sn=" << rtp_header.header.sequenceNumber <<
      ", pt=" << static_cast<int>(rtp_header.header.payloadType) <<
      ", ssrc=" << rtp_header.header.ssrc <<
      ", len=" << length_bytes;
  int error = InsertPacketInternal(rtp_header, payload, length_bytes,
                                   receive_timestamp, false);
  if (error != 0) {
    LOG_FERR1(LS_WARNING, InsertPacketInternal, error);
    error_code_ = error;
    return kFail;
  }
  return kOK;
}

int NetEqImpl::InsertSyncPacket(const WebRtcRTPHeader& rtp_header,
                                uint32_t receive_timestamp) {
  CriticalSectionScoped lock(crit_sect_.get());
  LOG(LS_VERBOSE) << "InsertPacket-Sync: ts="
      << rtp_header.header.timestamp <<
      ", sn=" << rtp_header.header.sequenceNumber <<
      ", pt=" << static_cast<int>(rtp_header.header.payloadType) <<
      ", ssrc=" << rtp_header.header.ssrc;

  const uint8_t kSyncPayload[] = { 's', 'y', 'n', 'c' };
  int error = InsertPacketInternal(
      rtp_header, kSyncPayload, sizeof(kSyncPayload), receive_timestamp, true);

  if (error != 0) {
    LOG_FERR1(LS_WARNING, InsertPacketInternal, error);
    error_code_ = error;
    return kFail;
  }
  return kOK;
}

int NetEqImpl::GetAudio(size_t max_length, int16_t* output_audio,
                        int* samples_per_channel, int* num_channels,
                        NetEqOutputType* type) {
  CriticalSectionScoped lock(crit_sect_.get());
  LOG(LS_VERBOSE) << "GetAudio";
  int error = GetAudioInternal(max_length, output_audio, samples_per_channel,
                               num_channels);
  LOG(LS_VERBOSE) << "Produced " << *samples_per_channel <<
      " samples/channel for " << *num_channels << " channel(s)";
  if (error != 0) {
    LOG_FERR1(LS_WARNING, GetAudioInternal, error);
    error_code_ = error;
    return kFail;
  }
  if (type) {
    *type = LastOutputType();
  }
  return kOK;
}

int NetEqImpl::RegisterPayloadType(enum NetEqDecoder codec,
                                   uint8_t rtp_payload_type) {
  CriticalSectionScoped lock(crit_sect_.get());
  LOG_API2(static_cast<int>(rtp_payload_type), codec);
  int ret = decoder_database_->RegisterPayload(rtp_payload_type, codec);
  if (ret != DecoderDatabase::kOK) {
    LOG_FERR2(LS_WARNING, RegisterPayload, rtp_payload_type, codec);
    switch (ret) {
      case DecoderDatabase::kInvalidRtpPayloadType:
        error_code_ = kInvalidRtpPayloadType;
        break;
      case DecoderDatabase::kCodecNotSupported:
        error_code_ = kCodecNotSupported;
        break;
      case DecoderDatabase::kDecoderExists:
        error_code_ = kDecoderExists;
        break;
      default:
        error_code_ = kOtherError;
    }
    return kFail;
  }
  return kOK;
}

int NetEqImpl::RegisterExternalDecoder(AudioDecoder* decoder,
                                       enum NetEqDecoder codec,
                                       int sample_rate_hz,
                                       uint8_t rtp_payload_type) {
  CriticalSectionScoped lock(crit_sect_.get());
  LOG_API2(static_cast<int>(rtp_payload_type), codec);
  if (!decoder) {
    LOG(LS_ERROR) << "Cannot register external decoder with NULL pointer";
    assert(false);
    return kFail;
  }
  int ret = decoder_database_->InsertExternal(rtp_payload_type, codec,
                                              sample_rate_hz, decoder);
  if (ret != DecoderDatabase::kOK) {
    LOG_FERR2(LS_WARNING, InsertExternal, rtp_payload_type, codec);
    switch (ret) {
      case DecoderDatabase::kInvalidRtpPayloadType:
        error_code_ = kInvalidRtpPayloadType;
        break;
      case DecoderDatabase::kCodecNotSupported:
        error_code_ = kCodecNotSupported;
        break;
      case DecoderDatabase::kDecoderExists:
        error_code_ = kDecoderExists;
        break;
      case DecoderDatabase::kInvalidSampleRate:
        error_code_ = kInvalidSampleRate;
        break;
      case DecoderDatabase::kInvalidPointer:
        error_code_ = kInvalidPointer;
        break;
      default:
        error_code_ = kOtherError;
    }
    return kFail;
  }
  return kOK;
}

int NetEqImpl::RemovePayloadType(uint8_t rtp_payload_type) {
  CriticalSectionScoped lock(crit_sect_.get());
  LOG_API1(static_cast<int>(rtp_payload_type));
  int ret = decoder_database_->Remove(rtp_payload_type);
  if (ret == DecoderDatabase::kOK) {
    return kOK;
  } else if (ret == DecoderDatabase::kDecoderNotFound) {
    error_code_ = kDecoderNotFound;
  } else {
    error_code_ = kOtherError;
  }
  LOG_FERR1(LS_WARNING, Remove, rtp_payload_type);
  return kFail;
}

bool NetEqImpl::SetMinimumDelay(int delay_ms) {
  CriticalSectionScoped lock(crit_sect_.get());
  if (delay_ms >= 0 && delay_ms < 10000) {
    assert(delay_manager_.get());
    return delay_manager_->SetMinimumDelay(delay_ms);
  }
  return false;
}

bool NetEqImpl::SetMaximumDelay(int delay_ms) {
  CriticalSectionScoped lock(crit_sect_.get());
  if (delay_ms >= 0 && delay_ms < 10000) {
    assert(delay_manager_.get());
    return delay_manager_->SetMaximumDelay(delay_ms);
  }
  return false;
}

int NetEqImpl::LeastRequiredDelayMs() const {
  CriticalSectionScoped lock(crit_sect_.get());
  assert(delay_manager_.get());
  return delay_manager_->least_required_delay_ms();
}

void NetEqImpl::SetPlayoutMode(NetEqPlayoutMode mode) {
  CriticalSectionScoped lock(crit_sect_.get());
  if (!decision_logic_.get() || mode != decision_logic_->playout_mode()) {
    
    decision_logic_.reset(DecisionLogic::Create(fs_hz_, output_size_samples_,
                                                mode,
                                                decoder_database_.get(),
                                                *packet_buffer_.get(),
                                                delay_manager_.get(),
                                                buffer_level_filter_.get()));
  }
}

NetEqPlayoutMode NetEqImpl::PlayoutMode() const {
  CriticalSectionScoped lock(crit_sect_.get());
  assert(decision_logic_.get());
  return decision_logic_->playout_mode();
}

int NetEqImpl::NetworkStatistics(NetEqNetworkStatistics* stats) {
  CriticalSectionScoped lock(crit_sect_.get());
  assert(decoder_database_.get());
  const int total_samples_in_buffers = packet_buffer_->NumSamplesInBuffer(
      decoder_database_.get(), decoder_frame_length_) +
          static_cast<int>(sync_buffer_->FutureLength());
  assert(delay_manager_.get());
  assert(decision_logic_.get());
  stats_.GetNetworkStatistics(fs_hz_, total_samples_in_buffers,
                              decoder_frame_length_, *delay_manager_.get(),
                              *decision_logic_.get(), stats);
  return 0;
}

void NetEqImpl::WaitingTimes(std::vector<int>* waiting_times) {
  CriticalSectionScoped lock(crit_sect_.get());
  stats_.WaitingTimes(waiting_times);
}

void NetEqImpl::GetRtcpStatistics(RtcpStatistics* stats) {
  CriticalSectionScoped lock(crit_sect_.get());
  if (stats) {
    rtcp_.GetStatistics(false, stats);
  }
}

void NetEqImpl::GetRtcpStatisticsNoReset(RtcpStatistics* stats) {
  CriticalSectionScoped lock(crit_sect_.get());
  if (stats) {
    rtcp_.GetStatistics(true, stats);
  }
}

void NetEqImpl::EnableVad() {
  CriticalSectionScoped lock(crit_sect_.get());
  assert(vad_.get());
  vad_->Enable();
}

void NetEqImpl::DisableVad() {
  CriticalSectionScoped lock(crit_sect_.get());
  assert(vad_.get());
  vad_->Disable();
}

uint32_t NetEqImpl::PlayoutTimestamp() {
  CriticalSectionScoped lock(crit_sect_.get());
  return timestamp_scaler_->ToExternal(playout_timestamp_);
}

int NetEqImpl::LastError() {
  CriticalSectionScoped lock(crit_sect_.get());
  return error_code_;
}

int NetEqImpl::LastDecoderError() {
  CriticalSectionScoped lock(crit_sect_.get());
  return decoder_error_code_;
}

void NetEqImpl::FlushBuffers() {
  CriticalSectionScoped lock(crit_sect_.get());
  LOG_API0();
  packet_buffer_->Flush();
  assert(sync_buffer_.get());
  assert(expand_.get());
  sync_buffer_->Flush();
  sync_buffer_->set_next_index(sync_buffer_->next_index() -
                               expand_->overlap_length());
  
  first_packet_ = true;
}

void NetEqImpl::PacketBufferStatistics(int* current_num_packets,
                                       int* max_num_packets,
                                       int* current_memory_size_bytes,
                                       int* max_memory_size_bytes) const {
  CriticalSectionScoped lock(crit_sect_.get());
  packet_buffer_->BufferStat(current_num_packets, max_num_packets,
                             current_memory_size_bytes, max_memory_size_bytes);
}

int NetEqImpl::DecodedRtpInfo(int* sequence_number, uint32_t* timestamp) const {
  CriticalSectionScoped lock(crit_sect_.get());
  if (decoded_packet_sequence_number_ < 0)
    return -1;
  *sequence_number = decoded_packet_sequence_number_;
  *timestamp = decoded_packet_timestamp_;
  return 0;
}

void NetEqImpl::SetBackgroundNoiseMode(NetEqBackgroundNoiseMode mode) {
  CriticalSectionScoped lock(crit_sect_.get());
  assert(background_noise_.get());
  background_noise_->set_mode(mode);
}

NetEqBackgroundNoiseMode NetEqImpl::BackgroundNoiseMode() const {
  CriticalSectionScoped lock(crit_sect_.get());
  assert(background_noise_.get());
  return background_noise_->mode();
}



int NetEqImpl::InsertPacketInternal(const WebRtcRTPHeader& rtp_header,
                                    const uint8_t* payload,
                                    int length_bytes,
                                    uint32_t receive_timestamp,
                                    bool is_sync_packet) {
  if (!payload) {
    LOG_F(LS_ERROR) << "payload == NULL";
    return kInvalidPointer;
  }
  
  if (is_sync_packet) {
    if (decoder_database_->IsDtmf(rtp_header.header.payloadType) ||
        decoder_database_->IsRed(rtp_header.header.payloadType) ||
        decoder_database_->IsComfortNoise(rtp_header.header.payloadType)) {
      LOG_F(LS_ERROR) << "Sync-packet with an unacceptable payload type "
          << rtp_header.header.payloadType;
      return kSyncPacketNotAccepted;
    }
    if (first_packet_ ||
        rtp_header.header.payloadType != current_rtp_payload_type_ ||
        rtp_header.header.ssrc != ssrc_) {
      
      
      LOG_F(LS_ERROR) << "Changing codec, SSRC or first packet "
          "with sync-packet.";
      return kSyncPacketNotAccepted;
    }
  }
  PacketList packet_list;
  RTPHeader main_header;
  {
    
    
    
    
    Packet* packet = new Packet;
    packet->header.markerBit = false;
    packet->header.payloadType = rtp_header.header.payloadType;
    packet->header.sequenceNumber = rtp_header.header.sequenceNumber;
    packet->header.timestamp = rtp_header.header.timestamp;
    packet->header.ssrc = rtp_header.header.ssrc;
    packet->header.numCSRCs = 0;
    packet->payload_length = length_bytes;
    packet->primary = true;
    packet->waiting_time = 0;
    packet->payload = new uint8_t[packet->payload_length];
    packet->sync_packet = is_sync_packet;
    if (!packet->payload) {
      LOG_F(LS_ERROR) << "Payload pointer is NULL.";
    }
    assert(payload);  
    memcpy(packet->payload, payload, packet->payload_length);
    
    packet_list.push_back(packet);
    
    memcpy(&main_header, &packet->header, sizeof(main_header));
  }

  bool update_sample_rate_and_channels = false;
  
  if ((main_header.ssrc != ssrc_) || first_packet_) {
    rtcp_.Init(main_header.sequenceNumber);
    first_packet_ = false;

    
    packet_buffer_->Flush();
    dtmf_buffer_->Flush();

    
    ssrc_ = main_header.ssrc;

    
    sync_buffer_->IncreaseEndTimestamp(main_header.timestamp - timestamp_);

    
    timestamp_ = main_header.timestamp;
    current_rtp_payload_type_ = main_header.payloadType;

    
    new_codec_ = true;

    
    timestamp_scaler_->Reset();

    
    update_sample_rate_and_channels = true;
  }

  
  if (!is_sync_packet)
    rtcp_.Update(main_header, receive_timestamp);

  
  if (decoder_database_->IsRed(main_header.payloadType)) {
    assert(!is_sync_packet);  
    if (payload_splitter_->SplitRed(&packet_list) != PayloadSplitter::kOK) {
      LOG_FERR1(LS_WARNING, SplitRed, packet_list.size());
      PacketBuffer::DeleteAllPackets(&packet_list);
      return kRedundancySplitError;
    }
    
    
    payload_splitter_->CheckRedPayloads(&packet_list, *decoder_database_);
    
    
    memcpy(&main_header, &packet_list.front()->header, sizeof(main_header));
  }

  
  if (decoder_database_->CheckPayloadTypes(packet_list) ==
      DecoderDatabase::kDecoderNotFound) {
    LOG_FERR1(LS_WARNING, CheckPayloadTypes, packet_list.size());
    PacketBuffer::DeleteAllPackets(&packet_list);
    return kUnknownRtpPayloadType;
  }

  
  timestamp_scaler_->ToInternal(&packet_list);

  
  
  PacketList::iterator it = packet_list.begin();
  while (it != packet_list.end()) {
    Packet* current_packet = (*it);
    assert(current_packet);
    assert(current_packet->payload);
    if (decoder_database_->IsDtmf(current_packet->header.payloadType)) {
      assert(!current_packet->sync_packet);  
      DtmfEvent event;
      int ret = DtmfBuffer::ParseEvent(
          current_packet->header.timestamp,
          current_packet->payload,
          current_packet->payload_length,
          &event);
      if (ret != DtmfBuffer::kOK) {
        LOG_FERR2(LS_WARNING, ParseEvent, ret,
                  current_packet->payload_length);
        PacketBuffer::DeleteAllPackets(&packet_list);
        return kDtmfParsingError;
      }
      if (dtmf_buffer_->InsertEvent(event) != DtmfBuffer::kOK) {
        LOG_FERR0(LS_WARNING, InsertEvent);
        PacketBuffer::DeleteAllPackets(&packet_list);
        return kDtmfInsertError;
      }
      
      delete [] current_packet->payload;
      delete current_packet;
      it = packet_list.erase(it);
    } else {
      ++it;
    }
  }

  
  
  
  int ret = payload_splitter_->SplitAudio(&packet_list, *decoder_database_);
  if (ret != PayloadSplitter::kOK) {
    LOG_FERR1(LS_WARNING, SplitAudio, packet_list.size());
    PacketBuffer::DeleteAllPackets(&packet_list);
    switch (ret) {
      case PayloadSplitter::kUnknownPayloadType:
        return kUnknownRtpPayloadType;
      case PayloadSplitter::kFrameSplitError:
        return kFrameSplitError;
      default:
        return kOtherError;
    }
  }

  
  if (!packet_list.empty() && !packet_list.front()->sync_packet) {
    
    AudioDecoder* decoder =
        decoder_database_->GetDecoder(main_header.payloadType);
    assert(decoder);  
                      
    decoder->IncomingPacket(packet_list.front()->payload,
                            packet_list.front()->payload_length,
                            packet_list.front()->header.sequenceNumber,
                            packet_list.front()->header.timestamp,
                            receive_timestamp);
  }

  
  int temp_bufsize = packet_buffer_->NumPacketsInBuffer();
  ret = packet_buffer_->InsertPacketList(
      &packet_list,
      *decoder_database_,
      &current_rtp_payload_type_,
      &current_cng_rtp_payload_type_);
  if (ret == PacketBuffer::kFlushed) {
    
    new_codec_ = true;
    update_sample_rate_and_channels = true;
    LOG_F(LS_WARNING) << "Packet buffer flushed";
  } else if (ret == PacketBuffer::kOversizePacket) {
    LOG_F(LS_WARNING) << "Packet larger than packet buffer";
    return kOversizePacket;
  } else if (ret != PacketBuffer::kOK) {
    LOG_FERR1(LS_WARNING, InsertPacketList, packet_list.size());
    PacketBuffer::DeleteAllPackets(&packet_list);
    return kOtherError;
  }
  if (current_rtp_payload_type_ != 0xFF) {
    const DecoderDatabase::DecoderInfo* dec_info =
        decoder_database_->GetDecoderInfo(current_rtp_payload_type_);
    if (!dec_info) {
      assert(false);  
    }
  }

  if (update_sample_rate_and_channels && !packet_buffer_->Empty()) {
    
    
    
    
    
    
    const RTPHeader* rtp_header = packet_buffer_->NextRtpHeader();
    assert(rtp_header);
    int payload_type = rtp_header->payloadType;
    AudioDecoder* decoder = decoder_database_->GetDecoder(payload_type);
    assert(decoder);  
    const DecoderDatabase::DecoderInfo* decoder_info =
        decoder_database_->GetDecoderInfo(payload_type);
    assert(decoder_info);
    if (decoder_info->fs_hz != fs_hz_ ||
        decoder->channels() != algorithm_buffer_->Channels())
      SetSampleRateAndChannels(decoder_info->fs_hz, decoder->channels());
  }

  
  const DecoderDatabase::DecoderInfo* dec_info =
          decoder_database_->GetDecoderInfo(main_header.payloadType);
  assert(dec_info);  
  delay_manager_->LastDecoderType(dec_info->codec_type);
  if (delay_manager_->last_pack_cng_or_dtmf() == 0) {
    
    temp_bufsize = packet_buffer_->NumPacketsInBuffer() - temp_bufsize;
    temp_bufsize *= decoder_frame_length_;

    if ((temp_bufsize > 0) &&
        (temp_bufsize != decision_logic_->packet_length_samples())) {
      decision_logic_->set_packet_length_samples(temp_bufsize);
      delay_manager_->SetPacketAudioLength((1000 * temp_bufsize) / fs_hz_);
    }

    
    if ((int32_t) (main_header.timestamp - timestamp_) >= 0 &&
        !new_codec_) {
      
      
      delay_manager_->Update(main_header.sequenceNumber, main_header.timestamp,
                             fs_hz_);
    }
  } else if (delay_manager_->last_pack_cng_or_dtmf() == -1) {
    
    
    
    delay_manager_->set_last_pack_cng_or_dtmf(0);
    delay_manager_->ResetPacketIatCount();
  }
  return 0;
}

int NetEqImpl::GetAudioInternal(size_t max_length, int16_t* output,
                                int* samples_per_channel, int* num_channels) {
  PacketList packet_list;
  DtmfEvent dtmf_event;
  Operations operation;
  bool play_dtmf;
  int return_value = GetDecision(&operation, &packet_list, &dtmf_event,
                                 &play_dtmf);
  if (return_value != 0) {
    LOG_FERR1(LS_WARNING, GetDecision, return_value);
    assert(false);
    last_mode_ = kModeError;
    return return_value;
  }
  LOG(LS_VERBOSE) << "GetDecision returned operation=" << operation <<
      " and " << packet_list.size() << " packet(s)";

  AudioDecoder::SpeechType speech_type;
  int length = 0;
  int decode_return_value = Decode(&packet_list, &operation,
                                   &length, &speech_type);

  assert(vad_.get());
  bool sid_frame_available =
      (operation == kRfc3389Cng && !packet_list.empty());
  vad_->Update(decoded_buffer_.get(), length, speech_type,
               sid_frame_available, fs_hz_);

  algorithm_buffer_->Clear();
  switch (operation) {
    case kNormal: {
      DoNormal(decoded_buffer_.get(), length, speech_type, play_dtmf);
      break;
    }
    case kMerge: {
      DoMerge(decoded_buffer_.get(), length, speech_type, play_dtmf);
      break;
    }
    case kExpand: {
      return_value = DoExpand(play_dtmf);
      break;
    }
    case kAccelerate: {
      return_value = DoAccelerate(decoded_buffer_.get(), length, speech_type,
                                  play_dtmf);
      break;
    }
    case kPreemptiveExpand: {
      return_value = DoPreemptiveExpand(decoded_buffer_.get(), length,
                                        speech_type, play_dtmf);
      break;
    }
    case kRfc3389Cng:
    case kRfc3389CngNoPacket: {
      return_value = DoRfc3389Cng(&packet_list, play_dtmf);
      break;
    }
    case kCodecInternalCng: {
      
      
      
      DoCodecInternalCng();
      break;
    }
    case kDtmf: {
      
      return_value = DoDtmf(dtmf_event, &play_dtmf);
      break;
    }
    case kAlternativePlc: {
      
      DoAlternativePlc(false);
      break;
    }
    case kAlternativePlcIncreaseTimestamp: {
      
      DoAlternativePlc(true);
      break;
    }
    case kAudioRepetitionIncreaseTimestamp: {
      
      sync_buffer_->IncreaseEndTimestamp(output_size_samples_);
      
      
    }
    case kAudioRepetition: {
      
      
      
      algorithm_buffer_->PushBackFromIndex(
          *sync_buffer_, sync_buffer_->Size() - output_size_samples_);
      expand_->Reset();
      break;
    }
    case kUndefined: {
      LOG_F(LS_ERROR) << "Invalid operation kUndefined.";
      assert(false);  
      last_mode_ = kModeError;
      return kInvalidOperation;
    }
  }  
  if (return_value < 0) {
    return return_value;
  }

  if (last_mode_ != kModeRfc3389Cng) {
    comfort_noise_->Reset();
  }

  
  sync_buffer_->PushBack(*algorithm_buffer_);

  
  size_t num_output_samples_per_channel = output_size_samples_;
  size_t num_output_samples = output_size_samples_ * sync_buffer_->Channels();
  if (num_output_samples > max_length) {
    LOG(LS_WARNING) << "Output array is too short. " << max_length << " < " <<
        output_size_samples_ << " * " << sync_buffer_->Channels();
    num_output_samples = max_length;
    num_output_samples_per_channel = static_cast<int>(
        max_length / sync_buffer_->Channels());
  }
  int samples_from_sync = static_cast<int>(
      sync_buffer_->GetNextAudioInterleaved(num_output_samples_per_channel,
                                            output));
  *num_channels = static_cast<int>(sync_buffer_->Channels());
  LOG(LS_VERBOSE) << "Sync buffer (" << *num_channels << " channel(s)):" <<
      " insert " << algorithm_buffer_->Size() << " samples, extract " <<
      samples_from_sync << " samples";
  if (samples_from_sync != output_size_samples_) {
    LOG_F(LS_ERROR) << "samples_from_sync != output_size_samples_";
    
    memset(output, 0, num_output_samples * sizeof(int16_t));
    *samples_per_channel = output_size_samples_;
    return kSampleUnderrun;
  }
  *samples_per_channel = output_size_samples_;

  
  assert(sync_buffer_->FutureLength() >= expand_->overlap_length());

  if (play_dtmf) {
    return_value = DtmfOverdub(dtmf_event, sync_buffer_->Channels(), output);
  }

  
  
  
  if ((last_mode_ == kModeNormal) ||
      (last_mode_ == kModeAccelerateFail) ||
      (last_mode_ == kModePreemptiveExpandFail) ||
      (last_mode_ == kModeRfc3389Cng) ||
      (last_mode_ == kModeCodecInternalCng)) {
    background_noise_->Update(*sync_buffer_, *vad_.get());
  }

  if (operation == kDtmf) {
    
    
    sync_buffer_->set_dtmf_index(sync_buffer_->Size());
  }

  if ((last_mode_ != kModeExpand) && (last_mode_ != kModeRfc3389Cng)) {
    
    
    
    uint32_t temp_timestamp = sync_buffer_->end_timestamp() -
        static_cast<uint32_t>(sync_buffer_->FutureLength());
    if (static_cast<int32_t>(temp_timestamp - playout_timestamp_) > 0) {
      playout_timestamp_ = temp_timestamp;
    }
  } else {
    
    playout_timestamp_ += output_size_samples_;
  }

  if (decode_return_value) return decode_return_value;
  return return_value;
}

int NetEqImpl::GetDecision(Operations* operation,
                           PacketList* packet_list,
                           DtmfEvent* dtmf_event,
                           bool* play_dtmf) {
  
  *play_dtmf = false;
  *operation = kUndefined;

  
  packet_buffer_->IncrementWaitingTimes();
  stats_.IncreaseCounter(output_size_samples_, fs_hz_);

  assert(sync_buffer_.get());
  uint32_t end_timestamp = sync_buffer_->end_timestamp();
  if (!new_codec_) {
    packet_buffer_->DiscardOldPackets(end_timestamp);
  }
  const RTPHeader* header = packet_buffer_->NextRtpHeader();

  if (decision_logic_->CngRfc3389On()) {
    
    
    
    while (header &&
        decoder_database_->IsComfortNoise(header->payloadType) &&
        end_timestamp >= header->timestamp) {
      
      
      if (packet_buffer_->DiscardNextPacket() != PacketBuffer::kOK) {
        assert(false);  
      }
      
      if (!new_codec_) {
        packet_buffer_->DiscardOldPackets(end_timestamp);
      }
      header = packet_buffer_->NextRtpHeader();
    }
  }

  assert(expand_.get());
  const int samples_left = static_cast<int>(sync_buffer_->FutureLength() -
      expand_->overlap_length());
  if (last_mode_ == kModeAccelerateSuccess ||
      last_mode_ == kModeAccelerateLowEnergy ||
      last_mode_ == kModePreemptiveExpandSuccess ||
      last_mode_ == kModePreemptiveExpandLowEnergy) {
    
    decision_logic_->AddSampleMemory(-(samples_left + output_size_samples_));
  }

  
  if (dtmf_buffer_->GetEvent(end_timestamp +
                             decision_logic_->generated_noise_samples(),
                             dtmf_event)) {
    *play_dtmf = true;
  }

  
  assert(sync_buffer_.get());
  assert(expand_.get());
  *operation = decision_logic_->GetDecision(*sync_buffer_,
                                            *expand_,
                                            decoder_frame_length_,
                                            header,
                                            last_mode_,
                                            *play_dtmf,
                                            &reset_decoder_);

  
  
  
  if (samples_left >= output_size_samples_ &&
      *operation != kMerge &&
      *operation != kAccelerate &&
      *operation != kPreemptiveExpand) {
    *operation = kNormal;
    return 0;
  }

  decision_logic_->ExpandDecision(*operation == kExpand);

  
  if (new_codec_ || *operation == kUndefined) {
    
    assert(new_codec_);
    if (*play_dtmf && !header) {
      timestamp_ = dtmf_event->timestamp;
    } else {
      assert(header);
      if (!header) {
        LOG_F(LS_ERROR) << "Packet missing where it shouldn't.";
        return -1;
      }
      timestamp_ = header->timestamp;
      if (*operation == kRfc3389CngNoPacket
#ifndef LEGACY_BITEXACT
          
          
          
          
          && decoder_database_->IsComfortNoise(header->payloadType)
#endif
      ) {
        
        
        *operation = kRfc3389Cng;
      } else if (*operation != kRfc3389Cng) {
        *operation = kNormal;
      }
    }
    
    
    sync_buffer_->IncreaseEndTimestamp(timestamp_ - end_timestamp);
    end_timestamp = timestamp_;
    new_codec_ = false;
    decision_logic_->SoftReset();
    buffer_level_filter_->Reset();
    delay_manager_->Reset();
    stats_.ResetMcu();
  }

  int required_samples = output_size_samples_;
  const int samples_10_ms = 80 * fs_mult_;
  const int samples_20_ms = 2 * samples_10_ms;
  const int samples_30_ms = 3 * samples_10_ms;

  switch (*operation) {
    case kExpand: {
      timestamp_ = end_timestamp;
      return 0;
    }
    case kRfc3389CngNoPacket:
    case kCodecInternalCng: {
      return 0;
    }
    case kDtmf: {
      
      
      timestamp_ = end_timestamp;
      if (decision_logic_->generated_noise_samples() > 0 &&
          last_mode_ != kModeDtmf) {
        
        uint32_t timestamp_jump = decision_logic_->generated_noise_samples();
        sync_buffer_->IncreaseEndTimestamp(timestamp_jump);
        timestamp_ += timestamp_jump;
      }
      decision_logic_->set_generated_noise_samples(0);
      return 0;
    }
    case kAccelerate: {
      
      if (samples_left >= samples_30_ms) {
        
        decision_logic_->set_sample_memory(samples_left);
        decision_logic_->set_prev_time_scale(true);
        return 0;
      } else if (samples_left >= samples_10_ms &&
          decoder_frame_length_ >= samples_30_ms) {
        
        *operation = kNormal;
        return 0;
      } else if (samples_left < samples_20_ms &&
          decoder_frame_length_ < samples_30_ms) {
        
        
        
        required_samples = 2 * output_size_samples_;
        *operation = kNormal;
      }
      
      
      
      
      
      break;
    }
    case kPreemptiveExpand: {
      
      
      if ((samples_left >= samples_30_ms) ||
          (samples_left >= samples_10_ms &&
              decoder_frame_length_ >= samples_30_ms)) {
        
        
        
        decision_logic_->set_sample_memory(samples_left);
        decision_logic_->set_prev_time_scale(true);
        return 0;
      }
      if (samples_left < samples_20_ms &&
          decoder_frame_length_ < samples_30_ms) {
        
        
        required_samples = 2 * output_size_samples_;
      }
      
      break;
    }
    default: {
      
    }
  }

  
  int extracted_samples = 0;
  if (header &&
      *operation != kAlternativePlc &&
      *operation != kAlternativePlcIncreaseTimestamp &&
      *operation != kAudioRepetition &&
      *operation != kAudioRepetitionIncreaseTimestamp) {
    sync_buffer_->IncreaseEndTimestamp(header->timestamp - end_timestamp);
    if (decision_logic_->CngOff()) {
      
      
      
      
      stats_.LostSamples(header->timestamp - end_timestamp);
    }

    if (*operation != kRfc3389Cng) {
      
      decision_logic_->SetCngOff();
    }
    
    
    decision_logic_->set_generated_noise_samples(0);

    extracted_samples = ExtractPackets(required_samples, packet_list);
    if (extracted_samples < 0) {
      LOG_F(LS_WARNING) << "Failed to extract packets from buffer.";
      return kPacketBufferCorruption;
    }
  }

  if (*operation == kAccelerate ||
      *operation == kPreemptiveExpand) {
    decision_logic_->set_sample_memory(samples_left + extracted_samples);
    decision_logic_->set_prev_time_scale(true);
  }

  if (*operation == kAccelerate) {
    
    if (extracted_samples + samples_left < samples_30_ms) {
      
      
      *operation = kNormal;
    }
  }

  timestamp_ = end_timestamp;
  return 0;
}

int NetEqImpl::Decode(PacketList* packet_list, Operations* operation,
                      int* decoded_length,
                      AudioDecoder::SpeechType* speech_type) {
  *speech_type = AudioDecoder::kSpeech;
  AudioDecoder* decoder = NULL;
  if (!packet_list->empty()) {
    const Packet* packet = packet_list->front();
    int payload_type = packet->header.payloadType;
    if (!decoder_database_->IsComfortNoise(payload_type)) {
      decoder = decoder_database_->GetDecoder(payload_type);
      assert(decoder);
      if (!decoder) {
        LOG_FERR1(LS_WARNING, GetDecoder, payload_type);
        PacketBuffer::DeleteAllPackets(packet_list);
        return kDecoderNotFound;
      }
      bool decoder_changed;
      decoder_database_->SetActiveDecoder(payload_type, &decoder_changed);
      if (decoder_changed) {
        
        const DecoderDatabase::DecoderInfo* decoder_info = decoder_database_
            ->GetDecoderInfo(payload_type);
        assert(decoder_info);
        if (!decoder_info) {
          LOG_FERR1(LS_WARNING, GetDecoderInfo, payload_type);
          PacketBuffer::DeleteAllPackets(packet_list);
          return kDecoderNotFound;
        }
        
        
        if (decoder_info->fs_hz != fs_hz_ ||
            decoder->channels() != algorithm_buffer_->Channels()) {
          LOG_F(LS_ERROR) << "Sampling rate or number of channels mismatch.";
          assert(false);
          SetSampleRateAndChannels(decoder_info->fs_hz, decoder->channels());
        }
        sync_buffer_->set_end_timestamp(timestamp_);
        playout_timestamp_ = timestamp_;
      }
    }
  }

  if (reset_decoder_) {
    
    
    if (decoder) {
      decoder->Init();
    }
    
    AudioDecoder* cng_decoder = decoder_database_->GetActiveCngDecoder();
    if (cng_decoder) {
      cng_decoder->Init();
    }
    reset_decoder_ = false;
  }

#ifdef LEGACY_BITEXACT
  
  
  
  
  if (*operation == kRfc3389Cng) {
    return 0;
  }
#endif

  *decoded_length = 0;
  
  if ((*operation == kMerge) && decoder && decoder->HasDecodePlc()) {
    decoder->DecodePlc(1, &decoded_buffer_[*decoded_length]);
  }

  int return_value = DecodeLoop(packet_list, operation, decoder,
                                decoded_length, speech_type);

  if (*decoded_length < 0) {
    
    *decoded_length = 0;
    sync_buffer_->IncreaseEndTimestamp(decoder_frame_length_);
    int error_code = 0;
    if (decoder)
      error_code = decoder->ErrorCode();
    if (error_code != 0) {
      
      decoder_error_code_ = error_code;
      return_value = kDecoderErrorCode;
    } else {
      
      return_value = kOtherDecoderError;
    }
    LOG_FERR2(LS_WARNING, DecodeLoop, error_code, packet_list->size());
    *operation = kExpand;  
  }
  if (*speech_type != AudioDecoder::kComfortNoise) {
    
    
    
    assert(*decoded_length == 0 ||
           (decoder && decoder->channels() == sync_buffer_->Channels()));
    sync_buffer_->IncreaseEndTimestamp(
        *decoded_length / static_cast<int>(sync_buffer_->Channels()));
  }
  return return_value;
}

int NetEqImpl::DecodeLoop(PacketList* packet_list, Operations* operation,
                          AudioDecoder* decoder, int* decoded_length,
                          AudioDecoder::SpeechType* speech_type) {
  Packet* packet = NULL;
  if (!packet_list->empty()) {
    packet = packet_list->front();
  }
  
  while (packet &&
      !decoder_database_->IsComfortNoise(packet->header.payloadType)) {
    assert(decoder);  
    
    
    assert(sync_buffer_->Channels() == decoder->channels());
    assert(decoded_buffer_length_ >= kMaxFrameSize * decoder->channels());
    assert(*operation == kNormal || *operation == kAccelerate ||
           *operation == kMerge || *operation == kPreemptiveExpand);
    packet_list->pop_front();
    int payload_length = packet->payload_length;
    int16_t decode_length;
    if (packet->sync_packet) {
      
      LOG(LS_VERBOSE) << "Decoding sync-packet: " <<
          " ts=" << packet->header.timestamp <<
          ", sn=" << packet->header.sequenceNumber <<
          ", pt=" << static_cast<int>(packet->header.payloadType) <<
          ", ssrc=" << packet->header.ssrc <<
          ", len=" << packet->payload_length;
      memset(&decoded_buffer_[*decoded_length], 0, decoder_frame_length_ *
             decoder->channels() * sizeof(decoded_buffer_[0]));
      decode_length = decoder_frame_length_;
    } else if (!packet->primary) {
      
      LOG(LS_VERBOSE) << "Decoding packet (redundant):" <<
          " ts=" << packet->header.timestamp <<
          ", sn=" << packet->header.sequenceNumber <<
          ", pt=" << static_cast<int>(packet->header.payloadType) <<
          ", ssrc=" << packet->header.ssrc <<
          ", len=" << packet->payload_length;
      decode_length = decoder->DecodeRedundant(
          packet->payload, packet->payload_length,
          &decoded_buffer_[*decoded_length], speech_type);
    } else {
      LOG(LS_VERBOSE) << "Decoding packet: ts=" << packet->header.timestamp <<
          ", sn=" << packet->header.sequenceNumber <<
          ", pt=" << static_cast<int>(packet->header.payloadType) <<
          ", ssrc=" << packet->header.ssrc <<
          ", len=" << packet->payload_length;
      decode_length = decoder->Decode(packet->payload,
                                      packet->payload_length,
                                      &decoded_buffer_[*decoded_length],
                                      speech_type);
    }

    delete[] packet->payload;
    delete packet;
    packet = NULL;
    if (decode_length > 0) {
      *decoded_length += decode_length;
      
      decoder_frame_length_ = decode_length /
          static_cast<int>(decoder->channels());
      LOG(LS_VERBOSE) << "Decoded " << decode_length << " samples (" <<
          decoder->channels() << " channel(s) -> " << decoder_frame_length_ <<
          " samples per channel)";
    } else if (decode_length < 0) {
      
      LOG_FERR2(LS_WARNING, Decode, decode_length, payload_length);
      *decoded_length = -1;
      PacketBuffer::DeleteAllPackets(packet_list);
      break;
    }
    if (*decoded_length > static_cast<int>(decoded_buffer_length_)) {
      
      LOG_F(LS_WARNING) << "Decoded too much.";
      PacketBuffer::DeleteAllPackets(packet_list);
      return kDecodedTooMuch;
    }
    if (!packet_list->empty()) {
      packet = packet_list->front();
    } else {
      packet = NULL;
    }
  }  

  
  
  assert(packet_list->empty() || *decoded_length < 0 ||
         (packet_list->size() == 1 && packet &&
             decoder_database_->IsComfortNoise(packet->header.payloadType)));
  return 0;
}

void NetEqImpl::DoNormal(const int16_t* decoded_buffer, size_t decoded_length,
                         AudioDecoder::SpeechType speech_type, bool play_dtmf) {
  assert(normal_.get());
  assert(mute_factor_array_.get());
  normal_->Process(decoded_buffer, decoded_length, last_mode_,
                   mute_factor_array_.get(), algorithm_buffer_.get());
  if (decoded_length != 0) {
    last_mode_ = kModeNormal;
  }

  
  if ((speech_type == AudioDecoder::kComfortNoise)
      || ((last_mode_ == kModeCodecInternalCng)
          && (decoded_length == 0))) {
    
    last_mode_ = kModeCodecInternalCng;
  }

  if (!play_dtmf) {
    dtmf_tone_generator_->Reset();
  }
}

void NetEqImpl::DoMerge(int16_t* decoded_buffer, size_t decoded_length,
                        AudioDecoder::SpeechType speech_type, bool play_dtmf) {
  assert(mute_factor_array_.get());
  assert(merge_.get());
  int new_length = merge_->Process(decoded_buffer, decoded_length,
                                   mute_factor_array_.get(),
                                   algorithm_buffer_.get());

  
  if (expand_->MuteFactor(0) == 0) {
    
    stats_.ExpandedNoiseSamples(new_length - static_cast<int>(decoded_length));
  } else {
    
    stats_.ExpandedVoiceSamples(new_length - static_cast<int>(decoded_length));
  }

  last_mode_ = kModeMerge;
  
  if (speech_type == AudioDecoder::kComfortNoise) {
    last_mode_ = kModeCodecInternalCng;
  }
  expand_->Reset();
  if (!play_dtmf) {
    dtmf_tone_generator_->Reset();
  }
}

int NetEqImpl::DoExpand(bool play_dtmf) {
  while ((sync_buffer_->FutureLength() - expand_->overlap_length()) <
      static_cast<size_t>(output_size_samples_)) {
    algorithm_buffer_->Clear();
    int return_value = expand_->Process(algorithm_buffer_.get());
    int length = static_cast<int>(algorithm_buffer_->Size());

    
    if (expand_->MuteFactor(0) == 0) {
      
      stats_.ExpandedNoiseSamples(length);
    } else {
      
      stats_.ExpandedVoiceSamples(length);
    }

    last_mode_ = kModeExpand;

    if (return_value < 0) {
      return return_value;
    }

    sync_buffer_->PushBack(*algorithm_buffer_);
    algorithm_buffer_->Clear();
  }
  if (!play_dtmf) {
    dtmf_tone_generator_->Reset();
  }
  return 0;
}

int NetEqImpl::DoAccelerate(int16_t* decoded_buffer, size_t decoded_length,
                            AudioDecoder::SpeechType speech_type,
                            bool play_dtmf) {
  const size_t required_samples = 240 * fs_mult_;  
  size_t borrowed_samples_per_channel = 0;
  size_t num_channels = algorithm_buffer_->Channels();
  size_t decoded_length_per_channel = decoded_length / num_channels;
  if (decoded_length_per_channel < required_samples) {
    
    borrowed_samples_per_channel = static_cast<int>(required_samples -
        decoded_length_per_channel);
    memmove(&decoded_buffer[borrowed_samples_per_channel * num_channels],
            decoded_buffer,
            sizeof(int16_t) * decoded_length);
    sync_buffer_->ReadInterleavedFromEnd(borrowed_samples_per_channel,
                                         decoded_buffer);
    decoded_length = required_samples * num_channels;
  }

  int16_t samples_removed;
  Accelerate::ReturnCodes return_code = accelerate_->Process(
      decoded_buffer, decoded_length, algorithm_buffer_.get(),
      &samples_removed);
  stats_.AcceleratedSamples(samples_removed);
  switch (return_code) {
    case Accelerate::kSuccess:
      last_mode_ = kModeAccelerateSuccess;
      break;
    case Accelerate::kSuccessLowEnergy:
      last_mode_ = kModeAccelerateLowEnergy;
      break;
    case Accelerate::kNoStretch:
      last_mode_ = kModeAccelerateFail;
      break;
    case Accelerate::kError:
      
      last_mode_ = kModeAccelerateFail;
      return kAccelerateError;
  }

  if (borrowed_samples_per_channel > 0) {
    
    size_t length = algorithm_buffer_->Size();
    if (length < borrowed_samples_per_channel) {
      
      
      sync_buffer_->ReplaceAtIndex(*algorithm_buffer_,
                                   sync_buffer_->Size() -
                                   borrowed_samples_per_channel);
      sync_buffer_->PushFrontZeros(borrowed_samples_per_channel - length);
      algorithm_buffer_->PopFront(length);
      assert(algorithm_buffer_->Empty());
    } else {
      sync_buffer_->ReplaceAtIndex(*algorithm_buffer_,
                                   borrowed_samples_per_channel,
                                   sync_buffer_->Size() -
                                   borrowed_samples_per_channel);
      algorithm_buffer_->PopFront(borrowed_samples_per_channel);
    }
  }

  
  if (speech_type == AudioDecoder::kComfortNoise) {
    last_mode_ = kModeCodecInternalCng;
  }
  if (!play_dtmf) {
    dtmf_tone_generator_->Reset();
  }
  expand_->Reset();
  return 0;
}

int NetEqImpl::DoPreemptiveExpand(int16_t* decoded_buffer,
                                  size_t decoded_length,
                                  AudioDecoder::SpeechType speech_type,
                                  bool play_dtmf) {
  const size_t required_samples = 240 * fs_mult_;  
  size_t num_channels = algorithm_buffer_->Channels();
  int borrowed_samples_per_channel = 0;
  int old_borrowed_samples_per_channel = 0;
  size_t decoded_length_per_channel = decoded_length / num_channels;
  if (decoded_length_per_channel < required_samples) {
    
    borrowed_samples_per_channel = static_cast<int>(required_samples -
        decoded_length_per_channel);
    
    old_borrowed_samples_per_channel = static_cast<int>(
        borrowed_samples_per_channel - sync_buffer_->FutureLength());
    old_borrowed_samples_per_channel = std::max(
        0, old_borrowed_samples_per_channel);
    memmove(&decoded_buffer[borrowed_samples_per_channel * num_channels],
            decoded_buffer,
            sizeof(int16_t) * decoded_length);
    sync_buffer_->ReadInterleavedFromEnd(borrowed_samples_per_channel,
                                         decoded_buffer);
    decoded_length = required_samples * num_channels;
  }

  int16_t samples_added;
  PreemptiveExpand::ReturnCodes return_code = preemptive_expand_->Process(
      decoded_buffer, static_cast<int>(decoded_length),
      old_borrowed_samples_per_channel,
      algorithm_buffer_.get(), &samples_added);
  stats_.PreemptiveExpandedSamples(samples_added);
  switch (return_code) {
    case PreemptiveExpand::kSuccess:
      last_mode_ = kModePreemptiveExpandSuccess;
      break;
    case PreemptiveExpand::kSuccessLowEnergy:
      last_mode_ = kModePreemptiveExpandLowEnergy;
      break;
    case PreemptiveExpand::kNoStretch:
      last_mode_ = kModePreemptiveExpandFail;
      break;
    case PreemptiveExpand::kError:
      
      last_mode_ = kModePreemptiveExpandFail;
      return kPreemptiveExpandError;
  }

  if (borrowed_samples_per_channel > 0) {
    
    sync_buffer_->ReplaceAtIndex(
        *algorithm_buffer_, borrowed_samples_per_channel,
        sync_buffer_->Size() - borrowed_samples_per_channel);
    algorithm_buffer_->PopFront(borrowed_samples_per_channel);
  }

  
  if (speech_type == AudioDecoder::kComfortNoise) {
    last_mode_ = kModeCodecInternalCng;
  }
  if (!play_dtmf) {
    dtmf_tone_generator_->Reset();
  }
  expand_->Reset();
  return 0;
}

int NetEqImpl::DoRfc3389Cng(PacketList* packet_list, bool play_dtmf) {
  if (!packet_list->empty()) {
    
    assert(packet_list->size() == 1);
    Packet* packet = packet_list->front();
    packet_list->pop_front();
    if (!decoder_database_->IsComfortNoise(packet->header.payloadType)) {
#ifdef LEGACY_BITEXACT
      
      
      
      
      if (fs_hz_ == 8000) {
        packet->header.payloadType =
            decoder_database_->GetRtpPayloadType(kDecoderCNGnb);
      } else if (fs_hz_ == 16000) {
        packet->header.payloadType =
            decoder_database_->GetRtpPayloadType(kDecoderCNGwb);
      } else if (fs_hz_ == 32000) {
        packet->header.payloadType =
            decoder_database_->GetRtpPayloadType(kDecoderCNGswb32kHz);
      } else if (fs_hz_ == 48000) {
        packet->header.payloadType =
            decoder_database_->GetRtpPayloadType(kDecoderCNGswb48kHz);
      }
      assert(decoder_database_->IsComfortNoise(packet->header.payloadType));
#else
      LOG(LS_ERROR) << "Trying to decode non-CNG payload as CNG.";
      return kOtherError;
#endif
    }
    
    if (comfort_noise_->UpdateParameters(packet) ==
        ComfortNoise::kInternalError) {
      LOG_FERR0(LS_WARNING, UpdateParameters);
      algorithm_buffer_->Zeros(output_size_samples_);
      return -comfort_noise_->internal_error_code();
    }
  }
  int cn_return = comfort_noise_->Generate(output_size_samples_,
                                           algorithm_buffer_.get());
  expand_->Reset();
  last_mode_ = kModeRfc3389Cng;
  if (!play_dtmf) {
    dtmf_tone_generator_->Reset();
  }
  if (cn_return == ComfortNoise::kInternalError) {
    LOG_FERR1(LS_WARNING, comfort_noise_->Generate, cn_return);
    decoder_error_code_ = comfort_noise_->internal_error_code();
    return kComfortNoiseErrorCode;
  } else if (cn_return == ComfortNoise::kUnknownPayloadType) {
    LOG_FERR1(LS_WARNING, comfort_noise_->Generate, cn_return);
    return kUnknownRtpPayloadType;
  }
  return 0;
}

void NetEqImpl::DoCodecInternalCng() {
  int length = 0;
  
  int16_t decoded_buffer[kMaxFrameSize];
  AudioDecoder* decoder = decoder_database_->GetActiveDecoder();
  if (decoder) {
    const uint8_t* dummy_payload = NULL;
    AudioDecoder::SpeechType speech_type;
    length = decoder->Decode(dummy_payload, 0, decoded_buffer, &speech_type);
  }
  assert(mute_factor_array_.get());
  normal_->Process(decoded_buffer, length, last_mode_, mute_factor_array_.get(),
                   algorithm_buffer_.get());
  last_mode_ = kModeCodecInternalCng;
  expand_->Reset();
}

int NetEqImpl::DoDtmf(const DtmfEvent& dtmf_event, bool* play_dtmf) {
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  int dtmf_return_value = 0;
  if (!dtmf_tone_generator_->initialized()) {
    
    dtmf_return_value = dtmf_tone_generator_->Init(fs_hz_, dtmf_event.event_no,
                                                   dtmf_event.volume);
  }

  if (dtmf_return_value == 0) {
    
    dtmf_return_value = dtmf_tone_generator_->Generate(output_size_samples_,
                                                       algorithm_buffer_.get());
  }

  if (dtmf_return_value < 0) {
    algorithm_buffer_->Zeros(output_size_samples_);
    return dtmf_return_value;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  sync_buffer_->IncreaseEndTimestamp(output_size_samples_);
  expand_->Reset();
  last_mode_ = kModeDtmf;

  
  *play_dtmf = false;
  return 0;
}

void NetEqImpl::DoAlternativePlc(bool increase_timestamp) {
  AudioDecoder* decoder = decoder_database_->GetActiveDecoder();
  int length;
  if (decoder && decoder->HasDecodePlc()) {
    
    
    int16_t decoded_buffer[kMaxFrameSize];
    length = decoder->DecodePlc(1, decoded_buffer);
    if (length > 0) {
      algorithm_buffer_->PushBackInterleaved(decoded_buffer, length);
    } else {
      length = 0;
    }
  } else {
    
    length = output_size_samples_;
    algorithm_buffer_->Zeros(length);
    
    stats_.AddZeros(length);
  }
  if (increase_timestamp) {
    sync_buffer_->IncreaseEndTimestamp(length);
  }
  expand_->Reset();
}

int NetEqImpl::DtmfOverdub(const DtmfEvent& dtmf_event, size_t num_channels,
                           int16_t* output) const {
  size_t out_index = 0;
  int overdub_length = output_size_samples_;  

  if (sync_buffer_->dtmf_index() > sync_buffer_->next_index()) {
    
    out_index = std::min(
        sync_buffer_->dtmf_index() - sync_buffer_->next_index(),
        static_cast<size_t>(output_size_samples_));
    overdub_length = output_size_samples_ - static_cast<int>(out_index);
  }

  AudioMultiVector dtmf_output(num_channels);
  int dtmf_return_value = 0;
  if (!dtmf_tone_generator_->initialized()) {
    dtmf_return_value = dtmf_tone_generator_->Init(fs_hz_, dtmf_event.event_no,
                                                   dtmf_event.volume);
  }
  if (dtmf_return_value == 0) {
    dtmf_return_value = dtmf_tone_generator_->Generate(overdub_length,
                                                       &dtmf_output);
    assert((size_t) overdub_length == dtmf_output.Size());
  }
  dtmf_output.ReadInterleaved(overdub_length, &output[out_index]);
  return dtmf_return_value < 0 ? dtmf_return_value : 0;
}

int NetEqImpl::ExtractPackets(int required_samples, PacketList* packet_list) {
  bool first_packet = true;
  uint8_t prev_payload_type = 0;
  uint32_t prev_timestamp = 0;
  uint16_t prev_sequence_number = 0;
  bool next_packet_available = false;

  const RTPHeader* header = packet_buffer_->NextRtpHeader();
  assert(header);
  if (!header) {
    return -1;
  }
  uint32_t first_timestamp = header->timestamp;
  int extracted_samples = 0;

  
  do {
    timestamp_ = header->timestamp;
    int discard_count = 0;
    Packet* packet = packet_buffer_->GetNextPacket(&discard_count);
    
    header = NULL;
    if (!packet) {
      LOG_FERR1(LS_ERROR, GetNextPacket, discard_count) <<
          "Should always be able to extract a packet here";
      assert(false);  
      return -1;
    }
    stats_.PacketsDiscarded(discard_count);
    
    stats_.StoreWaitingTime(packet->waiting_time * kOutputSizeMs);
    assert(packet->payload_length > 0);
    packet_list->push_back(packet);  

    if (first_packet) {
      first_packet = false;
      decoded_packet_sequence_number_ = prev_sequence_number =
          packet->header.sequenceNumber;
      decoded_packet_timestamp_ = prev_timestamp = packet->header.timestamp;
      prev_payload_type = packet->header.payloadType;
    }

    
    int packet_duration = 0;
    AudioDecoder* decoder = decoder_database_->GetDecoder(
        packet->header.payloadType);
    if (decoder) {
      packet_duration = packet->sync_packet ? decoder_frame_length_ :
          decoder->PacketDuration(packet->payload, packet->payload_length);
    } else {
      LOG_FERR1(LS_WARNING, GetDecoder, packet->header.payloadType) <<
          "Could not find a decoder for a packet about to be extracted.";
      assert(false);
    }
    if (packet_duration <= 0) {
      
      
      packet_duration = decoder_frame_length_;
    }
    extracted_samples = packet->header.timestamp - first_timestamp +
        packet_duration;

    
    header = packet_buffer_->NextRtpHeader();
    next_packet_available = false;
    if (header && prev_payload_type == header->payloadType) {
      int16_t seq_no_diff = header->sequenceNumber - prev_sequence_number;
      int32_t ts_diff = header->timestamp - prev_timestamp;
      if (seq_no_diff == 1 ||
          (seq_no_diff == 0 && ts_diff == decoder_frame_length_)) {
        
        
        next_packet_available = true;
      }
      prev_sequence_number = header->sequenceNumber;
    }
  } while (extracted_samples < required_samples && next_packet_available);

  return extracted_samples;
}

void NetEqImpl::SetSampleRateAndChannels(int fs_hz, size_t channels) {
  LOG_API2(fs_hz, channels);
  
  assert(fs_hz == 8000 || fs_hz == 16000 || fs_hz ==  32000 || fs_hz == 48000);
  assert(channels > 0);

  fs_hz_ = fs_hz;
  fs_mult_ = fs_hz / 8000;
  output_size_samples_ = kOutputSizeMs * 8 * fs_mult_;
  decoder_frame_length_ = 3 * output_size_samples_;  

  last_mode_ = kModeNormal;

  
  mute_factor_array_.reset(new int16_t[channels]);
  for (size_t i = 0; i < channels; ++i) {
    mute_factor_array_[i] = 16384;  
  }

  
  AudioDecoder* cng_decoder = decoder_database_->GetActiveCngDecoder();
  if (cng_decoder) {
    cng_decoder->Init();
  }

  
  assert(vad_.get());  
  vad_->Init();

  
  algorithm_buffer_.reset(new AudioMultiVector(channels));

  
  sync_buffer_.reset(new SyncBuffer(channels, kSyncBufferSize * fs_mult_));


  
  
  NetEqBackgroundNoiseMode current_mode = kBgnOn;
  if (background_noise_.get())
    current_mode = background_noise_->mode();
  background_noise_.reset(new BackgroundNoise(channels));
  background_noise_->set_mode(current_mode);

  
  random_vector_.Reset();

  
  expand_.reset(expand_factory_->Create(background_noise_.get(),
                                        sync_buffer_.get(), &random_vector_,
                                        fs_hz, channels));
  
  sync_buffer_->set_next_index(sync_buffer_->next_index() -
                               expand_->overlap_length());

  normal_.reset(new Normal(fs_hz, decoder_database_.get(), *background_noise_,
                           expand_.get()));
  merge_.reset(new Merge(fs_hz, channels, expand_.get(), sync_buffer_.get()));
  accelerate_.reset(
      accelerate_factory_->Create(fs_hz, channels, *background_noise_));
  preemptive_expand_.reset(
      preemptive_expand_factory_->Create(fs_hz, channels, *background_noise_));

  
  comfort_noise_.reset(new ComfortNoise(fs_hz, decoder_database_.get(),
                                        sync_buffer_.get()));

  
  if (decoded_buffer_length_ < kMaxFrameSize * channels) {
    
    decoded_buffer_length_ = kMaxFrameSize * channels;
    decoded_buffer_.reset(new int16_t[decoded_buffer_length_]);
  }

  
  assert(decision_logic_.get());
  decision_logic_->SetSampleRate(fs_hz_, output_size_samples_);
}

NetEqOutputType NetEqImpl::LastOutputType() {
  assert(vad_.get());
  assert(expand_.get());
  if (last_mode_ == kModeCodecInternalCng || last_mode_ == kModeRfc3389Cng) {
    return kOutputCNG;
  } else if (last_mode_ == kModeExpand && expand_->MuteFactor(0) == 0) {
    
    return kOutputPLCtoCNG;
  } else if (last_mode_ == kModeExpand) {
    return kOutputPLC;
  } else if (vad_->running() && !vad_->active_speech()) {
    return kOutputVADPassive;
  } else {
    return kOutputNormal;
  }
}

}  
