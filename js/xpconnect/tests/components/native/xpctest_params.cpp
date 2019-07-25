



































#include "xpctest_private.h"
#include "xpctest_interfaces.h"

NS_IMPL_ISUPPORTS1(nsXPCTestParams, nsIXPCTestParams)

nsXPCTestParams::nsXPCTestParams()
{
}

nsXPCTestParams::~nsXPCTestParams()
{
}

#define GENERIC_METHOD_IMPL {                                                 \
    *_retval = *b;                                                            \
    *b = a;                                                                   \
    return NS_OK;                                                             \
}

#define STRING_METHOD_IMPL {                                                  \
    _retval.Assign(b);                                                        \
    b.Assign(a);                                                              \
    return NS_OK;                                                             \
}

#define TAKE_OWNERSHIP_NOOP(val) {}
#define TAKE_OWNERSHIP_INTERFACE(val) {static_cast<nsISupports*>(val)->AddRef();}
#define TAKE_OWNERSHIP_STRING(val) {                                          \
    nsDependentCString vprime(val);                                           \
    val = ToNewCString(vprime);                                               \
}
#define TAKE_OWNERSHIP_WSTRING(val) {                                         \
    nsDependentString vprime(val);                                            \
    val = ToNewUnicode(vprime);                                               \
}






#define BUFFER_METHOD_IMPL(type, padding, TAKE_OWNERSHIP) {                   \
    PRUint32 elemSize = sizeof(type);                                         \
                                                                              \
    /* Copy b into rv. */                                                     \
    *rvLength = *bLength;                                                     \
    *rv = static_cast<type*>(NS_Alloc(elemSize * (*bLength + padding)));      \
    if (!*rv)                                                                 \
        return NS_ERROR_OUT_OF_MEMORY;                                        \
    memcpy(*rv, *b, elemSize * (*bLength + padding));                         \
                                                                              \
    /* Copy a into b. */                                                      \
    *bLength = aLength;                                                       \
    NS_Free(*b);                                                              \
    *b = static_cast<type*>(NS_Alloc(elemSize * (aLength + padding)));        \
    if (!*b)                                                                  \
        return NS_ERROR_OUT_OF_MEMORY;                                        \
    memcpy(*b, a, elemSize * (aLength + padding));                            \
                                                                              \
    /* We need to take ownership of the data we got from a,                   \
       since the caller owns it. */                                           \
    for (unsigned i = 0; i < *bLength + padding; ++i)                         \
        TAKE_OWNERSHIP((*b)[i]);                                              \
                                                                              \
    return NS_OK;                                                             \
}


NS_IMETHODIMP nsXPCTestParams::TestBoolean(bool a, bool *b NS_INOUTPARAM, bool *_retval NS_OUTPARAM)
{
    GENERIC_METHOD_IMPL;
}


NS_IMETHODIMP nsXPCTestParams::TestOctet(PRUint8 a, PRUint8 *b NS_INOUTPARAM, PRUint8 *_retval NS_OUTPARAM)
{
    GENERIC_METHOD_IMPL;
}


NS_IMETHODIMP nsXPCTestParams::TestShort(PRInt16 a, PRInt16 *b NS_INOUTPARAM, PRInt16 *_retval NS_OUTPARAM)
{
    GENERIC_METHOD_IMPL;
}


NS_IMETHODIMP nsXPCTestParams::TestLong(PRInt32 a, PRInt32 *b NS_INOUTPARAM, PRInt32 *_retval NS_OUTPARAM)
{
    GENERIC_METHOD_IMPL;
}


NS_IMETHODIMP nsXPCTestParams::TestLongLong(PRInt64 a, PRInt64 *b NS_INOUTPARAM, PRInt64 *_retval NS_OUTPARAM)
{
    GENERIC_METHOD_IMPL;
}


NS_IMETHODIMP nsXPCTestParams::TestUnsignedShort(PRUint16 a, PRUint16 *b NS_INOUTPARAM, PRUint16 *_retval NS_OUTPARAM)
{
    GENERIC_METHOD_IMPL;
}


NS_IMETHODIMP nsXPCTestParams::TestUnsignedLong(PRUint32 a, PRUint32 *b NS_INOUTPARAM, PRUint32 *_retval NS_OUTPARAM)
{
    GENERIC_METHOD_IMPL;
}


