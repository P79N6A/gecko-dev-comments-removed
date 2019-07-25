



































#include "xpctest_private.h"

NS_IMPL_ISUPPORTS1(nsXPCTestParams, nsIXPCTestParams)

nsXPCTestParams::nsXPCTestParams()
{
}

nsXPCTestParams::~nsXPCTestParams()
{
}

#define GENERIC_METHOD_IMPL { \
    *_retval = *b; \
    *b = a; \
    return NS_OK; \
}

#define STRING_METHOD_IMPL { \
    _retval.Assign(b); \
    b.Assign(a); \
    return NS_OK; \
}


NS_IMETHODIMP nsXPCTestParams::TestBoolean(PRBool a, PRBool *b NS_INOUTPARAM, PRBool *_retval NS_OUTPARAM)
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
