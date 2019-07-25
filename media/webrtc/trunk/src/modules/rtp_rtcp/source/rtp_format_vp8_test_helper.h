
















#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_FORMAT_VP8_TEST_HELPER_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_FORMAT_VP8_TEST_HELPER_H_

#include "modules/interface/module_common_types.h"
#include "modules/rtp_rtcp/source/rtp_format_vp8.h"
#include "system_wrappers/interface/constructor_magic.h"
#include "typedefs.h"

namespace webrtc {

namespace test {

class RtpFormatVp8TestHelper {
 public:
  explicit RtpFormatVp8TestHelper(const RTPVideoHeaderVP8* hdr);
  ~RtpFormatVp8TestHelper();
  bool Init(const int* partition_sizes, int num_partitions);
  void GetAllPacketsAndCheck(RtpFormatVp8* packetizer,
                             const int* expected_sizes,
                             const int* expected_part,
                             const bool* expected_frag_start,
                             int expected_num_packets);

  uint8_t* payload_data() const { return payload_data_; }
  int payload_size() const { return payload_size_; }
  RTPFragmentationHeader* fragmentation() const { return fragmentation_; }
  int buffer_size() const { return buffer_size_; }
  void set_sloppy_partitioning(bool value) { sloppy_partitioning_ = value; }

 private:
  void CheckHeader(bool frag_start);
  void CheckPictureID();
  void CheckTl0PicIdx();
  void CheckTIDAndKeyIdx();
  void CheckPayload(int payload_end);
  void CheckLast(bool last) const;
  void CheckPacket(int send_bytes, int expect_bytes, bool last,
                   bool frag_start);

  uint8_t* payload_data_;
  uint8_t* buffer_;
  uint8_t* data_ptr_;
  RTPFragmentationHeader* fragmentation_;
  const RTPVideoHeaderVP8* hdr_info_;
  int payload_start_;
  int payload_size_;
  int buffer_size_;
  bool sloppy_partitioning_;
  bool inited_;

  DISALLOW_COPY_AND_ASSIGN(RtpFormatVp8TestHelper);
};

}  

}  

#endif  
