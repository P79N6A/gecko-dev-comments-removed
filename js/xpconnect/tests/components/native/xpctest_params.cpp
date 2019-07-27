



#include "xpctest_private.h"
#include "xpctest_interfaces.h"
#include "js/Value.h"

NS_IMPL_ISUPPORTS(nsXPCTestParams, nsIXPCTestParams)

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
    uint32_t elemSize = sizeof(type);                                         \
                                                                              \
    /* Copy b into rv. */                                                     \
    *rvLength = *bLength;                                                     \
    *rv = static_cast<type*>(moz_xmalloc(elemSize * (*bLength + padding)));   \
    if (!*rv)                                                                 \
        return NS_ERROR_OUT_OF_MEMORY;                                        \
    memcpy(*rv, *b, elemSize * (*bLength + padding));                         \
                                                                              \
    /* Copy a into b. */                                                      \
    *bLength = aLength;                                                       \
    free(*b);                                                                 \
    *b = static_cast<type*>(moz_xmalloc(elemSize * (aLength + padding)));     \
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


NS_IMETHODIMP nsXPCTestParams::TestBoolean(bool a, bool* b, bool* _retval)
{
    GENERIC_METHOD_IMPL;
}


NS_IMETHODIMP nsXPCTestParams::TestOctet(uint8_t a, uint8_t* b, uint8_t* _retval)
{
    GENERIC_METHOD_IMPL;
}


NS_IMETHODIMP nsXPCTestParams::TestShort(int16_t a, int16_t* b, int16_t* _retval)
{
    GENERIC_METHOD_IMPL;
}


NS_IMETHODIMP nsXPCTestParams::TestLong(int32_t a, int32_t* b, int32_t* _retval)
{
    GENERIC_METHOD_IMPL;
}


NS_IMETHODIMP nsXPCTestParams::TestLongLong(int64_t a, int64_t* b, int64_t* _retval)
{
    GENERIC_METHOD_IMPL;
}


NS_IMETHODIMP nsXPCTestParams::TestUnsignedShort(uint16_t a, uint16_t* b, uint16_t* _retval)
{
    GENERIC_METHOD_IMPL;
}


NS_IMETHODIMP nsXPCTestParams::TestUnsignedLong(uint32_t a, uint32_t* b, uint32_t* _retval)
{
    GENERIC_METHOD_IMPL;
}


NS_IMETHODIMP nsXPCTestParams::TestUnsignedLongLong(uint64_t a, uint64_t* b, uint64_t* _retval)
{
    GENERIC_METHOD_IMPL;
}


NS_IMETHODIMP nsXPCTestParams::TestFloat(float a, float* b, float* _retval)
{
    GENERIC_METHOD_IMPL;
}


NS_IMETHODIMP nsXPCTestParams::TestDouble(double a, float* b, double* _retval)
{
    GENERIC_METHOD_IMPL;
}


NS_IMETHODIMP nsXPCTestParams::TestChar(char a, char* b, char* _retval)
{
    GENERIC_METHOD_IMPL;
}


NS_IMETHODIMP nsXPCTestParams::TestString(const char * a, char * *b, char * *_retval)
{
    nsDependentCString aprime(a);
    nsDependentCString bprime(*b);
    *_retval = ToNewCString(bprime);
    *b = ToNewCString(aprime);

    
    
    free(const_cast<char*>(bprime.get()));

    return NS_OK;
}


NS_IMETHODIMP nsXPCTestParams::TestWchar(char16_t a, char16_t* b, char16_t* _retval)
{
    GENERIC_METHOD_IMPL;
}


NS_IMETHODIMP nsXPCTestParams::TestWstring(const char16_t * a, char16_t * *b, char16_t * *_retval)
{
    nsDependentString aprime(a);
    nsDependentString bprime(*b);
    *_retval = ToNewUnicode(bprime);
    *b = ToNewUnicode(aprime);

    
    
    free((void*)bprime.get());

    return NS_OK;
}


NS_IMETHODIMP nsXPCTestParams::TestDOMString(const nsAString & a, nsAString & b, nsAString & _retval)
{
    STRING_METHOD_IMPL;
}



NS_IMETHODIMP nsXPCTestParams::TestAString(const nsAString & a, nsAString & b, nsAString & _retval)
{
    STRING_METHOD_IMPL;
}


NS_IMETHODIMP nsXPCTestParams::TestAUTF8String(const nsACString & a, nsACString & b, nsACString & _retval)
{
    STRING_METHOD_IMPL;
}


