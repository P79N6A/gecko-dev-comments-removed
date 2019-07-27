





































#ifndef GOOGLE_PROTOBUF_GENERATED_ENUM_REFLECTION_H__
#define GOOGLE_PROTOBUF_GENERATED_ENUM_REFLECTION_H__

#include <string>

#include <google/protobuf/stubs/template_util.h>

namespace google {
namespace protobuf {
  class EnumDescriptor;
}  

namespace protobuf {



template <typename T> struct is_proto_enum : ::google::protobuf::internal::false_type {};




template <typename E>
const EnumDescriptor* GetEnumDescriptor();

namespace internal {




LIBPROTOBUF_EXPORT bool ParseNamedEnum(const EnumDescriptor* descriptor,
                    const string& name,
                    int* value);

template<typename EnumType>
bool ParseNamedEnum(const EnumDescriptor* descriptor,
                    const string& name,
                    EnumType* value) {
  int tmp;
  if (!ParseNamedEnum(descriptor, name, &tmp)) return false;
  *value = static_cast<EnumType>(tmp);
  return true;
}




LIBPROTOBUF_EXPORT const string& NameOfEnum(const EnumDescriptor* descriptor, int value);

}  
}  

}  
#endif  
