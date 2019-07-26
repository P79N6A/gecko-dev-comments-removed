



#ifndef Utils_h
#define Utils_h

#include <stdint.h>
#include <stddef.h>
#include <sys/mman.h>
#include <unistd.h>
#include "mozilla/Assertions.h"
#include "mozilla/Scoped.h"






#if !defined(DEBUG) && (defined(__i386__) || defined(__x86_64__))
typedef uint16_t le_uint16;
typedef uint32_t le_uint32;
#else




template <int s> struct UInt { };
template <> struct UInt<16> { typedef uint16_t Type; };
template <> struct UInt<32> { typedef uint32_t Type; };





template <typename T>
class le_to_cpu
{
public:
  typedef typename UInt<16 * sizeof(T)>::Type Type;

  operator Type() const
  {
    return (b << (sizeof(T) * 8)) | a;
  }

  const le_to_cpu& operator =(const Type &v)
  {
    a = v & ((1 << (sizeof(T) * 8)) - 1);
    b = v >> (sizeof(T) * 8);
    return *this;
  }

  le_to_cpu() { }
  le_to_cpu(const Type &v)
  {
    operator =(v);
  }

  const le_to_cpu& operator +=(const Type &v)
  {
    return operator =(operator Type() + v);
  }

  const le_to_cpu& operator ++(int)
  {
    return operator =(operator Type() + 1);
  }

private:
  T a, b;
};




typedef le_to_cpu<unsigned char> le_uint16;
typedef le_to_cpu<le_uint16> le_uint32;
#endif





struct AutoCloseFDTraits
{
  typedef int type;
  static int empty() { return -1; }
  static void release(int fd) { if (fd != -1) close(fd); }
};
typedef mozilla::Scoped<AutoCloseFDTraits> AutoCloseFD;




struct AutoCloseFILETraits
{
  typedef FILE *type;
  static FILE *empty() { return nullptr; }
  static void release(FILE *f) { if (f) fclose(f); }
};
typedef mozilla::Scoped<AutoCloseFILETraits> AutoCloseFILE;




static inline size_t PageSize()
{
  return 4096;
}

static inline uintptr_t AlignedPtr(uintptr_t ptr, size_t alignment)
{
  return ptr & ~(alignment - 1);
}

template <typename T>
static inline T *AlignedPtr(T *ptr, size_t alignment)
{
  return reinterpret_cast<T *>(
         AlignedPtr(reinterpret_cast<uintptr_t>(ptr), alignment));
}

template <typename T>
static inline T PageAlignedPtr(T ptr)
{
  return AlignedPtr(ptr, PageSize());
}

static inline uintptr_t AlignedEndPtr(uintptr_t ptr, size_t alignment)
{
  return AlignedPtr(ptr + alignment - 1, alignment);
}

template <typename T>
static inline T *AlignedEndPtr(T *ptr, size_t alignment)
{
  return reinterpret_cast<T *>(
         AlignedEndPtr(reinterpret_cast<uintptr_t>(ptr), alignment));
}

template <typename T>
static inline T PageAlignedEndPtr(T ptr)
{
  return AlignedEndPtr(ptr,  PageSize());
}

static inline size_t AlignedSize(size_t size, size_t alignment)
{
  return (size + alignment - 1) & ~(alignment - 1);
}

static inline size_t PageAlignedSize(size_t size)
{
  return AlignedSize(size, PageSize());
}

static inline bool IsAlignedPtr(uintptr_t ptr, size_t alignment)
{
  return ptr % alignment == 0;
}

template <typename T>
static inline bool IsAlignedPtr(T *ptr, size_t alignment)
{
  return IsAlignedPtr(reinterpret_cast<uintptr_t>(ptr), alignment);
}

template <typename T>
static inline bool IsPageAlignedPtr(T ptr)
{
  return IsAlignedPtr(ptr, PageSize());
}

static inline bool IsAlignedSize(size_t size, size_t alignment)
{
  return size % alignment == 0;
}

static inline bool IsPageAlignedSize(size_t size)
{
  return IsAlignedSize(size, PageSize());
}

static inline size_t PageNumber(size_t size)
{
  return (size + PageSize() - 1) / PageSize();
}




class MemoryRange
{
public:
  MemoryRange(void *buf, size_t length): buf(buf), length(length) { }

  void Assign(void *b, size_t len) {
    buf = b;
    length = len;
  }

  void Assign(const MemoryRange& other) {
    buf = other.buf;
    length = other.length;
  }

  void *get() const
  {
    return buf;
  }

  operator void *() const
  {
    return buf;
  }

  operator unsigned char *() const
  {
    return reinterpret_cast<unsigned char *>(buf);
  }

  bool operator ==(void *ptr) const {
    return buf == ptr;
  }

  bool operator ==(unsigned char *ptr) const {
    return buf == ptr;
  }

  void *operator +(off_t offset) const
  {
    return reinterpret_cast<char *>(buf) + offset;
  }

  


  bool Contains(void *ptr) const
  {
    return (ptr >= buf) && (ptr < reinterpret_cast<char *>(buf) + length);
  }

  


  size_t GetLength() const
  {
    return length;
  }

