









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_AUDIO_MULTI_VECTOR_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_AUDIO_MULTI_VECTOR_H_

#include <string.h>  

#include <vector>

#include "webrtc/modules/audio_coding/neteq4/audio_vector.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"

namespace webrtc {

template <typename T>
class AudioMultiVector {
 public:
  
  
  explicit AudioMultiVector(size_t N);

  
  
  AudioMultiVector(size_t N, size_t initial_size);

  virtual ~AudioMultiVector();

  
  virtual void Clear();

  
  virtual void Zeros(size_t length);

  
  
  
  
  virtual void CopyFrom(AudioMultiVector<T>* copy_to) const;

  
  
  
  
  
  virtual void PushBackInterleaved(const T* append_this, size_t length);

  
  
  virtual void PushBack(const AudioMultiVector<T>& append_this);

  
  
  
  virtual void PushBackFromIndex(const AudioMultiVector<T>& append_this,
                                 size_t index);

  
  
  virtual void PopFront(size_t length);

  
  
  virtual void PopBack(size_t length);

  
  
  
  
  
  virtual size_t ReadInterleaved(size_t length, T* destination) const;

  
  
  virtual size_t ReadInterleavedFromIndex(size_t start_index,
                                          size_t length,
                                          T* destination) const;

  
  
  virtual size_t ReadInterleavedFromEnd(size_t length,
                                        T* destination) const;

  
  
  
  
  
  
  
  virtual void OverwriteAt(const AudioMultiVector<T>& insert_this,
                           size_t length,
                           size_t position);

  
  
  
  virtual void CrossFade(const AudioMultiVector<T>& append_this,
                         size_t fade_length);

  
  virtual size_t Channels() const { return channels_.size(); }

  
  virtual size_t Size() const;

  
  
  virtual void AssertSize(size_t required_size);

  virtual bool Empty() const;

  
  
  const AudioVector<T>& operator[](size_t index) const;
  AudioVector<T>& operator[](size_t index);

 protected:
  std::vector<AudioVector<T>*> channels_;

 private:
  DISALLOW_COPY_AND_ASSIGN(AudioMultiVector);
};

}  
#endif  
