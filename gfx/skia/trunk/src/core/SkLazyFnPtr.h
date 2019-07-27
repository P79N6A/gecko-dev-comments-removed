






#ifndef SkLazyFnPtr_DEFINED
#define SkLazyFnPtr_DEFINED



















#define SK_DECLARE_STATIC_LAZY_FN_PTR(F, name, Choose) static Private::SkLazyFnPtr<F, Choose> name




#include "SkDynamicAnnotations.h"
#include "SkThreadPriv.h"

namespace Private {


template <typename F, F (*Choose)()>
class SkLazyFnPtr {
public:
    F get() {
        
        F fn = (F)SK_ANNOTATE_UNPROTECTED_READ(fPtr);
        if (fn != NULL) {
            return fn;
        }

        
        fn = Choose();

        
        F prev = (F)sk_atomic_cas(&fPtr, NULL, (void*)fn);

        
        
        return prev != NULL ? prev : fn;
    }

private:
    void* fPtr;
};

}  

#endif
