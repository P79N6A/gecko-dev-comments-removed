
















#include "webrtc/modules/audio_coding/main/source/acm_codec_database.h"

#include <stdio.h>

#include "webrtc/modules/audio_coding/main/source/acm_common_defs.h"
#include "webrtc/system_wrappers/interface/trace.h"



#include "webrtc/modules/audio_coding/codecs/g711/include/g711_interface.h"
#include "webrtc/modules/audio_coding/main/source/acm_pcma.h"
#include "webrtc/modules/audio_coding/main/source/acm_pcmu.h"

#include "webrtc/modules/audio_coding/codecs/cng/include/webrtc_cng.h"
#include "webrtc/modules/audio_coding/main/source/acm_cng.h"

#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq.h"
#ifdef WEBRTC_CODEC_ISAC
#include "webrtc/modules/audio_coding/codecs/isac/main/interface/isac.h"
#include "webrtc/modules/audio_coding/main/source/acm_isac.h"
#include "webrtc/modules/audio_coding/main/source/acm_isac_macros.h"
#endif
#ifdef WEBRTC_CODEC_ISACFX
#include "webrtc/modules/audio_coding/codecs/isac/fix/interface/isacfix.h"
#include "webrtc/modules/audio_coding/main/source/acm_isac.h"
#include "webrtc/modules/audio_coding/main/source/acm_isac_macros.h"
#endif
#ifdef WEBRTC_CODEC_PCM16
#include "webrtc/modules/audio_coding/codecs/pcm16b/include/pcm16b.h"
#include "webrtc/modules/audio_coding/main/source/acm_pcm16b.h"
#endif
#ifdef WEBRTC_CODEC_ILBC
#include "webrtc/modules/audio_coding/codecs/ilbc/interface/ilbc.h"
#include "webrtc/modules/audio_coding/main/source/acm_ilbc.h"
#endif
#ifdef WEBRTC_CODEC_AMR
#include "amr_interface.h"
#include "webrtc/modules/audio_coding/main/source/acm_amr.h"
#endif
#ifdef WEBRTC_CODEC_AMRWB
#include "amrwb_interface.h"
#include "webrtc/modules/audio_coding/main/source/acm_amrwb.h"
#endif
#ifdef WEBRTC_CODEC_CELT
#include "celt_interface.h"
#include "webrtc/modules/audio_coding/main/source/acm_celt.h"
#endif
#ifdef WEBRTC_CODEC_G722
#include "webrtc/modules/audio_coding/codecs/g722/include/g722_interface.h"
#include "webrtc/modules/audio_coding/main/source/acm_g722.h"
#endif
#ifdef WEBRTC_CODEC_G722_1
#include "g7221_interface.h"
#include "webrtc/modules/audio_coding/main/source/acm_g7221.h"
#endif
#ifdef WEBRTC_CODEC_G722_1C
#include "g7221c_interface.h"
#include "webrtc/modules/audio_coding/main/source/acm_g7221c.h"
#endif
#ifdef WEBRTC_CODEC_G729
#include "g729_interface.h"
#include "webrtc/modules/audio_coding/main/source/acm_g729.h"
#endif
#ifdef WEBRTC_CODEC_G729_1
#include "g7291_interface.h"
#include "webrtc/modules/audio_coding/main/source/acm_g7291.h"
#endif
#ifdef WEBRTC_CODEC_GSMFR
#include "gsmfr_interface.h"
#include "webrtc/modules/audio_coding/main/source/acm_gsmfr.h"
#endif
#ifdef WEBRTC_CODEC_OPUS
#include "webrtc/modules/audio_coding/codecs/opus/interface/opus_interface.h"
#include "webrtc/modules/audio_coding/main/source/acm_opus.h"
#endif
#ifdef WEBRTC_CODEC_OPUS
    #include "acm_opus.h"
    #include "opus_interface.h"
