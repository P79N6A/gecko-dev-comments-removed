































#include "avmplus.h"

using namespace avmplus;

AvmConfiguration AvmCore::config;
static GC _gc;
GC* AvmCore::gc = &_gc;
GCHeap GC::heap;

