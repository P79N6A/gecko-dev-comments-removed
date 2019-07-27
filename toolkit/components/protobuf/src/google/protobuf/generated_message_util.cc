

































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

const ::std::string* empty_string_;
GOOGLE_PROTOBUF_DECLARE_ONCE(empty_string_once_init_);

void DeleteEmptyString() {
  delete empty_string_;
}

void InitEmptyString() {
  empty_string_ = new string;
  OnShutdown(&DeleteEmptyString);
}


}  
}  
}  