#endif
#ifdef WEBRTC_CODEC_SPEEX
#include "speex_interface.h"
#include "webrtc/modules/audio_coding/main/source/acm_speex.h"
#endif
#ifdef WEBRTC_CODEC_AVT
#include "webrtc/modules/audio_coding/main/source/acm_dtmf_playout.h"
#endif
#ifdef WEBRTC_CODEC_RED
#include "webrtc/modules/audio_coding/main/source/acm_red.h"
#endif

namespace webrtc {




const int kDynamicPayloadtypes[ACMCodecDB::kMaxNumCodecs] = {
  107, 108, 109, 111, 112, 113, 114, 115, 116, 117, 92,
  91,  90,  89,  88,  87,  86,  85,  84,  83,  82,  81, 80,
  79,  78,  77,  76,  75,  74,  73,  72,  71,  70,  69, 68,
  67, 66, 65
};





#if (defined(WEBRTC_CODEC_AMR) || defined(WEBRTC_CODEC_AMRWB) || \
    defined(WEBRTC_CODEC_CELT) || defined(WEBRTC_CODEC_G722_1) || \
    defined(WEBRTC_CODEC_G722_1C) || defined(WEBRTC_CODEC_G729_1) || \
    defined(WEBRTC_CODEC_PCM16) || defined(WEBRTC_CODEC_SPEEX))
static int count_database = 0;
#endif

const CodecInst ACMCodecDB::database_[] = {
#if (defined(WEBRTC_CODEC_ISAC) || defined(WEBRTC_CODEC_ISACFX))
  {103, "ISAC", 16000, kIsacPacSize480, 1, kIsacWbDefaultRate},
# if (defined(WEBRTC_CODEC_ISAC))
  {104, "ISAC", 32000, kIsacPacSize960, 1, kIsacSwbDefaultRate},
  {105, "ISAC", 48000, kIsacPacSize1440, 1, kIsacSwbDefaultRate},
# endif
#endif
#ifdef WEBRTC_CODEC_PCM16
  
  {kDynamicPayloadtypes[count_database++], "L16", 8000, 80, 1, 128000},
  {kDynamicPayloadtypes[count_database++], "L16", 16000, 160, 1, 256000},
  {kDynamicPayloadtypes[count_database++], "L16", 32000, 320, 1, 512000},
  
  {kDynamicPayloadtypes[count_database++], "L16", 8000, 80, 2, 128000},
  {kDynamicPayloadtypes[count_database++], "L16", 16000, 160, 2, 256000},
  {kDynamicPayloadtypes[count_database++], "L16", 32000, 320, 2, 512000},
#endif
  
  
  {0, "PCMU", 8000, 160, 1, 64000},
  {8, "PCMA", 8000, 160, 1, 64000},
  
  {110, "PCMU", 8000, 160, 2, 64000},
  {118, "PCMA", 8000, 160, 2, 64000},
#ifdef WEBRTC_CODEC_ILBC
  {102, "ILBC", 8000, 240, 1, 13300},
#endif
#ifdef WEBRTC_CODEC_AMR
  {kDynamicPayloadtypes[count_database++], "AMR", 8000, 160, 1, 12200},
#endif
#ifdef WEBRTC_CODEC_AMRWB
  {kDynamicPayloadtypes[count_database++], "AMR-WB", 16000, 320, 1, 20000},
#endif
#ifdef WEBRTC_CODEC_CELT
  
  {kDynamicPayloadtypes[count_database++], "CELT", 32000, 640, 1, 64000},
  
  {kDynamicPayloadtypes[count_database++], "CELT", 32000, 640, 2, 64000},
#endif
#ifdef WEBRTC_CODEC_G722
  
  {9, "G722", 16000, 320, 1, 64000},
  
  {119, "G722", 16000, 320, 2, 64000},
#endif
#ifdef WEBRTC_CODEC_G722_1
  {kDynamicPayloadtypes[count_database++], "G7221", 16000, 320, 1, 32000},
  {kDynamicPayloadtypes[count_database++], "G7221", 16000, 320, 1, 24000},
  {kDynamicPayloadtypes[count_database++], "G7221", 16000, 320, 1, 16000},
#endif
#ifdef WEBRTC_CODEC_G722_1C
  {kDynamicPayloadtypes[count_database++], "G7221", 32000, 640, 1, 48000},
  {kDynamicPayloadtypes[count_database++], "G7221", 32000, 640, 1, 32000},
  {kDynamicPayloadtypes[count_database++], "G7221", 32000, 640, 1, 24000},
#endif
#ifdef WEBRTC_CODEC_G729
  {18, "G729", 8000, 240, 1, 8000},
#endif
#ifdef WEBRTC_CODEC_G729_1
  {kDynamicPayloadtypes[count_database++], "G7291", 16000, 320, 1, 32000},
#endif
#ifdef WEBRTC_CODEC_GSMFR
  {3, "GSM", 8000, 160, 1, 13200},
#endif
#ifdef WEBRTC_CODEC_OPUS
  
  
  {120, "opus", 48000, 960, 2, 64000},
#endif
#ifdef WEBRTC_CODEC_SPEEX
  {kDynamicPayloadtypes[count_database++], "speex", 8000, 160, 1, 11000},
  {kDynamicPayloadtypes[count_database++], "speex", 16000, 320, 1, 22000},
#endif
  
  {13, "CN", 8000, 240, 1, 0},
  {98, "CN", 16000, 480, 1, 0},
  {99, "CN", 32000, 960, 1, 0},
  {100, "CN", 48000, 1440, 1, 0},
#ifdef WEBRTC_CODEC_AVT
  {106, "telephone-event", 8000, 240, 1, 0},
#endif
#ifdef WEBRTC_CODEC_RED
  {127, "red", 8000, 0, 1, 0},
#endif
  
  {-1, "Null", -1, -1, -1, -1}
};





