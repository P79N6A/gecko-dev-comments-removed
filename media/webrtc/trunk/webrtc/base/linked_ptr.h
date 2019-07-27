


















#ifndef WEBRTC_BASE_LINKED_PTR_H__
#define WEBRTC_BASE_LINKED_PTR_H__

namespace rtc {



#define NO_MEMBER_TEMPLATES

template <class X> class linked_ptr
{
public:

#ifndef NO_MEMBER_TEMPLATES
#   define TEMPLATE_FUNCTION template <class Y>
    TEMPLATE_FUNCTION friend class linked_ptr<Y>;
#else
#   define TEMPLATE_FUNCTION
    typedef X Y;
#endif

    typedef X element_type;

    explicit linked_ptr(X* p = 0) throw()
        : itsPtr(p) {itsPrev = itsNext = this;}
    ~linked_ptr()
        {release();}
    linked_ptr(const linked_ptr& r) throw()
        {acquire(r);}
    linked_ptr& operator=(const linked_ptr& r)
    {
        if (this != &r) {
            release();
            acquire(r);
        }
        return *this;
    }

#ifndef NO_MEMBER_TEMPLATES
    template <class Y> friend class linked_ptr<Y>;
    template <class Y> linked_ptr(const linked_ptr<Y>& r) throw()
        {acquire(r);}
    template <class Y> linked_ptr& operator=(const linked_ptr<Y>& r)
    {
        if (this != &r) {
            release();
            acquire(r);
        }
        return *this;
    }
#endif 

    X& operator*()  const throw()   {return *itsPtr;}
    X* operator->() const throw()   {return itsPtr;}
    X* get()        const throw()   {return itsPtr;}
    bool unique()   const throw()   {return itsPrev ? itsPrev==this : true;}

private:
    X*                          itsPtr;
    mutable const linked_ptr*   itsPrev;
    mutable const linked_ptr*   itsNext;

    void acquire(const linked_ptr& r) throw()
    { 
        itsPtr = r.itsPtr;
        itsNext = r.itsNext;
        itsNext->itsPrev = this;
        itsPrev = &r;
#ifndef mutable
        r.itsNext = this;
#else 
        (const_cast<linked_ptr<X>*>(&r))->itsNext = this;
#endif
    }

#ifndef NO_MEMBER_TEMPLATES
    template <class Y> void acquire(const linked_ptr<Y>& r) throw()
    { 
        itsPtr = r.itsPtr;
        itsNext = r.itsNext;
        itsNext->itsPrev = this;
        itsPrev = &r;
#ifndef mutable
        r.itsNext = this;
#else 
        (const_cast<linked_ptr<X>*>(&r))->itsNext = this;
#endif
    }
#endif 

    void release()
    { 
        if (unique()) delete itsPtr;
        else {
            itsPrev->itsNext = itsNext;
            itsNext->itsPrev = itsPrev;
            itsPrev = itsNext = 0;
        }
        itsPtr = 0;
    }
};

} 

#endif 

