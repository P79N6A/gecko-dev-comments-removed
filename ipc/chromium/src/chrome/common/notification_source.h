






#ifndef CHROME_COMMON_NOTIFICATION_SOURCE_H__
#define CHROME_COMMON_NOTIFICATION_SOURCE_H__

#include "base/basictypes.h"




class NotificationSource {
 public:
  NotificationSource(const NotificationSource& other) : ptr_(other.ptr_) { }
  ~NotificationSource() {}

  
  
  
  uintptr_t map_key() const { return reinterpret_cast<uintptr_t>(ptr_); }

  bool operator!=(const NotificationSource& other) const {
    return ptr_ != other.ptr_;
  }
  bool operator==(const NotificationSource& other) const {
    return ptr_ == other.ptr_;
  }

 protected:
  NotificationSource(void* ptr) : ptr_(ptr) {}

  void* ptr_;
};

template <class T>
class Source : public NotificationSource {
 public:
  Source(T* ptr) : NotificationSource(ptr) {}

  Source(const NotificationSource& other)
    : NotificationSource(other) {}

  T* operator->() const { return ptr(); }
  T* ptr() const { return static_cast<T*>(ptr_); }
};

#endif  
