










#ifndef WEBRTC_MODULES_RTP_RTCP_RTCP_PACKET_H_
#define WEBRTC_MODULES_RTP_RTCP_RTCP_PACKET_H_

#include <map>
#include <string>
#include <vector>

#include "webrtc/modules/rtp_rtcp/source/rtcp_utility.h"
#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp_defines.h"
#include "webrtc/typedefs.h"

namespace webrtc {
namespace rtcp {

enum { kCommonFbFmtLength = 12 };
enum { kReportBlockLength = 24 };

class Dlrr;
class RawPacket;
class Rrtr;
class VoipMetric;




























class RtcpPacket {
 public:
  virtual ~RtcpPacket() {}

  void Append(RtcpPacket* packet);

  RawPacket Build() const;

  void Build(uint8_t* packet, size_t* length, size_t max_length) const;

 protected:
  RtcpPacket() : kHeaderLength(4) {}

  virtual void Create(
      uint8_t* packet, size_t* length, size_t max_length) const = 0;

  const size_t kHeaderLength;

 private:
  void CreateAndAddAppended(
      uint8_t* packet, size_t* length, size_t max_length) const;

  std::vector<RtcpPacket*> appended_packets_;
};

class Empty : public RtcpPacket {
 public:
  Empty() : RtcpPacket() {}

  virtual ~Empty() {}

 protected:
  virtual void Create(
      uint8_t* packet, size_t* length, size_t max_length) const OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(Empty);
};





















class ReportBlock {
 public:
  ReportBlock() {
    
    memset(&report_block_, 0, sizeof(report_block_));
  }

  ~ReportBlock() {}

  void To(uint32_t ssrc) {
    report_block_.SSRC = ssrc;
  }
  void WithFractionLost(uint8_t fraction_lost) {
    report_block_.FractionLost = fraction_lost;
  }
  void WithCumulativeLost(uint32_t cumulative_lost) {
    report_block_.CumulativeNumOfPacketsLost = cumulative_lost;
  }
  void WithExtHighestSeqNum(uint32_t ext_highest_seq_num) {
    report_block_.ExtendedHighestSequenceNumber = ext_highest_seq_num;
  }
  void WithJitter(uint32_t jitter) {
    report_block_.Jitter = jitter;
  }
  void WithLastSr(uint32_t last_sr) {
    report_block_.LastSR = last_sr;
  }
  void WithDelayLastSr(uint32_t delay_last_sr) {
    report_block_.DelayLastSR = delay_last_sr;
  }

 private:
  friend class SenderReport;
  friend class ReceiverReport;
  RTCPUtility::RTCPPacketReportBlockItem report_block_;
};






















class SenderReport : public RtcpPacket {
 public:
  SenderReport() : RtcpPacket() {
    memset(&sr_, 0, sizeof(sr_));
  }

  virtual ~SenderReport() {}

  void From(uint32_t ssrc) {
    sr_.SenderSSRC = ssrc;
  }
  void WithNtpSec(uint32_t sec) {
    sr_.NTPMostSignificant = sec;
  }
  void WithNtpFrac(uint32_t frac) {
    sr_.NTPLeastSignificant = frac;
  }
  void WithRtpTimestamp(uint32_t rtp_timestamp) {
    sr_.RTPTimestamp = rtp_timestamp;
  }
  void WithPacketCount(uint32_t packet_count) {
    sr_.SenderPacketCount = packet_count;
  }
  void WithOctetCount(uint32_t octet_count) {
    sr_.SenderOctetCount = octet_count;
  }
  void WithReportBlock(ReportBlock* block);

 protected:
  virtual void Create(
      uint8_t* packet, size_t* length, size_t max_length) const OVERRIDE;

 private:
  enum { kMaxNumberOfReportBlocks = 0x1f };

  size_t BlockLength() const {
    const size_t kSrHeaderLength = 8;
    const size_t kSenderInfoLength = 20;
    return kSrHeaderLength + kSenderInfoLength +
           report_blocks_.size() * kReportBlockLength;
  }

  RTCPUtility::RTCPPacketSR sr_;
  std::vector<RTCPUtility::RTCPPacketReportBlockItem> report_blocks_;

  DISALLOW_COPY_AND_ASSIGN(SenderReport);
};














class ReceiverReport : public RtcpPacket {
 public:
  ReceiverReport() : RtcpPacket() {
    memset(&rr_, 0, sizeof(rr_));
  }

  virtual ~ReceiverReport() {}

  void From(uint32_t ssrc) {
    rr_.SenderSSRC = ssrc;
  }
  void WithReportBlock(ReportBlock* block);

