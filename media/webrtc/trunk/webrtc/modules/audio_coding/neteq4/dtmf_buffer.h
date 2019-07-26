









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_DTMF_BUFFER_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_DTMF_BUFFER_H_

#include <list>
#include <string>  

#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/typedefs.h"

namespace webrtc {

struct DtmfEvent {
  uint32_t timestamp;
  int event_no;
  int volume;
  int duration;
  bool end_bit;

  
  DtmfEvent()
      : timestamp(0),
        event_no(0),
        volume(0),
        duration(0),
        end_bit(false) {
  }
  DtmfEvent(uint32_t ts, int ev, int vol, int dur, bool end)
      : timestamp(ts),
        event_no(ev),
        volume(vol),
        duration(dur),
        end_bit(end) {
  }
};


class DtmfBuffer {
 public:
  enum BufferReturnCodes {
    kOK = 0,
    kInvalidPointer,
    kPayloadTooShort,
    kInvalidEventParameters,
    kInvalidSampleRate
  };

  
  explicit DtmfBuffer(int fs_hz) {
    SetSampleRate(fs_hz);
  }

  virtual ~DtmfBuffer() {}

  
  virtual void Flush() { buffer_.clear(); }

  
  
  
  static int ParseEvent(uint32_t rtp_timestamp,
                        const uint8_t* payload,
                        int payload_length_bytes,
                        DtmfEvent* event);

  
  
  virtual int InsertEvent(const DtmfEvent& event);

  
  
  
  virtual bool GetEvent(uint32_t current_timestamp, DtmfEvent* event);

  
  virtual size_t Length() const { return buffer_.size(); }

  virtual bool Empty() const { return buffer_.empty(); }

  
  virtual int SetSampleRate(int fs_hz);

 private:
  typedef std::list<DtmfEvent> DtmfList;

  int max_extrapolation_samples_;
  int frame_len_samples_;  

  
  static bool SameEvent(const DtmfEvent& a, const DtmfEvent& b);

  
  
  
  
  bool MergeEvents(DtmfList::iterator it, const DtmfEvent& event);

  
  static bool CompareEvents(const DtmfEvent& a, const DtmfEvent& b);

  DtmfList buffer_;

  DISALLOW_COPY_AND_ASSIGN(DtmfBuffer);
};

}  
#endif  