const ACMCodecDB::CodecSettings ACMCodecDB::codec_settings_[] = {
#if (defined(WEBRTC_CODEC_ISAC) || defined(WEBRTC_CODEC_ISACFX))
    {2, {kIsacPacSize480, kIsacPacSize960}, 0, 1},
# if (defined(WEBRTC_CODEC_ISAC))
    {1, {kIsacPacSize960}, 0, 1},
    {1, {kIsacPacSize1440}, 0, 1},
# endif
#endif
#ifdef WEBRTC_CODEC_PCM16
    
    {4, {80, 160, 240, 320}, 0, 2},
    {4, {160, 320, 480, 640}, 0, 2},
    {2, {320, 640}, 0, 2},
    
    {4, {80, 160, 240, 320}, 0, 2},
    {4, {160, 320, 480, 640}, 0, 2},
    {2, {320, 640}, 0, 2},
#endif
    
    
    {6, {80, 160, 240, 320, 400, 480}, 0, 2},
    {6, {80, 160, 240, 320, 400, 480}, 0, 2},
    
    {6, {80, 160, 240, 320, 400, 480}, 0, 2},
    {6, {80, 160, 240, 320, 400, 480}, 0, 2},
#ifdef WEBRTC_CODEC_ILBC
    {4, {160, 240, 320, 480}, 0, 1},
#endif
#ifdef WEBRTC_CODEC_AMR
    {3, {160, 320, 480}, 0, 1},
#endif
#ifdef WEBRTC_CODEC_AMRWB
    {3, {320, 640, 960}, 0, 1},
#endif
#ifdef WEBRTC_CODEC_CELT
    
    {1, {640}, 0, 2},
    
    {1, {640}, 0, 2},
#endif
#ifdef WEBRTC_CODEC_G722
    
    {6, {160, 320, 480, 640, 800, 960}, 0, 2},
    
    {6, {160, 320, 480, 640, 800, 960}, 0, 2},
#endif
#ifdef WEBRTC_CODEC_G722_1
    {1, {320}, 320, 1},
    {1, {320}, 320, 1},
    {1, {320}, 320, 1},
#endif
#ifdef WEBRTC_CODEC_G722_1C
    {1, {640}, 640, 1},
    {1, {640}, 640, 1},
    {1, {640}, 640, 1},
#endif
#ifdef WEBRTC_CODEC_G729
    {6, {80, 160, 240, 320, 400, 480}, 0, 1},
#endif
#ifdef WEBRTC_CODEC_G729_1
    {3, {320, 640, 960}, 0, 1},
#endif
#ifdef WEBRTC_CODEC_GSMFR
    {3, {160, 320, 480}, 160, 1},
#endif
#ifdef WEBRTC_CODEC_OPUS
    
    
    
    {1, {960}, 0, 2},
#endif
#ifdef WEBRTC_CODEC_OPUS
  
  
  {1, {640}, 0, 2},
#endif
#ifdef WEBRTC_CODEC_SPEEX
    {3, {160, 320, 480}, 0, 1},
    {3, {320, 640, 960}, 0, 1},
#endif
    
    {1, {240}, 240, 1},
    {1, {480}, 480, 1},
    {1, {960}, 960, 1},
    {1, {1440}, 1440, 1},
#ifdef WEBRTC_CODEC_AVT
    {1, {240}, 240, 1},
#endif
#ifdef WEBRTC_CODEC_RED
    {1, {0}, 0, 1},
#endif
    
    {-1, {-1}, -1, -1}
};


