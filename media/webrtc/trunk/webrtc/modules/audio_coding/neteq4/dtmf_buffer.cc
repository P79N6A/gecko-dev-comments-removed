









#include "webrtc/modules/audio_coding/neteq4/dtmf_buffer.h"

#include <assert.h>
#include <algorithm>  




#define LEGACY_BITEXACT

namespace webrtc {


































int DtmfBuffer::ParseEvent(uint32_t rtp_timestamp,
                           const uint8_t* payload,
                           int payload_length_bytes,
                           DtmfEvent* event) {
  if (!payload || !event) {
    return kInvalidPointer;
  }
  if (payload_length_bytes < 4) {
    return kPayloadTooShort;
  }

  event->event_no = payload[0];
  event->end_bit = ((payload[1] & 0x80) != 0);
  event->volume = (payload[1] & 0x3F);
  event->duration = payload[2] << 8 | payload[3];
  event->timestamp = rtp_timestamp;
  return kOK;
}













int DtmfBuffer::InsertEvent(const DtmfEvent& event) {
  if (event.event_no < 0 || event.event_no > 15 ||
      event.volume < 0 || event.volume > 36 ||
      event.duration <= 0 || event.duration > 65535) {
    return kInvalidEventParameters;
  }
  DtmfList::iterator it = buffer_.begin();
  while (it != buffer_.end()) {
    if (MergeEvents(it, event)) {
      
      return kOK;
    }
    ++it;
  }
  buffer_.push_back(event);
  
  buffer_.sort(CompareEvents);
  return kOK;
}

bool DtmfBuffer::GetEvent(uint32_t current_timestamp, DtmfEvent* event) {
  DtmfList::iterator it = buffer_.begin();
  while (it != buffer_.end()) {
    
    
    uint32_t event_end = it->timestamp + it->duration;
#ifdef LEGACY_BITEXACT
    bool next_available = false;
#endif
    if (!it->end_bit) {
      
      
      event_end += max_extrapolation_samples_;
      DtmfList::iterator next = it;
      ++next;
      if (next != buffer_.end()) {
        
        
        event_end = std::min(event_end, next->timestamp);
#ifdef LEGACY_BITEXACT
        next_available = true;
#endif
      }
    }
    if (current_timestamp >= it->timestamp
        && current_timestamp <= event_end) {  
      
      if (event) {
        event->event_no = it->event_no;
        event->end_bit = it->end_bit;
        event->volume = it->volume;
        event->duration = it->duration;
        event->timestamp = it->timestamp;
      }
#ifdef LEGACY_BITEXACT
      if (it->end_bit &&
          current_timestamp + frame_len_samples_ >= event_end) {
        
        buffer_.erase(it);
      }
#endif
      return true;
    } else if (current_timestamp > event_end) {  
      
      
#ifdef LEGACY_BITEXACT
      if (!next_available) {
        if (event) {
          event->event_no = it->event_no;
          event->end_bit = it->end_bit;
          event->volume = it->volume;
          event->duration = it->duration;
          event->timestamp = it->timestamp;
        }
        it = buffer_.erase(it);
        return true;
      } else {
        it = buffer_.erase(it);
      }
#else
      it = buffer_.erase(it);
#endif
    } else {
      ++it;
    }
  }
  return false;
}

int DtmfBuffer::SetSampleRate(int fs_hz) {
  if (fs_hz != 8000 &&
      fs_hz != 16000 &&
      fs_hz != 32000 &&
      fs_hz != 48000) {
    return kInvalidSampleRate;
  }
  max_extrapolation_samples_ = 7 * fs_hz / 100;
  frame_len_samples_ = fs_hz / 100;
  return kOK;
}






bool DtmfBuffer::SameEvent(const DtmfEvent& a, const DtmfEvent& b) {
  return (a.event_no == b.event_no) && (a.timestamp == b.timestamp);
}

bool DtmfBuffer::MergeEvents(DtmfList::iterator it, const DtmfEvent& event) {
  if (SameEvent(*it, event)) {
    if (!it->end_bit) {
      
      
      it->duration = std::max(event.duration, it->duration);
    }
    if (event.end_bit) {
      it->end_bit = true;
    }
    return true;
  } else {
    return false;
  }
}







bool DtmfBuffer::CompareEvents(const DtmfEvent& a, const DtmfEvent& b) {
  if (a.timestamp == b.timestamp) {
    return a.event_no < b.event_no;
  }
  
  return (static_cast<uint32_t>(b.timestamp - a.timestamp) < 0xFFFFFFFF / 2);
}
}  
