




























#ifndef GOOGLE_BREAKPAD_COMMON_MEMORY_H_
#define GOOGLE_BREAKPAD_COMMON_MEMORY_H_

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

#ifdef __APPLE__
#define sys_mmap mmap
#define sys_mmap2 mmap
#define sys_munmap munmap
#define MAP_ANONYMOUS MAP_ANON
#else
#include "third_party/lss/linux_syscall_support.h"
#endif

namespace google_breakpad {






class PageAllocator {
 public:
  PageAllocator()
      : page_size_(getpagesize()),
        last_(NULL),
        current_page_(NULL),
        page_offset_(0) {
  }

  ~PageAllocator() {
    FreeAll();
  }

  void *Alloc(unsigned bytes) {
    if (!bytes)
      return NULL;

    if (current_page_ && page_size_ - page_offset_ >= bytes) {
      uint8_t *const ret = current_page_ + page_offset_;
      page_offset_ += bytes;
      if (page_offset_ == page_size_) {
        page_offset_ = 0;
        current_page_ = NULL;
      }

      return ret;
    }

    const unsigned pages =
        (bytes + sizeof(PageHeader) + page_size_ - 1) / page_size_;
    uint8_t *const ret = GetNPages(pages);
    if (!ret)
      return NULL;

    page_offset_ = (page_size_ - (page_size_ * pages - (bytes + sizeof(PageHeader)))) % page_size_;
    current_page_ = page_offset_ ? ret + page_size_ * (pages - 1) : NULL;

    return ret + sizeof(PageHeader);
  }

 private:
  uint8_t *GetNPages(unsigned num_pages) {
#ifdef __x86_64
    void *a = sys_mmap(NULL, page_size_ * num_pages, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#else
    void *a = sys_mmap2(NULL, page_size_ * num_pages, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
    if (a == MAP_FAILED)
      return NULL;

    struct PageHeader *header = reinterpret_cast<PageHeader*>(a);
    header->next = last_;
    header->num_pages = num_pages;
    last_ = header;

    return reinterpret_cast<uint8_t*>(a);
  }

  void FreeAll() {
    PageHeader *next;

    for (PageHeader *cur = last_; cur; cur = next) {
      next = cur->next;
      sys_munmap(cur, cur->num_pages * page_size_);
    }
  }

  struct PageHeader {
    PageHeader *next;  
    unsigned num_pages;  
  };

  const unsigned page_size_;
  PageHeader *last_;
  uint8_t *current_page_;
  unsigned page_offset_;
};





template<class T>
class wasteful_vector {
 public:
  wasteful_vector(PageAllocator *allocator, unsigned size_hint = 16)
      : allocator_(allocator),
        a_((T*) allocator->Alloc(sizeof(T) * size_hint)),
        allocated_(size_hint),
        used_(0) {
  }

  T& back() {
    return a_[used_ - 1];
  }

  const T& back() const {
    return a_[used_ - 1];
  }

  bool empty() const {
    return used_ == 0;
  }

  void push_back(const T& new_element) {
    if (used_ == allocated_)
      Realloc(allocated_ * 2);
    a_[used_++] = new_element;
  }

  size_t size() const {
    return used_;
  }

  void resize(unsigned sz, T c = T()) {
    
    if (sz <= used_) {
      used_ = sz;
    } else {
      unsigned a = allocated_;
      if (sz > a) {
        while (sz > a) {
          a *= 2;
        }
        Realloc(a);
      }
      while (sz > used_) {
        a_[used_++] = c;
      }
    }
  }

  T& operator[](size_t index) {
    return a_[index];
  }

  const T& operator[](size_t index) const {
    return a_[index];
  }

 private:
  void Realloc(unsigned new_size) {
    T *new_array =
        reinterpret_cast<T*>(allocator_->Alloc(sizeof(T) * new_size));
    memcpy(new_array, a_, used_ * sizeof(T));
    a_ = new_array;
    allocated_ = new_size;
  }

  PageAllocator *const allocator_;
  T *a_;  
  unsigned allocated_;  
  unsigned used_;  
};

}  

inline void* operator new(size_t nbytes,
                          google_breakpad::PageAllocator& allocator) {
   return allocator.Alloc(nbytes);
}

#endif  