NS_IMETHODIMP nsXPCTestParams::TestUnsignedLongLong(PRUint64 a, PRUint64 *b NS_INOUTPARAM, PRUint64 *_retval NS_OUTPARAM)
{
    GENERIC_METHOD_IMPL;
}


NS_IMETHODIMP nsXPCTestParams::TestFloat(float a, float *b NS_INOUTPARAM, float *_retval NS_OUTPARAM)
{
    GENERIC_METHOD_IMPL;
}


NS_IMETHODIMP nsXPCTestParams::TestDouble(double a, float *b NS_INOUTPARAM, double *_retval NS_OUTPARAM)
{
    GENERIC_METHOD_IMPL;
}


NS_IMETHODIMP nsXPCTestParams::TestChar(char a, char *b NS_INOUTPARAM, char *_retval NS_OUTPARAM)
{
    GENERIC_METHOD_IMPL;
}


NS_IMETHODIMP nsXPCTestParams::TestString(const char * a, char * *b NS_INOUTPARAM, char * *_retval NS_OUTPARAM)
{
    nsDependentCString aprime(a);
    nsDependentCString bprime(*b);
    *_retval = ToNewCString(bprime);
    *b = ToNewCString(aprime);

    
    
    NS_Free(const_cast<char*>(bprime.get()));

    return NS_OK;
}


NS_IMETHODIMP nsXPCTestParams::TestWchar(PRUnichar a, PRUnichar *b NS_INOUTPARAM, PRUnichar *_retval NS_OUTPARAM)
{
    GENERIC_METHOD_IMPL;
}


NS_IMETHODIMP nsXPCTestParams::TestWstring(const PRUnichar * a, PRUnichar * *b NS_INOUTPARAM, PRUnichar * *_retval NS_OUTPARAM)
{
    nsDependentString aprime(a);
    nsDependentString bprime(*b);
    *_retval = ToNewUnicode(bprime);
    *b = ToNewUnicode(aprime);

    
    
    NS_Free(const_cast<PRUnichar*>(bprime.get()));

    return NS_OK;
}


NS_IMETHODIMP nsXPCTestParams::TestDOMString(const nsAString & a, nsAString & b NS_INOUTPARAM, nsAString & _retval NS_OUTPARAM)
{
    STRING_METHOD_IMPL;
}



NS_IMETHODIMP nsXPCTestParams::TestAString(const nsAString & a, nsAString & b NS_INOUTPARAM, nsAString & _retval NS_OUTPARAM)
{
    STRING_METHOD_IMPL;
}


NS_IMETHODIMP nsXPCTestParams::TestAUTF8String(const nsACString & a, nsACString & b NS_INOUTPARAM, nsACString & _retval NS_OUTPARAM)
{
    STRING_METHOD_IMPL;
}


NS_IMETHODIMP nsXPCTestParams::TestACString(const nsACString & a, nsACString & b NS_INOUTPARAM, nsACString & _retval NS_OUTPARAM)
{
    STRING_METHOD_IMPL;
}


NS_IMETHODIMP nsXPCTestParams::TestJsval(const jsval & a, jsval & b NS_INOUTPARAM, jsval *_retval NS_OUTPARAM)
{
    *_retval = b;
    b = a;
    return NS_OK;
}




NS_IMETHODIMP nsXPCTestParams::TestShortArray(PRUint32 aLength, PRInt16 *a,
                                              PRUint32 *bLength NS_INOUTPARAM, PRInt16 **b NS_INOUTPARAM,
                                              PRUint32 *rvLength NS_OUTPARAM, PRInt16 **rv NS_OUTPARAM)
{
    BUFFER_METHOD_IMPL(PRInt16, 0, TAKE_OWNERSHIP_NOOP);
}




NS_IMETHODIMP nsXPCTestParams::TestLongLongArray(PRUint32 aLength, PRInt64 *a,
                                                 PRUint32 *bLength NS_INOUTPARAM, PRInt64 **b NS_INOUTPARAM,
                                                 PRUint32 *rvLength NS_OUTPARAM, PRInt64 **rv NS_OUTPARAM)
{
    BUFFER_METHOD_IMPL(PRInt64, 0, TAKE_OWNERSHIP_NOOP);
}




