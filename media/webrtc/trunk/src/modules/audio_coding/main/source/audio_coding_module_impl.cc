









#include "audio_coding_module_impl.h"

#include <assert.h>
#include <stdlib.h>
#ifdef ACM_QA_TEST
#   include <stdio.h>
#endif

#include "acm_codec_database.h"
#include "acm_common_defs.h"
#include "acm_dtmf_detection.h"
#include "acm_generic_codec.h"
#include "acm_resampler.h"
#include "critical_section_wrapper.h"
#include "engine_configurations.h"
#include "rw_lock_wrapper.h"
#include "trace.h"

namespace webrtc {

enum {
  kACMToneEnd = 999
};


enum {
  kMaxPacketSize = 2560
};

AudioCodingModuleImpl::AudioCodingModuleImpl(const WebRtc_Word32 id)
    : _packetizationCallback(NULL),
      _id(id),
      _lastTimestamp(0),
      _lastInTimestamp(0),
      _cng_nb_pltype(255),
      _cng_wb_pltype(255),
      _cng_swb_pltype(255),
      _red_pltype(255),
      _vadEnabled(false),
      _dtxEnabled(false),
      _vadMode(VADNormal),
      _stereoReceiveRegistered(false),
      _stereoSend(false),
      _prev_received_channel(0),
      _expected_channels(1),
      _currentSendCodecIdx(-1),
      _current_receive_codec_idx(-1),
      _sendCodecRegistered(false),
      _acmCritSect(CriticalSectionWrapper::CreateCriticalSection()),
      _vadCallback(NULL),
      _lastRecvAudioCodecPlType(255),
      _isFirstRED(true),
      _fecEnabled(false),
      _fragmentation(NULL),
      _lastFECTimestamp(0),
      _receiveREDPayloadType(255),
      _previousPayloadType(255),
      _dummyRTPHeader(NULL),
      _recvPlFrameSizeSmpls(0),
      _receiverInitialized(false),
      _dtmfDetector(NULL),
      _dtmfCallback(NULL),
      _lastDetectedTone(kACMToneEnd),
      _callbackCritSect(CriticalSectionWrapper::CreateCriticalSection()) {
  _lastTimestamp = 0xD87F3F9F;
  _lastInTimestamp = 0xD87F3F9F;

  
  
  memset(&_sendCodecInst, 0, sizeof(CodecInst));
  strncpy(_sendCodecInst.plname, "noCodecRegistered", 31);
  _sendCodecInst.pltype = -1;

  for (int i = 0; i < ACMCodecDB::kMaxNumCodecs; i++) {
    _codecs[i] = NULL;
    _registeredPlTypes[i] = -1;
    _stereoReceive[i] = false;
    _slaveCodecs[i] = NULL;
    _mirrorCodecIdx[i] = -1;
  }

  _netEq.SetUniqueId(_id);

  
  _redBuffer = new WebRtc_UWord8[MAX_PAYLOAD_SIZE_BYTE];
  _fragmentation = new RTPFragmentationHeader;
  _fragmentation->fragmentationVectorSize = 2;
  _fragmentation->fragmentationOffset = new WebRtc_UWord32[2];
  _fragmentation->fragmentationLength = new WebRtc_UWord32[2];
  _fragmentation->fragmentationTimeDiff = new WebRtc_UWord16[2];
  _fragmentation->fragmentationPlType = new WebRtc_UWord8[2];

  
  
  for (int i = (ACMCodecDB::kNumCodecs - 1); i >= 0; i--) {
    if (IsCodecRED(i)) {
      _red_pltype = static_cast<uint8_t>(ACMCodecDB::database_[i].pltype);
    } else if (IsCodecCN(i)) {
      if (ACMCodecDB::database_[i].plfreq == 8000) {
        _cng_nb_pltype = static_cast<uint8_t>(ACMCodecDB::database_[i].pltype);
      } else if (ACMCodecDB::database_[i].plfreq == 16000) {
        _cng_wb_pltype = static_cast<uint8_t>(ACMCodecDB::database_[i].pltype);
      } else if (ACMCodecDB::database_[i].plfreq == 32000) {
        _cng_swb_pltype = static_cast<uint8_t>(ACMCodecDB::database_[i].pltype);
      }
    }
  }

  if (InitializeReceiverSafe() < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                 "Cannot initialize reciever");
  }
#ifdef ACM_QA_TEST
  char file_name[500];
  sprintf(file_name, "ACM_QA_incomingPL_%03d_%d%d%d%d%d%d.dat", _id,
          rand() % 10, rand() % 10, rand() % 10, rand() % 10, rand() % 10,
          rand() % 10);
  _incomingPL = fopen(file_name, "wb");
  sprintf(file_name, "ACM_QA_outgoingPL_%03d_%d%d%d%d%d%d.dat", _id,
          rand() % 10, rand() % 10, rand() % 10, rand() % 10, rand() % 10,
          rand() % 10);
  _outgoingPL = fopen(file_name, "wb");
#endif

  WEBRTC_TRACE(webrtc::kTraceMemory, webrtc::kTraceAudioCoding, id, "Created");
}

AudioCodingModuleImpl::~AudioCodingModuleImpl() {
  {
    CriticalSectionScoped lock(_acmCritSect);
    _currentSendCodecIdx = -1;

    for (int i = 0; i < ACMCodecDB::kMaxNumCodecs; i++) {
      if (_codecs[i] != NULL) {
        
        
        
        if (_slaveCodecs[i] == _codecs[i]) {
          _slaveCodecs[i] = NULL;
        }

        
        assert(_mirrorCodecIdx[i] > -1);
        if (_codecs[_mirrorCodecIdx[i]] != NULL) {
          delete _codecs[_mirrorCodecIdx[i]];
          _codecs[_mirrorCodecIdx[i]] = NULL;
        }

        _codecs[i] = NULL;
      }

      if (_slaveCodecs[i] != NULL) {
        
        assert(_mirrorCodecIdx[i] > -1);
        if (_slaveCodecs[_mirrorCodecIdx[i]] != NULL) {
          delete _slaveCodecs[_mirrorCodecIdx[i]];
          _slaveCodecs[_mirrorCodecIdx[i]] = NULL;
        }
        _slaveCodecs[i] = NULL;
      }
    }

    if (_dtmfDetector != NULL) {
      delete _dtmfDetector;
      _dtmfDetector = NULL;
    }
    if (_dummyRTPHeader != NULL) {
      delete _dummyRTPHeader;
      _dummyRTPHeader = NULL;
    }
    if (_redBuffer != NULL) {
      delete[] _redBuffer;
      _redBuffer = NULL;
    }
    if (_fragmentation != NULL) {
      
      
      delete _fragmentation;
      _fragmentation = NULL;
    }
  }

#ifdef ACM_QA_TEST
  if(_incomingPL != NULL) {
    fclose(_incomingPL);
  }

  if(_outgoingPL != NULL) {
    fclose(_outgoingPL);
  }
#endif

  delete _callbackCritSect;
  _callbackCritSect = NULL;

  delete _acmCritSect;
  _acmCritSect = NULL;
  WEBRTC_TRACE(webrtc::kTraceMemory, webrtc::kTraceAudioCoding, _id,
               "Destroyed");
}

WebRtc_Word32 AudioCodingModuleImpl::ChangeUniqueId(const WebRtc_Word32 id) {
  {
    CriticalSectionScoped lock(_acmCritSect);
    _id = id;

#ifdef ACM_QA_TEST
    if (_incomingPL != NULL) {
      fclose (_incomingPL);
    }
    if (_outgoingPL != NULL) {
      fclose (_outgoingPL);
    }
    char fileName[500];
    sprintf(fileName, "ACM_QA_incomingPL_%03d_%d%d%d%d%d%d.dat", _id,
            rand() % 10, rand() % 10, rand() % 10, rand() % 10, rand() % 10,
            rand() % 10);
    _incomingPL = fopen(fileName, "wb");
    sprintf(fileName, "ACM_QA_outgoingPL_%03d_%d%d%d%d%d%d.dat", _id,
            rand() % 10, rand() % 10, rand() % 10, rand() % 10, rand() % 10,
            rand() % 10);
    _outgoingPL = fopen(fileName, "wb");
#endif

    for (int i = 0; i < ACMCodecDB::kMaxNumCodecs; i++) {
      if (_codecs[i] != NULL) {
        _codecs[i]->SetUniqueID(id);
      }
    }
  }

  _netEq.SetUniqueId(_id);
  return 0;
}



WebRtc_Word32 AudioCodingModuleImpl::TimeUntilNextProcess() {
  CriticalSectionScoped lock(_acmCritSect);

  if (!HaveValidEncoder("TimeUntilNextProcess")) {
    return -1;
  }
  return _codecs[_currentSendCodecIdx]->SamplesLeftToEncode() /
      (_sendCodecInst.plfreq / 1000);
}


