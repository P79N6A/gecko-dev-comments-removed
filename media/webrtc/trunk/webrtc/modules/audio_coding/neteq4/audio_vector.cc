









#include "webrtc/modules/audio_coding/neteq4/audio_vector.h"

#include <assert.h>

#include <algorithm>

#include "webrtc/typedefs.h"

namespace webrtc {

template<typename T>
void AudioVector<T>::Clear() {
  vector_.clear();
}

template<typename T>
void AudioVector<T>::CopyFrom(AudioVector<T>* copy_to) const {
  if (copy_to) {
    copy_to->vector_.assign(vector_.begin(), vector_.end());
  }
}

template<typename T>
void AudioVector<T>::PushFront(const AudioVector<T>& prepend_this) {
  vector_.insert(vector_.begin(), prepend_this.vector_.begin(),
                 prepend_this.vector_.end());
}

template<typename T>
void AudioVector<T>::PushFront(const T* prepend_this, size_t length) {
  
  InsertAt(prepend_this, length, 0);
}

template<typename T>
void AudioVector<T>::PushBack(const AudioVector<T>& append_this) {
  vector_.reserve(vector_.size() + append_this.Size());
  for (size_t i = 0; i < append_this.Size(); ++i) {
    vector_.push_back(append_this[i]);
  }
}

template<typename T>
void AudioVector<T>::PushBack(const T* append_this, size_t length) {
  vector_.reserve(vector_.size() + length);
  for (size_t i = 0; i < length; ++i) {
    vector_.push_back(append_this[i]);
  }
}

template<typename T>
void AudioVector<T>::PopFront(size_t length) {
  if (length >= vector_.size()) {
    
    vector_.clear();
  } else {
    typename std::vector<T>::iterator end_range = vector_.begin();
    end_range += length;
    
    
    vector_.erase(vector_.begin(), end_range);
  }
}

template<typename T>
void AudioVector<T>::PopBack(size_t length) {
  
  size_t new_size = vector_.size() - std::min(length, vector_.size());
  vector_.resize(new_size);
}

template<typename T>
void AudioVector<T>::Extend(size_t extra_length) {
  vector_.insert(vector_.end(), extra_length, 0);
}

template<typename T>
void AudioVector<T>::InsertAt(const T* insert_this,
                              size_t length,
                              size_t position) {
  typename std::vector<T>::iterator insert_position = vector_.begin();
  
  
  position = std::min(vector_.size(), position);
  insert_position += position;
  
  
  vector_.insert(insert_position, length, 0);
  
  for (size_t i = 0; i < length; ++i) {
    vector_[position + i] = insert_this[i];
  }
}

template<typename T>
void AudioVector<T>::InsertZerosAt(size_t length,
                                   size_t position) {
  typename std::vector<T>::iterator insert_position = vector_.begin();
  
  
  position = std::min(vector_.size(), position);
  insert_position += position;
  
  
  vector_.insert(insert_position, length, 0);
}

template<typename T>
void AudioVector<T>::OverwriteAt(const T* insert_this,
                                 size_t length,
                                 size_t position) {
  
  position = std::min(vector_.size(), position);
  
  
  if (position + length > vector_.size()) {
    Extend(position + length - vector_.size());
  }
  for (size_t i = 0; i < length; ++i) {
    vector_[position + i] = insert_this[i];
  }
}

template<typename T>
void AudioVector<T>::CrossFade(const AudioVector<T>& append_this,
                               size_t fade_length) {
  
  assert(fade_length <= Size());
  assert(fade_length <= append_this.Size());
  fade_length = std::min(fade_length, Size());
  fade_length = std::min(fade_length, append_this.Size());
  size_t position = Size() - fade_length;
  
  
  
  
  int alpha_step = 16384 / (static_cast<int>(fade_length) + 1);
  int alpha = 16384;
  for (size_t i = 0; i < fade_length; ++i) {
    alpha -= alpha_step;
    vector_[position + i] = (alpha * vector_[position + i] +
        (16384 - alpha) * append_this[i] + 8192) >> 14;
  }
  assert(alpha >= 0);  
  
  size_t samples_to_push_back = append_this.Size() - fade_length;
  if (samples_to_push_back > 0)
    PushBack(&append_this[fade_length], samples_to_push_back);
}




template<>
void AudioVector<double>::CrossFade(const AudioVector<double>& append_this,
                                    size_t fade_length) {
  
  assert(fade_length <= Size());
  assert(fade_length <= append_this.Size());
  fade_length = std::min(fade_length, Size());
  fade_length = std::min(fade_length, append_this.Size());
  size_t position = Size() - fade_length;
  
  
  
  
  int alpha_step = 16384 / (static_cast<int>(fade_length) + 1);
  int alpha = 16384;
  for (size_t i = 0; i < fade_length; ++i) {
    alpha -= alpha_step;
    vector_[position + i] = (alpha * vector_[position + i] +
        (16384 - alpha) * append_this[i]) / 16384;
  }
  assert(alpha >= 0);  
  
  size_t samples_to_push_back = append_this.Size() - fade_length;
  if (samples_to_push_back > 0)
    PushBack(&append_this[fade_length], samples_to_push_back);
}

template<typename T>
const T& AudioVector<T>::operator[](size_t index) const {
  return vector_[index];
}

template<typename T>
T& AudioVector<T>::operator[](size_t index) {
  return vector_[index];
}


template class AudioVector<int16_t>;
template class AudioVector<int32_t>;
template class AudioVector<double>;

}  
