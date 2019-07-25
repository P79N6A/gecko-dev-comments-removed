


























#include <stdarg.h>

#include "v8.h"

#include "platform.h"

#include "sys/stat.h"

namespace v8 {
namespace internal {

void StringBuilder::AddString(const char* s) {
  AddSubstring(s, StrLength(s));
}


void StringBuilder::AddSubstring(const char* s, int n) {
  ASSERT(!is_finalized() && position_ + n < buffer_.length());
  ASSERT(static_cast<size_t>(n) <= strlen(s));
  memcpy(&buffer_[position_], s, n * kCharSize);
  position_ += n;
}



void StringBuilder::AddInteger(int n) {
  ASSERT(!is_finalized() && position_ < buffer_.length());
  
  int ndigits = 0;
  int n2 = n;
  do {
    ndigits++;
    n2 /= 10; 
  } while (n2);

  
  position_ += ndigits;
  int i = position_;
  do {
    buffer_[--i] = '0' + (n % 10);
    n /= 10;
  } while (n);
}


void StringBuilder::AddPadding(char c, int count) {
  for (int i = 0; i < count; i++) {
    AddCharacter(c);
  }
}


char* StringBuilder::Finalize() {
  ASSERT(!is_finalized() && position_ < buffer_.length());
  buffer_[position_] = '\0';
  
  
  ASSERT(strlen(buffer_.start()) == static_cast<size_t>(position_));
  position_ = -1;
  ASSERT(is_finalized());
  return buffer_.start();
}

} }  
