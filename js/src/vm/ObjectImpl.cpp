






#include "ObjectImpl.h"

using namespace js;

static ObjectElements emptyElementsHeader(0, 0);


HeapValue *js::emptyObjectElements =
    reinterpret_cast<HeapValue *>(uintptr_t(&emptyElementsHeader) + sizeof(ObjectElements));
