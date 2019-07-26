









#include <assert.h>

#include <algorithm>  

#include "webrtc/modules/audio_coding/neteq4/sync_buffer.h"

namespace webrtc {

size_t SyncBuffer::FutureLength() const {
  return Size() - next_index_;
}

void SyncBuffer::PushBack(const AudioMultiVector& append_this) {
  size_t samples_added = append_this.Size();
  AudioMultiVector::PushBack(append_this);
  AudioMultiVector::PopFront(samples_added);
  if (samples_added <= next_index_) {
    next_index_ -= samples_added;
  } else {
    

    
    
    
    next_index_ = 0;
  }
  dtmf_index_ -= std::min(dtmf_index_, samples_added);
}

void SyncBuffer::PushFrontZeros(size_t length) {
  InsertZerosAtIndex(length, 0);
}

void SyncBuffer::InsertZerosAtIndex(size_t length, size_t position) {
  position = std::min(position, Size());
  length = std::min(length, Size() - position);
  AudioMultiVector::PopBack(length);
  for (size_t channel = 0; channel < Channels(); ++channel) {
    channels_[channel]->InsertZerosAt(length, position);
  }
  if (next_index_ >= position) {
    
    set_next_index(next_index_ + length);  
  }
  if (dtmf_index_ > 0 && dtmf_index_ >= position) {
    
    set_dtmf_index(dtmf_index_ + length);  
  }
}

void SyncBuffer::ReplaceAtIndex(const AudioMultiVector& insert_this,
                                size_t length,
                                size_t position) {
  position = std::min(position, Size());  
  length = std::min(length, Size() - position);
  AudioMultiVector::OverwriteAt(insert_this, length, position);
}

void SyncBuffer::ReplaceAtIndex(const AudioMultiVector& insert_this,
                                size_t position) {
  ReplaceAtIndex(insert_this, insert_this.Size(), position);
}

size_t SyncBuffer::GetNextAudioInterleaved(size_t requested_len,
                                           int16_t* output) {
  if (!output) {
    assert(false);
    return 0;
  }
  size_t samples_to_read = std::min(FutureLength(), requested_len);
  ReadInterleavedFromIndex(next_index_, samples_to_read, output);
  next_index_ += samples_to_read;
  return samples_to_read;
}

void SyncBuffer::IncreaseEndTimestamp(uint32_t increment) {
  end_timestamp_ += increment;
}

void SyncBuffer::Flush() {
  Zeros(Size());
  next_index_ = Size();
  end_timestamp_ = 0;
  dtmf_index_ = 0;
}

void SyncBuffer::set_next_index(size_t value) {
  
  next_index_ = std::min(value, Size());
}

void SyncBuffer::set_dtmf_index(size_t value) {
  
  dtmf_index_ = std::min(value, Size());
}

}  
