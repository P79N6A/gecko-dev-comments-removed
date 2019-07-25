








#ifndef SkTScopedPtr_DEFINED
#define SkTScopedPtr_DEFINED

#include "SkTypes.h"











template <typename T> class SkTScopedPtr : SkNoncopyable {
public:
    explicit SkTScopedPtr(T* o = NULL) : fObj(o) {}
    ~SkTScopedPtr() {
        enum { kTypeMustBeComplete = sizeof(T) };
        delete fObj;
    }

    


    void reset(T* o = NULL) {
        if (o != fObj) {
            enum { kTypeMustBeComplete = sizeof(T) };
            delete fObj;
            fObj = o;
        }
    }

    


    T* release() {
        T* retVal = fObj;
        fObj = NULL;
        return retVal;
    }

    T& operator*() const {
        SkASSERT(fObj != NULL);
        return *fObj;
    }
    T* operator->() const  {
        SkASSERT(fObj != NULL);
        return fObj;
    }
    T* get() const { return fObj; }

    bool operator==(T* o) const { return fObj == o; }
    bool operator!=(T* o) const { return fObj != o; }

private:
    T* fObj;

    
    
    
    template <class T2> bool operator==(SkTScopedPtr<T2> const& o2) const;
    template <class T2> bool operator!=(SkTScopedPtr<T2> const& o2) const;
};

#endif
