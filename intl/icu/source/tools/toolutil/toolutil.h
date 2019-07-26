

















#ifndef __TOOLUTIL_H__
#define __TOOLUTIL_H__

#include "unicode/utypes.h"


#ifdef __cplusplus

#include "unicode/errorcode.h"

U_NAMESPACE_BEGIN





class U_TOOLUTIL_API IcuToolErrorCode : public ErrorCode {
public:
    


    IcuToolErrorCode(const char *loc) : location(loc) {}
    virtual ~IcuToolErrorCode();
protected:
    virtual void handleFailure() const;
private:
    const char *location;
};

U_NAMESPACE_END

#endif
















U_CAPI const char * U_EXPORT2
getLongPathname(const char *pathname);








U_CAPI const char * U_EXPORT2
findBasename(const char *filename);















U_CAPI const char * U_EXPORT2
findDirname(const char *path, char *buffer, int32_t bufLen, UErrorCode* status);




U_CAPI int32_t U_EXPORT2
getCurrentYear(void);






U_CAPI void U_EXPORT2
uprv_mkdir(const char *pathname, UErrorCode *status);
























struct UToolMemory;
typedef struct UToolMemory UToolMemory;





U_CAPI UToolMemory * U_EXPORT2
utm_open(const char *name, int32_t initialCapacity, int32_t maxCapacity, int32_t size);




U_CAPI void U_EXPORT2
utm_close(UToolMemory *mem);





U_CAPI void * U_EXPORT2
utm_getStart(UToolMemory *mem);




U_CAPI int32_t U_EXPORT2
utm_countItems(UToolMemory *mem);




U_CAPI void * U_EXPORT2
utm_alloc(UToolMemory *mem);




U_CAPI void * U_EXPORT2
utm_allocN(UToolMemory *mem, int32_t n);

#endif
