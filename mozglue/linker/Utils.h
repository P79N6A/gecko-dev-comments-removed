



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
  static FILE *empty() { return NULL; }
  static void release(FILE *f) { if (f) fclose(f); }
};
typedef mozilla::Scoped<AutoCloseFILETraits> AutoCloseFILE;








template <typename T>
class GenericMappedPtr
{
public:
  GenericMappedPtr(void *buf, size_t length): buf(buf), length(length) { }
  GenericMappedPtr(): buf(MAP_FAILED), length(0) { }

  void Assign(void *b, size_t len) {
    if (buf != MAP_FAILED)
      static_cast<T *>(this)->munmap(buf, length);
    buf = b;
    length = len;
  }

  ~GenericMappedPtr()
  {
    if (buf != MAP_FAILED)
      static_cast<T *>(this)->munmap(buf, length);
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

private:
  void *buf;
  size_t length;
};

struct MappedPtr: public GenericMappedPtr<MappedPtr>
{
  MappedPtr(void *buf, size_t length)
  : GenericMappedPtr<MappedPtr>(buf, length) { }
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

  


  UnsizedArray(): contents(NULL) { }
  UnsizedArray(const void *buf): contents(reinterpret_cast<const T *>(buf)) { }

  void Init(const void *buf)
  {
    MOZ_ASSERT(contents == NULL);
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
    return contents != NULL;
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
    iterator(): item(NULL) { }

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
    reverse_iterator(): item(NULL) { }

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
 
