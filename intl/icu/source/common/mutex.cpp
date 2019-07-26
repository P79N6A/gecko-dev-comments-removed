












#include "unicode/utypes.h"
#include "mutex.h"
#include "uassert.h"

U_NAMESPACE_BEGIN

void *SimpleSingleton::getInstance(InstantiatorFn *instantiator, const void *context,
                                   void *&duplicate,
                                   UErrorCode &errorCode) {
    duplicate=NULL;
    if(U_FAILURE(errorCode)) {
        return NULL;
    }
    
    
    void *instance=ANNOTATE_UNPROTECTED_READ(fInstance);
    UMTX_ACQUIRE_BARRIER;
    ANNOTATE_HAPPENS_AFTER(&fInstance);
    if(instance!=NULL) {
        return instance;
    }

    
    
    
    instance=instantiator(context, errorCode);
    UMTX_RELEASE_BARRIER;  
    Mutex mutex;
    if(fInstance==NULL && U_SUCCESS(errorCode)) {
        U_ASSERT(instance!=NULL);
        ANNOTATE_HAPPENS_BEFORE(&fInstance);
        
        
        fInstance=instance;
    } else {
        duplicate=instance;
    }
    return fInstance;
}

















void *TriStateSingleton::getInstance(InstantiatorFn *instantiator, const void *context,
                                     void *&duplicate,
                                     UErrorCode &errorCode) {
    duplicate=NULL;
    if(U_FAILURE(errorCode)) {
        return NULL;
    }
    
    
    void *instance=ANNOTATE_UNPROTECTED_READ(fInstance);
    UMTX_ACQUIRE_BARRIER;
    ANNOTATE_HAPPENS_AFTER(&fInstance);
    if(instance!=NULL) {
        
        return instance;
    }

    
    
    
    UErrorCode localErrorCode=ANNOTATE_UNPROTECTED_READ(fErrorCode);
    if(U_FAILURE(localErrorCode)) {
        
        errorCode=localErrorCode;
        return NULL;
    }

    
    
    
    instance=instantiator(context, errorCode);
    UMTX_RELEASE_BARRIER;  
    Mutex mutex;
    if(fInstance==NULL && U_SUCCESS(errorCode)) {
        
        U_ASSERT(instance!=NULL);
        ANNOTATE_HAPPENS_BEFORE(&fInstance);
        
        
        fInstance=instance;
        
        fErrorCode=errorCode;
        
    } else {
        
        
        duplicate=instance;
        if(fInstance==NULL && U_SUCCESS(fErrorCode) && U_FAILURE(errorCode)) {
            
            fErrorCode=errorCode;
            
        }
    }
    return fInstance;
}

void TriStateSingleton::reset() {
    fInstance=NULL;
    fErrorCode=U_ZERO_ERROR;
}

#if UCONFIG_NO_SERVICE



static Mutex *aMutex = 0;


#endif

U_NAMESPACE_END
