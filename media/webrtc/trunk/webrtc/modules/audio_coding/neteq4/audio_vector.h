









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_AUDIO_VECTOR_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_AUDIO_VECTOR_H_

#include <string.h>  

#include <vector>

#include "webrtc/system_wrappers/interface/constructor_magic.h"

namespace webrtc {

template <typename T>
class AudioVector {
 public:
  
  AudioVector() {}

  
  explicit AudioVector(size_t initial_size)
      : vector_(initial_size, 0) {}

  virtual ~AudioVector() {}

  
  virtual void Clear();

  
  
  
  virtual void CopyFrom(AudioVector<T>* copy_to) const;

  
  
  virtual void PushFront(const AudioVector<T>& prepend_this);

  
  
  virtual void PushFront(const T* prepend_this, size_t length);

  
  virtual void PushBack(const AudioVector<T>& append_this);

  
  virtual void PushBack(const T* append_this, size_t length);

  
  virtual void PopFront(size_t length);

  
  virtual void PopBack(size_t length);

  
  
  virtual void Extend(size_t extra_length);

  
  
  
  
  virtual void InsertAt(const T* insert_this, size_t length, size_t position);

  
  virtual void InsertZerosAt(size_t length, size_t position);

  
  
  
  
  
  virtual void OverwriteAt(const T* insert_this,
                           size_t length,
                           size_t position);

  
  
  
  virtual void CrossFade(const AudioVector<T>& append_this, size_t fade_length);

  
  virtual size_t Size() const { return vector_.size(); }

  
  virtual bool Empty() const { return vector_.empty(); }

  
  const T& operator[](size_t index) const;
  T& operator[](size_t index);

 private:
  std::vector<T> vector_;

  DISALLOW_COPY_AND_ASSIGN(AudioVector);
};

}  
#endif  
