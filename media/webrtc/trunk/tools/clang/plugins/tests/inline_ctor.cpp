



#include "inline_ctor.h"

#include <string>
#include <vector>


class InlineInCPPOK {
 public:
  InlineInCPPOK() {}
  ~InlineInCPPOK() {}

 private:
  std::vector<int> one_;
  std::vector<std::string> two_;
};

int main() {
  InlineInCPPOK one;
  InlineCtorsArentOKInHeader two;
  return 0;
}
