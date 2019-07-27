




































#ifndef GOOGLE_PROTOBUF_GENERATED_MESSAGE_UTIL_H__
#define GOOGLE_PROTOBUF_GENERATED_MESSAGE_UTIL_H__

#include <assert.h>
#include <string>

#include <google/protobuf/stubs/once.h>

#include <google/protobuf/stubs/common.h>
namespace google {

namespace protobuf {
namespace internal {








#undef DEPRECATED_PROTOBUF_FIELD
#define PROTOBUF_DEPRECATED



LIBPROTOBUF_EXPORT double Infinity();
LIBPROTOBUF_EXPORT double NaN();








LIBPROTOBUF_EXPORT extern const ::std::string* empty_string_;
LIBPROTOBUF_EXPORT extern ProtobufOnceType empty_string_once_init_;
LIBPROTOBUF_EXPORT void InitEmptyString();


LIBPROTOBUF_EXPORT inline const ::std::string& GetEmptyStringAlreadyInited() {
  assert(empty_string_ != NULL);
  return *empty_string_;
}
LIBPROTOBUF_EXPORT inline const ::std::string& GetEmptyString() {
  ::google::protobuf::GoogleOnceInit(&empty_string_once_init_, &InitEmptyString);
  return GetEmptyStringAlreadyInited();
}







LIBPROTOBUF_EXPORT int StringSpaceUsedExcludingSelf(const string& str);







template <class Type> bool AllAreInitialized(const Type& t) {
  for (int i = t.size(); --i >= 0; ) {
    if (!t.Get(i).IsInitialized()) return false;
  }
  return true;
}

}  
}  

}  
#endif  
