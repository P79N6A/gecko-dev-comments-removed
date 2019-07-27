






































































#ifndef GOOGLE_PROTOBUF_STUBS_ONCE_H__
#define GOOGLE_PROTOBUF_STUBS_ONCE_H__

#include <google/protobuf/stubs/common.h>

#ifndef _WIN32
#include <pthread.h>
#endif

namespace google {
namespace protobuf {

#ifdef _WIN32

struct ProtobufOnceInternal;

struct LIBPROTOBUF_EXPORT ProtobufOnceType {
  ProtobufOnceType();
  ~ProtobufOnceType();
  void Init(void (*init_func)());

  volatile bool initialized_;
  ProtobufOnceInternal* internal_;
};

#define GOOGLE_PROTOBUF_DECLARE_ONCE(NAME)                    \
  ::google::protobuf::ProtobufOnceType NAME

inline void GoogleOnceInit(ProtobufOnceType* once, void (*init_func)()) {
  
  if (!once->initialized_) {
    once->Init(init_func);
  }
}

#else

typedef pthread_once_t ProtobufOnceType;

#define GOOGLE_PROTOBUF_DECLARE_ONCE(NAME)                    \
  pthread_once_t NAME = PTHREAD_ONCE_INIT

inline void GoogleOnceInit(ProtobufOnceType* once, void (*init_func)()) {
  pthread_once(once, init_func);
}

#endif

}  
}  

#endif  
