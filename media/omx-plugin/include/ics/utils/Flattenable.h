















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

}; 


#endif 
