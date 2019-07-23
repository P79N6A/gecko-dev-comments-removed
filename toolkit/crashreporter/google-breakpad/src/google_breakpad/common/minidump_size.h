

































#ifndef GOOGLE_BREAKPAD_COMMON_MINIDUMP_SIZE_H__
#define GOOGLE_BREAKPAD_COMMON_MINIDUMP_SIZE_H__

#include <sys/types.h>

#include "google_breakpad/common/minidump_format.h"

namespace google_breakpad {

template<typename T>
class minidump_size {
 public:
  static size_t size() { return sizeof(T); }
};





template<>
class minidump_size<MDString> {
 public:
  static size_t size() { return MDString_minsize; }
};

template<>
class minidump_size<MDRawThreadList> {
 public:
  static size_t size() { return MDRawThreadList_minsize; }
};

template<>
class minidump_size<MDCVInfoPDB20> {
 public:
  static size_t size() { return MDCVInfoPDB20_minsize; }
};

template<>
class minidump_size<MDCVInfoPDB70> {
 public:
  static size_t size() { return MDCVInfoPDB70_minsize; }
};

template<>
class minidump_size<MDImageDebugMisc> {
 public:
  static size_t size() { return MDImageDebugMisc_minsize; }
};

template<>
class minidump_size<MDRawModuleList> {
 public:
  static size_t size() { return MDRawModuleList_minsize; }
};

template<>
class minidump_size<MDRawMemoryList> {
 public:
  static size_t size() { return MDRawMemoryList_minsize; }
};




template<>
class minidump_size<MDRawModule> {
 public:
  static size_t size() { return MD_MODULE_SIZE; }
};

}  

#endif  
