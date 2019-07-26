









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_PAYLOAD_SPLITTER_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_PAYLOAD_SPLITTER_H_

#include "webrtc/modules/audio_coding/neteq4/packet.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"

namespace webrtc {


class DecoderDatabase;






class PayloadSplitter {
 public:
  enum SplitterReturnCodes {
    kOK = 0,
    kNoSplit = 1,
    kTooLargePayload = -1,
    kFrameSplitError = -2,
    kUnknownPayloadType = -3,
    kRedLengthMismatch = -4
  };

  PayloadSplitter() {}

  virtual ~PayloadSplitter() {}

  
  
  
  
  
  
  virtual int SplitRed(PacketList* packet_list);

  
  
  
  virtual int CheckRedPayloads(PacketList* packet_list,
                               const DecoderDatabase& decoder_database);

  
  
  
  
  virtual int SplitAudio(PacketList* packet_list,
                         const DecoderDatabase& decoder_database);

 private:
  
  
  virtual void SplitBySamples(const Packet* packet,
                              int bytes_per_ms,
                              int timestamps_per_ms,
                              PacketList* new_packets);

  
  
  
  virtual int SplitByFrames(const Packet* packet,
                            int bytes_per_frame,
                            int timestamps_per_frame,
                            PacketList* new_packets);

  DISALLOW_COPY_AND_ASSIGN(PayloadSplitter);
};

}  
#endif
