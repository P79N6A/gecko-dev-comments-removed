



#ifndef MISSING_CTOR_H_
#define MISSING_CTOR_H_

#include <string>
#include <vector>

class MissingCtorsArentOKInHeader {
 public:

 private:
  std::vector<int> one_;
  std::vector<std::string> two_;
};

#endif  
