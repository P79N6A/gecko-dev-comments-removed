#include "mozilla/Module.h"



#ifdef _MSC_VER



#  pragma section(".kPStaticModules$Z", read)
#  undef NSMODULE_SECTION
#  define NSMODULE_SECTION __declspec(allocate(".kPStaticModules$Z"), dllexport)
#endif
NSMODULE_DEFN(end_kPStaticModules) = nullptr;