 protected:
  virtual void Create(
      uint8_t* packet, size_t* length, size_t max_length) const OVERRIDE;

 private:
  enum { kMaxNumberOfReportBlocks = 0x1f };

  size_t BlockLength() const {
    const size_t kRrHeaderLength = 8;
    return kRrHeaderLength + report_blocks_.size() * kReportBlockLength;
  }

  RTCPUtility::RTCPPacketRR rr_;
  std::vector<RTCPUtility::RTCPPacketReportBlockItem> report_blocks_;

  DISALLOW_COPY_AND_ASSIGN(ReceiverReport);
};




















class Ij : public RtcpPacket {
 public:
  Ij() : RtcpPacket() {}

  virtual ~Ij() {}

  void WithJitterItem(uint32_t jitter);

 protected:
  virtual void Create(
      uint8_t* packet, size_t* length, size_t max_length) const OVERRIDE;

 private:
  enum { kMaxNumberOfIjItems = 0x1f };

  size_t BlockLength() const {
    return kHeaderLength + 4 * ij_items_.size();
  }

  std::vector<uint32_t> ij_items_;

  DISALLOW_COPY_AND_ASSIGN(Ij);
};



























class Sdes : public RtcpPacket {
 public:
  Sdes() : RtcpPacket() {}

  virtual ~Sdes() {}

  void WithCName(uint32_t ssrc, std::string cname);

  struct Chunk {
    uint32_t ssrc;
    std::string name;
    int null_octets;
  };

 protected:
  virtual void Create(
      uint8_t* packet, size_t* length, size_t max_length) const OVERRIDE;

 private:
  enum { kMaxNumberOfChunks = 0x1f };

  size_t BlockLength() const;

  std::vector<Chunk> chunks_;

  DISALLOW_COPY_AND_ASSIGN(Sdes);
};















class Bye : public RtcpPacket {
 public:
  Bye() : RtcpPacket() {
    memset(&bye_, 0, sizeof(bye_));
  }

  virtual ~Bye() {}

  void From(uint32_t ssrc) {
    bye_.SenderSSRC = ssrc;
  }
  void WithCsrc(uint32_t csrc);

 protected:
  virtual void Create(
      uint8_t* packet, size_t* length, size_t max_length) const OVERRIDE;

 private:
  enum { kMaxNumberOfCsrcs = 0x1f - 1 };

  size_t BlockLength() const {
    size_t source_count = 1 + csrcs_.size();
    return kHeaderLength + 4 * source_count;
  }

  RTCPUtility::RTCPPacketBYE bye_;
  std::vector<uint32_t> csrcs_;

  DISALLOW_COPY_AND_ASSIGN(Bye);
};















class App : public RtcpPacket {
 public:
  App()
      : RtcpPacket(),
        ssrc_(0) {
    memset(&app_, 0, sizeof(app_));
  }

  virtual ~App() {}

  void From(uint32_t ssrc) {
    ssrc_ = ssrc;
  }
  void WithSubType(uint8_t subtype) {
    assert(subtype <= 0x1f);
    app_.SubType = subtype;
  }
  void WithName(uint32_t name) {
    app_.Name = name;
  }
  void WithData(const uint8_t* data, uint16_t data_length) {
    assert(data);
    assert(data_length <= kRtcpAppCode_DATA_SIZE);
    assert(data_length % 4 == 0);
    memcpy(app_.Data, data, data_length);
    app_.Size = data_length;
  }

 protected:
  virtual void Create(
      uint8_t* packet, size_t* length, size_t max_length) const OVERRIDE;

 private:
  size_t BlockLength() const {
    return 12 + app_.Size;
  }

  uint32_t ssrc_;
  RTCPUtility::RTCPPacketAPP app_;

  DISALLOW_COPY_AND_ASSIGN(App);
};





















class Pli : public RtcpPacket {
 public:
  Pli() : RtcpPacket() {
    memset(&pli_, 0, sizeof(pli_));
  }

  virtual ~Pli() {}

  void From(uint32_t ssrc) {
    pli_.SenderSSRC = ssrc;
  }
  void To(uint32_t ssrc) {
    pli_.MediaSSRC = ssrc;
  }

 protected:
  virtual void Create(
      uint8_t* packet, size_t* length, size_t max_length) const OVERRIDE;

 private:
  size_t BlockLength() const {
    return kCommonFbFmtLength;
  }

  RTCPUtility::RTCPPacketPSFBPLI pli_;

