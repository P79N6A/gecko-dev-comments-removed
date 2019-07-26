






#include <exception>

#if defined(XP_WIN)
#  define MOZALLOC_EXPORT __declspec(dllexport)
#endif

#include "mozilla/mozalloc_abort.h"

namespace std {





__declspec(dllexport) void mozilla_Throw(const exception& e);

void
mozilla_Throw(const exception& e)
{
    mozalloc_abort(e.what());
}

}  
