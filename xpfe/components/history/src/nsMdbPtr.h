













































#ifndef nsMdbPtr_h__
#define nsMdbPtr_h__

#include "mdb.h"

template <class T>
class nsMdbDerivedSafe : public T
{
private:
    virtual mdb_err AddStrongRef(nsIMdbEnv* aEnv) { return 0; }        
    virtual mdb_err CutStrongRef(nsIMdbEnv* aEnv) { return 0; }        
    virtual nsMdbDerivedSafe<T>& operator=(const T&) { return *this; } 
    void operator delete(void*, size_t) {}                             
};


template <class T>
class nsMdbPtr
{
private:
    nsIMdbEnv* mEnv;
    T* mRawPtr;

public:
    nsMdbPtr(nsIMdbEnv* aEnv) : mEnv(aEnv), mRawPtr(0)
    {
        NS_PRECONDITION(aEnv != 0, "null ptr");
    }

    nsMdbPtr(nsIMdbEnv* aEnv, T* aRawPtr) : mEnv(aEnv), mRawPtr(0)
    {
        NS_PRECONDITION(aEnv != 0, "null ptr");
        if (mEnv) {
            if (aRawPtr) {
                mRawPtr = aRawPtr;
                mRawPtr->AddStrongRef(mEnv);
            }
        }
    }

    nsMdbPtr(const nsMdbPtr<T>& aSmartPtr) : mEnv(aSmartPtr.mEnv), mRawPtr(0)
    {
        if (mEnv) {
            if (aSmartPtr) {
                mRawPtr = aSmartPtr;
                mRawPtr->AddStrongRef(mEnv);
            }
        }
    }

    nsMdbPtr<T>&
    operator=(const nsMdbPtr<T>& aSmartPtr)
    {
        if (mEnv) {
            if (mRawPtr) {
                mRawPtr->CutStrongRef(mEnv);
                mRawPtr = 0;
            }
        }
        mEnv = aSmartPtr.mEnv;
        if (mEnv) {
            mRawPtr = aSmartPtr.mRawPtr;
            if (mRawPtr)
                mRawPtr->AddStrongRef(mEnv);
        }

        return *this;
    }

    ~nsMdbPtr()
    {
        if (mEnv) {
            if (mRawPtr)
                mRawPtr->CutStrongRef(mEnv);
        }
    }

    nsMdbDerivedSafe<T>*
    get() const
    {
        return NS_REINTERPRET_CAST(nsMdbDerivedSafe<T>*, mRawPtr);
    }

    nsMdbDerivedSafe<T>*
    operator->() const
    {
        return get();
    }


    nsMdbDerivedSafe<T>&
    operator*() const
    {
        return *get();
    }

    operator nsMdbDerivedSafe<T>*() const
    {
        return get();
    }


    T**
    StartAssignment()
    {
        if (mRawPtr) {
            mRawPtr->CutStrongRef(mEnv);
            mRawPtr = 0;
        }
        return &mRawPtr;
    }
};

template <class T, class U>
inline
PRBool
operator==(const nsMdbPtr<T>& lhs, const nsMdbPtr<U>& rhs)
{
    return NS_STATIC_CAST(const void*, lhs.get()) == NS_STATIC_CAST(const void*, rhs.get());
}

template <class T, class U>
inline
PRBool
operator==(const nsMdbPtr<T>& lhs, const U* rhs)
{
    return NS_STATIC_CAST(const void*, lhs.get()) == NS_STATIC_CAST(const void*, rhs);
}


template <class T, class U>
inline
PRBool
operator==(const U* lhs, const nsMdbPtr<T>& rhs)
{
    return NS_STATIC_CAST(const void*, lhs) == NS_STATIC_CAST(const void*, rhs.get());
}




template <class T, class U>
inline
PRBool
operator!=(const nsMdbPtr<T>& lhs, const nsMdbPtr<U>& rhs)
{
    return NS_STATIC_CAST(const void*, lhs.get()) != NS_STATIC_CAST(const void*, rhs.get());
}

template <class T, class U>
inline
PRBool
operator!=(const nsMdbPtr<T>& lhs, const U* rhs)
{
    return NS_STATIC_CAST(const void*, lhs.get()) != NS_STATIC_CAST(const void*, rhs);
}


template <class T, class U>
inline
PRBool
operator!=(const U* lhs, const nsMdbPtr<T>& rhs)
{
    return NS_STATIC_CAST(const void*, lhs) != NS_STATIC_CAST(const void*, rhs.get());
}



template <class T>
class nsGetterAcquires
{
private:
    nsMdbPtr<T>& mTargetSmartPtr;

public:
    explicit
    nsGetterAcquires(nsMdbPtr<T>& aSmartPtr) : mTargetSmartPtr(aSmartPtr)
    {
        
    }

    operator T**()
    {
        return mTargetSmartPtr.StartAssignment();
    }        
};


template <class T>
inline
nsGetterAcquires<T>
getter_Acquires(nsMdbPtr<T>& aSmartPtr)
{
    return nsGetterAcquires<T>(aSmartPtr);
}


#endif 

