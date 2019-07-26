



















#ifndef STRINGS_STRINGPIECE_H_
#define STRINGS_STRINGPIECE_H_

#include <string.h>
#include <string>


typedef int stringpiece_ssize_type;

class StringPiece {
 private:
  const char* ptr_;
  stringpiece_ssize_type length_;

 public:
  
  
  
  StringPiece() : ptr_(NULL), length_(0) {}

  StringPiece(const char* str)  
      : ptr_(str), length_(0) {
    if (str != NULL) {
      length_ = strlen(str);
    }
  }

  StringPiece(const std::string& str)  
      : ptr_(str.data()), length_(0) {
    length_ = str.size();
  }

  StringPiece(const char* offset, stringpiece_ssize_type len)
      : ptr_(offset), length_(len) {
  }

  void remove_prefix(stringpiece_ssize_type n) {
    ptr_ += n;
    length_ -= n;
  }

  void remove_suffix(stringpiece_ssize_type n) {
    length_ -= n;
  }

  
  
  
  
  const char* data() const { return ptr_; }
  stringpiece_ssize_type size() const { return length_; }
  stringpiece_ssize_type length() const { return length_; }
  bool empty() const { return length_ == 0; }
};

class StringPiece;

#endif  