WebRtc_Word32 AudioCodingModuleImpl::Process() {
  
  WebRtc_UWord8 stream[2 * MAX_PAYLOAD_SIZE_BYTE];
  WebRtc_Word16 length_bytes = 2 * MAX_PAYLOAD_SIZE_BYTE;
  WebRtc_Word16 red_length_bytes = length_bytes;
  WebRtc_UWord32 rtp_timestamp;
  WebRtc_Word16 status;
  WebRtcACMEncodingType encoding_type;
  FrameType frame_type = kAudioFrameSpeech;
  WebRtc_UWord8 current_payload_type = 0;
  bool has_data_to_send = false;
  bool fec_active = false;

  
  {
    CriticalSectionScoped lock(_acmCritSect);
    if (!HaveValidEncoder("Process")) {
      return -1;
    }

    status = _codecs[_currentSendCodecIdx]->Encode(stream, &length_bytes,
                                                   &rtp_timestamp,
                                                   &encoding_type);
    if (status < 0) {
      
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                   "Process(): Encoding Failed");
      length_bytes = 0;
      return -1;
    } else if (status == 0) {
      
      return 0;
    } else {
      switch (encoding_type) {
        case kNoEncoding: {
          current_payload_type = _previousPayloadType;
          frame_type = kFrameEmpty;
          length_bytes = 0;
          break;
        }
        case kActiveNormalEncoded:
        case kPassiveNormalEncoded: {
          current_payload_type = (WebRtc_UWord8) _sendCodecInst.pltype;
          frame_type = kAudioFrameSpeech;
          break;
        }
        case kPassiveDTXNB: {
          current_payload_type = _cng_nb_pltype;
          frame_type = kAudioFrameCN;
          _isFirstRED = true;
          break;
        }
        case kPassiveDTXWB: {
          current_payload_type = _cng_wb_pltype;
          frame_type = kAudioFrameCN;
          _isFirstRED = true;
          break;
        }
        case kPassiveDTXSWB: {
          current_payload_type = _cng_swb_pltype;
          frame_type = kAudioFrameCN;
          _isFirstRED = true;
          break;
        }
      }
      has_data_to_send = true;
      _previousPayloadType = current_payload_type;

      
      
      
      if ((_fecEnabled) &&
          ((encoding_type == kActiveNormalEncoded) ||
              (encoding_type == kPassiveNormalEncoded))) {
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        fec_active = true;

        has_data_to_send = false;
        
        if (!_isFirstRED) {
          
          
          memcpy(stream + _fragmentation->fragmentationOffset[1], _redBuffer,
                 _fragmentation->fragmentationLength[1]);
          
          
          WebRtc_UWord16 time_since_last = WebRtc_UWord16(
              rtp_timestamp - _lastFECTimestamp);

          
          _fragmentation->fragmentationPlType[1] = _fragmentation
              ->fragmentationPlType[0];
          _fragmentation->fragmentationTimeDiff[1] = time_since_last;
          has_data_to_send = true;
        }

        
        _fragmentation->fragmentationLength[0] = length_bytes;

        
        _fragmentation->fragmentationPlType[0] = current_payload_type;
        _lastFECTimestamp = rtp_timestamp;

        
        red_length_bytes = length_bytes;

        
        
        
        
        length_bytes = static_cast<WebRtc_Word16>(
            _fragmentation->fragmentationLength[0] +
            _fragmentation->fragmentationLength[1]);

        
        
        
        
        if (_codecs[_currentSendCodecIdx]->GetRedPayload(
            _redBuffer,
            &red_length_bytes) == -1) {
          
          
          memcpy(_redBuffer, stream, red_length_bytes);
        }

        _isFirstRED = false;
        
        current_payload_type = _red_pltype;
      }
    }
  }

  if (has_data_to_send) {
    CriticalSectionScoped lock(_callbackCritSect);
#ifdef ACM_QA_TEST
    if(_outgoingPL != NULL) {
      if (fwrite(&rtp_timestamp, sizeof(WebRtc_UWord32), 1, _outgoingPL) != 1) {
        return -1;
      }
      if (fwrite(&current_payload_type, sizeof(WebRtc_UWord8),
                 1, _outgoingPL) != 1) {
        return -1;
      }
      if (fwrite(&length_bytes, sizeof(WebRtc_Word16), 1, _outgoingPL) != 1) {
        return -1;
      }
    }
#endif

    if (_packetizationCallback != NULL) {
      if (fec_active) {
        
        _packetizationCallback->SendData(frame_type, current_payload_type,
                                         rtp_timestamp, stream, length_bytes,
                                         _fragmentation);
      } else {
        
        _packetizationCallback->SendData(frame_type, current_payload_type,
                                         rtp_timestamp, stream, length_bytes,
                                         NULL);
      }
    }

    if (_vadCallback != NULL) {
      
      _vadCallback->InFrameType(((WebRtc_Word16) encoding_type));
    }
  }
  if (fec_active) {
    
    _fragmentation->fragmentationLength[1] = red_length_bytes;
  }
  return length_bytes;
}






WebRtc_Word32 AudioCodingModuleImpl::InitializeSender() {
  CriticalSectionScoped lock(_acmCritSect);

  
  _sendCodecRegistered = false;
  _currentSendCodecIdx = -1;
  _sendCodecInst.plname[0] = '\0';

  
  for (int id = 0; id < ACMCodecDB::kMaxNumCodecs; id++) {
    if (_codecs[id] != NULL) {
      _codecs[id]->DestructEncoder();
    }
  }

  
  _isFirstRED = true;
  if (_fecEnabled) {
    if (_redBuffer != NULL) {
      memset(_redBuffer, 0, MAX_PAYLOAD_SIZE_BYTE);
    }
    if (_fragmentation != NULL) {
      _fragmentation->fragmentationVectorSize = 2;
      _fragmentation->fragmentationOffset[0] = 0;
      _fragmentation->fragmentationOffset[0] = MAX_PAYLOAD_SIZE_BYTE;
      memset(_fragmentation->fragmentationLength, 0,
             sizeof(WebRtc_UWord32) * 2);
      memset(_fragmentation->fragmentationTimeDiff, 0,
             sizeof(WebRtc_UWord16) * 2);
      memset(_fragmentation->fragmentationPlType, 0, sizeof(WebRtc_UWord8) * 2);
    }
  }

  return 0;
}

WebRtc_Word32 AudioCodingModuleImpl::ResetEncoder() {
  CriticalSectionScoped lock(_acmCritSect);
  if (!HaveValidEncoder("ResetEncoder")) {
    return -1;
  }
  return _codecs[_currentSendCodecIdx]->ResetEncoder();
}

void AudioCodingModuleImpl::UnregisterSendCodec() {
  CriticalSectionScoped lock(_acmCritSect);
  _sendCodecRegistered = false;
  _currentSendCodecIdx = -1;
  return;
}

ACMGenericCodec* AudioCodingModuleImpl::CreateCodec(const CodecInst& codec) {
  ACMGenericCodec* my_codec = NULL;

  my_codec = ACMCodecDB::CreateCodecInstance(&codec);
  if (my_codec == NULL) {
    
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                 "ACMCodecDB::CreateCodecInstance() failed in CreateCodec()");
    return my_codec;
  }
  my_codec->SetUniqueID(_id);
  my_codec->SetNetEqDecodeLock(_netEq.DecodeLock());

  return my_codec;
}


