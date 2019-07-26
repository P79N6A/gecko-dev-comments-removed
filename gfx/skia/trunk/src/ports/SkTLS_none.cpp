






#include "SkTLS.h"

static void* gSpecific = NULL;

void* SkTLS::PlatformGetSpecific(bool) {
    return gSpecific;
}

void SkTLS::PlatformSetSpecific(void* ptr) {
    gSpecific = ptr;
}
