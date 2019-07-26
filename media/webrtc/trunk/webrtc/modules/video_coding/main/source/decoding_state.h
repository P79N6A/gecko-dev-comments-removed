









#ifndef WEBRTC_MODULES_VIDEO_CODING_DECODING_STATE_H_
#define WEBRTC_MODULES_VIDEO_CODING_DECODING_STATE_H_

#include "typedefs.h"

namespace webrtc {


class VCMFrameBuffer;
class VCMPacket;

class VCMDecodingState {
 public:
  VCMDecodingState();
  ~VCMDecodingState();
  
  bool IsOldFrame(const VCMFrameBuffer* frame) const;
  
  bool IsOldPacket(const VCMPacket* packet) const;
  
  
  bool ContinuousFrame(const VCMFrameBuffer* frame) const;
  void SetState(const VCMFrameBuffer* frame);
  
  void SetStateOneBack(const VCMFrameBuffer* frame);
  void UpdateEmptyFrame(const VCMFrameBuffer* frame);
  
  
  
  void UpdateOldPacket(const VCMPacket* packet);
  void SetSeqNum(uint16_t new_seq_num);
  void Reset();
  uint32_t time_stamp() const;
  uint16_t sequence_num() const;
  
  bool init() const;
  
  bool full_sync() const;

 private:
  void UpdateSyncState(const VCMFrameBuffer* frame);
  
  bool ContinuousPictureId(int picture_id) const;
  bool ContinuousSeqNum(uint16_t seq_num) const;
  bool ContinuousLayer(int temporal_id, int tl0_pic_id) const;
  bool UsingPictureId(const VCMFrameBuffer* frame) const;

  
  
  uint16_t    sequence_num_;
  uint32_t    time_stamp_;
  int         picture_id_;
  int         temporal_id_;
  int         tl0_pic_id_;
  bool        full_sync_;  
  bool        init_;
};

}  

#endif  