const WebRtcNetEQDecoder ACMCodecDB::neteq_decoders_[] = {
#if (defined(WEBRTC_CODEC_ISAC) || defined(WEBRTC_CODEC_ISACFX))
    kDecoderISAC,
# if (defined(WEBRTC_CODEC_ISAC))
    kDecoderISACswb,
    kDecoderISACfb,
# endif
#endif
#ifdef WEBRTC_CODEC_PCM16
    
    kDecoderPCM16B,
    kDecoderPCM16Bwb,
    kDecoderPCM16Bswb32kHz,
    
    kDecoderPCM16B_2ch,
    kDecoderPCM16Bwb_2ch,
    kDecoderPCM16Bswb32kHz_2ch,
#endif
    
    
    kDecoderPCMu,
    kDecoderPCMa,
    
    kDecoderPCMu_2ch,
    kDecoderPCMa_2ch,
#ifdef WEBRTC_CODEC_ILBC
    kDecoderILBC,
#endif
#ifdef WEBRTC_CODEC_AMR
    kDecoderAMR,
#endif
#ifdef WEBRTC_CODEC_AMRWB
    kDecoderAMRWB,
#endif
#ifdef WEBRTC_CODEC_CELT
    
    kDecoderCELT_32,
    
    kDecoderCELT_32_2ch,
#endif
#ifdef WEBRTC_CODEC_G722
    
    kDecoderG722,
    
    kDecoderG722_2ch,
#endif
#ifdef WEBRTC_CODEC_G722_1
    kDecoderG722_1_32,
    kDecoderG722_1_24,
    kDecoderG722_1_16,
#endif
#ifdef WEBRTC_CODEC_G722_1C
    kDecoderG722_1C_48,
    kDecoderG722_1C_32,
    kDecoderG722_1C_24,
#endif
#ifdef WEBRTC_CODEC_G729
    kDecoderG729,
#endif
#ifdef WEBRTC_CODEC_G729_1
    kDecoderG729_1,
#endif
#ifdef WEBRTC_CODEC_GSMFR
    kDecoderGSMFR,
#endif
#ifdef WEBRTC_CODEC_OPUS
    
    kDecoderOpus,
#endif
#ifdef WEBRTC_CODEC_OPUS
  kDecoderOpus,
#endif
#ifdef WEBRTC_CODEC_SPEEX
    kDecoderSPEEX_8,
    kDecoderSPEEX_16,
#endif
    
    kDecoderCNG,
    kDecoderCNG,
    kDecoderCNG,
    kDecoderCNG,
#ifdef WEBRTC_CODEC_AVT
    kDecoderAVT,
#endif
#ifdef WEBRTC_CODEC_RED
    kDecoderRED,
#endif
    kDecoderReservedEnd
};



