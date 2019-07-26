









#ifndef WEBRTC_MODULES_VIDEO_CODING_MAIN_SOURCE_FRAME_BUFFER_H_
#define WEBRTC_MODULES_VIDEO_CODING_MAIN_SOURCE_FRAME_BUFFER_H_

#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/modules/video_coding/main/interface/video_coding.h"
#include "webrtc/modules/video_coding/main/source/encoded_frame.h"
#include "webrtc/modules/video_coding/main/source/jitter_buffer_common.h"
#include "webrtc/modules/video_coding/main/source/session_info.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class VCMFrameBuffer : public VCMEncodedFrame {
 public:
  VCMFrameBuffer();
  virtual ~VCMFrameBuffer();

  VCMFrameBuffer(const VCMFrameBuffer& rhs);

  virtual void Reset();

  VCMFrameBufferEnum InsertPacket(const VCMPacket& packet,
                                  int64_t timeInMs,
                                  VCMDecodeErrorMode decode_error_mode,
                                  const FrameData& frame_data);

  
  
  VCMFrameBufferStateEnum GetState() const;
  
  VCMFrameBufferStateEnum GetState(uint32_t& timeStamp) const;
  void PrepareForDecode(bool continuous);

  bool IsRetransmitted() const;
  bool IsSessionComplete() const;
  bool HaveFirstPacket() const;
  bool HaveLastPacket() const;
  int NumPackets() const;
  
  void MakeSessionDecodable();

  
  
  int32_t GetLowSeqNum() const;
  
  int32_t GetHighSeqNum() const;

  int PictureId() const;
  int TemporalId() const;
  bool LayerSync() const;
  int Tl0PicId() const;
  bool NonReference() const;

  
  void SetCountedFrame(bool frameCounted);
  bool GetCountedFrame() const;

  
  
  void IncrementNackCount();
  
  
  int16_t GetNackCount() const;

  int64_t LatestPacketTimeMs() const;

  webrtc::FrameType FrameType() const;
  void SetPreviousFrameLoss();

  
  
  int NotDecodablePackets() const;

 private:
  void SetState(VCMFrameBufferStateEnum state);  

  VCMFrameBufferStateEnum    _state;         
  bool                       _frameCounted;  
  VCMSessionInfo             _sessionInfo;
  uint16_t             _nackCount;
  int64_t              _latestPacketTimeMs;
};

}  

#endif  
