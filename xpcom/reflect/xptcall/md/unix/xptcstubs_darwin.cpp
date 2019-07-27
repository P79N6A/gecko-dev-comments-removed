




#if defined(__i386__)
#include "xptcstubs_gcc_x86_unix.cpp"
#elif defined(__x86_64__)
#include "xptcstubs_x86_64_darwin.cpp"
#elif defined(__ppc__)
#include "xptcstubs_ppc_rhapsody.cpp"
#else
#error unknown cpu architecture
#endif