int ACMCodecDB::Codec(int codec_id, CodecInst* codec_inst) {
  
  if ((codec_id < 0) || (codec_id >= kNumCodecs)) {
    return -1;
  }

  
  memcpy(codec_inst, &database_[codec_id], sizeof(CodecInst));

  return 0;
}


enum {
  kInvalidCodec = -10,
  kInvalidPayloadtype = -30,
  kInvalidPacketSize = -40,
  kInvalidRate = -50
};




int ACMCodecDB::CodecNumber(const CodecInst* codec_inst, int* mirror_id,
                            char* err_message, int max_message_len_byte) {
  int codec_id = ACMCodecDB::CodecNumber(codec_inst, mirror_id);

  
  if ((codec_id < 0) && (err_message != NULL)) {
    char my_err_msg[1000];

    if (codec_id == kInvalidCodec) {
      sprintf(my_err_msg, "Call to ACMCodecDB::CodecNumber failed, Codec not "
              "found");
    } else if (codec_id == kInvalidPayloadtype) {
      sprintf(my_err_msg, "Call to ACMCodecDB::CodecNumber failed, payload "
              "number %d is out of range for %s", codec_inst->pltype,
              codec_inst->plname);
    } else if (codec_id == kInvalidPacketSize) {
      sprintf(my_err_msg, "Call to ACMCodecDB::CodecNumber failed, Packet "
              "size is out of range for %s", codec_inst->plname);
    } else if (codec_id == kInvalidRate) {
      sprintf(my_err_msg, "Call to ACMCodecDB::CodecNumber failed, rate=%d "
              "is not a valid rate for %s", codec_inst->rate,
              codec_inst->plname);
    } else {
      
      sprintf(my_err_msg, "invalid codec parameters to be registered, "
              "ACMCodecDB::CodecNumber failed");
    }

    strncpy(err_message, my_err_msg, max_message_len_byte - 1);
    
    err_message[max_message_len_byte - 1] = '\0';
  }

  return codec_id;
}




int ACMCodecDB::CodecNumber(const CodecInst* codec_inst, int* mirror_id) {
  
  int codec_id = CodecId(codec_inst);

  
  if (codec_id == -1) {
    return kInvalidCodec;
  }

  
  if (!ValidPayloadType(codec_inst->pltype)) {
    return kInvalidPayloadtype;
  }

  
  if (STR_CASE_CMP(database_[codec_id].plname, "CN") == 0) {
    *mirror_id = codec_id;
    return codec_id;
  }

  
  if (STR_CASE_CMP(database_[codec_id].plname, "red") == 0) {
    *mirror_id = codec_id;
    return codec_id;
  }

  
  if (codec_settings_[codec_id].num_packet_sizes > 0) {
    bool packet_size_ok = false;
    int i;
    int packet_size_samples;
    for (i = 0; i < codec_settings_[codec_id].num_packet_sizes; i++) {
      packet_size_samples =
          codec_settings_[codec_id].packet_sizes_samples[i];
      if (codec_inst->pacsize == packet_size_samples) {
        packet_size_ok = true;
        break;
      }
    }

    if (!packet_size_ok) {
      return kInvalidPacketSize;
    }
  }

  if (codec_inst->pacsize < 1) {
    return kInvalidPacketSize;
  }

  
  
  *mirror_id = codec_id;
  if (STR_CASE_CMP("isac", codec_inst->plname) == 0) {
    if (IsISACRateValid(codec_inst->rate)) {
      
      
      *mirror_id = kISAC;
      return  codec_id;
    } else {
      return kInvalidRate;
    }
  } else if (STR_CASE_CMP("ilbc", codec_inst->plname) == 0) {
    return IsILBCRateValid(codec_inst->rate, codec_inst->pacsize)
        ? codec_id : kInvalidRate;
  } else if (STR_CASE_CMP("amr", codec_inst->plname) == 0) {
    return IsAMRRateValid(codec_inst->rate)
        ? codec_id : kInvalidRate;
  } else if (STR_CASE_CMP("amr-wb", codec_inst->plname) == 0) {
    return IsAMRwbRateValid(codec_inst->rate)
        ? codec_id : kInvalidRate;
  } else if (STR_CASE_CMP("g7291", codec_inst->plname) == 0) {
    return IsG7291RateValid(codec_inst->rate)
        ? codec_id : kInvalidRate;
  } else if (STR_CASE_CMP("opus", codec_inst->plname) == 0) {
    return IsOpusRateValid(codec_inst->rate)
        ? codec_id : kInvalidRate;
  } else if (STR_CASE_CMP("speex", codec_inst->plname) == 0) {
    return IsSpeexRateValid(codec_inst->rate)
        ? codec_id : kInvalidRate;
  } else if (STR_CASE_CMP("celt", codec_inst->plname) == 0) {
    return IsCeltRateValid(codec_inst->rate)
        ? codec_id : kInvalidRate;
  }

  return IsRateValid(codec_id, codec_inst->rate) ?
      codec_id : kInvalidRate;
}






