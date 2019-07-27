






#ifndef CHROME_COMMON_NOTIFICATION_DETAILS_H__
#define CHROME_COMMON_NOTIFICATION_DETAILS_H__

#include "base/basictypes.h"




class NotificationDetails {
 public:
  NotificationDetails() : ptr_(NULL) {}
  NotificationDetails(const NotificationDetails& other) : ptr_(other.ptr_) {}
  ~NotificationDetails() {}

  
  
  
  uintptr_t map_key() const { return reinterpret_cast<uintptr_t>(ptr_); }

  bool operator!=(const NotificationDetails& other) const {
    return ptr_ != other.ptr_;
  }

  bool operator==(const NotificationDetails& other) const {
    return ptr_ == other.ptr_;
  }

 protected:
  explicit NotificationDetails(void* ptr) : ptr_(ptr) {}

  void* ptr_;
};

template <class T>
class Details : public NotificationDetails {
 public:
  explicit Details(T* ptr) : NotificationDetails(ptr) {}
  explicit Details(const NotificationDetails& other)
    : NotificationDetails(other) {}

  T* operator->() const { return ptr(); }
  T* ptr() const { return static_cast<T*>(ptr_); }
};

#endif  
