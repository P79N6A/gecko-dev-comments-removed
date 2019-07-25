





































#ifndef nsSupportsPrimitives_h__
#define nsSupportsPrimitives_h__

#include "nsISupportsPrimitives.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsDependentString.h"

class nsSupportsIDImpl : public nsISupportsID
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISUPPORTSPRIMITIVE
    NS_DECL_NSISUPPORTSID

    nsSupportsIDImpl();

private:
    ~nsSupportsIDImpl() { }

    nsID *mData;
};



class nsSupportsCStringImpl : public nsISupportsCString
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISUPPORTSPRIMITIVE
    NS_DECL_NSISUPPORTSCSTRING

    nsSupportsCStringImpl() {}

private:
    ~nsSupportsCStringImpl() {}
    
    nsCString mData;
};



class nsSupportsStringImpl : public nsISupportsString
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISUPPORTSPRIMITIVE
    NS_DECL_NSISUPPORTSSTRING

    nsSupportsStringImpl() {}

private:
    ~nsSupportsStringImpl() {}
    
    nsString mData;
};



class nsSupportsPRBoolImpl : public nsISupportsPRBool
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISUPPORTSPRIMITIVE
    NS_DECL_NSISUPPORTSPRBOOL

    nsSupportsPRBoolImpl();

private:
    ~nsSupportsPRBoolImpl() {}

    PRBool mData;
};



class nsSupportsPRUint8Impl : public nsISupportsPRUint8
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISUPPORTSPRIMITIVE
    NS_DECL_NSISUPPORTSPRUINT8

    nsSupportsPRUint8Impl();

private:
    ~nsSupportsPRUint8Impl() {}

    PRUint8 mData;
};



class nsSupportsPRUint16Impl : public nsISupportsPRUint16
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISUPPORTSPRIMITIVE
    NS_DECL_NSISUPPORTSPRUINT16

    nsSupportsPRUint16Impl();

private:
    ~nsSupportsPRUint16Impl() {}

    PRUint16 mData;
};



class nsSupportsPRUint32Impl : public nsISupportsPRUint32
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISUPPORTSPRIMITIVE
    NS_DECL_NSISUPPORTSPRUINT32

    nsSupportsPRUint32Impl();

private:
    ~nsSupportsPRUint32Impl() {}

    PRUint32 mData;
};



class nsSupportsPRUint64Impl : public nsISupportsPRUint64
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISUPPORTSPRIMITIVE
    NS_DECL_NSISUPPORTSPRUINT64

    nsSupportsPRUint64Impl();

private:
    ~nsSupportsPRUint64Impl() {}

    PRUint64 mData;
};



class nsSupportsPRTimeImpl : public nsISupportsPRTime
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISUPPORTSPRIMITIVE
    NS_DECL_NSISUPPORTSPRTIME

    nsSupportsPRTimeImpl();

private:
    ~nsSupportsPRTimeImpl() {}

    PRTime mData;
};



class nsSupportsCharImpl : public nsISupportsChar
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISUPPORTSPRIMITIVE
    NS_DECL_NSISUPPORTSCHAR

    nsSupportsCharImpl();

private:
    ~nsSupportsCharImpl() {}

    char mData;
};



class nsSupportsPRInt16Impl : public nsISupportsPRInt16
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISUPPORTSPRIMITIVE
    NS_DECL_NSISUPPORTSPRINT16

    nsSupportsPRInt16Impl();

private:
    ~nsSupportsPRInt16Impl() {}

    PRInt16 mData;
};



class nsSupportsPRInt32Impl : public nsISupportsPRInt32
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISUPPORTSPRIMITIVE
    NS_DECL_NSISUPPORTSPRINT32

    nsSupportsPRInt32Impl();

private:
    ~nsSupportsPRInt32Impl() {}

    PRInt32 mData;
};



class nsSupportsPRInt64Impl : public nsISupportsPRInt64
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISUPPORTSPRIMITIVE
    NS_DECL_NSISUPPORTSPRINT64

    nsSupportsPRInt64Impl();

private:
    ~nsSupportsPRInt64Impl() {}

    PRInt64 mData;
};



class nsSupportsFloatImpl : public nsISupportsFloat
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISUPPORTSPRIMITIVE
    NS_DECL_NSISUPPORTSFLOAT

    nsSupportsFloatImpl();

private:
    ~nsSupportsFloatImpl() {}

    float mData;
};



class nsSupportsDoubleImpl : public nsISupportsDouble
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISUPPORTSPRIMITIVE
    NS_DECL_NSISUPPORTSDOUBLE

    nsSupportsDoubleImpl();

private:
    ~nsSupportsDoubleImpl() {}

    double mData;
};



class nsSupportsVoidImpl : public nsISupportsVoid
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISUPPORTSPRIMITIVE
    NS_DECL_NSISUPPORTSVOID

    nsSupportsVoidImpl();

private:
    ~nsSupportsVoidImpl() {}

    void* mData;
};



class nsSupportsInterfacePointerImpl : public nsISupportsInterfacePointer
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISUPPORTSPRIMITIVE
    NS_DECL_NSISUPPORTSINTERFACEPOINTER

    nsSupportsInterfacePointerImpl();

private:
    ~nsSupportsInterfacePointerImpl();

    nsCOMPtr<nsISupports> mData;
    nsID *mIID;
};









class nsSupportsDependentCString : public nsISupportsCString
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISUPPORTSPRIMITIVE
  NS_DECL_NSISUPPORTSCSTRING

  nsSupportsDependentCString(const char* aStr);

private:
  ~nsSupportsDependentCString() {}

  nsDependentCString mData;
};

#endif 
