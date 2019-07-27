



#ifndef APKOpen_h
#define APKOpen_h

#include <jni.h>

#ifndef NS_EXPORT
#define NS_EXPORT __attribute__ ((visibility("default")))
#endif

struct mapping_info {
  char * name;
  uintptr_t base;
  size_t len;
  size_t offset;
};

NS_EXPORT const struct mapping_info * getLibraryMapping();
NS_EXPORT void abortThroughJava(const char* msg);

static const int SUCCESS = 0;
static const int FAILURE = 1;
void JNI_Throw(JNIEnv* jenv, const char* classname, const char* msg);

#endif 
