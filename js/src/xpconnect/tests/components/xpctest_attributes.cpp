







































#include "xpctest_attributes.h"
#include "nsISupports.h"
#include "xpctest_private.h"

#define NS_IXPCTESTOBJECTREADONLY_IID \
  {0x1364941e, 0x4462, 0x11d3, \
    { 0x82, 0xee, 0x00, 0x60, 0xb0, 0xeb, 0x59, 0x6f }}

class xpcTestObjectReadOnly : public nsIXPCTestObjectReadOnly {
 public: 
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXPCTESTOBJECTREADONLY
  xpcTestObjectReadOnly();
   
 private:
    PRBool  boolProperty;
    PRInt16 shortProperty;
    PRInt32 longProperty;
    float   floatProperty;
    char    charProperty;
    char    *stringID;
};

NS_IMPL_ISUPPORTS1(xpcTestObjectReadOnly, nsIXPCTestObjectReadOnly)

xpcTestObjectReadOnly :: xpcTestObjectReadOnly() {
    NS_ADDREF_THIS();

    boolProperty = PR_TRUE;
    shortProperty = 32767;
    longProperty =  2147483647;
    charProperty = 'X';

    const char _id[] = "a68cc6a6-6552-11d3-82ef-0060b0eb596f";
    stringID = (char*) nsMemory::Clone(_id, sizeof(char)*(strlen(_id)+1));
}

NS_IMETHODIMP xpcTestObjectReadOnly :: GetID(char **_retval) {
    *_retval= (char*) nsMemory::Clone(stringID, 
                                         sizeof(char)*(strlen(stringID)+1));
    return *_retval? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP xpcTestObjectReadOnly :: GetStrReadOnly(char * *aStrReadOnly){
    char aString[] = "XPConnect Read-Only String";

    if(!aStrReadOnly)
        return NS_ERROR_NULL_POINTER;
    *aStrReadOnly = (char*) nsMemory::Clone(aStrReadOnly, 
                                               sizeof(char)*(strlen(aString)+1));
    return *aStrReadOnly ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP xpcTestObjectReadOnly :: GetBoolReadOnly(PRBool *aBoolReadOnly) {
    *aBoolReadOnly = boolProperty;
    return NS_OK;
}
NS_IMETHODIMP xpcTestObjectReadOnly :: GetShortReadOnly(PRInt16 *aShortReadOnly){
    *aShortReadOnly = shortProperty;
    return NS_OK;
}
NS_IMETHODIMP xpcTestObjectReadOnly :: GetLongReadOnly(PRInt32 *aLongReadOnly){
    *aLongReadOnly = longProperty;
    return NS_OK;
}
NS_IMETHODIMP xpcTestObjectReadOnly :: GetFloatReadOnly(float *aFloatReadOnly){
    *aFloatReadOnly = floatProperty;
    return NS_OK;
}
NS_IMETHODIMP xpcTestObjectReadOnly :: GetCharReadOnly(char *aCharReadOnly){
    *aCharReadOnly = charProperty;
    return NS_OK;
}
NS_IMETHODIMP
xpctest::ConstructXPCTestObjectReadOnly(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    nsresult rv;
    NS_ASSERTION(aOuter == nsnull, "no aggregation");
    xpcTestObjectReadOnly *obj = new xpcTestObjectReadOnly();

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







#define NS_IXPCTESTOBJECTREADWRITE_IID \
  {0x3b9b1d38, 0x491a, 0x11d3, \
    { 0x82, 0xef, 0x00, 0x60, 0xb0, 0xeb, 0x59, 0x6f }}

class xpcTestObjectReadWrite : public nsIXPCTestObjectReadWrite {
  public: 
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXPCTESTOBJECTREADWRITE
  
  xpcTestObjectReadWrite();

 private:
     PRBool boolProperty;
     PRInt16 shortProperty;
     PRInt32 longProperty;
     float floatProperty;
     char charProperty;
     const char *stringProperty;
};

NS_IMPL_ISUPPORTS1(xpcTestObjectReadWrite, nsIXPCTestObjectReadWrite)

xpcTestObjectReadWrite :: xpcTestObjectReadWrite() {
    NS_ADDREF_THIS();


    boolProperty = PR_TRUE;
    shortProperty = 32767;
    longProperty =  2147483647;
    charProperty = 'X';

    const char s[] = "XPConnect Read-Writable String";
    stringProperty = (char*) nsMemory::Clone(s, 
                                                sizeof(char)*(strlen(s)+1));
}

NS_IMETHODIMP xpcTestObjectReadWrite :: GetStringProperty(char * *aStringProperty) {
    if(!aStringProperty)
        return NS_ERROR_NULL_POINTER;
    *aStringProperty = (char*) nsMemory::Clone(stringProperty, 
                                               sizeof(char)*(strlen(stringProperty)+1));
    return *aStringProperty ? NS_OK : NS_ERROR_OUT_OF_MEMORY;

}
NS_IMETHODIMP xpcTestObjectReadWrite :: SetStringProperty(const char * aStringProperty) {
    stringProperty = aStringProperty;
    return NS_OK;
}

NS_IMETHODIMP xpcTestObjectReadWrite :: GetBooleanProperty(PRBool *aBooleanProperty) {
    *aBooleanProperty = boolProperty;
    return NS_OK;
}
NS_IMETHODIMP xpcTestObjectReadWrite :: SetBooleanProperty(PRBool aBooleanProperty) {
    boolProperty = aBooleanProperty;
    return NS_OK;
}
NS_IMETHODIMP xpcTestObjectReadWrite :: GetShortProperty(PRInt16 *aShortProperty) {
    *aShortProperty = shortProperty;
    return NS_OK;
}
NS_IMETHODIMP xpcTestObjectReadWrite :: SetShortProperty(PRInt16 aShortProperty) {
    shortProperty = aShortProperty;
    return NS_OK;
}
NS_IMETHODIMP xpcTestObjectReadWrite :: GetLongProperty(PRInt32 *aLongProperty) {
    *aLongProperty = longProperty;
    return NS_OK;
}
NS_IMETHODIMP xpcTestObjectReadWrite :: SetLongProperty(PRInt32 aLongProperty) {
    longProperty = aLongProperty;
    return NS_OK;
}
NS_IMETHODIMP xpcTestObjectReadWrite :: GetFloatProperty(float *aFloatProperty) {
    *aFloatProperty = floatProperty;
    return NS_OK;
}
NS_IMETHODIMP xpcTestObjectReadWrite :: SetFloatProperty(float aFloatProperty) {
    floatProperty = aFloatProperty;
    return NS_OK;
}
NS_IMETHODIMP xpcTestObjectReadWrite :: GetCharProperty(char *aCharProperty) {
    *aCharProperty = charProperty;
    return NS_OK;
}
NS_IMETHODIMP xpcTestObjectReadWrite :: SetCharProperty(char aCharProperty) {
    charProperty = aCharProperty;
    return NS_OK;
}
NS_IMETHODIMP
xpctest::ConstructXPCTestObjectReadWrite(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    nsresult rv;
    NS_ASSERTION(aOuter == nsnull, "no aggregation");
    xpcTestObjectReadWrite *obj = new xpcTestObjectReadWrite();

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























































