















#ifndef ANDROID_UTILS_FLATTENABLE_H
#define ANDROID_UTILS_FLATTENABLE_H


#include <stdint.h>
#include <sys/types.h>
#include <utils/Errors.h>

namespace android {






class Flattenable
{
public:
    
    virtual size_t getFlattenedSize() const = 0;

    
    virtual size_t getFdCount() const = 0;

    
    
    
    
    
    virtual status_t flatten(void* buffer, size_t size,
            int fds[], size_t count) const = 0;

    
    
    
    
    
    
    
    virtual status_t unflatten(void const* buffer, size_t size,
            int fds[], size_t count) = 0;

protected:
    virtual ~Flattenable() = 0;

};









template <typename T>
class LightFlattenable {
public:
    
    
    inline bool isFixedSize() const;

    
    inline size_t getSize() const;

    
    inline status_t flatten(void* buffer) const;

    
    inline status_t unflatten(void const* buffer, size_t size);
};

template <typename T>
inline bool LightFlattenable<T>::isFixedSize() const {
    return static_cast<T const*>(this)->T::isFixedSize();
}
template <typename T>
inline size_t LightFlattenable<T>::getSize() const {
    return static_cast<T const*>(this)->T::getSize();
}
template <typename T>
inline status_t LightFlattenable<T>::flatten(void* buffer) const {
    return static_cast<T const*>(this)->T::flatten(buffer);
}
template <typename T>
inline status_t LightFlattenable<T>::unflatten(void const* buffer, size_t size) {
    return static_cast<T*>(this)->T::unflatten(buffer, size);
}





template <typename T>
class LightFlattenablePod : public LightFlattenable<T> {
public:
    inline bool isFixedSize() const {
        return true;
    }

    inline size_t getSize() const {
        return sizeof(T);
    }
    inline status_t flatten(void* buffer) const {
        *reinterpret_cast<T*>(buffer) = *static_cast<T const*>(this);
        return NO_ERROR;
    }
    inline status_t unflatten(void const* buffer, size_t) {
        *static_cast<T*>(this) = *reinterpret_cast<T const*>(buffer);
        return NO_ERROR;
    }
};


}; 


#endif 
