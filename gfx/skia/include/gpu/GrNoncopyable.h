









#ifndef GrNoncopyable_DEFINED
#define GrNoncopyable_DEFINED

#include "GrTypes.h"





class GR_API GrNoncopyable {
public:
    GrNoncopyable() {}

private:
    
    GrNoncopyable(const GrNoncopyable&);
    GrNoncopyable& operator=(const GrNoncopyable&);
};

#endif

