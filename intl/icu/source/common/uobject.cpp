















#include "unicode/uobject.h"
#include "cmemory.h"

U_NAMESPACE_BEGIN

#if U_OVERRIDE_CXX_ALLOCATION




































void * U_EXPORT2 UMemory::operator new(size_t size) U_NO_THROW {
    return uprv_malloc(size);
}

void U_EXPORT2 UMemory::operator delete(void *p) U_NO_THROW {
    if(p!=NULL) {
        uprv_free(p);
    }
}

void * U_EXPORT2 UMemory::operator new[](size_t size) U_NO_THROW {
    return uprv_malloc(size);
}

void U_EXPORT2 UMemory::operator delete[](void *p) U_NO_THROW {
    if(p!=NULL) {
        uprv_free(p);
    }
}

#if U_HAVE_DEBUG_LOCATION_NEW
void * U_EXPORT2 UMemory::operator new(size_t size, const char* , int ) U_NO_THROW {
    return UMemory::operator new(size);
}

void U_EXPORT2 UMemory::operator delete(void* p, const char* , int ) U_NO_THROW {
    UMemory::operator delete(p);
}
#endif 


#endif

UObject::~UObject() {}



















U_NAMESPACE_END

U_NAMESPACE_USE

U_CAPI void U_EXPORT2
uprv_deleteUObject(void *obj) {
    delete static_cast<UObject *>(obj);
}
