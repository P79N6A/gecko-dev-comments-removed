







#ifndef __SHAREDOBJECT_H__
#define __SHAREDOBJECT_H__


#include "unicode/uobject.h"
#include "umutex.h"

U_NAMESPACE_BEGIN










class U_COMMON_API SharedObject : public UObject {
public:
    
    SharedObject() : totalRefCount(0), softRefCount(0) {}

    
    SharedObject(const SharedObject &other)
        : UObject(other),
          totalRefCount(0),
          softRefCount(0) {}

    virtual ~SharedObject();

    


    void addRef() const;

    


    void addSoftRef() const;

    


    void removeRef() const;

    


    void removeSoftRef() const;

    



    int32_t getRefCount() const;

    



    int32_t getSoftRefCount() const;

    



    UBool allSoftReferences() const;

    


    void deleteIfZeroRefCount() const;

    










    template<typename T>
    static T *copyOnWrite(const T *&ptr) {
        const T *p = ptr;
        if(p->getRefCount() <= 1) { return const_cast<T *>(p); }
        T *p2 = new T(*p);
        if(p2 == NULL) { return NULL; }
        p->removeRef();
        ptr = p2;
        p2->addRef();
        return p2;
    }

    







    template<typename T>
    static void copyPtr(const T *src, const T *&dest) {
        if(src != dest) {
            if(dest != NULL) { dest->removeRef(); }
            dest = src;
            if(src != NULL) { src->addRef(); }
        }
    }

    


    template<typename T>
    static void clearPtr(const T *&ptr) {
        if (ptr != NULL) {
            ptr->removeRef();
            ptr = NULL;
        }
    }

private:
    mutable u_atomic_int32_t totalRefCount;
    mutable u_atomic_int32_t softRefCount;
};

U_NAMESPACE_END

#endif