  DISALLOW_COPY_AND_ASSIGN(Pli);
};










class Sli : public RtcpPacket {
 public:
  Sli() : RtcpPacket() {
    memset(&sli_, 0, sizeof(sli_));
    memset(&sli_item_, 0, sizeof(sli_item_));
  }

  virtual ~Sli() {}

  void From(uint32_t ssrc) {
    sli_.SenderSSRC = ssrc;
  }
  void To(uint32_t ssrc) {
    sli_.MediaSSRC = ssrc;
  }
  void WithFirstMb(uint16_t first_mb) {
    assert(first_mb <= 0x1fff);
    sli_item_.FirstMB = first_mb;
  }
  void WithNumberOfMb(uint16_t number_mb) {
    assert(number_mb <= 0x1fff);
    sli_item_.NumberOfMB = number_mb;
  }
  void WithPictureId(uint8_t picture_id) {
    assert(picture_id <= 0x3f);
    sli_item_.PictureId = picture_id;
  }

 protected:
  virtual void Create(
      uint8_t* packet, size_t* length, size_t max_length) const OVERRIDE;

 private:
  size_t BlockLength() const {
    const size_t kFciLength = 4;
    return kCommonFbFmtLength + kFciLength;
  }

  RTCPUtility::RTCPPacketPSFBSLI sli_;
  RTCPUtility::RTCPPacketPSFBSLIItem sli_item_;

  DISALLOW_COPY_AND_ASSIGN(Sli);
};










class Nack : public RtcpPacket {
 public:
  Nack() : RtcpPacket() {
    memset(&nack_, 0, sizeof(nack_));
  }

  virtual ~Nack() {}

  void From(uint32_t ssrc) {
    nack_.SenderSSRC = ssrc;
  }
  void To(uint32_t ssrc) {
    nack_.MediaSSRC = ssrc;
  }
  void WithList(const uint16_t* nack_list, int length);

 protected:
  virtual void Create(
      uint8_t* packet, size_t* length, size_t max_length) const OVERRIDE;

 private:
  size_t BlockLength() const {
    size_t fci_length = 4 * nack_fields_.size();
    return kCommonFbFmtLength + fci_length;
  }

  RTCPUtility::RTCPPacketRTPFBNACK nack_;
  std::vector<RTCPUtility::RTCPPacketRTPFBNACKItem> nack_fields_;

  DISALLOW_COPY_AND_ASSIGN(Nack);
};













class Rpsi : public RtcpPacket {
 public:
  Rpsi()
      : RtcpPacket(),
        padding_bytes_(0) {
    memset(&rpsi_, 0, sizeof(rpsi_));
  }

  virtual ~Rpsi() {}

  void From(uint32_t ssrc) {
    rpsi_.SenderSSRC = ssrc;
  }
  void To(uint32_t ssrc) {
    rpsi_.MediaSSRC = ssrc;
  }
  void WithPayloadType(uint8_t payload) {
    assert(payload <= 0x7f);
    rpsi_.PayloadType = payload;
  }
  void WithPictureId(uint64_t picture_id);

 protected:
  virtual void Create(
      uint8_t* packet, size_t* length, size_t max_length) const OVERRIDE;

 private:
  size_t BlockLength() const {
    size_t fci_length = 2 + (rpsi_.NumberOfValidBits / 8) + padding_bytes_;
    return kCommonFbFmtLength + fci_length;
  }

  uint8_t padding_bytes_;
  RTCPUtility::RTCPPacketPSFBRPSI rpsi_;

  DISALLOW_COPY_AND_ASSIGN(Rpsi);
};













class Fir : public RtcpPacket {
 public:
  Fir() : RtcpPacket() {
    memset(&fir_, 0, sizeof(fir_));
    memset(&fir_item_, 0, sizeof(fir_item_));
  }

  virtual ~Fir() {}

  void From(uint32_t ssrc) {
    fir_.SenderSSRC = ssrc;
  }
  void To(uint32_t ssrc) {
    fir_item_.SSRC = ssrc;
  }
  void WithCommandSeqNum(uint8_t seq_num) {
    fir_item_.CommandSequenceNumber = seq_num;
  }

 protected:
  virtual void Create(
      uint8_t* packet, size_t* length, size_t max_length) const OVERRIDE;

 private:
  size_t BlockLength() const {
    const size_t kFciLength = 8;
    return kCommonFbFmtLength + kFciLength;
  }

  RTCPUtility::RTCPPacketPSFBFIR fir_;
  RTCPUtility::RTCPPacketPSFBFIRItem fir_item_;
};













class Tmmbr : public RtcpPacket {
 public:
  Tmmbr() : RtcpPacket() {
    memset(&tmmbr_, 0, sizeof(tmmbr_));
    memset(&tmmbr_item_, 0, sizeof(tmmbr_item_));
  }

