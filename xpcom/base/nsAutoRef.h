






































#ifndef nsAutoRef_h_
#define nsAutoRef_h_

#include "nscore.h" 

template <class T> class nsSimpleRef;
template <class T> class nsAutoRefBase;
template <class T> class nsReturnRef;
template <class T> class nsReturningRef;

























































































template <class T>
class nsAutoRef : public nsAutoRefBase<T>
{
protected:
    typedef nsAutoRef<T> ThisClass;
    typedef nsAutoRefBase<T> BaseClass;
    typedef nsSimpleRef<T> SimpleRef;
    typedef typename BaseClass::RawRefOnly RawRefOnly;
    typedef typename BaseClass::LocalSimpleRef LocalSimpleRef;

public:
    nsAutoRef()
    {
    }

    
    
    explicit nsAutoRef(RawRefOnly aRefToRelease)
        : BaseClass(aRefToRelease)
    {
    }

    
    
    
    explicit nsAutoRef(const nsReturningRef<T>& aReturning)
        : BaseClass(aReturning)
    {
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    ThisClass& operator=(const nsReturningRef<T>& aReturning)
    {
        BaseClass::steal(aReturning.mReturnRef);
        return *this;
    }

    
    
    operator typename SimpleRef::RawRef() const
    {
        return this->get();
    }

    
    void steal(ThisClass& aOtherRef)
    {
        BaseClass::steal(aOtherRef);
    }

    
    
    
    
    
    
    
    
    void own(RawRefOnly aRefToRelease)
    {
        BaseClass::own(aRefToRelease);
    }

    
    void swap(ThisClass& aOther)
    {
        LocalSimpleRef temp;
        temp.SimpleRef::operator=(this);
        SimpleRef::operator=(aOther);
        aOther.SimpleRef::operator=(temp);
    }

    
    void reset()
    {
        this->SafeRelease();
        LocalSimpleRef empty;
        SimpleRef::operator=(empty);
    }

    
    nsReturnRef<T> out()
    {
        return nsReturnRef<T>(this->disown());
    }

    
    

private:
    
    explicit nsAutoRef(ThisClass& aRefToSteal);
};
















template <class T>
class nsCountedRef : public nsAutoRef<T>
{
protected:
    typedef nsCountedRef<T> ThisClass;
    typedef nsAutoRef<T> BaseClass;
    typedef nsSimpleRef<T> SimpleRef;
    typedef typename BaseClass::RawRef RawRef;

public:
    nsCountedRef()
    {
    }

    
    
    nsCountedRef(const ThisClass& aRefToCopy)
    {
        SimpleRef::operator=(aRefToCopy);
        SafeAddRef();
    }
    ThisClass& operator=(const ThisClass& aRefToCopy)
    {
        if (this == &aRefToCopy)
            return *this;

        this->SafeRelease();
        SimpleRef::operator=(aRefToCopy);
        SafeAddRef();
        return *this;
    }

    
    
    
    explicit nsCountedRef(RawRef aRefToCopy)
        : BaseClass(aRefToCopy)
    {
        SafeAddRef();
    }
    ThisClass& operator=(RawRef aRefToCopy)
    {
        this->own(aRefToCopy);
        SafeAddRef();
        return *this;
    }

    
    
    explicit nsCountedRef(const nsReturningRef<T>& aReturning)
        : BaseClass(aReturning)
    {
    }
    ThisClass& operator=(const nsReturningRef<T>& aReturning)
    {
        BaseClass::operator=(aReturning);
        return *this;
    }

protected:
    
    void SafeAddRef()
    {
        if (this->HaveResource())
            this->AddRef(this->get());
    }
};








template <class T>
class nsReturnRef : public nsAutoRefBase<T>
{
protected:
    typedef nsAutoRefBase<T> BaseClass;
    typedef typename BaseClass::RawRefOnly RawRefOnly;

public:
    