  static MemoryRange mmap(void *addr, size_t length, int prot, int flags,
                          int fd, off_t offset) {
    return MemoryRange(::mmap(addr, length, prot, flags, fd, offset), length);
  }

private:
  void *buf;
  size_t length;
};








template <typename T>
class GenericMappedPtr: public MemoryRange
{
public:
  GenericMappedPtr(void *buf, size_t length): MemoryRange(buf, length) { }
  GenericMappedPtr(const MemoryRange& other): MemoryRange(other) { }
  GenericMappedPtr(): MemoryRange(MAP_FAILED, 0) { }

  void Assign(void *b, size_t len) {
    if (get() != MAP_FAILED)
      static_cast<T *>(this)->munmap(get(), GetLength());
    MemoryRange::Assign(b, len);
  }

  void Assign(const MemoryRange& other) {
    Assign(other.get(), other.GetLength());
  }

  ~GenericMappedPtr()
  {
    if (get() != MAP_FAILED)
      static_cast<T *>(this)->munmap(get(), GetLength());
  }

};

struct MappedPtr: public GenericMappedPtr<MappedPtr>
{
  MappedPtr(void *buf, size_t length)
  : GenericMappedPtr<MappedPtr>(buf, length) { }
  MappedPtr(const MemoryRange& other)
  : GenericMappedPtr<MappedPtr>(other) { }
  MappedPtr(): GenericMappedPtr<MappedPtr>() { }

private:
  friend class GenericMappedPtr<MappedPtr>;
  void munmap(void *buf, size_t length)
  {
    ::munmap(buf, length);
  }
};















template <typename T>
class UnsizedArray
{
public:
  typedef size_t idx_t;

  


  UnsizedArray(): contents(nullptr) { }
  UnsizedArray(const void *buf): contents(reinterpret_cast<const T *>(buf)) { }

  void Init(const void *buf)
  {
    MOZ_ASSERT(contents == nullptr);
    contents = reinterpret_cast<const T *>(buf);
  }

  


  const T &operator[](const idx_t index) const
  {
    MOZ_ASSERT(contents);
    return contents[index];
  }

  operator const T *() const
  {
    return contents;
  }
  


  operator bool() const
  {
    return contents != nullptr;
  }
private:
  const T *contents;
};























template <typename T>
class Array: public UnsizedArray<T>
{
public:
  typedef typename UnsizedArray<T>::idx_t idx_t;

  


  Array(): UnsizedArray<T>(), length(0) { }
  Array(const void *buf, const idx_t length)
  : UnsizedArray<T>(buf), length(length) { }

  void Init(const void *buf)
  {
    UnsizedArray<T>::Init(buf);
  }

  void Init(const idx_t len)
  {
    MOZ_ASSERT(length == 0);
    length = len;
  }

  void InitSize(const idx_t size)
  {
    Init(size / sizeof(T));
  }

  void Init(const void *buf, const idx_t len)
  {
    UnsizedArray<T>::Init(buf);
    Init(len);
  }

  void InitSize(const void *buf, const idx_t size)
  {
    UnsizedArray<T>::Init(buf);
    InitSize(size);
  }

  


  const T &operator[](const idx_t index) const
  {
    MOZ_ASSERT(index < length);
    MOZ_ASSERT(operator bool());
    return UnsizedArray<T>::operator[](index);
  }

  


  idx_t numElements() const
  {
    return length;
  }

  


  operator bool() const
  {
    return (length > 0) && UnsizedArray<T>::operator bool();
  }

  








  class iterator
  {
  public:
    iterator(): item(nullptr) { }

    const T &operator *() const
    {
      return *item;
    }

    const T *operator ->() const
    {
      return item;
    }

    iterator &operator ++()
    {
      ++item;
      return *this;
    }

    bool operator<(const iterator &other) const
    {
      return item < other.item;
    }
  protected:
    friend class Array<T>;
    iterator(const T &item): item(&item) { }

  private:
    const T *item;
  };

  


  iterator begin() const {
    if (length)
      return iterator(UnsizedArray<T>::operator[](0));
    return iterator();
  }

  


  iterator end() const {
    if (length)
      return iterator(UnsizedArray<T>::operator[](length));
    return iterator();
  }

  









  class reverse_iterator
  {
  public:
    reverse_iterator(): item(nullptr) { }

    const T &operator *() const
    {
      const T *tmp = item;
      return *--tmp;
    }

    const T *operator ->() const
    {
      return &operator*();
    }

    reverse_iterator &operator ++()
    {
      --item;
      return *this;
    }

    bool operator<(const reverse_iterator &other) const
    {
      return item > other.item;
    }
  protected:
    friend class Array<T>;
    reverse_iterator(const T &item): item(&item) { }

  private:
    const T *item;
  };

  


  reverse_iterator rbegin() const {
    if (length)
      return reverse_iterator(UnsizedArray<T>::operator[](length));
    return reverse_iterator();
  }

  


  reverse_iterator rend() const {
    if (length)
      return reverse_iterator(UnsizedArray<T>::operator[](0));
    return reverse_iterator();
  }
private:
  idx_t length;
};





template <typename T>
void *FunctionPtr(T func)
{
  union {
    void *ptr;
    T func;
  } f;
  f.func = func;
  return f.ptr;
}

#endif 
 