NS_IMETHODIMP nsXPCTestParams::TestACString(const nsACString & a, nsACString & b, nsACString & _retval)
{
    STRING_METHOD_IMPL;
}


NS_IMETHODIMP nsXPCTestParams::TestJsval(JS::Handle<JS::Value> a,
                                         JS::MutableHandle<JS::Value> b,
                                         JS::MutableHandle<JS::Value> _retval)
{
    _retval.set(b);
    b.set(a);
    return NS_OK;
}




NS_IMETHODIMP nsXPCTestParams::TestShortArray(uint32_t aLength, int16_t* a,
                                              uint32_t* bLength, int16_t** b,
                                              uint32_t* rvLength, int16_t** rv)
{
    BUFFER_METHOD_IMPL(int16_t, 0, TAKE_OWNERSHIP_NOOP);
}




NS_IMETHODIMP nsXPCTestParams::TestDoubleArray(uint32_t aLength, double* a,
                                               uint32_t* bLength, double** b,
                                               uint32_t* rvLength,  double** rv)
{
    BUFFER_METHOD_IMPL(double, 0, TAKE_OWNERSHIP_NOOP);
}




NS_IMETHODIMP nsXPCTestParams::TestStringArray(uint32_t aLength, const char * *a,
                                               uint32_t* bLength, char * **b,
                                               uint32_t* rvLength, char * **rv)
{
    BUFFER_METHOD_IMPL(char*, 0, TAKE_OWNERSHIP_STRING);
}




NS_IMETHODIMP nsXPCTestParams::TestWstringArray(uint32_t aLength, const char16_t * *a,
                                                uint32_t* bLength, char16_t * **b,
                                                uint32_t* rvLength, char16_t * **rv)
{
    BUFFER_METHOD_IMPL(char16_t*, 0, TAKE_OWNERSHIP_WSTRING);
}




NS_IMETHODIMP nsXPCTestParams::TestInterfaceArray(uint32_t aLength, nsIXPCTestInterfaceA** a,
                                                  uint32_t* bLength, nsIXPCTestInterfaceA * **b,
                                                  uint32_t* rvLength, nsIXPCTestInterfaceA * **rv)
{
    BUFFER_METHOD_IMPL(nsIXPCTestInterfaceA*, 0, TAKE_OWNERSHIP_INTERFACE);
}




NS_IMETHODIMP nsXPCTestParams::TestSizedString(uint32_t aLength, const char * a,
                                               uint32_t* bLength, char * *b,
                                               uint32_t* rvLength, char * *rv)
{
    BUFFER_METHOD_IMPL(char, 1, TAKE_OWNERSHIP_NOOP);
}




NS_IMETHODIMP nsXPCTestParams::TestSizedWstring(uint32_t aLength, const char16_t * a,
                                                uint32_t* bLength, char16_t * *b,
                                                uint32_t* rvLength, char16_t * *rv)
{
    BUFFER_METHOD_IMPL(char16_t, 1, TAKE_OWNERSHIP_NOOP);
}




NS_IMETHODIMP nsXPCTestParams::TestInterfaceIs(const nsIID* aIID, void* a,
                                               nsIID** bIID, void** b,
                                               nsIID** rvIID, void** rv)
{
    
    
    

    
    
    
    
    
    *rv = *b;

    
    
    *rvIID = static_cast<nsIID*>(moz_xmalloc(sizeof(nsID)));
    if (!*rvIID)
        return NS_ERROR_OUT_OF_MEMORY;
    **rvIID = **bIID;

    
    
    *b = a;
    static_cast<nsISupports*>(*b)->AddRef();

    
    **bIID = *aIID;

    return NS_OK;
}







NS_IMETHODIMP nsXPCTestParams::TestInterfaceIsArray(uint32_t aLength, const nsIID* aIID,
                                                    void** a,
                                                    uint32_t* bLength, nsIID** bIID,
                                                    void*** b,
                                                    uint32_t* rvLength, nsIID** rvIID,
                                                    void*** rv)
{
    
    
    *rvIID = static_cast<nsIID*>(moz_xmalloc(sizeof(nsID)));
    if (!*rvIID)
        return NS_ERROR_OUT_OF_MEMORY;
    **rvIID = **bIID;
    **bIID = *aIID;

    
    
    
    BUFFER_METHOD_IMPL(void*, 0, TAKE_OWNERSHIP_INTERFACE);
}


NS_IMETHODIMP nsXPCTestParams::TestOutAString(nsAString & o)
{
    o.AssignLiteral("out");
    return NS_OK;
}
