



#ifndef Utils_h
#define Utils_h

#include <stdint.h>
#include <stddef.h>
#include <sys/mman.h>
#include <unistd.h>






#if defined(__i386__) || defined(__x86_64__)
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
  operator typename UInt<16 * sizeof(T)>::Type() const
  {
    return (b << (sizeof(T) * 8)) | a;
  }
private:
  T a, b;
};




typedef le_to_cpu<unsigned char> le_uint16;
typedef le_to_cpu<le_uint16> le_uint32;
#endif













template <typename Traits>
class AutoClean
{
  typedef typename Traits::type T;
public:
  AutoClean(): value(Traits::None()) { }
  AutoClean(const T& value): value(value) { }
  ~AutoClean()
  {
    if (value != Traits::None())
      Traits::clean(value);
  }

  operator const T&() const { return value; }
  const T& operator->() const { return value; }
  const T& get() const { return value; }

  T forget()
  {
    T _value = value;
    value = Traits::None();
    return _value;
  }

  bool operator ==(T other) const
  {
    return value == other;
  }

  AutoClean& operator =(T other)
  {
    if (value != Traits::None())
      Traits::clean(value);
    value = other;
    return *this;
  }

private:
  T value;
};





#define AUTOCLEAN_TEMPLATE(name, Traits) \
template <typename T> \
struct name: public AutoClean<Traits<T> > \
{ \
  using AutoClean<Traits<T> >::operator =; \
  name(): AutoClean<Traits<T> >() { } \
  name(typename Traits<T>::type ptr): AutoClean<Traits<T> >(ptr) { } \
}




struct AutoCloseFDTraits
{
  typedef int type;
  static int None() { return -1; }
  static void clean(int fd) { close(fd); }
};
typedef AutoClean<AutoCloseFDTraits> AutoCloseFD;








template <typename T>
struct AutoFreePtrTraits
{
  typedef T *type;
  static T *None() { return NULL; }
  static void clean(T *ptr) { free(ptr); }
};
AUTOCLEAN_TEMPLATE(AutoFreePtr, AutoFreePtrTraits);







template <typename T>
struct AutoDeletePtrTraits: public AutoFreePtrTraits<T>
{
  static void clean(T *ptr) { delete ptr; }
};
AUTOCLEAN_TEMPLATE(AutoDeletePtr, AutoDeletePtrTraits);







template <typename T>
struct AutoDeleteArrayTraits: public AutoFreePtrTraits<T>
{
  static void clean(T *ptr) { delete [] ptr; }
};
AUTOCLEAN_TEMPLATE(AutoDeleteArray, AutoDeleteArrayTraits);








template <typename T>
class GenericMappedPtr
{
public:
  GenericMappedPtr(void *buf, size_t length): buf(buf), length(length) { }
  GenericMappedPtr(): buf(MAP_FAILED), length(0) { }

  void Init(void *b, size_t len) {
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
    
    contents = reinterpret_cast<const T *>(buf);
  }

  


  const T &operator[](const idx_t index) const
  {
    
    return contents[index];
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

    const T &operator ++()
    {
      return *(++item);
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
 
