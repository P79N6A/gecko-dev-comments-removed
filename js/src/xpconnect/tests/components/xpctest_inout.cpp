







































#include "nsISupports.h"
#include "xpctest_inout.h"
#include "xpctest_private.h"

class xpcTestInOut : public nsIXPCTestInOut {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXPCTESTINOUT

  xpcTestInOut();
};

NS_IMPL_ISUPPORTS1(xpcTestInOut, nsIXPCTestInOut)

xpcTestInOut :: xpcTestInOut() {
    NS_ADDREF_THIS();
}

NS_IMETHODIMP xpcTestInOut :: EchoLong(PRInt32 li, PRInt32 *lo){ 
    return NS_OK; 
}
NS_IMETHODIMP xpcTestInOut :: EchoShort(PRInt16 si, PRInt16 *so){ 
    return NS_OK; 
}
NS_IMETHODIMP xpcTestInOut :: EchoChar(char ci, char *co){ 
    return NS_OK; 
}
NS_IMETHODIMP xpcTestInOut :: EchoBoolean(PRBool bi, PRBool *bo){ 
    return NS_OK; 
}
NS_IMETHODIMP xpcTestInOut :: EchoOctet(PRUint8 oi, PRUint8 *oo){ 
    return NS_OK; 
}
NS_IMETHODIMP xpcTestInOut :: EchoLongLong(PRInt64 lli, PRInt64 *llo){ 
    return NS_OK; 
}
NS_IMETHODIMP xpcTestInOut :: EchoUnsignedShort(PRUint16 usi, PRUint16 *uso){ 
    return NS_OK; 
}
NS_IMETHODIMP xpcTestInOut :: EchoUnsignedLong(PRUint32 uli, PRUint32 *ulo){ 
    return NS_OK; 
}
NS_IMETHODIMP xpcTestInOut :: EchoFloat(float fi, float *fo){ 
    return NS_OK; 
}
NS_IMETHODIMP xpcTestInOut :: EchoDouble(double di, double *dout){ 
    return NS_OK; 
}
NS_IMETHODIMP xpcTestInOut :: EchoWchar(PRUnichar wci, PRUnichar *wco){ 
    return NS_OK; 
}
NS_IMETHODIMP xpcTestInOut :: EchoString(const PRUnichar *wsi, PRUnichar **wso){ 
    return NS_OK; 
}
NS_IMETHODIMP xpcTestInOut :: EchoPRBool(PRBool bi, PRBool *bo){ 
    return NS_OK; 
}
NS_IMETHODIMP xpcTestInOut :: EchoPRInt32(PRInt32 li, PRInt32 *lo){ 
    return NS_OK; 
}
NS_IMETHODIMP xpcTestInOut :: EchoPRInt16(PRInt16 li, PRInt16 *lo){ 
    return NS_OK; 
}
NS_IMETHODIMP xpcTestInOut :: EchoPRInt64(PRInt64 ii, PRInt64 *io){ 
    return NS_OK; 
}
NS_IMETHODIMP xpcTestInOut :: EchoPRUint8(PRUint8 ii, PRUint8 *io){ 
    return NS_OK; 
}
NS_IMETHODIMP xpcTestInOut :: EchoPRUint16(PRUint16 ii, PRUint16 *io){ 
    return NS_OK; 
}
NS_IMETHODIMP xpcTestInOut :: EchoPRUint32(PRUint32 ii, PRUint32 *io){ 
    return NS_OK; 
}
NS_IMETHODIMP xpcTestInOut :: EchoPRUint32_2(PRUint32 ii, PRUint32 *io){ 
    return NS_OK; 
}
NS_IMETHODIMP xpcTestInOut :: EchoPRUint64(PRUint64 ii, PRUint64 *io){ 
    return NS_OK; 
}
NS_IMETHODIMP xpcTestInOut :: EchoVoidPtr(void * vsi, void * *vso){ 
    return NS_OK; 
}
NS_IMETHODIMP xpcTestInOut :: EchoCharPtr(char * csi, char * *cso){ 
    return NS_OK; 
}








NS_IMETHODIMP xpcTestInOut :: EchoNsIDPtr(const nsID * pi, nsID * *po){ 
    return NS_OK; 
}
NS_IMETHODIMP xpcTestInOut :: EchoNsIIDPtr(const nsIID * pi, nsIID * *po){ 
    return NS_OK; 
}
NS_IMETHODIMP xpcTestInOut :: EchoNsCIDPtr(const nsCID * pi, nsCID * *po){ 
    return NS_OK; 
}
NS_IMETHODIMP xpcTestInOut :: EchoNsQIResult(void * ri, void * *ro){ 
    return NS_OK; 
}
NS_IMETHODIMP xpcTestInOut :: EchoVoid(void) {
    return NS_OK;
}

NS_IMETHODIMP
xpctest::ConstructXPCTestInOut(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    nsresult rv;
    NS_ASSERTION(aOuter == nsnull, "no aggregation");
    xpcTestInOut *obj = new xpcTestInOut();

    if(obj)
    {
        rv = obj->QueryInterface(aIID, aResult);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to find correct interface");
        NS_RELEASE(obj);
    }
    else
    {
        *aResult = nsnull;
        rv = NS_ERROR_OUT_OF_MEMORY;
    }
    return rv;
}