int ACMCodecDB::CodecId(const CodecInst* codec_inst) {
  return (CodecId(codec_inst->plname, codec_inst->plfreq,
                  codec_inst->channels));
}

int ACMCodecDB::CodecId(const char* payload_name, int frequency, int channels) {
  for (int id = 0; id < kNumCodecs; id++) {
    bool name_match = false;
    bool frequency_match = false;
    bool channels_match = false;

    
    
    
    name_match = (STR_CASE_CMP(database_[id].plname, payload_name) == 0);
    frequency_match = (frequency == database_[id].plfreq) || (frequency == -1);
    
    if (STR_CASE_CMP(payload_name, "opus") != 0) {
      channels_match = (channels == database_[id].channels);
    } else {
      
      channels_match = (channels == 1 || channels == 2);
    }

    if (name_match && frequency_match && channels_match) {
      
      return id;
    }
  }

  
  return -1;
}

int ACMCodecDB::ReceiverCodecNumber(const CodecInst* codec_inst,
                                    int* mirror_id) {
  
  int codec_id = CodecId(codec_inst);

  
  
  
  if (STR_CASE_CMP(codec_inst->plname, "ISAC") != 0) {
    *mirror_id = codec_id;
  } else {
    *mirror_id = kISAC;
  }

  return codec_id;
}



int ACMCodecDB::CodecFreq(int codec_id) {
  
  if (codec_id < 0 || codec_id >= kNumCodecs) {
    return -1;
  }

  return database_[codec_id].plfreq;
}


int ACMCodecDB::BasicCodingBlock(int codec_id) {
  
  if (codec_id < 0 || codec_id >= kNumCodecs) {
    return -1;
  }

  return codec_settings_[codec_id].basic_block_samples;
}


const WebRtcNetEQDecoder* ACMCodecDB::NetEQDecoders() {
  return neteq_decoders_;
}



int ACMCodecDB::MirrorID(int codec_id) {
  if (STR_CASE_CMP(database_[codec_id].plname, "isac") == 0) {
    return kISAC;
  } else {
    return codec_id;
  }
}


