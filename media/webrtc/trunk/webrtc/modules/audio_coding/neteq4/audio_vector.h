









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_AUDIO_VECTOR_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_AUDIO_VECTOR_H_

#include <string.h>  

#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class AudioVector {
 public:
  
  AudioVector()
      : array_(new int16_t[kDefaultInitialSize]),
        first_free_ix_(0),
        capacity_(kDefaultInitialSize) {}

  
  explicit AudioVector(size_t initial_size)
      : array_(new int16_t[initial_size]),
        first_free_ix_(initial_size),
        capacity_(initial_size) {
    memset(array_.get(), 0, initial_size * sizeof(int16_t));
  }

  virtual ~AudioVector() {}

  
  virtual void Clear();

  
  
  
  virtual void CopyFrom(AudioVector* copy_to) const;

  
  
  virtual void PushFront(const AudioVector& prepend_this);

  
  
  virtual void PushFront(const int16_t* prepend_this, size_t length);

  
  virtual void PushBack(const AudioVector& append_this);

  
  virtual void PushBack(const int16_t* append_this, size_t length);

  
  virtual void PopFront(size_t length);

  
  virtual void PopBack(size_t length);

  
  
  virtual void Extend(size_t extra_length);

  
  
  
  
  virtual void InsertAt(const int16_t* insert_this, size_t length,
                        size_t position);

  
  virtual void InsertZerosAt(size_t length, size_t position);

  
  
  
  
  
  virtual void OverwriteAt(const int16_t* insert_this,
                           size_t length,
                           size_t position);

  
  
  
  virtual void CrossFade(const AudioVector& append_this, size_t fade_length);

  
  virtual size_t Size() const { return first_free_ix_; }

  
  virtual bool Empty() const { return (first_free_ix_ == 0); }

  
  const int16_t& operator[](size_t index) const;
  int16_t& operator[](size_t index);

 private:
  static const size_t kDefaultInitialSize = 10;

  void Reserve(size_t n);

  scoped_ptr<int16_t[]> array_;
  size_t first_free_ix_;  
                          
  size_t capacity_;  

  DISALLOW_COPY_AND_ASSIGN(AudioVector);
};

}  
#endif  
