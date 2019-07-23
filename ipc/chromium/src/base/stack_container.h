



#ifndef BASE_STACK_CONTAINER_H_
#define BASE_STACK_CONTAINER_H_

#include <string>
#include <vector>

#include "base/basictypes.h"



















template<typename T, size_t stack_capacity>
class StackAllocator : public std::allocator<T> {
 public:
  typedef typename std::allocator<T>::pointer pointer;
  typedef typename std::allocator<T>::size_type size_type;

  
  
  
  struct Source {
    Source() : used_stack_buffer_(false) {
    }

    
    T* stack_buffer() { return reinterpret_cast<T*>(stack_buffer_); }
    const T* stack_buffer() const {
      return reinterpret_cast<const T*>(stack_buffer_);
    }

    
    
    
    
    
    

    
    
    
    char stack_buffer_[sizeof(T[stack_capacity])];

    
    
    bool used_stack_buffer_;
  };

  
  template<typename U>
  struct rebind {
    typedef StackAllocator<U, stack_capacity> other;
  };

  
  StackAllocator(const StackAllocator<T, stack_capacity>& rhs)
      : source_(rhs.source_) {
  }

  
  
  
  
  
  
  
  
  
  template<typename U, size_t other_capacity>
  StackAllocator(const StackAllocator<U, other_capacity>& other)
      : source_(NULL) {
  }

  explicit StackAllocator(Source* source) : source_(source) {
  }

  
  
  
  pointer allocate(size_type n, void* hint = 0) {
    if (source_ != NULL && !source_->used_stack_buffer_
        && n <= stack_capacity) {
      source_->used_stack_buffer_ = true;
      return source_->stack_buffer();
    } else {
      return std::allocator<T>::allocate(n, hint);
    }
  }

  
  
  void deallocate(pointer p, size_type n) {
    if (source_ != NULL && p == source_->stack_buffer())
      source_->used_stack_buffer_ = false;
    else
      std::allocator<T>::deallocate(p, n);
  }

 private:
  Source* source_;
};









template<typename TContainerType, int stack_capacity>
class StackContainer {
 public:
  typedef TContainerType ContainerType;
  typedef typename ContainerType::value_type ContainedType;
  typedef StackAllocator<ContainedType, stack_capacity> Allocator;

  
  StackContainer() : allocator_(&stack_data_), container_(allocator_) {
    
    
    container_.reserve(stack_capacity);
  }

  
  
  
  
  
  
  ContainerType& container() { return container_; }
  const ContainerType& container() const { return container_; }

  
  
  
  ContainerType* operator->() { return &container_; }
  const ContainerType* operator->() const { return &container_; }

#ifdef UNIT_TEST
  
  
  const typename Allocator::Source& stack_data() const {
    return stack_data_;
  }
#endif

 protected:
  typename Allocator::Source stack_data_;
  Allocator allocator_;
  ContainerType container_;

  DISALLOW_EVIL_CONSTRUCTORS(StackContainer);
};


template<size_t stack_capacity>
class StackString : public StackContainer<
    std::basic_string<char,
                      std::char_traits<char>,
                      StackAllocator<char, stack_capacity> >,
    stack_capacity> {
 public:
  StackString() : StackContainer<
      std::basic_string<char,
                        std::char_traits<char>,
                        StackAllocator<char, stack_capacity> >,
      stack_capacity>() {
  }

 private:
  DISALLOW_EVIL_CONSTRUCTORS(StackString);
};


template<size_t stack_capacity>
class StackWString : public StackContainer<
    std::basic_string<wchar_t,
                      std::char_traits<wchar_t>,
                      StackAllocator<wchar_t, stack_capacity> >,
    stack_capacity> {
 public:
  StackWString() : StackContainer<
      std::basic_string<wchar_t,
                        std::char_traits<wchar_t>,
                        StackAllocator<wchar_t, stack_capacity> >,
      stack_capacity>() {
  }

 private:
  DISALLOW_EVIL_CONSTRUCTORS(StackWString);
};







template<typename T, size_t stack_capacity>
class StackVector : public StackContainer<
    std::vector<T, StackAllocator<T, stack_capacity> >,
    stack_capacity> {
 public:
  StackVector() : StackContainer<
      std::vector<T, StackAllocator<T, stack_capacity> >,
      stack_capacity>() {
  }

  
  
  
  
  StackVector(const StackVector<T, stack_capacity>& other)
      : StackContainer<
            std::vector<T, StackAllocator<T, stack_capacity> >,
            stack_capacity>() {
    this->container().assign(other->begin(), other->end());
  }

  StackVector<T, stack_capacity>& operator=(
      const StackVector<T, stack_capacity>& other) {
    this->container().assign(other->begin(), other->end());
    return *this;
  }

  
  
  T& operator[](size_t i) { return this->container().operator[](i); }
  const T& operator[](size_t i) const {
    return this->container().operator[](i);
  }
};

#endif  
