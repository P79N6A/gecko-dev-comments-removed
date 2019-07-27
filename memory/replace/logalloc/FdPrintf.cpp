





#include <cstdarg>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <cstring>
#include "mozilla/Assertions.h"
#include "mozilla/NullPtr.h"


template <typename T>
class CheckedIncrement
{
public:
  CheckedIncrement(T aValue, size_t aMaxIncrement)
    : mValue(aValue), mMaxIncrement(aMaxIncrement)
  {}

  T operator ++(int)
  {
    if (!mMaxIncrement) {
      MOZ_CRASH("overflow detected");
    }
    mMaxIncrement--;
    return mValue++;
  }

  T& operator ++()
  {
    (*this)++;
    return mValue;
  }

  operator T() { return mValue; }

private:
  T mValue;
  size_t mMaxIncrement;
};

void
FdPrintf(intptr_t aFd, const char* aFormat, ...)
{
  if (aFd == 0) {
    return;
  }
  char buf[256];
  CheckedIncrement<char*> b(buf, sizeof(buf));
  CheckedIncrement<const char*> f(aFormat, strlen(aFormat) + 1);
  va_list ap;
  va_start(ap, aFormat);
  while (true) {
    switch (*f) {
      case '\0':
        goto out;

      case '%':
        switch (*++f) {
          case 'z': {
            if (*(++f) == 'u') {
              size_t i = va_arg(ap, size_t);
              size_t x = 1;
              
              while (x <= i / 10) {
                x *= 10;
              }
              
              do {
                *(b++) = "0123456789"[(i / x) % 10];
                x /= 10;
              } while (x > 0);
            } else {
              
              *(b++) = '%';
              *(b++) = 'z';
              *(b++) = *f;
            }
            break;
          }

          case 'p': {
            intptr_t ptr = va_arg(ap, intptr_t);
            *(b++) = '0';
            *(b++) = 'x';
            int x = sizeof(intptr_t) * 8;
            bool wrote_msb = false;
            do {
              x -= 4;
              size_t hex_digit = ptr >> x & 0xf;
              if (hex_digit || wrote_msb) {
                *(b++) = "0123456789abcdef"[hex_digit];
                wrote_msb = true;
              }
            } while (x > 0);
            if (!wrote_msb) {
              *(b++) = '0';
            }
            break;
          }

          default:
            
            *(b++) = '%';
            *(b++) = *f;
            break;
        }
        break;

      default:
        *(b++) = *f;
        break;
    }
    f++;
  }
out:
#ifdef _WIN32
  
  DWORD written;
  WriteFile(reinterpret_cast<HANDLE>(aFd), buf, b - buf, &written, nullptr);
#else
  write(aFd, buf, b - buf);
#endif
  va_end(ap);
}