    nsReturnRef()
    {
    }

    
    
    
    explicit nsReturnRef(RawRefOnly aRefToRelease)
        : BaseClass(aRefToRelease)
    {
    }

    
    nsReturnRef(nsReturnRef<T>& aRefToSteal)
        : BaseClass(aRefToSteal)
    {
    }

    nsReturnRef(const nsReturningRef<T>& aReturning)
        : BaseClass(aReturning)
    {
    }

    
    
    
    
    
    operator nsReturningRef<T>()
    {
        return nsReturningRef<T>(*this);
    }

    
    
    
    
};
















template <class T>
class nsReturningRef
{
private:
    friend class nsReturnRef<T>;

    explicit nsReturningRef(nsReturnRef<T>& aReturnRef)
        : mReturnRef(aReturnRef)
    {
    }
public:
    nsReturnRef<T>& mReturnRef;
};





















































template <class T> class nsAutoRefTraits;



























template <class T>
class nsPointerRefTraits
{
public:
    
    typedef T* RawRef;
    
    static RawRef Void() { return nsnull; };
};

















template <class T>
class nsSimpleRef : protected nsAutoRefTraits<T>
{
protected:
    
    
    typedef nsAutoRefTraits<T> Traits;
    
    
    typedef typename Traits::RawRef RawRef;

    
    
    
    
    
    nsSimpleRef()
        : mRawRef(Traits::Void())
    {
    }
    
    
    nsSimpleRef(RawRef aRawRef)
        : mRawRef(aRawRef)
    {
    }

    
    
    
    
    bool HaveResource() const
    {
        return mRawRef != Traits::Void();
    }

public:
    
    
    
    RawRef get() const
    {
        return mRawRef;
    }

private:
    RawRef mRawRef;
};









template <class T>
class nsAutoRefBase : public nsSimpleRef<T>
{
protected:
    typedef nsAutoRefBase<T> ThisClass;
    typedef nsSimpleRef<T> SimpleRef;
    typedef typename SimpleRef::RawRef RawRef;

    nsAutoRefBase()
    {
    }

    
    
    
    
    class RawRefOnly
    {
    public:
        RawRefOnly(RawRef aRawRef)
            : mRawRef(aRawRef)
        {
        }
        operator RawRef() const
        {
            return mRawRef;
        }
    private:
        RawRef mRawRef;
    };

    
    explicit nsAutoRefBase(RawRefOnly aRefToRelease)
        : SimpleRef(aRefToRelease)
    {
    }

    
    explicit nsAutoRefBase(ThisClass& aRefToSteal)
        : SimpleRef(aRefToSteal.disown())
    {
    }
    explicit nsAutoRefBase(const nsReturningRef<T>& aReturning)
        : SimpleRef(aReturning.mReturnRef.disown())
    {
    }

    ~nsAutoRefBase()
    {
        SafeRelease();
    }

    
    
    
    class LocalSimpleRef : public SimpleRef
    {
    public:
        LocalSimpleRef()
        {
        }
        explicit LocalSimpleRef(RawRef aRawRef)
            : SimpleRef(aRawRef)
        {
        }
    };

private:
    ThisClass& operator=(const ThisClass& aSmartRef);
    
public:
    RawRef operator->() const
    {
        return this->get();
    }

    
    
    
    
    
    
    
    
    
    
    RawRef disown()
    {
        RawRef temp = this->get();
        LocalSimpleRef empty;
        SimpleRef::operator=(empty);
        return temp;
    }

protected:
    
    
    

    
    void steal(ThisClass& aOtherRef)
    {
        own(aOtherRef.disown());
    }
    
    void own(RawRefOnly aRefToRelease)
    {
        SafeRelease();
        LocalSimpleRef ref(aRefToRelease);
        SimpleRef::operator=(ref);
    }

    
    void SafeRelease()
    {
        if (this->HaveResource())
            this->Release(this->get());
    }
};

#endif 