WebRtc_Word32 AudioCodingModuleImpl::RegisterSendCodec(
    const CodecInst& send_codec) {
  if ((send_codec.channels != 1) && (send_codec.channels != 2)) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                 "Registering Send codec failed due to wrong number of "
                 "channels, %d. Only mono codecs are supported, i.e. "
                 "channels=1.", send_codec.channels);
    return -1;
  }

  char error_message[500];
  int mirror_id;
  int codec_id = ACMCodecDB::CodecNumber(&send_codec, &mirror_id, error_message,
                                        sizeof(error_message));
  CriticalSectionScoped lock(_acmCritSect);

  
  if (codec_id < 0) {
    if (!_sendCodecRegistered) {
      
      _currentSendCodecIdx = -1;
    }
    
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                 error_message);
    return -1;
  }

  
  if (!STR_CASE_CMP(send_codec.plname, "telephone-event")) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                 "telephone-event cannot be registered as send codec");
    return -1;
  }

  
  
  if (IsCodecRED(&send_codec)) {
    
    
    
    if (!ACMCodecDB::ValidPayloadType(send_codec.pltype)) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                   "Invalid payload-type %d for %s.", send_codec.pltype,
                   send_codec.plname);
      return -1;
    }
    
    _red_pltype = static_cast<uint8_t>(send_codec.pltype);
    return 0;
  }

  
  
  if (IsCodecCN(&send_codec)) {
    
    switch (send_codec.plfreq) {
      case 8000: {
        _cng_nb_pltype = static_cast<uint8_t>(send_codec.pltype);
        break;
      }
      case 16000: {
        _cng_wb_pltype = static_cast<uint8_t>(send_codec.pltype);
        break;
      }
      case 32000: {
        _cng_swb_pltype = static_cast<uint8_t>(send_codec.pltype);
        break;
      }
      default: {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                     "RegisterSendCodec() failed, invalid frequency for CNG "
                     "registration");
        return -1;
      }
    }

    return 0;
  }

  
  
  
  if (!ACMCodecDB::ValidPayloadType(send_codec.pltype)) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                 "Invalid payload-type %d for %s.", send_codec.pltype,
                 send_codec.plname);
    return -1;
  }

  
  if (ACMCodecDB::codec_settings_[codec_id].channel_support
      < send_codec.channels) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                 "%d number of channels not supportedn for %s.",
                 send_codec.channels, send_codec.plname);
    return -1;
  }

  
  if (send_codec.channels == 2) {
    _stereoSend = true;
    if (_vadEnabled || _dtxEnabled) {
      WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceAudioCoding, _id,
                   "VAD/DTX is turned off, not supported when sending stereo.");
    }
    _vadEnabled = false;
    _dtxEnabled = false;
  } else {
    _stereoSend = false;
  }

  
  bool is_send_codec;
  if (_sendCodecRegistered) {
    int send_codec_mirror_id;
    int send_codec_id = ACMCodecDB::CodecNumber(&_sendCodecInst,
                                              &send_codec_mirror_id);
    assert(send_codec_id >= 0);
    is_send_codec = (send_codec_id == codec_id) ||
        (mirror_id == send_codec_mirror_id);
  } else {
    is_send_codec = false;
  }

  
  if (!is_send_codec) {
    if (_codecs[mirror_id] == NULL) {

      _codecs[mirror_id] = CreateCodec(send_codec);
      if (_codecs[mirror_id] == NULL) {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                     "Cannot Create the codec");
        return -1;
      }
      _mirrorCodecIdx[mirror_id] = mirror_id;
    }

    if (mirror_id != codec_id) {
      _codecs[codec_id] = _codecs[mirror_id];
      _mirrorCodecIdx[codec_id] = mirror_id;
    }

    ACMGenericCodec* codec_ptr = _codecs[codec_id];
    WebRtc_Word16 status;
    WebRtcACMCodecParams codec_params;

    memcpy(&(codec_params.codecInstant), &send_codec, sizeof(CodecInst));
    codec_params.enableVAD = _vadEnabled;
    codec_params.enableDTX = _dtxEnabled;
    codec_params.vadMode = _vadMode;
    
    status = codec_ptr->InitEncoder(&codec_params, true);

    
    if (status == 1) {
      _vadEnabled = true;
    } else if (status < 0) {
      

      
      
      if (!_sendCodecRegistered) {
        _currentSendCodecIdx = -1;
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                     "Cannot Initialize the encoder No Encoder is registered");
      } else {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                     "Cannot Initialize the encoder, continue encoding with "
                     "the previously registered codec");
      }
      return -1;
    }

    
    if (_sendCodecRegistered) {
      
      
      _isFirstRED = true;

      if (codec_ptr->SetVAD(_dtxEnabled, _vadEnabled, _vadMode) < 0) {
        
        _vadEnabled = false;
        _dtxEnabled = false;
      }
    }

    _currentSendCodecIdx = codec_id;
    _sendCodecRegistered = true;
    memcpy(&_sendCodecInst, &send_codec, sizeof(CodecInst));
    _previousPayloadType = _sendCodecInst.pltype;
    return 0;
  } else {
    
    
    
    bool force_init = false;

    if (mirror_id != codec_id) {
      _codecs[codec_id] = _codecs[mirror_id];
      _mirrorCodecIdx[codec_id] = mirror_id;
    }

    
    if (send_codec.pltype != _sendCodecInst.pltype) {
      
      
      
      if (!ACMCodecDB::ValidPayloadType(send_codec.pltype)) {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                     "Out of range payload type");
        return -1;
      }
    }

    
    
    
    
    
    if (_sendCodecInst.plfreq != send_codec.plfreq) {
      force_init = true;

      
      _isFirstRED = true;
    }

    
    
    if (_sendCodecInst.pacsize != send_codec.pacsize) {
      force_init = true;
    }
    if (_sendCodecInst.channels != send_codec.channels) {
      force_init = true;
    }

    if (force_init) {
      WebRtcACMCodecParams codec_params;

      memcpy(&(codec_params.codecInstant), &send_codec, sizeof(CodecInst));
      codec_params.enableVAD = _vadEnabled;
      codec_params.enableDTX = _dtxEnabled;
      codec_params.vadMode = _vadMode;

      
      if (_codecs[_currentSendCodecIdx]->InitEncoder(&codec_params, true) < 0) {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                     "Could not change the codec packet-size.");
        return -1;
      }

      _sendCodecInst.plfreq = send_codec.plfreq;
      _sendCodecInst.pacsize = send_codec.pacsize;
      _sendCodecInst.channels = send_codec.channels;
    }

    
    
    _sendCodecInst.pltype = send_codec.pltype;

    
    if (send_codec.rate != _sendCodecInst.rate) {
      if (_codecs[codec_id]->SetBitRate(send_codec.rate) < 0) {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                     "Could not change the codec rate.");
        return -1;
      }
      _sendCodecInst.rate = send_codec.rate;
    }
    _previousPayloadType = _sendCodecInst.pltype;

    return 0;
  }
}


WebRtc_Word32 AudioCodingModuleImpl::SendCodec(
    CodecInst& current_codec) const {
  WEBRTC_TRACE(webrtc::kTraceStream, webrtc::kTraceAudioCoding, _id,
               "SendCodec()");
  CriticalSectionScoped lock(_acmCritSect);

  if (!_sendCodecRegistered) {
    WEBRTC_TRACE(webrtc::kTraceStream, webrtc::kTraceAudioCoding, _id,
                 "SendCodec Failed, no codec is registered");

    return -1;
  }
  WebRtcACMCodecParams encoder_param;
  _codecs[_currentSendCodecIdx]->EncoderParams(&encoder_param);
  encoder_param.codecInstant.pltype = _sendCodecInst.pltype;
  memcpy(&current_codec, &(encoder_param.codecInstant), sizeof(CodecInst));

  return 0;
}


WebRtc_Word32 AudioCodingModuleImpl::SendFrequency() const {
  WEBRTC_TRACE(webrtc::kTraceStream, webrtc::kTraceAudioCoding, _id,
               "SendFrequency()");
  CriticalSectionScoped lock(_acmCritSect);

  if (!_sendCodecRegistered) {
    WEBRTC_TRACE(webrtc::kTraceStream, webrtc::kTraceAudioCoding, _id,
                 "SendFrequency Failed, no codec is registered");

    return -1;
  }

  return _sendCodecInst.plfreq;
}




WebRtc_Word32 AudioCodingModuleImpl::SendBitrate() const {
  CriticalSectionScoped lock(_acmCritSect);

  if (!_sendCodecRegistered) {
    WEBRTC_TRACE(webrtc::kTraceStream, webrtc::kTraceAudioCoding, _id,
                 "SendBitrate Failed, no codec is registered");

    return -1;
  }

  WebRtcACMCodecParams encoder_param;
  _codecs[_currentSendCodecIdx]->EncoderParams(&encoder_param);

  return encoder_param.codecInstant.rate;
}



WebRtc_Word32 AudioCodingModuleImpl::SetReceivedEstimatedBandwidth(
    const WebRtc_Word32 bw) {
  return _codecs[_currentSendCodecIdx]->SetEstimatedBandwidth(bw);
}



WebRtc_Word32 AudioCodingModuleImpl::RegisterTransportCallback(
    AudioPacketizationCallback* transport) {
  CriticalSectionScoped lock(_callbackCritSect);
  _packetizationCallback = transport;
  return 0;
}



