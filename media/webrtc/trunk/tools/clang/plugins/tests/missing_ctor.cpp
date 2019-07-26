



#include "missing_ctor.h"

#include <string>
#include <vector>


class MissingInCPPOK {
 public:

 private:
  std::vector<int> one_;
  std::vector<std::string> two_;
};

int main() {
  MissingInCPPOK one;
  MissingCtorsArentOKInHeader two;
  return 0;
}
