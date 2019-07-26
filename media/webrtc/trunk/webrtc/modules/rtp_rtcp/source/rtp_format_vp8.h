























#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_FORMAT_VP8_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_FORMAT_VP8_H_

#include <queue>
#include <vector>

#include "modules/interface/module_common_types.h"
#include "system_wrappers/interface/constructor_magic.h"
#include "typedefs.h"  

namespace webrtc {

enum VP8PacketizerMode {
  kStrict = 0,  
                
  kAggregate,   
  kEqualSize,   
                
  kNumModes,
};


class RtpFormatVp8 {
 public:
  
  
  RtpFormatVp8(const WebRtc_UWord8* payload_data,
               WebRtc_UWord32 payload_size,
               const RTPVideoHeaderVP8& hdr_info,
               int max_payload_len,
               const RTPFragmentationHeader& fragmentation,
               VP8PacketizerMode mode);

  
  
  RtpFormatVp8(const WebRtc_UWord8* payload_data,
               WebRtc_UWord32 payload_size,
               const RTPVideoHeaderVP8& hdr_info,
               int max_payload_len);

  
  
  
  
  
  
  
  
  
  
  
  int NextPacket(WebRtc_UWord8* buffer,
                 int* bytes_to_send,
                 bool* last_packet);

 private:
  typedef struct {
    int payload_start_pos;
    int size;
    bool first_fragment;
    int first_partition_ix;
  } InfoStruct;
  typedef std::queue<InfoStruct> InfoQueue;
  enum AggregationMode {
    kAggrNone = 0,    
    kAggrPartitions,  
    kAggrFragments    
  };

  static const AggregationMode aggr_modes_[kNumModes];
  static const bool balance_modes_[kNumModes];
  static const bool separate_first_modes_[kNumModes];
  static const int kXBit        = 0x80;
  static const int kNBit        = 0x20;
  static const int kSBit        = 0x10;
  static const int kPartIdField = 0x0F;
  static const int kKeyIdxField = 0x1F;
  static const int kIBit        = 0x80;
  static const int kLBit        = 0x40;
  static const int kTBit        = 0x20;
  static const int kKBit        = 0x10;
  static const int kYBit        = 0x20;

  
  int CalcNextSize(int max_payload_len, int remaining_bytes,
                   bool split_payload) const;

  
  int GeneratePackets();

  
  
  int GeneratePacketsBalancedAggregates();

  
  
  
  
  
  
  
  
  
  void AggregateSmallPartitions(std::vector<int>* partition_vec,
                                int* min_size,
                                int* max_size);

  
  void QueuePacket(int start_pos,
                   int packet_size,
                   int first_partition_in_packet,
                   bool start_on_new_fragment);

  
  
  
  int WriteHeaderAndPayload(const InfoStruct& packet_info,
                            WebRtc_UWord8* buffer,
                            int buffer_length) const;


  
  
  
  int WriteExtensionFields(WebRtc_UWord8* buffer, int buffer_length) const;

  
  
  int WritePictureIDFields(WebRtc_UWord8* x_field, WebRtc_UWord8* buffer,
                           int buffer_length, int* extension_length) const;

  
  
  int WriteTl0PicIdxFields(WebRtc_UWord8* x_field, WebRtc_UWord8* buffer,
                           int buffer_length, int* extension_length) const;

  
  
  
  int WriteTIDAndKeyIdxFields(WebRtc_UWord8* x_field, WebRtc_UWord8* buffer,
                              int buffer_length, int* extension_length) const;

  
  
  
  int WritePictureID(WebRtc_UWord8* buffer, int buffer_length) const;

  
  
  int PayloadDescriptorExtraLength() const;

  
  
  int PictureIdLength() const;

  
  bool XFieldPresent() const;
  bool TIDFieldPresent() const;
  bool KeyIdxFieldPresent() const;
  bool TL0PicIdxFieldPresent() const;
  bool PictureIdPresent() const { return (PictureIdLength() > 0); }

  const WebRtc_UWord8* payload_data_;
  const int payload_size_;
  RTPFragmentationHeader part_info_;
  const int vp8_fixed_payload_descriptor_bytes_;  
                                                  
  const AggregationMode aggr_mode_;
  const bool balance_;
  const bool separate_first_;
  const RTPVideoHeaderVP8 hdr_info_;
  const int num_partitions_;
  const int max_payload_len_;
  InfoQueue packets_;
  bool packets_calculated_;

  DISALLOW_COPY_AND_ASSIGN(RtpFormatVp8);
};

}  

#endif  