WebRtc_Word32 AudioCodingModuleImpl::RegisterIncomingMessagesCallback(
#ifndef WEBRTC_DTMF_DETECTION
    AudioCodingFeedback* ,
    const ACMCountries ) {
  return -1;
#else
    AudioCodingFeedback* incoming_message,
    const ACMCountries cpt) {
  WebRtc_Word16 status = 0;

  
  {
    CriticalSectionScoped lock(_callbackCritSect);
    _dtmfCallback = incoming_message;
  }
  
  {
    CriticalSectionScoped lock(_acmCritSect);
    
    if (incoming_message == NULL) {
      
      if (_dtmfDetector != NULL) {
        delete _dtmfDetector;
        _dtmfDetector = NULL;
      }
      status = 0;
    } else {
      status = 0;
      if (_dtmfDetector == NULL) {
        _dtmfDetector = new (ACMDTMFDetection);
        if (_dtmfDetector == NULL) {
          status = -1;
        }
      }
      if (status >= 0) {
        status = _dtmfDetector->Enable(cpt);
        if (status < 0) {
          
          
          delete _dtmfDetector;
          _dtmfDetector = NULL;
        }
      }
    }
  }
  
  if ((status < 0)) {
    
    CriticalSectionScoped lock(_callbackCritSect);
    _dtmfCallback = NULL;
  }

  return status;
#endif
}


WebRtc_Word32 AudioCodingModuleImpl::Add10MsData(
    const AudioFrame& audio_frame) {
  
  CriticalSectionScoped lock(_acmCritSect);
  if (!HaveValidEncoder("Add10MsData")) {
    return -1;
  }

  if (audio_frame.samples_per_channel_ == 0) {
    assert(false);
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                 "Cannot Add 10 ms audio, payload length is zero");
    return -1;
  }
  
  if ((audio_frame.sample_rate_hz_ != 8000)
      && (audio_frame.sample_rate_hz_ != 16000)
      && (audio_frame.sample_rate_hz_ != 32000)
      && (audio_frame.sample_rate_hz_ != 48000)) {
    assert(false);
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                 "Cannot Add 10 ms audio, input frequency not valid");
    return -1;
  }

  
  if ((audio_frame.sample_rate_hz_ / 100)
      != audio_frame.samples_per_channel_) {
    WEBRTC_TRACE(
        webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
        "Cannot Add 10 ms audio, input frequency and length doesn't match");
    return -1;
  }

  
  
  
  bool resample = ((WebRtc_Word32) audio_frame.sample_rate_hz_
      != _sendCodecInst.plfreq);

  
  
  WebRtc_Word16 audio[WEBRTC_10MS_PCM_AUDIO];
  int audio_channels = _sendCodecInst.channels;
  
  
  if (audio_frame.num_channels_ != audio_channels) {
    if (audio_channels == 2) {
      
      for (int k = 0; k < audio_frame.samples_per_channel_; k++) {
        audio[k * 2] = audio_frame.data_[k];
        audio[(k * 2) + 1] = audio_frame.data_[k];
      }
    } else if (audio_channels == 1) {
      
      
      for (int k = 0; k < audio_frame.samples_per_channel_; k++) {
        audio[k] = (audio_frame.data_[k * 2]
            + audio_frame.data_[(k * 2) + 1]) >> 1;
      }
    }
  } else {
    
    size_t length = static_cast<size_t>(audio_frame.samples_per_channel_
        * audio_channels);
    memcpy(audio, audio_frame.data_, length * sizeof(WebRtc_UWord16));
  }

  WebRtc_UWord32 current_timestamp;
  WebRtc_Word32 status;
  
  if (resample) {
    WebRtc_Word16 resampled_audio[WEBRTC_10MS_PCM_AUDIO];
    WebRtc_Word32 send_freq = _sendCodecInst.plfreq;
    WebRtc_UWord32 timestamp_diff;
    WebRtc_Word16 new_length;

    
    if (_lastInTimestamp > audio_frame.timestamp_) {
      
      timestamp_diff = ((WebRtc_UWord32) 0xFFFFFFFF - _lastInTimestamp)
          + audio_frame.timestamp_;
    } else {
      timestamp_diff = audio_frame.timestamp_ - _lastInTimestamp;
    }
    current_timestamp = _lastTimestamp + (WebRtc_UWord32)(timestamp_diff *
        ((double) _sendCodecInst.plfreq / (double) audio_frame.sample_rate_hz_));

    new_length = _inputResampler.Resample10Msec(audio,
                                                audio_frame.sample_rate_hz_,
                                                resampled_audio, send_freq,
                                                audio_channels);

    if (new_length < 0) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                   "Cannot add 10 ms audio, resmapling failed");
      return -1;
    }
    status = _codecs[_currentSendCodecIdx]->Add10MsData(current_timestamp,
                                                        resampled_audio,
                                                        new_length,
                                                        audio_channels);
  } else {
    current_timestamp = audio_frame.timestamp_;

    status = _codecs[_currentSendCodecIdx]->Add10MsData(
        current_timestamp, audio, audio_frame.samples_per_channel_,
        audio_channels);
  }
  _lastInTimestamp = audio_frame.timestamp_;
  _lastTimestamp = current_timestamp;
  return status;
}





bool AudioCodingModuleImpl::FECStatus() const {
  CriticalSectionScoped lock(_acmCritSect);
  return _fecEnabled;
}


WebRtc_Word32
AudioCodingModuleImpl::SetFECStatus(
#ifdef WEBRTC_CODEC_RED
    const bool enable_fec) {
  CriticalSectionScoped lock(_acmCritSect);

  if (_fecEnabled != enable_fec) {
    
    memset(_redBuffer, 0, MAX_PAYLOAD_SIZE_BYTE);

    
    _fragmentation->fragmentationVectorSize = 2;
    _fragmentation->fragmentationOffset[0] = 0;
    _fragmentation->fragmentationOffset[1] = MAX_PAYLOAD_SIZE_BYTE;
    memset(_fragmentation->fragmentationLength, 0, sizeof(WebRtc_UWord32) * 2);
    memset(_fragmentation->fragmentationTimeDiff, 0,
           sizeof(WebRtc_UWord16) * 2);
    memset(_fragmentation->fragmentationPlType, 0, sizeof(WebRtc_UWord8) * 2);

    
    _fecEnabled = enable_fec;
  }
  _isFirstRED = true;  
  return 0;
#else
    const bool /* enable_fec */) {
  _fecEnabled = false;
  WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceAudioCoding, _id,
               "  WEBRTC_CODEC_RED is undefined => _fecEnabled = %d",
               _fecEnabled);
  return -1;
#endif
}





WebRtc_Word32 AudioCodingModuleImpl::SetVAD(const bool enable_dtx,
                                            const bool enable_vad,
                                            const ACMVADMode mode) {
  CriticalSectionScoped lock(_acmCritSect);

  
  if ((mode != VADNormal) && (mode != VADLowBitrate)
      && (mode != VADAggr) && (mode != VADVeryAggr)) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                 "Invalid VAD Mode %d, no change is made to VAD/DTX status",
                 (int) mode);
    return -1;
  }

  
  
  if ((enable_dtx || enable_vad) && _stereoSend) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                 "VAD/DTX not supported for stereo sending");
    return -1;
  }

  
  if (HaveValidEncoder("SetVAD")) {
    WebRtc_Word16 status = _codecs[_currentSendCodecIdx]->SetVAD(enable_dtx,
                                                                 enable_vad,
                                                                 mode);
    if (status == 1) {
      
      _vadEnabled = true;
      _dtxEnabled = enable_dtx;
      _vadMode = mode;

      return 0;
    } else if (status < 0) {
      
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                   "SetVAD failed");

      _vadEnabled = false;
      _dtxEnabled = false;

      return -1;
    }
  }

  _vadEnabled = enable_vad;
  _dtxEnabled = enable_dtx;
  _vadMode = mode;

  return 0;
}


WebRtc_Word32 AudioCodingModuleImpl::VAD(bool& dtx_enabled, bool& vad_enabled,
                                         ACMVADMode& mode) const {
  CriticalSectionScoped lock(_acmCritSect);

  dtx_enabled = _dtxEnabled;
  vad_enabled = _vadEnabled;
  mode = _vadMode;

  return 0;
}





WebRtc_Word32 AudioCodingModuleImpl::InitializeReceiver() {
  CriticalSectionScoped lock(_acmCritSect);
  return InitializeReceiverSafe();
}


WebRtc_Word32 AudioCodingModuleImpl::InitializeReceiverSafe() {
  
  
  
  if (_receiverInitialized) {
    for (int i = 0; i < ACMCodecDB::kNumCodecs; i++) {
      if (UnregisterReceiveCodecSafe(i) < 0) {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                     "InitializeReceiver() failed, Could not unregister codec");
        return -1;
      }
    }
  }
  if (_netEq.Init() != 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                 "InitializeReceiver() failed, Could not initialize NetEQ");
    return -1;
  }
  _netEq.SetUniqueId(_id);
  if (_netEq.AllocatePacketBuffer(ACMCodecDB::NetEQDecoders(),
                                  ACMCodecDB::kNumCodecs) != 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                 "NetEQ cannot allocatePacket Buffer");
    return -1;
  }

  
  for (int i = 0; i < ACMCodecDB::kNumCodecs; i++) {
    if (IsCodecRED(i) || IsCodecCN(i)) {
      if (RegisterRecCodecMSSafe(ACMCodecDB::database_[i], i, i,
                                 ACMNetEQ::masterJB) < 0) {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                     "Cannot register master codec.");
        return -1;
      }
      _registeredPlTypes[i] = ACMCodecDB::database_[i].pltype;
    }
  }

  _receiverInitialized = true;
  return 0;
}