NS_IMETHODIMP nsXPCTestParams::TestStringArray(PRUint32 aLength, const char * *a,
                                               PRUint32 *bLength NS_INOUTPARAM, char * **b NS_INOUTPARAM,
                                               PRUint32 *rvLength NS_OUTPARAM, char * **rv NS_OUTPARAM)
{
    BUFFER_METHOD_IMPL(char*, 0, TAKE_OWNERSHIP_STRING);
}




NS_IMETHODIMP nsXPCTestParams::TestWstringArray(PRUint32 aLength, const PRUnichar * *a,
                                                PRUint32 *bLength NS_INOUTPARAM, PRUnichar * **b NS_INOUTPARAM,
                                                PRUint32 *rvLength NS_OUTPARAM, PRUnichar * **rv NS_OUTPARAM)
{
    BUFFER_METHOD_IMPL(PRUnichar*, 0, TAKE_OWNERSHIP_WSTRING);
}




NS_IMETHODIMP nsXPCTestParams::TestInterfaceArray(PRUint32 aLength, nsIXPCTestInterfaceA **a,
                                                  PRUint32 *bLength NS_INOUTPARAM, nsIXPCTestInterfaceA * **b NS_INOUTPARAM,
                                                  PRUint32 *rvLength NS_OUTPARAM, nsIXPCTestInterfaceA * **rv NS_OUTPARAM)
{
    BUFFER_METHOD_IMPL(nsIXPCTestInterfaceA*, 0, TAKE_OWNERSHIP_INTERFACE);
}




NS_IMETHODIMP nsXPCTestParams::TestSizedString(PRUint32 aLength, const char * a,
                                               PRUint32 *bLength NS_INOUTPARAM, char * *b NS_INOUTPARAM,
                                               PRUint32 *rvLength NS_OUTPARAM, char * *rv NS_OUTPARAM)
{
    BUFFER_METHOD_IMPL(char, 1, TAKE_OWNERSHIP_NOOP);
}




NS_IMETHODIMP nsXPCTestParams::TestSizedWstring(PRUint32 aLength, const PRUnichar * a,
                                                PRUint32 *bLength NS_INOUTPARAM, PRUnichar * *b NS_INOUTPARAM,
                                                PRUint32 *rvLength NS_OUTPARAM, PRUnichar * *rv NS_OUTPARAM)
{
    BUFFER_METHOD_IMPL(PRUnichar, 1, TAKE_OWNERSHIP_NOOP);
}




NS_IMETHODIMP nsXPCTestParams::TestInterfaceIs(const nsIID *aIID, void *a,
                                               nsIID **bIID NS_INOUTPARAM, void **b NS_INOUTPARAM,
                                               nsIID **rvIID NS_OUTPARAM, void **rv NS_OUTPARAM)
{
    
    
    

    
    
    
    
    
    *rv = *b;

    
    
    *rvIID = static_cast<nsIID*>(NS_Alloc(sizeof(nsID)));
    if (!*rvIID)
        return NS_ERROR_OUT_OF_MEMORY;
    **rvIID = **bIID;

    
    
    *b = a;
    static_cast<nsISupports*>(*b)->AddRef();

    
    **bIID = *aIID;

    return NS_OK;
}







NS_IMETHODIMP nsXPCTestParams::TestInterfaceIsArray(PRUint32 aLength, const nsIID *aIID,
                                                    void **a,
                                                    PRUint32 *bLength NS_INOUTPARAM, nsIID **bIID NS_INOUTPARAM,
                                                    void ***b NS_INOUTPARAM,
                                                    PRUint32 *rvLength NS_OUTPARAM, nsIID **rvIID NS_OUTPARAM,
                                                    void ***rv NS_OUTPARAM)
{
    
    
    *rvIID = static_cast<nsIID*>(NS_Alloc(sizeof(nsID)));
    if (!*rvIID)
        return NS_ERROR_OUT_OF_MEMORY;
    **rvIID = **bIID;
    **bIID = *aIID;

    
    
    
    BUFFER_METHOD_IMPL(void*, 0, TAKE_OWNERSHIP_INTERFACE);
}
