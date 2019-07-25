















#ifndef ANDROID_ANDROID_NATIVES_H
#define ANDROID_ANDROID_NATIVES_H

#include <sys/types.h>
#include <string.h>

#include <hardware/gralloc.h>
#include <system/window.h>

#include <android/native_window.h>



typedef struct egl_native_pixmap_t
{
    int32_t     version;    
    int32_t     width;
    int32_t     height;
    int32_t     stride;
    uint8_t*    data;
    uint8_t     format;
    uint8_t     rfu[3];
    union {
        uint32_t    compressedFormat;
        int32_t     vstride;
    };
    int32_t     reserved;
} egl_native_pixmap_t;



#ifdef __cplusplus

#include <utils/RefBase.h>

namespace android {





template <typename NATIVE_TYPE, typename TYPE, typename REF>
class EGLNativeBase : public NATIVE_TYPE, public REF
{
public:
    
    void incStrong(const void* id) const {
        REF::incStrong(id);
    }
    void decStrong(const void* id) const {
        REF::decStrong(id);
    }

protected:
    typedef EGLNativeBase<NATIVE_TYPE, TYPE, REF> BASE;
    EGLNativeBase() : NATIVE_TYPE(), REF() {
        NATIVE_TYPE::common.incRef = incRef;
        NATIVE_TYPE::common.decRef = decRef;
    }
    static inline TYPE* getSelf(NATIVE_TYPE* self) {
        return static_cast<TYPE*>(self);
    }
    static inline TYPE const* getSelf(NATIVE_TYPE const* self) {
        return static_cast<TYPE const *>(self);
    }
    static inline TYPE* getSelf(android_native_base_t* base) {
        return getSelf(reinterpret_cast<NATIVE_TYPE*>(base));
    }
    static inline TYPE const * getSelf(android_native_base_t const* base) {
        return getSelf(reinterpret_cast<NATIVE_TYPE const*>(base));
    }
    static void incRef(android_native_base_t* base) {
        EGLNativeBase* self = getSelf(base);
        self->incStrong(self);
    }
    static void decRef(android_native_base_t* base) {
        EGLNativeBase* self = getSelf(base);
        self->decStrong(self);
    }
};

} 
#endif 



#endif 
