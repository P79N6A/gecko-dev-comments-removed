









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_SYNC_BUFFER_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_SYNC_BUFFER_H_

#include "webrtc/modules/audio_coding/neteq4/audio_multi_vector.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class SyncBuffer : public AudioMultiVector {
 public:
  SyncBuffer(size_t channels, size_t length)
      : AudioMultiVector(channels, length),
        next_index_(length),
        end_timestamp_(0),
        dtmf_index_(0) {}

  virtual ~SyncBuffer() {}

  
  size_t FutureLength() const;

  
  
  
  
  void PushBack(const AudioMultiVector& append_this);

  
  
  
  
  
  
  void PushFrontZeros(size_t length);

  
  
  
  virtual void InsertZerosAtIndex(size_t length, size_t position);

  
  
  
  
  
  
  
  virtual void ReplaceAtIndex(const AudioMultiVector& insert_this,
                              size_t length,
                              size_t position);

  
  
  virtual void ReplaceAtIndex(const AudioMultiVector& insert_this,
                              size_t position);

  
  
  
  size_t GetNextAudioInterleaved(size_t requested_len, int16_t* output);

  
  void IncreaseEndTimestamp(uint32_t increment);

  
  
  
  void Flush();

  const AudioVector& Channel(size_t n) { return *channels_[n]; }

  
  size_t next_index() const { return next_index_; }
  void set_next_index(size_t value);
  uint32_t end_timestamp() const { return end_timestamp_; }
  void set_end_timestamp(uint32_t value) { end_timestamp_ = value; }
  size_t dtmf_index() const { return dtmf_index_; }
  void set_dtmf_index(size_t value);

 private:
  size_t next_index_;
  uint32_t end_timestamp_;  
  size_t dtmf_index_;  

  DISALLOW_COPY_AND_ASSIGN(SyncBuffer);
};

}  
#endif  