ACMGenericCodec* ACMCodecDB::CreateCodecInstance(const CodecInst* codec_inst) {
  
  if (!STR_CASE_CMP(codec_inst->plname, "ISAC")) {
#if (defined(WEBRTC_CODEC_ISAC) || defined(WEBRTC_CODEC_ISACFX))
    return new ACMISAC(kISAC);
#endif
  } else if (!STR_CASE_CMP(codec_inst->plname, "PCMU")) {
    if (codec_inst->channels == 1) {
      return new ACMPCMU(kPCMU);
    } else {
      return new ACMPCMU(kPCMU_2ch);
    }
  } else if (!STR_CASE_CMP(codec_inst->plname, "PCMA")) {
    if (codec_inst->channels == 1) {
      return new ACMPCMA(kPCMA);
    } else {
      return new ACMPCMA(kPCMA_2ch);
    }
  } else if (!STR_CASE_CMP(codec_inst->plname, "ILBC")) {
#ifdef WEBRTC_CODEC_ILBC
    return new ACMILBC(kILBC);
#endif
  } else if (!STR_CASE_CMP(codec_inst->plname, "AMR")) {
#ifdef WEBRTC_CODEC_AMR
    return new ACMAMR(kGSMAMR);
#endif
  } else if (!STR_CASE_CMP(codec_inst->plname, "AMR-WB")) {
#ifdef WEBRTC_CODEC_AMRWB
    return new ACMAMRwb(kGSMAMRWB);
#endif
  } else if (!STR_CASE_CMP(codec_inst->plname, "CELT")) {
#ifdef WEBRTC_CODEC_CELT
    if (codec_inst->channels == 1) {
      return new ACMCELT(kCELT32);
    } else {
      return new ACMCELT(kCELT32_2ch);
    }
#endif
  } else if (!STR_CASE_CMP(codec_inst->plname, "G722")) {
#ifdef WEBRTC_CODEC_G722
    if (codec_inst->channels == 1) {
      return new ACMG722(kG722);
    } else {
      return new ACMG722(kG722_2ch);
    }
#endif
  } else if (!STR_CASE_CMP(codec_inst->plname, "G7221")) {
    switch (codec_inst->plfreq) {
      case 16000: {
#ifdef WEBRTC_CODEC_G722_1
        int codec_id;
        switch (codec_inst->rate) {
          case 16000 : {
            codec_id = kG722_1_16;
            break;
          }
          case 24000 : {
            codec_id = kG722_1_24;
            break;
          }
          case 32000 : {
            codec_id = kG722_1_32;
            break;
          }
          default: {
            return NULL;
          }
          return new ACMG722_1(codec_id);
        }
#endif
      }
      case 32000: {
#ifdef WEBRTC_CODEC_G722_1C
        int codec_id;
        switch (codec_inst->rate) {
          case 24000 : {
            codec_id = kG722_1C_24;
            break;
          }
          case 32000 : {
            codec_id = kG722_1C_32;
            break;
          }
          case 48000 : {
            codec_id = kG722_1C_48;
            break;
          }
          default: {
            return NULL;
          }
          return new ACMG722_1C(codec_id);
        }
#endif
      }
    }
  } else if (!STR_CASE_CMP(codec_inst->plname, "CN")) {
    
    int codec_id;
    switch (codec_inst->plfreq) {
      case 8000: {
        codec_id = kCNNB;
        break;
      }
      case 16000: {
        codec_id = kCNWB;
        break;
      }
      case 32000: {
        codec_id = kCNSWB;
        break;
      }
      case 48000: {
        codec_id = kCNFB;
        break;
      }
      default: {
        return NULL;
      }
    }
    return new ACMCNG(codec_id);
  } else if (!STR_CASE_CMP(codec_inst->plname, "G729")) {
#ifdef WEBRTC_CODEC_G729
    return new ACMG729(kG729);
#endif
  } else if (!STR_CASE_CMP(codec_inst->plname, "G7291")) {
#ifdef WEBRTC_CODEC_G729_1
    return new ACMG729_1(kG729_1);
#endif
  } else if (!STR_CASE_CMP(codec_inst->plname, "opus")) {
#ifdef WEBRTC_CODEC_OPUS
    return new ACMOpus(kOpus);
#endif
  } else if (!STR_CASE_CMP(codec_inst->plname, "speex")) {
#ifdef WEBRTC_CODEC_SPEEX
    int codec_id;
    switch (codec_inst->plfreq) {
      case 8000: {
        codec_id = kSPEEX8;
        break;
      }
      case 16000: {
        codec_id = kSPEEX16;
        break;
      }
      default: {
        return NULL;
      }
    }
    return new ACMSPEEX(codec_id);
#endif
  } else if (!STR_CASE_CMP(codec_inst->plname, "CN")) {
    
    int codec_id;
    switch (codec_inst->plfreq) {
      case 8000: {
        codec_id = kCNNB;
        break;
      }
      case 16000: {
        codec_id = kCNWB;
        break;
      }
      case 32000: {
        codec_id = kCNSWB;
        break;
      }
      case 48000: {
        codec_id = kCNFB;
        break;
      }
      default: {
        return NULL;
      }
    }
    return new ACMCNG(codec_id);
  } else if (!STR_CASE_CMP(codec_inst->plname, "L16")) {
#ifdef WEBRTC_CODEC_PCM16
    
    int codec_id;
    if (codec_inst->channels == 1) {
      switch (codec_inst->plfreq) {
        case 8000: {
          codec_id = kPCM16B;
          break;
        }
        case 16000: {
          codec_id = kPCM16Bwb;
          break;
        }
        case 32000: {
          codec_id = kPCM16Bswb32kHz;
          break;
        }
        default: {
          return NULL;
        }
      }
    } else {
      switch (codec_inst->plfreq) {
        case 8000: {
          codec_id = kPCM16B_2ch;
          break;
        }
        case 16000: {
          codec_id = kPCM16Bwb_2ch;
          break;
        }
        case 32000: {
          codec_id = kPCM16Bswb32kHz_2ch;
          break;
        }
        default: {
          return NULL;
        }
      }
    }
    return new ACMPCM16B(codec_id);
#endif
  } else if (!STR_CASE_CMP(codec_inst->plname, "telephone-event")) {
#ifdef WEBRTC_CODEC_AVT
    return new ACMDTMFPlayout(kAVT);
#endif
  } else if (!STR_CASE_CMP(codec_inst->plname, "red")) {
#ifdef WEBRTC_CODEC_RED
    return new ACMRED(kRED);
#endif
  }
  return NULL;
}


