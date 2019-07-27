




#if defined(__i386__)
#include "xptcinvoke_gcc_x86_unix.cpp"
#elif defined(__x86_64__)
#include "xptcinvoke_x86_64_unix.cpp"
#elif defined(__ppc__)
#include "xptcinvoke_ppc_rhapsody.cpp"
#else
#error unknown cpu architecture
#endif
