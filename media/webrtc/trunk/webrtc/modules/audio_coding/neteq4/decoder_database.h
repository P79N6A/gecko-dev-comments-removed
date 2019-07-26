









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_DECODER_DATABASE_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_DECODER_DATABASE_H_

#include <map>

#include "webrtc/common_types.h"  
#include "webrtc/modules/audio_coding/neteq4/interface/audio_decoder.h"
#include "webrtc/modules/audio_coding/neteq4/packet.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/typedefs.h"

namespace webrtc {


class AudioDecoder;

class DecoderDatabase {
 public:
  enum DatabaseReturnCodes {
    kOK = 0,
    kInvalidRtpPayloadType = -1,
    kCodecNotSupported = -2,
    kInvalidSampleRate = -3,
    kDecoderExists = -4,
    kDecoderNotFound = -5,
    kInvalidPointer = -6
  };

  
  struct DecoderInfo {
    
    DecoderInfo()
        : codec_type(kDecoderArbitrary),
          fs_hz(8000),
          decoder(NULL),
          external(false) {
    }
    DecoderInfo(NetEqDecoder ct, int fs, AudioDecoder* dec, bool ext)
        : codec_type(ct),
          fs_hz(fs),
          decoder(dec),
          external(ext) {
    }
    
    ~DecoderInfo();

    NetEqDecoder codec_type;
    int fs_hz;
    AudioDecoder* decoder;
    bool external;
  };

  static const uint8_t kMaxRtpPayloadType = 0x7F;  
  
  
  static const uint8_t kRtpPayloadTypeError = 0xFF;

  DecoderDatabase();

  virtual ~DecoderDatabase();

  
  virtual bool Empty() const;

  
  virtual int Size() const;

  
  
  
  virtual void Reset();

  
  
  virtual int RegisterPayload(uint8_t rtp_payload_type,
                              NetEqDecoder codec_type);

  
  
  virtual int InsertExternal(uint8_t rtp_payload_type,
                             NetEqDecoder codec_type,
                             int fs_hz, AudioDecoder* decoder);

  
  
  virtual int Remove(uint8_t rtp_payload_type);

  
  
  virtual const DecoderInfo* GetDecoderInfo(uint8_t rtp_payload_type) const;

  
  
  
  
  virtual uint8_t GetRtpPayloadType(NetEqDecoder codec_type) const;

  
  
  
  virtual AudioDecoder* GetDecoder(uint8_t rtp_payload_type);

  
  virtual bool IsType(uint8_t rtp_payload_type,
                      NetEqDecoder codec_type) const;

  
  virtual bool IsComfortNoise(uint8_t rtp_payload_type) const;

  
  virtual bool IsDtmf(uint8_t rtp_payload_type) const;

  
  virtual bool IsRed(uint8_t rtp_payload_type) const;

  
  
  
  virtual int SetActiveDecoder(uint8_t rtp_payload_type, bool* new_decoder);

  
  virtual AudioDecoder* GetActiveDecoder();

  
  
  
  virtual int SetActiveCngDecoder(uint8_t rtp_payload_type);

  
  
  virtual AudioDecoder* GetActiveCngDecoder();

  
  
  virtual int CheckPayloadTypes(const PacketList& packet_list) const;

 private:
  typedef std::map<uint8_t, DecoderInfo> DecoderMap;

  DecoderMap decoders_;
  int active_decoder_;
  int active_cng_decoder_;

  DISALLOW_COPY_AND_ASSIGN(DecoderDatabase);
};

}  
#endif  
