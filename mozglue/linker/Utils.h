



#ifndef Utils_h
#define Utils_h

#include <stdint.h>






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




class AutoCloseFD
{
public:
  AutoCloseFD(): fd(-1) { }
  AutoCloseFD(int fd): fd(fd) { }
  ~AutoCloseFD()
  {
    if (fd != -1)
      close(fd);
  }

  operator int() const
  {
    return fd;
  }

  int forget()
  {
    int _fd = fd;
    fd = -1;
    return _fd;
  }

  bool operator ==(int other) const
  {
    return fd == other;
  }

  int operator =(int other)
  {
    if (fd != -1)
      close(fd);
    fd = other;
    return fd;
  }

private:
  int fd;
};

#endif 
