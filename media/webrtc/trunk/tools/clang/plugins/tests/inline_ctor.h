



#ifndef INLINE_CTOR_H_
#define INLINE_CTOR_H_

#include <string>
#include <vector>

class InlineCtorsArentOKInHeader {
 public:
  InlineCtorsArentOKInHeader() {}
  ~InlineCtorsArentOKInHeader() {}

 private:
  std::vector<int> one_;
  std::vector<std::string> two_;
};

#endif  
