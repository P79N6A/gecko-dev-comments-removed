




#include "mozilla/Attributes.h"














extern "C" MOZ_ASAN_BLACKLIST
const char* __asan_default_options() {
    return "allow_user_segv_handler=1:alloc_dealloc_mismatch=0";
}
