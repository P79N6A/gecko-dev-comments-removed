




































#ifdef MOZ_DEMANGLE_SYMBOLS

#include <cxxabi.h>
#include <stdlib.h>

extern "C" char *nsDemangle(const char *symbol);

char *nsDemangle(const char *symbol) {
    return  abi::__cxa_demangle(symbol, 0, 0, 0);
}

#endif