bool ACMCodecDB::IsRateValid(int codec_id, int rate) {
  if (database_[codec_id].rate == rate) {
    return true;
  } else {
    return false;
  }
}


bool ACMCodecDB::IsISACRateValid(int rate) {
  if ((rate == -1) || ((rate <= 56000) && (rate >= 10000))) {
    return true;
  } else {
    return false;
  }
}


bool ACMCodecDB::IsILBCRateValid(int rate, int frame_size_samples) {
  if (((frame_size_samples == 240) || (frame_size_samples == 480)) &&
      (rate == 13300)) {
    return true;
  } else if (((frame_size_samples == 160) || (frame_size_samples == 320)) &&
      (rate == 15200)) {
    return true;
  } else {
    return false;
  }
}


bool ACMCodecDB::IsAMRRateValid(int rate) {
  switch (rate) {
    case 4750:
    case 5150:
    case 5900:
    case 6700:
    case 7400:
    case 7950:
    case 10200:
    case 12200: {
      return true;
    }
    default: {
      return false;
    }
  }
}


bool ACMCodecDB::IsAMRwbRateValid(int rate) {
  switch (rate) {
    case 7000:
    case 9000:
    case 12000:
    case 14000:
    case 16000:
    case 18000:
    case 20000:
    case 23000:
    case 24000: {
      return true;
    }
    default: {
      return false;
    }
  }
}


bool ACMCodecDB::IsG7291RateValid(int rate) {
  switch (rate) {
    case 8000:
    case 12000:
    case 14000:
    case 16000:
    case 18000:
    case 20000:
    case 22000:
    case 24000:
    case 26000:
    case 28000:
    case 30000:
    case 32000: {
      return true;
    }
    default: {
      return false;
    }
  }
}


bool ACMCodecDB::IsSpeexRateValid(int rate) {
  if (rate > 2000) {
    return true;
  } else {
    return false;
  }
}


bool ACMCodecDB::IsOpusRateValid(int rate) {
  if ((rate < 6000) || (rate > 510000)) {
    return false;
  }
  return true;
}


bool ACMCodecDB::IsCeltRateValid(int rate) {
  if ((rate >= 48000) && (rate <= 128000)) {
    return true;
  } else {
    return false;
  }
}


bool ACMCodecDB::ValidPayloadType(int payload_type) {
  if ((payload_type < 0) || (payload_type > 127)) {
    return false;
  }
  return true;
}

}  
