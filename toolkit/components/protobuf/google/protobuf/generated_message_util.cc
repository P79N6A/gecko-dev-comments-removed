

































#include <google/protobuf/generated_message_util.h>

#include <limits>

namespace google {
namespace protobuf {
namespace internal {

double Infinity() {
  return std::numeric_limits<double>::infinity();
}
double NaN() {
  return std::numeric_limits<double>::quiet_NaN();
}

const ::std::string kEmptyString;


}  
}  
}  
