



















































#ifndef PROCESSOR_LINKED_PTR_H__
#define PROCESSOR_LINKED_PTR_H__

namespace google_breakpad {








class linked_ptr_internal {
 public:
  
  void join_new() {
    next_ = this;
  }

  
  void join(linked_ptr_internal const* ptr) {
    linked_ptr_internal const* p = ptr;
    while (p->next_ != ptr) p = p->next_;
    p->next_ = this;
    next_ = ptr;
  }

  
  
  bool depart() {
    if (next_ == this) return true;
    linked_ptr_internal const* p = next_;
    while (p->next_ != this) p = p->next_;
    p->next_ = next_;
    return false;
  }

 private:
  mutable linked_ptr_internal const* next_;
};

template <typename T>
class linked_ptr {
 public:
  typedef T element_type;

  
  
  explicit linked_ptr(T* ptr = NULL) { capture(ptr); }
  ~linked_ptr() { depart(); }

  
  template <typename U> linked_ptr(linked_ptr<U> const& ptr) { copy(&ptr); }
  linked_ptr(linked_ptr const& ptr) { copy(&ptr); }

  
  template <typename U> linked_ptr& operator=(linked_ptr<U> const& ptr) {
    depart();
    copy(&ptr);
    return *this;
  }

  linked_ptr& operator=(linked_ptr const& ptr) {
    if (&ptr != this) {
      depart();
      copy(&ptr);
    }
    return *this;
  }

  
  void reset(T* ptr = NULL) { depart(); capture(ptr); }
  T* get() const { return value_; }
  T* operator->() const { return value_; }
  T& operator*() const { return *value_; }
  
  
  T* release() {
    link_.depart();
    T* v = value_;
    value_ = NULL;
    return v;
  }

  bool operator==(T* p) const { return value_ == p; }
  bool operator!=(T* p) const { return value_ != p; }
  template <typename U>
  bool operator==(linked_ptr<U> const& ptr) const {
    return value_ == ptr.get();
  }
  template <typename U>
  bool operator!=(linked_ptr<U> const& ptr) const {
    return value_ != ptr.get();
  }

 private:
  template <typename U>
  friend class linked_ptr;

  T* value_;
  linked_ptr_internal link_;

  void depart() {
    if (link_.depart()) delete value_;
  }

  void capture(T* ptr) {
    value_ = ptr;
    link_.join_new();
  }

  template <typename U> void copy(linked_ptr<U> const* ptr) {
    value_ = ptr->get();
    if (value_)
      link_.join(&ptr->link_);
    else
      link_.join_new();
  }
};

template<typename T> inline
bool operator==(T* ptr, const linked_ptr<T>& x) {
  return ptr == x.get();
}

template<typename T> inline
bool operator!=(T* ptr, const linked_ptr<T>& x) {
  return ptr != x.get();
}




template <typename T>
linked_ptr<T> make_linked_ptr(T* ptr) {
  return linked_ptr<T>(ptr);
}

}  

#endif  
