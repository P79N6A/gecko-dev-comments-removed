































#include "avmplus.h"

using namespace avmplus;

AvmConfiguration AvmCore::config;
static GC _gc;
GC* AvmCore::gc = &_gc;
GCHeap GC::heap;
String* AvmCore::k_str[] = { (String*)"" };
bool AvmCore::sse2_available = false;
bool AvmCore::cmov_available = false;

#ifdef _DEBUG

void NanoAssertFail()
{
    #if defined(WIN32)
        DebugBreak();
        exit(3);
    #elif defined(XP_OS2) || (defined(__GNUC__) && defined(__i386))
        asm("int $3");
    #endif

    abort();
}
#endif