  virtual ~Tmmbr() {}

  void From(uint32_t ssrc) {
    tmmbr_.SenderSSRC = ssrc;
  }
  void To(uint32_t ssrc) {
    tmmbr_item_.SSRC = ssrc;
  }
  void WithBitrateKbps(uint32_t bitrate_kbps) {
    tmmbr_item_.MaxTotalMediaBitRate = bitrate_kbps;
  }
  void WithOverhead(uint16_t overhead) {
    assert(overhead <= 0x1ff);
    tmmbr_item_.MeasuredOverhead = overhead;
  }

 protected:
  virtual void Create(
      uint8_t* packet, size_t* length, size_t max_length) const OVERRIDE;

 private:
  size_t BlockLength() const {
    const size_t kFciLen = 8;
    return kCommonFbFmtLength + kFciLen;
  }

  RTCPUtility::RTCPPacketRTPFBTMMBR tmmbr_;
  RTCPUtility::RTCPPacketRTPFBTMMBRItem tmmbr_item_;

  DISALLOW_COPY_AND_ASSIGN(Tmmbr);
};













class Tmmbn : public RtcpPacket {
 public:
  Tmmbn() : RtcpPacket() {
    memset(&tmmbn_, 0, sizeof(tmmbn_));
  }

  virtual ~Tmmbn() {}

  void From(uint32_t ssrc) {
    tmmbn_.SenderSSRC = ssrc;
  }
  void WithTmmbr(uint32_t ssrc, uint32_t bitrate_kbps, uint16_t overhead);

 protected:
  virtual void Create(
      uint8_t* packet, size_t* length, size_t max_length) const OVERRIDE;

 private:
  enum { kMaxNumberOfTmmbrs = 50 };

  size_t BlockLength() const {
    const size_t kFciLen = 8;
    return kCommonFbFmtLength + kFciLen * tmmbn_items_.size();
  }

  RTCPUtility::RTCPPacketRTPFBTMMBN tmmbn_;
  std::vector<RTCPUtility::RTCPPacketRTPFBTMMBRItem> tmmbn_items_;

  DISALLOW_COPY_AND_ASSIGN(Tmmbn);
};




















class Remb : public RtcpPacket {
 public:
  Remb() : RtcpPacket() {
    memset(&remb_, 0, sizeof(remb_));
    memset(&remb_item_, 0, sizeof(remb_item_));
  }

  virtual ~Remb() {}

  void From(uint32_t ssrc) {
    remb_.SenderSSRC = ssrc;
  }
  void AppliesTo(uint32_t ssrc);

  void WithBitrateBps(uint32_t bitrate_bps) {
    remb_item_.BitRate = bitrate_bps;
  }

 protected:
  virtual void Create(
      uint8_t* packet, size_t* length, size_t max_length) const OVERRIDE;

 private:
  enum { kMaxNumberOfSsrcs = 0xff };

  size_t BlockLength() const {
    return (remb_item_.NumberOfSSRCs + 5) * 4;
  }

  RTCPUtility::RTCPPacketPSFBAPP remb_;
  RTCPUtility::RTCPPacketPSFBREMBItem remb_item_;

  DISALLOW_COPY_AND_ASSIGN(Remb);
};















class Xr : public RtcpPacket {
 public:
  typedef std::vector<RTCPUtility::RTCPPacketXRDLRRReportBlockItem> DlrrBlock;
  Xr() : RtcpPacket() {
    memset(&xr_header_, 0, sizeof(xr_header_));
  }

  virtual ~Xr() {}

  void From(uint32_t ssrc) {
    xr_header_.OriginatorSSRC = ssrc;
  }
  void WithRrtr(Rrtr* rrtr);
  void WithDlrr(Dlrr* dlrr);
  void WithVoipMetric(VoipMetric* voip_metric);

 protected:
  virtual void Create(
      uint8_t* packet, size_t* length, size_t max_length) const OVERRIDE;

 private:
  enum { kMaxNumberOfRrtrBlocks = 50 };
  enum { kMaxNumberOfDlrrBlocks = 50 };
  enum { kMaxNumberOfVoipMetricBlocks = 50 };

  size_t BlockLength() const {
    const size_t kXrHeaderLength = 8;
    return kXrHeaderLength + RrtrLength() + DlrrLength() + VoipMetricLength();
  }

  size_t RrtrLength() const {
    const size_t kRrtrBlockLength = 12;
    return kRrtrBlockLength * rrtr_blocks_.size();
  }