WebRtc_Word32 AudioCodingModuleImpl::ResetDecoder() {
  CriticalSectionScoped lock(_acmCritSect);

  for (int id = 0; id < ACMCodecDB::kMaxNumCodecs; id++) {
    if ((_codecs[id] != NULL) && (_registeredPlTypes[id] != -1)) {
      if (_codecs[id]->ResetDecoder(_registeredPlTypes[id]) < 0) {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                     "ResetDecoder failed:");
        return -1;
      }
    }
  }
  return _netEq.FlushBuffers();
}


WebRtc_Word32 AudioCodingModuleImpl::ReceiveFrequency() const {
  WEBRTC_TRACE(webrtc::kTraceStream, webrtc::kTraceAudioCoding, _id,
               "ReceiveFrequency()");
  WebRtcACMCodecParams codec_params;

  CriticalSectionScoped lock(_acmCritSect);
  if (DecoderParamByPlType(_lastRecvAudioCodecPlType, codec_params) < 0) {
    return _netEq.CurrentSampFreqHz();
  } else {
    return codec_params.codecInstant.plfreq;
  }
}


WebRtc_Word32 AudioCodingModuleImpl::PlayoutFrequency() const {
  WEBRTC_TRACE(webrtc::kTraceStream, webrtc::kTraceAudioCoding, _id,
               "PlayoutFrequency()");

  CriticalSectionScoped lock(_acmCritSect);

  return _netEq.CurrentSampFreqHz();
}



WebRtc_Word32 AudioCodingModuleImpl::RegisterReceiveCodec(
    const CodecInst& receive_codec) {
  CriticalSectionScoped lock(_acmCritSect);

  if (receive_codec.channels > 2) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                 "More than 2 audio channel is not supported.");
    return -1;
  }

  int mirror_id;
  int codec_id = ACMCodecDB::ReceiverCodecNumber(&receive_codec, &mirror_id);

  if (codec_id < 0 || codec_id >= ACMCodecDB::kNumCodecs) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                 "Wrong codec params to be registered as receive codec");
    return -1;
  }
  
  if (!ACMCodecDB::ValidPayloadType(receive_codec.pltype)) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                 "Invalid payload-type %d for %s.", receive_codec.pltype,
                 receive_codec.plname);
    return -1;
  }

  if (!_receiverInitialized) {
    if (InitializeReceiverSafe() < 0) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                   "Cannot initialize reciver, so failed registering a codec.");
      return -1;
    }
  }

  
  
  if ((_registeredPlTypes[codec_id] == receive_codec.pltype)
      && IsCodecCN(&receive_codec)) {
    
    
    return 0;
  } else if (_registeredPlTypes[codec_id] != -1) {
    if (UnregisterReceiveCodecSafe(codec_id) < 0) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                   "Cannot register master codec.");
      return -1;
    }
  }

  if (RegisterRecCodecMSSafe(receive_codec, codec_id, mirror_id,
                             ACMNetEQ::masterJB) < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                 "Cannot register master codec.");
    return -1;
  }

  
  
  
  

  
  
  if (receive_codec.channels == 2 ||
      (_stereoReceiveRegistered && (IsCodecCN(&receive_codec) ||
          IsCodecRED(&receive_codec)))) {
    

    if (!_stereoReceiveRegistered) {
      
      

      
      assert(_netEq.NumSlaves() == 0);
      if (_netEq.AddSlave(ACMCodecDB::NetEQDecoders(),
                          ACMCodecDB::kNumCodecs) < 0) {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                     "Cannot add slave jitter buffer to NetEQ.");
        return -1;
      }

      
      for (int i = 0; i < ACMCodecDB::kNumCodecs; i++) {
        if (_registeredPlTypes[i] != -1 && (IsCodecRED(i) || IsCodecCN(i))) {
          _stereoReceive[i] = true;

          CodecInst codec;
          memcpy(&codec, &ACMCodecDB::database_[i], sizeof(CodecInst));
          codec.pltype = _registeredPlTypes[i];
          if (RegisterRecCodecMSSafe(codec, i, i, ACMNetEQ::slaveJB) < 0) {
            WEBRTC_TRACE(kTraceError, kTraceAudioCoding, _id,
                         "Cannot register slave codec.");
            return -1;
          }
        }
      }
    }

    if (RegisterRecCodecMSSafe(receive_codec, codec_id, mirror_id,
                               ACMNetEQ::slaveJB) < 0) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                   "Cannot register slave codec.");
      return -1;
    }

    if (!_stereoReceive[codec_id]
        && (_lastRecvAudioCodecPlType == receive_codec.pltype)) {
      
      
      _lastRecvAudioCodecPlType = -1;
    }

    _stereoReceive[codec_id] = true;
    _stereoReceiveRegistered = true;
  } else {
    _stereoReceive[codec_id] = false;
  }

  _registeredPlTypes[codec_id] = receive_codec.pltype;

  if (IsCodecRED(&receive_codec)) {
    _receiveREDPayloadType = receive_codec.pltype;
  }
  return 0;
}

WebRtc_Word32 AudioCodingModuleImpl::RegisterRecCodecMSSafe(
    const CodecInst& receive_codec, WebRtc_Word16 codec_id,
    WebRtc_Word16 mirror_id, ACMNetEQ::JB jitter_buffer) {
  ACMGenericCodec** codecs;
  if (jitter_buffer == ACMNetEQ::masterJB) {
    codecs = &_codecs[0];
  } else if (jitter_buffer == ACMNetEQ::slaveJB) {
    codecs = &_slaveCodecs[0];
    if (_codecs[codec_id]->IsTrueStereoCodec()) {
      
      
      _slaveCodecs[mirror_id] = _codecs[mirror_id];
      _mirrorCodecIdx[mirror_id] = mirror_id;
    }
  } else {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                 "RegisterReceiveCodecMSSafe failed, jitter_buffer is neither "
                 "master or slave ");
    return -1;
  }

  if (codecs[mirror_id] == NULL) {
    codecs[mirror_id] = CreateCodec(receive_codec);
    if (codecs[mirror_id] == NULL) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                   "Cannot create codec to register as receive codec");
      return -1;
    }
    _mirrorCodecIdx[mirror_id] = mirror_id;
  }
  if (mirror_id != codec_id) {
    codecs[codec_id] = codecs[mirror_id];
    _mirrorCodecIdx[codec_id] = mirror_id;
  }

  codecs[codec_id]->SetIsMaster(jitter_buffer == ACMNetEQ::masterJB);

  WebRtc_Word16 status = 0;
  WebRtcACMCodecParams codec_params;
  memcpy(&(codec_params.codecInstant), &receive_codec, sizeof(CodecInst));
  codec_params.enableVAD = false;
  codec_params.enableDTX = false;
  codec_params.vadMode = VADNormal;
  if (!codecs[codec_id]->DecoderInitialized()) {
    
    status = codecs[codec_id]->InitDecoder(&codec_params, true);
    if (status < 0) {
      
      
      WEBRTC_TRACE(
          webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
          "could not initialize the receive codec, codec not registered");

      return -1;
    }
  } else if (mirror_id != codec_id) {
    
    
    codecs[codec_id]->SaveDecoderParam(&codec_params);
  }

  if (codecs[codec_id]->RegisterInNetEq(&_netEq, receive_codec) != 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                 "Receive codec could not be registered in NetEQ");
      return -1;
  }
  
  
  codecs[codec_id]->SaveDecoderParam(&codec_params);

  return status;
}


WebRtc_Word32 AudioCodingModuleImpl::ReceiveCodec(
    CodecInst& current_codec) const {
  WebRtcACMCodecParams decoderParam;
  CriticalSectionScoped lock(_acmCritSect);

  for (int id = 0; id < ACMCodecDB::kMaxNumCodecs; id++) {
    if (_codecs[id] != NULL) {
      if (_codecs[id]->DecoderInitialized()) {
        if (_codecs[id]->DecoderParams(&decoderParam,
                                       _lastRecvAudioCodecPlType)) {
          memcpy(&current_codec, &decoderParam.codecInstant, sizeof(CodecInst));
          return 0;
        }
      }
    }
  }

  
  
  current_codec.pltype = -1;
  return -1;
}


