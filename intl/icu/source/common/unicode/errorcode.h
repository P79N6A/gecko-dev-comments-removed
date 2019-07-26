















#ifndef __ERRORCODE_H__
#define __ERRORCODE_H__







#include "unicode/utypes.h"
#include "unicode/uobject.h"

U_NAMESPACE_BEGIN

















































class U_COMMON_API ErrorCode: public UMemory {
public:
    



    ErrorCode() : errorCode(U_ZERO_ERROR) {}
    
    virtual ~ErrorCode();
    
    operator UErrorCode & () { return errorCode; }
    
    operator UErrorCode * () { return &errorCode; }
    
    UBool isSuccess() const { return U_SUCCESS(errorCode); }
    
    UBool isFailure() const { return U_FAILURE(errorCode); }
    
    UErrorCode get() const { return errorCode; }
    
    void set(UErrorCode value) { errorCode=value; }
    
    UErrorCode reset();
    








    void assertSuccess() const;
    





    const char* errorName() const;

protected:
    



    UErrorCode errorCode;
    





    virtual void handleFailure() const {}
};

U_NAMESPACE_END

#endif  