  size_t DlrrLength() const;

  size_t VoipMetricLength() const {
    const size_t kVoipMetricBlockLength = 36;
    return kVoipMetricBlockLength * voip_metric_blocks_.size();
  }

  RTCPUtility::RTCPPacketXR xr_header_;
  std::vector<RTCPUtility::RTCPPacketXRReceiverReferenceTimeItem> rrtr_blocks_;
  std::vector<DlrrBlock> dlrr_blocks_;
  std::vector<RTCPUtility::RTCPPacketXRVOIPMetricItem> voip_metric_blocks_;

  DISALLOW_COPY_AND_ASSIGN(Xr);
};













class Rrtr {
 public:
  Rrtr() {
    memset(&rrtr_block_, 0, sizeof(rrtr_block_));
  }
  ~Rrtr() {}

  void WithNtpSec(uint32_t sec) {
    rrtr_block_.NTPMostSignificant = sec;
  }
  void WithNtpFrac(uint32_t frac) {
    rrtr_block_.NTPLeastSignificant = frac;
  }

 private:
  friend class Xr;
  RTCPUtility::RTCPPacketXRReceiverReferenceTimeItem rrtr_block_;

  DISALLOW_COPY_AND_ASSIGN(Rrtr);
};


















class Dlrr {
 public:
  Dlrr() {}
  ~Dlrr() {}

  void WithDlrrItem(uint32_t ssrc, uint32_t last_rr, uint32_t delay_last_rr);

 private:
  friend class Xr;
  enum { kMaxNumberOfDlrrItems = 100 };

  std::vector<RTCPUtility::RTCPPacketXRDLRRReportBlockItem> dlrr_block_;

  DISALLOW_COPY_AND_ASSIGN(Dlrr);
};

























class VoipMetric {
 public:
  VoipMetric() {
    memset(&metric_, 0, sizeof(metric_));
  }
  ~VoipMetric() {}

  void To(uint32_t ssrc) { metric_.SSRC = ssrc; }
  void LossRate(uint8_t loss_rate) { metric_.lossRate = loss_rate; }
  void DiscardRate(uint8_t discard_rate) { metric_.discardRate = discard_rate; }
  void BurstDensity(uint8_t burst_density) {
    metric_.burstDensity = burst_density;
  }
  void GapDensity(uint8_t gap_density) { metric_.gapDensity = gap_density; }
  void BurstDuration(uint16_t burst_duration) {
    metric_.burstDuration = burst_duration;
  }
  void GapDuration(uint16_t gap_duration) {
    metric_.gapDuration = gap_duration;
  }
  void RoundTripDelay(uint16_t round_trip_delay) {
    metric_.roundTripDelay = round_trip_delay;
  }
  void EndSystemDelay(uint16_t end_system_delay) {
    metric_.endSystemDelay = end_system_delay;
  }
  void SignalLevel(uint8_t signal_level) { metric_.signalLevel = signal_level; }
  void NoiseLevel(uint8_t noise_level) { metric_.noiseLevel = noise_level; }
  void Rerl(uint8_t rerl) { metric_.RERL = rerl; }
  void Gmin(uint8_t gmin) { metric_.Gmin = gmin; }
  void Rfactor(uint8_t rfactor) { metric_.Rfactor = rfactor; }
  void ExtRfactor(uint8_t extrfactor) { metric_.extRfactor = extrfactor; }
  void MosLq(uint8_t moslq) { metric_.MOSLQ = moslq; }
  void MosCq(uint8_t moscq) { metric_.MOSCQ = moscq; }
  void RxConfig(uint8_t rxconfig) { metric_.RXconfig = rxconfig; }
  void JbNominal(uint16_t jbnominal) { metric_.JBnominal = jbnominal; }
  void JbMax(uint16_t jbmax) { metric_.JBmax = jbmax; }
  void JbAbsMax(uint16_t jbabsmax) { metric_.JBabsMax = jbabsmax; }

 private:
  friend class Xr;
  RTCPUtility::RTCPPacketXRVOIPMetricItem metric_;

  DISALLOW_COPY_AND_ASSIGN(VoipMetric);
};










class RawPacket {
 public:
  RawPacket(const uint8_t* packet, size_t length) {
    assert(length <= IP_PACKET_SIZE);
    memcpy(buffer_, packet, length);
    buffer_length_ = length;
  }

  const uint8_t* buffer() {
    return buffer_;
  }
  size_t buffer_length() const {
    return buffer_length_;
  }

 private:
  size_t buffer_length_;
  uint8_t buffer_[IP_PACKET_SIZE];
};

}  
}  
#endif