WebRtc_Word32 AudioCodingModuleImpl::IncomingPacket(
    const WebRtc_UWord8* incoming_payload,
    const WebRtc_Word32 payload_length,
    const WebRtcRTPHeader& rtp_info) {
  WebRtcRTPHeader rtp_header;

  memcpy(&rtp_header, &rtp_info, sizeof(WebRtcRTPHeader));

  if (payload_length < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                 "IncomingPacket() Error, payload-length cannot be negative");
    return -1;
  }
  {
    
    
    CriticalSectionScoped lock(_acmCritSect);
#ifdef ACM_QA_TEST
    if(_incomingPL != NULL) {
      if (fwrite(&rtp_info.header.timestamp, sizeof(WebRtc_UWord32),
                 1, _incomingPL) != 1) {
        return -1;
      }
      if (fwrite(&rtp_info.header.payloadType, sizeof(WebRtc_UWord8),
                 1, _incomingPL) != 1) {
        return -1;
      }
      if (fwrite(&payload_length, sizeof(WebRtc_Word16),
                 1, _incomingPL) != 1) {
        return -1;
      }
    }
#endif

    WebRtc_UWord8 myPayloadType;

    
    if (rtp_info.header.payloadType == _receiveREDPayloadType) {
      
      myPayloadType = incoming_payload[0] & 0x7F;
    } else {
      myPayloadType = rtp_info.header.payloadType;
    }

    
    
    if (!rtp_info.type.Audio.isCNG) {
      

      if (myPayloadType != _lastRecvAudioCodecPlType) {
        
        
        
        
        for (int i = 0; i < ACMCodecDB::kMaxNumCodecs; i++) {
          if (_registeredPlTypes[i] == myPayloadType) {
            if (UpdateUponReceivingCodec(i) != 0)
              return -1;
            break;
          }
        }
      }
      _lastRecvAudioCodecPlType = myPayloadType;
    }
  }

  
  
  if (_expected_channels == 2) {
    if (!rtp_info.type.Audio.isCNG) {
      
      WebRtc_Word32 length = payload_length;
      WebRtc_UWord8 payload[kMaxPacketSize];
      assert(payload_length <= kMaxPacketSize);
      memcpy(payload, incoming_payload, payload_length);
      _codecs[_current_receive_codec_idx]->SplitStereoPacket(payload, &length);
      rtp_header.type.Audio.channel = 2;
      
      return _netEq.RecIn(payload, length, rtp_header);
    } else {
      
      
      return 0;
    }
  } else {
    return _netEq.RecIn(incoming_payload, payload_length, rtp_header);
  }
}

int AudioCodingModuleImpl::UpdateUponReceivingCodec(int index) {
  if (_codecs[index] == NULL) {
    WEBRTC_TRACE(kTraceError, kTraceAudioCoding, _id,
        "IncomingPacket() error: payload type found but corresponding codec "
        "is NULL");
    return -1;
  }
  _codecs[index]->UpdateDecoderSampFreq(index);
  _netEq.SetReceivedStereo(_stereoReceive[index]);
  _current_receive_codec_idx = index;

  
  
  if ((_stereoReceive[index] && (_expected_channels == 1)) ||
      (!_stereoReceive[index] && (_expected_channels == 2))) {
    _netEq.FlushBuffers();
    _codecs[index]->ResetDecoder(_registeredPlTypes[index]);
  }

  if (_stereoReceive[index] && (_expected_channels == 1)) {
    
    if (InitStereoSlave() != 0)
      return -1;
  }

  
  if (_stereoReceive[index]) {
    _expected_channels = 2;
  } else {
    _expected_channels = 1;
  }

  
  _prev_received_channel = 0;
  return 0;
}

bool AudioCodingModuleImpl::IsCodecForSlave(int index) const {
  return (_registeredPlTypes[index] != -1 && _stereoReceive[index]);
}

bool AudioCodingModuleImpl::IsCodecRED(int index) const {
  return (IsCodecRED(&ACMCodecDB::database_[index]));
}

bool AudioCodingModuleImpl::IsCodecRED(const CodecInst* codec) const {
  return (STR_CASE_CMP(codec->plname, "RED") == 0);
}

bool AudioCodingModuleImpl::IsCodecCN(int index) const {
  return (IsCodecCN(&ACMCodecDB::database_[index]));
}

bool AudioCodingModuleImpl::IsCodecCN(const CodecInst* codec) const {
  return (STR_CASE_CMP(codec->plname, "CN") == 0);
}

int AudioCodingModuleImpl::InitStereoSlave() {
  _netEq.RemoveSlaves();

  if (_netEq.AddSlave(ACMCodecDB::NetEQDecoders(),
                      ACMCodecDB::kNumCodecs) < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                 "Cannot add slave jitter buffer to NetEQ.");
    return -1;
  }

  
  for (int i = 0; i < ACMCodecDB::kNumCodecs; i++) {
    if (_codecs[i] != NULL && IsCodecForSlave(i)) {
      WebRtcACMCodecParams decoder_params;
      if (_codecs[i]->DecoderParams(&decoder_params, _registeredPlTypes[i])) {
        if (RegisterRecCodecMSSafe(decoder_params.codecInstant,
                                   i, ACMCodecDB::MirrorID(i),
                                   ACMNetEQ::slaveJB) < 0) {
            WEBRTC_TRACE(kTraceError, kTraceAudioCoding, _id,
                         "Cannot register slave codec.");
            return -1;
        }
      }
    }
  }
  return 0;
}


WebRtc_Word32 AudioCodingModuleImpl::SetMinimumPlayoutDelay(
    const WebRtc_Word32 time_ms) {
  if ((time_ms < 0) || (time_ms > 1000)) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                 "Delay must be in the range of 0-1000 milliseconds.");
    return -1;
  }
  return _netEq.SetExtraDelay(time_ms);
}


bool AudioCodingModuleImpl::DtmfPlayoutStatus() const {
#ifndef WEBRTC_CODEC_AVT
  return false;
#else
  return _netEq.AVTPlayout();
#endif
}



WebRtc_Word32 AudioCodingModuleImpl::SetDtmfPlayoutStatus(
#ifndef WEBRTC_CODEC_AVT
    const bool ) {
  WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceAudioCoding, _id,
               "SetDtmfPlayoutStatus() failed: AVT is not supported.");
  return -1;
#else
    const bool enable) {
  return _netEq.SetAVTPlayout(enable);
#endif
}




WebRtc_Word32 AudioCodingModuleImpl::DecoderEstimatedBandwidth() const {
  CodecInst codec;
  WebRtc_Word16 codec_id = -1;
  int payloadtype_wb;
  int payloadtype_swb;

  
  for (int id = 0; id < ACMCodecDB::kNumCodecs; id++) {
    
    ACMCodecDB::Codec(id, &codec);

    if (!STR_CASE_CMP(codec.plname, "isac")) {
      codec_id = 1;
      payloadtype_wb = codec.pltype;

      ACMCodecDB::Codec(id + 1, &codec);
      payloadtype_swb = codec.pltype;

      break;
    }
  }

  if (codec_id < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                 "DecoderEstimatedBandwidth failed");
    return -1;
  }

  if ((_lastRecvAudioCodecPlType == payloadtype_wb) ||
      (_lastRecvAudioCodecPlType == payloadtype_swb)) {
    return _codecs[codec_id]->GetEstimatedBandwidth();
  } else {
    return -1;
  }
}


WebRtc_Word32 AudioCodingModuleImpl::SetPlayoutMode(
    const AudioPlayoutMode mode) {
  if ((mode != voice) && (mode != fax) && (mode != streaming)) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                 "Invalid playout mode.");
    return -1;
  }
  return _netEq.SetPlayoutMode(mode);
}


AudioPlayoutMode AudioCodingModuleImpl::PlayoutMode() const {
  return _netEq.PlayoutMode();
}



