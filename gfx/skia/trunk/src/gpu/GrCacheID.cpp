






#include "GrTypes.h"
#include "SkThread.h"       



#if 0



static inline void dummy_function_to_avoid_unused_var_warning() {
    GrCacheID::Key kAssertKey;
    GR_STATIC_ASSERT(sizeof(kAssertKey.fData8) == sizeof(kAssertKey.fData32));
    GR_STATIC_ASSERT(sizeof(kAssertKey.fData8) == sizeof(kAssertKey.fData64));
    GR_STATIC_ASSERT(sizeof(kAssertKey.fData8) == sizeof(kAssertKey));
}
#endif

GrCacheID::Domain GrCacheID::GenerateDomain() {
    static int32_t gNextDomain = kInvalid_Domain + 1;

    int32_t domain = sk_atomic_inc(&gNextDomain);
    if (domain >= 1 << (8 * sizeof(Domain))) {
        SkFAIL("Too many Cache Domains");
    }

    return static_cast<Domain>(domain);
}