WebRtc_Word32 AudioCodingModuleImpl::PlayoutData10Ms(
    const WebRtc_Word32 desired_freq_hz, AudioFrame& audio_frame) {
  bool stereo_mode;

  
  if (_netEq.RecOut(_audioFrame) != 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                 "PlayoutData failed, RecOut Failed");
    return -1;
  }

  audio_frame.num_channels_ = _audioFrame.num_channels_;
  audio_frame.vad_activity_ = _audioFrame.vad_activity_;
  audio_frame.speech_type_ = _audioFrame.speech_type_;

  stereo_mode = (_audioFrame.num_channels_ > 1);
  
  

  const WebRtc_UWord16 receive_freq =
      static_cast<WebRtc_UWord16>(_audioFrame.sample_rate_hz_);
  bool tone_detected = false;
  WebRtc_Word16 last_detected_tone;
  WebRtc_Word16 tone;

  
  {
    CriticalSectionScoped lock(_acmCritSect);

    if ((receive_freq != desired_freq_hz) && (desired_freq_hz != -1)) {
      
      WebRtc_Word16 temp_len = _outputResampler.Resample10Msec(
          _audioFrame.data_, receive_freq, audio_frame.data_,
          desired_freq_hz, _audioFrame.num_channels_);

      if (temp_len < 0) {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                     "PlayoutData failed, resampler failed");
        return -1;
      }

      
      audio_frame.samples_per_channel_ = (WebRtc_UWord16) temp_len;
      
      audio_frame.sample_rate_hz_ = desired_freq_hz;
    } else {
      memcpy(audio_frame.data_, _audioFrame.data_,
             _audioFrame.samples_per_channel_ * audio_frame.num_channels_
             * sizeof(WebRtc_Word16));
      
      audio_frame.samples_per_channel_ =
          _audioFrame.samples_per_channel_;
      
      audio_frame.sample_rate_hz_ = receive_freq;
    }

    
    if (_dtmfDetector != NULL) {
      
      if (audio_frame.sample_rate_hz_ == 8000) {
        
        
        if (!stereo_mode) {
          _dtmfDetector->Detect(audio_frame.data_,
                                audio_frame.samples_per_channel_,
                                audio_frame.sample_rate_hz_, tone_detected,
                                tone);
        } else {
          
          WebRtc_Word16 master_channel[80];
          for (int n = 0; n < 80; n++) {
            master_channel[n] = audio_frame.data_[n << 1];
          }
          _dtmfDetector->Detect(master_channel,
                                audio_frame.samples_per_channel_,
                                audio_frame.sample_rate_hz_, tone_detected,
                                tone);
        }
      } else {
        
        if (!stereo_mode) {
          _dtmfDetector->Detect(_audioFrame.data_,
                                _audioFrame.samples_per_channel_,
                                receive_freq, tone_detected, tone);
        } else {
          WebRtc_Word16 master_channel[WEBRTC_10MS_PCM_AUDIO];
          for (int n = 0; n < _audioFrame.samples_per_channel_; n++) {
            master_channel[n] = _audioFrame.data_[n << 1];
          }
          _dtmfDetector->Detect(master_channel,
                                _audioFrame.samples_per_channel_,
                                receive_freq, tone_detected, tone);
        }
      }
    }

    
    
    
    last_detected_tone = kACMToneEnd;
    if (tone_detected) {
      last_detected_tone = _lastDetectedTone;
      _lastDetectedTone = tone;
    }
  }

  if (tone_detected) {
    
    CriticalSectionScoped lock(_callbackCritSect);

    if (_dtmfCallback != NULL) {
      if (tone != kACMToneEnd) {
        
        _dtmfCallback->IncomingDtmf((WebRtc_UWord8) tone, false);
      } else if ((tone == kACMToneEnd) && (last_detected_tone != kACMToneEnd)) {
        
        
        _dtmfCallback->IncomingDtmf((WebRtc_UWord8) last_detected_tone, true);
      }
    }
  }

  audio_frame.id_ = _id;
  audio_frame.energy_ = -1;
  audio_frame.timestamp_ = 0;

  return 0;
}







ACMVADMode AudioCodingModuleImpl::ReceiveVADMode() const {
  return _netEq.VADMode();
}


WebRtc_Word16 AudioCodingModuleImpl::SetReceiveVADMode(const ACMVADMode mode) {
  return _netEq.SetVADMode(mode);
}





WebRtc_Word32 AudioCodingModuleImpl::NetworkStatistics(
    ACMNetworkStatistics& statistics) const {
  WebRtc_Word32 status;
  status = _netEq.NetworkStatistics(&statistics);
  return status;
}


void AudioCodingModuleImpl::DestructEncoderInst(void* inst) {
  WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceAudioCoding, _id,
               "DestructEncoderInst()");
  if (!HaveValidEncoder("DestructEncoderInst")) {
    return;
  }

  _codecs[_currentSendCodecIdx]->DestructEncoderInst(inst);
}

WebRtc_Word16 AudioCodingModuleImpl::AudioBuffer(
    WebRtcACMAudioBuff& buffer) {
  WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceAudioCoding, _id,
               "AudioBuffer()");
  if (!HaveValidEncoder("AudioBuffer")) {
    return -1;
  }
  buffer.lastInTimestamp = _lastInTimestamp;
  return _codecs[_currentSendCodecIdx]->AudioBuffer(buffer);
}

WebRtc_Word16 AudioCodingModuleImpl::SetAudioBuffer(
    WebRtcACMAudioBuff& buffer) {
  WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceAudioCoding, _id,
               "SetAudioBuffer()");
  if (!HaveValidEncoder("SetAudioBuffer")) {
    return -1;
  }
  return _codecs[_currentSendCodecIdx]->SetAudioBuffer(buffer);
}

WebRtc_UWord32 AudioCodingModuleImpl::EarliestTimestamp() const {
  WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceAudioCoding, _id,
               "EarliestTimestamp()");
  if (!HaveValidEncoder("EarliestTimestamp")) {
    return -1;
  }
  return _codecs[_currentSendCodecIdx]->EarliestTimestamp();
}

WebRtc_Word32 AudioCodingModuleImpl::RegisterVADCallback(
    ACMVADCallback* vad_callback) {
  WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceAudioCoding, _id,
               "RegisterVADCallback()");
  CriticalSectionScoped lock(_callbackCritSect);
  _vadCallback = vad_callback;
  return 0;
}


WebRtc_Word32 AudioCodingModuleImpl::IncomingPayload(
    const WebRtc_UWord8* incoming_payload, const WebRtc_Word32 payload_length,
    const WebRtc_UWord8 payload_type, const WebRtc_UWord32 timestamp) {
  if (payload_length < 0) {
    
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                 "IncomingPacket() Error, payload-length cannot be negative");
    return -1;
  }

  if (_dummyRTPHeader == NULL) {
    
    
    WebRtcACMCodecParams codec_params;
    _dummyRTPHeader = new WebRtcRTPHeader;
    if (_dummyRTPHeader == NULL) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                   "IncomingPacket() Error, out of memory");
      return -1;
    }
    _dummyRTPHeader->header.payloadType = payload_type;
    
    _dummyRTPHeader->header.ssrc = 0;
    _dummyRTPHeader->header.markerBit = false;
    
    _dummyRTPHeader->header.sequenceNumber = rand();
    _dummyRTPHeader->header.timestamp = (((WebRtc_UWord32) rand()) << 16)
        + (WebRtc_UWord32) rand();
    _dummyRTPHeader->type.Audio.channel = 1;

    if (DecoderParamByPlType(payload_type, codec_params) < 0) {
      
      
      
      delete _dummyRTPHeader;
      _dummyRTPHeader = NULL;
      return -1;
    }
    _recvPlFrameSizeSmpls = codec_params.codecInstant.pacsize;
  }

  if (payload_type != _dummyRTPHeader->header.payloadType) {
    
    
    WebRtcACMCodecParams codec_params;
    if (DecoderParamByPlType(payload_type, codec_params) < 0) {
      
      return -1;
    }
    _recvPlFrameSizeSmpls = codec_params.codecInstant.pacsize;
    _dummyRTPHeader->header.payloadType = payload_type;
  }

  if (timestamp > 0) {
    _dummyRTPHeader->header.timestamp = timestamp;
  }

  
  
  _lastRecvAudioCodecPlType = payload_type;

  
  if (_netEq.RecIn(incoming_payload, payload_length, (*_dummyRTPHeader)) < 0) {
    return -1;
  }

  
  _dummyRTPHeader->header.sequenceNumber++;
  _dummyRTPHeader->header.timestamp += _recvPlFrameSizeSmpls;
  return 0;
}

WebRtc_Word16 AudioCodingModuleImpl::DecoderParamByPlType(
    const WebRtc_UWord8 payload_type,
    WebRtcACMCodecParams& codec_params) const {
  CriticalSectionScoped lock(_acmCritSect);
  for (WebRtc_Word16 id = 0; id < ACMCodecDB::kMaxNumCodecs;
      id++) {
    if (_codecs[id] != NULL) {
      if (_codecs[id]->DecoderInitialized()) {
        if (_codecs[id]->DecoderParams(&codec_params, payload_type)) {
          return 0;
        }
      }
    }
  }
  
  
  
  codec_params.codecInstant.plname[0] = '\0';
  codec_params.codecInstant.pacsize = 0;
  codec_params.codecInstant.rate = 0;
  codec_params.codecInstant.pltype = -1;
  return -1;
}

WebRtc_Word16 AudioCodingModuleImpl::DecoderListIDByPlName(
    const char* name, const WebRtc_UWord16 frequency) const {
  WebRtcACMCodecParams codec_params;
  CriticalSectionScoped lock(_acmCritSect);
  for (WebRtc_Word16 id = 0; id < ACMCodecDB::kMaxNumCodecs; id++) {
    if ((_codecs[id] != NULL)) {
      if (_codecs[id]->DecoderInitialized()) {
        assert(_registeredPlTypes[id] >= 0);
        assert(_registeredPlTypes[id] <= 255);
        _codecs[id]->DecoderParams(
            &codec_params, (WebRtc_UWord8) _registeredPlTypes[id]);
        if (!STR_CASE_CMP(codec_params.codecInstant.plname, name)) {
          
          
          
          
          
          
          if ((frequency == 0)||
              (codec_params.codecInstant.plfreq == frequency)) {
            return id;
          }
        }
      }
    }
  }
  
  
  return -1;
}

WebRtc_Word32 AudioCodingModuleImpl::LastEncodedTimestamp(
    WebRtc_UWord32& timestamp) const {
  CriticalSectionScoped lock(_acmCritSect);
  if (!HaveValidEncoder("LastEncodedTimestamp")) {
    return -1;
  }
  timestamp = _codecs[_currentSendCodecIdx]->LastEncodedTimestamp();
  return 0;
}

WebRtc_Word32 AudioCodingModuleImpl::ReplaceInternalDTXWithWebRtc(
    bool use_webrtc_dtx) {
  CriticalSectionScoped lock(_acmCritSect);

  if (!HaveValidEncoder("ReplaceInternalDTXWithWebRtc")) {
    WEBRTC_TRACE(
        webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
        "Cannot replace codec internal DTX when no send codec is registered.");
    return -1;
  }

  WebRtc_Word32 res = _codecs[_currentSendCodecIdx]->ReplaceInternalDTX(
      use_webrtc_dtx);
  
  if (res == 1) {
    _vadEnabled = true;
  } else if (res < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                 "Failed to set ReplaceInternalDTXWithWebRtc(%d)",
                 use_webrtc_dtx);
    return res;
  }

  return 0;
}

WebRtc_Word32 AudioCodingModuleImpl::IsInternalDTXReplacedWithWebRtc(
    bool& uses_webrtc_dtx) {
  CriticalSectionScoped lock(_acmCritSect);

  if (!HaveValidEncoder("IsInternalDTXReplacedWithWebRtc")) {
    return -1;
  }
  if (_codecs[_currentSendCodecIdx]->IsInternalDTXReplaced(&uses_webrtc_dtx)
      < 0) {
    return -1;
  }
  return 0;
}

WebRtc_Word32 AudioCodingModuleImpl::SetISACMaxRate(
    const WebRtc_UWord32 max_bit_per_sec) {
  CriticalSectionScoped lock(_acmCritSect);

  if (!HaveValidEncoder("SetISACMaxRate")) {
    return -1;
  }

  return _codecs[_currentSendCodecIdx]->SetISACMaxRate(max_bit_per_sec);
}

WebRtc_Word32 AudioCodingModuleImpl::SetISACMaxPayloadSize(
    const WebRtc_UWord16 max_size_bytes) {
  CriticalSectionScoped lock(_acmCritSect);

  if (!HaveValidEncoder("SetISACMaxPayloadSize")) {
    return -1;
  }

  return _codecs[_currentSendCodecIdx]->SetISACMaxPayloadSize(
      max_size_bytes);
}

WebRtc_Word32 AudioCodingModuleImpl::ConfigISACBandwidthEstimator(
    const WebRtc_UWord8 frame_size_ms,
    const WebRtc_UWord16 rate_bit_per_sec,
    const bool enforce_frame_size) {
  CriticalSectionScoped lock(_acmCritSect);

  if (!HaveValidEncoder("ConfigISACBandwidthEstimator")) {
    return -1;
  }

  return _codecs[_currentSendCodecIdx]->ConfigISACBandwidthEstimator(
      frame_size_ms, rate_bit_per_sec, enforce_frame_size);
}

WebRtc_Word32 AudioCodingModuleImpl::SetBackgroundNoiseMode(
    const ACMBackgroundNoiseMode mode) {
  if ((mode < On) || (mode > Off)) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                 "The specified background noise is out of range.\n");
    return -1;
  }
  return _netEq.SetBackgroundNoiseMode(mode);
}

WebRtc_Word32 AudioCodingModuleImpl::BackgroundNoiseMode(
    ACMBackgroundNoiseMode& mode) {
  return _netEq.BackgroundNoiseMode(mode);
}

WebRtc_Word32 AudioCodingModuleImpl::PlayoutTimestamp(
    WebRtc_UWord32& timestamp) {
  WEBRTC_TRACE(webrtc::kTraceStream, webrtc::kTraceAudioCoding, _id,
               "PlayoutTimestamp()");
  return _netEq.PlayoutTimestamp(timestamp);
}

bool AudioCodingModuleImpl::HaveValidEncoder(const char* caller_name) const {
  if ((!_sendCodecRegistered) || (_currentSendCodecIdx < 0) ||
      (_currentSendCodecIdx >= ACMCodecDB::kNumCodecs)) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                 "%s failed: No send codec is registered.", caller_name);
    return false;
  }
  if ((_currentSendCodecIdx < 0) ||
      (_currentSendCodecIdx >= ACMCodecDB::kNumCodecs)) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                 "%s failed: Send codec index out of range.", caller_name);
    return false;
  }
  if (_codecs[_currentSendCodecIdx] == NULL) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                 "%s failed: Send codec is NULL pointer.", caller_name);
    return false;
  }
  return true;
}

WebRtc_Word32 AudioCodingModuleImpl::UnregisterReceiveCodec(
    const WebRtc_Word16 payload_type) {
  CriticalSectionScoped lock(_acmCritSect);
  int id;

  
  for (id = 0; id < ACMCodecDB::kMaxNumCodecs; id++) {
    if (_registeredPlTypes[id] == payload_type) {
      
      break;
    }
  }

  if (id >= ACMCodecDB::kNumCodecs) {
    
    return 0;
  }

  
  return UnregisterReceiveCodecSafe(id);
}

WebRtc_Word32 AudioCodingModuleImpl::UnregisterReceiveCodecSafe(
    const WebRtc_Word16 codec_id) {
  const WebRtcNetEQDecoder *neteq_decoder = ACMCodecDB::NetEQDecoders();
  WebRtc_Word16 mirror_id = ACMCodecDB::MirrorID(codec_id);
  bool stereo_receiver = false;

  if (_codecs[codec_id] != NULL) {
    if (_registeredPlTypes[codec_id] != -1) {
      
      stereo_receiver = _stereoReceive[codec_id];

      
      if (_netEq.RemoveCodec(neteq_decoder[codec_id],
                             _stereoReceive[codec_id])  < 0) {
        CodecInst codec;
        ACMCodecDB::Codec(codec_id, &codec);
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                     "Unregistering %s-%d from NetEQ failed.", codec.plname,
                     codec.plfreq);
        return -1;
      }

      
      
      if (IsCodecCN(codec_id)) {
        for (int i = 0; i < ACMCodecDB::kNumCodecs; i++) {
          if (IsCodecCN(i)) {
            _stereoReceive[i] = false;
            _registeredPlTypes[i] = -1;
          }
        }
      } else {
        if (codec_id == mirror_id) {
          _codecs[codec_id]->DestructDecoder();
          if (_stereoReceive[codec_id]) {
            _slaveCodecs[codec_id]->DestructDecoder();
            _stereoReceive[codec_id] = false;

          }
        }
      }

      
      if (stereo_receiver) {
        bool no_stereo = true;

        for (int i = 0; i < ACMCodecDB::kNumCodecs; i++) {
          if (_stereoReceive[i]) {
            
            no_stereo = false;
            break;
          }
        }

        
        if (no_stereo) {
          _netEq.RemoveSlaves();  
          _stereoReceiveRegistered = false;
        }
      }
    }
  }

  if (_registeredPlTypes[codec_id] == _receiveREDPayloadType) {
    
    _receiveREDPayloadType = 255;
  }
  _registeredPlTypes[codec_id] = -1;

  return 0;
}

WebRtc_Word32 AudioCodingModuleImpl::REDPayloadISAC(
    const WebRtc_Word32 isac_rate, const WebRtc_Word16 isac_bw_estimate,
    WebRtc_UWord8* payload, WebRtc_Word16* length_bytes) {
  if (!HaveValidEncoder("EncodeData")) {
    return -1;
  }
  WebRtc_Word16 status;
  status = _codecs[_currentSendCodecIdx]->REDPayloadISAC(isac_rate,
                                                         isac_bw_estimate,
                                                         payload,
                                                         length_bytes);
  return status;
}

}  
